
#include "ue2common.h"
#include "grey.h"
#include "hs_pcre_mnrl.h"
#include "hs_internal.h"
#include "hs_pcre_mnrl_int.h"
#include "compiler/compiler.h"
#include "compiler/error.h"
#include "nfagraph/ng.h"
#include "nfagraph/ng_expr_info.h"
#include "nfagraph/ng_extparam.h"
#include "nfagraph/ng_util.h"
#include "parser/parse_error.h"
#include "parser/unsupported.h"
#include "parser/Parser.h"
#include "parser/prefilter.h"
#include "util/compile_error.h"
#include "util/dump_charclass.h"

#include <memory>
#include <iostream>
#include <fstream>
#include <sstream>
#include <mnrl.hpp>

#include <cassert>
#include <cstddef>
#include <cstring>
#include <limits.h>
#include <string>
#include <vector>

using namespace std;
using namespace MNRL;

namespace ue2 {
    
    // borrowed from parser/compiler.cpp
    static
    void optimise(ParsedExpression &expr) {
        if (expr.min_length || expr.som) {
            return;
        }
    
        DEBUG_PRINTF("optimising\n");
        expr.component->optimise(true /* root is connected to sds */);
    }
    
    static
    MNRLDefs::EnableType convert_enable(u32 idx) {
        switch(idx) {
            case NODE_START:
                return MNRLDefs::EnableType::ENABLE_ON_START_AND_ACTIVATE_IN;
            case NODE_START_DOTSTAR:
                return MNRLDefs::EnableType::ENABLE_ALWAYS;
            case NODE_ACCEPT_EOD:
                return MNRLDefs::EnableType::ENABLE_ON_LAST;
            default:
                return MNRLDefs::EnableType::ENABLE_ON_ACTIVATE_IN;
        }
    }
    
    static
    void addExpressionMNRL(NG &ng, MNRLNetwork &mnrl, unsigned index, const char *expression,
        unsigned flags, const hs_expr_ext *ext, ReportID id) {
        assert(expression);
        const CompileContext &cc = ng.cc;
        DEBUG_PRINTF("index=%u, id=%u, flags=%u, expr='%s'\n", index, id, flags,
                         expression);
        // Ensure that our pattern isn't too long (in characters).
        if (strlen(expression) > cc.grey.limitPatternLength) {
            throw CompileError("Pattern length exceeds limit.");
        }
        // Do per-expression processing: errors here will result in an exception
        // being thrown up to our caller
        ParsedExpression expr(index, expression, flags, id, ext);
        //dumpExpression(expr, "orig", cc.grey);
        // Apply prefiltering transformations if desired.
        if (expr.prefilter) {
            prefilterTree(expr.component, ParseMode(flags));
            //dumpExpression(expr, "prefiltered", cc.grey);
        }
        // Expressions containing zero-width assertions and other extended pcre
        // types aren't supported yet. This call will throw a ParseError exception
        // if the component tree contains such a construct.
        checkUnsupported(*expr.component);
            expr.component->checkEmbeddedStartAnchor(true);
            expr.component->checkEmbeddedEndAnchor(true);
        if (cc.grey.optimiseComponentTree) {
            optimise(expr);
            //dumpExpression(expr, "opt", cc.grey);
        }
        
        DEBUG_PRINTF("component=%p, nfaId=%u, reportId=%u\n",
                 expr.component.get(), expr.index, expr.id);
        
        
        unique_ptr<NGWrapper> graph_ptr = buildWrapper(ng.rm, cc, expr);
        
        if (!graph_ptr) {
            DEBUG_PRINTF("NFA build failed on ID %u, but no exception was "
            "thrown.\n", expr.id);
            throw CompileError("Internal error.");
        }
        if (!expr.allow_vacuous && matches_everywhere(*graph_ptr)) {
            throw CompileError("Pattern matches empty buffer; use "
            "HS_FLAG_ALLOWEMPTY to enable support.");
        }
        
        NGWrapper &g = *graph_ptr;
        
        // Okay we have a graph right here
        // What can we do with it?
        
        
        
        vector<string> start_nodes;
        vector<string> report_nodes;
        vector<string> eod_nodes;
        
        bool start_to_report = false;
        
        // add all vertices to MNRL network
        for(const auto &v : vertices_range(g)) {
            
            CharReach reach = g[v].char_reach;
            u32 idx = g[v].index;
            
            MNRLDefs::EnableType node_mode = MNRLDefs::ENABLE_ON_ACTIVATE_IN;
            
            ostringstream node_id;
            node_id << expr.id << "_" << idx;
            
            if (is_special(v,g)) {
                switch (idx) {
                    case NODE_START:
                        break;
                    case NODE_START_DOTSTAR:
                        break;
                    case NODE_ACCEPT:                        
                        break;
                    case NODE_ACCEPT_EOD:
                        break;
                    default:
                        DEBUG_PRINTF("Unknown special element in ID %u.\n", expr.id);
                        throw CompileError("Error compiling expression.");
                }
                
                // we generally don't add these to the network
                continue;
            }
            
            // add the node
            mnrl.addHState(
                describeClass(reach, 8),
                node_mode,
                node_id.str(),
                false,
                expr.id,
                false
            );
        }
        
        // iterate through all edges
        for (const auto &e : edges_range(g)) {
            NFAVertex src = source(e,g);
            NFAVertex dst = target(e,g);
            
            u32 src_idx = g[src].index;
            u32 dst_idx = g[dst].index;
               
            switch(src_idx) {
                case NODE_START:
                case NODE_START_DOTSTAR:
                    // make sure that the dest isn't a special element
                    if(!is_special(dst,g)) {
                        //just make the node a start
                        ostringstream mnrl_id;
                        mnrl_id << expr.id << "_" << dst_idx;
                        
                        // get the node
                        shared_ptr<MNRLNode> n = mnrl.getNodeById(mnrl_id.str());
                        
                        n->setEnable(convert_enable(src_idx));
                        
                        // let's save this id in case we need to deal
                        // with a start-->report edge
                        start_nodes.push_back(mnrl_id.str());
                    } else {
                       // FIXME do we need to handle anything here?
                       // FIXME handle start-->report
                       // just set a flag that we have seen a strt-->rpt
                       switch(dst_idx) {
                            case NODE_ACCEPT:
                            case NODE_ACCEPT_EOD:
                                start_to_report = true;
                                break;
                            case NODE_START:
                            case NODE_START_DOTSTAR:
                                // don't do anything
                                break;
                            default:
                                // something strange; quit
                                DEBUG_PRINTF("Found an edge to special start as dest in ID %u: %u -> %u.\n", expr.id, src_idx, dst_idx);
                                throw CompileError("Error compiling expression.");
                       }
                    }
                    // that's all we do with this edge
                    continue;
                
                case NODE_ACCEPT:
                case NODE_ACCEPT_EOD:
                    
                    // do nothing?
                    break;
                
                default:
                    // the src is just a standard vertex
                    // check the dest
                    
                    switch (dst_idx) {
                        case NODE_ACCEPT:
                        case NODE_ACCEPT_EOD: {
                            
                            // we need to report
                            ostringstream mnrl_id;
                            mnrl_id << expr.id << "_" << src_idx;
                            
                            // get the node
                            shared_ptr<MNRLNode> n = mnrl.getNodeById(mnrl_id.str());
                            
                            // deal with EOD
                            n->setEnable(convert_enable(dst_idx));
                            
                            n->setReport(true);
                            
                            // that's all we need to do with this edge
                            continue;
                        }
                        
                        case NODE_START:
                        case NODE_START_DOTSTAR: 
                            // do nothing
                            break;
                        default: {
                            // this is just a standard edge, so add it!
                            ostringstream s_id, d_id;
                            s_id << expr.id << "_" << src_idx;
                            d_id << expr.id << "_" << dst_idx;
                            mnrl.addConnection(
                                s_id.str(), // src id
                                MNRLDefs::H_STATE_OUTPUT, // src port
                                d_id.str(), // dst id
                                MNRLDefs::H_STATE_INPUT // dst port
                            );
                        }
                    }
            }
        }
        
        if(start_to_report) {
            // go through and set all starts to report
            for(const auto id : start_nodes) {
                shared_ptr<MNRLNode> n = mnrl.getNodeById(id);
                n->setReport(true);
            }
        }
    }
    
    hs_error_t
    hs_pcre_mnrl_multi_int(const char *const *expressions, MNRLNetwork &mnrl, const unsigned *flags,
                            const unsigned *ids, unsigned elements,
                            hs_compile_error_t **comp_error, const Grey &g) {
        
        // Check the args: note that it's OK for flags, ids or ext to be null.
        if (!comp_error) {
            // nowhere to write the string, but we can still report an error code
            return HS_COMPILER_ERROR;
        }
        if (!expressions) {
            *comp_error
                = generateCompileError("Invalid parameter: expressions is NULL",
                                       -1);
            return HS_COMPILER_ERROR;
        }
        if (elements == 0) {
            *comp_error = generateCompileError("Invalid parameter: elements is zero", -1);
            return HS_COMPILER_ERROR;
        }

        if (elements > g.limitPatternCount) {
            *comp_error = generateCompileError("Number of patterns too large", -1);
            return HS_COMPILER_ERROR;
        }
        
        // create the compile context
        CompileContext cc(false, false, get_current_target(), g);
        NG ng(cc, elements, 0);
        
        try {
            
            for (unsigned int i = 0; i < elements; i++) {
                // Add this expression to the compiler
                try {
                    addExpressionMNRL(ng, mnrl, i, expressions[i], flags ? flags[i] : 0,
                                  nullptr, ids ? ids[i] : 0);
                } catch (CompileError &e) {
                    /* Caught a parse error:
                     * throw it upstream as a CompileError with a specific index */
                    e.setExpressionIndex(i);
                    throw; /* do not slice */
                }
            }
            
            return HS_SUCCESS;
        }
        catch (const CompileError &e) {
            // Compiler error occurred
            *comp_error = generateCompileError(e.reason,
                                               e.hasIndex ? (int)e.index : -1);
            return HS_COMPILER_ERROR;
        }
        catch (std::bad_alloc) {
            *comp_error = const_cast<hs_compile_error_t *>(&hs_enomem);
            return HS_COMPILER_ERROR;
        }
        catch (...) {
            assert(!"Internal error, unexpected exception");
            *comp_error = const_cast<hs_compile_error_t *>(&hs_einternal);
            return HS_COMPILER_ERROR;
        }
    }
    
    
} // namespace

// public
hs_error_t hs_pcre_mnrl_multi(const char * const *expressions,
                                 MNRLNetwork &mnrl,
                                 const unsigned *flags, const unsigned *ids,
                                 unsigned elements,
                                 hs_compile_error_t **error) {
    return ue2::hs_pcre_mnrl_multi_int(expressions, mnrl, flags, ids, elements, error, ue2::Grey());
}
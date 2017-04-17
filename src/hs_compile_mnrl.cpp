
#include "ht.h"
#include "allocator.h"
#include "ue2common.h"
#include "grey.h"
#include "hs_compile_mnrl.h"
#include "hs_internal.h"
#include "hs_compile_mnrl_int.h"
#include "database.h"
#include "parse_symbol_set.h"
#include "compiler/compiler.h"
#include "compiler/error.h"
#include "nfagraph/ng.h"
#include "nfagraph/ng_expr_info.h"
#include "nfagraph/ng_extparam.h"
#include "parser/parse_error.h"
#include "parser/Parser.h"
#include "parser/prefilter.h"
#include "util/compile_error.h"
#include "util/cpuid_flags.h"
#include "util/depth.h"
#include "util/popcount.h"
#include "util/target_info.h"
#include "util/make_unique.h"
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
    
    hs_error_t
    hs_compile_mnrl_int(const char * graphFN,
            hs_database_t **db,
            hs_compile_error_t **comp_error,
            r_map **report_map,
            const Grey &g) {
        // Check the args: note that it's OK for flags, ids or ext to be null.
        if (!comp_error) {
            if (db) {
                *db = nullptr;
            }
            // nowhere to write the string, but we can still report an error code
            return HS_COMPILER_ERROR;
        }
    
        if (!db) {
            *comp_error = generateCompileError("Invalid parameter: db is NULL", -1);
            return HS_COMPILER_ERROR;
        }
    
        // Setup
        // KAA Hardcoding
        unsigned somPrecision = 0;
        target_t target_info = get_current_target();
        CompileContext cc(false, false, target_info, g);
        NG ng(cc, 1, somPrecision);
    
        // Build graph
        try {
    
            //
            unordered_map<string, NFAVertex> vertices;
    
            unsigned int exp_ind = 0;
            bool highlander_in = false;
            bool utf_8 = false;
            bool prefilter_in = false;
            som_type som_in = SOM_NONE;
            ReportID gid = 1;
            u64a min_offset_in = 0;
            u64a max_offset_in = MAX_OFFSET;
            u64a min_length_in = 0;
    
            // NFA CONSTRUCTION CODE
            // construct NGWrapper object
            auto graph_ptr = make_unique<NGWrapper>(exp_ind, // unsigned int expression index
                    highlander_in, // bool highlander_in
                    utf_8, // bool utf8
                    prefilter_in, // bool prefilter_in
                    som_in, // som_type som_in
                    gid, // ReportID r
                    min_offset_in, // u64a min_offset_in
                    max_offset_in, // u64a max_offset_in
                    min_length_in); //min_length_in
    
    
            NGWrapper &graph = *graph_ptr;
    
            // initialize dummy report ID index
            unsigned int report_id_int = 0;
    
            // MNRL read in file name
            // wrap with try/catch later maybe?
            shared_ptr<MNRLNetwork> mnrl_graph = loadMNRL(graphFN);
    
            map<string, shared_ptr<MNRLNode>> mnrl_nodes = mnrl_graph->getNodes();
            
            // keep track of report id's
            // FIXME need to decide about merging
            // map<string, unsigned int> report_id_mapping = map<string, unsigned int>();
            
            // add nodes
            for(auto n : mnrl_nodes){
    
                string node_id = n.first;
                if(n.second->getNodeType() != MNRLDefs::NodeType::HSTATE) {
                    cout << "found node that wasn't hState: " << node_id << endl;
                    exit(1);
                }
                // can do a cast to a MNRLHState
                shared_ptr<MNRLNode> node_tmp = n.second;
                shared_ptr<MNRLHState> node = dynamic_pointer_cast<MNRLHState>(node_tmp);
    
                // generate vertex
                NFAVertex tmp = add_vertex(graph);
                vertices[node_id] = tmp;
    
                // add charset
                graph[tmp].char_reach = CharReach();
                graph[tmp].char_reach.clear();
                
                parseSymbolSet(graph[tmp].char_reach, node->getSymbolSet());
    
                // handle starts
                MNRLDefs::EnableType start_type = node->getEnable();
                switch(start_type) {
                case MNRLDefs::ENABLE_ALWAYS :
                    add_edge(graph.startDs, tmp, graph);
                    break;
                case MNRLDefs::ENABLE_ON_START_AND_ACTIVATE_IN :
                    add_edge(graph.start, tmp, graph);
                    break;
                default:
                    // do nothing
                    // there is "ENABLE ON LAST" which ANMLZoo never uses but MNRL/ANML supports
                    break;
                };
    
                // report
                if(node->getReport()) {
                    add_edge(tmp, graph.accept, graph);
                    
                    // figure out if this has a report code or not
                    shared_ptr<MNRLReportId> rid = node->getReportId();
                    string mnrl_rid = rid->toString();
                    string mnrl_id = node->getId();
                    
                    unsigned int hs_report_id;
                    map<string, unsigned int>::iterator it;
                    
                    
                    /* FIXME how do we handle reportId's?
                             * Do thy get merged or no?
                    switch(rid->get_type()) {
                        case MNRLDefs::ReportIdType::INT:
                        case MNRLDefs::ReportIdType::STRING:
                            mnrl_rid = rid->toString();
                            
                            it = report_id_mapping.find(mnrl_rid);
                            
                            if(it != report_id_mapping.end()) {
                                // there was an entry
                                hs_report_id = it->second;
                            } else {
                                // not found, so add
                                hs_report_id = report_id_int++;
                                report_id_mapping.insert(map<string, unsigned int>::value_type(mnrl_rid, hs_report_id));
                            }
                            hs_report_id = report_id_int++;
                            break;
                        default:
                            // no report ID
                            hs_report_id = report_id_int++;
                            mnrl_rid = node->getId();
                            break;
                    }
                    */
                    hs_report_id = report_id_int++;
                    
                    // store this in our map
                    insert_mapping(hs_report_id, mnrl_id.c_str(), mnrl_rid.c_str(), report_map);
                    
                    
                    // For now just register a dummy report code for all reports
                    Report report(EXTERNAL_CALLBACK, hs_report_id);
                    ReportID report_id = ng.rm.getInternalId(report); // register with report manager
                    
                    // add report id
                    // should get HState report id, for now, just use our dummy report id
                    graph[tmp].reports.insert(report_id);
                }
    
            }
    
            // add edges
            for(auto n : mnrl_nodes){
    
                string node_id = n.first;
                shared_ptr<MNRLNode> node = n.second;
    
                // get hs_vertex
                NFAVertex hs_vertex = vertices[node_id];
    
                for(auto to : *(node->getOutputConnections())){
                    //ask port pointer to get connections
                    shared_ptr<MNRLPort> out_port = to.second;
                    //for each connection in the port
                    for(auto sink : out_port->getConnections()) {
                        shared_ptr<MNRLNode> sinkNode = sink.first;
                        add_edge(vertices[node_id], vertices[sinkNode->getId()], graph);
                    }
                }
    
            }
    
            // add graph
            printf("Attempting to add graph...\n");
            if (!ng.addGraph(graph)) {
                DEBUG_PRINTF("NFA addGraph failed.\n");
                throw CompileError("Error compiling expression.");
            }
            printf("     Added graph.\n");
            // END NFA CONSTRUCTION CODE
            
            printf("Building database...\n");
            // Build database using graph
            unsigned length = 0;
            struct hs_database *out = build(ng, &length);
            assert(out);    // should have thrown exception on error
            assert(length);
            
            printf("     Built database.\n");
    
            *db = out;
            *comp_error = nullptr;
            
            return HS_SUCCESS;
        }
        catch (const CompileError &e) {
            // Compiler error occurred
            printf("COMPILER ERROR!\n");
            *db = nullptr;
            *comp_error = generateCompileError(e.reason,
                    e.hasIndex ? (int)e.index : -1);
            return HS_COMPILER_ERROR;
        }
        catch (std::bad_alloc) {
            printf("BAD ALLOC!\n");
            *db = nullptr;
            *comp_error = const_cast<hs_compile_error_t *>(&hs_enomem);
            return HS_COMPILER_ERROR;
        } catch (MNRL::MNRLError::MNRLError &e) {
            printf("MNRL ERROR!\n");
            *comp_error = generateCompileError(e.what(), -1);
            *db = nullptr;
            return HS_COMPILER_ERROR;
        }
        catch (...) {
            printf("OTHER ERROR!\n");
            assert(!"Internal error, unexpected exception");
            *db = nullptr;
            *comp_error = const_cast<hs_compile_error_t *>(&hs_einternal);
            return HS_COMPILER_ERROR;
        }
    }  

};

hs_error_t hs_compile_mnrl(const char * graphFN, hs_database_t **db,
            hs_compile_error_t **error, r_map **report_map) {
    
        return ue2::hs_compile_mnrl_int(graphFN, db, error, report_map, ue2::Grey());
    }
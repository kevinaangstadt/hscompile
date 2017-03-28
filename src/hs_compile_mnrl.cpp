
#include "ht.h"
#include "allocator.h"
#include "ue2common.h"
#include "grey.h"
#include "hs_compile_mnrl.h"
#include "hs_internal.h"
#include "hs_compile_mnrl_int.h"
#include "database.h"
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

    // KAA adding from VASim for right now...there has to be a better way
    static
    void parseSymbolSet(CharReach &column, std::string symbol_set) {
    
        if(symbol_set.compare("*") == 0){
            column.setall();
            return;
        }
    
        // KAA found that apcompile parses symbol-set="." to mean "^\x0a"
        // hard-coding this here
        if(symbol_set.compare(".") == 0) {
            column.set('\n');
            column.flip();
            return;
        }
    
        bool in_charset = false;
        bool escaped = false;
        bool inverting = false;
        bool range_set = false;
        int bracket_sem = 0;
        int brace_sem = 0;
        const unsigned int value = 1;
        unsigned char last_char = 0;
        unsigned char range_start = 0;
    
        // handle symbol sets that start and end with curly braces {###}
        if((symbol_set[0] == '{') &&
                (symbol_set[symbol_set.size() - 1] == '}')){
    
            std::cout << "CURLY BRACES NOT IMPLEMENTED" << std::endl;
            exit(1);
        }
    
        int index = 0;
        while(index < symbol_set.size()) {
    
            unsigned char c = symbol_set[index];
    
            //std::cout << "PROCESSING CHAR: " << c << std::endl;
    
            switch(c){
    
            // Brackets
            case '[' :
                if(escaped){
                    column.set(c);
                    if(range_set){
                        column.setRange(range_start,c);
                        range_set = false;
                    }
                    last_char = c;
                    escaped = false;
                }else{
                    bracket_sem++;
                }
                break;
            case ']' :
                if(escaped){
                    column.set(c);
                    if(range_set){
                        column.setRange(range_start,c);
                        range_set = false;
                    }
    
                    last_char = c;
                }else{
                    bracket_sem--;
                }
    
                break;
    
                // Braces
            case '{' :
    
                //if(escaped){
                column.set(c);
                if(range_set){
                    column.setRange(range_start,c);
                    range_set = false;
                }
    
                last_char = c;
                //escaped = false;
                //}else{
                    //brace_sem++;
                    //}
                break;
            case '}' :
                //if(escaped){
                column.set(c);
                if(range_set){
                    column.setRange(range_start,c);
                    range_set = false;
                }
                last_char = c;
                //escaped = false;
                //}else{
                    //brace_sem--;
                    //}
                break;
    
                //escape
            case '\\' :
                if(escaped){
                    column.set(c);
                    if(range_set){
                        column.setRange(range_start,c);
                        range_set = false;
                    }
    
                    last_char = c;
                    escaped = false;
                }else{
                    escaped = true;
                }
                break;
    
                // escaped chars
            case 'n' :
                if(escaped){
                    column.set('\n');
                    if(range_set){
                        column.setRange(range_start,'\n');
                        range_set = false;
                    }
                    last_char = '\n';
                    escaped = false;
                }else{
                    column.set(c);
                    if(range_set){
                        column.setRange(range_start,c);
                        range_set = false;
                    }
                    last_char = c;
                }
                break;
            case 'r' :
                if(escaped){
                    column.set('\r');
                    if(range_set){
                        column.setRange(range_start,'\r');
                        range_set = false;
                    }
                    last_char = '\r';
                    escaped = false;
                }else{
                    column.set(c);
                    if(range_set){
                        column.setRange(range_start,c);
                        range_set = false;
                    }
                    last_char = c;
                }
                break;
            case 't' :
                if(escaped){
                    column.set('\t');
                    if(range_set){
                        column.setRange(range_start,'\t');
                        range_set = false;
                    }
                    last_char = '\t';
                    escaped = false;
                }else{
                    column.set(c);
                    if(range_set){
                        column.setRange(range_start,c);
                        range_set = false;
                    }
                    last_char = c;
                }
                break;
            case 'a' :
                if(escaped){
                    column.set('\a');
                    if(range_set){
                        column.setRange(range_start,'\a');
                        range_set = false;
                    }
                    last_char = '\a';
                    escaped = false;
                }else{
                    column.set(c);
                    if(range_set){
                        column.setRange(range_start,c);
                        range_set = false;
                    }
                    last_char = c;
                }
                break;
            case 'b' :
                if(escaped){
                    column.set('\b');
                    if(range_set){
                        column.setRange(range_start,'\b');
                        range_set = false;
                    }
                    last_char = '\b';
                    escaped = false;
                }else{
                    column.set(c);
                    if(range_set){
                        //std::cout << "RANGE SET" << std::endl;
                        column.setRange(range_start,c);
                        range_set = false;
                    }
                    last_char = c;
                }
                break;
            case 'f' :
                if(escaped){
                    column.set('\f');
                    if(range_set){
                        column.setRange(range_start,'\f');
                        range_set = false;
                    }
                    last_char = '\f';
                    escaped = false;
                }else{
                    column.set(c);
                    if(range_set){
                        column.setRange(range_start,c);
                        range_set = false;
                    }
                    last_char = c;
                }
                break;
            case 'v' :
                if(escaped){
                    column.set('\v');
                    if(range_set){
                        column.setRange(range_start,'\v');
                        range_set = false;
                    }
                    last_char = '\v';
                    escaped = false;
                }else{
                    column.set(c);
                    if(range_set){
                        column.setRange(range_start,c);
                        range_set = false;
                    }
                    last_char = c;
                }
                break;
            case '\'' :
                if(escaped){
                    column.set('\'');
                    if(range_set){
                        column.setRange(range_start,'\'');
                        range_set = false;
                    }
                    last_char = '\'';
                    escaped = false;
                }else{
                    column.set(c);
                    if(range_set){
                        column.setRange(range_start,c);
                        range_set = false;
                    }
                    last_char = c;
                }
                break;
            case '\"' :
                if(escaped){
                    column.set('\"');
                    if(range_set){
                        column.setRange(range_start,'\"');
                        range_set = false;
                    }
                    last_char = '\"';
                    escaped = false;
                }else{
                    column.set(c);
                    if(range_set){
                        column.setRange(range_start,c);
                        range_set = false;
                    }
                    last_char = c;
                }
                break;
                /*
                         case '?' :
                         if(escaped){
                         column.set('?',value);
                         last_char = '?';
                         escaped = false;
                         }else{
                         column.set(c, value);
                         last_char = c;
                         }
                         break;
                 */
                // Range
            case '-' :
                if(escaped){
                    column.set('-');
                    if(range_set){
                        column.setRange(range_start,'-');
                        range_set = false;
                    }
                    last_char = '-';
                }else{
                    range_set = true;
                    range_start = last_char;
                }
                break;
    
                // Special Classes
            case 's' :
                if(escaped){
                    column.set('\n');
                    column.set('\t');
                    column.set('\r');
                    column.set('\x0B'); //vertical tab
                    column.set('\x0C');
                    column.set('\x20');
                    escaped = false;
                }else{
                    column.set(c);
                    if(range_set){
                        column.setRange(range_start,c);
                        range_set = false;
                    }
                    last_char = c;
                }
                break;
    
            case 'd' :
                if(escaped){
                    column.setRange(48,57);
                    //setRange(column,48,57, value);
                    escaped = false;
                }else{
                    column.set(c);
                    if(range_set){
                        column.setRange(range_start,c);
                        range_set = false;
                    }
                    last_char = c;
                }
                break;
    
            case 'w' :
                if(escaped){
                    column.set('_'); // '_'
                    column.setRange(48,57);
                    //setRange(column,48,57, value); // d
                    column.setRange(65,90);
                    //setRange(column,65,90, value); // A-Z
                    column.setRange(97,122);
                    // setRange(column,97,122, value); // a-z
                    escaped = false;
                }else{
                    column.set(c);
                    if(range_set){
                        column.setRange(range_start,c);
                        range_set = false;
                    }
                    last_char = c;
                }
                break;
    
                // Inversion
            case '^' :
                if(escaped){
                    column.set(c);
                    if(range_set){
                        column.setRange(range_start,c);
                        range_set = false;
                    }
                    last_char = c;
                    escaped = false;
                }else{
                    inverting = true;
                }
                break;
    
                // HEX
            case 'x' :
                if(escaped){
                    //process hex char
                    ++index;
                    char hex[3];
                    hex[0] = (char)symbol_set.c_str()[index];
                    hex[1] = (char)symbol_set.c_str()[index+1];
                    hex[2] = '\0';
                    unsigned char number = (unsigned char)std::strtoul(hex, NULL, 16);
    
                    //
                    ++index;
                    column.set(number);
                    if(range_set){
                        column.setRange(range_start,number);
                        range_set = false;
                    }
                    last_char = number;
                    escaped = false;
                }else{
                    column.set(c);
                    if(range_set){
                        column.setRange(range_start,c);
                        range_set = false;
                    }
                    last_char = c;
                }
                break;
    
    
                // Other characters
            default:
                if(escaped){
                    // we escaped a char that is not valid so treat it normaly
                    escaped = false;
                }
                column.set(c);
                if(range_set){
                    column.setRange(range_start,c);
                    range_set = false;
                }
                last_char = c;
            };
    
            index++;
        } // char while loop
    
        if(inverting)
            column.flip();
    
        if(bracket_sem != 0 ||
                brace_sem != 0){
            std::cout << "MALFORMED BRACKETS OR BRACES: " << symbol_set <<  std::endl;
            std::cout << "brackets: " << bracket_sem << std::endl;
            exit(1);
        }
    
    
        /*
             std::cout << "***" << std::endl;
             for(int i = 0; i < 256; i++){
             if(column.test(i))
             std::cout << i << " : 1" << std::endl;
             else
             std::cout << i << " : 0" << std::endl;
             }
             std::cout << "***" << std::endl;
         */
    }
    
    //WADDEN
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
            map<string, unsigned int> report_id_mapping = map<string, unsigned int>();
            
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
                    MNRLReportId rid = node->getReportId();
                    string mnrl_rid;
                    
                    unsigned int hs_report_id;
                    map<string, unsigned int>::iterator it;
                    
                    switch(rid.get_type()) {
                        case MNRLDefs::ReportIdType::INT:
                        case MNRLDefs::ReportIdType::STRING:
                            mnrl_rid = rid.toString();
                            it = report_id_mapping.find(mnrl_rid);
                            if(it != report_id_mapping.end()) {
                                // there was an entry
                                hs_report_id = it->second;
                            } else {
                                // not found, so add
                                hs_report_id = report_id_int++;
                                report_id_mapping.insert(map<string, unsigned int>::value_type(mnrl_rid, hs_report_id));
                            }
                            break;
                        default:
                            // no report ID
                            hs_report_id = report_id_int++;
                            mnrl_rid = node->getId();
                            break;
                    }
                    
                    // store this in our map
                    insert_mapping(hs_report_id, mnrl_rid.c_str(), report_map);
                    
                    
                    // For now just register a dummy report code for all reports
                    Report report(EXTERNAL_CALLBACK, report_id_int++);
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
    
            /*
    
            // Open file
            string line;
            string graphFN_tmp(graphFN);
            ifstream graph_file(graphFN_tmp);
            if(graph_file.is_open()){
    
                // first line is the number of nodes
                getline(graph_file, line);
                int num_nodes = stoi(line,nullptr,0);
                //cout << "NUM NODES: " << num_nodes << endl;
    
                // For now just register a dummy report code for all reports
                Report report(EXTERNAL_CALLBACK, 1000);
                ReportID report_id = ng.rm.getInternalId(report); // register with report manager
    
                // parse nodes
                for(int i = 0; i < num_nodes; i++){
                    //cout << "Parsing node: " << i << endl;
                    // next input is node names, char reach, and accept/report
                    // FORMAT: name char_reach startDs start report
                    // EXAMPLE: __blah__ 000000001000001000100000000001000...000 1 0 0
    
                    // get line
                    getline(graph_file, line);
    
                    // tokenize
                    string buf;
                    stringstream ss(line);
                    vector<string> tokens;
                    while (ss >> buf)
                        tokens.push_back(buf);
    
                    // extract name and create new vertex
                    string name = tokens[0];
                    NFAVertex tmp = add_vertex(graph);
                    vertices[name] = tmp;
    
                    // extract char reach
                    string char_reach = tokens[1];
                    if(char_reach.size() != 256){
                        cout << "CHAR REACH ISNT 256! " << char_reach.size() << " Exiting..." << endl;
                        exit(1);
                    }
                    graph[tmp].char_reach = CharReach();
                    graph[tmp].char_reach.clear();
                    for(int index = 0; index < 256; ++index) {
                        if(char_reach[index] == '1'){
                            graph[tmp].char_reach.set(255 - index);
                            //cout << "SET BIT NUMBER: " << (255-index) << endl;
                        }
                    }
    
                    // extract start
                    string start = tokens[2];
                    if(start.compare("1") == 0){
                        //cout << "IS START" << endl;
                        add_edge(graph.start, tmp, graph);
                    }
                    // extract startDs
                    string startDs = tokens[3];
                    if(startDs.compare("1") == 0){
                        //cout << "IS STARTDS" << endl;
                        add_edge(graph.startDs, tmp, graph);
                    }
    
                    // extract accept
                    string accept = tokens[4];
                    if(accept.compare("1") == 0){
                        //cout << "IS ACCEPT" << endl;
                        add_edge(tmp, graph.accept, graph);
                        //register report code here
                        graph[tmp].reports.insert(report_id);
                    }
    
                    // print line
                    //for(int j = 0; j < 5; j++)
                        //cout << tokens[j] << endl;
                }
    
                // Parse edges
                while(!graph_file.eof()){
                    getline(graph_file, line);
                    if(!line.empty()){
                        //cout << line << endl;
                        string from;
                        string to;
                        stringstream ss2(line);
                        ss2 >> from;
                        while (ss2 >> to){
                            //cout << "ADDING EDGE" << endl;
                            add_edge(vertices[from], vertices[to], graph);
                        }
                    }
                }
    
                graph_file.close();
    
            }else{
                cout << "Could not open graph file!" << endl;
                exit(1);
            }
             */
    
            /*
            NGWrapper &graph = *graph_ptr;
    
            // Register reports
            Report report(EXTERNAL_CALLBACK, 1000);
            ReportID id = ng.rm.getInternalId(report); // register with report manager
    
            // Build graph
            // Construct vertices
            // a
            NFAVertex a = add_vertex(graph);
            graph[a].char_reach = CharReach('a');
    
            // b
            NFAVertex b = add_vertex(graph);
            graph[b].char_reach = CharReach('b');
    
            // c
            NFAVertex c = add_vertex(graph);
            graph[c].char_reach = CharReach('c');
    
    
    
            // Construct edges
            add_edge(graph.startDs, a, graph);
            add_edge(graph.start, a, graph);
            add_edge(a, b, graph);
            add_edge(b, c, graph);
            add_edge(c, graph.accept, graph);
    
            // Add reports
            graph[c].reports.insert(id);
    
    
             */
    
    
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
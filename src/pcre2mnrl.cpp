#include <iostream>
#include <fstream>
#include <cstring>
#include <boost/lexical_cast.hpp>
#include "hs.h"
#include "hs_pcre_mnrl.h"
#include <mnrl.hpp>

using namespace std;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <regex file> <mnrl file>" << endl;
        return 1;
    }
    
    // read in the file
    ifstream infile(argv[1]);
    string line;
    
    vector<string> expressions;
    vector<const char*> cexpressions;
    vector<unsigned> ids;
    vector<unsigned> flags;
    
    unsigned i = 0;
    while(getline(infile, line)) {
        
        vector<string> line_tokens;
        
        int colon = -1;
        int first_slash = -1;
        int second_slash = -1;
        
        unsigned int j=0;
        for(const char &c : line) {
            if(first_slash < 0 && c == ':') {
                colon = j;
            } else if (c == '/') {
                if(first_slash < 0) {
                    first_slash = j;
                } else {
                    second_slash = j;
                }
            }
            j++;
        }
        
        // check that we have 3 parts (i.e. 2 / characters)
        if(first_slash < 0 || second_slash < 0) {
            cerr << "Rule on line " << i+1 << " was not surrounded by slashes!" << endl;
            exit(5);
        }
        
        if(colon < 0) {
            line_tokens.push_back("");
        } else {
            line_tokens.push_back(line.substr(0,colon));
        }
        
        // push the expressions and modifiers
        line_tokens.push_back(line.substr(first_slash+1, second_slash));
        line_tokens.push_back(line.substr(second_slash+1, line.size()));
        
        /*
         * line_tokens[0] == empty/id
         * line_tokens[1] == the expression
         * line_tokens[2] == the modifiers
         */
        
        // convert the modifiers
        int e_flags = 0;
        bool failed = false;
        if(line_tokens[2].size() > 0) {
            for(char &m : line_tokens[2]) {
                // PAY ATTENTION note that we are ORing things together
                // DON'T FORGET A BREAK
                switch(m) {
                case 'i':
                    e_flags |= HS_FLAG_CASELESS;
                    break;
                case 'm':
                    e_flags |= HS_FLAG_MULTILINE;
                    break;
                case 's':
                    e_flags |= HS_FLAG_DOTALL;
                    break;
                default:
                    failed = true;
                    cerr << "Unsupported modifier '" << m << "' on line " << i+1 << endl;
                }
            }
        }
        
        
        // if we failed to parse, continue
        if(failed) {
            cerr << "Rule on line " << i+1 << " had unsupported modifiers, skipping" << endl;
        } else {
            expressions.push_back(line_tokens[1]);
            cexpressions.push_back(expressions.back().c_str());
            if(line_tokens[0].size() != 0) {
                try
                {
                    ids.push_back(boost::lexical_cast<int>(line_tokens[0]));
                }
                catch(boost::bad_lexical_cast &)
                {
                    cerr << "Warning: could not convert ID on line " << i+1 << " to an integer; using line number." << endl;
                    ids.push_back(i);
                }
            } else {
                ids.push_back(i);
            }
            flags.push_back(e_flags);
        }
        
        i++;
    }
    
    hs_compile_error_t *compile_err;
    MNRL::MNRLNetwork mnrl("pcre");
    
    
    if (hs_pcre_mnrl_multi(cexpressions.data(),
                     mnrl,
                     &flags[0],
                     &ids[0],
                     cexpressions.size(),
                     &compile_err) != HS_SUCCESS) {
        cerr << "ERROR: Unable to compile PCRE expressions file " << argv[1] << ": " << compile_err->message << endl;
    }
    
    mnrl.exportToFile(argv[2]);
    
    return 0;
    
}
#include <iostream>
#include <fstream>
#include <cstring>
#include <boost/regex.hpp>
#include <boost/algorithm/string_regex.hpp>
#include "hs.h"
#include "hs_pcre_mnrl.h"
#include <mnrl.hpp>

using namespace std;

char *strdup(const char *src_str) noexcept {
    char *new_str = new char[std::strlen(src_str) + 1];
    std::strcpy(new_str, src_str);
    return new_str;
}

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
        
        size_t start_loc = line.find_first_of("/");
        size_t end_loc = line.find_last_of("/");
        
        
        // check that we have 3 parts (i.e. 2 / characters)
        if(start_loc == string::npos || end_loc == string::npos){
            cerr << "Rule on line " << i+1 << " was not surrounded by slashes!" << endl;
            exit(5);
        }
        
        string expr = line.substr(start_loc + 1, end_loc - 1);
        //cout << "EXPR: " << expr << endl;
        string mods = line.substr(end_loc + 1, line.size());
        //cout << "MODS: " << mods << endl;

        // convert the modifiers
        int e_flags = 0;
        bool failed = false;
        if(mods.size() > 0) {
            for(char &m : mods) {
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
                case 'H':
                    e_flags |= HS_FLAG_SINGLEMATCH;
                    break;
                default:
                    failed = true;
                    //cerr << "Unsupported modifier '" << m << "' on line " << i+1 << endl;
                }
            }
        }
        
        
        // if we failed to parse, continue
        if(failed) {
            //cerr << "Rule on line " << i+1 << " had unsupported modifiers, skipping" << endl;
        } else {
            expressions.push_back(expr);
            const char *cexpr = strdup(expr.c_str());
            cexpressions.push_back(cexpr);
            ids.push_back(i);
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
        cerr << "ERROR: Unable to compile PCRE expression file " << argv[1] << " expression number " << compile_err->expression << " : " << compile_err->message << endl;
        cerr << " pcre: " << expressions[compile_err->expression] << endl;
        cerr << " flags: " << flags[compile_err->expression] << endl;
    }
    
    mnrl.exportToFile(argv[2]);
    
    return 0;
    
}

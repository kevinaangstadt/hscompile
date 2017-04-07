#include <iostream>
#include <fstream>
#include <cstring>
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
        // check that first and last char are '/'
        if(line.length() < 1 || line[0] != '/' || line[line.length()-1] != '/') {
            cerr << "Rule on line " << i+1 << " was not surrounded by slashes!" << endl;
            exit(5);
        }
        
        // strip the slashes
        expressions.push_back(line.substr(1,line.length()-2));
        cexpressions.push_back(expressions[i].c_str());
        ids.push_back(i);
        flags.push_back(0);
        
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
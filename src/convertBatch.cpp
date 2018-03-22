#include <iostream>
#include <fstream>
#include <cstring>
#include <boost/lexical_cast.hpp>
#include "hs.h"
#include "hs_pcre_mnrl.h"
#include <mnrl.hpp>

using namespace std;

char *strdup(const char *src_str) noexcept {
    char *new_str = new char[std::strlen(src_str) + 1];
    std::strcpy(new_str, src_str);
    return new_str;
}

int extractExpr(string line, string &expression, vector<unsigned> &flags){

  size_t start_loc = line.find_first_of("/");
  size_t end_loc = line.find_last_of("/");
  
  // check that we have 3 parts (i.e. 2 / characters)
  if(start_loc == string::npos || end_loc == string::npos){
    cerr << "Rule: " << line << " was not surrounded by slashes!" << endl;
    exit(5);
  }
  
  string mods = line.substr(end_loc + 1, line.size());
  
  // convert the modifiers
  int e_flags = 0;
  if(mods.size() > 0) {
    for(char &m : mods) {
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
	  continue; // ignore unrecognized modifiers.
      }
    }
  }
  
  // write results back.
  expression = line.substr(start_loc + 1, end_loc - 1);
  flags.push_back(e_flags);
  
  return 0;

}

int main(int argc, char *argv[]) {
  if (argc != 2){
    cerr << "Usage: " << argv[0] << " <regex file> " << endl;
    return 1;
  }
  
  // read in the file
  ifstream infile(argv[1]);
  string line;

  /*
   * we want to create one mnrl file for one regex.
   * probabily not the best practice,
   * but I just want to pinpoint which regex -> mnrl breaks in Vasim.
   */ 
  string output_dir = "/zf18/lw2ef/Documents/workspace/ANMLZoo2/Snort/Snort_debug/giant";
  unsigned i = 0;
  while(getline(infile, line)) {

    string expression;
    vector<unsigned> flags;
    
    // get the expression and ORing the modifiers.
    extractExpr(line, expression, flags);
    
//     cout << expression << "\n";
    
    vector<const char*> cexpressions;
    // reformat expression for Hyperscan.
    const char *cexpr = strdup(expression.c_str());
    cexpressions.push_back(cexpr);
  
    // other variables that hscompile use.
    vector<unsigned> ids;
    ids.push_back(i);
    string output_file = output_dir + "/" + to_string(i) + ".mnrl";

    hs_compile_error_t *compile_err;
    MNRL::MNRLNetwork mnrl("pcre");
    
    if (hs_pcre_mnrl_multi(cexpressions.data(),
                     mnrl,
                     &flags[0],
                     &ids[0],
                     cexpressions.size(),
                     &compile_err) != HS_SUCCESS) {
      
     
      cout << "something went wrong\n";
      cerr << " pcre: " << cexpressions.front() << endl;
      cerr << " flags: " << flags[compile_err->expression] << endl;
      
    }
    
    // create a mnrl file which contains 
    mnrl.exportToFile(output_file);
    i++;
    
  }
  
  cout << "total: " << i << "\n";
  
  return 0;
    
}

#include <iostream>
#include <fstream>
#include <cstring>
#include <boost/lexical_cast.hpp>
#include "hs.h"
#include <map>
#include <set>

using namespace std;

int main(int argc, char *argv[]) {
  
  // One arg for regex.
  if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <snort_regex_file_name> <filtered_snort_regex_file_path>" << endl;
        return 1;
  }

  // read in the file
  ifstream infile(argv[1]);
  string line;
  string prevExpr;

  // counters
  int successCounter = 0;
  int failedCounter = 0;
  
  // error hash maps.
  map<string, int> errorMsgMap;
  
  // a set that contains unique pcre_lines
  set<string> pcreSet;
  
  // remove redundant pcres.
  while(getline(infile, line)){
    pcreSet.insert(line);
  }
  
  // Open files for output.
  ofstream failedPCRE;
  ofstream succeedPCRE;
  //failedPCRE.open("/zf18/lw2ef/Documents/workspace/ANMLZoo2/Snort/Snort_debug/regex/failedPCRE.regex");
  //succeedPCRE.open("/zf18/lw2ef/Documents/workspace/ANMLZoo2/Snort/Snort_debug/regex/succeedPCRE.regex");
  failedPCRE.open(std::string(argv[2])+"/failedPCRE.regex");
  succeedPCRE.open(std::string(argv[2])+"/succeedPCRE.regex");
  unsigned i = 0;
  // iterate through set and compile pcre one-by-one.
  for(auto line : pcreSet){
    
   size_t start_loc = line.find_first_of("/");
   size_t end_loc = line.find_last_of("/");
   
   if(start_loc == string::npos || end_loc == string::npos){
    cerr << "Rule on line " << i+1 << " was not surrounded by slashes!" << endl;
    continue; 
   }
   
   string expr = line.substr(start_loc + 1, end_loc - 1);
   
   // Try to compile it with hyperscan.
   hs_database_t *database;
   hs_compile_error_t *compile_err;
   
   // Compile
   if (hs_compile(expr.c_str(), HS_FLAG_SINGLEMATCH, HS_MODE_BLOCK, NULL, &database,
     &compile_err) == HS_COMPILER_ERROR) {
     
    errorMsgMap[compile_err->message] ++;
    failedCounter ++;
   
    // Save the bad ones to file.
    failedPCRE << line << " " << compile_err->message << "\n";
   
   } else {
     successCounter ++;
     
     // Save the good ones to file.
     succeedPCRE << line << "\n";
     
   }
   
   // Free mem space...
   hs_free_compile_error(compile_err);
   hs_free_database(database);
   
  }
  
  // Close the files.
  succeedPCRE.close();
  failedPCRE.close();
  
  // Result printing...  
  cout << "\n--- Summary ---\n";  
  cout << "Total num of PCREs: " << pcreSet.size() << "\n";
  cout << "Types of errors: " << errorMsgMap.size() << "\n"; 
  cout << "----------------------" << "\n";
  for (map<string,int>::iterator it=errorMsgMap.begin(); it!=errorMsgMap.end(); ++it)
    cout << it->first << " => " << it->second << '\n';
  cout << "----------------------" << "\n";
  cout << "Num of succeed: " << successCounter << "\n";
  cout << "Num of failed: " << failedCounter << "\n";
  cout << "success + failed: " << successCounter + failedCounter << "\n";
  cout << "--- End of Summary ---\n"; 
  
  return 0;
}



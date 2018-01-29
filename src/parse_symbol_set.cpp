/* 
 * Copyright (c) 2016, University of Virginia
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University of Virginia nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE UNIVERSITY OF VIRGINIA BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * This software was originally developed by Jack Wadden (jackwadden@gmail.com). 
 * A list of all contributors is maintained at https://github.com/jackwadden/VASim.
 * 
 * If you use VASim in your project, please use the following citation:
 * Wadden, J. and Skadron, K. "VASim: An Open Source Simulation and Analysis
 * Platform for Finite Automata Applications and Architecture Research." GitHub
 * repository, https://github.com/jackwadden/VASim. University of Virginia, 2016.
 */

/*
 * Modified by Kevin Angstadt <angstadt@virginia.edu> for use with HyperScan
 */

#include "parse_symbol_set.h"
#include <string>
#include <iostream>

namespace ue2 {
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
                    escaped = false;
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
                    escaped = false;
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
};

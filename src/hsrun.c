#include <limits.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <omp.h>
#include "hs.h"
#include "hs_compile_mnrl.h"
#include "ht.h"
#include "read_input.h"

typedef struct run_ctx_t {
    r_map *report_map;
    hs_database_t *database;
    hs_scratch_t *scratch;
    char *inputData;
    size_t length;
    unsigned int inp_off; //for the location in filename list
    unsigned int db_off; // for the location in filename list
} run_ctx;

/**
 * This is the function that will be called for each match that occurs. @a ctx
 * is to allow you to have some application-specific state that you will get
 * access to for each match. In our simple example we're just going to use it
 * to pass in the pattern that was being searched for so we can print it out.
 */
static int eventHandler(unsigned int id, unsigned long long from,
                        unsigned long long to, unsigned int flags, void *ctx) {
    
    r_map *report_map = (r_map *) ctx;
    
    r_map *m = find_mapping(id, &report_map);
    if(m == NULL) {
        printf("couldn't find mapping: %u\n", id);
        return 1;
    }    
    printf("Match at id::code::offset %s::%s::%llu\n", m->name, m->report, to);
    return 0;
}


static void usage(char *prog) {
    fprintf(stderr, "Usage: %s [-t NUM_TREADS] <hs databases> <input files>\n", prog);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        usage(argv[0]);
		return 1;
    }
    
    unsigned int num_dbs = 0;
    unsigned int num_inputs = 0;
    
    int num_threads = 0;
    
    char *db_fns[argc];
    char *input_fns[argc];
    
    for ( int i=1; i<argc; i++ ) {
        
        if ( strcmp("-t", argv[i]) == 0 ) {
            // setting number of threads
            
            if ( i+1 <= argc ) {
                i++;
                num_threads = atoi(argv[i]);
            } else {
                usage(argv[0]);
                return 44;  
            }
            continue;
        }
        
        size_t len = strlen(argv[i]);
        if ( len < 4 ) {
            input_fns[num_inputs] = argv[i];
            num_inputs += 1;
        } else {
            if ( argv[i][len-1] == 's' && argv[i][len-2] == 'h' && argv[i][len-3] == '.' )  {
                db_fns[num_dbs] = argv[i];
                num_dbs += 1;
            } else {
                input_fns[num_inputs] = argv[i];
                num_inputs += 1;
            }
        }
    }
    
    if ( num_dbs == 0 || num_inputs == 0 ) {
        usage(argv[0]);
        return 45;
    }
    
    run_ctx contexts[num_dbs*num_inputs];
    
    size_t inputs_length[num_inputs];
    
    // for cleanup
    hs_database_t *dbs_to_delete[num_dbs];
    char *inputs_to_delete[num_inputs];
    r_map *rmaps_to_delete[num_dbs];
    
    //loop through the inputs to get input data
    for ( int j=0; j < num_inputs; j++ ) {
        char *inputFN = input_fns[j];
        size_t length;
        
        /* Next, we read the input data file into a buffer. */
        char *inputData;
        inputData = readInputData(inputFN, &length);
        if (!inputData) {
            fprintf(stderr, "ERROR: Unable to read input data '%s'. Exiting.\n", inputFN);
            //hs_free_database(database);
            for( int i=0; i<j; i++){
                free(inputs_to_delete[i]);
            }
            return 4;
        }
        
        inputs_to_delete[j] = inputData;
        inputs_length[j] = length;
    }
    
    // loop through the dbs
    for ( int i=0; i < num_dbs; i++ ) {
        
        char *hsDB = db_fns[i];
        
        // First, read in the database
        size_t length;
        char *hsSerDB;
        char *hsmSer;
        
        hsmSer = readInputData(hsDB, &length);
        
        if(!hsmSer) {
            return 2;
        }
        
        // extract the mapping
        r_map *report_map = NULL;
        
        size_t map_length;
        unserialize_mapping (hsmSer, &map_length, &report_map);
        
        rmaps_to_delete[i] = report_map;
        
        // redo the database pointer 
        hsSerDB = hsmSer + map_length ;
        length -= map_length ;
        
        // Next, we try to deserialize
        hs_database_t *database;
        hs_compile_error_t *compile_err;
    
        if(hs_deserialize_database(hsSerDB, length, &database) != HS_SUCCESS) {
            fprintf(stderr, "ERROR: Unable to load HyperScan database file \"%s\": %s. Exiting.\n",
                    hsDB, compile_err->message);
            free(hsmSer);
            
            delete_all(&report_map);
            hs_free_compile_error(compile_err);
            
            // deallocate inputs
            for(int j=0; j<num_inputs; j++) {
                free(inputs_to_delete[j]);
            }
            
            
            for(int j=0; j<i; j++) {
                // deallocate previous databases
                free(dbs_to_delete[j]);
                
                // delete report map
                delete_all(&(rmaps_to_delete[j]));
                
                // kill off all the scratch space that was stored previously
                for(int k=0; k<num_inputs; k++) {
                    free(contexts[j*num_inputs+k].scratch);
                }
            }
            
            return 3;
        }
        
        // keep track of the database
        dbs_to_delete[i] = database;
        
        //printf("Allocating scratch...\n");
        hs_scratch_t *db_scratch = NULL;
        if (hs_alloc_scratch(database, &db_scratch) != HS_SUCCESS) {
            fprintf(stderr, "ERROR: Unable to allocate scratch space for database '%s'. Exiting.\n", hsDB);
            free(hsmSer);
            delete_all(&report_map);
            hs_free_database(database);
            
            // deallocate inputs
            for(int j=0; j<num_inputs; j++) {
                free(inputs_to_delete[j]);
            }
            
            
            for(int j=0; j<i; j++) {
                // deallocate previous databases
                free(dbs_to_delete[j]);
                
                // delete report map
                delete_all(&(rmaps_to_delete[j]));
                
                // kill off all the scratch space that was stored previously
                for(int k=0; k<num_inputs; k++) {
                    free(contexts[j*num_inputs+k].scratch);
                }
            }
            
            return 5;
        }
        
        
        //loop through the inputs
        for ( int j=0; j < num_inputs; j++ ) {
            char *inputData;
            inputData = inputs_to_delete[j];
            
            /* Finally, we issue a call to hs_scan, which will search the input buffer
             * for the pattern represented in the bytecode. Note that in order to do
             * this, scratch space needs to be allocated with the hs_alloc_scratch
             * function. In typical usage, you would reuse this scratch space for many
             * calls to hs_scan, but as we're only doing one, we'll be allocating it
             * and deallocating it as soon as our matching is done.
             *
             * When matches occur, the specified callback function (eventHandler in
             * this file) will be called. Note that although it is reminiscent of
             * asynchronous APIs, Hyperscan operates synchronously: all matches will be
             * found, and all callbacks issued, *before* hs_scan returns.
             *
             * In this example, we provide the input pattern as the context pointer so
             * that the callback is able to print out the pattern that matched on each
             * match event.
             */
            
            hs_scratch_t *scratch = NULL;
            if ( hs_clone_scratch(db_scratch, &scratch) != HS_SUCCESS ) {
                printf("ERROR: Unable to allocate cloned scratch space. Exiting.\n");
                free(inputData);
                free(hsmSer);
                delete_all(&report_map);
                hs_free_database(database);
                
                // deallocate inputs
                for(int j=0; j<num_inputs; j++) {
                    free(inputs_to_delete[j]);
                }
                
                
                for(int j=0; j<i; j++) {
                    // deallocate previous databases
                    free(dbs_to_delete[j]);
                    
                    // delete report map
                    delete_all(&(rmaps_to_delete[j]));
                    
                    // kill off all the scratch space that was stored previously
                    for(int k=0; k<num_inputs; k++) {
                        free(contexts[j*num_inputs+k].scratch);
                    }
                }
                return 7;
            }
            
            /* Store all of the context information */
            contexts[i*num_inputs+j].report_map = report_map;
            contexts[i*num_inputs+j].database = database;
            contexts[i*num_inputs+j].scratch = scratch;
            contexts[i*num_inputs+j].inputData = inputData;
            contexts[i*num_inputs+j].length = inputs_length[j];
            contexts[i*num_inputs+j].db_off = i;
            contexts[i*num_inputs+j].inp_off = j;
            
            
            /* Scanning is complete, any matches have been handled, so now we just
             * clean up and exit.
             */
        } // input loop
        free(hsmSer);
        // hs_free_database(database);
        
        // free up db_scratch
        hs_free_scratch(db_scratch);
    } // database loop
 
    //printf("Simulating graph on input data with Hyperscan...\n");
    
    //okay do the scanning
    if(num_threads > 0) {
        omp_set_dynamic(1);
        omp_set_num_threads(num_threads);
    }
    #pragma omp parallel for
    for ( int i=0; i<num_inputs*num_dbs; i++ ) {
        run_ctx ctx = contexts[i];
        
        // scan each input and report runtime
        if (hs_scan(ctx.database, ctx.inputData, ctx.length, 0, ctx.scratch, eventHandler,
                    ctx.report_map) != HS_SUCCESS) {
            fprintf(stderr, "ERROR: Unable to scan input buffer '%s' with database '%s'.\n", input_fns[ctx.inp_off], db_fns[ctx.db_off]);
            /*
             * No need to stop, just keep trying
            hs_free_scratch(scratch);
            free(inputData);
            free(hsmSer);
            hs_free_database(database);
            return 6;
            */
        }
        
        hs_free_scratch(ctx.scratch);
    }
    
    // cleanup
    for ( int i=0; i<num_dbs; i++ ) {
        free(dbs_to_delete[i]);
        delete_all(&(rmaps_to_delete[i]));
    }
    
    for ( int i=0; i<num_inputs; i++ ) {
        free(inputs_to_delete[i]);
    }
    
    return 0;
}
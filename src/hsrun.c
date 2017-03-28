#include <limits.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "hs.h"
#include "hs_compile_mnrl.h"

/**
 * This is the function that will be called for each match that occurs. @a ctx
 * is to allow you to have some application-specific state that you will get
 * access to for each match. In our simple example we're just going to use it
 * to pass in the pattern that was being searched for so we can print it out.
 */
static int eventHandler(unsigned int id, unsigned long long from,
                        unsigned long long to, unsigned int flags, void *ctx) {
    printf("Match at id::offset %u::%llu\n", id, to);
    return 0;
}

/**
 * Fill a data buffer from the given filename, returning it and filling @a
 * length with its length. Returns NULL on failure.
 */
static char *readInputData(const char *inputFN, unsigned int *length) {
    

    FILE *f = fopen(inputFN, "rb");
    if (!f) {
        fprintf(stderr, "ERROR: unable to open file \"%s\": %s\n", inputFN,
                strerror(errno));
        return NULL;
    }

    /* We use fseek/ftell to get our data length, in order to keep this example
     * code as portable as possible. */
    if (fseek(f, 0, SEEK_END) != 0) {
        fprintf(stderr, "ERROR: unable to seek file \"%s\": %s\n", inputFN,
                strerror(errno));
        fclose(f);
        return NULL;
    }
    long dataLen = ftell(f);
    if (dataLen < 0) {
        fprintf(stderr, "ERROR: ftell() failed: %s\n", strerror(errno));
        fclose(f);
        return NULL;
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        fprintf(stderr, "ERROR: unable to seek file \"%s\": %s\n", inputFN,
                strerror(errno));
        fclose(f);
        return NULL;
    }

    /* Hyperscan's hs_scan function accepts length as an unsigned int, so we
     * limit the size of our buffer appropriately. */
    if ((unsigned long)dataLen > UINT_MAX) {
        dataLen = UINT_MAX;
        printf("WARNING: clipping data to %ld bytes\n", dataLen);
    } else if (dataLen == 0) {
        fprintf(stderr, "ERROR: input file \"%s\" is empty\n", inputFN);
        fclose(f);
        return NULL;
    }

    char *inputData = (char *)malloc(dataLen);
    if (!inputData) {
        fprintf(stderr, "ERROR: unable to malloc %ld bytes\n", dataLen);
        fclose(f);
        return NULL;
    }

    char *p = inputData;
    size_t bytesLeft = dataLen;
    while (bytesLeft) {
        size_t bytesRead = fread(p, 1, bytesLeft, f);
        bytesLeft -= bytesRead;
        p += bytesRead;
        if (ferror(f) != 0) {
            fprintf(stderr, "ERROR: fread() failed\n");
            free(inputData);
            fclose(f);
            return NULL;
        }
    }

    fclose(f);

    *length = (unsigned int)dataLen;
    return inputData;
}


int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <hs database> <input file>\n", argv[0]);
		return 1;
    }
    
    char *hsDB = argv[1];
    char *inputFN = argv[2];
    
    
    // First, read in the database
    unsigned int length;
    char *hsSerDB;
    
    hsSerDB = readInputData(hsDB, &length);
    
    if(!hsSerDB) {
        return 2;
    }
    
    // Next, we try to deserialize
    hs_database_t *database;
    hs_compile_error_t *compile_err;
    
    if(hs_deserialize_database(hsSerDB, length, &database) != HS_SUCCESS) {
        fprintf(stderr, "ERROR: Unable to load HyperScan database file \"%s\": %s\n",
				hsDB, compile_err->message);
		hs_free_compile_error(compile_err);
		return 3;
    }
    
    /* Next, we read the input data file into a buffer. */
    char *inputData;
    inputData = readInputData(inputFN, &length);
    if (!inputData) {
        hs_free_database(database);
        return -4;
    }
    
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
    //printf("Allocating scratch...\n");
    hs_scratch_t *scratch = NULL;
    if (hs_alloc_scratch(database, &scratch) != HS_SUCCESS) {
        fprintf(stderr, "ERROR: Unable to allocate scratch space. Exiting.\n");
        free(inputData);
        hs_free_database(database);
        return 5;
    }

    //printf("Simulating graph on input data with Hyperscan...\n");

    // scan each input and report runtime
    if (hs_scan(database, inputData, length, 0, scratch, eventHandler,
                NULL) != HS_SUCCESS) {
        fprintf(stderr, "ERROR: Unable to scan input buffer. Exiting.\n");
        hs_free_scratch(scratch);
        free(inputData);
        hs_free_database(database);
        return 6;
    }



    
    /* Scanning is complete, any matches have been handled, so now we just
     * clean up and exit.
     */
    hs_free_scratch(scratch);
    free(inputData);
    hs_free_database(database);
    return 0;
}
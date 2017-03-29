#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "hs.h"
#include "hs_compile_mnrl.h"

int main(int argc, char *argv[]) {
	if (argc != 3) {
		fprintf(stderr, "Usage: %s <mnrl file name> <compiled file name>\n", argv[0]);
		return 1;
	}

	char *mnrlFN = argv[1];
	char *compiledFN = argv[2];

	printf("Compiling MNRL...\n");
	hs_database_t *database;
	hs_compile_error_t *compile_err;
	
	r_map *report_map = NULL;

	if (hs_compile_mnrl(mnrlFN,
						&database,
						&compile_err,
						&report_map) != HS_SUCCESS) {
		fprintf(stderr, "ERROR: Unable to compile MNRL file \"%s\": %s\n",
				mnrlFN, compile_err->message);
		hs_free_compile_error(compile_err);
		return 2;
	}
	
	char *mapping_out;
	size_t mapping_size;
	serialize_mapping(&mapping_out, &mapping_size, &report_map);
	
	
	printf("Serializing...\n");
	char *ser;
	size_t size;
	if (hs_serialize_database(database, &ser, &size) != HS_SUCCESS) {
		fprintf(stderr, "ERROR: Unable to serialize: %s\n",
				compile_err->message);
		hs_free_compile_error(compile_err);
		return 3;
	}

	printf("Writing...\n");

	FILE *f = fopen(compiledFN, "wb");
	if (!f) {
		fprintf(stderr, "ERROR: unable to open file \"%s\": %s\n", compiledFN,
				strerror(errno));
		return 4;
	}

	// write the file
	fwrite(mapping_out, 1, mapping_size, f);
	fwrite(ser, 1, size, f);

	fclose(f);

	//clean up
	delete_all(&report_map);
	free(mapping_out);
	free(ser);
	printf("Compilation done.\n");

	return 0;
}

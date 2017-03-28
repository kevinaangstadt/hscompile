#ifndef HT_H_
#define HT_H_

#include "uthash.h"
#include "string.h"
#include "stdio.h"

struct hs_report_mapping {
    unsigned int id;
    UT_hash_handle hh;
    char *name;
};

typedef struct hs_report_mapping r_map;

void insert_mapping (unsigned int id, const char *name, r_map **ht) {
    r_map *s;
    
    HASH_FIND_INT(*ht, &id, s);
    if(s==NULL) {
        s = (r_map*)malloc(sizeof(r_map));
        s->id = id;
    
        HASH_ADD_INT( *ht, id, s );
    }
    
    char *tmp = (char *)malloc(strlen(name));
    strcpy(tmp, name);
    s->name = tmp;
    
}

r_map *find_mapping (unsigned int id, r_map **ht) {
    r_map *s;
    
    HASH_FIND_INT( *ht, &id, s );
    return s;
}

void serialize_mapping (char **outstr, size_t *size, r_map **ht) {
    *size = sizeof(unsigned int);
    *outstr = (char *)malloc(*size*sizeof(char));
    
    // first print the number of entries
    unsigned int num_entries;
    num_entries = HASH_COUNT(*ht);
    
    // write num entries
    *(*outstr) = (char) (num_entries >> 24) & 0xFF;
    *(*outstr+1) = (char) (num_entries >> 16) & 0xFF;
    *(*outstr+2) = (char) (num_entries >> 8) & 0xFF;
    *(*outstr+3) = (char) num_entries & 0xFF;
    
    r_map *s, *tmp;
    
    HASH_ITER(hh, *ht, s, tmp) {
        unsigned int old_size = *size;
        *size +=  2*sizeof(unsigned int) + strlen(s->name);
        *outstr = (char *)realloc(*outstr, *size);
        
        // write id
        (*outstr+old_size)[0] = (s->id >> 24) & 0xFF;
        (*outstr+old_size)[1] = (s->id >> 16) & 0xFF;
        (*outstr+old_size)[2] = (s->id >> 8) & 0xFF;
        (*outstr+old_size)[3] = s->id & 0xFF;
        
        unsigned int name_length = strlen(s->name);
        (*outstr+old_size+sizeof(unsigned int))[0] = (name_length >> 24) & 0xFF;
        (*outstr+old_size+sizeof(unsigned int))[1] = (name_length >> 16) & 0xFF;
        (*outstr+old_size+sizeof(unsigned int))[2] = (name_length >> 8) & 0xFF;
        (*outstr+old_size+sizeof(unsigned int))[3] = name_length & 0xFF;
        
        memcpy((*outstr)+old_size+2*sizeof(unsigned int), s->name, strlen(s->name));
        //printf("user id %d: name %s\n", s->id, s->name);
    }
}

void unserialize_mapping (const char *instr, size_t num_entries, r_map **ht) {
    unsigned int i, loc;
    loc = 0;
    for( i = 0; i < num_entries; ++i ){
        unsigned int id = instr[loc];
        loc += sizeof(unsigned int);
        unsigned int len = instr[loc];
        loc += sizeof(unsigned int);
        
        char *tmp =(char*) malloc(len);
        insert_mapping(id, tmp, ht);
        free(tmp);
        loc += len;
    }
}

#endif
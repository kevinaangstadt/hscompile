#ifndef HT_H_
#define HT_H_

#include "uthash.h"
#include "string.h"
#include "stdio.h"

struct hs_report_mapping {
    unsigned int id;
    UT_hash_handle hh;
    char *name;
    char *report;
};

typedef struct hs_report_mapping r_map;

void insert_mapping (unsigned int id, const char *name, const char *report, r_map **ht) {
    r_map *s;
    
    HASH_FIND_INT(*ht, &id, s);
    if(s==NULL) {
        s = (r_map*)malloc(sizeof(r_map));
        s->id = id;
    
        HASH_ADD_INT( *ht, id, s );
    }
    
    char *tmp = (char *)malloc(strlen(name));
    char *tmp2 = (char *)malloc(strlen(report));
    strcpy(tmp, name);
    strcpy(tmp2, report);
    s->name = tmp;
    s->report = tmp2;
    
}

r_map *find_mapping (unsigned int id, r_map **ht) {
    r_map *s;
    
    HASH_FIND_INT( *ht, &id, s );
    return s;
}

void print_mapping (r_map **ht) {
    r_map *s, *tmp;
    
    
    HASH_ITER(hh, *ht, s, tmp) {
        printf("%u :: %s :: %s \n", s->id, s->name, s->report);
    }
}

void serialize_mapping (char **outstr, size_t *size, r_map **ht) {
    *size = sizeof(unsigned int);
    *outstr = (char *)malloc(*size*sizeof(char));
    
    // first print the number of entries
    unsigned int num_entries;
    num_entries = HASH_COUNT(*ht);
    
    // write num entries
    (*outstr)[0] = (char) (num_entries >> 24) & 0xFF;
    (*outstr)[1] = (char) (num_entries >> 16) & 0xFF;
    (*outstr)[2] = (char) (num_entries >> 8) & 0xFF;
    (*outstr)[3] = (char) num_entries & 0xFF;
    
    r_map *s, *tmp;
    
    
    HASH_ITER(hh, *ht, s, tmp) {
        unsigned int old_size = *size;
        *size +=  3*sizeof(unsigned int) + strlen(s->name) + strlen(s->report);
        *outstr = (char *)realloc(*outstr, *size);
        
        // write id
        (*outstr+old_size)[0] = (s->id >> 24) & 0xFF;
        (*outstr+old_size)[1] = (s->id >> 16) & 0xFF;
        (*outstr+old_size)[2] = (s->id >> 8) & 0xFF;
        (*outstr+old_size)[3] = s->id & 0xFF;
        
        old_size += sizeof(unsigned int);
        
        // write name length
        unsigned int name_length = strlen(s->name);
        (*outstr+old_size)[0] = (name_length >> 24) & 0xFF;
        (*outstr+old_size)[1] = (name_length >> 16) & 0xFF;
        (*outstr+old_size)[2] = (name_length >> 8) & 0xFF;
        (*outstr+old_size)[3] = name_length & 0xFF;
        
        old_size += sizeof(unsigned int);
        
        // write name
        memcpy((*outstr)+old_size, s->name, strlen(s->name));
        
        old_size += strlen(s->name);
        
        // write report id length
        unsigned int report_length = strlen(s->report);
        (*outstr+old_size)[0] = (report_length >> 24) & 0xFF;
        (*outstr+old_size)[1] = (report_length >> 16) & 0xFF;
        (*outstr+old_size)[2] = (report_length >> 8) & 0xFF;
        (*outstr+old_size)[3] = report_length & 0xFF;
        
        old_size += sizeof(unsigned int);
        
        // write report id
        memcpy((*outstr)+old_size, s->report, strlen(s->report));
        //printf("user id %d: name %s\n", s->id, s->name);
    }
    
    // append the length of the serialized form to the front of this
    char *tmp2 = *outstr;
    *outstr = (char *) malloc(*size + sizeof(size_t));
    *size += sizeof(size_t);
    (*outstr)[0] = (char) (*size >> 56) & 0xFF;
    (*outstr)[1] = (char) (*size >> 48) & 0xFF;
    (*outstr)[2] = (char) (*size >> 40) & 0xFF;
    (*outstr)[3] = (char) (*size >> 32) & 0xFF;
    (*outstr)[4] = (char) (*size >> 24) & 0xFF;
    (*outstr)[5] = (char) (*size >> 16) & 0xFF;
    (*outstr)[6] = (char) (*size >> 8) & 0xFF;
    (*outstr)[7] = (char) *size & 0xFF;
    
    memcpy((*outstr)+sizeof(size_t), tmp2, *size-sizeof(size_t));
    
}

void unserialize_mapping (const char *instr, size_t *size, r_map **ht)
{
    unsigned int i, loc;
    loc = 0;
    // read the map size
    *size = ((size_t) (instr[loc + 0] & 0xFF) << 56) +
            ((size_t) (instr[loc + 1]& 0xFF)  << 48) +
            ((size_t) (instr[loc + 2]& 0xFF) << 40) +
            ((size_t) (instr[loc + 3] & 0xFF) << 32) +
            ((size_t) (instr[loc + 4]& 0xFF) << 24) +
            ((size_t) (instr[loc + 5]& 0xFF) << 16) +
            ((size_t) (instr[loc + 6]& 0xFF) << 8) +
            (size_t) (instr[loc + 7]& 0xFF);
    loc += sizeof(size_t);
    
    // read the number of entries
    unsigned int num_entries;
    num_entries = ((unsigned int)(instr[loc + 0] & 0xFF) << 24) +
                  ((unsigned int)(instr[loc + 1] & 0xFF) << 16) +
                  ((unsigned int)(instr[loc + 2] & 0xFF) << 8) +
                  (unsigned int)(instr[loc + 3] & 0xFF);
    loc += sizeof(unsigned int);
    

    for( i = 0; i < num_entries; ++i ){
        unsigned int id = ((unsigned int)(instr[loc + 0] & 0xFF) << 24) +
                  ((unsigned int)(instr[loc + 1] & 0xFF) << 16) +
                  ((unsigned int)(instr[loc + 2] & 0xFF) << 8) +
                  (unsigned int)(instr[loc + 3] & 0xFF);
        loc += sizeof(unsigned int);
        unsigned int len = ((unsigned int)(instr[loc + 0] & 0xFF) << 24) +
                  ((unsigned int)(instr[loc + 1] & 0xFF) << 16) +
                  ((unsigned int)(instr[loc + 2] & 0xFF) << 8) +
                  (unsigned int)(instr[loc + 3] & 0xFF);
        loc += sizeof(unsigned int);
        
        
        char *tmp =(char*) malloc(len + 1);
        memcpy(tmp, instr+loc, len);
        
        tmp[len] = '\0';
        
        loc += len;
        
        // read report
        len = ((unsigned int)(instr[loc + 0] & 0xFF) << 24) +
                  ((unsigned int)(instr[loc + 1] & 0xFF) << 16) +
                  ((unsigned int)(instr[loc + 2] & 0xFF) << 8) +
                  (unsigned int)(instr[loc + 3] & 0xFF);
        loc += sizeof(unsigned int);
        
        char *tmp2 = (char*) malloc(len + 1);
        memcpy(tmp2, instr+loc, len);
        
        tmp2[len] = '\0';
        
        insert_mapping(id, tmp, tmp2, ht);
        free(tmp);
        free(tmp2);
        loc += len;
    }
}

#endif
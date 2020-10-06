


#ifndef _UTIL_HASHMAP_H
#define _UTIL_HASHMAP_H

// #include <string.h>
// #include <stdlib.h>
// #include <stdio.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


static int hashmap_def_hash(const void *key)
{
    return key ? (int)((size_t) key) : 0;
}

static int hashmap_def_cmp(const void *o1, const void *o2)
{
    return (int) ((size_t) o1 - (size_t) o2);
}


typedef struct
{

    int capacity;                           /* 16 */
    float load_factor;                      /* 0.75f */
    int access_order;                       /* 0 */
    int (*hash)(const void*);               /* hashmap_def_hash */
    int (*cmp)(const void*, const void*);   /* hashmap_def_cmp */

} hashmap_options;


struct hashmap_entry;
typedef struct hashmap_entry hashmap_entry;


struct hashmap_entry
{
    const void *key;
    const void *value;
    int hash;
    hashmap_entry *next;
    hashmap_entry *before, *after;
};


typedef struct
{
    hashmap_options ops;
    hashmap_entry **tables;
    int size;
    hashmap_entry *head, *tail;

} hashmap;



int hashmap_setup(hashmap *map, hashmap_options *ops);



void hashmap_clear(hashmap *map);


hashmap_entry* hashmap_contains(hashmap *map, const void *key);


const void* hashmap_get(hashmap *map, const void *key);


const void* hashmap_remove(hashmap *map, const void *key, const void **old_key);


const void* hashmap_put(hashmap *map, const void *key, const void *value, const void **old_key);


hashmap_entry** hashmap_entries(hashmap *map, int *pSize);


void** hashmap_values(hashmap *map, int *pSize);


void** hashmap_keys(hashmap* map, int *pSize);


void hashmap_destroy(hashmap *map);


#ifdef __cplusplus
}
#endif

#endif
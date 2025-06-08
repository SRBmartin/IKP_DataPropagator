#ifndef HASHMAP_H
#define HASHMAP_H

#include <stddef.h>
#include <stdbool.h>

typedef struct HashMap HashMap;
typedef size_t(*hashmap_hash_func)(const void* key);
typedef bool   (*hashmap_key_eq_func)(const void* a, const void* b);
typedef void   (*hashmap_free_func)(void* ptr);

HashMap* hashmap_create(size_t capacity,
    hashmap_hash_func hashFunc,
    hashmap_key_eq_func keyEqFunc,
    hashmap_free_func freeKeyFunc,
    hashmap_free_func freeValueFunc);
int       hashmap_put(HashMap* map, void* key, void* value);
void*     hashmap_get(const HashMap* map, const void* key);
bool      hashmap_remove(HashMap* map, const void* key);
bool      hashmap_contains(const HashMap* map, const void* key);
size_t    hashmap_size(const HashMap* map);
void      hashmap_destroy(HashMap* map);

#endif
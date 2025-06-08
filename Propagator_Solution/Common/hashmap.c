#include "hashmap.h"
#include <stdlib.h>

typedef struct HashNode {
    void* key;
    void* value;
    struct HashNode* next;
} HashNode;

struct HashMap {
    HashNode** buckets;
    size_t             capacity;
    size_t             size;
    hashmap_hash_func  hashFunc;
    hashmap_key_eq_func keyEqFunc;
    hashmap_free_func  freeKeyFunc;
    hashmap_free_func  freeValueFunc;
};

static const float MAX_LOAD_FACTOR = 0.75f;

static bool resize_if_needed(HashMap* map) {
    if ((float)(map->size + 1) / (float)map->capacity <= MAX_LOAD_FACTOR)
        return true;

    size_t newCap = map->capacity * 2;
    HashNode** newBuckets = calloc(newCap, sizeof(HashNode*));
    if (!newBuckets) return false;

    for (size_t i = 0; i < map->capacity; i++) {
        HashNode* node = map->buckets[i];
        while (node) {
            HashNode* next = node->next;
            size_t idx = map->hashFunc(node->key) % newCap;
            node->next = newBuckets[idx];
            newBuckets[idx] = node;
            node = next;
        }
    }

    free(map->buckets);
    map->buckets = newBuckets;
    map->capacity = newCap;
    return true;
}

HashMap* hashmap_create(size_t capacity,
    hashmap_hash_func hashFunc,
    hashmap_key_eq_func keyEqFunc,
    hashmap_free_func freeKeyFunc,
    hashmap_free_func freeValueFunc)
{
    if (capacity == 0 || !hashFunc || !keyEqFunc) return NULL;
    HashMap* map = malloc(sizeof(HashMap));
    if (!map) return NULL;
    map->capacity = capacity;
    map->size = 0;
    map->hashFunc = hashFunc;
    map->keyEqFunc = keyEqFunc;
    map->freeKeyFunc = freeKeyFunc;
    map->freeValueFunc = freeValueFunc;
    map->buckets = calloc(capacity, sizeof(HashNode*));
    if (!map->buckets) {
        free(map);
        return NULL;
    }
    return map;
}

int hashmap_put(HashMap* map, void* key, void* value) {
    if (!map || !key) return -1;
    if (!resize_if_needed(map)) return -1;

    size_t idx = map->hashFunc(key) % map->capacity;
    HashNode* node = map->buckets[idx];
    while (node) {
        if (map->keyEqFunc(node->key, key)) {
            if (map->freeValueFunc)
                map->freeValueFunc(node->value);
            node->value = value;
            if (map->freeKeyFunc)
                map->freeKeyFunc(key);
            return 0;
        }
        node = node->next;
    }

    HashNode* newNode = malloc(sizeof(HashNode));
    if (!newNode) return -1;
    newNode->key = key;
    newNode->value = value;
    newNode->next = map->buckets[idx];
    map->buckets[idx] = newNode;
    map->size++;
    return 0;
}

void* hashmap_get(const HashMap* map, const void* key) {
    if (!map || !key) return NULL;
    size_t idx = map->hashFunc(key) % map->capacity;
    HashNode* node = map->buckets[idx];
    while (node) {
        if (map->keyEqFunc(node->key, key))
            return node->value;
        node = node->next;
    }
    return NULL;
}

bool hashmap_remove(HashMap* map, const void* key) {
    if (!map || !key) return false;
    size_t idx = map->hashFunc(key) % map->capacity;
    HashNode* node = map->buckets[idx];
    HashNode* prev = NULL;
    while (node) {
        if (map->keyEqFunc(node->key, key)) {
            if (prev)
                prev->next = node->next;
            else
                map->buckets[idx] = node->next;

            if (map->freeKeyFunc)
                map->freeKeyFunc(node->key);
            if (map->freeValueFunc)
                map->freeValueFunc(node->value);
            free(node);
            map->size--;
            return true;
        }
        prev = node;
        node = node->next;
    }
    return false;
}

bool hashmap_contains(const HashMap* map, const void* key) {
    return hashmap_get(map, key) != NULL;
}

size_t hashmap_size(const HashMap* map) {
    return map ? map->size : 0;
}

void hashmap_destroy(HashMap* map) {
    if (!map) return;
    for (size_t i = 0; i < map->capacity; i++) {
        HashNode* node = map->buckets[i];
        while (node) {
            HashNode* next = node->next;
            if (map->freeKeyFunc)
                map->freeKeyFunc(node->key);
            if (map->freeValueFunc)
                map->freeValueFunc(node->value);
            free(node);
            node = next;
        }
    }
    free(map->buckets);
    free(map);
}
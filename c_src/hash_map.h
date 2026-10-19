#ifndef HASH_MAP_H
#define HASH_MAP_H

#include "common.h"

#define HASH_MAP_NUM_BUCKETS 16

typedef struct HashNode {
    char key[MAX_NAME_LEN]; // Account name
    int tx_count;
    double total_amount;
    int is_flagged;
    struct HashNode *next;
} HashNode;

typedef struct {
    HashNode *buckets[HASH_MAP_NUM_BUCKETS];
    int num_buckets;
    int total_items;
    int collision_count;
} HashMap;

/* Hash Map API */
HashMap* hash_map_create(void);
void hash_map_free(HashMap *hm);
unsigned long hash_function(const char *key);
void hash_map_put(HashMap *hm, const char *key, double amount, int is_flagged);
HashNode* hash_map_get(const HashMap *hm, const char *key);
int hash_map_remove(HashMap *hm, const char *key);
void hash_map_clear(HashMap *hm);

void hash_map_to_json(const HashMap *hm, FILE *out);

#endif /* HASH_MAP_H */

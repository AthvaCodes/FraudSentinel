#include "hash_map.h"

unsigned long hash_function(const char *key) {
    // DJB2 hash algorithm
    unsigned long hash = 5381;
    int c;
    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

HashMap* hash_map_create(void) {
    HashMap *hm = (HashMap *)malloc(sizeof(HashMap));
    if (!hm) return NULL;
    hm->num_buckets = HASH_MAP_NUM_BUCKETS;
    hm->total_items = 0;
    hm->collision_count = 0;
    for (int i = 0; i < HASH_MAP_NUM_BUCKETS; i++) {
        hm->buckets[i] = NULL;
    }
    return hm;
}

void hash_map_free(HashMap *hm) {
    if (!hm) return;
    hash_map_clear(hm);
    free(hm);
}

void hash_map_clear(HashMap *hm) {
    if (!hm) return;
    for (int i = 0; i < hm->num_buckets; i++) {
        HashNode *curr = hm->buckets[i];
        while (curr) {
            HashNode *temp = curr;
            curr = curr->next;
            free(temp);
        }
        hm->buckets[i] = NULL;
    }
    hm->total_items = 0;
    hm->collision_count = 0;
}

void hash_map_put(HashMap *hm, const char *key, double amount, int is_flagged) {
    if (!hm || !key) return;

    unsigned long raw_hash = hash_function(key);
    int bucket = (int)(raw_hash % hm->num_buckets);

    HashNode *curr = hm->buckets[bucket];
    while (curr) {
        if (strcmp(curr->key, key) == 0) {
            curr->tx_count++;
            curr->total_amount += amount;
            if (is_flagged) curr->is_flagged = 1;
            return;
        }
        curr = curr->next;
    }

    // Key not found, insert new node at head of bucket chain
    HashNode *new_node = (HashNode *)malloc(sizeof(HashNode));
    if (!new_node) return;

    safe_strcpy(new_node->key, key, sizeof(new_node->key));
    new_node->tx_count = 1;
    new_node->total_amount = amount;
    new_node->is_flagged = is_flagged;

    if (hm->buckets[bucket] != NULL) {
        hm->collision_count++; // Collision occurred because bucket wasn't empty!
    }

    new_node->next = hm->buckets[bucket];
    hm->buckets[bucket] = new_node;
    hm->total_items++;
}

HashNode* hash_map_get(const HashMap *hm, const char *key) {
    if (!hm || !key) return NULL;
    unsigned long raw_hash = hash_function(key);
    int bucket = (int)(raw_hash % hm->num_buckets);

    HashNode *curr = hm->buckets[bucket];
    while (curr) {
        if (strcmp(curr->key, key) == 0) return curr;
        curr = curr->next;
    }
    return NULL;
}

int hash_map_remove(HashMap *hm, const char *key) {
    if (!hm || !key) return 0;
    unsigned long raw_hash = hash_function(key);
    int bucket = (int)(raw_hash % hm->num_buckets);

    HashNode *curr = hm->buckets[bucket];
    HashNode *prev = NULL;

    while (curr) {
        if (strcmp(curr->key, key) == 0) {
            if (prev == NULL) {
                hm->buckets[bucket] = curr->next;
            } else {
                prev->next = curr->next;
            }
            free(curr);
            hm->total_items--;
            return 1;
        }
        prev = curr;
        curr = curr->next;
    }
    return 0;
}

void hash_map_to_json(const HashMap *hm, FILE *out) {
    if (!hm) {
        fputs("{\"num_buckets\":0,\"total_items\":0,\"buckets\":[]}", out);
        return;
    }

    double load_factor = (double)hm->total_items / hm->num_buckets;

    fputs("{", out);
    fprintf(out, "\"num_buckets\":%d,", hm->num_buckets);
    fprintf(out, "\"total_items\":%d,", hm->total_items);
    fprintf(out, "\"collision_count\":%d,", hm->collision_count);
    fprintf(out, "\"load_factor\":%.3f,", load_factor);
    fputs("\"hash_algorithm\":\"djb2 (hash * 33 + c) % num_buckets\",", out);

    fputs("\"buckets\":[", out);
    for (int i = 0; i < hm->num_buckets; i++) {
        if (i > 0) fputc(',', out);
        fputs("{", out);
        fprintf(out, "\"bucket_index\":%d,", i);

        int chain_length = 0;
        HashNode *curr = hm->buckets[i];
        while (curr) {
            chain_length++;
            curr = curr->next;
        }
        fprintf(out, "\"chain_length\":%d,", chain_length);

        fputs("\"chain\":[", out);
        curr = hm->buckets[i];
        int node_idx = 0;
        while (curr) {
            if (node_idx > 0) fputc(',', out);
            fputs("{", out);
            fputs("\"key\":", out); print_json_escaped_string(out, curr->key); fputc(',', out);
            fprintf(out, "\"raw_hash\":%lu,", hash_function(curr->key));
            fprintf(out, "\"tx_count\":%d,", curr->tx_count);
            fprintf(out, "\"total_amount\":%.2f,", curr->total_amount);
            fprintf(out, "\"is_flagged\":%s", curr->is_flagged ? "true" : "false");
            fputs("}", out);

            curr = curr->next;
            node_idx++;
        }
        fputs("]}", out);
    }
    fputs("]}", out);
}

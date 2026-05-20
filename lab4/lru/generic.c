#include "generic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOAD_FACTOR 0.75
#define GROWTH_FACTOR 2


static unsigned int hashString(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;
    return (unsigned int)hash;
}

LRUCache* createLRUCache(size_t capacity) {
    LRUCache *cache = malloc(sizeof(LRUCache));
    if (!cache) return NULL;

    if (capacity < 16) capacity = 16;
    
    cache->capacity = capacity;
    cache->size = 0;
    cache->buckets = calloc(capacity, sizeof(DoublyLinkedList*));
    
    for (size_t i = 0; i < capacity; ++i) {
        cache->buckets[i] = malloc(sizeof(DoublyLinkedList));
        cache->buckets[i]->head = NULL;
    }
    return cache;
}

static void freeNode(Node *node) {
    if (node) {
        free(node->key);
        free(node);
    }
}

void freeLRUCache(LRUCache* cache) {
    if (!cache) return;
    for (size_t i = 0; i < cache->capacity; ++i) {
        Node *curr = cache->buckets[i]->head;
        while (curr) {
            Node *next = curr->next;
            freeNode(curr);
            curr = next;
        }
        free(cache->buckets[i]);
    }
    free(cache->buckets);
    free(cache);
}


static void moveOneStepTowardsHead(DoublyLinkedList *list, Node *curr) {
    Node *prev = curr->prev;
    if (!prev) return;

    Node *prevPrev = prev->prev;
    Node *next = curr->next;

    if (prevPrev) {
        prevPrev->next = curr;
    } else {
        list->head = curr;
    }
    curr->prev = prevPrev;

    curr->next = prev;
    prev->prev = curr;

    prev->next = next;
    if (next) {
        next->prev = prev;
    }
}


static void rehash(LRUCache *cache) {
    size_t oldCap = cache->capacity;
    size_t newCap = oldCap * GROWTH_FACTOR;
    
    DoublyLinkedList **newBuckets = calloc(newCap, sizeof(DoublyLinkedList*));
    for (size_t i = 0; i < newCap; ++i) {
        newBuckets[i] = malloc(sizeof(DoublyLinkedList));
        newBuckets[i]->head = NULL;
    }

    for (size_t i = 0; i < oldCap; ++i) {
        Node *curr = cache->buckets[i]->head;
        while (curr) {
            Node *next = curr->next;
            
            size_t newIdx = hashString(curr->key) % newCap;
            
            curr->next = newBuckets[newIdx]->head;
            curr->prev = NULL;
            if (newBuckets[newIdx]->head) {
                newBuckets[newIdx]->head->prev = curr;
            }
            newBuckets[newIdx]->head = curr;

            curr = next;
        }
        free(cache->buckets[i]);
    }
    
    free(cache->buckets);
    cache->buckets = newBuckets;
    cache->capacity = newCap;
}

size_t accessItemLRU(LRUCache* cache, const char* key, int* out_val) {
    if ((double)cache->size / cache->capacity > LOAD_FACTOR) {
        rehash(cache);
    }

    size_t idx = hashString(key) % cache->capacity;
    DoublyLinkedList *list = cache->buckets[idx];

    size_t steps = 1;
    Node *curr = list->head;

    while (curr) {
        if (strcmp(curr->key, key) == 0) {
            // НАШЛИ
            curr->value++;
            if (out_val) *out_val = curr->value;

            if (curr->prev) {
                moveOneStepTowardsHead(list, curr);
            }
            return steps;
        }
        steps++;
        curr = curr->next;
    }

    Node *newNode = malloc(sizeof(Node));
    newNode->key = strdup(key);
    newNode->value = 1;
    if (out_val) *out_val = 1;

    
    newNode->next = list->head;
    newNode->prev = NULL;
    if (list->head) {
        list->head->prev = newNode;
    }
    list->head = newNode;
    
    cache->size++;
    
    return 0; 
}

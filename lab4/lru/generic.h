#ifndef LRU_H
#define LRU_H

#include <stddef.h>
#include <stdbool.h>


typedef struct Node {
    char *key;
    int value;
    struct Node *next;
    struct Node *prev;
} Node;


typedef struct {
    Node *head;
} DoublyLinkedList;

typedef struct {
    DoublyLinkedList **buckets;
    size_t capacity;
    size_t size;
    size_t key_size;
    size_t val_size;
} LRUCache;

LRUCache* createLRUCache(size_t capacity);
void freeLRUCache(LRUCache* cache);

size_t accessItemLRU(LRUCache* cache, const char* key, int* out_val);

#endif

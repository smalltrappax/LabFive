#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>

#include "../vector/generic.h"
#include "generic.h"


static size_t slot_size(const HashTable *table)
{
    return (size_t)1 + table->key_size + table->val_size;
}

static unsigned char *get_slot_ptr(HashTable *table, size_t index)
{
    if (!table || !table->values || index >= table->capacity){
        return NULL;
    }
    return (unsigned char *)getVectorItem(table->values, index);
}

static unsigned char *get_slot_state_ptr(HashTable *table, size_t index)
{
    return get_slot_ptr(table, index);
}

static void *get_slot_key_ptr(HashTable *table, size_t index)
{
    unsigned char *slot = get_slot_ptr(table, index);
    if (!slot) return NULL;
    
    return slot + 1;
}

static void *get_slot_val_ptr(HashTable *table, size_t index)
{
    unsigned char *slot = get_slot_ptr(table, index);
    if (!slot) return NULL;
    
    return slot + 1 + table->key_size;
}

static size_t secondary_hash(unsigned int base_hash, size_t capacity)
{
    size_t h = (size_t)base_hash;
    /* значение в диапазоне [1, capacity-1] */
    size_t mod_base = (capacity > 1) ? (capacity - 1) : 1;
    size_t step = 1 + (h % mod_base);
    return step;
}

static void setItemInternal(HashTable *table, void *key, void *data,
                            HashFunc hash, CmpFunc cmp, bool count_collisions);

unsigned int HashInt(const void *key)
{
    if (!key) return 0;

    const int *v = (const int *)key;
    int value = *v;
    if (value < 0)
    {
        value = -value;
    }
    return (unsigned int)value;
}

unsigned int HashString(const void *key)
{
    if (!key) return 0;

    const unsigned char *str = (const unsigned char *)key;
    unsigned long hash = 5381UL;
    int c;

    while ((c = *str++) != 0)
    {
        hash = ((hash << 5) + hash) + (unsigned long)c;
    }

    return (unsigned  int)(hash & 0x7fffffff);
}

HashTable *createHashTable(size_t key_size, size_t val_size)
{
    if (key_size == 0 || val_size == 0) return NULL;

    HashTable *table = (HashTable *)malloc(sizeof(HashTable));
    if (!table) return NULL;

    table->key_size = key_size;
    table->val_size = val_size;
    table->size = 0;
    table->capacity = TABLE_MIN_SIZE;
    table->collisions = 0UL;

    size_t el_size = 1 + key_size + val_size;
    Vector *vec = createVector(el_size);
    if (!vec){
        free(table);
        return NULL;
    }

    /* Заполняем вектор пустыми слотами */
    unsigned char *empty_slot = (unsigned char *)calloc(el_size, 1);
    if (!empty_slot)
    {
        vectorFree(vec);
        free(table);
        return NULL;
    }
    empty_slot[0] = SLOT_EMPTY;

    for (size_t i = 0; i < table->capacity; ++i)
    {
        if (appendVectorItem(vec, empty_slot) != 0)
        {
            free(empty_slot);
            vectorFree(vec);
            free(table);
            return NULL;
        }
    }
    free(empty_slot);

    table->values = vec;
    return table;
}

void rehashHashTable(HashTable *table, HashFunc hash, CmpFunc cmp)
{
    if (!table || !hash || !cmp) return;

    size_t old_capacity = table->capacity;
    Vector *old_values = table->values;

    size_t new_capacity = (old_capacity == 0) ? TABLE_MIN_SIZE : old_capacity * 2;
    size_t el_size = slot_size(table);

    Vector *new_vec = createVector(el_size);
    if (!new_vec) return;

    unsigned char *empty_slot = (unsigned char *)calloc(el_size, 1);
    if (!empty_slot){
        vectorFree(new_vec);
        return;
    }
    empty_slot[0] = SLOT_EMPTY;

    for (size_t i = 0; i < new_capacity; ++i)
    {
        if (appendVectorItem(new_vec, empty_slot) != 0)
        {
            free(empty_slot);
            vectorFree(new_vec);
            return;
        }
    }
    free(empty_slot);

    table->values = new_vec;
    table->capacity = new_capacity;
    table->size = 0;

    for (size_t i = 0; i < old_capacity; ++i)
    {
        unsigned char *state_ptr = (unsigned char *)getVectorItem(old_values, i);
        if (!state_ptr)
        {
            continue;
        }
        if (*state_ptr != SLOT_OCCUPIED)
        {
            continue;
        }

        void *key_ptr = state_ptr + 1;
        void *val_ptr = state_ptr + 1 + table->key_size;

        setItemInternal(table, key_ptr, val_ptr, hash, cmp, false);
    }

    vectorFree(old_values);
}

void setItemHashTable(HashTable *table, void *key, void *data, HashFunc hash, CmpFunc cmp)
{
    setItemInternal(table, key, data, hash, cmp, true);
}

static void setItemInternal(HashTable *table, void *key, void *data,
                            HashFunc hash, CmpFunc cmp, bool count_collisions)
{
    if (!table || !table->values || !key || !data || !hash || !cmp) return;

    if (table->capacity == 0 || (table->size + 1) * 2 > table->capacity)
    {
        rehashHashTable(table, hash, cmp);
    }

    unsigned int base_hash = hash(key);
    size_t capacity = table->capacity;
    size_t h1 = (size_t)base_hash % capacity;
    size_t h2 = secondary_hash(base_hash, capacity);

    ssize_t first_deleted = -1;

    for (size_t i = 0; i < capacity; ++i)
    {
        size_t idx = (h1 + i * h2) % capacity;
        unsigned char *state_ptr = get_slot_state_ptr(table, idx);
        void *slot_key = get_slot_key_ptr(table, idx);
        void *slot_val = get_slot_val_ptr(table, idx);

        if (!state_ptr || !slot_key || !slot_val) return;

        if (*state_ptr == SLOT_EMPTY)
        {
            size_t target_idx = (first_deleted >= 0) ? (size_t)first_deleted : idx;
            unsigned char *t_state = get_slot_state_ptr(table, target_idx);
            void *t_key = get_slot_key_ptr(table, target_idx);
            void *t_val = get_slot_val_ptr(table, target_idx);

            if (!t_state || !t_key || !t_val) return;

            *t_state = SLOT_OCCUPIED;
            memcpy(t_key, key, table->key_size);
            memcpy(t_val, data, table->val_size);
            table->size += 1;
            return;
        }
        else if (*state_ptr == SLOT_OCCUPIED)
        {
            if (cmp(slot_key, key) == 0)
            {
                memcpy(slot_val, data, table->val_size);
                return;
            }
            if (count_collisions)
            {
                table->collisions += 1UL;
            }
        }
        else if (*state_ptr == SLOT_DELETED)
        {
            if (first_deleted < 0)
            {
                first_deleted = (ssize_t)idx;
            }
            if (count_collisions)
            {
                table->collisions += 1UL;
            }
        }
    }

    if (first_deleted >= 0)
    {
        size_t target_idx = (size_t)first_deleted;
        unsigned char *t_state = get_slot_state_ptr(table, target_idx);
        void *t_key = get_slot_key_ptr(table, target_idx);
        void *t_val = get_slot_val_ptr(table, target_idx);

        if (!t_state || !t_key || !t_val) return;
        
        *t_state = SLOT_OCCUPIED;
        memcpy(t_key, key, table->key_size);
        memcpy(t_val, data, table->val_size);
        table->size += 1;
    }
}

void *getItemHashTable(HashTable *table, void *key, HashFunc hash, CmpFunc cmp)
{
    if (!table || !table->values || !key || !hash || !cmp || table->capacity == 0){
        return NULL;
    }

    unsigned int base_hash = hash(key);
    size_t capacity = table->capacity;
    size_t h1 = (size_t)base_hash % capacity;
    size_t h2 = secondary_hash(base_hash, capacity);

    for (size_t i = 0; i < capacity; ++i)
    {
        size_t idx = (h1 + i * h2) % capacity;
        unsigned char *state_ptr = get_slot_state_ptr(table, idx);
        void *slot_key = get_slot_key_ptr(table, idx);
        void *slot_val = get_slot_val_ptr(table, idx);

        if (!state_ptr || !slot_key || !slot_val) return NULL;

        if (*state_ptr == SLOT_EMPTY){
            return NULL;
        }
        else if (*state_ptr == SLOT_OCCUPIED){
            if (cmp(slot_key, key) == 0)
            {
                return slot_val;
            }
        }
        else continue;
    }

    return NULL;
}

void *popItemHashTable(HashTable *table, void *key, HashFunc hash, CmpFunc cmp)
{
    if (!table || !table->values || !key || !hash || !cmp || table->capacity == 0)
    {
        return NULL;
    }

    unsigned int base_hash = hash(key);
    size_t capacity = table->capacity;
    size_t h1 = (size_t)base_hash % capacity;
    size_t h2 = secondary_hash(base_hash, capacity);

    for (size_t i = 0; i < capacity; ++i)
    {
        size_t idx = (h1 + i * h2) % capacity;
        unsigned char *state_ptr = get_slot_state_ptr(table, idx);
        void *slot_key = get_slot_key_ptr(table, idx);
        void *slot_val = get_slot_val_ptr(table, idx);

        if (!state_ptr || !slot_key || !slot_val) return NULL;

        if (*state_ptr == SLOT_EMPTY) return NULL;
        
        else if (*state_ptr == SLOT_OCCUPIED)
        {
            if (cmp(slot_key, key) == 0)
            {
                void *copy = malloc(table->val_size);
                if (!copy)
                {
                    return NULL;
                }
                memcpy(copy, slot_val, table->val_size);
                *state_ptr = SLOT_DELETED;
                if (table->size > 0)
                {
                    table->size -= 1;
                }
                return copy;
            }
        }
        else continue;
    }

    return NULL;
}

unsigned long int getCollisionCount(HashTable *table, HashFunc hash)
{
    if (!table || !table->values || !hash || table->capacity == 0) return 0;

    unsigned long int collisions = 0;

    for (size_t i = 0; i < table->capacity; ++i)
    {
        unsigned char *state_ptr = get_slot_state_ptr(table, i);
        if (!state_ptr || *state_ptr != SLOT_OCCUPIED) continue;

        void *key_ptr = get_slot_key_ptr(table, i);
        if (!key_ptr) continue;

        unsigned int base_hash = hash(key_ptr);
        size_t ideal = (size_t)base_hash % table->capacity;

        if (ideal != i) collisions += 1UL;
    }

    return collisions;
}

void freeHashTable(HashTable *table){
    if (!table) return;

    if (table->values) vectorFree(table->values);
    free(table);
}

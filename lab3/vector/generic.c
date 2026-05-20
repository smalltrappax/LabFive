#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "generic.h"

// Вспомогательная функция для изменения размера
static bool needToResize(Vector *vector, bool *increase)
{
    if (!vector || !increase) return false;

    if (vector->size == vector->capacity)
    {
        *increase = true;
        return true;
    }
    if (vector->capacity > MIN_SIZE && vector->size <= vector->capacity / 4)
    {
        *increase = false;
        return true;
    }
    return false;
}

// Определяем увеличивать размер или уменьшать
static int resize(Vector *vector, bool increase)
{
    if (!vector) return -1;
    
    size_t new_capacity;
    if (increase){
        new_capacity = (vector->capacity == 0) ? MIN_SIZE : vector->capacity * 2;
    }
    else
    {
        new_capacity = vector->capacity / 2;
        if (new_capacity < MIN_SIZE) new_capacity = MIN_SIZE;
        
        if (new_capacity < vector->size) new_capacity = vector->size;
        
    }

    void *new_data = realloc(vector->data, new_capacity * vector->elem_size);
    if (!new_data) return -1;
    
    vector->data = new_data;
    vector->capacity = new_capacity;
    return 0;
}

Vector *createVector(size_t elem_size)
{
    Vector *v = malloc(sizeof(Vector));
    if (!v) return NULL;
    
    v->elem_size = elem_size;
    v->size = 0;
    v->capacity = MIN_SIZE;
    v->data = malloc(v->capacity * v->elem_size);
    if (!v->data)
    {
        free(v);
        return NULL;
    }
    return v;
}

int appendVectorItem(Vector *vector, void *el)
{
    if (!vector || !el) return -1;
    
    bool inc = false;
    if (needToResize(vector, &inc) && inc){
        if (resize(vector, true) != 0) return -1;
    }
    void *dest = (char *)vector->data + vector->size * vector->elem_size;
    memcpy(dest, el, vector->elem_size);
    vector->size += 1;
    return 0;
}

void *getVectorItem(Vector *vector, size_t index)
{
    if (!vector || index >= vector->size) return NULL;
    
    return (char *)vector->data + index * vector->elem_size;
}

int setVectorItem(Vector *vector, size_t index, void *value)
{
    if (!vector || !value || index >= vector->size) return -1;
    
    void *dest = (char *)vector->data + index * vector->elem_size;
    memcpy(dest, value, vector->elem_size);
    return 0;
}

void *popVectorItem(Vector *vector, size_t index)
{
    if (!vector || index >= vector->size)
    {
        return NULL;
    }
    void *src = (char *)vector->data + index * vector->elem_size;
    void *copy = malloc(vector->elem_size);
    if (!copy)
    {
        return NULL;
    }
    memcpy(copy, src, vector->elem_size);

    if (index < vector->size - 1)
    {
        void *from = (char *)vector->data + (index + 1) * vector->elem_size;
        memmove(src, from, (vector->size - index - 1) * vector->elem_size);
    }
    vector->size -= 1;

    bool inc = false;
    if (needToResize(vector, &inc) && !inc)
    {
        (void)resize(vector, false);
    }
    return copy;
}

long int findVectorItem(Vector *vector, void *value, EqualsFunc cmp)
{
    if (!vector || !value) return -1;
    
    for (size_t i = 0; i < vector->size; ++i)
    {
        void *el = (char *)vector->data + i * vector->elem_size;
        int eq;
        if (cmp){
            eq = cmp(el, value);
        }
        else{
            eq = memcmp(el, value, vector->elem_size) == 0;
        }
        if (eq){
            return (long int)i;
        }
    }
    return -1;
}

int vectorFree(Vector *vector)
{
    if (!vector) return -1;
    
    free(vector->data);
    free(vector);
    return 0; 
}

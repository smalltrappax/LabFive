#include "generic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

GenericList *createList(size_t elem_size)
{
    GenericList *list = malloc(sizeof(GenericList));
    if (!list){
        fprintf(stderr, "Ошибка при создании GenericList\n");
        exit(EXIT_FAILURE);
    }
    list->head = NULL;
    list->elem_size = elem_size;
    return list;
}

void appendItem(GenericList *list, void *data)
{
    if (!list || !data) return;

    Node *node = malloc(sizeof(Node));
    if (!node) {
        fprintf(stderr, "Не выделил память для Node\n");
        exit(EXIT_FAILURE);
    }
    node->data = malloc(list->elem_size);
    if (!node->data){
        fprintf(stderr, "не выделил память для node data\n");
        free(node);
        exit(EXIT_FAILURE);
    }
    memcpy(node->data, data, list->elem_size);
    node->next = NULL;

    if (!list->head) {
        list->head = node;
        return;
    }

    Node *cur = list->head;
    while (cur->next){
        cur = cur->next;
    }
    cur->next = node;
}

int findItem(GenericList *list, void *value, EqualsFunc cmp)
{
    if (!list || !value || !cmp) return -1;
    
    int index = 0;
    for (Node *cur = list->head; cur; cur = cur->next, ++index)
    {
        if (cmp(cur->data, value))
        {
            return index;
        }
    }
    return -1;
}

void *popItem(GenericList *list, size_t index)
{
    if (!list || !list->head) return NULL;

    Node *prev = NULL;
    Node *cur = list->head;
    size_t i = 0;
    while (cur && i < index)
    {
        prev = cur;
        cur = cur->next;
        ++i;
    }

    if (!cur) return NULL;

    void *copy = malloc(list->elem_size);
    if (!copy)
    {
        fprintf(stderr, "не удалось выделить память для pop copy\n");
        exit(EXIT_FAILURE);
    }
    memcpy(copy, cur->data, list->elem_size);

    if (prev) prev->next = cur->next;
    else{
        list->head = cur->next;
    }
    // Освобождаем память
    free(cur->data);
    free(cur);
    return copy;
}

void freeList(GenericList *list)
{
    if (!list) return;
    
    Node *cur = list->head;
    while (cur)
    {
        Node *next = cur->next;
        free(cur->data);
        free(cur);
        cur = next;
    }
    free(list);
}

unsigned int listLength(GenericList *list)
{
    if (!list) return 0;
    
    unsigned int count = 0;
    for (Node *cur = list->head; cur; cur = cur->next)
    {
        ++count;
    }
    return count;
}

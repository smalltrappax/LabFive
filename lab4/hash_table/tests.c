#include "generic.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static int CmpInt(const void *a, const void *b){
    int ia = *(const int *)a;
    int ib = *(const int *)b;
    if (ia < ib) return -1;
    if (ia > ib) return 1;
    return 0;
}


static void test_empty_table(){
    printf("Test: empty table ... ");
    HashTable *t = createHashTable(sizeof(int), sizeof(int));
    assert(t);

    int key = 42;
    int *val = (int *)getItemHashTable(t, &key, HashInt, CmpInt);
    assert(val == NULL);

    int *popped = (int *)popItemHashTable(t, &key, HashInt, CmpInt);
    assert(popped == NULL);

    freeHashTable(t);
    printf("OK\n");
}

static void test_basic_insertion()
{
    printf("Test: basic insertion ... ");
    HashTable *t = createHashTable(sizeof(int), sizeof(int));
    assert(t);

    int key = 10, val = 100;
    setItemHashTable(t, &key, &val, HashInt, CmpInt);

    int *res = (int *)getItemHashTable(t, &key, HashInt, CmpInt);
    assert(res && *res == 100);

    freeHashTable(t);
    printf("OK\n");
}

static void test_update_existing(){
    printf("Test: update existing key ... ");
    HashTable *t = createHashTable(sizeof(int), sizeof(int));
    assert(t);

    int key = 5, v1 = 1, v2 = 2;
    setItemHashTable(t, &key, &v1, HashInt, CmpInt);
    setItemHashTable(t, &key, &v2, HashInt, CmpInt);

    int *res = (int *)getItemHashTable(t, &key, HashInt, CmpInt);
    assert(res && *res == 2);

    freeHashTable(t);
    printf("OK\n");
}

static unsigned int BadHashInt(const void *key){
    (void)key;
    return 1;
}

static void test_collisions()
{
    printf("Test: collisions handling ... ");
    HashTable *t = createHashTable(sizeof(int), sizeof(int));
    assert(t);

    int k1 = 1, v1 = 10;
    int k2 = 2, v2 = 20;
    int k3 = 3, v3 = 30;

    setItemHashTable(t, &k1, &v1, BadHashInt, CmpInt);
    setItemHashTable(t, &k2, &v2, BadHashInt, CmpInt);
    setItemHashTable(t, &k3, &v3, BadHashInt, CmpInt);

    int *r1 = (int *)getItemHashTable(t, &k1, BadHashInt, CmpInt);
    int *r2 = (int *)getItemHashTable(t, &k2, BadHashInt, CmpInt);
    int *r3 = (int *)getItemHashTable(t, &k3, BadHashInt, CmpInt);

    assert(r1 && *r1 == 10);
    assert(r2 && *r2 == 20);
    assert(r3 && *r3 == 30);

    unsigned long int col = getCollisionCount(t, BadHashInt);
    assert(col > 0);

    freeHashTable(t);
    printf("OK\n");
}

static void test_rehash()
{
    printf("Test: rehash ... ");
    HashTable *t = createHashTable(sizeof(int), sizeof(int));
    assert(t);

    size_t initial_cap = t->capacity;

    /* Вставляем достаточно элементов, чтобы переполнить >50% */
    for (int i = 0; i < 100; ++i){
        int key = i;
        int val = i * 2;
        setItemHashTable(t, &key, &val, HashInt, CmpInt);
    }

    assert(t->capacity > initial_cap);

    for (int i = 0; i < 100; ++i){
        int key = i;
        int *res = (int *)getItemHashTable(t, &key, HashInt, CmpInt);
        assert(res && *res == i * 2);
    }

    freeHashTable(t);
    printf("OK\n");
}

static void test_delete_and_reinsert()
{
    printf("Test: delete and reinsert ... ");
    HashTable *t = createHashTable(sizeof(int), sizeof(int));
    assert(t);

    int k1 = 1, v1 = 100;
    int k2 = 2, v2 = 200;

    setItemHashTable(t, &k1, &v1, HashInt, CmpInt);
    setItemHashTable(t, &k2, &v2, HashInt, CmpInt);

    int *popped = (int *)popItemHashTable(t, &k1, HashInt, CmpInt);
    assert(popped && *popped == 100);
    free(popped);

    int *res1 = (int *)getItemHashTable(t, &k1, HashInt, CmpInt);
    assert(res1 == NULL);

    int v3 = 300;
    setItemHashTable(t, &k1, &v3, HashInt, CmpInt);

    int *res2 = (int *)getItemHashTable(t, &k1, HashInt, CmpInt);
    assert(res2 && *res2 == 300);

    freeHashTable(t);
    printf("OK\n");
}


static void test_invalid_args()
{
    printf("Test: invalid args ... ");

    HashTable *bad = createHashTable(0, sizeof(int));
    assert(bad == NULL);

    int key = 1;
    int *res = (int *)getItemHashTable(NULL, &key, HashInt, CmpInt);
    assert(res == NULL);

    int *p = (int *)popItemHashTable(NULL, &key, HashInt, CmpInt);
    assert(p == NULL);

    freeHashTable(NULL);

    printf("OK\n");
}

int main(void){
    test_empty_table();
    test_basic_insertion();
    test_update_existing();
    test_collisions();
    test_rehash();
    test_delete_and_reinsert();
    test_invalid_args();

    printf("All hash table tests passed.\n");
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "avl.h"

static int g_count;

static void count_visit(const char* key, Vector* postings, void* ctx) {
    (void)key;
    (void)postings;
    (void)ctx;
    g_count++;
}

int main(void) {
    AVLTree* t = createAVLTree();
    avlInsert(t, "beta", 1, "t1");
    avlInsert(t, "alpha", 2, "t2");
    avlInsert(t, "gamma", 3, "t3");
    avlInsert(t, "alpha", 4, "t2b");

    Vector* a = avlSearch(t, "alpha");
    if (!a || a->size != 2) {
        fprintf(stderr, "alpha postings size fail\n");
        return 1;
    }

    g_count = 0;
    avlTraverse(t, count_visit, NULL);
    if (g_count != 3) {
        fprintf(stderr, "traverse count %d\n", g_count);
        return 1;
    }

    freeAVLTree(t);
    return 0;
}

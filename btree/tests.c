#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "btree.h"

int main(void) {
    BTree* t = createBTree();
    for (int i = 0; i < 20; i++) {
        char k[8];
        snprintf(k, sizeof k, "k%02d", i);
        btreeInsert(t, k, i, "title");
    }
    Vector* v = btreeSearch(t, "k05");
    if (!v || v->size != 1) {
        fprintf(stderr, "btree search fail\n");
        return 1;
    }
    btreeInsert(t, "k05", 99, "dup");
    if (v->size != 2) {
        fprintf(stderr, "btree dup append fail\n");
        return 1;
    }
    freeBTree(t);
    return 0;
}

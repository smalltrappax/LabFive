#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rbtree.h"

int main(void) {
    RBTree* t = createRBTree();
    rbInsert(t, "b", 1, "x");
    rbInsert(t, "a", 2, "y");
    rbInsert(t, "c", 3, "z");
    rbInsert(t, "a", 4, "y2");

    Vector* v = rbSearch(t, "a");
    if (!v || v->size != 2) {
        fprintf(stderr, "rb duplicate fail\n");
        return 1;
    }
    freeRBTree(t);
    return 0;
}

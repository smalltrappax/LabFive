#include "btree.h"
#include <stdlib.h>
#include <string.h>

static BTreeNode* new_node(int leaf) {
    BTreeNode* x = calloc(1, sizeof(BTreeNode));
    if (!x) return NULL;
    x->is_leaf = leaf;
    x->n       = 0;
    for (int i = 0; i < BTREE_MAX_CH; i++) x->children[i] = NULL;
    for (int i = 0; i < BTREE_MAX_KEYS; i++) {
        x->keys[i]      = NULL;
        x->postings[i] = NULL;
    }
    return x;
}

static void free_node(BTreeNode* x) {
    if (!x) return;
    if (!x->is_leaf) {
        for (int i = 0; i <= x->n; i++) free_node(x->children[i]);
    }
    for (int i = 0; i < x->n; i++) {
        free(x->keys[i]);
        if (x->postings[i]) vectorFree(x->postings[i]);
    }
    free(x);
}

BTree* createBTree(void) {
    BTree* t = malloc(sizeof(BTree));
    t->root = new_node(1);
    t->size = 0;
    return t;
}

void freeBTree(BTree* tree) {
    if (!tree) return;
    free_node(tree->root);
    free(tree);
}

static int find_key_index(const BTreeNode* x, const char* key) {
    int i = 0;
    while (i < x->n && strcmp(key, x->keys[i]) > 0) i++;
    return i;
}

Vector* btreeSearch(const BTree* tree, const char* key) {
    if (!tree) return NULL;
    BTreeNode* x = tree->root;
    while (x) {
        int i = find_key_index(x, key);
        if (i < x->n && strcmp(key, x->keys[i]) == 0) return x->postings[i];
        if (x->is_leaf) return NULL;
        x = x->children[i];
    }
    return NULL;
}

/* y = x->children[i] has 2t-1 keys; split into y (t-1) + median in x + z (t-1) */
static void split_child(BTree* tree, BTreeNode* x, int i) {
    (void)tree;
    BTreeNode* y = x->children[i];
    BTreeNode* z = new_node(y->is_leaf);
    z->n         = BTREE_T - 1;

    for (int j = 0; j < BTREE_T - 1; j++) {
        z->keys[j]      = y->keys[j + BTREE_T];
        z->postings[j] = y->postings[j + BTREE_T];
        y->keys[j + BTREE_T]      = NULL;
        y->postings[j + BTREE_T] = NULL;
    }
    if (!y->is_leaf) {
        for (int j = 0; j < BTREE_T; j++) {
            z->children[j] = y->children[j + BTREE_T];
            y->children[j + BTREE_T] = NULL;
        }
    }

    char*   mid_key  = y->keys[BTREE_T - 1];
    Vector* mid_post = y->postings[BTREE_T - 1];
    y->keys[BTREE_T - 1]      = NULL;
    y->postings[BTREE_T - 1] = NULL;
    y->n                     = BTREE_T - 1;

    for (int j = x->n; j >= i + 1; j--) x->children[j + 1] = x->children[j];
    x->children[i + 1] = z;

    for (int j = x->n - 1; j >= i; j--) {
        x->keys[j + 1]      = x->keys[j];
        x->postings[j + 1] = x->postings[j];
    }
    x->keys[i]      = mid_key;
    x->postings[i] = mid_post;
    x->n++;
}

static void insert_non_full(BTree* tree, BTreeNode* x, const char* key, int doc_id,
                            const char* title) {
    int i = find_key_index(x, key);
    if (i < x->n && strcmp(key, x->keys[i]) == 0) {
        appendPosting(x->postings[i], doc_id, title);
        return;
    }

    if (x->is_leaf) {
        for (int j = x->n; j > i; j--) {
            x->keys[j]      = x->keys[j - 1];
            x->postings[j] = x->postings[j - 1];
        }
        x->keys[i]      = strdup(key);
        x->postings[i] = createPostingList();
        appendPosting(x->postings[i], doc_id, title);
        x->n++;
        tree->size++;
        return;
    }

    if (x->children[i]->n == BTREE_MAX_KEYS) {
        split_child(tree, x, i);
        if (strcmp(key, x->keys[i]) > 0) i++;
    }
    insert_non_full(tree, x->children[i], key, doc_id, title);
}

void btreeInsert(BTree* tree, const char* key, int doc_id, const char* title) {
    if (!tree || !key) return;

    BTreeNode* r = tree->root;
    if (r->n == BTREE_MAX_KEYS) {
        BTreeNode* s = new_node(0);
        tree->root     = s;
        s->children[0] = r;
        s->n           = 0;
        split_child(tree, s, 0);
        insert_non_full(tree, s, key, doc_id, title);
    } else {
        insert_non_full(tree, r, key, doc_id, title);
    }
}

static void traverse_node(const BTreeNode* x,
                          void (*visit)(const char* key, Vector* postings, void* ctx),
                          void* ctx) {
    if (!x) return;
    for (int i = 0; i < x->n; i++) {
        if (!x->is_leaf) traverse_node(x->children[i], visit, ctx);
        visit(x->keys[i], x->postings[i], ctx);
    }
    if (!x->is_leaf) traverse_node(x->children[x->n], visit, ctx);
}

void btreeTraverse(const BTree* tree,
                   void (*visit)(const char* key, Vector* postings, void* ctx), void* ctx) {
    if (!tree || !visit) return;
    traverse_node(tree->root, visit, ctx);
}

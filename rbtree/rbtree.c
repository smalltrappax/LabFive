#include "rbtree.h"
#include <stdlib.h>
#include <string.h>

static void left_rotate(RBTree* T, RBNode* x) {
    RBNode* y = x->right;
    x->right    = y->left;
    if (y->left != T->nil) y->left->parent = x;
    y->parent = x->parent;
    if (x->parent == T->nil)
        T->root = y;
    else if (x == x->parent->left)
        x->parent->left = y;
    else
        x->parent->right = y;
    y->left   = x;
    x->parent = y;
}

static void right_rotate(RBTree* T, RBNode* y) {
    RBNode* x = y->left;
    y->left     = x->right;
    if (x->right != T->nil) x->right->parent = y;
    x->parent = y->parent;
    if (y->parent == T->nil)
        T->root = x;
    else if (y == y->parent->right)
        y->parent->right = x;
    else
        y->parent->left = x;
    x->right  = y;
    y->parent = x;
}

static void rb_insert_fixup(RBTree* T, RBNode* z) {
    while (z->parent->color == RB_RED) {
        if (z->parent == z->parent->parent->left) {
            RBNode* y = z->parent->parent->right;
            if (y->color == RB_RED) {
                z->parent->color         = RB_BLACK;
                y->color                 = RB_BLACK;
                z->parent->parent->color   = RB_RED;
                z                        = z->parent->parent;
            } else {
                if (z == z->parent->right) {
                    z = z->parent;
                    left_rotate(T, z);
                }
                z->parent->color       = RB_BLACK;
                z->parent->parent->color = RB_RED;
                right_rotate(T, z->parent->parent);
            }
        } else {
            RBNode* y = z->parent->parent->left;
            if (y->color == RB_RED) {
                z->parent->color         = RB_BLACK;
                y->color                 = RB_BLACK;
                z->parent->parent->color   = RB_RED;
                z                        = z->parent->parent;
            } else {
                if (z == z->parent->left) {
                    z = z->parent;
                    right_rotate(T, z);
                }
                z->parent->color       = RB_BLACK;
                z->parent->parent->color = RB_RED;
                left_rotate(T, z->parent->parent);
            }
        }
    }
    T->root->color = RB_BLACK;
}

static RBNode* search_node(const RBTree* T, const RBNode* x, const char* key) {
    if (x == T->nil) return NULL;
    int cmp = strcmp(key, x->key);
    if (cmp == 0) return (RBNode*)x;
    if (cmp < 0) return search_node(T, x->left, key);
    return search_node(T, x->right, key);
}

static void free_subtree(RBTree* T, RBNode* n) {
    if (!n || n == T->nil) return;
    free_subtree(T, n->left);
    free_subtree(T, n->right);
    free(n->key);
    if (n->postings) vectorFree(n->postings);
    free(n);
}

RBTree* createRBTree(void) {
    RBTree* T     = calloc(1, sizeof(RBTree));
    T->nil        = calloc(1, sizeof(RBNode));
    T->nil->color = RB_BLACK;
    T->nil->left = T->nil->right = T->nil->parent = T->nil;
    T->nil->key      = NULL;
    T->nil->postings = NULL;
    T->root = T->nil;
    T->size = 0;
    return T;
}

void freeRBTree(RBTree* tree) {
    if (!tree) return;
    free_subtree(tree, tree->root);
    free(tree->nil);
    free(tree);
}

static RBNode* new_internal(RBTree* T, const char* key, int doc_id, const char* title) {
    RBNode* z = calloc(1, sizeof(RBNode));
    if (!z) return NULL;
    if (!z) return NULL;
    z->key      = strdup(key);
    z->postings = createPostingList();
    appendPosting(z->postings, doc_id, title);
    z->left = z->right = T->nil;
    z->color           = RB_RED;
    return z;
}

void rbInsert(RBTree* tree, const char* key, int doc_id, const char* title) {
    if (!tree || !key) return;

    RBNode* hit = search_node(tree, tree->root, key);
    if (hit) {
        appendPosting(hit->postings, doc_id, title);
        return;
    }

    RBNode* z = new_internal(tree, key, doc_id, title);
    if (!z) return;

    RBNode* y = tree->nil;
    RBNode* x = tree->root;
    while (x != tree->nil) {
        y = x;
        if (strcmp(z->key, x->key) < 0)
            x = x->left;
        else
            x = x->right;
    }
    z->parent = y;
    if (y == tree->nil)
        tree->root = z;
    else if (strcmp(z->key, y->key) < 0)
        y->left = z;
    else
        y->right = z;

    tree->size++;
    rb_insert_fixup(tree, z);
}

Vector* rbSearch(const RBTree* tree, const char* key) {
    if (!tree) return NULL;
    RBNode* n = search_node(tree, tree->root, key);
    return n ? n->postings : NULL;
}

static void traverse_inorder(const RBTree* T, const RBNode* n,
                             void (*visit)(const char* key, Vector* postings, void* ctx),
                             void* ctx) {
    if (!n || n == T->nil) return;
    traverse_inorder(T, n->left, visit, ctx);
    visit(n->key, n->postings, ctx);
    traverse_inorder(T, n->right, visit, ctx);
}

void rbTraverse(const RBTree* tree,
                void (*visit)(const char* key, Vector* postings, void* ctx), void* ctx) {
    if (!tree || !visit) return;
    traverse_inorder(tree, tree->root, visit, ctx);
}

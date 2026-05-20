#include "avl.h"
#include <stdlib.h>
#include <string.h>

static int max_h(int a, int b) { return a > b ? a : b; }

static int node_height(const AVLNode* n) { return n ? n->height : 0; }

static void refresh_height(AVLNode* n) {
    n->height = 1 + max_h(node_height(n->left), node_height(n->right));
}

static int balance_factor(const AVLNode* n) {
    return n ? node_height(n->left) - node_height(n->right) : 0;
}

static AVLNode* rotate_right(AVLNode* y) {
    AVLNode* x  = y->left;
    AVLNode* t2 = x->right;
    x->right      = y;
    y->left       = t2;
    refresh_height(y);
    refresh_height(x);
    return x;
}

static AVLNode* rotate_left(AVLNode* x) {
    AVLNode* y  = x->right;
    AVLNode* t2 = y->left;
    y->left       = x;
    x->right      = t2;
    refresh_height(x);
    refresh_height(y);
    return y;
}

static AVLNode* balance(AVLNode* node) {
    refresh_height(node);
    int bf = balance_factor(node);
    if (bf > 1) {
        if (balance_factor(node->left) < 0) node->left = rotate_left(node->left);
        return rotate_right(node);
    }
    if (bf < -1) {
        if (balance_factor(node->right) > 0) node->right = rotate_right(node->right);
        return rotate_left(node);
    }
    return node;
}

static void free_node(AVLNode* n) {
    if (!n) return;
    free_node(n->left);
    free_node(n->right);
    free(n->key);
    if (n->postings) vectorFree(n->postings);
    free(n);
}

AVLTree* createAVLTree(void) {
    AVLTree* t = calloc(1, sizeof(AVLTree));
    return t;
}

void freeAVLTree(AVLTree* tree) {
    if (!tree) return;
    free_node(tree->root);
    free(tree);
}

static AVLNode* insert_rec(AVLTree* tree, AVLNode* node, const char* key,
                           int doc_id, const char* title) {
    if (!node) {
        tree->size++;
        AVLNode* nn = calloc(1, sizeof(AVLNode));
        nn->key      = strdup(key);
        nn->postings = createPostingList();
        appendPosting(nn->postings, doc_id, title);
        nn->height = 1;
        return nn;
    }
    int cmp = strcmp(key, node->key);
    if (cmp == 0) {
        appendPosting(node->postings, doc_id, title);
        return node;
    }
    if (cmp < 0)
        node->left = insert_rec(tree, node->left, key, doc_id, title);
    else
        node->right = insert_rec(tree, node->right, key, doc_id, title);
    return balance(node);
}

void avlInsert(AVLTree* tree, const char* key, int doc_id, const char* title) {
    if (!tree || !key) return;
    tree->root = insert_rec(tree, tree->root, key, doc_id, title);
}

static const AVLNode* search_node(const AVLNode* node, const char* key) {
    if (!node) return NULL;
    int cmp = strcmp(key, node->key);
    if (cmp == 0) return node;
    if (cmp < 0) return search_node(node->left, key);
    return search_node(node->right, key);
}

Vector* avlSearch(const AVLTree* tree, const char* key) {
    if (!tree) return NULL;
    const AVLNode* n = search_node(tree->root, key);
    return n ? n->postings : NULL;
}

static void traverse_inorder(const AVLNode* node,
                             void (*visit)(const char* key, Vector* postings, void* ctx),
                             void* ctx) {
    if (!node) return;
    traverse_inorder(node->left, visit, ctx);
    visit(node->key, node->postings, ctx);
    traverse_inorder(node->right, visit, ctx);
}

void avlTraverse(const AVLTree* tree,
                 void (*visit)(const char* key, Vector* postings, void* ctx), void* ctx) {
    if (!tree || !visit) return;
    traverse_inorder(tree->root, visit, ctx);
}

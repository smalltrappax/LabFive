#include "index.h"
#include "../avl/avl.h"
#include "../rbtree/rbtree.h"
#include "../btree/btree.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define strcasecmp _stricmp
#else
#include <strings.h>
#endif

TreeType parseType(const char* s) {
    if (!s) return TREE_AVL;
    if (strcasecmp(s, "avl") == 0) return TREE_AVL;
    if (strcasecmp(s, "rb") == 0 || strcasecmp(s, "rbtree") == 0) return TREE_RB;
    if (strcasecmp(s, "btree") == 0) return TREE_BTREE;
    return TREE_AVL;
}

const char* typeName(TreeType t) {
    switch (t) {
    case TREE_AVL: return "avl";
    case TREE_RB: return "rb";
    case TREE_BTREE: return "btree";
    default: return "avl";
    }
}

Index* createIndex(TreeType type) {
    Index* idx = calloc(1, sizeof(Index));
    idx->type = type;
    switch (type) {
    case TREE_AVL: idx->tree = createAVLTree(); break;
    case TREE_RB: idx->tree = createRBTree(); break;
    case TREE_BTREE: idx->tree = createBTree(); break;
    }
    return idx;
}

void freeIndex(Index* idx) {
    if (!idx) return;
    switch (idx->type) {
    case TREE_AVL: freeAVLTree((AVLTree*)idx->tree); break;
    case TREE_RB: freeRBTree((RBTree*)idx->tree); break;
    case TREE_BTREE: freeBTree((BTree*)idx->tree); break;
    }
    free(idx);
}

void insertTerm(Index* idx, const char* term, int doc_id, const char* title) {
    if (!idx || !term) return;
    switch (idx->type) {
    case TREE_AVL: avlInsert((AVLTree*)idx->tree, term, doc_id, title); break;
    case TREE_RB: rbInsert((RBTree*)idx->tree, term, doc_id, title); break;
    case TREE_BTREE: btreeInsert((BTree*)idx->tree, term, doc_id, title); break;
    }
}

Vector* lookupTerm(const Index* idx, const char* term) {
    if (!idx || !term) return NULL;
    switch (idx->type) {
    case TREE_AVL: return avlSearch((const AVLTree*)idx->tree, term);
    case TREE_RB: return rbSearch((const RBTree*)idx->tree, term);
    case TREE_BTREE: return btreeSearch((const BTree*)idx->tree, term);
    default: return NULL;
    }
}

void indexDocument(Index* idx, int doc_id, const char* title, const char** tokens,
                   int n_tokens) {
    if (!idx || !tokens) return;
    for (int i = 0; i < n_tokens; i++)
        if (tokens[i] && tokens[i][0]) insertTerm(idx, tokens[i], doc_id, title);
}

void traverseIndex(const Index* idx,
                   void (*visit)(const char* key, Vector* postings, void* ctx), void* ctx) {
    if (!idx || !visit) return;
    switch (idx->type) {
    case TREE_AVL: avlTraverse((const AVLTree*)idx->tree, visit, ctx); break;
    case TREE_RB: rbTraverse((const RBTree*)idx->tree, visit, ctx); break;
    case TREE_BTREE: btreeTraverse((const BTree*)idx->tree, visit, ctx); break;
    }
}

typedef struct {
    FILE* f;
} SaveCtx;

static void visit_save(const char* key, Vector* postings, void* ctx) {
    SaveCtx* sc = ctx;
    FILE*      f = sc->f;
    size_t     kl = strlen(key);
    fprintf(f, "%zu\n", kl);
    fwrite(key, 1, kl, f);
    fputc('\n', f);
    fprintf(f, "%zu\n", postings->size);
    for (size_t i = 0; i < postings->size; i++) {
        PostingEntry* e = getVectorItem(postings, i);
        fprintf(f, "%d\t", e->doc_id);
        for (const char* t = e->title; *t; t++)
            fputc((*t == '\n' || *t == '\r') ? ' ' : *t, f);
        fputc('\n', f);
    }
}

void saveIndex(const Index* idx, const char* path) {
    if (!idx || !path) return;
    FILE* f = fopen(path, "w");
    if (!f) {
        perror(path);
        return;
    }
    fprintf(f, "LAB5IDX1\n%s\n", typeName(idx->type));
    SaveCtx sc = {f};
    traverseIndex(idx, visit_save, &sc);
    fprintf(f, "END\n");
    fclose(f);
}

Index* loadIndex(const char* path, TreeType type) {
    FILE* f = fopen(path, "r");
    if (!f) {
        perror(path);
        return NULL;
    }
    char buf[8192];
    if (!fgets(buf, sizeof buf, f) || strncmp(buf, "LAB5IDX1", 8) != 0) {
        fprintf(stderr, "bad index magic\n");
        fclose(f);
        return NULL;
    }
    if (!fgets(buf, sizeof buf, f)) {
        fclose(f);
        return NULL;
    }
    buf[strcspn(buf, "\r\n")] = '\0';
    if (strcasecmp(buf, typeName(type)) != 0) {
        fprintf(stderr, "index type mismatch: file=%s expected=%s\n", buf, typeName(type));
        fclose(f);
        return NULL;
    }

    Index* idx = createIndex(type);

    while (fgets(buf, sizeof buf, f)) {
        buf[strcspn(buf, "\r\n")] = '\0';
        if (strcmp(buf, "END") == 0) break;

        size_t klen = strtoull(buf, NULL, 10);
        char*  key  = malloc(klen + 1);
        if (!key) break;
        if (fread(key, 1, klen, f) != klen) {
            free(key);
            break;
        }
        key[klen] = '\0';
        int nl = fgetc(f);
        if (nl != '\n' && nl != EOF) { /* ignore */ }

        if (!fgets(buf, sizeof buf, f)) {
            free(key);
            break;
        }
        size_t npost = strtoull(buf, NULL, 10);

        for (size_t pi = 0; pi < npost; pi++) {
            if (!fgets(buf, sizeof buf, f)) {
                free(key);
                fclose(f);
                return idx;
            }
            int         doc_id = atoi(buf);
            const char* tab    = strchr(buf, '\t');
            const char* title  = tab ? tab + 1 : "";
            char        tit[MAX_TITLE_LEN];
            strncpy(tit, title, sizeof tit - 1);
            tit[sizeof tit - 1] = '\0';
            tit[strcspn(tit, "\r\n")] = '\0';
            insertTerm(idx, key, doc_id, tit);
        }
        free(key);
    }

    fclose(f);
    return idx;
}

static char* read_line(FILE* fp) {
    size_t cap = 4096, len = 0;
    char*  buf = malloc(cap);
    if (!buf) return NULL;
    int c;
    while ((c = fgetc(fp)) != EOF && c != '\n') {
        if (len + 2 >= cap) {
            cap *= 2;
            char* nb = realloc(buf, cap);
            if (!nb) {
                free(buf);
                return NULL;
            }
            buf = nb;
        }
        buf[len++] = (char)c;
    }
    if (c == EOF && len == 0) {
        free(buf);
        return NULL;
    }
    buf[len] = '\0';
    return buf;
}

static int parse_doc_id(const char* line, int* out_id) {
    const char* p = strstr(line, "\"doc_id\"");
    if (!p) return -1;
    p = strchr(p, ':');
    if (!p) return -1;
    p++;
    while (*p == ' ' || *p == '\t') p++;
    if (*p == '"') p++;
    *out_id = atoi(p);
    return 0;
}

static int json_title(const char* line, char* out, size_t outsz) {
    const char* p = strstr(line, "\"title\"");
    if (!p) {
        out[0] = '\0';
        return 0;
    }
    p = strchr(p, ':');
    if (!p) {
        out[0] = '\0';
        return 0;
    }
    p++;
    while (*p == ' ' || *p == '\t') p++;
    if (*p != '"') {
        out[0] = '\0';
        return 0;
    }
    p++;
    size_t j = 0;
    while (*p && *p != '"' && j + 1 < outsz) {
        if (*p == '\\' && p[1]) {
            p++;
            out[j++] = *p++;
        } else
            out[j++] = *p++;
    }
    out[j] = '\0';
    return 0;
}

static int parse_tokens(const char* line, char tokens[][256], int max_tok) {
    const char* p = strstr(line, "\"tokens\"");
    if (!p) return 0;
    p = strchr(p, '[');
    if (!p) return 0;
    p++;
    int n = 0;
    while (*p && n < max_tok) {
        while (*p && (isspace((unsigned char)*p) || *p == ',')) p++;
        if (*p == ']') break;
        if (*p != '"') {
            p++;
            continue;
        }
        p++;
        int j = 0;
        while (*p && *p != '"' && j < 255) tokens[n][j++] = *p++;
        tokens[n][j] = '\0';
        if (*p == '"') p++;
        if (tokens[n][0]) n++;
    }
    return n;
}

void runIndex(TreeType type, const char* data_jsonl, const char* idx_path) {
    FILE* fp = fopen(data_jsonl, "r");
    if (!fp) {
        perror(data_jsonl);
        return;
    }
    Index* idx = createIndex(type);
    char*  line;
    while ((line = read_line(fp)) != NULL) {
        int   doc_id;
        char  title[MAX_TITLE_LEN];
        char  tokens[256][256];
        const char* tokptr[256];

        if (parse_doc_id(line, &doc_id) != 0) {
            free(line);
            continue;
        }
        json_title(line, title, sizeof title);
        int nt = parse_tokens(line, tokens, 256);
        for (int i = 0; i < nt; i++) tokptr[i] = tokens[i];
        indexDocument(idx, doc_id, title, tokptr, nt);
        free(line);
    }
    fclose(fp);
    saveIndex(idx, idx_path);
    freeIndex(idx);
}

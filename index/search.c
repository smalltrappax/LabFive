#if defined(__linux__) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include "search.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

static double wall_ms(void) {
#ifdef _WIN32
    return (double)GetTickCount64();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
#endif
}

static int is_word_char(unsigned char c) { return c == '_' || isalnum(c); }

static void tokenize_query(const char* query, char terms[][256], int* n_out, int max_terms) {
    int n = 0;
    for (const unsigned char* p = (const unsigned char*)query; *p && n < max_terms;) {
        while (*p && !is_word_char(*p)) p++;
        int j = 0;
        while (*p && is_word_char(*p) && j < 255) {
            terms[n][j++] = (char)tolower(*p);
            p++;
        }
        terms[n][j] = '\0';
        if ((size_t)j > 2) n++;
    }
    *n_out = n;
}

Vector* intersectPostings(Vector** lists, int n) {
    Vector* out = createPostingList();
    if (n <= 0 || !lists) return out;
    if (n == 1) {
        if (!lists[0]) return out;
        return clonePostingList(lists[0]);
    }
    for (int i = 0; i < n; i++)
        if (!lists[i] || lists[i]->size == 0) return out;

    Vector* base = lists[0];
    for (size_t bi = 0; bi < base->size; bi++) {
        PostingEntry* e = getVectorItem(base, bi);
        int           ok = 1;
        for (int li = 1; li < n; li++) {
            Vector* L = lists[li];
            int     found = 0;
            for (size_t j = 0; j < L->size; j++) {
                PostingEntry* ej = getVectorItem(L, j);
                if (ej->doc_id == e->doc_id) {
                    found = 1;
                    break;
                }
            }
            if (!found) {
                ok = 0;
                break;
            }
        }
        if (ok) appendPosting(out, e->doc_id, e->title);
    }
    return out;
}

static int cmp_search_result_doc(const void* a, const void* b) {
    const SearchResult* x = a;
    const SearchResult* y = b;
    if (x->doc_id < y->doc_id) return -1;
    if (x->doc_id > y->doc_id) return 1;
    return 0;
}

SearchResults* search(Index* idx, const char* query) {
    SearchResults* sr = calloc(1, sizeof(SearchResults));
    sr->results       = createVector(sizeof(SearchResult));

    char   terms[64][256];
    int    nt = 0;
    double t0 = wall_ms();
    tokenize_query(query, terms, &nt, 64);
    if (nt == 0) {
        sr->time_ms = wall_ms() - t0;
        return sr;
    }

    Vector* lists[64];
    for (int i = 0; i < nt; i++) {
        lists[i] = lookupTerm(idx, terms[i]);
        if (!lists[i] || lists[i]->size == 0) {
            sr->time_ms = wall_ms() - t0;
            return sr;
        }
    }

    Vector* inter = intersectPostings(lists, nt);
    sr->total     = (int)inter->size;
    sr->time_ms   = wall_ms() - t0;

    Vector* tmp = createVector(sizeof(SearchResult));
    for (size_t i = 0; i < inter->size; i++) {
        PostingEntry* e = getVectorItem(inter, i);
        SearchResult  r;
        r.doc_id = e->doc_id;
        strncpy(r.title, e->title, sizeof r.title - 1);
        r.title[sizeof r.title - 1] = '\0';
        r.score = nt;
        appendVectorItem(tmp, &r);
    }
    vectorFree(inter);

    qsort((char*)tmp->data, tmp->size, tmp->elem_size, cmp_search_result_doc);

    size_t cap = tmp->size < 10 ? tmp->size : 10;
    for (size_t i = 0; i < cap; i++) {
        SearchResult* r = getVectorItem(tmp, i);
        appendVectorItem(sr->results, r);
    }
    vectorFree(tmp);
    return sr;
}

void printResultsText(const SearchResults* sr) {
    if (!sr) return;
    printf("Время: %.1f мс | Найдено: %d документов\n\n", sr->time_ms, sr->total);
    for (size_t i = 0; i < sr->results->size; i++) {
        SearchResult* r = getVectorItem(sr->results, i);
        printf("%2zu. [id=%d] %s\n", i + 1, r->doc_id, r->title);
    }
}

static void json_escape(const char* s, FILE* fp) {
    fputc('"', fp);
    for (; *s; s++) {
        if (*s == '"' || *s == '\\')
            fputc('\\', fp);
        fputc(*s, fp);
    }
    fputc('"', fp);
}

void printResultsJSON(const SearchResults* sr) {
    if (!sr) {
        puts("{}");
        return;
    }
    printf("{\n  \"total\": %d,\n  \"time_ms\": %.3f,\n  \"results\": [\n", sr->total,
           sr->time_ms);
    for (size_t i = 0; i < sr->results->size; i++) {
        SearchResult* r = getVectorItem(sr->results, i);
        printf("    {\"doc_id\": %d, \"title\": ", r->doc_id);
        json_escape(r->title, stdout);
        printf(", \"score\": %d}%s\n", r->score, i + 1 < sr->results->size ? "," : "");
    }
    printf("  ]\n}\n");
}

void freeSearchResults(SearchResults* sr) {
    if (!sr) return;
    if (sr->results) vectorFree(sr->results);
    free(sr);
}

#define LEV_MAX 220

static int levenshtein(const char* a, const char* b) {
    size_t la = strlen(a), lb = strlen(b);
    if (la > LEV_MAX) la = LEV_MAX;
    if (lb > LEV_MAX) lb = LEV_MAX;
    static unsigned short dp[LEV_MAX + 1][LEV_MAX + 1];
    size_t i, j;
    for (i = 0; i <= la; i++) dp[i][0] = (unsigned short)i;
    for (j = 0; j <= lb; j++) dp[0][j] = (unsigned short)j;
    for (i = 1; i <= la; i++) {
        for (j = 1; j <= lb; j++) {
            unsigned short cost = (unsigned char)a[i - 1] == (unsigned char)b[j - 1] ? 0 : 1;
            unsigned short m1     = dp[i - 1][j] + 1;
            unsigned short m2     = dp[i][j - 1] + 1;
            unsigned short m3     = dp[i - 1][j - 1] + cost;
            unsigned short m      = m1 < m2 ? m1 : m2;
            dp[i][j]                = m < m3 ? m : m3;
        }
    }
    return (int)dp[la][lb];
}

typedef struct {
    const char* term;
    int         maxd;
    Vector*     out;
} FuzzyWalkCtx;

static void visit_fuzzy_cb(const char* key, Vector* postings, void* ctx) {
    FuzzyWalkCtx* w = ctx;
    int           d = levenshtein(w->term, key);
    if (d <= w->maxd) {
        FuzzyCandidate c;
        memset(&c, 0, sizeof c);
        strncpy(c.term, key, sizeof c.term - 1);
        c.distance  = d;
        c.postings = postings;
        appendVectorItem(w->out, &c);
    }
}

Vector* fuzzyFindCandidates(Index* idx, const char* term, int max_distance) {
    Vector* out = createVector(sizeof(FuzzyCandidate));
    if (!idx || !term) return out;
    FuzzyWalkCtx w = {term, max_distance, out};
    traverseIndex(idx, visit_fuzzy_cb, &w);
    return out;
}

typedef struct {
    int  doc_id;
    char title[MAX_TITLE_LEN];
    int  min_d[32];
    int  hit[32];
    int  nq;
} DocAgg;

static int find_doc(Vector* v, int doc_id) {
    for (size_t i = 0; i < v->size; i++) {
        DocAgg* d = getVectorItem(v, i);
        if (d->doc_id == doc_id) return (int)i;
    }
    return -1;
}

SearchResults* fuzzySearch(Index* idx, const char* query, int max_distance) {
    SearchResults* sr = calloc(1, sizeof(SearchResults));
    sr->results       = createVector(sizeof(SearchResult));

    char terms[32][256];
    int  nq = 0;
    double t0 = wall_ms();
    tokenize_query(query, terms, &nq, 32);
    if (nq == 0) {
        sr->time_ms = wall_ms() - t0;
        return sr;
    }

    Vector* docs = createVector(sizeof(DocAgg));

    for (int wi = 0; wi < nq; wi++) {
        Vector* cl = fuzzyFindCandidates(idx, terms[wi], max_distance);
        for (size_t ci = 0; ci < cl->size; ci++) {
            FuzzyCandidate* fc = getVectorItem(cl, ci);
            Vector*         pl = fc->postings;
            if (!pl) continue;
            for (size_t pi = 0; pi < pl->size; pi++) {
                PostingEntry* e = getVectorItem(pl, pi);
                int           ix = find_doc(docs, e->doc_id);
                if (ix < 0) {
                    DocAgg da;
                    memset(&da, 0, sizeof da);
                    da.doc_id = e->doc_id;
                    da.nq     = nq;
                    memcpy(da.title, e->title, sizeof da.title - 1);
                    da.title[sizeof da.title - 1] = '\0';
                    da.min_d[wi] = fc->distance;
                    da.hit[wi]   = 1;
                    appendVectorItem(docs, &da);
                } else {
                    DocAgg* da = getVectorItem(docs, (size_t)ix);
                    if (!da->hit[wi] || fc->distance < da->min_d[wi]) {
                        da->min_d[wi] = fc->distance;
                        da->hit[wi]   = 1;
                    }
                }
            }
        }
        vectorFree(cl);
    }

    Vector* scored = createVector(sizeof(SearchResult));
    for (size_t i = 0; i < docs->size; i++) {
        DocAgg* da = getVectorItem(docs, i);
        int     matched = 0, sumd = 0;
        for (int w = 0; w < nq; w++) {
            if (da->hit[w]) {
                matched++;
                sumd += da->min_d[w];
            }
        }
        if (matched == 0) continue;
        int avg_d = (sumd + matched / 2) / matched;
        int sc    = matched * 10 - avg_d;
        SearchResult r;
        r.doc_id = da->doc_id;
        strncpy(r.title, da->title, sizeof r.title - 1);
        r.title[sizeof r.title - 1] = '\0';
        r.score                     = sc;
        appendVectorItem(scored, &r);
    }
    vectorFree(docs);

    for (size_t a = 0; a < scored->size; a++) {
        for (size_t b = a + 1; b < scored->size; b++) {
            SearchResult* ra = getVectorItem(scored, a);
            SearchResult* rb = getVectorItem(scored, b);
            if (rb->score > ra->score || (rb->score == ra->score && rb->doc_id < ra->doc_id)) {
                SearchResult tmp = *ra;
                *ra                = *rb;
                *rb                = tmp;
            }
        }
    }

    sr->total   = (int)scored->size;
    sr->time_ms = wall_ms() - t0;

    size_t cap = scored->size < 10 ? scored->size : 10;
    for (size_t i = 0; i < cap; i++) {
        SearchResult* r = getVectorItem(scored, i);
        appendVectorItem(sr->results, r);
    }
    vectorFree(scored);
    return sr;
}

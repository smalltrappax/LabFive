#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif
#include "index/index.h"
#include "index/search.h"

#if defined(__linux__)
#include <sys/resource.h>
#endif

static void runSearch(TreeType type, const char* idx_path, const char* query, int json_out) {
    Index* idx = loadIndex(idx_path, type);
    if (!idx) {
        fprintf(stderr, "Failed to load index: %s\n", idx_path);
        exit(1);
    }
    SearchResults* sr = search(idx, query);
    if (json_out)
        printResultsJSON(sr);
    else
        printResultsText(sr);
    freeSearchResults(sr);
    freeIndex(idx);
}

static void runFuzzy(TreeType type, const char* idx_path, const char* query, int max_dist,
                     int json_out) {
    Index* idx = loadIndex(idx_path, type);
    if (!idx) {
        fprintf(stderr, "Failed to load index: %s\n", idx_path);
        exit(1);
    }
    SearchResults* sr = fuzzySearch(idx, query, max_dist);
    if (json_out)
        printResultsJSON(sr);
    else
        printResultsText(sr);
    freeSearchResults(sr);
    freeIndex(idx);
}

static double mono_ms(void) {
#ifdef _WIN32
    return (double)GetTickCount64();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
#endif
}

static long peak_rss_kb(void) {
#if defined(__linux__)
    struct rusage ru;
    if (getrusage(RUSAGE_SELF, &ru) == 0) return ru.ru_maxrss;
#endif
    return -1;
}

static void runBench(TreeType type, const char* data_path, int limit, int n_queries) {
    char tmp[512];
    snprintf(tmp, sizeof tmp, "data/bench_tmp_%s.jsonl", typeName(type));
    FILE* w = fopen(tmp, "w");
    if (!w) {
        perror(tmp);
        exit(1);
    }
    FILE* in = fopen(data_path, "r");
    if (!in) {
        perror(data_path);
        fclose(w);
        exit(1);
    }
    char buf[65536];
    int  c = 0;
    while (fgets(buf, sizeof buf, in)) {
        if (limit > 0 && c >= limit) break;
        fputs(buf, w);
        c++;
    }
    fclose(in);
    fclose(w);

    char idxp[512];
    snprintf(idxp, sizeof idxp, "data/bench_idx_%s.txt", typeName(type));

    double t0 = mono_ms();
    runIndex(type, tmp, idxp);
    double idx_ms = mono_ms() - t0;
    long   rss    = peak_rss_kb();

    Index* ld = loadIndex(idxp, type);
    if (!ld) {
        fprintf(stderr, "bench load failed\n");
        remove(tmp);
        exit(1);
    }

    const char* samples[] = {"python", "list", "sort", "memory", "how", "code"};
    int         ns        = (int)(sizeof samples / sizeof samples[0]);

    t0 = mono_ms();
    for (int q = 0; q < n_queries; q++) {
        char qq[128];
        int  wn = 1 + (q % 3);
        qq[0]   = '\0';
        for (int w = 0; w < wn; w++) {
            if (w) strcat(qq, " ");
            strcat(qq, samples[(q + w) % ns]);
        }
        SearchResults* sr = search(ld, qq);
        freeSearchResults(sr);
    }
    double qms = mono_ms() - t0;

    freeIndex(ld);
    remove(tmp);
    remove(idxp);

    printf("bench type=%s docs=%d index_ms=%.1f search_%d_ms=%.1f rss_kb=%ld\n", typeName(type),
           c, idx_ms, n_queries, qms, rss >= 0 ? rss : -1L);
}

static void usage(const char* prog) {
    fprintf(stderr,
            "Usage:\n"
            "  %s index  --type=<avl|rb|btree> [--data=PATH] [--index=PATH]\n"
            "  %s search --type=<avl|rb|btree> [--index=PATH] [--json] \"query\"\n"
            "  %s fuzzysearch --type=... [--index=PATH] [--dist=N] [--json] \"query\"\n"
            "  %s bench   --type=... [--data=PATH] [--limit=N] [--queries=N]\n",
            prog, prog, prog, prog);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        usage(argv[0]);
        return 1;
    }

    const char* mode      = argv[1];
    TreeType    type      = TREE_AVL;
    const char* data_path = "data/processed/docs.jsonl";
    char        idx_path[512] = {0};
    int         json_out = 0;
    const char* query    = NULL;
    int         max_dist = 2;
    int         bench_limit   = 5000;
    int         bench_queries = 1000;

    for (int i = 2; i < argc; i++) {
        if (strncmp(argv[i], "--type=", 7) == 0)
            type = parseType(argv[i] + 7);
        else if (strncmp(argv[i], "--data=", 7) == 0)
            data_path = argv[i] + 7;
        else if (strncmp(argv[i], "--index=", 8) == 0)
            strncpy(idx_path, argv[i] + 8, sizeof idx_path - 1);
        else if (strncmp(argv[i], "--dist=", 7) == 0)
            max_dist = atoi(argv[i] + 7);
        else if (strncmp(argv[i], "--limit=", 9) == 0)
            bench_limit = atoi(argv[i] + 9);
        else if (strncmp(argv[i], "--queries=", 10) == 0)
            bench_queries = atoi(argv[i] + 10);
        else if (strcmp(argv[i], "--json") == 0)
            json_out = 1;
        else if (argv[i][0] != '-')
            query = argv[i];
    }

    if (idx_path[0] == '\0')
        snprintf(idx_path, sizeof idx_path, "data/index_%s.txt", typeName(type));

    if (strcmp(mode, "index") == 0) {
        runIndex(type, data_path, idx_path);
    } else if (strcmp(mode, "search") == 0) {
        if (!query) {
            fprintf(stderr, "No query provided\n");
            return 1;
        }
        runSearch(type, idx_path, query, json_out);
    } else if (strcmp(mode, "fuzzysearch") == 0) {
        if (!query) {
            fprintf(stderr, "No query provided\n");
            return 1;
        }
        runFuzzy(type, idx_path, query, max_dist, json_out);
    } else if (strcmp(mode, "bench") == 0) {
        runBench(type, data_path, bench_limit, bench_queries);
    } else {
        fprintf(stderr, "Unknown mode: %s\n", mode);
        usage(argv[0]);
        return 1;
    }
    return 0;
}

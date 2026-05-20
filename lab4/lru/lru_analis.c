#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "generic.h"

#define BUFFER_SIZE 4096

void trim_newline(char *str) {
    size_t len = strlen(str);
    if (len > 0 && str[len - 1] == '\n') {
        str[len - 1] = '\0';
    }
}

int main() {
    printf("Starting LRU Analysis...\n");

    LRUCache *cache = createLRUCache(1024);
    if (!cache) {
        printf("Failed to create cache\n");
        return 1;
    }

    FILE *file = fopen("../data/hard.csv", "r");
    if (!file) {
        printf("Error: Could not open hard.csv\n");
        freeLRUCache(cache);
        return 1;
    }

    char buffer[BUFFER_SIZE];
    unsigned long long total_ops = 0;

    printf("Processing dataset...\n");
    
    while (fgets(buffer, sizeof(buffer), file)) {
        trim_newline(buffer);
        if (strlen(buffer) == 0) continue;

        int current_val = 0;
        accessItemLRU(cache, buffer, &current_val);
        
        total_ops++;
    }
    fclose(file);
    
    printf("Total operations: %llu\n", total_ops);
    printf("Current Cache Capacity: %zu\n", cache->capacity);

    
    const char *target_keys[] = {
        "https://en.wikipedia.org/wiki/Special:MobileLanguages/Category:Perfect_scores_in_sports",
        "https://en.wikipedia.org/wiki/San_Escobar",
        "https://en.wikipedia.org/wiki/UFC_209",
        "https://en.wikipedia.org/wiki/Blond"
    };

    printf("\n%-90s | %5s | %5s\n", "Key", "Count", "Steps");
    printf("-------------------------------\n");

    for (int i = 0; i < 4; ++i) {
        int count = 0;
        size_t steps = accessItemLRU(cache, target_keys[i], &count);
        
        printf("%-90.90s | %5d | %5zu\n", target_keys[i], count - 1, steps);
    }

    freeLRUCache(cache);
    return 0;
}

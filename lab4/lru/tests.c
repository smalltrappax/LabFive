#include "generic.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

// Вспомогательная функция для очистки кэша между тестами
static void clear_and_free(LRUCache **cache) {
    if (*cache) {
        freeLRUCache(*cache);
        *cache = NULL;
    }
}

// Тест 1: Создание и уничтожение кэша
void test_create_destroy(void) {
    printf("Test 1: Create and destroy cache... ");
    
    LRUCache *cache = createLRUCache(5);
    assert(cache != NULL);
    assert(cache->capacity == 5);
    
    freeLRUCache(cache);
    printf("PASSED\n");
}

// Тест 2: Базовые операции добавления
void test_basic_operations(void) {
    printf("Test 2: Basic operations... ");
    
    LRUCache *cache = createLRUCache(3);
    assert(cache != NULL);
    
    // Добавляем элементы
    char *key1 = "key1";
    char *key2 = "key2";
    char *key3 = "key3";
    
    int val1 = 100;
    int val2 = 200;
    int val3 = 300;
    
    size_t steps;
    
    // Добавление первого элемента
    steps = accessItemLRU(cache, key1, &val1);
    assert(steps == 0); // Не найден, добавлен новый
    assert(cache->size == 1);
    
    // Добавление второго элемента
    steps = accessItemLRU(cache, key2, &val2);
    assert(steps == 0);
    assert(cache->size == 2);
    
    // Добавление третьего элемента
    steps = accessItemLRU(cache, key3, &val3);
    assert(steps == 0);
    assert(cache->size == 3);
    
    freeLRUCache(cache);
    printf("PASSED\n");
}

// Тест 3: Доступ к существующему элементу
void test_access_existing(void) {
    printf("Test 3: Access existing item... ");
    
    LRUCache *cache = createLRUCache(3);
    
    char *key1 = "url1";
    char *key2 = "url2";
    
    int val1 = 100;
    int val2 = 200;
    
    // Добавляем элементы
    accessItemLRU(cache, key1, &val1);
    accessItemLRU(cache, key2, &val2);
    
    // Доступ к существующему элементу
    size_t steps = accessItemLRU(cache, key1, &val1);
    assert(steps > 0); // Должен быть найден за >0 шагов
    assert(steps <= 2); // Но не более чем за 2 шага (максимум в bucket'е)
    
    freeLRUCache(cache);
    printf("PASSED (found in %zu steps)\n", steps);
}

// Тест 4: Вытеснение LRU элемента
void test_lru_eviction(void) {
    printf("Test 4: LRU eviction... ");
    
    LRUCache *cache = createLRUCache(2);
    
    char *keys[] = {"A", "B", "C"};
    int values[] = {1, 2, 3};
    
    // Паттерн: A, B, A, C
    // Ожидание: после C в кэше должны быть A и C, B вытеснен
    
    // Добавляем A
    accessItemLRU(cache, keys[0], &values[0]);
    assert(cache->size == 1);
    
    // Добавляем B
    accessItemLRU(cache, keys[1], &values[1]);
    assert(cache->size == 2);
    
    // Снова обращаемся к A (теперь A становится MRU)
    accessItemLRU(cache, keys[0], &values[0]);
    assert(cache->size == 2);
    
    // Добавляем C - должен вытеснить B (самый старый)
    accessItemLRU(cache, keys[2], &values[2]);
    assert(cache->size == 2); // Размер не должен превысить capacity
    
    // Проверяем, что A всё ещё доступен (не был вытеснен)
    size_t steps_a = accessItemLRU(cache, keys[0], &values[0]);
    assert(steps_a > 0); // A должен быть найден
    
    // Проверяем, что B больше не доступен (был вытеснен)
    int new_val = 999;
    size_t steps_b = accessItemLRU(cache, keys[1], &new_val);
    assert(steps_b == 0); // B не найден, добавлен как новый
    
    freeLRUCache(cache);
    printf("PASSED\n");
}

// Тест 5: Обновление значения существующего ключа
void test_update_value(void) {
    printf("Test 5: Update existing value... ");
    
    LRUCache *cache = createLRUCache(2);
    
    char *key = "test_key";
    int val1 = 100;
    int val2 = 200;
    
    // Добавляем первый раз
    accessItemLRU(cache, key, &val1);
    
    // Обновляем значение
    size_t steps = accessItemLRU(cache, key, &val2);
    assert(steps > 0); // Ключ должен быть найден
    
    // При следующем обращении должно быть новое значение
    // (это можно проверить только если есть функция get без side effects)
    
    freeLRUCache(cache);
    printf("PASSED\n");
}

// Тест 6: Кэш размером 1
void test_cache_size_one(void) {
    printf("Test 6: Cache size = 1... ");
    
    LRUCache *cache = createLRUCache(1);
    
    char *key1 = "first";
    char *key2 = "second";
    
    int val1 = 10;
    int val2 = 20;
    
    // Добавляем первый элемент
    size_t steps1 = accessItemLRU(cache, key1, &val1);
    assert(steps1 == 0);
    assert(cache->size == 1);
    
    // Доступ к тому же элементу
    size_t steps2 = accessItemLRU(cache, key1, &val1);
    assert(steps2 > 0);
    assert(cache->size == 1);
    
    // Добавляем второй элемент - должен вытеснить первый
    size_t steps3 = accessItemLRU(cache, key2, &val2);
    assert(steps3 == 0); // Не найден, добавлен как новый
    assert(cache->size == 1);
    
    // Проверяем, что первого элемента больше нет
    int new_val = 999;
    size_t steps4 = accessItemLRU(cache, key1, &new_val);
    assert(steps4 == 0); // Не найден, добавлен как новый
    
    freeLRUCache(cache);
    printf("PASSED\n");
}

// Тест 7: Множественные обращения к одному элементу
void test_multiple_access_same_key(void) {
    printf("Test 7: Multiple access to same key... ");
    
    LRUCache *cache = createLRUCache(3);
    
    char *key = "hot_key";
    int value = 42;
    
    // Первое добавление
    size_t steps1 = accessItemLRU(cache, key, &value);
    assert(steps1 == 0);
    
    // Многократные обращения
    for (int i = 0; i < 10; i++) {
        size_t steps = accessItemLRU(cache, key, &value);
        assert(steps > 0); // Всегда должен находиться
        assert(steps <= 2); // Должен быть близко к началу списка
    }
    
    // Кэш не должен переполниться
    assert(cache->size == 1);
    
    freeLRUCache(cache);
    printf("PASSED\n");
}

// Тест 8: Чередование ключей
void test_key_alternation(void) {
    printf("Test 8: Key alternation pattern... ");
    
    LRUCache *cache = createLRUCache(2);
    
    char *keys[] = {"A", "B"};
    int values[] = {1, 2};
    
    // Паттерн: A, B, A, B, A, B
    for (int i = 0; i < 6; i++) {
        int idx = i % 2;
        size_t steps = accessItemLRU(cache, keys[idx], &values[idx]);
        
        if (i < 2) {
            assert(steps == 0); // Первые два добавления
        } else {
            assert(steps > 0); // Последующие - должны находиться
        }
    }
    
    // Оба ключа должны быть в кэше
    assert(cache->size == 2);
    
    freeLRUCache(cache);
    printf("PASSED\n");
}

// Тест 9: Проверка сохранения порядка
void test_order_preservation(void) {
    printf("Test 9: Order preservation... ");
    
    LRUCache *cache = createLRUCache(5);
    
    // Добавляем 5 элементов
    for (int i = 0; i < 5; i++) {
        char key[10];
        sprintf(key, "key%d", i);
        int value = i * 100;
        accessItemLRU(cache, key, &value);
    }
    
    // Все 5 должны быть в кэше
    assert(cache->size == 5);
    
    // Обращаемся к key0 - он становится MRU
    char *old_key = "key0";
    int value = 999;
    accessItemLRU(cache, old_key, &value);
    
    // Добавляем новый элемент - должен вытеснить самый старый (key1, если key0 стал MRU)
    char *new_key = "key5";
    int new_value = 500;
    accessItemLRU(cache, new_key, &new_value);
    
    // Проверяем, что key0 всё ещё в кэше (он стал MRU)
    int test_val = 0;
    size_t steps = accessItemLRU(cache, old_key, &test_val);
    assert(steps > 0); // key0 должен быть найден
    
    freeLRUCache(cache);
    printf("PASSED\n");
}

// Тест 10: Граничные случаи
void test_edge_cases(void) {
    printf("Test 10: Edge cases... ");
    
    // Тест с capacity = 0 (должен обрабатываться)
    LRUCache *cache = createLRUCache(0);
    if (cache) {
        // Если создание не должно падать
        char *key = "test";
        int value = 1;
        size_t steps = accessItemLRU(cache, key, &value);
        // Поведение зависит от реализации
        freeLRUCache(cache);
    }
    
    // Тест с NULL параметрами
    cache = createLRUCache(5);
    size_t steps = accessItemLRU(NULL, "key", NULL);
    assert(steps == 0); // Должен безопасно обрабатывать NULL
    
    steps = accessItemLRU(cache, NULL, NULL);
    assert(steps == 0); // Должен безопасно обрабатывать NULL
    
    freeLRUCache(cache);
    printf("PASSED\n");
}

int main(void) {
    
    test_create_destroy();
    test_basic_operations();
    test_access_existing();
    test_lru_eviction();
    test_update_value();
    test_cache_size_one();
    test_multiple_access_same_key();
    test_key_alternation();
    test_order_preservation();
    test_edge_cases();
    
    printf("\n=== All %d tests PASSED ===\n", 10);
    return 0;
}

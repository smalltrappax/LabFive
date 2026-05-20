#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "generic.h"
#include "../base_tasks.h" // для структуры Student
#include "../comparators.h"

static int tests_run = 0;
static int tests_failed = 0;

#define ASSERT_TRUE(cond, msg)                                                                 \
    do                                                                                         \
    {                                                                                          \
        ++tests_run;                                                                           \
        if (!(cond))                                                                           \
        {                                                                                      \
            ++tests_failed;                                                                    \
            printf("[FAIL] %s\n", msg);                                                       \
        }                                                                                      \
    } while (0)

static void test_ints_basic()
{
    GenericList *list = createList(sizeof(int));
    ASSERT_TRUE(list != NULL, "createList вернул non-null");

    int a = 10, b = 20, c = 30;
    appendItem(list, &a);
    appendItem(list, &b);
    appendItem(list, &c);
    ASSERT_TRUE(listLength(list) == 3, "длина после 3 добавлений = 3");

    int q = 20;
    ASSERT_TRUE(findItem(list, &q, intEquals) == 1, "findItem нашел 20 на index 1");

    int *popped_mid = popItem(list, 1);
    ASSERT_TRUE(popped_mid && *popped_mid == 20, "popItem вернул middle 20");
    free(popped_mid);
    ASSERT_TRUE(listLength(list) == 2, "длина после удаления == 2");

    int *popped_head = popItem(list, 0);
    ASSERT_TRUE(popped_head && *popped_head == 10, "pop head вернул 10");
    free(popped_head);

    int *popped_tail = popItem(list, 0);
    ASSERT_TRUE(popped_tail && *popped_tail == 30, "pop last вернул 30");
    free(popped_tail);

    ASSERT_TRUE(listLength(list) == 0, "list empty after removing all");

    // Попытка удалить из пустого
    ASSERT_TRUE(popItem(list, 0) == NULL, "pop frtom empty returns NULL");

    freeList(list);
}

static void test_deep_copy_struct()
{
    GenericList *list = createList(sizeof(Student));
    Student st = {"Alice", 4.5f};
    appendItem(list, &st);

    // Меняем исходную структуру — данные в списке должны остаться прежними
    strcpy(st.name, "ZZZ");
    st.avg = 1.0f;

    Student *in_list = list->head->data;
    ASSERT_TRUE(strcmp(in_list->name, "Alice") == 0, "deep copy preserved name");
    ASSERT_TRUE(in_list->avg == 4.5f, "deep copy preserved среднее");

    freeList(list);
}

static void test_find_not_found()
{
    GenericList *list = createList(sizeof(int));
    int vals[] = {1, 2, 3};
    for (int i = 0; i < 3; ++i)
        appendItem(list, &vals[i]);
    int q = 99;
    ASSERT_TRUE(findItem(list, &q, intEquals) == -1, "findItem вернул -1 for несуществующего элемента");
    freeList(list);
}

int main(void)
{
    test_ints_basic();
    test_deep_copy_struct();
    test_find_not_found();

    if (tests_failed == 0)
    {
        printf("List tests passed (%d)\n", tests_run);
        return 0;
    }
    else
    {
        printf("List tests failed: %d of %d\n", tests_failed, tests_run);
        return 1;
    }
}
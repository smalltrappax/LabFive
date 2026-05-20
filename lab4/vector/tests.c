#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "generic.h"
#include "comparators.h"

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
    Vector *v = createVector(sizeof(int));
    ASSERT_TRUE(v != NULL, "createVector non-null");
    for (int i = 0; i < 100; ++i)
    {
        ASSERT_TRUE(appendVectorItem(v, &i) == 0, "append succceeds");
    }
    ASSERT_TRUE(v->size == 100, "size is 100");

    // get/set
    int *p50 = (int *)getVectorItem(v, 50);
    ASSERT_TRUE(p50 && *p50 == 50, "get index 50 == 50");
    int x = 777;
    ASSERT_TRUE(setVectorItem(v, 50, &x) == 0, "setVectorItem success");
    p50 = (int *)getVectorItem(v, 50);
    ASSERT_TRUE(p50 && *p50 == 777, "value at 50 updated to 777");

    // pop middle
    int *mid = (int *)popVectorItem(v, 50);
    ASSERT_TRUE(mid && *mid == 777, "pop returns correct copy");
    free(mid);
    ASSERT_TRUE(v->size == 99, "size decreased to 99");

    // pop first
    int *first = (int *)popVectorItem(v, 0);
    ASSERT_TRUE(first && *first == 0, "pop first returns 0");
    free(first);

    // pop last
    int *last = (int *)popVectorItem(v, v->size - 1);
    ASSERT_TRUE(last != NULL, "pop lastr non-null");
    free(last);

    // find
    int query = 42;
    long int idx = findVectorItem(v, &query, intEquals);
    ASSERT_TRUE(idx >= 0, "find 42 exists");

    vectorFree(v);
}

static void test_floats_and_strings()
{
    // floats
    Vector *vf = createVector(sizeof(float));
    float fv[] = {1.25f, 2.5f, 3.75f};
    for (size_t i = 0; i < sizeof(fv) / sizeof(fv[0]); ++i)
        appendVectorItem(vf, &fv[i]);
    float look = 2.5f;
    ASSERT_TRUE(findVectorItem(vf, &look, floatEquals) == 1, "find fkloat 2.5 at 1");
    vectorFree(vf);

    // strings (хранит фиксированный размер char[16])
    typedef struct { char s[16]; } Str16;
    Vector *vs = createVector(sizeof(Str16));
    Str16 a = {"hello"}, b = {"world"}, c = {"hello"};
    appendVectorItem(vs, &a);
    appendVectorItem(vs, &b);
    appendVectorItem(vs, &c);

    // find first "world"
    Str16 q = {"world"};
    long int id = findVectorItem(vs, &q, NULL); // memcmp by default
    ASSERT_TRUE(id == 1, "find struct string by memcmp at 1");

    // remove duplicates manually check size after removing middle
    void *p = popVectorItem(vs, 2);
    ASSERT_TRUE(p != NULL, "pop duplicate non-null");
    free(p);
    ASSERT_TRUE(vs->size == 2, "size after pop duplicate == 2");
    vectorFree(vs);
}

int main(void)
{
    test_ints_basic();
    test_floats_and_strings();

    if (tests_failed == 0)
    {
        printf("Vector tests passed (%d)\n", tests_run);
        return 0;
    }
    else
    {
        printf("Vector tests failed: %d of %d\n", tests_failed, tests_run);
        return 1;
    }
}

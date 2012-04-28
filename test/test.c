#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

#include "bitset/operation.h"
#include "bitset/list.h"
#include "test.h"

void bitset_dump(bitset *b) {
    printf("\x1B[33mDumping bitset of size %d\x1B[0m\n", b->length);
    for (unsigned i = 0; i < b->length; i++) {
        printf("\x1B[36m%3d.\x1B[0m %-8x\n", i, b->words[i]);
    }
}

#define TEST_DEFINE(pref, type, format) \
    void test_##pref(char *title, type expected, type value) { \
        if (value != expected) { \
            printf("\x1B[31m%s\x1B[0m", title); \
            printf("   expected '" format "', got '" format "'\n", expected, value); \
            exit(1); \
        } \
    }

TEST_DEFINE(int, int, "%d");
TEST_DEFINE(ulong, unsigned long, "%lu");
TEST_DEFINE(bool, bool, "%d");
TEST_DEFINE(str, char *, "%s");
TEST_DEFINE(hex, int, "%#x");

bool test_bitset(char *title, bitset *b, unsigned length, uint32_t *expected) {
    bool mismatch = length != b->length;
    if (!mismatch) {
        for (unsigned i = 0; i < length; i++) {
            if (b->words[i] != expected[i]) {
                mismatch = true;
                break;
            }
        }
    }
    if (mismatch) {
        unsigned length_max = BITSET_MAX(b->length, length);
        printf("\x1B[31m%s\x1B[0m\n", title);
        for (unsigned i = 0; i < length_max; i++) {
            printf("  \x1B[36m%3d.\x1B[0m ", i);
            if (i < b->length) {
                printf("%-8x ", b->words[i]);
            } else {
                printf("         ");
            }
            if (i < length) {
                printf("\x1B[32m%-8x\x1B[0m", expected[i]);
            }
            putchar('\n');
        }
    }
    return !mismatch;
}

int main(int argc, char **argv) {
    printf("Testing get\n");
    test_suite_get();
    printf("Testing set\n");
    test_suite_set();
    printf("Testing count\n");
    test_suite_count();
    printf("Testing operations\n");
    test_suite_operation();
    printf("Testing min / max\n");
    test_suite_min();
    test_suite_max();
    printf("Testing list\n");
    test_suite_list();
    printf("Testing stress\n");
    test_suite_stress();
}

void test_suite_get() {
    bitset *b = bitset_new();
    for (unsigned i = 0; i < 32; i++)
        test_bool("Testing initial bits are unset\n", false, bitset_get(b, i));
    bitset_free(b);

    uint32_t p1[] = { 0x80000000, BITSET_CREATE_LITERAL(30) };
    b = bitset_new_array(2, p1);
    test_bool("Testing get in the first literal 1\n", true, bitset_get(b, 30));
    test_bool("Testing get in the first literal 2\n", false, bitset_get(b, 31));
    bitset_free(b);

    uint32_t p2[] = { 0x80000000, BITSET_CREATE_LITERAL(0) };
    b = bitset_new_array(2, p2);
    test_bool("Testing get in the first literal 3\n", true, bitset_get(b, 0));
    test_bool("Testing get in the first literal 4\n", false, bitset_get(b, 1));
    bitset_free(b);

    uint32_t p3[] = { 0x80000001, BITSET_CREATE_LITERAL(0) };
    b = bitset_new_array(2, p3);
    test_bool("Testing get in the first literal with offset 1\n", false, bitset_get(b, 1));
    test_bool("Testing get in the first literal with offset 2\n", true, bitset_get(b, 31));
    bitset_free(b);

    uint32_t p4[] = { 0x80000001, 0x80000001, 0x40000000 };
    b = bitset_new_array(3, p4);
    test_bool("Testing get in the first literal with offset 4\n", false, bitset_get(b, 0));
    test_bool("Testing get in the first literal with offset 5\n", false, bitset_get(b, 31));
    test_bool("Testing get in the first literal with offset 6\n", true, bitset_get(b, 62));
    bitset_free(b);

    uint32_t p5[] = { 0x82000001 };
    b = bitset_new_array(1, p5);
    test_bool("Testing get with position following a fill 1\n", false, bitset_get(b, 0));
    test_bool("Testing get with position following a fill 2\n", true, bitset_get(b, 31));
    test_bool("Testing get with position following a fill 3\n", false, bitset_get(b, 32));
    bitset_free(b);
}

void test_suite_count() {
    bitset *b = bitset_new();
    test_ulong("Testing pop count of empty set\n", 0, bitset_count(b));
    bitset_free(b);

    uint32_t p1[] = { 0x80000000, 0x00000001 };
    b = bitset_new_array(2, p1);
    test_ulong("Testing pop count of single literal 1\n", 1, bitset_count(b));
    bitset_free(b);

    uint32_t p2[] = { 0x80000000, 0x11111111 };
    b = bitset_new_array(2, p2);
    test_ulong("Testing pop count of single literal 2\n", 8, bitset_count(b));
    bitset_free(b);

    uint32_t p3[] = { 0x80000001 };
    b = bitset_new_array(1, p3);
    test_ulong("Testing pop count of single fill 1\n", 0, bitset_count(b));
    bitset_free(b);

    uint32_t p8[] = { 0x8C000011 };
    b = bitset_new_array(1, p8);
    test_ulong("Testing pop count of fill with position 1\n", 1, bitset_count(b));
    bitset_free(b);
}

void test_suite_min() {
    bitset *b = bitset_new();
    bitset_set_to(b, 1000, true);
    test_ulong("Test find first set 1", 1000, bitset_min(b));
    bitset_set_to(b, 300, true);
    test_ulong("Test find first set 2", 300, bitset_min(b));
    bitset_set_to(b, 299, true);
    test_ulong("Test find first set 3", 299, bitset_min(b));
    bitset_set_to(b, 298, true);
    test_ulong("Test find first set 4", 298, bitset_min(b));
    bitset_set_to(b, 290, true);
    test_ulong("Test find first set 5", 290, bitset_min(b));
    bitset_set_to(b, 240, true);
    test_ulong("Test find first set 6", 240, bitset_min(b));
    bitset_set_to(b, 12, true);
    test_ulong("Test find first set 7", 12, bitset_min(b));
    bitset_set_to(b, 3, true);
    test_ulong("Test find first set 8", 3, bitset_min(b));
    bitset_free(b);
}

void test_suite_max() {
    bitset *b = bitset_new();
    bitset_set_to(b, 3, true);
    test_ulong("Test find last set 8", 3, bitset_max(b));
    bitset_set_to(b, 12, true);
    test_ulong("Test find last set 7", 12, bitset_max(b));
    bitset_set_to(b, 240, true);
    test_ulong("Test find last set 6", 240, bitset_max(b));
    bitset_set_to(b, 290, true);
    test_ulong("Test find last set 5", 290, bitset_max(b));
    bitset_set_to(b, 298, true);
    test_ulong("Test find last set 4", 298, bitset_max(b));
    bitset_set_to(b, 299, true);
    test_ulong("Test find last set 3", 299, bitset_max(b));
    bitset_set_to(b, 300, true);
    test_ulong("Test find last set 2", 300, bitset_max(b));
    bitset_set_to(b, 1000, true);
    test_ulong("Test find last set 1", 1000, bitset_max(b));
    bitset_free(b);
}

void test_suite_set() {
    bitset *b = bitset_new();
    test_bool("Testing set on empty set 1\n", false, bitset_set_to(b, 0, true));
    test_bool("Testing set on empty set 2\n", true, bitset_get(b, 0));
    test_bool("Testing set on empty set 3\n", false, bitset_get(b, 1));
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing unset on empty set 1\n", false, bitset_set_to(b, 100, false));
    test_ulong("Testing unset on empty set doesn't create it\n", 0, b->length);
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing set on empty set 4\n", false, bitset_set_to(b, 31, true));
    test_bool("Testing set on empty set 5\n", true, bitset_get(b, 31));
    bitset_free(b);

    uint32_t p1[] = { 0x80000001 };
    b = bitset_new_array(1, p1);
    test_bool("Testing append after fill 1\n", false, bitset_set_to(b, 93, true));
    uint32_t e1[] = { 0x80000001, 0x82000002 };
    test_bitset("Testing append after fill 2", b, 2, e1);
    bitset_free(b);

    uint32_t p2[] = { 0x82000001 };
    b = bitset_new_array(1, p2);
    test_bool("Testing append after fill 3\n", false, bitset_set_to(b, 93, true));
    uint32_t e2[] = { 0x82000001, 0x82000001 };
    test_bitset("Testing append after fill 4", b, 2, e2);
    bitset_free(b);

    uint32_t p3[] = { 0x80000001, 0x00000000 };
    b = bitset_new_array(2, p3);
    test_bool("Testing set in literal 1\n", false, bitset_set_to(b, 32, true));
    test_bool("Testing set in literal 2\n", false, bitset_set_to(b, 38, true));
    test_bool("Testing set in literal 3\n", false, bitset_set_to(b, 45, true));
    test_bool("Testing set in literal 4\n", false, bitset_set_to(b, 55, true));
    test_bool("Testing set in literal 5\n", false, bitset_set_to(b, 61, true));
    uint32_t e3[] = { 0x80000001, 0x20810041 };
    test_bitset("Testing set in literal 6", b, 2, e3);
    test_bool("Testing set in literal 7\n", true, bitset_set_to(b, 61, false));
    uint32_t e4[] = { 0x80000001, 0x20810040 };
    test_bitset("Testing set in literal 8", b, 2, e4);
    bitset_free(b);

    uint32_t p5[] = { 0x82000001 };
    b = bitset_new_array(1, p5);
    test_bool("Testing partition of fill 1\n", false, bitset_set_to(b, 32, true));
    uint32_t e5[] = { 0x80000001, 0x60000000 };
    test_bitset("Testing partition of fill 2", b, 2, e5);
    bitset_free(b);

    uint32_t p6[] = { 0x82000001, 0x82000001 };
    b = bitset_new_array(2, p6);
    test_bool("Testing partition of fill 3\n", false, bitset_set_to(b, 32, true));
    uint32_t e6[] = { 0x80000001, 0x60000000, 0x82000001 };
    test_bitset("Testing partition of fill 4", b, 3, e6);
    bitset_free(b);

    uint32_t p7[] = { 0x80000001 };
    b = bitset_new_array(1, p7);
    test_bool("Testing partition of fill 5\n", false, bitset_set_to(b, 31, true));
    uint32_t e7[] = { 0x82000001 };
    test_bitset("Testing partition of fill 6", b, 1, e7);
    bitset_free(b);

    uint32_t p8[] = { 0x82000001, 0x86000001 };
    b = bitset_new_array(2, p8);
    test_bool("Testing partition of fill 7\n", false, bitset_set_to(b, 0, true));
    uint32_t e8[] = { 0x40000000, 0x40000000, 0x86000001 };
    test_bitset("Testing partition of fill 7", b, 3, e8);
    bitset_free(b);

    uint32_t p8b[] = { 0x82000002, 0x86000001 };
    b = bitset_new_array(2, p8b);
    test_bool("Testing partition of fill 7b\n", false, bitset_set_to(b, 32, true));
    uint32_t e8b[] = { 0x84000001, 0x40000000, 0x86000001 };
    test_bitset("Testing partition of fill 7b - 3", b, 3, e8b);
    test_bool("Testing partition of fill 7b - 1\n", true, bitset_get(b, 32));
    test_bool("Testing partition of fill 7b - 2\n", true, bitset_get(b, 62));
    bitset_free(b);

    uint32_t p9[] = { 0x82000003, 0x86000001 };
    b = bitset_new_array(2, p9);
    test_bool("Testing partition of fill 8\n", false, bitset_set_to(b, 32, true));
    uint32_t e9[] = { 0x84000001, 0x82000001, 0x86000001 };
    test_bitset("Testing partition of fill 9", b, 3, e9);
    bitset_free(b);

    uint32_t p10[] = { 0x80000001, 0x82000001 };
    b = bitset_new_array(2, p10);
    test_bool("Testing partition of fill 10\n", false, bitset_set_to(b, 1, true));
    uint32_t e10[] = { 0x20000000, 0x82000001 };
    test_bitset("Testing partition of fill 11", b, 2, e10);
    bitset_free(b);

    uint32_t p11[] = { 0x82000001 };
    b = bitset_new_array(1, p11);
    test_bool("Testing setting position bit 1\n", true, bitset_set_to(b, 31, true));
    test_bitset("Testing setting position bit 2", b, 1, p11);
    uint32_t e11[] = { 0x80000001 };
    test_bool("Testing setting position bit 3\n", true, bitset_set_to(b, 31, false));
    test_bitset("Testing setting position bit 4", b, 1, e11);
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 0, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 36, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 4, true));
    test_bool("Testing random set/get 2\n", true, bitset_get(b, 0));
    test_bool("Testing random set/get 2\n", true, bitset_get(b, 36));
    test_bool("Testing random set/get 2\n", true, bitset_get(b, 4));
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 47, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 58, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 34, true));
    test_bool("Testing random set/get 3\n", true, bitset_get(b, 47));
    test_bool("Testing random set/get 4\n", true, bitset_get(b, 58));
    test_bool("Testing random set/get 5\n", true, bitset_get(b, 34));
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 99, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 85, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 27, true));
    test_bool("Testing random set/get 6\n", true, bitset_get(b, 99));
    test_bool("Testing random set/get 7\n", true, bitset_get(b, 85));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 27));
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 62, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 29, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 26, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 65, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 54, true));
    test_bool("Testing random set/get 6\n", true, bitset_get(b, 62));
    test_bool("Testing random set/get 7\n", true, bitset_get(b, 29));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 26));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 65));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 54));
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 73, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 83, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 70, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 48, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 11, true));
    test_bool("Testing random set/get 6\n", true, bitset_get(b, 73));
    test_bool("Testing random set/get 7\n", true, bitset_get(b, 83));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 70));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 48));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 11));
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 10, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 20, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 96, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 52, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 32, true));
    test_bool("Testing random set/get 6\n", true, bitset_get(b, 10));
    test_bool("Testing random set/get 7\n", true, bitset_get(b, 20));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 96));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 52));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 32));
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 62, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 96, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 55, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 88, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 19, true));
    test_bool("Testing random set/get 6\n", true, bitset_get(b, 62));
    test_bool("Testing random set/get 7\n", true, bitset_get(b, 96));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 55));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 88));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 19));
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 73, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 93, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 14, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 51, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 41, true));
    test_bool("Testing random set/get 6\n", true, bitset_get(b, 73));
    test_bool("Testing random set/get 7\n", true, bitset_get(b, 93));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 14));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 51));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 41));
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 99, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 23, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 45, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 57, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 67, true));
    test_bool("Testing random set/get 6\n", true, bitset_get(b, 99));
    test_bool("Testing random set/get 7\n", true, bitset_get(b, 23));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 45));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 57));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 67));
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 71, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 74, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 94, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 19, true));
    test_bool("Testing random set/get 6\n", true, bitset_get(b, 71));
    test_bool("Testing random set/get 7\n", true, bitset_get(b, 74));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 94));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 19));
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 85, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 25, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 93, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 88, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 54, true));
    test_bool("Testing random set/get 6\n", true, bitset_get(b, 85));
    test_bool("Testing random set/get 7\n", true, bitset_get(b, 25));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 93));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 88));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 54));
    bitset_free(b);

    b = bitset_new();
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 94, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 47, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 79, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 67, true));
    test_bool("Testing random set/get 1\n", false, bitset_set_to(b, 24, true));
    test_bool("Testing random set/get 6\n", true, bitset_get(b, 94));
    test_bool("Testing random set/get 7\n", true, bitset_get(b, 47));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 79));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 67));
    test_bool("Testing random set/get 8\n", true, bitset_get(b, 24));
    bitset_free(b);

#ifdef BITSET_64BIT_OFFSETS
    b = bitset_new(b);
    bitset_set_to(b, 1, true);
    bitset_set_to(b, 1000000000000, true);
    test_bool("Testing set where a chain of fills is required 1\n", true, bitset_get(b, 1));
    test_bool("Testing set where a chain of fills is required 2\n", true, bitset_get(b, 1000000000000));
    test_ulong("Testing set where a chain of fills is required 3\n", 2, bitset_count(b));
    bitset_free(b);
#endif

    //Test unsetting of position bit of 0-colour => fill_length++
    //Test setting & unsetting the position bit of a 1-colour fill word
    //Test append where bit becomes position in 1-colour fill
    //Test append where unset requires break of 1-colour fill position
    //Test partition where 1-colour position has to be split out
}

void test_suite_stress() {
    bitset *b = bitset_new();
    unsigned int max = 100000000, num = 1000;
    unsigned *bits = malloc(sizeof(unsigned) * num);
    srand(time(NULL));
    for (unsigned i = 0; i < num; i++) {
        bits[i] = rand() % max;
        //bits[i] = i;
        bitset_set_to(b, bits[i], true);
    }
    for (unsigned i = 0; i < num; i++) {
        test_bool("Checking stress test bits were set", true, bitset_get(b, bits[i]));
    }
    for (unsigned i = 0; i < 86400; i++) {
        bitset_count(b);
    }
    free(bits);
    bitset_free(b);
}

void test_suite_operation() {
    bitset_operation *ops;
    bitset *b1, *b2, *b3, *b4;

    b1 = bitset_new();
    b2 = bitset_new();
    b3 = bitset_new();
    bitset_set_to(b1, 100, true);
    //bitset_set_to(b1, 138, true);
    //bitset_set_to(b1, 169, true);
    bitset_set_to(b1, 200, true);
    bitset_set_to(b1, 300, true);
    bitset_set_to(b2, 100, true);
    bitset_set_to(b3, 300, true);
    bitset_set_to(b3, 400, true);
    ops = bitset_operation_new(b1);
    bitset_operation_add(ops, b2, BITSET_OR);
    bitset_operation_add(ops, b3, BITSET_OR);
    b4 = bitset_operation_exec(ops);
    test_int("Checking operation regression count 1\n", 4, bitset_count(b4));
    test_bool("Checking operation regression 1\n", true, bitset_get(b4, 100));
    test_bool("Checking operation regression 2\n", true, bitset_get(b4, 200));
    test_bool("Checking operation regression 3\n", true, bitset_get(b4, 300));
    test_bool("Checking opreation regression 4\n", true, bitset_get(b4, 400));
    bitset_operation_free(ops);
    bitset_free(b1);
    bitset_free(b2);
    bitset_free(b3);
    bitset_free(b4);

    b1 = bitset_new();
    bitset_set_to(b1, 10, true);
    b2 = bitset_new();
    bitset_set_to(b2, 20, true);
    b3 = bitset_new();
    bitset_set_to(b3, 12, true);
    ops = bitset_operation_new(b1);
    test_int("Checking initial operation length is one\n", 1, ops->length);
    test_bool("Checking primary bitset is added\n", true, bitset_get(ops->steps[0]->data.b, 10));
    bitset_operation_add(ops, b2, BITSET_OR);
    test_int("Checking op length increases\n", 2, ops->length);
    bitset_operation_add(ops, b3, BITSET_OR);
    test_int("Checking op length increases\n", 3, ops->length);
    test_bool("Checking bitset was added correctly\n", true, bitset_get(ops->steps[1]->data.b, 20));
    test_int("Checking op was added correctly\n", BITSET_OR, ops->steps[1]->type);
    test_bool("Checking bitset was added correctly\n", true, bitset_get(ops->steps[2]->data.b, 12));
    test_int("Checking op was added correctly\n", BITSET_OR, ops->steps[2]->type);
    test_ulong("Checking operation count 1\n", 3, bitset_operation_count(ops));
    bitset_operation_free(ops);
    bitset_free(b1);
    bitset_free(b2);
    bitset_free(b3);

    b1 = bitset_new();
    b2 = bitset_new();
    b3 = bitset_new();
    bitset_set_to(b1, 1000, true);
    bitset_set_to(b2, 100, true);
    bitset_set_to(b3, 20, true);
    ops = bitset_operation_new(b1);
    bitset_operation_add(ops, b2, BITSET_OR);
    bitset_operation_add(ops, b3, BITSET_OR);
    test_ulong("Checking operation count 2\n", 3, bitset_operation_count(ops));
    bitset_operation_free(ops);
    bitset_free(b1);
    bitset_free(b2);
    bitset_free(b3);

    b1 = bitset_new();
    b2 = bitset_new();
    b3 = bitset_new();
    bitset_set_to(b1, 102, true);
    bitset_set_to(b1, 10000, true);
    bitset_set_to(b2, 100, true);
    bitset_set_to(b3, 20, true);
    bitset_set_to(b3, 101, true);
    bitset_set_to(b3, 20000, true);
    ops = bitset_operation_new(b1);
    bitset_operation_add(ops, b2, BITSET_OR);
    bitset_operation_add(ops, b3, BITSET_OR);
    test_ulong("Checking operation count 3\n", 6, bitset_operation_count(ops));
    bitset_operation_free(ops);
    bitset_free(b1);
    bitset_free(b2);
    bitset_free(b3);

    b1 = bitset_new();
    b2 = bitset_new();
    b3 = bitset_new();
    bitset_set_to(b1, 101, true);
    bitset_set_to(b1, 8000, true);
    bitset_set_to(b2, 100, true);
    bitset_set_to(b3, 20, true);
    bitset_set_to(b3, 101, true);
    bitset_set_to(b3, 8001, true);
    ops = bitset_operation_new(b1);
    bitset_operation_add(ops, b2, BITSET_OR);
    bitset_operation_add(ops, b3, BITSET_OR);
    test_ulong("Checking operation count 4\n", 5, bitset_operation_count(ops));
    bitset_operation_free(ops);
    bitset_free(b1);
    bitset_free(b2);
    bitset_free(b3);

    b1 = bitset_new();
    b2 = bitset_new();
    b3 = bitset_new();
    bitset_set_to(b1, 101, true);
    bitset_set_to(b1, 102, true);
    bitset_set_to(b2, 1000, true);
    bitset_set_to(b3, 101, true);
    bitset_set_to(b3, 1000, true);
    ops = bitset_operation_new(b1);
    bitset_operation_add(ops, b2, BITSET_OR);
    bitset_operation_add(ops, b3, BITSET_AND);
    test_ulong("Checking operation count 5\n", 2, bitset_operation_count(ops));
    bitset_operation_free(ops);
    bitset_free(b1);
    bitset_free(b2);
    bitset_free(b3);

    b1 = bitset_new();
    b2 = bitset_new();
    b3 = bitset_new();
    bitset_set_to(b1, 1000, true);
    bitset_set_to(b2, 100, true);
    bitset_set_to(b2, 105, true);
    bitset_set_to(b2, 130, true);
    bitset_set_to(b3, 20, true);
    ops = bitset_operation_new(b1);
    bitset_operation_add(ops, b2, BITSET_OR);
    bitset_operation_add(ops, b3, BITSET_OR);
    b4 = bitset_operation_exec(ops);
    test_ulong("Checking operation exec 1\n", 5, bitset_count(b4));
    test_bool("Checking operation exec get 1\n", true, bitset_get(b4, 1000));
    test_bool("Checking operation exec get 2\n", true, bitset_get(b4, 100));
    test_bool("Checking operation exec get 3\n", true, bitset_get(b4, 105));
    test_bool("Checking operation exec get 4\n", true, bitset_get(b4, 130));
    test_bool("Checking operation exec get 5\n", true, bitset_get(b4, 20));
    bitset_operation_free(ops);
    bitset_free(b1);
    bitset_free(b2);
    bitset_free(b3);
    bitset_free(b4);

    b1 = bitset_new();
    b2 = bitset_new();
    b3 = bitset_new();
    bitset_set_to(b1, 1000, true);
    bitset_set_to(b1, 1001, true);
    bitset_set_to(b1, 1100, true);
    bitset_set_to(b1, 3, true);
    bitset_set_to(b2, 1000, true);
    bitset_set_to(b2, 1101, true);
    bitset_set_to(b2, 3, true);
    bitset_set_to(b2, 130, true);
    bitset_set_to(b3, 1000, true);
    ops = bitset_operation_new(b1);
    bitset_operation_add(ops, b2, BITSET_AND);
    bitset_operation_add(ops, b3, BITSET_ANDNOT);
    b4 = bitset_operation_exec(ops);
    test_bool("Checking operation exec get 6\n", true, bitset_get(b4, 3));
    test_bool("Checking operation exec get 7\n", false, bitset_get(b4, 1000));
    test_bool("Checking operation exec get 8\n", false, bitset_get(b4, 130));
    test_bool("Checking operation exec get 9\n", false, bitset_get(b4, 1001));
    test_bool("Checking operation exec get 10\n", false, bitset_get(b4, 1100));
    test_bool("Checking operation exec get 11\n", false, bitset_get(b4, 1101));
    test_ulong("Checking operation exec 2\n", 1, bitset_count(b4));
    bitset_operation_free(ops);
    bitset_free(b1);
    bitset_free(b2);
    bitset_free(b3);
    bitset_free(b4);

    b1 = bitset_new();
    b2 = bitset_new();
    b3 = bitset_new();
    bitset_set_to(b1, 100, true);
    bitset_set_to(b1, 200, true);
    bitset_set_to(b1, 300, true);
    bitset_set_to(b2, 100, true);
    bitset_set_to(b3, 300, true);
    bitset_set_to(b3, 400, true);
    ops = bitset_operation_new(b1);
    bitset_operation *op2 = bitset_operation_new(b2);
    bitset_operation_add(op2, b3, BITSET_OR);
    bitset_operation_add_nested(ops, op2, BITSET_AND);
    b4 = bitset_operation_exec(ops);
    test_int("Checking nested operation count 1\n", 2, bitset_count(b4));
    test_bool("Checking nested operation get 1\n", true, bitset_get(b4, 100));
    test_bool("Checking nested operation get 2\n", true, bitset_get(b4, 300));
    bitset_operation_free(ops);
    bitset_free(b1);
    bitset_free(b2);
    bitset_free(b3);
    bitset_free(b4);

    b1 = bitset_new();
    b2 = bitset_new();
    b3 = bitset_new();
    bitset_set_to(b1, 100, true);
    bitset_set_to(b1, 200, true);
    bitset_set_to(b1, 300, true);
    bitset_set_to(b2, 100, true);
    bitset_set_to(b3, 300, true);
    bitset_set_to(b3, 400, true);
    ops = bitset_operation_new(b1);
    op2 = bitset_operation_new(b2);
    bitset_operation_add(op2, b3, BITSET_OR);
    bitset_operation_add_nested(ops, op2, BITSET_OR);
    b4 = bitset_operation_exec(ops);
    test_int("Checking nested operation count 2\n", 4, bitset_count(b4));
    test_bool("Checking nested operation get 3\n", true, bitset_get(b4, 100));
    test_bool("Checking nested operation get 4\n", true, bitset_get(b4, 200));
    test_bool("Checking nested operation get 5\n", true, bitset_get(b4, 300));
    test_bool("Checking nested operation get 6\n", true, bitset_get(b4, 400));
    bitset_operation_free(ops);
    bitset_free(b1);
    bitset_free(b2);
    bitset_free(b3);
    bitset_free(b4);

#ifdef BITSET_64BIT_OFFSETS
    b1 = bitset_new();
    b2 = bitset_new();
    bitset_set_to(b1, 1, true);
    bitset_set_to(b2, 10000000000, true);
    bitset_set_to(b2, 100000000000, true);
    ops = bitset_operation_new(b1);
    bitset_operation_add(ops, b2, BITSET_OR);
    b4 = bitset_operation_exec(ops);
    test_ulong("Checking operation exec 2\n", 3, bitset_count(b4));
    test_bool("Checking operation exec get 12\n", true, bitset_get(b4, 1));
    test_bool("Checking operation exec get 12\n", true, bitset_get(b4, 10000000000));
    test_bool("Checking operation exec get 13\n", true, bitset_get(b4, 100000000000));
    bitset_operation_free(ops);
    bitset_free(b1);
    bitset_free(b2);
    bitset_free(b4);
#endif
}

void test_suite_list() {
    bitset_list *l;
    bitset_list_iterator *i, *i2;
    bitset *b;
    bitset_word *tmp;
    unsigned loop_count;
    unsigned offset;

    l = bitset_list_new();
    test_int("Checking list length is zero initially\n", 0, bitset_list_length(l));
    test_int("Checking list count is zero initially\n", 0, bitset_list_count(l));
    test_int("Checking list size is zero initially\n", 0, l->size);
    test_int("Checking list tail offset is zero initially\n", 0, l->tail_offset);
    i = bitset_list_iterator_new(l, BITSET_LIST_START, BITSET_LIST_END);
    loop_count = 0;
    BITSET_LIST_FOREACH(i, b, offset) {
        loop_count++;
    }
    test_int("Checking an empty iterator is safe to use with foreach\n", 0, loop_count);
    bitset_list_iterator_free(i);
    bitset_list_free(l);

    l = bitset_list_new();
    b = bitset_new();
    bitset_list_push(l, b, 0);
    test_int("Checking list was resized properly\n", 2, l->size);
    test_int("Checking list was resized properly\n", 2, l->length);
    test_int("Checking list was resized properly\n", 1, l->count);
    test_int("Checking the offset is zero\n", 0, (unsigned char)l->buffer[0]);
    test_int("Checking the length is zero\n", 0, (unsigned char)l->buffer[1]);
    bitset_free(b);
    bitset_list_free(l);

    l = bitset_list_new();
    b = bitset_new();
    bitset_set_to(b, 10, true);
    bitset_list_push(l, b, 3);
    test_int("Checking list was resized properly 1\n", 8, l->size);
    test_int("Checking list was resized properly 2\n", 6, l->length);
    test_int("Checking list was resized properly 3\n", 1, l->count);
    test_int("Checking the offset is set properly 1\n", 3, (unsigned char)l->buffer[0]);
    test_int("Checking the length is set properly 1\n", 1, (unsigned char)l->buffer[1]);
    tmp = b->words;
    b->words = (bitset_word *) (l->buffer + 2);
    test_bool("Checking bitset was added properly 1\n", true, bitset_get(b, 10));
    test_bool("Checking bitset was added properly 2\n", false, bitset_get(b, 100));
    b->words = tmp;
    bitset_free(b);

    b = bitset_new();
    bitset_set_to(b, 100, true);
    bitset_set_to(b, 1000, true);
    bitset_list_push(l, b, 10);
    test_int("Checking list was resized properly 4\n", 16, l->size);
    test_int("Checking list was resized properly 5\n", 16, l->length);
    test_int("Checking list was resized properly 6\n", 2, l->count);
    test_int("Checking the offset is set properly 2\n", 7, (unsigned char)l->buffer[6]);
    test_int("Checking the length is set properly 2\n", 2, (unsigned char)l->buffer[7]);
    test_int("Check tail offset is set correctly\n", 10, l->tail_offset);
    test_int("Check tail ptr is set correctly\n", (uintptr_t)l->buffer+6, (uintptr_t)l->tail);
    tmp = b->words;
    b->words = (bitset_word *) (l->buffer + 8);
    test_bool("Checking bitset was added properly 3\n", true, bitset_get(b, 100));
    test_bool("Checking bitset was added properly 4\n", true, bitset_get(b, 1000));
    test_bool("Checking bitset was added properly 5\n", false, bitset_get(b, 10));
    b->words = tmp;
    bitset_free(b);

    i = bitset_list_iterator_new(l, BITSET_LIST_START, BITSET_LIST_END);
    test_bool("Checking bitset was added properly to iter 1\n", true, bitset_get(i->bitsets[0], 10));
    test_bool("Checking bitset was added properly to iter 2\n", false, bitset_get(i->bitsets[0], 100));
    test_bool("Checking bitset was added properly to iter 3\n", false, bitset_get(i->bitsets[1], 10));
    test_bool("Checking bitset was added properly to iter 4\n", true, bitset_get(i->bitsets[1], 100));
    test_bool("Checking bitset was added properly to iter 5\n", true, bitset_get(i->bitsets[1], 1000));
    loop_count = 0;
    BITSET_LIST_FOREACH(i, b, offset) {
        loop_count++;
        test_bool("Checking foreach works\n", true, offset == 3 || offset == 10);
        if (offset == 3) {
            test_bool("Checking bitset was added properly to iter 6\n", true, bitset_get(b, 10));
            test_bool("Checking bitset was added properly to iter 7\n", false, bitset_get(b, 100));
        } else if (offset == 10) {
            test_bool("Checking bitset was added properly to iter 8\n", false, bitset_get(i->bitsets[1], 10));
            test_bool("Checking bitset was added properly to iter 9\n", true, bitset_get(i->bitsets[1], 100));
            test_bool("Checking bitset was added properly to iter 10\n", true, bitset_get(i->bitsets[1], 1000));
        }
    }
    test_int("Checking it looped the right number of times\n", 2, loop_count);
    bitset_list_iterator_free(i);

    i = bitset_list_iterator_new(l, 3, 10);
    test_bool("Checking bitset was added properly to iter 11\n", false, bitset_get(i->bitsets[0], 100));
    loop_count = 0;
    BITSET_LIST_FOREACH(i, b, offset) {
        loop_count++;
        test_bool("Checking foreach works 2\n", true, offset == 3);
    }
    test_int("Checking it looped the right number of times 2\n", 1, loop_count);
    bitset_list_iterator_free(i);

    i = bitset_list_iterator_new(l, 4, 5);
    loop_count = 0;
    BITSET_LIST_FOREACH(i, b, offset) {
        loop_count++;
    }
    test_int("Checking it looped the right number of times 3\n", 0, loop_count);
    bitset_list_iterator_free(i);

    i = bitset_list_iterator_new(l, BITSET_LIST_START, BITSET_LIST_END);
    i2 = bitset_list_iterator_new(l, BITSET_LIST_START, BITSET_LIST_END);
    bitset_list_iterator_concat(i, i2, 10);
    loop_count = 0;
    BITSET_LIST_FOREACH(i, b, offset) {
        loop_count++;
        test_bool("Checking foreach works 3\n", true, offset == 3 ||
            offset == 10 || offset == 13 || offset == 20);
    }
    test_int("Checking it looped the right number of times 4\n", 4, loop_count);
    bitset_list_iterator_free(i);

    //Make a copy of the buffer
    char *buffer = malloc(sizeof(char) * l->length);
    memcpy(buffer, l->buffer, l->length);
    unsigned length = l->length;

    bitset_list_free(l);

    //Check the copy is the same
    l = bitset_list_new_buffer(length, buffer);
    test_int("Check size is copied\n", 16, l->size);
    test_int("Check length is copied\n", 16, l->length);
    test_int("Check count is copied\n", 2, l->count);
    test_int("Check tail offset is set correctly\n", 10, l->tail_offset);
    test_int("Check tail ptr is set correctly\n", (uintptr_t)l->buffer+6, (uintptr_t)l->tail);
    bitset_list_free(l);
}


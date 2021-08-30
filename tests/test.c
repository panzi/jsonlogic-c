// for fileno()
#define _POSIX_C_SOURCE 1

#include "jsonlogic.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <inttypes.h>

struct TestContext;

typedef void (*TestFunc)(struct TestContext *);

typedef struct TestCase {
    const char *name;
    TestFunc func;
} TestCase;

typedef struct TestContext {
    const TestCase *test_case;
    size_t testnr;
    bool newline;
    bool passed;
} TestContext;

#define TEST_STR_(CODE) (#CODE)
#define TEST_STR(CODE) TEST_STR_(CODE)

#define TEST_FAIL() \
    { \
        test_context->passed = false; \
        if (!test_context->newline) { \
            puts("FAILED"); \
            fflush(stdout); \
            test_context->newline = true; \
        } \
    }

#define TEST_ASSERT(EXPR) \
    if (!(EXPR)) { \
        TEST_FAIL(); \
        fprintf(stderr, "%s:%u: Assertion failed: %s\n", \
            __FILE__, __LINE__, TEST_STR(EXPR)); \
        goto cleanup; \
    }

#define TEST_ASSERT_MSG(EXPR, MSG) \
    if (!(EXPR)) { \
        TEST_FAIL(); \
        fprintf(stderr, "%s:%u: Assertion failed: %s\n", \
            __FILE__, __LINE__, MSG); \
        goto cleanup; \
    }

#define TEST_ASSERT_FMT(EXPR, MSG, ...) \
    if (!(EXPR)) { \
        TEST_FAIL(); \
        fprintf(stderr, "%s:%u: Assertion failed: " MSG "\n", \
            __FILE__, __LINE__, __VA_ARGS__); \
        goto cleanup; \
    }

#define TEST_ASSERT_X(EXPR, CODE) \
    if (!(EXPR)) { \
        TEST_FAIL(); \
        fprintf(stderr, "%s:%u: Assertion failed: ", \
            __FILE__, __LINE__); \
        CODE; \
        goto cleanup; \
    }

#define TEST_DECL(NAME, FUNC) \
    { .name = NAME, .func = test_##FUNC }

#define TEST_END { .name = NULL, .func = NULL }

void test_bad_operator(TestContext *test_context) {
    JsonLogic_Handle logic  = jsonlogic_parse("{\"fubar\": []}", NULL);
    JsonLogic_Handle result = jsonlogic_apply(
        logic,
        JsonLogic_Null
    );

    TEST_ASSERT(result == JSONLOGIC_ERROR_ILLEGAL_OPERATION);

cleanup:
    jsonlogic_decref(result);
    jsonlogic_decref(logic);
}

const TestCase TEST_CASES[] = {
    TEST_DECL("Bad operator", bad_operator),
    TEST_END,
};

int main(int argc, char *argv[]) {
    if (argc != 2) {
        const char *progname = argc > 0 ? argv[0] : "test";
        fprintf(stderr, "usage: %s <path/to/tests.json>\n", progname);
        return 1;
    }

    const char *filename = argv[1];
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        perror(filename);
        return 1;
    }

    struct stat stbuf;
    if (fstat(fileno(fp), &stbuf) != 0) {
        fclose(fp);
        perror(filename);
        return 1;
    }

    char *tests_json = malloc(stbuf.st_size);
    if (tests_json == NULL) {
        fclose(fp);
        perror(filename);
        return 1;
    }

    if (fread(tests_json, stbuf.st_size, 1, fp) != 1) {
        free(tests_json);
        fclose(fp);
        perror(filename);
        return 1;
    }

    fclose(fp);

    JsonLogic_LineInfo info = JSONLOGIC_LINEINFO_INIT;
    JsonLogic_Handle tests = jsonlogic_parse_sized(tests_json, stbuf.st_size, &info);

    JsonLogic_Error error = jsonlogic_get_error(tests);
    if (error != JSONLOGIC_ERROR_SUCCESS) {
        jsonlogic_print_parse_error_sized(stderr, tests_json, stbuf.st_size, error, info);
        free(tests_json);
        return 1;
    }
    free(tests_json);
    tests_json = NULL;

    JsonLogic_Iterator iter = jsonlogic_iter(tests);

    TestContext test_context = {
        .test_case = NULL,
        .newline   = false,
        .passed    = true,
        .testnr    = 0,
    };
    size_t test_count = 0;
    size_t pass_count = 0;

    for (const TestCase *test = TEST_CASES; test->func; ++ test) {
        ++ test_count;
        printf(" - %s ... ", test->name);
        fflush(stdout);
        test_context.test_case = test;
        test_context.testnr    = test_count;
        test_context.newline   = false;
        test_context.passed    = true;

        test->func(&test_context);

        if (!test_context.newline) {
            puts(test_context.passed ? "OK" : "FAILED");
            fflush(stdout);
            test_context.newline = true;
        }

        if (test_context.passed) {
            ++ pass_count;
        }
    }

    test_context.test_case = NULL;
    test_context.testnr    = 0;
    test_context.newline   = true;
    test_context.passed    = true;

    #define FAIL() \
        if (!test_context.newline) { \
            puts("FAILED"); \
            fflush(stdout); \
            test_context.newline = true; \
        } \
        fprintf(stderr, "      test: %" PRIuPTR "\n", test_count);

    for (;;) {
        JsonLogic_Handle test = jsonlogic_iter_next(&iter);

        if (test == JSONLOGIC_ERROR_STOP_ITERATION) {
            if (!test_context.newline) {
                puts("OK");
                fflush(stdout);
                test_context.newline = false;
            }
            break;
        }

        if (jsonlogic_is_string(test)) {
            if (!test_context.newline) {
                puts("OK");
            }
            size_t size = 0;
            const char16_t *str = jsonlogic_get_string_content(test, &size);
            printf(" - "); jsonlogic_print_utf16(stdout, str, size); printf(" ... ");
            fflush(stdout);
            test_context.newline = false;
        } else {
            ++ test_count;

            JsonLogic_Handle logic    = jsonlogic_get(test, jsonlogic_number_from(0));
            JsonLogic_Handle data     = jsonlogic_get(test, jsonlogic_number_from(1));
            JsonLogic_Handle expected = jsonlogic_get(test, jsonlogic_number_from(2));

            if (jsonlogic_is_error(logic)) {
                FAIL();
                fprintf(stderr, "     error: in tests JSON: %s\n", jsonlogic_get_error_message(logic));
                goto test_cleanup;
            }

            if (jsonlogic_is_error(data)) {
                FAIL();
                fprintf(stderr, "     error: in tests JSON: %s\n", jsonlogic_get_error_message(data));
                goto test_cleanup;
            }

            if (jsonlogic_is_error(expected)) {
                FAIL();
                fprintf(stderr, "     error: in tests JSON: %s\n", jsonlogic_get_error_message(expected));
                goto test_cleanup;
            }

            JsonLogic_Handle actual = jsonlogic_apply(logic, data);

            if (!jsonlogic_deep_strict_equal(expected, actual)) {
                FAIL();
                fprintf(stderr, "     error: Wrong result\n");
                fprintf(stderr, "      test: "); jsonlogic_println(stderr, test);
                fprintf(stderr, "     logic: "); jsonlogic_println(stderr, logic);
                fprintf(stderr, "      data: "); jsonlogic_println(stderr, data);
                fprintf(stderr, "  expected: "); jsonlogic_println(stderr, expected);
                fprintf(stderr, "    actual: "); jsonlogic_println(stderr, actual);
                fputc('\n', stderr);
                goto test_cleanup;
            }

            ++ pass_count;

        test_cleanup:
            jsonlogic_decref(logic);
            jsonlogic_decref(data);
            jsonlogic_decref(expected);
        }

        jsonlogic_decref(test);
    }

    jsonlogic_iter_free(&iter);

    printf("\ntests: %" PRIuPTR ", failed: %" PRIuPTR ", passed: %" PRIuPTR "\n",
        test_count, test_count - pass_count, pass_count);

    jsonlogic_decref(tests);

    return test_count == pass_count ? 0 : 1;
}

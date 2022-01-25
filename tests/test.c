// for fileno()
#define _POSIX_C_SOURCE 1

#include "jsonlogic_intern.h"
#include "jsonlogic_extras.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <assert.h>
#include <limits.h>

#if defined(JSONLOGIC_WINDOWS)
    #include <process.h>
    #include <windows.h>
#else
    #include <sys/types.h>
    #include <unistd.h>
#endif

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

#define TEST_ASSERT_ERRNO(EXPR) \
    if (!(EXPR)) { \
        TEST_FAIL(); \
        fprintf(stderr, "%s:%u: Assertion failed: %s: %s\n", \
            __FILE__, __LINE__, TEST_STR(EXPR), strerror(errno)); \
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
        fprintf(stderr, "%s:%u: Assertion failed:\n", \
            __FILE__, __LINE__); \
        CODE; \
        goto cleanup; \
    }

#define TEST_DECL(NAME, FUNC) \
    { .name = NAME, .func = test_##FUNC }

#define TEST_END { .name = NULL, .func = NULL }

const double MOCK_TIME = 1629205800000; // 2021-08-17T15:10:00+02:00

void test_bad_operator(TestContext *test_context) {
    JsonLogic_Handle logic  = jsonlogic_parse("{\"fubar\": []}", NULL);
    JsonLogic_Handle result = jsonlogic_apply(logic, JsonLogic_Null);

    TEST_ASSERT(jsonlogic_get_error(result) == JSONLOGIC_ERROR_ILLEGAL_OPERATION);

cleanup:
    jsonlogic_decref(result);
    jsonlogic_decref(logic);
}

#if !defined(JSONLOGIC_WINDOWS)
void test_logging(TestContext *test_context) {
    struct stat stbuf;
    JsonLogic_Handle logic  = JsonLogic_Null;
    JsonLogic_Handle result = JsonLogic_Null;
    FILE *stdout_backup = stdout;
    FILE *fp = NULL;
    char path[PATH_MAX];
    char *actual = NULL;
    bool need_unlink = false;

#if defined(JSONLOGIC_WINDOWS)
    int pid = _getpid();
#else
    pid_t pid = getpid();
#endif

    int count = snprintf(path, sizeof(path), "test_logging.%" PRIi64 ".txt", (int64_t)pid);
    TEST_ASSERT_ERRNO(count >= 0);
    TEST_ASSERT(count < sizeof(path));

    fp = fopen(path, "wb");
    TEST_ASSERT_ERRNO(fp != NULL);
    stdout = fp;
    need_unlink = true;

    logic  = jsonlogic_parse("{\"log\": [[1,\"foo\\u000a\", \t\n true,null, {   }]]}", NULL);
    result = jsonlogic_apply(logic, JsonLogic_Null);

    stdout = stdout_backup;

    fclose(fp);
    fp = NULL;

    TEST_ASSERT(jsonlogic_get_error(result) == JSONLOGIC_ERROR_SUCCESS);

    fp = fopen(path, "rb");
    TEST_ASSERT(fp != NULL);

    TEST_ASSERT_ERRNO(fstat(fileno(fp), &stbuf) == 0);
    TEST_ASSERT(stbuf.st_size >= 0);

    size_t actual_size = stbuf.st_size;
    actual = malloc(actual_size);
    TEST_ASSERT_ERRNO(actual != NULL);
    TEST_ASSERT_ERRNO(fread(actual, actual_size, 1, fp) == 1);
    fclose(fp);
    fp = NULL;

    const char *expected = "[1,\"foo\\n\",true,null,{}]\n";

    TEST_ASSERT_X(actual_size == strlen(expected) && memcmp(actual, expected, actual_size) == 0, {
        fprintf(stderr, "     error: Wrong output\n");
        fprintf(stderr, "     logic: "); jsonlogic_println(stderr, logic);
        fprintf(stderr, "      data: null\n");
        fprintf(stderr, "  expected: (length=%" PRIuPTR ") %s\n", strlen(expected), expected);
        fprintf(stderr, "    actual: (length=%" PRIuPTR ") \n", actual_size);
        fwrite(actual, actual_size, 1, stderr);
        fputc('\n', stderr);
    });

cleanup:
    if (need_unlink) {
        unlink(path);
    }
    free(actual);
    stdout = stdout_backup;

    jsonlogic_decref(result);
    jsonlogic_decref(logic);

    if (fp != NULL) {
        fclose(fp);
    }
}
#endif

void test_edge_cases(TestContext *test_context) {
    JsonLogic_Handle logic = jsonlogic_parse("{\"var\": \"\"}", NULL);
    JsonLogic_Handle fallback = jsonlogic_string_from_latin1("fallback");

    TEST_ASSERT(jsonlogic_is_string(fallback));

    TEST_ASSERT_MSG(jsonlogic_deep_strict_equal(jsonlogic_apply(JsonLogic_Null, JsonLogic_Null), JsonLogic_Null), "Called with null");
    TEST_ASSERT_MSG(jsonlogic_deep_strict_equal(jsonlogic_apply(logic, jsonlogic_number_from(0)), jsonlogic_number_from(0)), "Var when data is 'falsy'");
    TEST_ASSERT_MSG(jsonlogic_deep_strict_equal(jsonlogic_apply(logic, JsonLogic_Null), JsonLogic_Null), "Var when data is null");

    jsonlogic_decref(logic);
    logic = jsonlogic_parse("{\"var\":[\"a\",\"fallback\"]}", NULL);
    JsonLogic_Handle result = jsonlogic_apply(logic, JsonLogic_Null);
    TEST_ASSERT_MSG(jsonlogic_deep_strict_equal(result, fallback), "Fallback works when data is a non-object");
    jsonlogic_decref(result);

cleanup:
    jsonlogic_decref(logic);
    jsonlogic_decref(fallback);
}

typedef struct TestCustomOps {
    double a;
} TestCustomOps;

JsonLogic_Handle op_add_to_a(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    assert(context != NULL);
    TestCustomOps *ptr = context;
    double b = argc == 0 ? 1 : jsonlogic_to_double(args[0]);
    return jsonlogic_number_from(ptr->a += b);
}

JsonLogic_Handle op_fives_add(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    double i = argc == 0 ? 0.0 : jsonlogic_to_double(args[0]);
    return jsonlogic_number_from(i + 5);
}

JsonLogic_Handle op_fives_subtract(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    double i = argc == 0 ? 0.0 : jsonlogic_to_double(args[0]);
    return jsonlogic_number_from(i - 5);
}

JsonLogic_Handle op_times(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    if (argc < 2) {
        return JsonLogic_Error_IllegalArgument;
    }

    JsonLogic_Handle a = args[0];
    JsonLogic_Handle b = args[1];

    if (jsonlogic_is_error(a)) {
        return a;
    }

    if (jsonlogic_is_error(b)) {
        return b;
    }

    double anum = jsonlogic_to_double(a);
    double bnum = jsonlogic_to_double(b);
    return jsonlogic_number_from(anum * bnum);
}

JsonLogic_Handle op_array_times(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    if (argc < 1) {
        return JsonLogic_Error_IllegalArgument;
    }

    JsonLogic_Handle a = jsonlogic_get_index(args[0], 0);
    if (jsonlogic_is_error(a)) {
        return a;
    }
    JsonLogic_Handle b = jsonlogic_get_index(args[0], 1);
    if (jsonlogic_is_error(b)) {
        jsonlogic_decref(a);
        return b;
    }

    double anum = jsonlogic_to_double(a);
    double bnum = jsonlogic_to_double(b);

    jsonlogic_decref(a);
    jsonlogic_decref(b);

    return jsonlogic_number_from(anum * bnum);
}

void test_custom_operators(TestContext *test_context) {
    // Set up some outside data, and build a basic function operator
    TestCustomOps context = {
        .a = 0.0,
    };
    JsonLogic_Handle data  = JsonLogic_Null;
    JsonLogic_Handle logic = jsonlogic_parse("{\"add_to_a\":[]}", NULL);
    // Operator is not yet defined
    JsonLogic_Handle result = jsonlogic_apply_custom(logic, JsonLogic_Null, &JsonLogic_Builtins);

    JsonLogic_Operations ops  = JSONLOGIC_OPERATIONS_INIT;
    JsonLogic_Operations ops2 = JSONLOGIC_OPERATIONS_INIT;
    JsonLogic_Operations ops3 = JSONLOGIC_OPERATIONS_INIT;
    JsonLogic_Operations ops4 = JSONLOGIC_OPERATIONS_INIT;

    TEST_ASSERT(jsonlogic_get_error(result) == JSONLOGIC_ERROR_ILLEGAL_OPERATION);
    jsonlogic_decref(result);
    result = JsonLogic_Null;

    TEST_ASSERT(jsonlogic_operations_extend(&ops, &JsonLogic_Builtins) == JSONLOGIC_ERROR_SUCCESS);
    TEST_ASSERT(jsonlogic_operations_set(&ops, u"add_to_a", &context, op_add_to_a) == JSONLOGIC_ERROR_SUCCESS);

    // New operation executes, returns desired result
    // No args
    result = jsonlogic_apply_custom(logic, JsonLogic_Null, &ops);

    TEST_ASSERT(jsonlogic_deep_strict_equal(result, jsonlogic_number_from(1)));

    // Unary syntactic sugar
    jsonlogic_decref(logic);
    logic  = jsonlogic_parse("{\"add_to_a\":41}", NULL);
    result = jsonlogic_apply_custom(logic, JsonLogic_Null, &ops);

    TEST_ASSERT(jsonlogic_deep_strict_equal(result, jsonlogic_number_from(42)));
    // New operation had side effects.
    TEST_ASSERT(context.a == 42.0);

    TEST_ASSERT(jsonlogic_operations_extend(&ops2, &JsonLogic_Builtins) == JSONLOGIC_ERROR_SUCCESS);
    TEST_ASSERT(jsonlogic_operations_build(&ops2, (JsonLogic_Operations_BuildEntry[]) {
        { u"fives.add",      { &context, op_fives_add      } },
        { u"fives.subtract", { &context, op_fives_subtract } },
        { NULL,              { NULL,     NULL           } },
    }) == JSONLOGIC_ERROR_SUCCESS);

    jsonlogic_decref(logic);
    logic  = jsonlogic_parse("{\"fives.add\":37}", NULL);
    result = jsonlogic_apply_custom(logic, JsonLogic_Null, &ops2);

    TEST_ASSERT(jsonlogic_deep_strict_equal(result, jsonlogic_number_from(42)));

    jsonlogic_decref(logic);
    logic  = jsonlogic_parse("{\"fives.subtract\":47}", NULL);
    result = jsonlogic_apply_custom(logic, JsonLogic_Null, &ops2);

    TEST_ASSERT(jsonlogic_deep_strict_equal(result, jsonlogic_number_from(42)));

    TEST_ASSERT(jsonlogic_operations_extend(&ops3, &JsonLogic_Builtins) == JSONLOGIC_ERROR_SUCCESS);
    TEST_ASSERT(jsonlogic_operations_set(&ops3, u"times", &context, op_times) == JSONLOGIC_ERROR_SUCCESS);

    // Calling a method with multiple var as arguments.
    jsonlogic_decref(logic);
    logic = jsonlogic_parse("{\"times\":[{\"var\":\"a\"},{\"var\":\"b\"}]}", NULL);
    data  = jsonlogic_parse("{\"a\":6,\"b\":7}", NULL);

    result = jsonlogic_apply_custom(logic, data, &ops3);

    TEST_ASSERT(jsonlogic_deep_strict_equal(result, jsonlogic_number_from(42)));

    result = jsonlogic_apply_custom(logic, data, &JsonLogic_Builtins);
    TEST_ASSERT(jsonlogic_get_error(result) == JSONLOGIC_ERROR_ILLEGAL_OPERATION);

    // Calling a method that takes an array, but the inside of the array has rules, too
    TEST_ASSERT(jsonlogic_operations_extend(&ops4, &JsonLogic_Builtins) == JSONLOGIC_ERROR_SUCCESS);
    TEST_ASSERT(jsonlogic_operations_set(&ops4, u"array_times", &context, op_array_times) == JSONLOGIC_ERROR_SUCCESS);

    jsonlogic_decref(logic);
    logic = jsonlogic_parse("{\"array_times\":[[{\"var\":\"a\"},{\"var\":\"b\"}]]}", NULL);

    result = jsonlogic_apply_custom(logic, data, &ops4);

    TEST_ASSERT_X(jsonlogic_deep_strict_equal(result, jsonlogic_number_from(42)), {
        fprintf(stderr, "     error: Wrong result\n");
        fprintf(stderr, "     logic: "); jsonlogic_println(stderr, logic);
        fprintf(stderr, "      data: "); jsonlogic_println(stderr, data);
        fprintf(stderr, "  expected: 42\n");
        fprintf(stderr, "    actual: "); jsonlogic_println(stderr, result);
        fputc('\n', stderr);
    });

cleanup:
    jsonlogic_decref(logic);
    jsonlogic_decref(result);
    jsonlogic_decref(data);

    jsonlogic_operations_free(&ops);
    jsonlogic_operations_free(&ops2);
    jsonlogic_operations_free(&ops3);
    jsonlogic_operations_free(&ops4);
}

typedef struct TestShortCircuit {
    JsonLogic_ArrayBuf conditions;
    JsonLogic_ArrayBuf consequences;
    JsonLogic_ArrayBuf list;
} TestShortCircuit;

JsonLogic_Handle mock_now(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    return jsonlogic_number_from(MOCK_TIME);
}

JsonLogic_Handle mock_time_since(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    if (argc == 0) {
        return JsonLogic_Error_IllegalArgument;
    }

    double tv = jsonlogic_parse_date_time(args[0]);
    return jsonlogic_number_from(MOCK_TIME - tv);
}

JsonLogic_Handle push_if(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    TestShortCircuit *ctx = context;
    JsonLogic_Handle value = argc == 0 ? JsonLogic_Null : args[0];
    JsonLogic_Error error = jsonlogic_arraybuf_append(&ctx->conditions, value);
    if (error != JSONLOGIC_ERROR_SUCCESS) {
        return jsonlogic_error_from(error);
    }
    return jsonlogic_incref(value);
}

JsonLogic_Handle push_then(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    TestShortCircuit *ctx = context;
    JsonLogic_Handle value = argc == 0 ? JsonLogic_Null : args[0];
    JsonLogic_Error error = jsonlogic_arraybuf_append(&ctx->consequences, value);
    if (error != JSONLOGIC_ERROR_SUCCESS) {
        return jsonlogic_error_from(error);
    }
    return jsonlogic_incref(value);
}

JsonLogic_Handle push_else(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    TestShortCircuit *ctx = context;
    JsonLogic_Handle value = argc == 0 ? JsonLogic_Null : args[0];
    JsonLogic_Error error = jsonlogic_arraybuf_append(&ctx->consequences, value);
    if (error != JSONLOGIC_ERROR_SUCCESS) {
        return jsonlogic_error_from(error);
    }
    return jsonlogic_incref(value);
}

JsonLogic_Handle push_list(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    TestShortCircuit *ctx = context;
    JsonLogic_Handle value = argc == 0 ? JsonLogic_Null : args[0];
    JsonLogic_Error error = jsonlogic_arraybuf_append(&ctx->list, value);
    if (error != JSONLOGIC_ERROR_SUCCESS) {
        return jsonlogic_error_from(error);
    }
    return jsonlogic_incref(value);
}

void test_short_circuit(TestContext *test_context) {
    JsonLogic_Handle logic = JsonLogic_Null;
    JsonLogic_Handle data  = JsonLogic_Null;
    JsonLogic_Operations ops = JSONLOGIC_OPERATIONS_INIT;
    TestShortCircuit context = {
        .conditions   = JSONLOGIC_ARRAYBUF_INIT,
        .consequences = JSONLOGIC_ARRAYBUF_INIT,
        .list         = JSONLOGIC_ARRAYBUF_INIT,
    };

    TEST_ASSERT(jsonlogic_operations_extend(&ops, &JsonLogic_Builtins) == JSONLOGIC_ERROR_SUCCESS);
    TEST_ASSERT(jsonlogic_operations_build(&ops, (JsonLogic_Operations_BuildEntry[]) {
        { u"push",      { &context, push_list } },
        { u"push.else", { &context, push_else } },
        { u"push.if",   { &context, push_if   } },
        { u"push.then", { &context, push_then } },
        { NULL,         { NULL,     NULL      } },
    }) == JSONLOGIC_ERROR_SUCCESS);

    logic = jsonlogic_parse("{\"if\":["
        "{\"push.if\":[true]},"
        "{\"push.then\":[\"first\"]},"
        "{\"push.if\":[false]},"
        "{\"push.then\":[\"second\"]},"
        "{\"push.else\":[\"third\"]}"
    "]}", NULL);

    jsonlogic_decref(jsonlogic_apply_custom(
        logic, JsonLogic_Null,
        &ops));

    data = jsonlogic_parse("[true]", NULL);
    JsonLogic_Handle actual = context.conditions.array == NULL ? JsonLogic_Null : jsonlogic_array_into_handle(context.conditions.array);
    TEST_ASSERT_X(jsonlogic_deep_strict_equal(actual, data), {
        fprintf(stderr, "     error: Wrong result\n");
        fprintf(stderr, "     logic: "); jsonlogic_println(stderr, logic);
        fprintf(stderr, "  expected: "); jsonlogic_println(stderr, data);
        fprintf(stderr, "    actual: "); jsonlogic_println(stderr, actual);
        fputc('\n', stderr);
    });

    jsonlogic_decref(data);
    data = jsonlogic_parse("[\"first\"]", NULL);

    actual = context.consequences.array == NULL ?
        JsonLogic_Null :
        jsonlogic_array_into_handle(context.consequences.array);
    TEST_ASSERT_X(jsonlogic_deep_strict_equal(
        actual,
        data
    ), {
        fprintf(stderr, "     error: Wrong result\n");
        fprintf(stderr, "     logic: "); jsonlogic_println(stderr, logic);
        fprintf(stderr, "  expected: "); jsonlogic_println(stderr, data);
        fprintf(stderr, "    actual: "); jsonlogic_println(stderr, actual);
        fputc('\n', stderr);
    });

    jsonlogic_arraybuf_clear(&context.conditions);
    jsonlogic_arraybuf_clear(&context.consequences);

    jsonlogic_decref(logic);
    logic = jsonlogic_parse("{\"if\":["
        "{\"push.if\":[false]},"
        "{\"push.then\":[\"first\"]},"
        "{\"push.if\":[false]},"
        "{\"push.then\":[\"second\"]},"
        "{\"push.else\":[\"third\"]}"
    "]}", NULL);

    jsonlogic_decref(jsonlogic_apply_custom(
        logic, JsonLogic_Null,
        &ops));

    jsonlogic_decref(data);
    data = jsonlogic_parse("[false, false]", NULL);

    TEST_ASSERT(jsonlogic_deep_strict_equal(
        context.conditions.array == NULL ? JsonLogic_Null : jsonlogic_array_into_handle(context.conditions.array),
        data
    ));

    jsonlogic_decref(data);
    data = jsonlogic_parse("[\"third\"]", NULL);

    actual = context.consequences.array == NULL ? JsonLogic_Null : jsonlogic_array_into_handle(context.consequences.array);
    TEST_ASSERT(jsonlogic_deep_strict_equal(
        actual,
        data
    ));

    jsonlogic_decref(logic);
    logic = jsonlogic_parse("{\"and\": [{\"push\": [false]}, {\"push\": [false]}]}", NULL);

    jsonlogic_decref(jsonlogic_apply_custom(
        logic, JsonLogic_Null,
        &ops));

    jsonlogic_decref(data);
    data = jsonlogic_parse("[false]", NULL);

    TEST_ASSERT(jsonlogic_deep_strict_equal(
        jsonlogic_array_into_handle(context.list.array),
        data
    ));

    jsonlogic_arraybuf_clear(&context.list);
    jsonlogic_decref(logic);
    logic = jsonlogic_parse("{\"and\": [{\"push\": [false]}, {\"push\": [true]}]}", NULL);

    jsonlogic_decref(jsonlogic_apply_custom(
        logic, JsonLogic_Null,
        &ops));

    jsonlogic_decref(data);
    data = jsonlogic_parse("[false]", NULL);

    TEST_ASSERT(jsonlogic_deep_strict_equal(
        jsonlogic_array_into_handle(context.list.array),
        data
    ));

    jsonlogic_arraybuf_clear(&context.list);
    jsonlogic_decref(logic);
    logic = jsonlogic_parse("{\"and\": [{\"push\": [true]}, {\"push\": [false]}]}", NULL);

    jsonlogic_decref(jsonlogic_apply_custom(
        logic, JsonLogic_Null,
        &ops));

    jsonlogic_decref(data);
    data = jsonlogic_parse("[true, false]", NULL);

    TEST_ASSERT(jsonlogic_deep_strict_equal(
        jsonlogic_array_into_handle(context.list.array),
        data
    ));

    jsonlogic_arraybuf_clear(&context.list);
    jsonlogic_decref(logic);
    logic = jsonlogic_parse("{\"and\": [{\"push\": [true]}, {\"push\": [true]}]}", NULL);

    jsonlogic_decref(jsonlogic_apply_custom(
        logic, JsonLogic_Null,
        &ops));

    jsonlogic_decref(data);
    data = jsonlogic_parse("[true, true]", NULL);

    TEST_ASSERT(jsonlogic_deep_strict_equal(
        jsonlogic_array_into_handle(context.list.array),
        data
    ));



    jsonlogic_arraybuf_clear(&context.list);
    jsonlogic_decref(logic);
    logic = jsonlogic_parse("{\"or\": [{\"push\": [false]}, {\"push\": [false]}]}", NULL);

    jsonlogic_decref(jsonlogic_apply_custom(
        logic, JsonLogic_Null,
        &ops));

    jsonlogic_decref(data);
    data = jsonlogic_parse("[false, false]", NULL);

    TEST_ASSERT(jsonlogic_deep_strict_equal(
        jsonlogic_array_into_handle(context.list.array),
        data
    ));

    jsonlogic_arraybuf_clear(&context.list);
    jsonlogic_decref(logic);
    logic = jsonlogic_parse("{\"or\": [{\"push\": [false]}, {\"push\": [true]}]}", NULL);

    jsonlogic_decref(jsonlogic_apply_custom(
        logic, JsonLogic_Null,
        &ops));

    jsonlogic_decref(data);
    data = jsonlogic_parse("[false, true]", NULL);

    TEST_ASSERT(jsonlogic_deep_strict_equal(
        jsonlogic_array_into_handle(context.list.array),
        data
    ));

    jsonlogic_arraybuf_clear(&context.list);
    jsonlogic_decref(logic);
    logic = jsonlogic_parse("{\"or\": [{\"push\": [true]}, {\"push\": [false]}]}", NULL);

    jsonlogic_decref(jsonlogic_apply_custom(
        logic, JsonLogic_Null,
        &ops));

    jsonlogic_decref(data);
    data = jsonlogic_parse("[true]", NULL);

    TEST_ASSERT(jsonlogic_deep_strict_equal(
        jsonlogic_array_into_handle(context.list.array),
        data
    ));

    jsonlogic_arraybuf_clear(&context.list);
    jsonlogic_decref(logic);
    logic = jsonlogic_parse("{\"or\": [{\"push\": [true]}, {\"push\": [true]}]}", NULL);

    jsonlogic_decref(jsonlogic_apply_custom(
        logic, JsonLogic_Null,
        &ops));

    jsonlogic_decref(data);
    data = jsonlogic_parse("[true]", NULL);

    TEST_ASSERT(jsonlogic_deep_strict_equal(
        jsonlogic_array_into_handle(context.list.array),
        data
    ));

cleanup:
    jsonlogic_decref(data);
    jsonlogic_decref(logic);

    jsonlogic_arraybuf_free(&context.conditions);
    jsonlogic_arraybuf_free(&context.consequences);
    jsonlogic_arraybuf_free(&context.list);

    jsonlogic_operations_free(&ops);
}

void test_substr(TestContext *test_context) {
    JsonLogic_Handle logic    = jsonlogic_parse("{\"substr\": [\"äöü\", 0, -2]}", NULL);
    JsonLogic_Handle expected = jsonlogic_string_from_utf16(u"ä");
    JsonLogic_Handle actual   = jsonlogic_apply(logic, JsonLogic_Null);

    TEST_ASSERT_X(jsonlogic_deep_strict_equal(expected, actual), {
        fprintf(stderr, "     error: Wrong result\n");
        fprintf(stderr, "     logic: "); jsonlogic_println(stderr, logic);
        fprintf(stderr, "      data: "); jsonlogic_println(stderr, JsonLogic_Null);
        fprintf(stderr, "  expected: "); jsonlogic_println(stderr, expected);
        fprintf(stderr, "    actual: "); jsonlogic_println(stderr, actual);
        fputc('\n', stderr);
    });

    jsonlogic_decref(actual);
    jsonlogic_decref(logic);
    jsonlogic_decref(expected);

    logic    = jsonlogic_parse("{\"substr\": [\"\\uD80C\\uDC00\", 1]}", NULL);
    expected = jsonlogic_string_from_utf16((char16_t[]){ 0xdc00, 0 });
    actual   = jsonlogic_apply(logic, JsonLogic_Null);

    TEST_ASSERT_X(jsonlogic_deep_strict_equal(expected, actual), {
        fprintf(stderr, "     error: Wrong result\n");
        fprintf(stderr, "     logic: "); jsonlogic_println(stderr, logic);
        fprintf(stderr, "      data: "); jsonlogic_println(stderr, JsonLogic_Null);
        fprintf(stderr, "  expected: "); jsonlogic_println(stderr, expected);
        fprintf(stderr, "    actual: "); jsonlogic_println(stderr, actual);
        fputc('\n', stderr);
    });

cleanup:
    jsonlogic_decref(logic);
    jsonlogic_decref(expected);
    jsonlogic_decref(actual);
}

const char *JSON_SYNTAX_ERROR[] = {
    "{\"foo\": \"bar\" \"a\":\"b\"}",
    "[]]",
    "\"\\j\"",
    "\"\\u123g\"",
    "\"foo",
    "\"foo\",",
    "[,]",
    "[123,]",
    "{,}",
    "{\"foo\":123,}",
    "{\"foo\":}",
    "{\"foo\":,}",
    "{123:123}",
    NULL
};

const char *VALID_JSON[] = {
    "0",
    "123",
    "123.456",
    "1.2e12",
    "[]",
    "{}",
    "true",
    "false",
    "null",
    "\"\"",
    NULL
};

void test_parsing(TestContext *test_context) {
    const char *data = NULL;
    JsonLogic_Handle handle = JsonLogic_Null;

    data = "\"\xb0\"";
    jsonlogic_decref(handle);
    handle = jsonlogic_parse(data, NULL);
    TEST_ASSERT_X(jsonlogic_deep_strict_equal(handle, JsonLogic_Error_UnicodeError), {
        fprintf(stderr, "     error: Wrong result\n");
        fprintf(stderr, "      data: %s\n", data);
        fprintf(stderr, "  expected: "); jsonlogic_println(stderr, JsonLogic_Error_UnicodeError);
        fprintf(stderr, "    actual: "); jsonlogic_println(stderr, handle);
        fputc('\n', stderr);
    });

    for (const char **ptr = JSON_SYNTAX_ERROR; *ptr; ++ ptr) {
        data = *ptr;
        jsonlogic_decref(handle);
        handle = jsonlogic_parse(data, NULL);
        TEST_ASSERT_X(jsonlogic_deep_strict_equal(handle, JsonLogic_Error_SyntaxError), {
            fprintf(stderr, "     error: Wrong result\n");
            fprintf(stderr, "      data: %s\n", data);
            fprintf(stderr, "  expected: "); jsonlogic_println(stderr, JsonLogic_Error_SyntaxError);
            fprintf(stderr, "    actual: "); jsonlogic_println(stderr, handle);
            fputc('\n', stderr);
        });
    }

    for (const char **ptr = VALID_JSON; *ptr; ++ ptr) {
        data = *ptr;
        jsonlogic_decref(handle);
        handle = jsonlogic_parse(data, NULL);
        TEST_ASSERT_X(jsonlogic_get_error(handle) == JSONLOGIC_ERROR_SUCCESS, {
            fprintf(stderr, "     error: Wrong result\n");
            fprintf(stderr, "      data: %s\n", data);
            fprintf(stderr, "  expected: no error\n");
            fprintf(stderr, "    actual: "); jsonlogic_println(stderr, handle);
            fputc('\n', stderr);
        });
    }

cleanup:
    jsonlogic_decref(handle);
}

void test_extras(TestContext *test_context) {
    JsonLogic_Handle logic    = jsonlogic_parse("{\"zip\": [[1,2,3],[\"a\",\"b\"]]}", NULL);
    JsonLogic_Handle expected = jsonlogic_parse("[[1,\"a\"],[2,\"b\"]]", NULL);
    JsonLogic_Handle actual   = jsonlogic_apply_custom(logic, JsonLogic_Null, &JsonLogic_Extras);

    TEST_ASSERT_X(jsonlogic_deep_strict_equal(expected, actual), {
        fprintf(stderr, "     error: Wrong result\n");
        fprintf(stderr, "     logic: "); jsonlogic_println(stderr, logic);
        fprintf(stderr, "      data: "); jsonlogic_println(stderr, JsonLogic_Null);
        fprintf(stderr, "  expected: "); jsonlogic_println(stderr, expected);
        fprintf(stderr, "    actual: "); jsonlogic_println(stderr, actual);
        fputc('\n', stderr);
    });

    jsonlogic_decref(logic);
    jsonlogic_decref(expected);
    jsonlogic_decref(actual);

    logic    = jsonlogic_parse("{\"combinations\": [[1,2,3],[\"a\",\"b\",\"c\"],[\"x\",\"y\",\"z\"]]}", NULL);
    expected = jsonlogic_parse("[[1, \"a\", \"x\"], [1, \"a\", \"y\"], [1, \"a\", \"z\"], [1, \"b\", \"x\"], [1, \"b\", \"y\"], [1, \"b\", \"z\"], [1, \"c\", \"x\"], [1, \"c\", \"y\"], [1, \"c\", \"z\"], [2, \"a\", \"x\"], [2, \"a\", \"y\"], [2, \"a\", \"z\"], [2, \"b\", \"x\"], [2, \"b\", \"y\"], [2, \"b\", \"z\"], [2, \"c\", \"x\"], [2, \"c\", \"y\"], [2, \"c\", \"z\"], [3, \"a\", \"x\"], [3, \"a\", \"y\"], [3, \"a\", \"z\"], [3, \"b\", \"x\"], [3, \"b\", \"y\"], [3, \"b\", \"z\"], [3, \"c\", \"x\"], [3, \"c\", \"y\"], [3, \"c\", \"z\"]]", NULL);
    actual   = jsonlogic_apply_custom(logic, JsonLogic_Null, &JsonLogic_Extras);

    TEST_ASSERT_X(jsonlogic_deep_strict_equal(expected, actual), {
        fprintf(stderr, "     error: Wrong result\n");
        fprintf(stderr, "     logic: "); jsonlogic_println(stderr, logic);
        fprintf(stderr, "      data: "); jsonlogic_println(stderr, JsonLogic_Null);
        fprintf(stderr, "  expected: "); jsonlogic_println(stderr, expected);
        fprintf(stderr, "    actual: "); jsonlogic_println(stderr, actual);
        fputc('\n', stderr);
    });

cleanup:
    jsonlogic_decref(logic);
    jsonlogic_decref(expected);
    jsonlogic_decref(actual);
}

const TestCase TEST_CASES[] = {
    TEST_DECL("Unicode and JSON parsing", parsing),
    TEST_DECL("Bad Operator", bad_operator),
#if !defined(JSONLOGIC_WINDOWS)
    TEST_DECL("Logging", logging),
#endif
    TEST_DECL("Edge Cases", edge_cases),
    TEST_DECL("Expanding functionality with custom operators", custom_operators),
    TEST_DECL("Control structures don't eval depth-first", short_circuit),
    TEST_DECL("Sub-string of non-ASCII strings", substr),
    TEST_DECL("Test extra operators", extras),
    TEST_END,
};

JsonLogic_Handle parse_file(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        perror(filename);
        return JsonLogic_Error_IOError;
    }

    struct stat stbuf;
    if (fstat(fileno(fp), &stbuf) != 0) {
        fclose(fp);
        perror(filename);
        return JsonLogic_Error_IOError;
    }

    char *data = malloc(stbuf.st_size + 1);
    if (data == NULL) {
        fclose(fp);
        perror(filename);
        return JsonLogic_Error_OutOfMemory;
    }

    if (fread(data, stbuf.st_size, 1, fp) != 1) {
        free(data);
        fclose(fp);
        perror(filename);
        return JsonLogic_Error_IOError;
    }

    data[stbuf.st_size] = 0;

    fclose(fp);

    JsonLogic_LineInfo info = JSONLOGIC_LINEINFO_INIT;
    JsonLogic_Handle handle = jsonlogic_parse(data, &info);

    JsonLogic_Error error = jsonlogic_get_error(handle);
    if (error != JSONLOGIC_ERROR_SUCCESS) {
        jsonlogic_print_parse_error(stderr, data, error, info);
        free(data);
        return handle;
    }
    free(data);

    return handle;
}

const char *CertLogicTests[] = {
    "tests/certlogic/and.json",
    "tests/certlogic/comparison.json",
    "tests/certlogic/date-times.json",
    "tests/certlogic/detect-missing-values.json",
    "tests/certlogic/equality.json",
    "tests/certlogic/extractFromUCVI.json",
    "tests/certlogic/if.json",
    "tests/certlogic/in.json",
    "tests/certlogic/ins-with-nulls.json",
    "tests/certlogic/JsonLogic-testSuite.json",
    "tests/certlogic/patched-reduce.json",
    "tests/certlogic/var.json",
    NULL,
};

#if defined(__MINGW32__)
    // XXX: mingw (but not msvc) seems to somehow break ANSI escape sequences?
    #define COL_RED
    #define COL_GREEN
    #define COL_NORMAL
#else
    #define COL_RED    "\x1B[31m"
    #define COL_GREEN  "\x1B[32m"
    #define COL_NORMAL "\x1B[0m"
#endif

void print_ok() {
    puts(COL_GREEN "OK" COL_NORMAL);
}

void print_failed() {
    puts(COL_RED "FAILED" COL_NORMAL);
}

#if defined(JSONLOGIC_WINDOWS) && !defined(__MINGW32__)
void print_win32_error() {
    char buf[2048];
    DWORD count = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        buf, sizeof(buf), NULL);
    if (count > 0) {
        fprintf(stderr, "*** error: %s\n", buf);
    } else {
        fprintf(stderr, "*** error: getting last Win32 error\n");
    }
}
#endif

int main(int argc, char *argv[]) {
    int status = 0;
    JsonLogic_Handle tests = JsonLogic_Null;
    JsonLogic_Handle rule  = JsonLogic_Null;
    JsonLogic_Handle valid_examples   = JsonLogic_Null;
    JsonLogic_Handle invalid_examples = JsonLogic_Null;

    JsonLogic_Operations ops = JSONLOGIC_OPERATIONS_INIT;
    JsonLogic_Error error = jsonlogic_operations_extend(&ops, &JsonLogic_Extras);

#if defined(JSONLOGIC_WINDOWS) && !defined(__MINGW32__)
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            if (!SetConsoleMode(hOut, dwMode)) {
                print_win32_error();
                goto error;
            }
        }
    }
#endif

    if (error != JSONLOGIC_ERROR_SUCCESS) {
        fprintf(stderr, "*** error: building operations table: %s\n", jsonlogic_get_error_message(error));
        goto error;
    }

    error = jsonlogic_operations_build(&ops, (JsonLogic_Operations_BuildEntry[]) {
        { u"now",        { NULL, mock_now        } },
        { u"time-since", { NULL, mock_time_since } },
        { NULL,          { NULL, NULL            } },
    });
    if (error != JSONLOGIC_ERROR_SUCCESS) {
        fprintf(stderr, "*** error: building operations table: %s\n", jsonlogic_get_error_message(error));
        goto error;
    }

    tests = parse_file("tests/tests.json");
    if (jsonlogic_is_error(tests)) {
        goto error;
    }

    rule = parse_file("tests/rule.json");
    if (jsonlogic_is_error(rule)) {
        goto error;
    }

    valid_examples = parse_file("tests/valid.json");
    if (jsonlogic_is_error(valid_examples)) {
        goto error;
    }

    invalid_examples = parse_file("tests/invalid.json");
    if (jsonlogic_is_error(invalid_examples)) {
        goto error;
    }

    TestContext test_context = {
        .test_case = NULL,
        .newline   = false,
        .passed    = true,
        .testnr    = 0,
    };
    size_t test_count = 0;
    size_t pass_count = 0;

    puts("Basic Tests");
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
            if (test_context.passed) {
                print_ok();
            } else {
                print_failed();
            }
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
            print_failed(); \
            fflush(stdout); \
            test_context.newline = true; \
        } \
        test_context.passed = false; \
        fprintf(stderr, "      test: %" PRIuPTR "\n", test_count);

    JsonLogic_Iterator iter = jsonlogic_iter(tests);

    putchar('\n');
    puts("Tests from https://jsonlogic.com/tests.json");
    for (;;) {
        JsonLogic_Handle test = jsonlogic_iter_next(&iter);
        JsonLogic_Error error = jsonlogic_get_error(test);

        if (error == JSONLOGIC_ERROR_STOP_ITERATION) {
            if (!test_context.newline) {
                print_ok();
                fflush(stdout);
                test_context.newline = false;
            }
            break;
        } else if (error != JSONLOGIC_ERROR_SUCCESS) {
            FAIL();
            fprintf(stderr, "     error: in tests/tests.json: %s\n", jsonlogic_get_error_message(error));
            break;
        }

        if (jsonlogic_is_string(test)) {
            if (!test_context.newline) {
                print_ok();
            }
            size_t size = 0;
            const char16_t *str = jsonlogic_get_string_content(test, &size);
            printf(" - "); jsonlogic_print_utf16(stdout, str, size); printf(" ... ");
            fflush(stdout);
            test_context.newline = false;
        } else {
            ++ test_count;

            JsonLogic_Handle logic    = jsonlogic_get_index(test, 0);
            JsonLogic_Handle data     = jsonlogic_get_index(test, 1);
            JsonLogic_Handle expected = jsonlogic_get_index(test, 2);
            JsonLogic_Handle actual   = JsonLogic_Null;

            if (jsonlogic_is_error(logic)) {
                FAIL();
                fprintf(stderr, "     error: in tests/tests.json: %s\n", jsonlogic_get_error_message(jsonlogic_get_error(logic)));
                goto test_cleanup;
            }

            if (jsonlogic_is_error(data)) {
                FAIL();
                fprintf(stderr, "     error: in tests/tests.json: %s\n", jsonlogic_get_error_message(jsonlogic_get_error(data)));
                goto test_cleanup;
            }

            if (jsonlogic_is_error(expected)) {
                FAIL();
                fprintf(stderr, "     error: in tests/tests.json: %s\n", jsonlogic_get_error_message(jsonlogic_get_error(expected)));
                goto test_cleanup;
            }

            actual = jsonlogic_apply(logic, data);

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
            jsonlogic_decref(actual);
        }

        jsonlogic_decref(test);
    }

    jsonlogic_iter_free(&iter);

    iter = jsonlogic_iter(valid_examples);

    putchar('\n');
    puts("Test health data rules with valid data");
    for (;;) {
        JsonLogic_Handle test = jsonlogic_iter_next(&iter);
        JsonLogic_Error error = jsonlogic_get_error(test);

        if (error == JSONLOGIC_ERROR_STOP_ITERATION) {
            break;
        }

        ++ test_count;

        if (error != JSONLOGIC_ERROR_SUCCESS) {
            fflush(stdout);
            fprintf(stderr, "     error: in tests/valid.json: %s\n", jsonlogic_get_error_message(error));
            break;
        }

        size_t size = 0;
        JsonLogic_Handle test_name = jsonlogic_get_utf16(test, u"name");
        const char16_t *str = jsonlogic_get_string_content(test_name, &size);
        if (str == NULL) {
            puts("FAIL");
            fflush(stdout);
            if (jsonlogic_is_string(test_name)) {
                fprintf(stderr, "     error: in tests/valid.json: error getting test name\n");
            } else {
                fprintf(stderr, "     error: in tests/valid.json: error getting test name: ");
                jsonlogic_println(stderr, test);
            }
            jsonlogic_decref(test);
            jsonlogic_decref(test_name);
            continue;
        }

        printf(" - "); jsonlogic_print_utf16(stdout, str, size); printf(" ... ");
        fflush(stdout);

        JsonLogic_Handle code   = jsonlogic_get_utf16(test, u"code");
        JsonLogic_Handle result = jsonlogic_apply_custom(rule, code, &ops);

        if (jsonlogic_is_true(result)) {
            print_ok();
            ++ pass_count;
        } else {
            print_failed();
            fflush(stdout);
            fprintf(stderr, "     error: Wrong result\n");
            //fprintf(stderr, "     logic: "); jsonlogic_println(stderr, rule);
            fprintf(stderr, "      data: "); jsonlogic_println(stderr, code);
            fprintf(stderr, "  expected: true\n");
            fprintf(stderr, "    actual: "); jsonlogic_println(stderr, result);
            fputc('\n', stderr);
        }

        jsonlogic_decref(result);
        jsonlogic_decref(code);
        jsonlogic_decref(test_name);
        jsonlogic_decref(test);
    }

    jsonlogic_iter_free(&iter);

    iter = jsonlogic_iter(invalid_examples);

    putchar('\n');
    puts("Test health data rules with invalid data");
    for (;;) {
        JsonLogic_Handle test = jsonlogic_iter_next(&iter);
        JsonLogic_Error error = jsonlogic_get_error(test);

        if (error == JSONLOGIC_ERROR_STOP_ITERATION) {
            break;
        }

        ++ test_count;

        if (error != JSONLOGIC_ERROR_SUCCESS) {
            fflush(stdout);
            fprintf(stderr, "     error: in tests/valid.json: %s\n", jsonlogic_get_error_message(error));
            break;
        }

        size_t size = 0;
        JsonLogic_Handle test_name = jsonlogic_get_utf16(test, u"name");
        const char16_t *str = jsonlogic_get_string_content(test_name, &size);
        if (str == NULL) {
            puts("FAIL");
            fflush(stdout);
            if (jsonlogic_is_string(test_name)) {
                fprintf(stderr, "     error: in tests/valid.json: error getting test name\n");
            } else {
                fprintf(stderr, "     error: in tests/valid.json: error getting test name: ");
                jsonlogic_println(stderr, test_name);
            }
            jsonlogic_decref(test);
            jsonlogic_decref(test_name);
            continue;
        }

        printf(" - "); jsonlogic_print_utf16(stdout, str, size); printf(" ... ");
        fflush(stdout);

        JsonLogic_Handle code   = jsonlogic_get_utf16(test, u"code");
        JsonLogic_Handle result = jsonlogic_apply_custom(rule, code, &ops);

        if (jsonlogic_is_false(result)) {
            print_ok();
            ++ pass_count;
        } else {
            print_failed();
            fflush(stdout);
            fprintf(stderr, "     error: Wrong result\n");
            //fprintf(stderr, "     logic: "); jsonlogic_println(stderr, rule);
            fprintf(stderr, "      data: "); jsonlogic_println(stderr, code);
            fprintf(stderr, "  expected: false\n");
            fprintf(stderr, "    actual: "); jsonlogic_println(stderr, result);
            fputc('\n', stderr);
        }

        jsonlogic_decref(result);
        jsonlogic_decref(code);
        jsonlogic_decref(test_name);
        jsonlogic_decref(test);
    }

    jsonlogic_iter_free(&iter);

    putchar('\n');
    puts("CertLogic TestSuit");
    for (const char **filename = CertLogicTests; *filename; ++ filename) {
        JsonLogic_Handle test_group = parse_file(*filename);

        if (JSONLOGIC_IS_ERROR(test_group)) {
            ++ test_count;
            fprintf(stderr, "*** error: loading tests from %s: %s\n", *filename,
                jsonlogic_get_error_message(jsonlogic_get_error(test_group)));
            continue;
        }

        size_t size = 0;
        JsonLogic_Handle test_group_name = jsonlogic_get_utf16(test_group, u"name");
        const char16_t *str = jsonlogic_get_string_content(test_group_name, &size);
        if (str == NULL) {
            if (jsonlogic_is_string(test_group_name)) {
                fprintf(stderr, "*** error: in %s: error getting test group name\n", *filename);
            } else {
                fprintf(stderr, "*** error: in %s: error getting test group name: ", *filename);
                jsonlogic_println(stderr, test_group_name);
            }
            jsonlogic_decref(test_group);
            jsonlogic_decref(test_group_name);
            continue;
        }

        JsonLogic_Handle cases = jsonlogic_get_utf16(test_group, u"cases");
        printf(" - "); jsonlogic_println_utf16(stdout, str, size);
        fflush(stdout);

        JsonLogic_Iterator case_iter = jsonlogic_iter(cases);

        for (;;) {
            JsonLogic_Handle test_case = jsonlogic_iter_next(&case_iter);
            JsonLogic_Error error = jsonlogic_get_error(test_case);

            if (error == JSONLOGIC_ERROR_STOP_ITERATION) {
                break;
            } else if (error != JSONLOGIC_ERROR_SUCCESS) {
                ++ test_count;
                fprintf(stderr, "*** error: loading tests from %s: %s\n", *filename,
                    jsonlogic_get_error_message(error));
                break;
            }

            ++ test_count;
            test_context.newline = false;
            test_context.passed  = true;
            JsonLogic_Handle test_case_name = jsonlogic_get_utf16(test_case, u"name");

            str = jsonlogic_get_string_content(test_case_name, &size);
            if (str == NULL) {
                if (jsonlogic_is_string(test_group_name)) {
                    fprintf(stderr, "*** error: in %s: error getting test case name\n", *filename);
                } else {
                    fprintf(stderr, "*** error: in %s: error getting test case name: ", *filename);
                    jsonlogic_println(stderr, test_group_name);
                }
                jsonlogic_decref(test_case);
                jsonlogic_decref(test_case_name);
                continue;
            }
            printf("   - "); jsonlogic_print_utf16(stdout, str, size); printf(" ... ");
            fflush(stdout);

            JsonLogic_Handle logic      = jsonlogic_get_utf16(test_case, u"certLogicExpression");
            JsonLogic_Handle assertions = jsonlogic_get_utf16(test_case, u"assertions");

            JsonLogic_Iterator assert_iter = jsonlogic_iter(assertions);

            for (;;) {
                JsonLogic_Handle assertion = jsonlogic_iter_next(&assert_iter);
                error = jsonlogic_get_error(assertion);

                if (error == JSONLOGIC_ERROR_STOP_ITERATION) {
                    if (!test_context.newline) {
                        print_ok();
                        fflush(stdout);
                        test_context.newline = false;
                    }
                    break;
                } else if (error != JSONLOGIC_ERROR_SUCCESS) {
                    FAIL();
                    fprintf(stderr, "     error: in %s: %s\n", *filename, jsonlogic_get_error_message(error));
                    break;
                }

                JsonLogic_Handle assert_logic = jsonlogic_get_utf16(assertion, u"certLogicExpression");
                JsonLogic_Handle data = jsonlogic_get_utf16(assertion, u"data");
                JsonLogic_Handle expected = jsonlogic_get_utf16(assertion, u"expected");
                JsonLogic_Handle used_logic = JSONLOGIC_IS_NULL(assert_logic) ? logic : assert_logic;
                JsonLogic_Handle actual = certlogic_apply(used_logic, data);

                if (!jsonlogic_deep_strict_equal(actual, expected)) {
                    FAIL();
                    fprintf(stderr, "     error: Wrong result\n");
                    fprintf(stderr, "     logic: "); jsonlogic_println(stderr, used_logic);
                    fprintf(stderr, "      data: "); jsonlogic_println(stderr, data);
                    fprintf(stderr, "  expected: "); jsonlogic_println(stderr, expected);
                    fprintf(stderr, "    actual: "); jsonlogic_println(stderr, actual);
                    fputc('\n', stderr);
                }

                jsonlogic_decref(assertion);
                jsonlogic_decref(assert_logic);
                jsonlogic_decref(data);
                jsonlogic_decref(expected);
                jsonlogic_decref(actual);
            }

            if (test_context.passed) {
                ++ pass_count;
            }

            jsonlogic_iter_free(&assert_iter);

            jsonlogic_decref(assertions);
            jsonlogic_decref(logic);
            jsonlogic_decref(test_case);
            jsonlogic_decref(test_case_name);
        }

        jsonlogic_iter_free(&case_iter);

        jsonlogic_decref(cases);
        jsonlogic_decref(test_group_name);
        jsonlogic_decref(test_group);
    }

    printf("\ntests: %" PRIuPTR ", failed: ", test_count);
    size_t failed_count = test_count - pass_count;
    if (failed_count > 0) {
        printf(COL_RED "%" PRIuPTR COL_NORMAL, failed_count);
    } else {
        putchar('0');
    }
    printf(", passed: ");
    if (pass_count > 0) {
        printf(COL_GREEN "%" PRIuPTR COL_NORMAL, pass_count);
    } else {
        putchar('0');
    }
    putchar('\n');

    status = test_count == pass_count ? 0 : 1;

    goto cleanup;

error:
    status = 1;

cleanup:

    jsonlogic_decref(tests);
    jsonlogic_decref(rule);
    jsonlogic_decref(valid_examples);
    jsonlogic_decref(invalid_examples);
    jsonlogic_operations_free(&ops);

    return status;
}

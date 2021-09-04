// for fileno()
#define _POSIX_C_SOURCE 1

#include "jsonlogic_intern.h"
#include "jsonlogic_extras.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <assert.h>

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

void test_logging(TestContext *test_context) {
    JsonLogic_Handle logic  = JsonLogic_Null;
    JsonLogic_Handle result = JsonLogic_Null;
    int pair[2] = { -1, -1 };
    int stdout_backup = dup(STDOUT_FILENO);

    TEST_ASSERT_FMT(stdout_backup != -1, "dup(STDOUT_FILENO): %s", strerror(errno));
#if defined(_WIN32) || defined(_WIN64)
    TEST_ASSERT_FMT(_pipe(pair, 1024, 0) != -1, "_pipe(&pair, 1024, 0): %s", strerror(errno));
#else
    TEST_ASSERT_FMT(pipe(pair) != -1, "pipe(&pair): %s", strerror(errno));
#endif
    TEST_ASSERT_FMT(dup2(pair[1], STDOUT_FILENO) != -1, "dup2(pair[1], STDOUT_FILENO): %s", strerror(errno))

    logic  = jsonlogic_parse("{\"log\": [1]}", NULL);
    result = jsonlogic_apply(logic, JsonLogic_Null);

    TEST_ASSERT(jsonlogic_get_error(result) == JSONLOGIC_ERROR_SUCCESS);

    char buf[2];

    TEST_ASSERT(read(pair[0], buf, sizeof(buf)) == 2);
    TEST_ASSERT(memcmp(buf, "1\n", sizeof(buf)) == 0);

cleanup:
    jsonlogic_decref(result);
    jsonlogic_decref(logic);

    if (pair[0] != -1) {
        close(pair[0]);
    }

    if (pair[1] != -1) {
        close(pair[1]);
    }

    if (stdout_backup != -1) {
        dup2(stdout_backup, STDOUT_FILENO);
        close(stdout_backup);
    }
}

void test_edge_cases(TestContext *test_context) {
    JsonLogic_Handle logic = jsonlogic_parse("{\"var\": \"\"}", NULL);
    JsonLogic_Handle fallback = jsonlogic_string_from_latin1("fallback");

    TEST_ASSERT(jsonlogic_is_string(fallback));

    TEST_ASSERT_MSG(jsonlogic_deep_strict_equal(jsonlogic_apply(JsonLogic_Null, JsonLogic_Null), JsonLogic_Null), "Called with null");
    TEST_ASSERT_MSG(jsonlogic_deep_strict_equal(jsonlogic_apply(logic, jsonlogic_number_from(0)), jsonlogic_number_from(0)), "Var when date is 'falsy'");
    TEST_ASSERT_MSG(jsonlogic_deep_strict_equal(jsonlogic_apply(logic, JsonLogic_Null), JsonLogic_Null), "Var when date is null");

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

JsonLogic_Handle add_to_a(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    assert(context != NULL);
    TestCustomOps *ptr = context;
    double b = argc == 0 ? 1 : jsonlogic_to_double(args[0]);
    return jsonlogic_number_from(ptr->a += b);
}

JsonLogic_Handle fives_add(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    double i = argc == 0 ? 0.0 : jsonlogic_to_double(args[0]);
    return jsonlogic_number_from(i + 5);
}

JsonLogic_Handle fives_subtract(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    double i = argc == 0 ? 0.0 : jsonlogic_to_double(args[0]);
    return jsonlogic_number_from(i - 5);
}

JsonLogic_Handle times(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
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

JsonLogic_Handle array_times(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
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
    JsonLogic_Handle result = jsonlogic_apply_custom(logic, JsonLogic_Null, NULL, 0);

    TEST_ASSERT(jsonlogic_get_error(result) == JSONLOGIC_ERROR_ILLEGAL_OPERATION);
    jsonlogic_decref(result);
    result = JsonLogic_Null;

    JsonLogic_Operations ops = jsonlogic_operations(
        jsonlogic_operation(u"add_to_a", &context, add_to_a),
    );

    // New operation executes, returns desired result
    // No args
    result = jsonlogic_apply_custom(logic, JsonLogic_Null, ops.entries, ops.size);

    TEST_ASSERT(jsonlogic_deep_strict_equal(result, jsonlogic_number_from(1)));

    // Unary syntactic sugar
    jsonlogic_decref(logic);
    logic  = jsonlogic_parse("{\"add_to_a\":41}", NULL);
    result = jsonlogic_apply_custom(logic, JsonLogic_Null, ops.entries, ops.size);

    TEST_ASSERT(jsonlogic_deep_strict_equal(result, jsonlogic_number_from(42)));
    // New operation had side effects.
    TEST_ASSERT(context.a == 42.0);

    JsonLogic_Operations ops2 = jsonlogic_operations(
        jsonlogic_operation(u"fives.add", &context, fives_add),
        jsonlogic_operation(u"fives.subtract", &context, fives_subtract),
    );

    jsonlogic_decref(logic);
    logic  = jsonlogic_parse("{\"fives.add\":37}", NULL);
    result = jsonlogic_apply_custom(logic, JsonLogic_Null, ops2.entries, ops2.size);

    TEST_ASSERT(jsonlogic_deep_strict_equal(result, jsonlogic_number_from(42)));

    jsonlogic_decref(logic);
    logic  = jsonlogic_parse("{\"fives.subtract\":47}", NULL);
    result = jsonlogic_apply_custom(logic, JsonLogic_Null, ops2.entries, ops2.size);

    TEST_ASSERT(jsonlogic_deep_strict_equal(result, jsonlogic_number_from(42)));

    JsonLogic_Operations ops3 = jsonlogic_operations(
        jsonlogic_operation(u"times", &context, times),
    );

    // Calling a method with multiple var as arguments.
    jsonlogic_decref(logic);
    logic = jsonlogic_parse("{\"times\":[{\"var\":\"a\"},{\"var\":\"b\"}]}", NULL);
    data  = jsonlogic_parse("{\"a\":6,\"b\":7}", NULL);

    result = jsonlogic_apply_custom(logic, data, ops3.entries, ops3.size);

    TEST_ASSERT(jsonlogic_deep_strict_equal(result, jsonlogic_number_from(42)));

    result = jsonlogic_apply_custom(logic, data, NULL, 0);
    TEST_ASSERT(jsonlogic_get_error(result) == JSONLOGIC_ERROR_ILLEGAL_OPERATION);

    // Calling a method that takes an array, but the inside of the array has rules, too
    JsonLogic_Operations ops4 = jsonlogic_operations(
        jsonlogic_operation(u"array_times", &context, array_times),
    );

    jsonlogic_decref(logic);
    logic = jsonlogic_parse("{\"array_times\":[[{\"var\":\"a\"},{\"var\":\"b\"}]]}", NULL);

    result = jsonlogic_apply_custom(logic, data, ops4.entries, ops4.size);

    TEST_ASSERT(jsonlogic_deep_strict_equal(result, jsonlogic_number_from(42)));

cleanup:
    jsonlogic_decref(logic);
    jsonlogic_decref(result);
    jsonlogic_decref(data);
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
    TestShortCircuit context = {
        .conditions   = JSONLOGIC_ARRAYBUF_INIT,
        .consequences = JSONLOGIC_ARRAYBUF_INIT,
        .list         = JSONLOGIC_ARRAYBUF_INIT,
    };

    JsonLogic_Operations ops = jsonlogic_operations(
        jsonlogic_operation(u"push",      &context, push_list),
        jsonlogic_operation(u"push.else", &context, push_else),
        jsonlogic_operation(u"push.if",   &context, push_if),
        jsonlogic_operation(u"push.then", &context, push_then),
    );

    JsonLogic_Handle logic = jsonlogic_parse("{\"if\":["
        "{\"push.if\":[true]},"
        "{\"push.then\":[\"first\"]},"
        "{\"push.if\":[false]},"
        "{\"push.then\":[\"second\"]},"
        "{\"push.else\":[\"third\"]},"
    "]}", NULL);

    jsonlogic_decref(jsonlogic_apply_custom(
        logic, JsonLogic_Null,
        ops.entries, ops.size));

    JsonLogic_Handle data = jsonlogic_parse("[true]", NULL);

    TEST_ASSERT(jsonlogic_deep_strict_equal(
        jsonlogic_array_into_handle(context.conditions.array),
        data
    ));

    jsonlogic_decref(data);
    data = jsonlogic_parse("[\"first\"]", NULL);

    TEST_ASSERT(jsonlogic_deep_strict_equal(
        jsonlogic_array_into_handle(context.consequences.array),
        data
    ));

    jsonlogic_arraybuf_clear(&context.conditions);
    jsonlogic_arraybuf_clear(&context.consequences);

    jsonlogic_decref(logic);
    logic = jsonlogic_parse("{\"if\":["
        "{\"push.if\":[false]},"
        "{\"push.then\":[\"first\"]},"
        "{\"push.if\":[false]},"
        "{\"push.then\":[\"second\"]},"
        "{\"push.else\":[\"third\"]},"
    "]}", NULL);

    jsonlogic_decref(jsonlogic_apply_custom(
        logic, JsonLogic_Null,
        ops.entries, ops.size));

    jsonlogic_decref(data);
    data = jsonlogic_parse("[false, false]", NULL);

    TEST_ASSERT(jsonlogic_deep_strict_equal(
        jsonlogic_array_into_handle(context.conditions.array),
        data
    ));

    jsonlogic_decref(data);
    data = jsonlogic_parse("[\"third\"]", NULL);

    TEST_ASSERT(jsonlogic_deep_strict_equal(
        jsonlogic_array_into_handle(context.consequences.array),
        data
    ));

    jsonlogic_decref(logic);
    logic = jsonlogic_parse("{\"and\": [{\"push\": [false]}, {\"push\": [false]}]}", NULL);

    jsonlogic_decref(jsonlogic_apply_custom(
        logic, JsonLogic_Null,
        ops.entries, ops.size));

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
        ops.entries, ops.size));

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
        ops.entries, ops.size));

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
        ops.entries, ops.size));

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
        ops.entries, ops.size));

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
        ops.entries, ops.size));

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
        ops.entries, ops.size));

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
        ops.entries, ops.size));

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

    data = "{\"foo\": \"bar\" \"a\":\"b\"}";
    jsonlogic_decref(handle);
    handle = jsonlogic_parse(data, NULL);
    TEST_ASSERT_X(jsonlogic_deep_strict_equal(handle, JsonLogic_Error_SyntaxError), {
        fprintf(stderr, "     error: Wrong result\n");
        fprintf(stderr, "      data: %s\n", data);
        fprintf(stderr, "  expected: "); jsonlogic_println(stderr, JsonLogic_Error_SyntaxError);
        fprintf(stderr, "    actual: "); jsonlogic_println(stderr, handle);
        fputc('\n', stderr);
    });

    data = "[]]";
    jsonlogic_decref(handle);
    handle = jsonlogic_parse(data, NULL);
    TEST_ASSERT_X(jsonlogic_deep_strict_equal(handle, JsonLogic_Error_SyntaxError), {
        fprintf(stderr, "     error: Wrong result\n");
        fprintf(stderr, "      data: %s\n", data);
        fprintf(stderr, "  expected: "); jsonlogic_println(stderr, JsonLogic_Error_SyntaxError);
        fprintf(stderr, "    actual: "); jsonlogic_println(stderr, handle);
        fputc('\n', stderr);
    });

    data = "\"\\j\"";
    jsonlogic_decref(handle);
    handle = jsonlogic_parse(data, NULL);
    TEST_ASSERT_X(jsonlogic_deep_strict_equal(handle, JsonLogic_Error_SyntaxError), {
        fprintf(stderr, "     error: Wrong result\n");
        fprintf(stderr, "      data: %s\n", data);
        fprintf(stderr, "  expected: "); jsonlogic_println(stderr, JsonLogic_Error_SyntaxError);
        fprintf(stderr, "    actual: "); jsonlogic_println(stderr, handle);
        fputc('\n', stderr);
    });

    data = "\"\\u123g\"";
    jsonlogic_decref(handle);
    handle = jsonlogic_parse(data, NULL);
    TEST_ASSERT_X(jsonlogic_deep_strict_equal(handle, JsonLogic_Error_SyntaxError), {
        fprintf(stderr, "     error: Wrong result\n");
        fprintf(stderr, "      data: %s\n", data);
        fprintf(stderr, "  expected: "); jsonlogic_println(stderr, JsonLogic_Error_SyntaxError);
        fprintf(stderr, "    actual: "); jsonlogic_println(stderr, handle);
        fputc('\n', stderr);
    });

    data = "\"foo";
    jsonlogic_decref(handle);
    handle = jsonlogic_parse(data, NULL);
    TEST_ASSERT_X(jsonlogic_deep_strict_equal(handle, JsonLogic_Error_SyntaxError), {
        fprintf(stderr, "     error: Wrong result\n");
        fprintf(stderr, "      data: %s\n", data);
        fprintf(stderr, "  expected: "); jsonlogic_println(stderr, JsonLogic_Error_SyntaxError);
        fprintf(stderr, "    actual: "); jsonlogic_println(stderr, handle);
        fputc('\n', stderr);
    });

    data = "\"foo\",";
    jsonlogic_decref(handle);
    handle = jsonlogic_parse(data, NULL);
    TEST_ASSERT_X(jsonlogic_deep_strict_equal(handle, JsonLogic_Error_SyntaxError), {
        fprintf(stderr, "     error: Wrong result\n");
        fprintf(stderr, "      data: %s\n", data);
        fprintf(stderr, "  expected: "); jsonlogic_println(stderr, JsonLogic_Error_SyntaxError);
        fprintf(stderr, "    actual: "); jsonlogic_println(stderr, handle);
        fputc('\n', stderr);
    });

cleanup:
    jsonlogic_decref(handle);
}

void test_extras(TestContext *test_context) {
    // TODO: make zip work also on strings and object (keys)?
    JsonLogic_Handle logic    = jsonlogic_parse("{\"zip\": [[1,2,3],[\"a\",\"b\"]]}", NULL);
    JsonLogic_Handle expected = jsonlogic_parse("[[1,\"a\"],[2,\"b\"]]", NULL);
    JsonLogic_Handle actual   = jsonlogic_apply_custom(logic, JsonLogic_Null, JsonLogic_Extras.entries, JsonLogic_Extras.size);

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
    TEST_DECL("Logging", logging),
    TEST_DECL("Edge Cases", edge_cases),
    TEST_DECL("Expanding functionality with custom operators", custom_operators),
    TEST_DECL("Control structures don't eval depth-first", short_circuit),
    TEST_DECL("Sub-string of non-ASCII strings", substr),
    TEST_DECL("Test extra operators", extras),
    TEST_END,
};

JsonLogic_Handle parse_file(const char *filename) {
    FILE *fp = fopen(filename, "r");
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

int main(int argc, char *argv[]) {
    const JsonLogic_Operations ops_for_merge[] = {
        JsonLogic_Extras,
        (JsonLogic_Operations)jsonlogic_operations(
            jsonlogic_operation(u"now",       NULL, mock_now),
            jsonlogic_operation(u"timeSince", NULL, mock_time_since),
        ),
    };

    JsonLogic_Handle tests = JsonLogic_Null;
    JsonLogic_Handle rule  = JsonLogic_Null;
    JsonLogic_Handle valid_examples   = JsonLogic_Null;
    JsonLogic_Handle invalid_examples = JsonLogic_Null;
    int status = 0;

    size_t ops_size = 0;
    JsonLogic_Operation_Entry *ops = jsonlogic_operations_merge(
        ops_for_merge,
        sizeof(ops_for_merge) / sizeof(ops_for_merge[0]),
        &ops_size);

    if (ops == NULL) {
        fprintf(stderr, "*** error: merging operations: %s\n", strerror(errno));
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

    JsonLogic_Iterator iter = jsonlogic_iter(tests);

    putchar('\n');
    puts("Tests from https://jsonlogic.com/tests.json");
    for (;;) {
        JsonLogic_Handle test = jsonlogic_iter_next(&iter);
        JsonLogic_Error error = jsonlogic_get_error(test);

        if (error == JSONLOGIC_ERROR_STOP_ITERATION) {
            if (!test_context.newline) {
                puts("OK");
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
                puts("OK");
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
    puts("Test European Health Certificate rules with valid data");
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
        JsonLogic_Handle result = jsonlogic_apply_custom(rule, code, ops, ops_size);

        if (jsonlogic_is_true(result)) {
            puts("OK");
            ++ pass_count;
        } else {
            puts("FAIL");
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
    puts("Test European Health Certificate rules with invalid data");
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
        JsonLogic_Handle result = jsonlogic_apply_custom(rule, code, ops, ops_size);

        if (jsonlogic_is_false(result)) {
            puts("OK");
            ++ pass_count;
        } else {
            puts("FAIL");
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

    printf("\ntests: %" PRIuPTR ", failed: %" PRIuPTR ", passed: %" PRIuPTR "\n",
        test_count, test_count - pass_count, pass_count);

    status = test_count == pass_count ? 0 : 1;

    goto cleanup;

error:
    status = 1;

cleanup:

    jsonlogic_decref(tests);
    jsonlogic_decref(rule);
    jsonlogic_decref(valid_examples);
    jsonlogic_decref(invalid_examples);
    free(ops);

    return status;
}

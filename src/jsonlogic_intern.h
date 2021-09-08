#ifndef JSONLOGIC_INTERN_H
#define JSONLOGIC_INTERN_H
#pragma once

// for strtod_l and locale_t
#define _GNU_SOURCE 1

#include "jsonlogic_defs.h"

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <locale.h>

#define JsonLogic_PtrMask  (~(uint64_t)0xffff000000000000)
#define JsonLogic_TypeMask  ((uint64_t)0xffff000000000000)
#define JsonLogic_MaxNumber ((uint64_t)0xfff8000000000000)

#define JSONLOGIC_MALLOC(HEAD_SIZE, ITEM_SIZE, ITEM_COUNT) \
    ((ITEM_COUNT) >= (SIZE_MAX - (HEAD_SIZE)) / (ITEM_SIZE) ? (errno = ENOMEM, NULL) : \
    malloc((HEAD_SIZE) + (ITEM_SIZE) * (ITEM_COUNT)))

#define JSONLOGIC_REALLOC(PTR, HEAD_SIZE, ITEM_SIZE, ITEM_COUNT) \
    ((ITEM_COUNT) >= (SIZE_MAX - (HEAD_SIZE)) / (ITEM_SIZE) ? (errno = ENOMEM, NULL) : \
    realloc((PTR), (HEAD_SIZE) + (ITEM_SIZE) * (ITEM_COUNT)))

#define JSONLOGIC_MALLOC_OBJECT(ITEM_COUNT) \
    (JsonLogic_Object*)JSONLOGIC_MALLOC(sizeof(JsonLogic_Object) - sizeof(JsonLogic_Object_Entry), sizeof(JsonLogic_Object_Entry), (ITEM_COUNT))

#define JSONLOGIC_MALLOC_EMPTY_OBJECT() \
    (JsonLogic_Object*)malloc(sizeof(JsonLogic_Object) - sizeof(JsonLogic_Object_Entry))

#define JSONLOGIC_MALLOC_ARRAY(ITEM_COUNT) \
    (JsonLogic_Array*)JSONLOGIC_MALLOC(sizeof(JsonLogic_Array) - sizeof(JsonLogic_Handle), sizeof(JsonLogic_Handle), (ITEM_COUNT))

#define JSONLOGIC_REALLOC_ARRAY(ARRAY, ITEM_COUNT) \
    (JsonLogic_Array*)JSONLOGIC_REALLOC((ARRAY), sizeof(JsonLogic_Array) - sizeof(JsonLogic_Handle), sizeof(JsonLogic_Handle), (ITEM_COUNT))

#define JSONLOGIC_MALLOC_EMPTY_ARRAY() \
    (JsonLogic_Array*)malloc(sizeof(JsonLogic_Array) - sizeof(JsonLogic_Handle))

#define JSONLOGIC_MALLOC_STRING(SIZE) \
    (JsonLogic_String*)JSONLOGIC_MALLOC(sizeof(JsonLogic_String) - sizeof(char16_t), sizeof(char16_t), (SIZE))

#define JSONLOGIC_REALLOC_STRING(STRING, SIZE) \
    (JsonLogic_String*)JSONLOGIC_REALLOC((STRING), sizeof(JsonLogic_String) - sizeof(char16_t), sizeof(char16_t), (SIZE))

#define JSONLOGIC_MALLOC_EMPTY_STRING() \
    (JsonLogic_String*)malloc(sizeof(JsonLogic_String) - sizeof(char16_t))

#if defined(NDEBUG)
    #define JSONLOGIC_DEBUG(...)
    #define JSONLOGIC_ERROR(...)
    #define JSONLOGIC_DEBUG_UTF16(...)
    #define JSONLOGIC_ERROR_UTF16(...)
    #define JSONLOGIC_ASSERT(...)
    #define JSONLOGIC_DEBUG_CODE(...)
#else
    #define JSONLOGIC_DEBUG_(PREFIX, FMT, ...) \
        fprintf(stderr, "" PREFIX "%s:%u: in %s: " FMT "\n", \
            __FILE__, __LINE__, __func__, __VA_ARGS__)

    #define JSONLOGIC_DEBUG_UTF16_(PREFIX, FMT, STR, SIZE, ...) \
        ( \
            fprintf(stderr, "" PREFIX "%s:%u: in %s: " FMT ": ", \
                __FILE__, __LINE__, __func__, __VA_ARGS__), \
            jsonlogic_print_utf16(stderr, (STR), (SIZE)), \
            fputc('\n', stderr) \
        )

    #define JSONLOGIC_DEBUG(...) JSONLOGIC_DEBUG_("*** debug: ", __VA_ARGS__)
    #define JSONLOGIC_DEBUG_UTF16(...) JSONLOGIC_DEBUG_UTF16_("*** debug: ", __VA_ARGS__)

    #if defined(JSONLOGIC_NO_ABORT_ON_ERROR)
        #define JSONLOGIC_ERROR(...)       JSONLOGIC_DEBUG_("*** error: ", __VA_ARGS__)
        #define JSONLOGIC_ERROR_UTF16(...) JSONLOGIC_DEBUG_UTF16_("*** error: ", __VA_ARGS__)
    #else
        #include <stdlib.h>

        #define JSONLOGIC_ERROR(...) \
            JSONLOGIC_DEBUG_("*** error: ", __VA_ARGS__); \
            abort();

        #define JSONLOGIC_ERROR_UTF16(...) \
            JSONLOGIC_DEBUG_UTF16_("*** error: ", __VA_ARGS__); \
            abort();
    #endif

    #define JSONLOGIC_ASSERT(EXPR, ...) \
        if (!(EXPR)) { JSONLOGIC_ERROR(__VA_ARGS__); }

    #define JSONLOGIC_DEBUG_CODE(...) __VA_ARGS__
#endif

#define JSONLOGIC_ERROR_MEMORY() \
    JSONLOGIC_ERROR("%s", "memory allocation failed")

#ifdef __cplusplus
extern "C" {
#endif

union JsonLogic_Handle;
struct JsonLogic_Array;
struct JsonLogic_String;
struct JsonLogic_Object;

#define JSONLOGIC_CAST_STRING(handle) ((JsonLogic_String*)(uintptr_t)((handle).intptr & JsonLogic_PtrMask))
#define JSONLOGIC_CAST_ARRAY( handle) ((JsonLogic_Array*) (uintptr_t)((handle).intptr & JsonLogic_PtrMask))
#define JSONLOGIC_CAST_OBJECT(handle) ((JsonLogic_Object*)(uintptr_t)((handle).intptr & JsonLogic_PtrMask))

#define JSONLOGIC_IS_STRING(handle)  (((handle).intptr & JsonLogic_TypeMask) == JsonLogic_Type_String)
#define JSONLOGIC_IS_OBJECT(handle)  (((handle).intptr & JsonLogic_TypeMask) == JsonLogic_Type_Object)
#define JSONLOGIC_IS_ARRAY(handle)   (((handle).intptr & JsonLogic_TypeMask) == JsonLogic_Type_Array)
#define JSONLOGIC_IS_BOOLEAN(handle) (((handle).intptr & JsonLogic_TypeMask) == JsonLogic_Type_Boolean)
#define JSONLOGIC_IS_NULL(handle)    ((handle).intptr == JsonLogic_Type_Null)
#define JSONLOGIC_IS_NUMBER(handle)  ((handle).intptr < JsonLogic_MaxNumber)
#define JSONLOGIC_IS_TRUE(handle)    ((handle).intptr == (JsonLogic_Type_Boolean | 1))
#define JSONLOGIC_IS_FALSE(handle)   ((handle).intptr == (JsonLogic_Type_Boolean | 0))

#define JSONLOGIC_IS_ERROR_(handle)  (((handle).intptr & JsonLogic_TypeMask) == JsonLogic_Type_Error)

#if defined(NDEBUG) || 1
    #define JSONLOGIC_IS_ERROR(handle) JSONLOGIC_IS_ERROR_(handle)
#else
    // extra debug stuf prints error trace wherever JSONLOGIC_IS_ERROR() is used:
    #define JSONLOGIC_IS_ERROR(handle) \
        (JSONLOGIC_IS_ERROR_(handle) ? (JSONLOGIC_DEBUG("trace: %s", jsonlogic_get_error_message((handle).intptr)), true) : false)
#endif

#define JSONLOGIC_DECL_UTF16(NAME) \
    JSONLOGIC_PRIVATE extern const size_t NAME##_SIZE; \
    JSONLOGIC_PRIVATE extern const char16_t NAME[];

#define JSONLOGIC_DEF_UTF16(NAME, STR) \
    const size_t NAME##_SIZE = sizeof(STR) / sizeof(STR[0]) - 1; \
    const char16_t NAME[] = STR;

JSONLOGIC_DECL_UTF16(JSONLOGIC_NULL_STRING)
JSONLOGIC_DECL_UTF16(JSONLOGIC_FALSE_STRING)
JSONLOGIC_DECL_UTF16(JSONLOGIC_TRUE_STRING)
JSONLOGIC_DECL_UTF16(JSONLOGIC_NAN_STRING)
JSONLOGIC_DECL_UTF16(JSONLOGIC_INFINITY_STRING)
JSONLOGIC_DECL_UTF16(JSONLOGIC_POS_INFINITY_STRING)
JSONLOGIC_DECL_UTF16(JSONLOGIC_NEG_INFINITY_STRING)

JSONLOGIC_DECL_UTF16(JSONLOGIC_ALT_IF)
JSONLOGIC_DECL_UTF16(JSONLOGIC_ALL)
JSONLOGIC_DECL_UTF16(JSONLOGIC_AND)
JSONLOGIC_DECL_UTF16(JSONLOGIC_FILTER)
JSONLOGIC_DECL_UTF16(JSONLOGIC_IF)
JSONLOGIC_DECL_UTF16(JSONLOGIC_MAP)
JSONLOGIC_DECL_UTF16(JSONLOGIC_NONE)
JSONLOGIC_DECL_UTF16(JSONLOGIC_OR)
JSONLOGIC_DECL_UTF16(JSONLOGIC_REDUCE)
JSONLOGIC_DECL_UTF16(JSONLOGIC_SOME)

JSONLOGIC_DECL_UTF16(JSONLOGIC_ACCUMULATOR)
JSONLOGIC_DECL_UTF16(JSONLOGIC_CURRENT)
JSONLOGIC_DECL_UTF16(JSONLOGIC_DATA)

#define JSONLOGIC_ACCUMULATOR_HASH 0xd70624b7f0caa74d
#define JSONLOGIC_CURRENT_HASH     0x1e84ef3a9c8034b4
#define JSONLOGIC_DATA_HASH        0xf8c35b7f283d7585

#define JSONLOGIC_STATIC_ARGC 8

#define JSONLOGIC_IS_OP(OPSRT, OP) \
    jsonlogic_utf16_equals((OPSRT)->str, (OPSRT)->size, (JSONLOGIC_##OP), (JSONLOGIC_##OP##_SIZE))

#define JSONLOGIC_HASH_UNSET ((uint64_t)0)

typedef struct JsonLogic_String {
    size_t refcount;
    uint64_t hash;
    size_t size;
    char16_t str[1];
} JsonLogic_String;

typedef struct JsonLogic_Array {
    size_t refcount;
    size_t size;
    JsonLogic_Handle items[1];
} JsonLogic_Array;

typedef struct JsonLogic_Object {
    size_t refcount;
    size_t size;
    size_t used;
    size_t first_index;
    JsonLogic_Object_Entry entries[1];
} JsonLogic_Object;

#ifdef __cplusplus
}
#endif

#include "jsonlogic.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(_WIN64)
    #define JSONLOGIC_LOCALE_T _locale_t
    #define JSONLOGIC_STRTOD_L _strtod_l
    #define JSONLOGIC_CREATE_C_LOCALE() _create_locale(LC_ALL, "C")
#else
    #define JSONLOGIC_LOCALE_T locale_t
    #define JSONLOGIC_STRTOD_L strtod_l
    #define JSONLOGIC_CREATE_C_LOCALE() newlocale(LC_ALL_MASK, "C", NULL)
#endif
JSONLOGIC_PRIVATE extern JSONLOGIC_LOCALE_T JsonLogic_C_Locale;

JSONLOGIC_PRIVATE JsonLogic_Array *jsonlogic_array_with_capacity(size_t size);

JSONLOGIC_PRIVATE size_t jsonlogic_object_get_index_utf16_with_hash(const JsonLogic_Object *object, uint64_t hash, const char16_t *key, size_t key_size);
JSONLOGIC_PRIVATE size_t jsonlogic_object_get_index_utf16(const JsonLogic_Object *object, const char16_t *key, size_t key_size);
JSONLOGIC_PRIVATE size_t jsonlogic_object_get_index(const JsonLogic_Object *object, JsonLogic_Handle key);

JSONLOGIC_PRIVATE void jsonlogic_string_free(JsonLogic_String *string);
JSONLOGIC_PRIVATE void jsonlogic_array_free (JsonLogic_Array  *array);
JSONLOGIC_PRIVATE void jsonlogic_object_free(JsonLogic_Object *object);

JSONLOGIC_PRIVATE inline JsonLogic_Handle jsonlogic_string_into_handle(JsonLogic_String *string) {
    if (string == NULL) {
        return JsonLogic_Error_OutOfMemory;
    }
    assert(string->refcount == 1);
    return (JsonLogic_Handle){ .intptr = ((uint64_t)(uintptr_t)string) | JsonLogic_Type_String };
}

JSONLOGIC_PRIVATE inline JsonLogic_Handle jsonlogic_array_into_handle(JsonLogic_Array *array) {
    if (array == NULL) {
        return JsonLogic_Error_OutOfMemory;
    }
    assert(array->refcount == 1);
    return (JsonLogic_Handle){ .intptr = ((uint64_t)(uintptr_t)array) | JsonLogic_Type_Array };
}

JSONLOGIC_PRIVATE inline JsonLogic_Handle jsonlogic_object_into_handle(JsonLogic_Object *object) {
    if (object == NULL) {
        return JsonLogic_Error_OutOfMemory;
    }
    assert(object->refcount == 1);
    return (JsonLogic_Handle){ .intptr = ((uint64_t)(uintptr_t)object) | JsonLogic_Type_Object };
}

#ifndef NDEBUG
JSONLOGIC_PRIVATE void jsonlogic_object_debug(const JsonLogic_Object *object);
#endif

JSONLOGIC_PRIVATE bool jsonlogic_string_equals(const JsonLogic_String *a, const JsonLogic_String *b);

JSONLOGIC_PRIVATE inline int jsonlogic_string_compare(const JsonLogic_String *a, const JsonLogic_String *b) {
    return jsonlogic_utf16_compare(a->str, a->size, b->str, b->size);
}

JSONLOGIC_PRIVATE size_t jsonlogic_utf16_to_index(const char16_t *str, size_t size);

JSONLOGIC_PRIVATE inline size_t jsonlogic_string_to_index(const JsonLogic_String *string) {
    return jsonlogic_utf16_to_index(string->str, string->size);
}

JSONLOGIC_PRIVATE JsonLogic_Array *jsonlogic_array_truncate(JsonLogic_Array *array, size_t size);

#define JSONLOGIC_CHUNK_SIZE 256

typedef struct JsonLogic_StrBuf {
    size_t capacity;
    JsonLogic_String *string;
} JsonLogic_StrBuf;

#define JSONLOGIC_STRBUF_INIT ((JsonLogic_StrBuf){ .capacity = 0, .string = NULL })

JSONLOGIC_PRIVATE JsonLogic_Error jsonlogic_strbuf_ensure(JsonLogic_StrBuf *buf, size_t want_free_size);
JSONLOGIC_PRIVATE JsonLogic_Error jsonlogic_strbuf_append_latin1 (JsonLogic_StrBuf *buf, const char *str);
JSONLOGIC_PRIVATE JsonLogic_Error jsonlogic_strbuf_append_utf16 (JsonLogic_StrBuf *buf, const char16_t *str, size_t size);
JSONLOGIC_PRIVATE JsonLogic_Error jsonlogic_strbuf_append_double(JsonLogic_StrBuf *buf, double value);
JSONLOGIC_PRIVATE JsonLogic_Error jsonlogic_strbuf_append(JsonLogic_StrBuf *buf, JsonLogic_Handle handle);
JSONLOGIC_PRIVATE JsonLogic_String *jsonlogic_strbuf_take(JsonLogic_StrBuf *buf);
JSONLOGIC_PRIVATE void jsonlogic_strbuf_free(JsonLogic_StrBuf *buf);

JSONLOGIC_PRIVATE inline JsonLogic_Error jsonlogic_strbuf_append_ascii(JsonLogic_StrBuf *buf, const char *str) {
    return jsonlogic_strbuf_append_latin1(buf, str);
}

typedef struct JsonLogic_Utf8Buf {
    size_t capacity;
    size_t used;
    char *string;
} JsonLogic_Utf8Buf;

#define JSONLOGIC_UTF8BUF_INIT ((JsonLogic_Utf8Buf){ .capacity = 0, .used = 0, .string = NULL })

JSONLOGIC_PRIVATE JsonLogic_Error jsonlogic_utf8buf_ensure(JsonLogic_Utf8Buf *buf, size_t want_free_size);
JSONLOGIC_PRIVATE JsonLogic_Error jsonlogic_utf8buf_append_utf8  (JsonLogic_Utf8Buf *buf, const char *str);
JSONLOGIC_PRIVATE JsonLogic_Error jsonlogic_utf8buf_append_utf16 (JsonLogic_Utf8Buf *buf, const char16_t *str, size_t size);
JSONLOGIC_PRIVATE JsonLogic_Error jsonlogic_utf8buf_append_double(JsonLogic_Utf8Buf *buf, double value);
JSONLOGIC_PRIVATE char *jsonlogic_utf8buf_take(JsonLogic_Utf8Buf *buf);
JSONLOGIC_PRIVATE void jsonlogic_utf8buf_free(JsonLogic_Utf8Buf *buf);

JSONLOGIC_PRIVATE inline JsonLogic_Error jsonlogic_utf8buf_append_ascii(JsonLogic_Utf8Buf *buf, const char *str) {
    return jsonlogic_utf8buf_append_utf8(buf, str);
}

typedef struct JsonLogic_ArrayBuf {
    size_t capacity;
    JsonLogic_Array *array;
} JsonLogic_ArrayBuf;

#define JSONLOGIC_ARRAYBUF_INIT ((JsonLogic_ArrayBuf){ .capacity = 0, .array = NULL })

JSONLOGIC_PRIVATE JsonLogic_Error jsonlogic_arraybuf_append(JsonLogic_ArrayBuf *buf, JsonLogic_Handle handle);
JSONLOGIC_PRIVATE void            jsonlogic_arraybuf_clear(JsonLogic_ArrayBuf *buf);
JSONLOGIC_PRIVATE JsonLogic_Array *jsonlogic_arraybuf_take(JsonLogic_ArrayBuf *buf);
JSONLOGIC_PRIVATE void jsonlogic_arraybuf_free(JsonLogic_ArrayBuf *buf);

typedef struct JsonLogic_ObjBuf {
    JsonLogic_Object *object;
} JsonLogic_ObjBuf;

#define JSONLOGIC_OBJBUF_INIT ((JsonLogic_ObjBuf){ .object = NULL })

JSONLOGIC_PRIVATE JsonLogic_Error jsonlogic_objbuf_set(JsonLogic_ObjBuf *buf, JsonLogic_Handle key, JsonLogic_Handle value);
JSONLOGIC_PRIVATE JsonLogic_Object *jsonlogic_objbuf_take(JsonLogic_ObjBuf *buf);
JSONLOGIC_PRIVATE void jsonlogic_objbuf_free(JsonLogic_ObjBuf *buf);

JSONLOGIC_PRIVATE const char16_t *jsonlogic_find_char(const char16_t *str, size_t size, char16_t ch);

JSONLOGIC_PRIVATE const JsonLogic_Operation *jsonlogic_operations_get_with_hash(const JsonLogic_Operations *operations, uint64_t hash, const char16_t *key, size_t key_size);
JSONLOGIC_PRIVATE JsonLogic_Error jsonlogic_operations_set_with_hash(JsonLogic_Operations *operations, uint64_t hash, const char16_t *key, size_t key_size, void *context, JsonLogic_Operation_Funct funct);
#ifndef NDEBUG
JSONLOGIC_PRIVATE void jsonlogic_operations_debug(const JsonLogic_Operations *operations);
#endif

JSONLOGIC_PRIVATE uint64_t jsonlogic_hash_fnv1a(const uint8_t *data, size_t size);
JSONLOGIC_PRIVATE uint64_t jsonlogic_hash_fnv1a_utf16(const char16_t *str, size_t size);

JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_op_NOT         (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_op_TO_BOOL     (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_op_NE          (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_op_STRICT_NE   (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_op_MOD         (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_op_MUL         (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_op_ADD         (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_op_SUB         (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_op_DIV         (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_op_LT          (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_op_LE          (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_op_EQ          (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_op_STRICT_EQ   (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_op_GT          (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_op_GE          (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_op_CAT         (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_op_IN          (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_op_LOG         (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_op_MAX         (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_op_MERGE       (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_op_MIN         (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_op_MISSING     (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_op_MISSING_SOME(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_op_SUBSTR      (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_op_VAR         (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);

JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_extra_ADD_YEARS        (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_extra_AFTER            (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_extra_BEFORE           (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_extra_COMBINATIONS     (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_extra_DAYS             (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_extra_EXTRACT_FROM_UVCI(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_extra_FORMAT_TIME      (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_extra_HOURS            (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_extra_NOT_AFTER        (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_extra_NOT_BEFORE       (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_extra_NOW              (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_extra_PARSE_TIME       (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_extra_PLUS_TIME        (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_extra_TIME_SINCE       (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_extra_TO_ARRAY         (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
JSONLOGIC_PRIVATE JsonLogic_Handle jsonlogic_extra_ZIP              (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);

JSONLOGIC_PRIVATE JsonLogic_Handle certlogic_op_NOT    (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
JSONLOGIC_PRIVATE JsonLogic_Handle certlogic_op_TO_BOOL(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);

#define TRY(EXPR) { \
        const JsonLogic_Error json_logic_error__ = (EXPR); \
        if (json_logic_error__ != JSONLOGIC_ERROR_SUCCESS) { \
            return json_logic_error__; \
        } \
    }

#ifdef __cplusplus
}
#endif

#endif

#ifndef JSONLOGIC_INTERN_H
#define JSONLOGIC_INTERN_H
#pragma once

#include "jsonlogic_defs.h"

#define JsonLogic_PtrMask  (~(uintptr_t)0xffff000000000000)
#define JsonLogic_TypeMask  ((uintptr_t)0xffff000000000000)
#define JsonLogic_MaxNumber ((uintptr_t)0xfff8000000000000)

#if defined(NDEBUG)
    #define JSONLOGIC_DEBUG(...) 0
    #define JSONLOGIC_ERROR(...)
    #define JSONLOGIC_ASSERT(...)
#else
    #include <stdio.h>

    #define JSONLOGIC_DEBUG(FMT, ...) \
        fprintf(stderr, "*** error: %s:%u: in %s: " FMT "\n", \
            __FILE__, __LINE__, __func__, __VA_ARGS__)

    #if defined(JSONLOGIC_NO_ABORT_ON_ERROR)
        #define JSONLOGIC_ERROR(...) JSONLOGIC_DEBUG(__VA_ARGS__)
    #else
        #include <stdlib.h>

        #define JSONLOGIC_ERROR(...) \
            JSONLOGIC_DEBUG(__VA_ARGS__); \
            abort();
    #endif

    #define JSONLOGIC_ASSERT(EXPR, ...) \
        if (!(EXPR)) { JSONLOGIC_ERROR(__VA_ARGS__); }

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

#define JSONLOGIC_ERROR_SUCCESS            (JsonLogic_Type_Error | 0)
#define JSONLOGIC_ERROR_OUT_OF_MEMORY      (JsonLogic_Type_Error | 1)
#define JSONLOGIC_ERROR_ILLEGAL_OPERATION  (JsonLogic_Type_Error | 2)
#define JSONLOGIC_ERROR_ILLEGAL_ARGUMENT   (JsonLogic_Type_Error | 3)
#define JSONLOGIC_ERROR_INTERNAL_ERROR     (JsonLogic_Type_Error | 4)

#define JSONLOGIC_CAST_STRING(handle) ((JsonLogic_String*)((handle).intptr & JsonLogic_PtrMask))
#define JSONLOGIC_CAST_ARRAY( handle) ((JsonLogic_Array*) ((handle).intptr & JsonLogic_PtrMask))
#define JSONLOGIC_CAST_OBJECT(handle) ((JsonLogic_Object*)((handle).intptr & JsonLogic_PtrMask))

#define JSONLOGIC_IS_STRING(handle)  (((handle).intptr & JsonLogic_TypeMask) == JsonLogic_Type_String)
#define JSONLOGIC_IS_OBJECT(handle)  (((handle).intptr & JsonLogic_TypeMask) == JsonLogic_Type_Object)
#define JSONLOGIC_IS_ARRAY(handle)   (((handle).intptr & JsonLogic_TypeMask) == JsonLogic_Type_Array)
#define JSONLOGIC_IS_BOOLEAN(handle) (((handle).intptr & JsonLogic_TypeMask) == JsonLogic_Type_Boolean)
#define JSONLOGIC_IS_NULL(handle)    ((handle).intptr == JsonLogic_Type_Null)
#define JSONLOGIC_IS_NUMBER(handle)  ((handle).intptr < JsonLogic_MaxNumber)
#define JSONLOGIC_IS_TRUE(handle)    ((handle).intptr == (JsonLogic_Type_Boolean | 1))
#define JSONLOGIC_IS_FALSE(handle)   ((handle).intptr == (JsonLogic_Type_Boolean | 0))

#define JSONLOGIC_IS_ERROR_(handle)   (((handle).intptr & JsonLogic_TypeMask) == JsonLogic_Type_Error)

#ifndef NDEBUG
    #define JSONLOGIC_IS_ERROR(handle) JSONLOGIC_IS_ERROR_(handle)
#else
    #define JSONLOGIC_IS_ERROR(handle) \
        (JSONLOGIC_IS_ERROR_(handle) && \
         JSONLOGIC_DEBUG("trace: %s", jsonlogic_get_error_message(jsonlogic_get_error(handle))), \
         JSONLOGIC_IS_ERROR_(handle))
#endif

#define JSONLOGIC_DECL_UTF16(NAME) \
    JSONLOGIC_PRIVATE extern const size_t NAME##_SIZE; \
    JSONLOGIC_PRIVATE extern const JsonLogic_Char NAME[];

#define JSONLOGIC_DEF_UTF16(NAME, ...) \
    const size_t NAME##_SIZE = sizeof((char[]){__VA_ARGS__}); \
    const JsonLogic_Char NAME[sizeof((char[]){__VA_ARGS__})] = { __VA_ARGS__ };

JSONLOGIC_DECL_UTF16(JSONLOGIC_NULL_STRING)
JSONLOGIC_DECL_UTF16(JSONLOGIC_FALSE_STRING)
JSONLOGIC_DECL_UTF16(JSONLOGIC_TRUE_STRING)
JSONLOGIC_DECL_UTF16(JSONLOGIC_NAN_STRING)
JSONLOGIC_DECL_UTF16(JSONLOGIC_INFINITY_STRING)
JSONLOGIC_DECL_UTF16(JSONLOGIC_POS_INFINITY_STRING)
JSONLOGIC_DECL_UTF16(JSONLOGIC_NEG_INFINITY_STRING)

typedef struct JsonLogic_String {
    size_t refcount;
    size_t size;
    JsonLogic_Char str[1];
} JsonLogic_String;

typedef struct JsonLogic_Array {
    size_t refcount;
    size_t size;
    JsonLogic_Handle items[1];
} JsonLogic_Array;

typedef struct JsonLogic_Object {
    size_t refcount;
    size_t size;
    JsonLogic_Object_Entry entries[1];
} JsonLogic_Object;

#ifdef __cplusplus
}
#endif

#include "jsonlogic.h"

#ifdef __cplusplus
extern "C" {
#endif

JSONLOGIC_PRIVATE JsonLogic_Array *jsonlogic_array_with_capacity(size_t size);

JSONLOGIC_PRIVATE size_t jsonlogic_object_get_index_utf16(JsonLogic_Object *object, const JsonLogic_Char *key, size_t key_size);
JSONLOGIC_PRIVATE size_t jsonlogic_object_get_index(JsonLogic_Object *object, JsonLogic_Handle key);

JSONLOGIC_PRIVATE void jsonlogic_string_free(JsonLogic_String *string);
JSONLOGIC_PRIVATE void jsonlogic_array_free (JsonLogic_Array  *array);
JSONLOGIC_PRIVATE void jsonlogic_object_free(JsonLogic_Object *object);

JSONLOGIC_PRIVATE inline JsonLogic_Handle jsonlogic_string_into_handle(JsonLogic_String *string) {
    if (string == NULL) {
        return JsonLogic_Null;
    }
    return (JsonLogic_Handle){ .intptr = ((uintptr_t)string) | JsonLogic_Type_String };
}

JSONLOGIC_PRIVATE inline JsonLogic_Handle jsonlogic_array_into_handle(JsonLogic_Array *array) {
    if (array == NULL) {
        return JsonLogic_Null;
    }
    return (JsonLogic_Handle){ .intptr = ((uintptr_t)array) | JsonLogic_Type_Array };
}

JSONLOGIC_PRIVATE inline JsonLogic_Handle jsonlogic_object_into_handle(JsonLogic_Object *object) {
    if (object == NULL) {
        return JsonLogic_Null;
    }
    return (JsonLogic_Handle){ .intptr = ((uintptr_t)object) | JsonLogic_Type_Object };
}

typedef struct JsonLogic_StrBuf {
    size_t size;
    JsonLogic_String *data;
} JsonLogic_StrBuf;

#define JSONLOGIC_BUFFER_INIT ((JsonLogic_StrBuf){ .size = 0, .data = NULL })
#define JSONLOGIC_CHUNK_SIZE 256

JSONLOGIC_PRIVATE bool jsonlogic_string_equals (const JsonLogic_String *a, const JsonLogic_String *b);
JSONLOGIC_PRIVATE int  jsonlogic_string_compare(const JsonLogic_String *a, const JsonLogic_String *b);

JSONLOGIC_PRIVATE size_t jsonlogic_string_to_index(const JsonLogic_String *string);

JSONLOGIC_PRIVATE JsonLogic_Array *jsonlogic_array_truncate(JsonLogic_Array *array, size_t size);

JSONLOGIC_PRIVATE JsonLogic_Error jsonlogic_strbuf_ensure(JsonLogic_StrBuf *buf, size_t want_free_size);
JSONLOGIC_PRIVATE JsonLogic_Error jsonlogic_strbuf_append_latin1(JsonLogic_StrBuf *buf, const char *str);
JSONLOGIC_PRIVATE JsonLogic_Error jsonlogic_strbuf_append_utf16 (JsonLogic_StrBuf *buf, const JsonLogic_Char *str, size_t size);
JSONLOGIC_PRIVATE JsonLogic_Error jsonlogic_strbuf_append_double(JsonLogic_StrBuf *buf, double value);
JSONLOGIC_PRIVATE JsonLogic_Error jsonlogic_strbuf_append(JsonLogic_StrBuf *buf, JsonLogic_Handle handle);
JSONLOGIC_PRIVATE JsonLogic_String *jsonlogic_strbuf_take(JsonLogic_StrBuf *buf);
JSONLOGIC_PRIVATE void jsonlogic_strbuf_free(JsonLogic_StrBuf *buf);

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

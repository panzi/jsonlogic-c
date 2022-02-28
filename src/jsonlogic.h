#ifndef JSONLOGIC_H
#define JSONLOGIC_H
#pragma once

#include "jsonlogic_defs.h"

#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct JsonLogic_LineInfo {
    size_t index;
    size_t lineno;
    size_t column;
} JsonLogic_LineInfo;

#define JSONLOGIC_LINEINFO_INIT (JsonLogic_LineInfo){ .index = 0, .lineno = 0, .column = 0 }

JSONLOGIC_EXPORT_CONST const JsonLogic_Handle JsonLogic_NaN;
JSONLOGIC_EXPORT_CONST const JsonLogic_Handle JsonLogic_Null;
JSONLOGIC_EXPORT_CONST const JsonLogic_Handle JsonLogic_True;
JSONLOGIC_EXPORT_CONST const JsonLogic_Handle JsonLogic_False;
JSONLOGIC_EXPORT_CONST const JsonLogic_Handle JsonLogic_Error_Success;
JSONLOGIC_EXPORT_CONST const JsonLogic_Handle JsonLogic_Error_OutOfMemory;
JSONLOGIC_EXPORT_CONST const JsonLogic_Handle JsonLogic_Error_IllegalOperation;
JSONLOGIC_EXPORT_CONST const JsonLogic_Handle JsonLogic_Error_IllegalArgument;
JSONLOGIC_EXPORT_CONST const JsonLogic_Handle JsonLogic_Error_InternalError;
JSONLOGIC_EXPORT_CONST const JsonLogic_Handle JsonLogic_Error_StopIteration;
JSONLOGIC_EXPORT_CONST const JsonLogic_Handle JsonLogic_Error_IOError;
JSONLOGIC_EXPORT_CONST const JsonLogic_Handle JsonLogic_Error_SyntaxError;
JSONLOGIC_EXPORT_CONST const JsonLogic_Handle JsonLogic_Error_UnicodeError;

JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_incref(JsonLogic_Handle handle);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_decref(JsonLogic_Handle handle);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_dissolve(JsonLogic_Handle handle);
JSONLOGIC_EXPORT size_t           jsonlogic_get_refcount(JsonLogic_Handle handle);

JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_parse(const char *str, JsonLogic_LineInfo *infoptr);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_parse_sized(const char *str, size_t size, JsonLogic_LineInfo *infoptr);

JSONLOGIC_EXPORT JsonLogic_LineInfo jsonlogic_get_lineinfo(const char *str, size_t size, size_t index);

JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_stringify(JsonLogic_Handle value);
JSONLOGIC_EXPORT char *jsonlogic_stringify_utf8(JsonLogic_Handle value);
JSONLOGIC_EXPORT JsonLogic_Error jsonlogic_stringify_file(FILE *file, JsonLogic_Handle value);

JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_string_from_latin1      (const char *str);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_string_from_latin1_sized(const char *str, size_t size);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_string_from_utf8        (const char *str);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_string_from_utf8_sized  (const char *str, size_t size);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_string_from_utf16       (const char16_t *str);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_string_from_utf16_sized (const char16_t *str, size_t size);

JSONLOGIC_EXPORT bool jsonlogic_utf16_equals( const char16_t *a, size_t asize, const char16_t *b, size_t bsize);
JSONLOGIC_EXPORT int  jsonlogic_utf16_compare(const char16_t *a, size_t asize, const char16_t *b, size_t bsize);

JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_number_from(double value);

JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_error_from(JsonLogic_Error error);

JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_empty_string(void);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_empty_array(void);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_empty_object(void);

typedef struct JsonLogic_Object_Utf16Entry {
    const char16_t *key;
    JsonLogic_Handle value;
} JsonLogic_Object_Utf16Entry;

JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_array_from (const JsonLogic_Handle items[], size_t size);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_array_from_and_decref(const JsonLogic_Handle items[], size_t size);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_object_from(const JsonLogic_Object_Entry entries[], size_t size);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_object_from_and_decref(const JsonLogic_Object_Entry entries[], size_t size);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_object_from_utf16(const JsonLogic_Object_Utf16Entry entries[], size_t size);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_object_from_utf16_and_decref(const JsonLogic_Object_Utf16Entry entries[], size_t size);

JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_array_from_vararg (size_t count, ...);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_object_from_vararg(size_t count, ...);

#define jsonlogic_count_values(...) (sizeof((JsonLogic_Handle[]){__VA_ARGS__} / sizeof(JsonLogic_Handle)))
#define jsonlogic_array_build(...) jsonlogic_array_from((JsonLogic_Handle[]){ __VA_ARGS__ }, jsonlogic_count_values(__VA_ARGS__))
#define jsonlogic_array_build_and_decref(...) jsonlogic_array_from_and_decref((JsonLogic_Handle[]){ __VA_ARGS__ }, jsonlogic_count_values(__VA_ARGS__))
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_to_array(JsonLogic_Handle handle);


#define jsonlogic_count_entries(...) (sizeof((JsonLogic_Object_Entry[]){__VA_ARGS__}) / sizeof(JsonLogic_Object_Entry))
#define jsonlogic_object_build(...) jsonlogic_object_from((JsonLogic_Object_Entry[]){ __VA_ARGS__ }, jsonlogic_count_entries(__VA_ARGS__))
#define jsonlogic_object_build_and_decref(...) jsonlogic_object_from_and_decref((JsonLogic_Object_Entry[]){ __VA_ARGS__ }, jsonlogic_count_entries(__VA_ARGS__))

#define jsonlogic_count_entries_utf16(...) (sizeof((JsonLogic_Object_Utf16Entry[]){__VA_ARGS__}) / sizeof(JsonLogic_Object_Utf16Entry))
#define jsonlogic_object_build_utf16(...) jsonlogic_object_from_utf16((JsonLogic_Object_Utf16Entry[]){ __VA_ARGS__ }, jsonlogic_count_entries_utf16(__VA_ARGS__))
#define jsonlogic_object_build_utf16_and_decref(...) jsonlogic_object_from_utf16_and_decref((JsonLogic_Object_Utf16Entry[]){ __VA_ARGS__ }, jsonlogic_count_entries_utf16(__VA_ARGS__))

JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_to_string (JsonLogic_Handle handle);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_to_number (JsonLogic_Handle handle);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_to_boolean(JsonLogic_Handle handle);

JSONLOGIC_EXPORT bool   jsonlogic_to_bool  (JsonLogic_Handle handle);
JSONLOGIC_EXPORT double jsonlogic_to_double(JsonLogic_Handle handle);

JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_substr(JsonLogic_Handle string, JsonLogic_Handle index, JsonLogic_Handle size);
JSONLOGIC_EXPORT const char16_t *jsonlogic_get_string_content(JsonLogic_Handle string, size_t *sizeptr);

JSONLOGIC_EXPORT size_t jsonlogic_utf16_len(const char16_t *key);
JSONLOGIC_EXPORT char *jsonlogic_utf16_to_utf8(const char16_t *str, size_t size);
JSONLOGIC_EXPORT JsonLogic_Error jsonlogic_print_utf16(FILE *stream, const char16_t *str, size_t size);
JSONLOGIC_EXPORT JsonLogic_Error jsonlogic_println_utf16(FILE *stream, const char16_t *str, size_t size);
JSONLOGIC_EXPORT JsonLogic_Error jsonlogic_print(FILE *stream, JsonLogic_Handle handle);
JSONLOGIC_EXPORT JsonLogic_Error jsonlogic_println(FILE *stream, JsonLogic_Handle handle);

JSONLOGIC_EXPORT JsonLogic_Type jsonlogic_get_type(JsonLogic_Handle handle);
JSONLOGIC_EXPORT const char    *jsonlogic_get_type_name(JsonLogic_Type type);

JSONLOGIC_EXPORT JsonLogic_Error jsonlogic_get_error(JsonLogic_Handle handle);
JSONLOGIC_EXPORT const char     *jsonlogic_get_error_message(JsonLogic_Error error);
JSONLOGIC_EXPORT const char     *jsonlogic_get_linestart(const char *str, size_t index);
JSONLOGIC_EXPORT void jsonlogic_print_parse_error(FILE *stream, const char *str, JsonLogic_Error error, JsonLogic_LineInfo info);
JSONLOGIC_EXPORT void jsonlogic_print_parse_error_sized(FILE *stream, const char *str, size_t size, JsonLogic_Error error, JsonLogic_LineInfo info);

JSONLOGIC_EXPORT bool jsonlogic_is_string (JsonLogic_Handle handle);
JSONLOGIC_EXPORT bool jsonlogic_is_error  (JsonLogic_Handle handle);
JSONLOGIC_EXPORT bool jsonlogic_is_null   (JsonLogic_Handle handle);
JSONLOGIC_EXPORT bool jsonlogic_is_number (JsonLogic_Handle handle);
JSONLOGIC_EXPORT bool jsonlogic_is_array  (JsonLogic_Handle handle);
JSONLOGIC_EXPORT bool jsonlogic_is_object (JsonLogic_Handle handle);
JSONLOGIC_EXPORT bool jsonlogic_is_boolean(JsonLogic_Handle handle);
JSONLOGIC_EXPORT bool jsonlogic_is_true   (JsonLogic_Handle handle);
JSONLOGIC_EXPORT bool jsonlogic_is_false  (JsonLogic_Handle handle);

JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_add(JsonLogic_Handle a, JsonLogic_Handle b);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_sub(JsonLogic_Handle a, JsonLogic_Handle b);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_mul(JsonLogic_Handle a, JsonLogic_Handle b);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_div(JsonLogic_Handle a, JsonLogic_Handle b);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_mod(JsonLogic_Handle a, JsonLogic_Handle b);

JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_negative(JsonLogic_Handle value);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_not(JsonLogic_Handle value);

JSONLOGIC_EXPORT int jsonlogic_compare(JsonLogic_Handle a, JsonLogic_Handle b);

JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_lt(JsonLogic_Handle a, JsonLogic_Handle b);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_gt(JsonLogic_Handle a, JsonLogic_Handle b);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_le(JsonLogic_Handle a, JsonLogic_Handle b);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_ge(JsonLogic_Handle a, JsonLogic_Handle b);

JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_equal    (JsonLogic_Handle a, JsonLogic_Handle b);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_not_equal(JsonLogic_Handle a, JsonLogic_Handle b);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_strict_equal    (JsonLogic_Handle a, JsonLogic_Handle b);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_strict_not_equal(JsonLogic_Handle a, JsonLogic_Handle b);

/**
 * @brief Check for deep equality.
 * @warning Has no cycle detection!
 * 
 * @param a 
 * @param b 
 * @return 
 */
JSONLOGIC_EXPORT bool jsonlogic_deep_strict_equal(JsonLogic_Handle a, JsonLogic_Handle b);

JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_includes(JsonLogic_Handle list, JsonLogic_Handle item);

JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_get(JsonLogic_Handle object, JsonLogic_Handle key);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_get_index(JsonLogic_Handle handle, size_t index);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_get_utf16_sized(JsonLogic_Handle object, const char16_t *key, size_t size);

JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_apply(JsonLogic_Handle logic, JsonLogic_Handle input);

typedef JsonLogic_Handle (*JsonLogic_Operation_Funct)(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);

typedef struct JsonLogic_Operation {
    void *context;
    JsonLogic_Operation_Funct funct;
} JsonLogic_Operation;

typedef struct JsonLogic_Operation_Entry {
    uint64_t hash;
    const char16_t *key;
    size_t key_size;
    JsonLogic_Operation operation;
} JsonLogic_Operation_Entry;

typedef struct JsonLogic_Operations {
    size_t capacity;
    size_t used;
    JsonLogic_Operation_Entry *entries;
} JsonLogic_Operations;

JSONLOGIC_EXPORT const JsonLogic_Operation *jsonlogic_operations_get_sized(const JsonLogic_Operations *operations, const char16_t *key, size_t key_size);
JSONLOGIC_EXPORT JsonLogic_Error jsonlogic_operations_set_sized(JsonLogic_Operations *operations, const char16_t *key, size_t key_size, void *context, JsonLogic_Operation_Funct funct);

JSONLOGIC_EXPORT_CONST const JsonLogic_Operations JsonLogic_Builtins;
JSONLOGIC_EXPORT_CONST const JsonLogic_Operations CertLogic_Builtins;

typedef struct JsonLogic_Operations_BuildEntry {
    const char16_t *key;
    JsonLogic_Operation operation;
} JsonLogic_Operations_BuildEntry;

JSONLOGIC_EXPORT JsonLogic_Error jsonlogic_operations_build(JsonLogic_Operations *operations, const JsonLogic_Operations_BuildEntry *build_operations);
JSONLOGIC_EXPORT JsonLogic_Error jsonlogic_operations_extend(JsonLogic_Operations *operations, const JsonLogic_Operations *more_operations);
JSONLOGIC_EXPORT void jsonlogic_operations_free(JsonLogic_Operations *operations);

#define JSONLOGIC_OPERATIONS_INIT { .capacity = 0, .used = 0, .entries = NULL }

typedef struct JsonLogic_Iterator {
    JsonLogic_Handle handle;
    size_t index;
} JsonLogic_Iterator;

#define JSONLOGIC_ITERATOR_INIT { .handle = JsonLogic_Null, .index = 0 }

// can't get JSONLOGIC_EXPORT inline to work, so I its marcos
#define jsonlogic_get_utf16(object, key) jsonlogic_get_utf16_sized((object), (key), jsonlogic_utf16_len((key)))
#define jsonlogic_boolean_from(value) ((value) ? JsonLogic_True : JsonLogic_False)
#define jsonlogic_iter(HANDLE) (JsonLogic_Iterator) { \
        .handle = jsonlogic_incref((HANDLE)), \
        .index  = 0, \
    }
#define jsonlogic_operations_get(operations, key) \
    jsonlogic_operations_get_sized((operations), (key), jsonlogic_utf16_len((key)))

#define jsonlogic_operations_set(operations, key, context, funct) \
    jsonlogic_operations_set_sized((operations), (key), jsonlogic_utf16_len((key)), (context), (funct))

JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_iter_next(JsonLogic_Iterator *iter);
JSONLOGIC_EXPORT void             jsonlogic_iter_free(JsonLogic_Iterator *iter);

/**
 * @brief 
 * 
 * @param logic 
 * @param input 
 * @param operations need to be sorted because binaray search is performed on it
 * @param operation_count 
 * @return JSONLOGIC_EXPORT 
 */
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_apply_custom(
    JsonLogic_Handle logic,
    JsonLogic_Handle input,
    const JsonLogic_Operations *operations
);

JSONLOGIC_EXPORT JsonLogic_Handle certlogic_to_boolean(JsonLogic_Handle handle);
JSONLOGIC_EXPORT bool             certlogic_to_bool   (JsonLogic_Handle handle);
JSONLOGIC_EXPORT JsonLogic_Handle certlogic_not       (JsonLogic_Handle value);

JSONLOGIC_EXPORT JsonLogic_Handle certlogic_apply(JsonLogic_Handle logic, JsonLogic_Handle input);

JSONLOGIC_EXPORT JsonLogic_Handle certlogic_apply_custom(
    JsonLogic_Handle logic,
    JsonLogic_Handle input,
    const JsonLogic_Operations *operations
);

#ifdef __cplusplus
}
#endif

#endif

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

JSONLOGIC_EXPORT extern const JsonLogic_Handle JsonLogic_NaN;
JSONLOGIC_EXPORT extern const JsonLogic_Handle JsonLogic_Null;
JSONLOGIC_EXPORT extern const JsonLogic_Handle JsonLogic_True;
JSONLOGIC_EXPORT extern const JsonLogic_Handle JsonLogic_False;
JSONLOGIC_EXPORT extern const JsonLogic_Handle JsonLogic_Error_Success;
JSONLOGIC_EXPORT extern const JsonLogic_Handle JsonLogic_Error_OutOfMemory;
JSONLOGIC_EXPORT extern const JsonLogic_Handle JsonLogic_Error_IllegalOperation;
JSONLOGIC_EXPORT extern const JsonLogic_Handle JsonLogic_Error_IllegalArgument;
JSONLOGIC_EXPORT extern const JsonLogic_Handle JsonLogic_Error_InternalError;
JSONLOGIC_EXPORT extern const JsonLogic_Handle JsonLogic_Error_StopIteration;
JSONLOGIC_EXPORT extern const JsonLogic_Handle JsonLogic_Error_IOError;
JSONLOGIC_EXPORT extern const JsonLogic_Handle JsonLogic_Error_SyntaxError;

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

JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_empty_string();
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_empty_array();
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_empty_object();

JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_array_from_vararg (size_t count, ...);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_object_from_vararg(size_t count, ...);

#define jsonlogic_count_values(...) (sizeof((JsonLogic_Handle[]){__VA_ARGS__} / sizeof(JsonLogic_Handle)))
#define jsonlogic_array_from(...) jsonlogic_array_from_vararg(jsonlogic_count_values(__VA_ARGS__), __VA_ARGS__)


#define jsonlogic_count_entries(...) (sizeof((JsonLogic_Object_Entry[]){__VA_ARGS__}) / sizeof(JsonLogic_Object_Entry))
/**
 * @code
 * JsonLogic_Handle object = jsonlogic_object_from(
 *      jsonlogic_entry_latin1("foo", jsonlogic_number_from(1.2)),
 *      jsonlogic_entry_latin1("bar", jsonlogic_string_from_latin1("blubb")),
 *      jsonlogic_entry(key_handle, value_handle)
 * );
 * @endcode
 * 
 */
#define jsonlogic_object_from(...) jsonlogic_object_from_vararg(jsonlogic_count_entries(__VA_ARGS__), __VA_ARGS__)
#define jsonlogic_entry(KEY, VALUE) (JsonLogic_Object_Entry){ .key = (KEY), .value = (VALUE) }

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

JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_add(JsonLogic_Handle a, JsonLogic_Handle b);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_sub(JsonLogic_Handle a, JsonLogic_Handle b);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_mul(JsonLogic_Handle a, JsonLogic_Handle b);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_div(JsonLogic_Handle a, JsonLogic_Handle b);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_mod(JsonLogic_Handle a, JsonLogic_Handle b);

JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_negative(JsonLogic_Handle value);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_not(JsonLogic_Handle value);

JSONLOGIC_EXPORT int jsonlogic_comapre(JsonLogic_Handle a, JsonLogic_Handle b);

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

typedef JsonLogic_Handle (*JsonLogic_Operation)(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);

typedef struct JsonLogic_Operation_Entry {
    const char16_t *key;
    size_t key_size;
    JsonLogic_Operation operation;
} JsonLogic_Operation_Entry;

#define jsonlogic_operation(KEY, OPERATION) { \
        .key = KEY, \
        .key_size = sizeof(KEY) / sizeof(char16_t) - 1, \
        .operation = OPERATION, \
    }

#define jsonlogic_operations_size(OPERATIONS) \
    (sizeof(OPERATIONS) / sizeof(JsonLogic_Operation_Entry))

JSONLOGIC_EXPORT void jsonlogic_operations_sort(JsonLogic_Operation_Entry operations[], size_t count);
JSONLOGIC_EXPORT JsonLogic_Operation jsonlogic_operation_get(const JsonLogic_Operation_Entry operations[], size_t count, const char16_t *key, size_t key_size);

typedef struct JsonLogic_Iterator {
    JsonLogic_Handle handle;
    size_t index;
} JsonLogic_Iterator;

#if defined(JSONLOGIC_INLINE_SUPPORTED)
JSONLOGIC_EXPORT inline JsonLogic_Handle jsonlogic_get_utf16(JsonLogic_Handle object, const char16_t *key) {
    return jsonlogic_get_utf16_sized(object, key, jsonlogic_utf16_len(key));
}

JSONLOGIC_EXPORT inline JsonLogic_Handle jsonlogic_boolean_from(bool value) {
    return value ? JsonLogic_True : JsonLogic_False;
}

JSONLOGIC_EXPORT inline JsonLogic_Iterator jsonlogic_iter(JsonLogic_Handle handle) {
    return (JsonLogic_Iterator){
        .handle = jsonlogic_incref(handle),
        .index  = 0,
    };
}
#else

JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_get_utf16(JsonLogic_Handle object, const char16_t *key);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_boolean_from(bool value);
JSONLOGIC_EXPORT JsonLogic_Iterator jsonlogic_iter(JsonLogic_Handle handle);

#endif

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
    void *context,
    const JsonLogic_Operation_Entry operations[],
    size_t operation_count
);

#ifdef __cplusplus
}
#endif

#endif

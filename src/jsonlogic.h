#ifndef JSONLOGIC_H
#define JSONLOGIC_H
#pragma once

#include "jsonlogic_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

JSONLOGIC_EXPORT extern const JsonLogic_Handle JsonLogic_NaN;
JSONLOGIC_EXPORT extern const JsonLogic_Handle JsonLogic_Null;
JSONLOGIC_EXPORT extern const JsonLogic_Handle JsonLogic_True;
JSONLOGIC_EXPORT extern const JsonLogic_Handle JsonLogic_False;

JSONLOGIC_EXPORT void jsonlogic_incref(JsonLogic_Handle handle);
JSONLOGIC_EXPORT void jsonlogic_decref(JsonLogic_Handle handle);

JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_parse(const char *str);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_parse_sized(const char *str, size_t size);

JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_stringify(JsonLogic_Handle value);

JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_string_from_latin1      (const char *str);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_string_from_latin1_sized(const char *str, size_t size);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_string_from_utf8        (const char *str);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_string_from_utf8_sized  (const char *str, size_t size);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_string_from_utf16       (const JsonLogic_Char *str);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_string_from_utf16_sized (const JsonLogic_Char *str, size_t size);

JSONLOGIC_EXPORT bool jsonlogic_utf16_equals( const JsonLogic_Char *a, size_t asize, const JsonLogic_Char *b, size_t bsize);
JSONLOGIC_EXPORT int  jsonlogic_utf16_compare(const JsonLogic_Char *a, size_t asize, const JsonLogic_Char *b, size_t bsize);

JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_number_from(double value);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_boolean_from(bool value);

JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_empty_array();
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_empty_object();

JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_array_from_vararg (size_t count, ...);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_object_from_vararg(size_t count, ...);

#define jsonlogic_count_values(...) (sizeof((JsonLogic_Handle[]){__VA_ARGS__} / sizeof(JsonLogic_Handle)))
#define jsonlogic_array_from(...) jsonlogic_array_from_vararg(jsonlogic_count_values(__VA_ARGS__), __VA_ARGS__)


#define jsonlogic_count_entries(...) (sizeof((JsonLogic_Object_Entry[]){__VA_ARGS__} / sizeof(JsonLogic_Object_Entry)))
/**
 * @code
 * JsonLogic_Handle object = jsonlogic_object_from(
 *      jsonlogic_entry_latin1("foo", jsonlogic_number_from(1.2)),
 *      jsonlogic_entry_latin1("bar", jsonlogic_string_from_latin1("blubb")),
 *      jsonlogic_entry(key_handle, value_handle)
 * );
 * @endcode
 * 
 * 
 */
#define jsonlogic_object_from(...) jsonlogic_array_from_vararg(jsonlogic_count_entries(__VA_ARGS__), __VA_ARGS__)
#define jsonlogic_entry(KEY, VALUE) (JsonLogic_Object_Entry){ .key = (KEY), .value = (VALUE) }
#define jsonlogic_entry_latin1(KEY, VALUE) (JsonLogic_Object_Entry){ .key = jsonlogic_string_from_latin1((KEY)), .value = (VALUE) }

JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_to_string (JsonLogic_Handle handle);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_to_number (JsonLogic_Handle handle);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_to_boolean(JsonLogic_Handle handle);

JSONLOGIC_EXPORT bool   jsonlogic_to_bool  (JsonLogic_Handle handle);
JSONLOGIC_EXPORT double jsonlogic_to_double(JsonLogic_Handle handle);

JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_substr(JsonLogic_Handle string, JsonLogic_Handle index, JsonLogic_Handle size);
JSONLOGIC_EXPORT const JsonLogic_Char *jsonlogic_get_utf16(JsonLogic_Handle string, size_t *sizeptr);

JSONLOGIC_EXPORT char *jsonlogic_utf16_to_utf8(const JsonLogic_Char *str, size_t size);

JSONLOGIC_EXPORT JsonLogic_Type jsonlogic_typeof(JsonLogic_Handle handle);

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

JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_get_item(JsonLogic_Handle object, JsonLogic_Handle key);

JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_apply(JsonLogic_Handle logic, JsonLogic_Handle input);

typedef JsonLogic_Handle (*JsonLogic_Operation)(JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);

typedef struct JsonLogic_Operation_Entry {
    JsonLogic_Handle key;
    JsonLogic_Operation operation;
} JsonLogic_Operation_Entry;

JSONLOGIC_EXPORT void jsonlogic_operations_sort(JsonLogic_Operation_Entry operations[], size_t count);
JSONLOGIC_EXPORT JsonLogic_Operation jsonlogic_operation_get(const JsonLogic_Operation_Entry operations[], size_t count, const JsonLogic_Char *key, size_t key_size);

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
    const JsonLogic_Operation_Entry operations[],
    size_t operation_count
);

#ifdef __cplusplus
}
#endif

#endif

#ifndef JSONLOGIC_H
#define JSONLOGIC_H
#pragma once

#include "jsonlogic_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef JSONLOGIC_INTERN_H
typedef struct JsonLogic_Struct* JsonLogic_Handle;
#endif

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

JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_number_from(double value);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_boolean_from(bool value);

JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_empty_array();
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_empty_object();

JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_array_from_vararg (size_t count, ...);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_object_from_vararg(size_t count, ...);

#define jsonlogic_numargs(...) (sizeof((JsonLogic_Handle[]){__VA_ARGS__} / sizeof(JsonLogic_Handle)))
#define jsonlogic_array_from(...)  jsonlogic_array_from_vararg(jsonlogic_numargs(__VA_ARGS__),     __VA_ARGS__)
#define jsonlogic_object_from(...) jsonlogic_array_from_vararg(jsonlogic_numargs(__VA_ARGS__) / 2, __VA_ARGS__)

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

#ifdef __cplusplus
}
#endif

#endif

#include "jsonlogic_intern.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <errno.h>
#include <string.h>

JSONLOGIC_DEF_UTF16(JSONLOGIC_ACCUMULATOR, u"accumulator")
JSONLOGIC_DEF_UTF16(JSONLOGIC_CURRENT,     u"current")
JSONLOGIC_DEF_UTF16(JSONLOGIC_DATA,        u"data")

const JsonLogic_Handle JsonLogic_Null = JsonLogic_Type_Null;

JSONLOGIC_LOCALE_T JsonLogic_C_Locale = NULL;

JsonLogic_Type jsonlogic_get_type(JsonLogic_Handle handle) {
    static_assert(sizeof(void*) <= 8, "This library only works on platforms where pointers use <= 48 bits (e.g. x86_64).");

    if (JSONLOGIC_IS_NUMBER(handle)) {
        return JsonLogic_Type_Number;
    }

    return handle & JsonLogic_TypeMask;
}

const char *jsonlogic_get_type_name(JsonLogic_Type type) {
    switch (type) {
        case JsonLogic_Type_Array:
            return "array";

        case JsonLogic_Type_Boolean:
            return "boolean";

        case JsonLogic_Type_Error:
            return "error";

        case JsonLogic_Type_Null:
            return "null";

        case JsonLogic_Type_Number:
            return "number";

        case JsonLogic_Type_Object:
            return "object";

        case JsonLogic_Type_String:
            return "string";

        default:
            return "(illegal handle)";
    }
}

bool jsonlogic_is_error(JsonLogic_Handle handle) {
    return JSONLOGIC_IS_ERROR(handle);
}

bool jsonlogic_is_null(JsonLogic_Handle handle) {
    return JSONLOGIC_IS_NULL(handle);
}

bool jsonlogic_is_number(JsonLogic_Handle handle) {
    return JSONLOGIC_IS_NUMBER(handle);
}

bool jsonlogic_is_array(JsonLogic_Handle handle) {
    return JSONLOGIC_IS_ARRAY(handle);
}

bool jsonlogic_is_object(JsonLogic_Handle handle) {
    return JSONLOGIC_IS_OBJECT(handle);
}

bool jsonlogic_is_boolean(JsonLogic_Handle handle) {
    return JSONLOGIC_IS_BOOLEAN(handle);
}

bool jsonlogic_is_string(JsonLogic_Handle handle) {
    return JSONLOGIC_IS_STRING(handle);
}

bool jsonlogic_is_true(JsonLogic_Handle handle) {
    return handle == JSONLOGIC_TURE;
}

bool jsonlogic_is_false(JsonLogic_Handle handle) {
    return handle == JSONLOGIC_FALSE;
}

JsonLogic_Handle jsonlogic_incref(JsonLogic_Handle handle) {
    switch (handle & JsonLogic_TypeMask) {
        case JsonLogic_Type_String:
            assert(JSONLOGIC_CAST_STRING(handle)->refcount < SIZE_MAX);
            JSONLOGIC_CAST_STRING(handle)->refcount ++;
            break;

        case JsonLogic_Type_Array:
            assert(JSONLOGIC_CAST_ARRAY(handle)->refcount < SIZE_MAX);
            JSONLOGIC_CAST_ARRAY(handle)->refcount ++;
            break;

        case JsonLogic_Type_Object:
            assert(JSONLOGIC_CAST_OBJECT(handle)->refcount < SIZE_MAX);
            JSONLOGIC_CAST_OBJECT(handle)->refcount ++;
            break;
    }
    return handle;
}

JsonLogic_Handle jsonlogic_decref(JsonLogic_Handle handle) {
    switch (handle & JsonLogic_TypeMask) {
        case JsonLogic_Type_String:
        {
            JsonLogic_String *string = JSONLOGIC_CAST_STRING(handle);
            assert(string->refcount > 0);
            string->refcount --;
            if (string->refcount == 0) {
                jsonlogic_string_free(string);
                return JsonLogic_Null;
            }
            break;
        }
        case JsonLogic_Type_Array:
        {
            JsonLogic_Array *array = JSONLOGIC_CAST_ARRAY(handle);
            assert(array->refcount > 0);
            array->refcount --;
            if (array->refcount == 0) {
                jsonlogic_array_free(array);
                return JsonLogic_Null;
            }
            break;
        }
        case JsonLogic_Type_Object:
        {
            JsonLogic_Object *object = JSONLOGIC_CAST_OBJECT(handle);
            assert(object->refcount > 0);
            object->refcount --;
            if (object->refcount == 0) {
                jsonlogic_object_free(object);
                return JsonLogic_Null;
            }
            break;
        }
    }
    return handle;
}

JsonLogic_Handle jsonlogic_dissolve(JsonLogic_Handle handle) {
    switch (handle & JsonLogic_TypeMask) {
        case JsonLogic_Type_Array:
        {
            JsonLogic_Array *array = JSONLOGIC_CAST_ARRAY(handle);
            size_t size = array->size;
            // Set size to 0 before calling jsonlogic_dissolve() on any
            // children to stop recursion for ref-loops
            array->size = 0;
            for (size_t index = 0; index < size; ++ index) {
                jsonlogic_dissolve(array->items[index]);
            }
            break;
        }
        case JsonLogic_Type_Object:
        {
            JsonLogic_Object *object = JSONLOGIC_CAST_OBJECT(handle);
            size_t size = object->size;
            // Set size to 0 before calling jsonlogic_dissolve() on any
            // children to stop recursion for ref-loops
            object->size = 0;
            for (size_t index = 0; index < size; ++ index) {
                JsonLogic_Object_Entry *entry = &object->entries[index];
                jsonlogic_dissolve(entry->key);
                jsonlogic_dissolve(entry->value);
            }
            break;
        }
    }
    return jsonlogic_decref(handle);
}

size_t jsonlogic_get_refcount(JsonLogic_Handle handle) {
    switch (handle & JsonLogic_TypeMask) {
        case JsonLogic_Type_String:
            return JSONLOGIC_CAST_STRING(handle)->refcount;

        case JsonLogic_Type_Array:
            return JSONLOGIC_CAST_ARRAY(handle)->refcount;

        case JsonLogic_Type_Object:
            return JSONLOGIC_CAST_OBJECT(handle)->refcount;
    }
    return 1;
}

JSONLOGIC_DEF_UTF16(JSONLOGIC_ALT_IF, u"?:")
JSONLOGIC_DEF_UTF16(JSONLOGIC_ALL,    u"all")
JSONLOGIC_DEF_UTF16(JSONLOGIC_AND,    u"and")
JSONLOGIC_DEF_UTF16(JSONLOGIC_FILTER, u"filter")
JSONLOGIC_DEF_UTF16(JSONLOGIC_IF,     u"if")
JSONLOGIC_DEF_UTF16(JSONLOGIC_MAP,    u"map")
JSONLOGIC_DEF_UTF16(JSONLOGIC_NONE,   u"none")
JSONLOGIC_DEF_UTF16(JSONLOGIC_OR,     u"or")
JSONLOGIC_DEF_UTF16(JSONLOGIC_REDUCE, u"reduce")
JSONLOGIC_DEF_UTF16(JSONLOGIC_SOME,   u"some")

JsonLogic_Handle jsonlogic_apply(JsonLogic_Handle logic, JsonLogic_Handle input) {
    return jsonlogic_apply_custom(logic, input, &JsonLogic_Builtins);
}

#include "apply.c"

JsonLogic_Handle jsonlogic_op_NOT(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    if (argc == 0) {
        return JsonLogic_True;
    }
    return jsonlogic_not(args[0]);
}

JsonLogic_Handle jsonlogic_op_TO_BOOL(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    if (argc == 0) {
        return JsonLogic_False;
    }
    return jsonlogic_to_boolean(args[0]);
}

JsonLogic_Handle jsonlogic_op_NE(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    switch (argc) {
        case 0:  return JsonLogic_False;
        case 1:  return jsonlogic_not_equal(args[0], JsonLogic_Null);
        default: return jsonlogic_not_equal(args[0], args[1]);
    }
}

JsonLogic_Handle jsonlogic_op_STRICT_NE(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    switch (argc) {
        case 0:  return JsonLogic_False;
        case 1:  return jsonlogic_strict_not_equal(args[0], JsonLogic_Null);
        default: return jsonlogic_strict_not_equal(args[0], args[1]);
    }
}

JsonLogic_Handle jsonlogic_op_MOD(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    if (argc < 2) {
        return JsonLogic_NaN;
    }
    return jsonlogic_mod(args[0], args[1]);
}

JsonLogic_Handle jsonlogic_op_MUL(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    double value = 1.0;
    for (size_t index = 0; index < argc; ++ index) {
        value *= jsonlogic_to_double(args[index]);
    }
    return JSONLOGIC_NUM_TO_HNDL(value);
}

JsonLogic_Handle jsonlogic_op_ADD(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    double value = 0.0;
    for (size_t index = 0; index < argc; ++ index) {
        value += jsonlogic_to_double(args[index]);
    }
    return JSONLOGIC_NUM_TO_HNDL(value);
}

JsonLogic_Handle jsonlogic_op_SUB(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    switch (argc) {
        case 0:  return JSONLOGIC_NUM_TO_HNDL(0.0);
        case 1:  return jsonlogic_negative(args[0]);
        default: return jsonlogic_sub(args[0], args[1]);
    }
}

JsonLogic_Handle jsonlogic_op_DIV(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    switch (argc) {
        case 0:  return JsonLogic_NaN;
        case 1:  return jsonlogic_div(args[0], JsonLogic_Null);
        default: return jsonlogic_div(args[0], args[1]);
    }
}

JsonLogic_Handle jsonlogic_op_LT(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    switch (argc) {
        case 0: return JsonLogic_False;
        case 1: return jsonlogic_lt(args[0], JsonLogic_Null);
        case 2: return jsonlogic_lt(args[0], args[1]);
        default:
        {
            JsonLogic_Handle result = jsonlogic_lt(args[0], args[1]);
            if (JSONLOGIC_IS_ERROR(result)) {
                return result;
            }

            if (JSONLOGIC_IS_FALSE(result)) {
                return result;
            }

            return jsonlogic_lt(args[1], args[2]);
        }
    }
}

JsonLogic_Handle jsonlogic_op_LE(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    switch (argc) {
        case 0: return JsonLogic_True;
        case 1: return jsonlogic_le(args[0], JsonLogic_Null);
        case 2: return jsonlogic_le(args[0], args[1]);
        default:
        {
            JsonLogic_Handle result = jsonlogic_le(args[0], args[1]);
            if (JSONLOGIC_IS_ERROR(result)) {
                return result;
            }

            if (JSONLOGIC_IS_FALSE(result)) {
                return result;
            }

            return jsonlogic_le(args[1], args[2]);
        }
    }
}

JsonLogic_Handle jsonlogic_op_EQ(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    switch (argc) {
        case 0:  return JsonLogic_True;
        case 1:  return jsonlogic_equal(args[0], JsonLogic_Null);
        default: return jsonlogic_equal(args[0], args[1]);
    }
}

JsonLogic_Handle jsonlogic_op_STRICT_EQ(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    switch (argc) {
        case 0:  return JsonLogic_True;
        case 1:  return jsonlogic_strict_equal(args[0], JsonLogic_Null);
        default: return jsonlogic_strict_equal(args[0], args[1]);
    }
}

JsonLogic_Handle jsonlogic_op_GT(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    switch (argc) {
        case 0: return JsonLogic_False;
        case 1: return jsonlogic_gt(args[0], JsonLogic_Null);
        case 2: return jsonlogic_gt(args[0], args[1]);
        default:
        {
            JsonLogic_Handle result = jsonlogic_gt(args[0], args[1]);
            if (JSONLOGIC_IS_ERROR(result)) {
                return result;
            }

            if (JSONLOGIC_IS_FALSE(result)) {
                return result;
            }

            return jsonlogic_gt(args[1], args[2]);
        }
    }
}

JsonLogic_Handle jsonlogic_op_GE(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    switch (argc) {
        case 0: return JsonLogic_True;
        case 1: return jsonlogic_ge(args[0], JsonLogic_Null);
        case 2: return jsonlogic_ge(args[0], args[1]);
        default:
        {
            JsonLogic_Handle result = jsonlogic_ge(args[0], args[1]);
            if (JSONLOGIC_IS_ERROR(result)) {
                return result;
            }

            if (JSONLOGIC_IS_FALSE(result)) {
                return result;
            }

            return jsonlogic_ge(args[1], args[2]);
        }
    }
}

JsonLogic_Handle jsonlogic_op_CAT(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    if (argc > 0) {
        JsonLogic_StrBuf buf = JSONLOGIC_STRBUF_INIT;
        for (size_t index = 0; index < argc; ++ index) {
            JsonLogic_Handle item = args[index];
            if (!JSONLOGIC_IS_NULL(item) && !jsonlogic_strbuf_append(&buf, item)) {
                jsonlogic_strbuf_free(&buf);
                JSONLOGIC_ERROR_MEMORY();
                return JsonLogic_Error_OutOfMemory;
            }
        }

        return jsonlogic_string_into_handle(jsonlogic_strbuf_take(&buf));
    } else {
        return jsonlogic_empty_string();
    }
}

JsonLogic_Handle jsonlogic_op_IN(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    switch (argc) {
        case 0:  return JsonLogic_False;
        case 1:  return JsonLogic_False;
        default: return jsonlogic_includes(args[1], args[0]);
    }
}

JsonLogic_Handle jsonlogic_op_LOG(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    if (argc == 0) {
        fputs("null\n", stderr);
        return JsonLogic_Null;
    }
    JsonLogic_Handle value = args[0];
    if (JSONLOGIC_IS_ERROR(value)) {
        fprintf(stderr, "%s\n", jsonlogic_get_error_message(jsonlogic_get_error(value)));
        return value;
    }

    JsonLogic_Error error = jsonlogic_stringify_file(stdout, value);
    if (error == JSONLOGIC_ERROR_IO_ERROR) {
        int errnum = errno;
        puts(errnum != 0 ? strerror(errnum) : jsonlogic_get_error_message(error));
    } else if (error != JSONLOGIC_ERROR_SUCCESS) {
        puts(jsonlogic_get_error_message(error));
    } else {
        putchar('\n');
    }

    return jsonlogic_incref(value);
}

JsonLogic_Handle jsonlogic_op_MAX(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    if (argc == 0) {
        return JSONLOGIC_NUM_TO_HNDL(-INFINITY);
    }
    double number = JSONLOGIC_HNDL_TO_NUM(jsonlogic_to_number(args[0]));
    for (size_t index = 1; index < argc; ++ index) {
        JsonLogic_Handle other = JSONLOGIC_HNDL_TO_NUM(jsonlogic_to_number(args[index]));
        if (other > number) {
            number = other;
        }
    }
    return JSONLOGIC_NUM_TO_HNDL(number);
}

JsonLogic_Handle jsonlogic_op_MERGE(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    size_t array_size = 0;
    for (size_t index = 0; index < argc; ++ index) {
        JsonLogic_Handle item = args[index];
        if (JSONLOGIC_IS_ARRAY(item)) {
            array_size += JSONLOGIC_CAST_ARRAY(item)->size;
        } else {
            array_size += 1;
        }
    }
    JsonLogic_Array *array = jsonlogic_array_with_capacity(array_size);
    if (array == NULL) {
        return JsonLogic_Null;
    }
    size_t array_index = 0;
    for (size_t index = 0; index < argc; ++ index) {
        JsonLogic_Handle item = args[index];
        if (JSONLOGIC_IS_ARRAY(item)) {
            const JsonLogic_Array *child_array = JSONLOGIC_CAST_ARRAY(item);

            for (size_t child_index = 0; child_index < child_array->size; ++ child_index) {
                JsonLogic_Handle child_item = child_array->items[child_index];
                array->items[array_index ++] = jsonlogic_incref(child_item);
            }
        } else {
            array->items[array_index ++] = jsonlogic_incref(item);
        }
    }
    assert(array_index == array_size);
    return jsonlogic_array_into_handle(array);
}

JsonLogic_Handle jsonlogic_op_MIN(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    if (argc == 0) {
        return JSONLOGIC_NUM_TO_HNDL(INFINITY);
    }
    double number = JSONLOGIC_HNDL_TO_NUM(jsonlogic_to_number(args[0]));
    for (size_t index = 1; index < argc; ++ index) {
        double other = JSONLOGIC_HNDL_TO_NUM(jsonlogic_to_number(args[index]));
        if (other < number) {
            number = other;
        }
    }
    return JSONLOGIC_NUM_TO_HNDL(number);
}

JsonLogic_Handle jsonlogic_op_MISSING(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    size_t key_count;
    const JsonLogic_Handle *keys;
    if (argc > 0 && JSONLOGIC_IS_ARRAY(args[0])) {
        const JsonLogic_Array *array = JSONLOGIC_CAST_ARRAY(args[0]);
        key_count = array->size;
        keys      = array->items;
    } else {
        key_count = argc;
        keys      = args;
    }

    JsonLogic_Array *missing = jsonlogic_array_with_capacity(key_count);
    if (missing == NULL) {
        JSONLOGIC_ERROR_MEMORY();
        return JsonLogic_Error_OutOfMemory;
    }

    size_t missing_index = 0;
    for (size_t index = 0; index < key_count; ++ index) {
        JsonLogic_Handle key   = keys[index];
        JsonLogic_Handle value = jsonlogic_op_VAR(context, data, (JsonLogic_Handle[]){ key }, 1);

        if (JSONLOGIC_IS_NULL(value) || (JSONLOGIC_IS_STRING(value) && JSONLOGIC_CAST_STRING(value)->size == 0)) {
            jsonlogic_incref(key);
            missing->items[missing_index ++] = key;
        }

        jsonlogic_decref(value);
    }

    missing = jsonlogic_array_truncate(missing, missing_index);

    return jsonlogic_array_into_handle(missing);
}

JsonLogic_Handle jsonlogic_op_MISSING_SOME(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    if (argc < 2) {
        // jsonlogic.js crashes if argc < 2
        return JsonLogic_Null;
    }

    double need_count = jsonlogic_to_double(args[0]);
    JsonLogic_Handle options = args[1];

    size_t options_length = JSONLOGIC_IS_ARRAY(options) ?
        JSONLOGIC_CAST_ARRAY(options)->size : 1;

    JsonLogic_Handle missing = jsonlogic_op_MISSING(context, data, args + 1, 1);
    if (JSONLOGIC_IS_ERROR(missing)) {
        return missing;
    }
    JsonLogic_Array *array = JSONLOGIC_CAST_ARRAY(missing);

    if (options_length - array->size >= need_count) {
        missing = jsonlogic_array_into_handle(jsonlogic_array_truncate(array, 0));
    }

    return missing;
}

JsonLogic_Handle jsonlogic_op_SUBSTR(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    switch (argc) {
        case 0:  return JsonLogic_Null;
        case 1:  return jsonlogic_substr(args[0], JsonLogic_Null, JsonLogic_Null);
        case 2:  return jsonlogic_substr(args[0], args[1], JsonLogic_Null);
        default: return jsonlogic_substr(args[0], args[1], args[2]);
    }
}

JsonLogic_Handle jsonlogic_op_VAR(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    if (argc == 0 || JSONLOGIC_IS_NULL(args[0])) {
        return jsonlogic_incref(data);
    }

    JsonLogic_Handle arg0 = args[0];

    if (JSONLOGIC_IS_NUMBER(arg0)) {
        double number = JSONLOGIC_HNDL_TO_NUM(arg0);
        if (isfinite(number) && (double)(size_t)number == number) {
            return jsonlogic_get_index(data, (size_t)number);
        }
    }

    JsonLogic_Handle key = jsonlogic_to_string(arg0);
    if (JSONLOGIC_IS_ERROR(key)) {
        return key;
    }
    JsonLogic_Handle default_value = argc > 1 ? args[1] : JsonLogic_Null;

    const JsonLogic_String *strkey = JSONLOGIC_CAST_STRING(key);

    if (strkey->size == 0) {
        jsonlogic_decref(key);
        return jsonlogic_incref(data);
    }

    const char16_t *pos = strkey->str;
    const char16_t *next = jsonlogic_find_char(pos, strkey->size, u'.');
    if (next == NULL) {
        JsonLogic_Handle value = jsonlogic_get_utf16_sized(data, strkey->str, strkey->size);
        jsonlogic_decref(key);
        if (JSONLOGIC_IS_NULL(value)) {
            jsonlogic_incref(default_value);
            return default_value;
        }
        return value;
    }

    const char16_t *end = strkey->str + strkey->size;
    jsonlogic_incref(data);
    for (;;) {
        size_t size = next - pos;
        JsonLogic_Handle next_data = jsonlogic_get_utf16_sized(data, pos, size);
        jsonlogic_decref(data);
        if (JSONLOGIC_IS_NULL(next_data)) {
            jsonlogic_incref(default_value);
            jsonlogic_decref(key);
            return default_value;
        }

        pos = next + 1;
        if (pos >= end) {
            jsonlogic_decref(key);
            return next_data;
        }
        next = jsonlogic_find_char(pos, end - pos, u'.');
        if (next == NULL) {
            next = end;
        }
        data = next_data;
    }
}

#include "jsonlogic_intern.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <errno.h>
#include <string.h>

JSONLOGIC_DEF_UTF16(JSONLOGIC_ACCUMULATOR, u"accumulator")
JSONLOGIC_DEF_UTF16(JSONLOGIC_CURRENT,     u"current")

#define JSONLOGIC_ACCUMULATOR_HASH 0xd70624b7f0caa74d
#define JSONLOGIC_CURRENT_HASH     0x1e84ef3a9c8034b4

const JsonLogic_Handle JsonLogic_Null = { .intptr = JsonLogic_Type_Null };

JsonLogic_Type jsonlogic_get_type(JsonLogic_Handle handle) {
    static_assert(sizeof(void*) <= 8, "This library only works on platforms where pointers use <= 48 bits (e.g. x86_64).");

    if (JSONLOGIC_IS_NUMBER(handle)) {
        return JsonLogic_Type_Number;
    }

    return handle.intptr & JsonLogic_TypeMask;
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
    return handle.intptr == JSONLOGIC_TURE;
}

bool jsonlogic_is_false(JsonLogic_Handle handle) {
    return handle.intptr == JSONLOGIC_FALSE;
}

JsonLogic_Handle jsonlogic_incref(JsonLogic_Handle handle) {
    switch (handle.intptr & JsonLogic_TypeMask) {
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
    switch (handle.intptr & JsonLogic_TypeMask) {
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
    switch (handle.intptr & JsonLogic_TypeMask) {
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
    switch (handle.intptr & JsonLogic_TypeMask) {
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

#define JSONLOGIC_IS_OP(OPSRT, OP) \
    jsonlogic_utf16_equals((OPSRT)->str, (OPSRT)->size, (JSONLOGIC_##OP), (JSONLOGIC_##OP##_SIZE))

JsonLogic_Handle jsonlogic_apply_custom(
        JsonLogic_Handle logic,
        JsonLogic_Handle input,
        const JsonLogic_Operations *operations) {
    JsonLogic_Handle result = JsonLogic_Null;

    if (JSONLOGIC_IS_ARRAY(logic)) {
        const JsonLogic_Array *array = JSONLOGIC_CAST_ARRAY(logic);
        JsonLogic_Array *new_array = jsonlogic_array_with_capacity(array->size);
        if (new_array == NULL) {
            JSONLOGIC_ERROR_MEMORY();
            return JsonLogic_Error_OutOfMemory;
        }
        for (size_t index = 0; index < array->size; ++ index) {
            new_array->items[index] = jsonlogic_apply_custom(
                array->items[index],
                input,
                operations);
        }
        return jsonlogic_array_into_handle(new_array);
    }

    if (!JSONLOGIC_IS_OBJECT(logic)) {
        jsonlogic_incref(logic);
        return logic;
    }

    const JsonLogic_Object *object = JSONLOGIC_CAST_OBJECT(logic);

    if (object->used != 1) {
        jsonlogic_incref(logic);
        return logic;
    }

    const JsonLogic_Object_Entry *entry = NULL;
    for (size_t index = 0; index < object->size; ++ index) {
        if (!JSONLOGIC_IS_NULL(object->entries[index].key)) {
            entry = &object->entries[index];
            break;
        }
    }
    assert(entry != NULL);

    JsonLogic_Handle op    = entry->key;
    JsonLogic_Handle value = entry->value;

    if (!JSONLOGIC_IS_STRING(op)) {
        jsonlogic_incref(logic);
        return logic;
    }

    size_t value_count;
    JsonLogic_Handle *values;

    if (JSONLOGIC_IS_ARRAY(value)) {
        JsonLogic_Array *array = JSONLOGIC_CAST_ARRAY(value);
        value_count = array->size;
        values      = array->items;
    } else {
        value_count = 1;
        values      = &value;
    }

    JsonLogic_String *opstr = JSONLOGIC_CAST_STRING(op);

    if (JSONLOGIC_IS_OP(opstr, IF) || JSONLOGIC_IS_OP(opstr, ALT_IF)) {
        if (value_count == 0) {
            return JsonLogic_Null;
        }
        size_t index = 0;
        while (index < value_count - 1) {
            JsonLogic_Handle value = jsonlogic_apply_custom(
                values[index],
                input,
                operations);
            bool condition = jsonlogic_to_bool(value);
            jsonlogic_decref(value);
            if (condition) {
                return jsonlogic_apply_custom(
                    values[index + 1],
                    input,
                    operations);
            }
            index += 2;
        }
        if (index < value_count) {
            return jsonlogic_apply_custom(
                values[index],
                input,
                operations);
        }
        return JsonLogic_Null;
    } else if (JSONLOGIC_IS_OP(opstr, AND)) {
        if (value_count == 0) {
            return JsonLogic_Null;
        }
        for (size_t index = 0; index < value_count - 1; ++ index) {
            JsonLogic_Handle value = jsonlogic_apply_custom(
                values[index],
                input,
                operations);
            if (!jsonlogic_to_bool(value)) {
                return value;
            }
            jsonlogic_decref(value);
        }
        return jsonlogic_apply_custom(
            values[value_count - 1],
            input,
            operations);
    } else if (JSONLOGIC_IS_OP(opstr, OR)) {
        if (value_count == 0) {
            return JsonLogic_Null;
        }
        for (size_t index = 0; index < value_count - 1; ++ index) {
            JsonLogic_Handle value = jsonlogic_apply_custom(
                values[index],
                input,
                operations);
            if (jsonlogic_to_bool(value)) {
                return value;
            }
            jsonlogic_decref(value);
        }
        return jsonlogic_apply_custom(
            values[value_count - 1],
            input,
            operations);
    } else if (JSONLOGIC_IS_OP(opstr, FILTER)) {
        if (value_count == 0) {
            return jsonlogic_empty_array();
        }
        JsonLogic_Handle items = jsonlogic_apply_custom(
            values[0],
            input,
            operations);
        if (JSONLOGIC_IS_ERROR(items)) {
            return items;
        }
        if (!JSONLOGIC_IS_ARRAY(items) || value_count < 2 || !jsonlogic_to_bool(values[1])) {
            jsonlogic_decref(items);
            return jsonlogic_empty_array();
        }
        const JsonLogic_Array *array = JSONLOGIC_CAST_ARRAY(items);
        JsonLogic_Array *filtered = jsonlogic_array_with_capacity(array->size);
        if (filtered == NULL) {
            jsonlogic_decref(items);
            JSONLOGIC_ERROR_MEMORY();
            return JsonLogic_Error_OutOfMemory;
        }

        JsonLogic_Handle logic = values[1];

        size_t filtered_index = 0;
        for (size_t index = 0; index < array->size; ++ index) {
            JsonLogic_Handle item = array->items[index];
            JsonLogic_Handle condition = jsonlogic_apply_custom(
                logic,
                item,
                operations);
            if (jsonlogic_to_bool(condition)) {
                jsonlogic_incref(item);
                filtered->items[filtered_index ++] = item;
            }
            jsonlogic_decref(condition);
        }

        jsonlogic_decref(items);
        filtered = jsonlogic_array_truncate(filtered, filtered_index);
        return jsonlogic_array_into_handle(filtered);
    } else if (JSONLOGIC_IS_OP(opstr, MAP)) {
        if (value_count == 0) {
            return jsonlogic_empty_array();
        }
        JsonLogic_Handle items = jsonlogic_apply_custom(
            values[0],
            input,
            operations);
        if (JSONLOGIC_IS_ERROR(items)) {
            return items;
        }
        if (!JSONLOGIC_IS_ARRAY(items)) {
            jsonlogic_decref(items);
            return jsonlogic_empty_array();
        }
        const JsonLogic_Array *array = JSONLOGIC_CAST_ARRAY(items);
        JsonLogic_Array *mapped = jsonlogic_array_with_capacity(array->size);
        if (mapped == NULL) {
            jsonlogic_decref(items);
            JSONLOGIC_ERROR_MEMORY();
            return JsonLogic_Error_OutOfMemory;
        }

        JsonLogic_Handle logic = value_count < 2 ? JsonLogic_Null : values[1];

        for (size_t index = 0; index < array->size; ++ index) {
            JsonLogic_Handle item = array->items[index];
            JsonLogic_Handle value = jsonlogic_apply_custom(
                logic,
                item,
                operations);
            mapped->items[index] = value;
        }

        jsonlogic_decref(items);
        return jsonlogic_array_into_handle(mapped);
    } else if (JSONLOGIC_IS_OP(opstr, REDUCE)) {
        if (value_count == 0) {
            return JsonLogic_Null;
        }
        JsonLogic_Handle logic = JsonLogic_Null;
        JsonLogic_Handle init  = JsonLogic_Null;
        if (value_count > 1) {
            logic = values[1];
            if (value_count > 2) {
                init = values[2];
            }
        }

        JsonLogic_Handle items = jsonlogic_apply_custom(
            values[0],
            input,
            operations);
        if (JSONLOGIC_IS_ERROR(items)) {
            return items;
        }

        if (!JSONLOGIC_IS_ARRAY(items)) {
            jsonlogic_incref(init);
            jsonlogic_decref(items);
            return init;
        }
        const JsonLogic_Array *array = JSONLOGIC_CAST_ARRAY(items);
        JsonLogic_Handle str_accumulator = jsonlogic_string_from_utf16_sized(JSONLOGIC_ACCUMULATOR, JSONLOGIC_ACCUMULATOR_SIZE);
        JsonLogic_Handle str_current     = jsonlogic_string_from_utf16_sized(JSONLOGIC_CURRENT, JSONLOGIC_CURRENT_SIZE);
        JsonLogic_Handle reduce_context = jsonlogic_object_from(
            jsonlogic_entry(str_accumulator, JsonLogic_Null),
            jsonlogic_entry(str_current,     JsonLogic_Null)
        );
        jsonlogic_decref(str_accumulator);
        jsonlogic_decref(str_current);
        if (JSONLOGIC_IS_ERROR(reduce_context)) {
            jsonlogic_decref(items);
            return reduce_context;
        }
        JsonLogic_Object *object = JSONLOGIC_CAST_OBJECT(reduce_context);
        JsonLogic_Handle accumulator = jsonlogic_incref(init);

        size_t accumulator_index = jsonlogic_object_get_index_utf16_with_hash(
            object,
            JSONLOGIC_ACCUMULATOR_HASH,
            JSONLOGIC_ACCUMULATOR,
            JSONLOGIC_ACCUMULATOR_SIZE);
        assert(accumulator_index < object->size);

        size_t current_index = jsonlogic_object_get_index_utf16_with_hash(
            object,
            JSONLOGIC_CURRENT_HASH,
            JSONLOGIC_CURRENT,
            JSONLOGIC_CURRENT_SIZE);
        assert(current_index < object->size);

        for (size_t index = 0; index < array->size; ++ index) {
            object->entries[accumulator_index].value = accumulator;
            object->entries[current_index].value     = array->items[index];

            JsonLogic_Handle new_accumulator = jsonlogic_apply_custom(
                logic,
                reduce_context,
                operations);

            jsonlogic_decref(accumulator);
            accumulator = new_accumulator;

            object->entries[accumulator_index].value = JsonLogic_Null;
            object->entries[current_index].value     = JsonLogic_Null;
        }

        jsonlogic_decref(reduce_context);
        jsonlogic_decref(items);

        return accumulator;
    } else if (JSONLOGIC_IS_OP(opstr, ALL)) {
        // to be sane and logical it should return true on empty array,
        // but JsonLogic is not logical here:
        // https://github.com/jwadhams/json-logic-js/blob/c1dd82f5b15d8a553bb7a0cfa841ab8a11a9c227/logic.js#L318
        if (value_count == 0) {
            return JsonLogic_False;
        }

        JsonLogic_Handle items = jsonlogic_apply_custom(
            values[0],
            input,
            operations);
        if (JSONLOGIC_IS_ERROR(items)) {
            return items;
        }

        if (!JSONLOGIC_IS_ARRAY(items) || value_count < 1) {
            jsonlogic_decref(items);
            return JsonLogic_False;
        }

        JsonLogic_Handle logic = value_count > 1 ? values[1] : JsonLogic_Null;

        const JsonLogic_Array *array = JSONLOGIC_CAST_ARRAY(items);
        if (array->size == 0) {
            jsonlogic_decref(items);
            return JsonLogic_False;
        }

        for (size_t index = 0; index < array->size; ++ index) {
            JsonLogic_Handle item = array->items[index];
            JsonLogic_Handle condition = jsonlogic_apply_custom(
                logic,
                item,
                operations);
            if (!jsonlogic_to_bool(condition)) {
                jsonlogic_decref(condition);
                jsonlogic_decref(items);
                return JsonLogic_False;
            }
            jsonlogic_decref(condition);
        }

        jsonlogic_decref(items);
        return JsonLogic_True;
    } else if (JSONLOGIC_IS_OP(opstr, SOME)) {
        if (value_count == 0) {
            return JsonLogic_False;
        }

        JsonLogic_Handle items = jsonlogic_apply_custom(
            values[0],
            input,
            operations);
        if (JSONLOGIC_IS_ERROR(items)) {
            return items;
        }

        if (!JSONLOGIC_IS_ARRAY(items) || value_count < 1) {
            jsonlogic_decref(items);
            return JsonLogic_False;
        }

        JsonLogic_Handle logic = value_count > 1 ? values[1] : JsonLogic_Null;

        const JsonLogic_Array *array = JSONLOGIC_CAST_ARRAY(items);
        if (array->size == 0) {
            jsonlogic_decref(items);
            return JsonLogic_False;
        }

        for (size_t index = 0; index < array->size; ++ index) {
            JsonLogic_Handle item = array->items[index];
            JsonLogic_Handle condition = jsonlogic_apply_custom(
                logic,
                item,
                operations);
            if (jsonlogic_to_bool(condition)) {
                jsonlogic_decref(condition);
                jsonlogic_decref(items);
                return JsonLogic_True;
            }
            jsonlogic_decref(condition);
        }

        jsonlogic_decref(items);
        return JsonLogic_False;
    } else if (JSONLOGIC_IS_OP(opstr, NONE)) {
        if (value_count == 0) {
            return JsonLogic_True;
        }

        JsonLogic_Handle items = jsonlogic_apply_custom(
            values[0],
            input,
            operations);
        if (JSONLOGIC_IS_ERROR(items)) {
            return items;
        }

        if (!JSONLOGIC_IS_ARRAY(items) || value_count < 1) {
            jsonlogic_decref(items);
            return JsonLogic_True;
        }

        JsonLogic_Handle logic = value_count > 1 ? values[1] : JsonLogic_Null;

        const JsonLogic_Array *array = JSONLOGIC_CAST_ARRAY(items);
        if (array->size == 0) {
            jsonlogic_decref(items);
            return JsonLogic_True;
        }

        for (size_t index = 0; index < array->size; ++ index) {
            JsonLogic_Handle item = array->items[index];
            JsonLogic_Handle condition = jsonlogic_apply_custom(
                logic,
                item,
                operations);
            if (jsonlogic_to_bool(condition)) {
                jsonlogic_decref(condition);
                jsonlogic_decref(items);
                return JsonLogic_False;
            }
            jsonlogic_decref(condition);
        }

        jsonlogic_decref(items);
        return JsonLogic_True;
    }

    if (opstr->hash == JSONLOGIC_HASH_UNSET) {
        opstr->hash = jsonlogic_hash_fnv1a_utf16(opstr->str, opstr->size);
    }

    const JsonLogic_Operation *opptr = jsonlogic_operations_get_with_hash(
        operations, opstr->hash, opstr->str, opstr->size);

    if (opptr == NULL) {
        // JSONLOGIC_DEBUG_UTF16("%s", opstr->str, opstr->size, "illegal operation");
        return JsonLogic_Error_IllegalOperation;
    }

#define ARGC 8
    JsonLogic_Handle argbuf[ARGC];
    JsonLogic_Handle *args;

    if (value_count > ARGC) {
        args = malloc(sizeof(JsonLogic_Handle) * value_count);
        if (args == NULL) {
            JSONLOGIC_ERROR_MEMORY();
            return JsonLogic_Error_OutOfMemory;
        }
    } else {
        args = argbuf;
    }

    for (size_t index = 0; index < value_count; ++ index) {
        args[index] = jsonlogic_apply_custom(
            values[index],
            input,
            operations);
    }

    result = opptr->funct(opptr->context, input, args, value_count);

    for (size_t index = 0; index < value_count; ++ index) {
        jsonlogic_decref(args[index]);
    }

    if (value_count > ARGC) {
        free(args);
    }

    return result;
}

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
    return (JsonLogic_Handle){ .number = value };
}

JsonLogic_Handle jsonlogic_op_ADD(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    double value = 0.0;
    for (size_t index = 0; index < argc; ++ index) {
        value += jsonlogic_to_double(args[index]);
    }
    return (JsonLogic_Handle){ .number = value };
}

JsonLogic_Handle jsonlogic_op_SUB(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    switch (argc) {
        case 0:  return (JsonLogic_Handle){ .number = 0.0 };
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
        puts(errnum != 0 ? strerror(errno) : jsonlogic_get_error_message(error));
    } else if (error != JSONLOGIC_ERROR_SUCCESS) {
        puts(jsonlogic_get_error_message(error));
    } else {
        putchar('\n');
    }

    return value;
}

JsonLogic_Handle jsonlogic_op_MAX(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    if (argc == 0) {
        return (JsonLogic_Handle){ .number = -INFINITY };
    }
    JsonLogic_Handle value = jsonlogic_to_number(args[0]);
    for (size_t index = 1; index < argc; ++ index) {
        JsonLogic_Handle other = jsonlogic_to_number(args[index]);
        if (other.number > value.number) {
            value.number = other.number;
        }
    }
    return value;
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
        return (JsonLogic_Handle){ .number = INFINITY };
    }
    JsonLogic_Handle value = jsonlogic_to_number(args[0]);
    for (size_t index = 1; index < argc; ++ index) {
        JsonLogic_Handle other = jsonlogic_to_number(args[index]);
        if (other.number < value.number) {
            value.number = other.number;
        }
    }
    return value;
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

    if (JSONLOGIC_IS_NUMBER(arg0) && isfinite(arg0.number) && floor(arg0.number) == arg0.number) {
        return jsonlogic_get_index(data, (size_t)arg0.number);
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

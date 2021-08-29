#include "jsonlogic_intern.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

JSONLOGIC_DEF_UTF16(JSONLOGIC_ACCUMULATOR, u"accumulator")
JSONLOGIC_DEF_UTF16(JSONLOGIC_CURRENT,     u"current")

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
    return JSONLOGIC_IS_NUMBER(handle);
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

static int jsonlogic_operation_entry_compare(const void *leftptr, const void *rightptr) {
    const JsonLogic_Operation_Entry *left  = leftptr;
    const JsonLogic_Operation_Entry *right = rightptr;

    return jsonlogic_utf16_compare(left->key, left->key_size, right->key, right->key_size);
}

JsonLogic_Operation jsonlogic_operation_get(const JsonLogic_Operation_Entry operations[], size_t count, const char16_t *key, size_t key_size) {
    if (count == 0) {
        return NULL;
    }

    size_t left  = 0;
    size_t right = count;
    while (left < right) {
        size_t mid = (left + right - 1) / 2;
        const JsonLogic_Operation_Entry *entry = &operations[mid];
        int cmp = jsonlogic_utf16_compare(
            entry->key, entry->key_size,
            key, key_size);
        if (cmp < 0) {
            left  = mid + 1;
        } else if (cmp > 0) {
            right = mid;
        } else {
            return entry->operation;
        }
    }

    return NULL;
}

void jsonlogic_operations_sort(JsonLogic_Operation_Entry operations[], size_t count) {
    qsort(operations, count, sizeof(JsonLogic_Operation_Entry), jsonlogic_operation_entry_compare);
}

JSONLOGIC_DEF_UTF16(JSONLOGIC_NOT,          u"!")
JSONLOGIC_DEF_UTF16(JSONLOGIC_TO_BOOL,      u"!!")
JSONLOGIC_DEF_UTF16(JSONLOGIC_NE,           u"!=")
JSONLOGIC_DEF_UTF16(JSONLOGIC_STRICT_NE,    u"!==")
JSONLOGIC_DEF_UTF16(JSONLOGIC_MOD,          u"%")
JSONLOGIC_DEF_UTF16(JSONLOGIC_MUL,          u"*")
JSONLOGIC_DEF_UTF16(JSONLOGIC_ADD,          u"+")
JSONLOGIC_DEF_UTF16(JSONLOGIC_SUB,          u"-")
JSONLOGIC_DEF_UTF16(JSONLOGIC_DIV,          u"/")
JSONLOGIC_DEF_UTF16(JSONLOGIC_LT,           u"<")
JSONLOGIC_DEF_UTF16(JSONLOGIC_LE,           u"<=")
JSONLOGIC_DEF_UTF16(JSONLOGIC_EQ,           u"==")
JSONLOGIC_DEF_UTF16(JSONLOGIC_STRICT_EQ,    u"===")
JSONLOGIC_DEF_UTF16(JSONLOGIC_GT,           u">")
JSONLOGIC_DEF_UTF16(JSONLOGIC_GE,           u">=")
JSONLOGIC_DEF_UTF16(JSONLOGIC_ALT_IF,       u"?:")
JSONLOGIC_DEF_UTF16(JSONLOGIC_ALL,          u"all")
JSONLOGIC_DEF_UTF16(JSONLOGIC_AND,          u"and")
JSONLOGIC_DEF_UTF16(JSONLOGIC_CAT,          u"cat")
JSONLOGIC_DEF_UTF16(JSONLOGIC_FILTER,       u"filter")
JSONLOGIC_DEF_UTF16(JSONLOGIC_IF,           u"if")
JSONLOGIC_DEF_UTF16(JSONLOGIC_IN,           u"in")
JSONLOGIC_DEF_UTF16(JSONLOGIC_LOG,          u"log")
JSONLOGIC_DEF_UTF16(JSONLOGIC_MAP,          u"map")
JSONLOGIC_DEF_UTF16(JSONLOGIC_MAX,          u"max")
JSONLOGIC_DEF_UTF16(JSONLOGIC_MERGE,        u"merge")
JSONLOGIC_DEF_UTF16(JSONLOGIC_MIN,          u"min")
JSONLOGIC_DEF_UTF16(JSONLOGIC_MISSING,      u"missing")
JSONLOGIC_DEF_UTF16(JSONLOGIC_MISSING_SOME, u"missing_some")
JSONLOGIC_DEF_UTF16(JSONLOGIC_NONE,         u"none")
JSONLOGIC_DEF_UTF16(JSONLOGIC_OR,           u"or")
JSONLOGIC_DEF_UTF16(JSONLOGIC_REDUCE,       u"reduce")
JSONLOGIC_DEF_UTF16(JSONLOGIC_SOME,         u"some")
JSONLOGIC_DEF_UTF16(JSONLOGIC_SUBSTR,       u"substr")
JSONLOGIC_DEF_UTF16(JSONLOGIC_VAR,          u"var")

static JsonLogic_Handle jsonlogic_op_NOT         (JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
static JsonLogic_Handle jsonlogic_op_TO_BOOL     (JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
static JsonLogic_Handle jsonlogic_op_NE          (JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
static JsonLogic_Handle jsonlogic_op_STRICT_NE   (JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
static JsonLogic_Handle jsonlogic_op_MOD         (JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
static JsonLogic_Handle jsonlogic_op_MUL         (JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
static JsonLogic_Handle jsonlogic_op_ADD         (JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
static JsonLogic_Handle jsonlogic_op_SUB         (JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
static JsonLogic_Handle jsonlogic_op_DIV         (JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
static JsonLogic_Handle jsonlogic_op_LT          (JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
static JsonLogic_Handle jsonlogic_op_LE          (JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
static JsonLogic_Handle jsonlogic_op_EQ          (JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
static JsonLogic_Handle jsonlogic_op_STRICT_EQ   (JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
static JsonLogic_Handle jsonlogic_op_GT          (JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
static JsonLogic_Handle jsonlogic_op_GE          (JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
static JsonLogic_Handle jsonlogic_op_CAT         (JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
static JsonLogic_Handle jsonlogic_op_IN          (JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
static JsonLogic_Handle jsonlogic_op_LOG         (JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
static JsonLogic_Handle jsonlogic_op_MAX         (JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
static JsonLogic_Handle jsonlogic_op_MERGE       (JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
static JsonLogic_Handle jsonlogic_op_MIN         (JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
static JsonLogic_Handle jsonlogic_op_MISSING     (JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
static JsonLogic_Handle jsonlogic_op_MISSING_SOME(JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
static JsonLogic_Handle jsonlogic_op_SUBSTR      (JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
static JsonLogic_Handle jsonlogic_op_VAR         (JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);

#define JSONLOGIC_BUILTIN(NAME) \
    { .key = (JSONLOGIC_##NAME), .key_size = (JSONLOGIC_##NAME##_SIZE), .operation = (jsonlogic_op_##NAME) }

#define JSONLOGIC_BUILTIN_COUNT 25
const JsonLogic_Operation_Entry JsonLogic_Builtins[JSONLOGIC_BUILTIN_COUNT] = {
    JSONLOGIC_BUILTIN(NOT         ),
    JSONLOGIC_BUILTIN(TO_BOOL     ),
    JSONLOGIC_BUILTIN(NE          ),
    JSONLOGIC_BUILTIN(STRICT_NE   ),
    JSONLOGIC_BUILTIN(MOD         ),
    JSONLOGIC_BUILTIN(MUL         ),
    JSONLOGIC_BUILTIN(ADD         ),
    JSONLOGIC_BUILTIN(SUB         ),
    JSONLOGIC_BUILTIN(DIV         ),
    JSONLOGIC_BUILTIN(LT          ),
    JSONLOGIC_BUILTIN(LE          ),
    JSONLOGIC_BUILTIN(EQ          ),
    JSONLOGIC_BUILTIN(STRICT_EQ   ),
    JSONLOGIC_BUILTIN(GT          ),
    JSONLOGIC_BUILTIN(GE          ),
    JSONLOGIC_BUILTIN(CAT         ),
    JSONLOGIC_BUILTIN(IN          ),
    JSONLOGIC_BUILTIN(LOG         ),
    JSONLOGIC_BUILTIN(MAX         ),
    JSONLOGIC_BUILTIN(MERGE       ),
    JSONLOGIC_BUILTIN(MIN         ),
    JSONLOGIC_BUILTIN(MISSING     ),
    JSONLOGIC_BUILTIN(MISSING_SOME),
    JSONLOGIC_BUILTIN(SUBSTR      ),
    JSONLOGIC_BUILTIN(VAR         ),
};

JsonLogic_Handle jsonlogic_apply(JsonLogic_Handle logic, JsonLogic_Handle input) {
    return jsonlogic_apply_custom(logic, input, NULL, 0);
}

#define JSONLOGIC_IS_LOGIC(handle) \
    (JSONLOGIC_IS_OBJECT(handle) && JSONLOGIC_CAST_OBJECT(handle)->size == 1)

#define JSONLOGIC_IS_OP(OPSRT, OP) \
    jsonlogic_utf16_equals((OPSRT)->str, (OPSRT)->size, (JSONLOGIC_##OP), (JSONLOGIC_##OP##_SIZE))

JsonLogic_Handle jsonlogic_apply_custom(
        JsonLogic_Handle logic,
        JsonLogic_Handle input,
        const JsonLogic_Operation_Entry operations[],
        const size_t operation_count) {
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
                operations, operation_count);
        }
        return jsonlogic_array_into_handle(new_array);
    }

    if (!JSONLOGIC_IS_LOGIC(logic)) {
        jsonlogic_incref(logic);
        return logic;
    }

    const JsonLogic_Object *object = JSONLOGIC_CAST_OBJECT(logic);
    JsonLogic_Handle op    = object->entries[0].key;
    JsonLogic_Handle value = object->entries[0].value;

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

    const JsonLogic_String *opstr = JSONLOGIC_CAST_STRING(op);

    if (JSONLOGIC_IS_OP(opstr, IF) || JSONLOGIC_IS_OP(opstr, ALT_IF)) {
        if (value_count == 0) {
            return JsonLogic_Null;
        }
        size_t index = 0;
        while (index < value_count - 1) {
            JsonLogic_Handle value = jsonlogic_apply_custom(
                values[index ++],
                input,
                operations,
                operation_count);
            bool condition = jsonlogic_to_bool(value);
            jsonlogic_decref(value);
            if (condition) {
                return jsonlogic_apply_custom(
                    values[index ++],
                    input,
                    operations,
                    operation_count);
            }
        }
        if (index < value_count) {
            return jsonlogic_apply_custom(
                values[index],
                input,
                operations,
                operation_count);
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
                operations,
                operation_count);
            if (!jsonlogic_to_bool(value)) {
                return value;
            }
            jsonlogic_decref(value);
        }
        return jsonlogic_apply_custom(
            values[value_count - 1],
            input,
            operations,
            operation_count);
    } else if (JSONLOGIC_IS_OP(opstr, OR)) {
        if (value_count == 0) {
            return JsonLogic_Null;
        }
        for (size_t index = 0; index < value_count - 1; ++ index) {
            JsonLogic_Handle value = jsonlogic_apply_custom(
                values[index],
                input,
                operations,
                operation_count);
            if (jsonlogic_to_bool(value)) {
                return value;
            }
            jsonlogic_decref(value);
        }
        return jsonlogic_apply_custom(
            values[value_count - 1],
            input,
            operations,
            operation_count);
    } else if (JSONLOGIC_IS_OP(opstr, FILTER)) {
        if (value_count == 0) {
            return jsonlogic_empty_array();
        }
        JsonLogic_Handle items = jsonlogic_apply_custom(
            values[0],
            input,
            operations,
            operation_count);
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
                operations,
                operation_count);
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
            operations,
            operation_count);
        if (!JSONLOGIC_IS_ARRAY(items) || value_count < 1) {
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
                operations,
                operation_count);
            jsonlogic_incref(value);
            mapped->items[index ++] = value;
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
            operations,
            operation_count);

        if (!JSONLOGIC_IS_ARRAY(items)) {
            jsonlogic_incref(init);
            jsonlogic_decref(items);
            return init;
        }
        const JsonLogic_Array *array = JSONLOGIC_CAST_ARRAY(items);
        JsonLogic_Handle context = jsonlogic_object_from(
            jsonlogic_entry_latin1("accumulator", JsonLogic_Null),
            jsonlogic_entry_latin1("current", JsonLogic_Null)
        );
        if (!JSONLOGIC_IS_OBJECT(context)) {
            jsonlogic_decref(items);
            JSONLOGIC_ERROR_MEMORY();
            return JsonLogic_Error_OutOfMemory;
        }
        JsonLogic_Object *object = JSONLOGIC_CAST_OBJECT(context);
        JsonLogic_Handle accumulator = jsonlogic_incref(init);

        size_t accumulator_index = jsonlogic_object_get_index_utf16(object, JSONLOGIC_ACCUMULATOR, JSONLOGIC_ACCUMULATOR_SIZE);
        assert(accumulator_index < object->size);

        size_t current_index = jsonlogic_object_get_index_utf16(object, JSONLOGIC_CURRENT, JSONLOGIC_CURRENT_SIZE);
        assert(current_index < object->size);

        for (size_t index = 0; index < array->size; ++ index) {
            object->entries[accumulator_index].value = accumulator;
            object->entries[current_index].value     = array->items[index];

            JsonLogic_Handle new_accumulator = jsonlogic_apply_custom(
                logic,
                context,
                operations,
                operation_count
            );

            jsonlogic_decref(accumulator);
            accumulator = new_accumulator;

            object->entries[accumulator_index].value = JsonLogic_Null;
            object->entries[current_index].value     = JsonLogic_Null;
        }

        jsonlogic_decref(context);
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
            operations,
            operation_count);
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
                operations,
                operation_count
            );
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
            operations,
            operation_count);
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
                operations,
                operation_count
            );
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
            operations,
            operation_count);
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
                operations,
                operation_count
            );
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

    JsonLogic_Operation opfunc = jsonlogic_operation_get(operations, operation_count, opstr->str, opstr->size);
    if (opfunc == NULL) {
        opfunc = jsonlogic_operation_get(JsonLogic_Builtins, JSONLOGIC_BUILTIN_COUNT, opstr->str, opstr->size);
        if (opfunc == NULL) {
            JSONLOGIC_ERROR("%s", "illegal operation");
            return JsonLogic_Error_IllegalOperation;
        }
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
            operations,
            operation_count);
    }

    result = opfunc(input, args, value_count);

    for (size_t index = 0; index < value_count; ++ index) {
        jsonlogic_decref(args[index]);
    }

    if (value_count > ARGC) {
        free(args);
    }

    return result;
}

JsonLogic_Handle jsonlogic_op_NOT(JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    if (argc == 0) {
        return JsonLogic_True;
    }
    return jsonlogic_not(args[0]);
}

JsonLogic_Handle jsonlogic_op_TO_BOOL(JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    if (argc == 0) {
        return JsonLogic_False;
    }
    return jsonlogic_to_boolean(args[0]);
}

JsonLogic_Handle jsonlogic_op_NE(JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    switch (argc) {
        case 0:  return JsonLogic_False;
        case 1:  return jsonlogic_not_equal(args[0], JsonLogic_Null);
        default: return jsonlogic_not_equal(args[0], args[1]);
    }
}

JsonLogic_Handle jsonlogic_op_STRICT_NE(JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    switch (argc) {
        case 0:  return JsonLogic_False;
        case 1:  return jsonlogic_strict_not_equal(args[0], JsonLogic_Null);
        default: return jsonlogic_strict_not_equal(args[0], args[1]);
    }
}

JsonLogic_Handle jsonlogic_op_MOD(JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    if (argc < 2) {
        return JsonLogic_NaN;
    }
    return jsonlogic_mod(args[0], args[1]);
}

JsonLogic_Handle jsonlogic_op_MUL(JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    double value = 1.0;
    for (size_t index = 0; index < argc; ++ index) {
        value *= jsonlogic_to_double(args[index]);
    }
    return (JsonLogic_Handle){ .number = value };
}

JsonLogic_Handle jsonlogic_op_ADD(JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    double value = 0.0;
    for (size_t index = 0; index < argc; ++ index) {
        value += jsonlogic_to_double(args[index]);
    }
    return (JsonLogic_Handle){ .number = value };
}

JsonLogic_Handle jsonlogic_op_SUB(JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    switch (argc) {
        case 0:  return (JsonLogic_Handle){ .number = 0.0 };
        case 1:  return jsonlogic_negative(args[0]);
        default: return jsonlogic_sub(args[0], args[1]);
    }
}

JsonLogic_Handle jsonlogic_op_DIV(JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    switch (argc) {
        case 0:  return JsonLogic_NaN;
        case 1:  return jsonlogic_div(args[0], JsonLogic_Null);
        default: return jsonlogic_div(args[0], args[1]);
    }
}

JsonLogic_Handle jsonlogic_op_LT(JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    switch (argc) {
        case 0:  return JsonLogic_False;
        case 1:  return jsonlogic_lt(args[0], JsonLogic_Null);
        default: return jsonlogic_lt(args[0], args[1]);
    }
}

JsonLogic_Handle jsonlogic_op_LE(JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    switch (argc) {
        case 0:  return JsonLogic_True;
        case 1:  return jsonlogic_le(args[0], JsonLogic_Null);
        default: return jsonlogic_le(args[0], args[1]);
    }
}

JsonLogic_Handle jsonlogic_op_EQ(JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    switch (argc) {
        case 0:  return JsonLogic_True;
        case 1:  return jsonlogic_equal(args[0], JsonLogic_Null);
        default: return jsonlogic_equal(args[0], args[1]);
    }
}

JsonLogic_Handle jsonlogic_op_STRICT_EQ(JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    switch (argc) {
        case 0:  return JsonLogic_True;
        case 1:  return jsonlogic_strict_equal(args[0], JsonLogic_Null);
        default: return jsonlogic_strict_equal(args[0], args[1]);
    }
}

JsonLogic_Handle jsonlogic_op_GT(JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    switch (argc) {
        case 0:  return JsonLogic_False;
        case 1:  return jsonlogic_gt(args[0], JsonLogic_Null);
        default: return jsonlogic_gt(args[0], args[1]);
    }
}

JsonLogic_Handle jsonlogic_op_GE(JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    switch (argc) {
        case 0:  return JsonLogic_True;
        case 1:  return jsonlogic_gt(args[0], JsonLogic_Null);
        default: return jsonlogic_gt(args[0], args[1]);
    }
}

JsonLogic_Handle jsonlogic_op_CAT(JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    if (argc > 0) {
        JsonLogic_StrBuf buf = JSONLOGIC_STRBUF_INIT;

        if (!jsonlogic_strbuf_append(&buf, args[0])) {
            jsonlogic_strbuf_free(&buf);
            JSONLOGIC_ERROR_MEMORY();
            return JsonLogic_Error_OutOfMemory;
        }

        for (size_t index = 1; index < argc; ++ index) {
            if (!jsonlogic_strbuf_append_utf16(&buf, u",", 1)) {
                jsonlogic_strbuf_free(&buf);
                JSONLOGIC_ERROR_MEMORY();
                return JsonLogic_Error_OutOfMemory;
            }
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

JsonLogic_Handle jsonlogic_op_IN(JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    switch (argc) {
        case 0:  return JsonLogic_False;
        case 1:  return JsonLogic_False;
        default: return jsonlogic_includes(args[1], args[0]);
    }
}

JsonLogic_Handle jsonlogic_op_LOG(JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    if (argc == 0) {
        fputs("null\n", stderr);
        return JsonLogic_Null;
    }
    JsonLogic_Handle value = args[0];
    if (JSONLOGIC_IS_ERROR(value)) {
        fprintf(stderr, "%s\n", jsonlogic_get_error_message(jsonlogic_get_error(value)));
        return value;
    }
    char *utf8 = jsonlogic_stringify_utf8(value);
    if (utf8 == NULL) {
        fputs("error calling stringify\n", stderr);
    } else {
        fprintf(stderr, "%s\n", utf8);
        free(utf8);
    }
    return value;
}

JsonLogic_Handle jsonlogic_op_MAX(JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
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

JsonLogic_Handle jsonlogic_op_MERGE(JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
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

JsonLogic_Handle jsonlogic_op_MIN(JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
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

JsonLogic_Handle jsonlogic_op_MISSING(JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
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
        return JsonLogic_Null;
    }

    size_t missing_index = 0;
    for (size_t index = 0; index < key_count; ++ index) {
        JsonLogic_Handle key   = keys[index];
        JsonLogic_Handle value = jsonlogic_op_VAR(data, (JsonLogic_Handle[]){ key }, 1);

        if (JSONLOGIC_IS_NULL(value) || (JSONLOGIC_IS_STRING(value) && JSONLOGIC_CAST_STRING(value)->size == 0)) {
            jsonlogic_incref(key);
            missing->items[missing_index ++] = key;
        }

        jsonlogic_decref(value);
    }

    missing = jsonlogic_array_truncate(missing, missing_index);

    return jsonlogic_array_into_handle(missing);
}

JsonLogic_Handle jsonlogic_op_MISSING_SOME(JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    if (argc < 2) {
        // jsonlogic.js crashes if argc < 2
        return JsonLogic_Null;
    }

    double need_count = jsonlogic_to_double(args[0]);
    JsonLogic_Handle options = args[1];

    size_t options_length = JSONLOGIC_IS_ARRAY(options) ?
        JSONLOGIC_CAST_ARRAY(options)->size : 1;

    JsonLogic_Handle missing = jsonlogic_op_MISSING(data, args + 1, 1);
    if (!JSONLOGIC_IS_ARRAY(missing)) {
        jsonlogic_decref(missing);
        return JsonLogic_Null;
    }
    JsonLogic_Array *array = JSONLOGIC_CAST_ARRAY(missing);

    if (options_length - array->size >= need_count) {
        missing = jsonlogic_array_into_handle(jsonlogic_array_truncate(array, 0));
    }

    return missing;
}

JsonLogic_Handle jsonlogic_op_SUBSTR(JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    switch (argc) {
        case 0:  return JsonLogic_Null;
        case 1:  return jsonlogic_substr(args[0], JsonLogic_Null, JsonLogic_Null);
        case 2:  return jsonlogic_substr(args[0], args[1], JsonLogic_Null);
        default: return jsonlogic_substr(args[0], args[1], args[2]);
    }
}

JsonLogic_Handle jsonlogic_op_VAR(JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    if (argc == 0) {
        jsonlogic_incref(data);
        return data;
    }

    JsonLogic_Handle key = jsonlogic_to_string(args[0]);
    if (JSONLOGIC_IS_ERROR(key)) {
        return key;
    }
    JsonLogic_Handle default_value = argc > 1 ? args[1] : JsonLogic_Null;

    if (JSONLOGIC_IS_NULL(key) || (JSONLOGIC_IS_STRING(key) && JSONLOGIC_CAST_STRING(key)->size == 0)) {
        jsonlogic_incref(data);
        return data;
    }

    const JsonLogic_String *strkey = JSONLOGIC_CAST_STRING(key);

    const char16_t *pos = strkey->str;
    size_t keysize = strkey->size;
    const char16_t *next = jsonlogic_find_char(pos, keysize, u'.');
    if (next == NULL) {
        JsonLogic_Handle value = jsonlogic_get_item(data, key);
        if (JSONLOGIC_IS_NULL(value)) {
            jsonlogic_incref(default_value);
            return default_value;
        }
        return value;
    }

    for (;;) {
        size_t size = next - pos;
        JsonLogic_Handle prop = jsonlogic_string_from_utf16_sized(pos, size);
        data = jsonlogic_get_item(data, key);
        jsonlogic_decref(prop);
        if (JSONLOGIC_IS_NULL(data)) {
            jsonlogic_incref(default_value);
            return default_value;
        }

        pos = next + 1;
        keysize -= size;
        if (keysize == 0) {
            jsonlogic_incref(default_value);
            return default_value;
        }
        -- keysize;
        next = jsonlogic_find_char(pos, keysize, u'.');
        if (next == NULL) {
            next = pos + keysize;
        }
    }
}

#include "jsonlogic_intern.h"


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
    for (size_t index = object->first_index; index < object->size; ++ index) {
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

    if (JSONLOGIC_IS_OP(opstr, IF)
#ifndef JSONLOGIC_COMPILE_CERTLOGIC
        || JSONLOGIC_IS_OP(opstr, ALT_IF)
#endif
     ) {
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
#ifndef JSONLOGIC_COMPILE_CERTLOGIC
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
#endif
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
#ifdef JSONLOGIC_COMPILE_CERTLOGIC
        JsonLogic_Handle str_data        = jsonlogic_string_from_utf16_sized(JSONLOGIC_DATA, JSONLOGIC_DATA_SIZE);
        JsonLogic_Handle reduce_context  = jsonlogic_object_from(
            jsonlogic_entry(str_accumulator, JsonLogic_Null),
            jsonlogic_entry(str_current,     JsonLogic_Null),
            jsonlogic_entry(str_data,        input)
        );
#else
        JsonLogic_Handle reduce_context  = jsonlogic_object_from(
            jsonlogic_entry(str_accumulator, JsonLogic_Null),
            jsonlogic_entry(str_current,     JsonLogic_Null)
        );
#endif
        jsonlogic_decref(str_accumulator);
        jsonlogic_decref(str_current);
#ifdef JSONLOGIC_COMPILE_CERTLOGIC
        jsonlogic_decref(str_data);
#endif
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
    }
#ifndef JSONLOGIC_COMPILE_CERTLOGIC
    else if (JSONLOGIC_IS_OP(opstr, ALL)) {
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
#endif

    if (opstr->hash == JSONLOGIC_HASH_UNSET) {
        opstr->hash = jsonlogic_hash_fnv1a_utf16(opstr->str, opstr->size);
    }

    const JsonLogic_Operation *opptr = jsonlogic_operations_get_with_hash(
        operations, opstr->hash, opstr->str, opstr->size);

    if (opptr == NULL) {
        // JSONLOGIC_DEBUG_UTF16("%s", opstr->str, opstr->size, "illegal operation");
        // jsonlogic_operations_debug(operations);
        return JsonLogic_Error_IllegalOperation;
    }

    JsonLogic_Handle argbuf[JSONLOGIC_STATIC_ARGC];
    JsonLogic_Handle *args;

    if (value_count > JSONLOGIC_STATIC_ARGC) {
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

    if (value_count > JSONLOGIC_STATIC_ARGC) {
        free(args);
    }

    return result;
}

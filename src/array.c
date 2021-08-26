#include "jsonlogic_intern.h"

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

JsonLogic_Handle jsonlogic_empty_array() {
    JsonLogic_Array *array = malloc(sizeof(JsonLogic_Array) - sizeof(JsonLogic_Handle));
    if (array == NULL) {
        return JsonLogic_Null;
    }

    array->refcount = 1;
    array->size     = 0;

    return (JsonLogic_Handle){ .intptr = ((uintptr_t)array) | JsonLogic_Type_Array };
}

JsonLogic_Array *jsonlogic_array_with_capacity(size_t size) {
    JsonLogic_Array *array = malloc(sizeof(JsonLogic_Array) - sizeof(JsonLogic_Handle) + sizeof(JsonLogic_Handle) * size);
    if (array == NULL) {
        return NULL;
    }

    array->refcount = 1;
    array->size     = 0;

    for (size_t index = 0; index < size; ++ index) {
        array->items[index] = JsonLogic_Null;
    }

    return array;
}

JsonLogic_Handle jsonlogic_array_from_vararg(size_t count, ...) {
    JsonLogic_Array *array = malloc(sizeof(JsonLogic_Array) - sizeof(JsonLogic_Handle) + sizeof(JsonLogic_Handle) * count);
    if (array == NULL) {
        return JsonLogic_Null;
    }

    array->refcount = 1;
    array->size     = count;

    va_list ap;
    va_start(ap, count);

    for (size_t index = 0; index < count; ++ index) {
        JsonLogic_Handle item = va_arg(ap, JsonLogic_Handle);
        jsonlogic_incref(item);
        array->items[index] = item;
    }

    va_end(ap);

    return (JsonLogic_Handle){ .intptr = ((uintptr_t)array) | JsonLogic_Type_Array };
}

void jsonlogic_array_free(JsonLogic_Array *array) {
    for (size_t index = 0; index < array->size; ++ index) {
        jsonlogic_decref(array->items[index]);
    }
    free(array);
}

JsonLogic_Handle jsonlogic_includes(JsonLogic_Handle list, JsonLogic_Handle item) {
    switch (list.intptr & JsonLogic_TypeMask) {
        case JsonLogic_Type_Array:
        {
            const JsonLogic_Array *array = JSONLOGIC_CAST_ARRAY(list);
            for (size_t index = 0; index < array->size; ++ index) {
                if (JSONLOGIC_IS_TRUE(jsonlogic_strict_equal(item, array->items[index]))) {
                    return JsonLogic_True;
                }
            }
            return JsonLogic_False;
        }
        case JsonLogic_Type_String:
        {
            const JsonLogic_String *string = JSONLOGIC_CAST_STRING(list);
            const JsonLogic_Handle needle = jsonlogic_to_string(item);
            if (!JSONLOGIC_IS_STRING(needle)) {
                // memory allocation failed
                assert(false);
                jsonlogic_decref(needle);
                return JsonLogic_False;
            }
            const JsonLogic_String *strneedle = JSONLOGIC_CAST_STRING(needle);
            // there are much more efficient string search algorithms
            for (size_t index = 0; index + strneedle->size < string->size; ++ index) {
                if (memcmp(strneedle->str, string->str + index, strneedle->size * sizeof(JsonLogic_Char)) == 0) {
                    jsonlogic_decref(needle);
                    return JsonLogic_True;
                }
            }
            jsonlogic_decref(needle);
            return JsonLogic_False;
        }
        default:
            return JsonLogic_False;
    }
}

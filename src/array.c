#include "jsonlogic_intern.h"

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

JsonLogic_Handle jsonlogic_empty_array() {
    JsonLogic_Array *array = malloc(sizeof(JsonLogic_Array) - sizeof(JsonLogic_Handle));
    if (array == NULL) {
        JSONLOGIC_ERROR_MEMORY();
        return JsonLogic_Null;
    }

    array->refcount = 1;
    array->size     = 0;

    return (JsonLogic_Handle){ .intptr = ((uintptr_t)array) | JsonLogic_Type_Array };
}

JsonLogic_Array *jsonlogic_array_with_capacity(size_t size) {
    JsonLogic_Array *array = malloc(sizeof(JsonLogic_Array) - sizeof(JsonLogic_Handle) + sizeof(JsonLogic_Handle) * size);
    if (array == NULL) {
        JSONLOGIC_ERROR_MEMORY();
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
        JSONLOGIC_ERROR_MEMORY();
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
    if (array != NULL) {
        for (size_t index = 0; index < array->size; ++ index) {
            jsonlogic_decref(array->items[index]);
        }
        free(array);
    }
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
            if (JSONLOGIC_IS_ERROR(needle)) {
                return needle;
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
        case JsonLogic_Type_Error:
            return list;

        default:
            return JsonLogic_False;
    }
}

JsonLogic_Array *jsonlogic_array_truncate(JsonLogic_Array *array, size_t size) {
    assert(array->refcount < 2);

    if (size < array->size) {
        for (size_t index = size; index < array->size; ++ index) {
            jsonlogic_decref(array->items[index]);
            array->items[index] = JsonLogic_Null;
        }

        JsonLogic_Array *new_array = realloc(array, sizeof(JsonLogic_Array) - sizeof(JsonLogic_Handle) + sizeof(JsonLogic_Handle) * size);
        if (new_array == NULL) {
            JSONLOGIC_DEBUG("%s", "shrinking realloc() failed");
        } else {
            array = new_array;
        }

        array->size = size;
    }

    return array;
}

JsonLogic_Error jsonlogic_arraybuf_append(JsonLogic_ArrayBuf *buf, JsonLogic_Handle handle) {
    if (buf->capacity == 0 || buf->capacity == buf->array->size) {
        size_t new_capacity = buf->capacity + JSONLOGIC_CHUNK_SIZE;
        JsonLogic_Array *new_array = realloc(buf->array, sizeof(JsonLogic_Array) - sizeof(JsonLogic_Handle) + sizeof(JsonLogic_Handle) * new_capacity);
        if (new_array == NULL) {
            JSONLOGIC_ERROR_MEMORY();
            return JSONLOGIC_ERROR_OUT_OF_MEMORY;
        }
        buf->array    = new_array;
        buf->capacity = new_capacity;
    }
    buf->array->items[buf->array->size] = jsonlogic_incref(handle);
    ++ buf->array->size;
    return JSONLOGIC_ERROR_SUCCESS;
}

JsonLogic_Array *jsonlogic_arraybuf_take(JsonLogic_ArrayBuf *buf) {
    JsonLogic_Array *array = buf->array;
    if (array == NULL) {
        array = malloc(sizeof(JsonLogic_Array) - sizeof(JsonLogic_Handle));
        if (array == NULL) {
            JSONLOGIC_ERROR_MEMORY();
        } else {
            array->refcount = 1;
            array->size     = 0;
        }
    } else {
        // shrink to fit
        array = realloc(array, sizeof(JsonLogic_Array) - sizeof(JsonLogic_Handle) + sizeof(JsonLogic_Handle) * array->size);
        if (array == NULL) {
            // should not happen
            JSONLOGIC_ERROR_MEMORY();
            array = buf->array;
        }
    }
    buf->capacity = 0;
    buf->array = NULL;
    return array;
}

void jsonlogic_arraybuf_free(JsonLogic_ArrayBuf *buf) {
    jsonlogic_array_free(buf->array);
    buf->array    = NULL;
    buf->capacity = 0;
}

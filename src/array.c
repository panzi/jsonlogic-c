#include "jsonlogic_intern.h"

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

JsonLogic_Handle jsonlogic_array_into_handle(JsonLogic_Array *array);

JsonLogic_Handle jsonlogic_empty_array() {
    JsonLogic_Array *array = JSONLOGIC_MALLOC_EMPTY_ARRAY();
    if (array == NULL) {
        JSONLOGIC_ERROR_MEMORY();
        return JsonLogic_Error_OutOfMemory;
    }

    array->refcount = 1;
    array->size     = 0;

    return jsonlogic_array_into_handle(array);
}

JsonLogic_Array *jsonlogic_array_with_capacity(size_t size) {
    JsonLogic_Array *array = JSONLOGIC_MALLOC_ARRAY(size);
    if (array == NULL) {
        JSONLOGIC_ERROR_MEMORY();
        return NULL;
    }

    array->refcount = 1;
    array->size     = size;

    for (size_t index = 0; index < size; ++ index) {
        array->items[index] = JsonLogic_Null;
    }

    return array;
}

JsonLogic_Handle jsonlogic_array_from_vararg(size_t count, ...) {
    JsonLogic_Array *array = JSONLOGIC_MALLOC_ARRAY(count);
    if (array == NULL) {
        JSONLOGIC_ERROR_MEMORY();
        return JsonLogic_Error_OutOfMemory;
    }

    array->refcount = 1;
    array->size     = count;

    va_list ap;
    va_start(ap, count);

    for (size_t index = 0; index < count; ++ index) {
        JsonLogic_Handle item = va_arg(ap, JsonLogic_Handle);
        if (JSONLOGIC_IS_ERROR(item)) {
            for (size_t free_index = 0; free_index < index; ++ free_index) {
                jsonlogic_decref(array->items[free_index]);
            }
            free(array);
            return item;
        }
        array->items[index] = jsonlogic_incref(item);
    }

    va_end(ap);

    return jsonlogic_array_into_handle(array);
}

JsonLogic_Handle jsonlogic_array_from(const JsonLogic_Handle items[], size_t size) {
    JsonLogic_Array *array = JSONLOGIC_MALLOC_ARRAY(size);
    if (array == NULL) {
        JSONLOGIC_ERROR_MEMORY();
        return JsonLogic_Error_OutOfMemory;
    }

    array->refcount = 1;
    array->size     = size;

    for (size_t index = 0; index < size; ++ index) {
        JsonLogic_Handle item = items[index];
        if (JSONLOGIC_IS_ERROR(item)) {
            for (size_t free_index = 0; free_index < index; ++ free_index) {
                jsonlogic_decref(array->items[free_index]);
            }
            free(array);
            return item;
        }
        array->items[index] = jsonlogic_incref(item);
    }

    return jsonlogic_array_into_handle(array);
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
    if (JSONLOGIC_IS_ERROR(item)) {
        return item;
    }

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
                if (memcmp(strneedle->str, string->str + index, strneedle->size * sizeof(char16_t)) == 0) {
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

        JsonLogic_Array *new_array = JSONLOGIC_REALLOC_ARRAY(array, size);
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
    if (JSONLOGIC_IS_ERROR(handle)) {
        return handle.intptr;
    }

    if (buf->capacity == 0 || buf->capacity == buf->array->size) {
        size_t new_capacity = buf->capacity + 1024;
        JsonLogic_Array *new_array = JSONLOGIC_REALLOC_ARRAY(buf->array, new_capacity);
        if (new_array == NULL) {
            JSONLOGIC_ERROR_MEMORY();
            return JSONLOGIC_ERROR_OUT_OF_MEMORY;
        }
        if (buf->array == NULL) {
            new_array->refcount = 1;
            new_array->size     = 0;
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
        array = JSONLOGIC_MALLOC_EMPTY_ARRAY();
        if (array == NULL) {
            JSONLOGIC_ERROR_MEMORY();
        } else {
            array->refcount = 1;
            array->size     = 0;
        }
    } else {
        // shrink to fit
        array = JSONLOGIC_REALLOC_ARRAY(array, array->size);
        if (array == NULL) {
            // should not happen
            JSONLOGIC_ERROR_MEMORY();
            array = buf->array;
        }
        assert(array->refcount == 1);
    }
    buf->capacity = 0;
    buf->array = NULL;
    return array;
}

void jsonlogic_arraybuf_clear(JsonLogic_ArrayBuf *buf) {
    JsonLogic_Array *array = buf->array;
    if (array != NULL) {
        for (size_t index = 0; index < array->size; ++ index) {
            jsonlogic_decref(array->items[index]);
        }
        array->size = 0;
    }
}

void jsonlogic_arraybuf_free(JsonLogic_ArrayBuf *buf) {
    jsonlogic_array_free(buf->array);
    buf->array    = NULL;
    buf->capacity = 0;
}

JsonLogic_Handle jsonlogic_to_array(JsonLogic_Handle handle) {
    if (JSONLOGIC_IS_NUMBER(handle)) {
        return JsonLogic_Error_IllegalArgument;
    }

    switch (handle.intptr & JsonLogic_TypeMask) {
        case JsonLogic_Type_Array:
            return jsonlogic_incref(handle);

        case JsonLogic_Type_String:
        {
            const JsonLogic_String *string = JSONLOGIC_CAST_STRING(handle);
            size_t size = string->size;
            JsonLogic_Array *array = JSONLOGIC_MALLOC_ARRAY(size);
            if (array == NULL) {
                JSONLOGIC_ERROR_MEMORY();
                return JsonLogic_Error_OutOfMemory;
            }
            for (size_t index = 0; index < size; ++ index) {
                JsonLogic_Handle item = jsonlogic_string_from_utf16_sized(string->str + index, 1);
                if (JSONLOGIC_IS_ERROR(item)) {
                    for (size_t free_index = 0; free_index < index; ++ index) {
                        jsonlogic_decref(array->items[free_index]);
                    }
                    free(array);
                    return item;
                }
                array->items[index] = item;
            }
            return jsonlogic_array_into_handle(array);
        }
        case JsonLogic_Type_Object:
        {
            const JsonLogic_Object *object = JSONLOGIC_CAST_OBJECT(handle);
            size_t size = object->used;
            JsonLogic_Array *array = JSONLOGIC_MALLOC_ARRAY(size);
            if (array == NULL) {
                JSONLOGIC_ERROR_MEMORY();
                return JsonLogic_Error_OutOfMemory;
            }
            size_t array_index = 0;
            for (size_t obj_index = object->first_index; obj_index < object->size; ++ obj_index) {
                const JsonLogic_Object_Entry *entry = &object->entries[obj_index];
                if (!JSONLOGIC_IS_NULL(entry->key)) {
                    array->items[array_index ++] = jsonlogic_incref(entry->key);
                }
            }
            return jsonlogic_array_into_handle(array);
        }
        case JsonLogic_Type_Error:
            return handle;

        default:
            return JsonLogic_Error_IllegalArgument;
    }
}

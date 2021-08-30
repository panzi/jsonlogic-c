#include "jsonlogic_intern.h"

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <math.h>

JsonLogic_Handle jsonlogic_object_into_handle(JsonLogic_Object *object);

JsonLogic_Handle jsonlogic_empty_object() {
    JsonLogic_Object *object = malloc(sizeof(JsonLogic_Object) - sizeof(JsonLogic_Object_Entry));
    if (object == NULL) {
        JSONLOGIC_ERROR_MEMORY();
        return JsonLogic_Error_OutOfMemory;
    }

    object->refcount = 1;
    object->size     = 0;

    return jsonlogic_object_into_handle(object);
}

void jsonlogic_object_free(JsonLogic_Object *object) {
    if (object != NULL) {
        for (size_t index = 0; index < object->size; ++ index) {
            JsonLogic_Object_Entry *entry = &object->entries[index];
            jsonlogic_decref(entry->key);
            jsonlogic_decref(entry->value);
        }
        free(object);
    }
}

int jsonlogic_object_entry_compary(const void *leftptr, const void *rightptr) {
    const JsonLogic_Object_Entry *left  = (const JsonLogic_Object_Entry *)leftptr;
    const JsonLogic_Object_Entry *right = (const JsonLogic_Object_Entry *)rightptr;

    return jsonlogic_string_compare(
        JSONLOGIC_CAST_STRING(left->key),
        JSONLOGIC_CAST_STRING(right->key)
    );
}

JsonLogic_Handle jsonlogic_object_from_vararg(size_t count, ...) {
    JsonLogic_Object *object = malloc(sizeof(JsonLogic_Object) - sizeof(JsonLogic_Object_Entry) + sizeof(JsonLogic_Object_Entry) * count);

    if (object == NULL) {
        JSONLOGIC_ERROR_MEMORY();
        return JsonLogic_Error_OutOfMemory;
    }

    object->refcount = 1;
    object->size     = count;

    va_list ap;
    va_start(ap, count);

    for (size_t index = 0; index < count; ++ index) {
        JsonLogic_Object_Entry entry = va_arg(ap, JsonLogic_Object_Entry);
        JsonLogic_Handle key = jsonlogic_to_string(entry.key);
        if (JSONLOGIC_IS_ERROR(key)) {
            for (size_t free_index = 0; free_index < index; ++ free_index) {
                JsonLogic_Object_Entry *entry = &object->entries[free_index];
                jsonlogic_decref(entry->key);
                jsonlogic_decref(entry->value);
            }
            free(object);
            va_end(ap);

            return key;
        }
        if (JSONLOGIC_IS_ERROR(entry.value)) {
            jsonlogic_decref(key);
            for (size_t free_index = 0; free_index < index; ++ free_index) {
                JsonLogic_Object_Entry *entry = &object->entries[free_index];
                jsonlogic_decref(entry->key);
                jsonlogic_decref(entry->value);
            }
            free(object);
            va_end(ap);

            return entry.value;
        }
        jsonlogic_incref(entry.value);
        object->entries[index] = (JsonLogic_Object_Entry){
            .key   = key,
            .value = entry.value,
        };
    }

    va_end(ap);

    qsort(object->entries, object->size,
        sizeof(JsonLogic_Object_Entry),
        jsonlogic_object_entry_compary);

    // check uniquness
    if (count > 1) {
        const JsonLogic_String *prev = JSONLOGIC_CAST_STRING(object->entries[0].key);
        for (size_t index = 1; index < count; ++ index) {
            const JsonLogic_String *key = JSONLOGIC_CAST_STRING(object->entries[index].key);
            if (jsonlogic_string_equals(prev, key)) {
                jsonlogic_object_free(object);
                JSONLOGIC_ERROR("%s", "duplicated key in object");
                return JsonLogic_Error_IllegalArgument;
            }
            prev = key;
        }
    }

    return jsonlogic_object_into_handle(object);
}

JSONLOGIC_DEF_UTF16(JSONLOGIC_LENGTH, u"length")

JsonLogic_Handle jsonlogic_get_utf16(JsonLogic_Handle object, const char16_t *key);

JsonLogic_Handle jsonlogic_get_utf16_sized(JsonLogic_Handle handle, const char16_t *key, size_t size) {
    if (JSONLOGIC_IS_NUMBER(handle)) {
        return JsonLogic_Null;
    }

    switch (handle.intptr & JsonLogic_TypeMask) {
        case JsonLogic_Type_String:
        {
            if (jsonlogic_utf16_equals(key, size, JSONLOGIC_LENGTH, JSONLOGIC_LENGTH_SIZE)) {
                return jsonlogic_number_from(JSONLOGIC_CAST_STRING(handle)->size);
            }
            size_t index = jsonlogic_utf16_to_index(key, size);
            const JsonLogic_String *string = JSONLOGIC_CAST_STRING(handle);
            if (index < string->size) {
                return jsonlogic_string_from_utf16_sized(string->str + index, 1);
            }
            return JsonLogic_Null;
        }
        case JsonLogic_Type_Array:
        {
            if (jsonlogic_utf16_equals(key, size, JSONLOGIC_LENGTH, JSONLOGIC_LENGTH_SIZE)) {
                return jsonlogic_number_from(JSONLOGIC_CAST_STRING(handle)->size);
            }
            size_t index = jsonlogic_utf16_to_index(key, size);
            const JsonLogic_Array *array = JSONLOGIC_CAST_ARRAY(handle);
            if (index < array->size) {
                JsonLogic_Handle item = array->items[index];
                return jsonlogic_incref(item);
            }
            return JsonLogic_Null;
        }
        case JsonLogic_Type_Object:
        {
            JsonLogic_Object *object = JSONLOGIC_CAST_OBJECT(handle);
            size_t index = jsonlogic_object_get_index_utf16(object, key, size);
            if (index >= object->size) {
                return JsonLogic_Null;
            }
            return jsonlogic_incref(object->entries[index].value);
        }
        case JsonLogic_Type_Boolean:
        case JsonLogic_Type_Number:
        case JsonLogic_Type_Null:
            return JsonLogic_Null;

        case JsonLogic_Type_Error:
            return handle;

        default:
            assert(false);
            return JsonLogic_Error_InternalError;
    }
}

JsonLogic_Handle jsonlogic_get_index(JsonLogic_Handle handle, size_t index) {
    if (JSONLOGIC_IS_NUMBER(handle)) {
        return JsonLogic_Null;
    }

    switch (handle.intptr & JsonLogic_TypeMask) {
        case JsonLogic_Type_String:
        {
            const JsonLogic_String *string = JSONLOGIC_CAST_STRING(handle);
            if (index >= string->size) {
                return JsonLogic_Null;
            }

            return jsonlogic_string_from_utf16_sized(string->str + index, 1);
        }
        case JsonLogic_Type_Array:
        {
            const JsonLogic_Array *array = JSONLOGIC_CAST_ARRAY(handle);
            if (index >= array->size) {
                return JsonLogic_Null;
            }

            return jsonlogic_incref(array->items[index]);
        }
        case JsonLogic_Type_Object:
        {
            JsonLogic_Handle strkey = jsonlogic_to_string(jsonlogic_number_from(index));
            if (JSONLOGIC_IS_ERROR(strkey)) {
                return strkey;
            }
            JsonLogic_String *stringkey = JSONLOGIC_CAST_STRING(strkey);
            JsonLogic_Handle result = jsonlogic_get_utf16_sized(handle, stringkey->str, stringkey->size);
            jsonlogic_decref(strkey);
            return result;
        }
        case JsonLogic_Type_Error:
            return handle;

        default:
            return JsonLogic_Null;
    }
}

JsonLogic_Handle jsonlogic_get(JsonLogic_Handle handle, JsonLogic_Handle key) {
    if (JSONLOGIC_IS_NUMBER(handle)) {
        return JsonLogic_Null;
    }

    if (JSONLOGIC_IS_NUMBER(key)) {
        switch (handle.intptr & JsonLogic_TypeMask) {
            case JsonLogic_Type_String:
            {
                if (!isfinite(key.number) || key.number < 0) {
                    return JsonLogic_Null;
                }

                size_t index = key.number;
                const JsonLogic_String *string = JSONLOGIC_CAST_STRING(handle);
                if (index >= string->size) {
                    return JsonLogic_Null;
                }

                return jsonlogic_string_from_utf16_sized(string->str + index, 1);
            }
            case JsonLogic_Type_Array:
            {
                if (!isfinite(key.number) || key.number < 0) {
                    return JsonLogic_Null;
                }

                size_t index = key.number;
                const JsonLogic_Array *array = JSONLOGIC_CAST_ARRAY(handle);
                if (index >= array->size) {
                    return JsonLogic_Null;
                }

                return jsonlogic_incref(array->items[index]);
            }
        }
    }

    switch (handle.intptr & JsonLogic_TypeMask) {
        case JsonLogic_Type_String:
        case JsonLogic_Type_Array:
        case JsonLogic_Type_Object:
        {
            JsonLogic_Handle strkey = jsonlogic_to_string(key);
            if (JSONLOGIC_IS_ERROR(strkey)) {
                return strkey;
            }
            JsonLogic_String *stringkey = JSONLOGIC_CAST_STRING(strkey);
            JsonLogic_Handle result = jsonlogic_get_utf16_sized(handle, stringkey->str, stringkey->size);
            jsonlogic_decref(strkey);
            return result;
        }
        case JsonLogic_Type_Error:
            return handle;

        default:
            return JsonLogic_Null;
    }
}

size_t jsonlogic_object_get_index_utf16(JsonLogic_Object *object, const char16_t *key, size_t key_size) {
    if (object->size == 0) {
        return 0;
    }
    size_t left  = 0;
    size_t right = object->size;
    while (left < right) {
        size_t mid = (left + right - 1) / 2;
        const JsonLogic_String *entrykey = JSONLOGIC_CAST_STRING(object->entries[mid].key);
        int cmp = jsonlogic_utf16_compare(
            entrykey->str,
            entrykey->size,
            key,
            key_size);
        if (cmp < 0) {
            left = mid + 1;
        } else if (cmp > 0) {
            right = mid;
        } else {
            return left;
        }
    }
    return object->size;
}

size_t jsonlogic_object_get_index(JsonLogic_Object *object, JsonLogic_Handle key) {
    if (object->size == 0) {
        return 0;
    }
    JsonLogic_Handle strkey = jsonlogic_to_string(key);
    if (JSONLOGIC_IS_ERROR(strkey)) {
        return SIZE_MAX;
    }
    const JsonLogic_String *stringkey = JSONLOGIC_CAST_STRING(strkey);
    const size_t index = jsonlogic_object_get_index_utf16(object, stringkey->str, stringkey->size);
    jsonlogic_decref(strkey);
    return index;
}

JsonLogic_Error jsonlogic_objbuf_set(JsonLogic_ObjBuf *buf, JsonLogic_Handle key, JsonLogic_Handle value) {
    JsonLogic_Handle stringkey = jsonlogic_to_string(key);
    if (JSONLOGIC_IS_ERROR(stringkey)) {
        return stringkey.intptr;
    }

    const JsonLogic_String *strkey = JSONLOGIC_CAST_STRING(stringkey);
    JsonLogic_Object_Entry *entry;

    if (buf->capacity == 0) {
        size_t new_capacity = JSONLOGIC_CHUNK_SIZE;
        JsonLogic_Object *new_object = malloc(sizeof(JsonLogic_Object) - sizeof(JsonLogic_Object_Entry) + sizeof(JsonLogic_Object_Entry) * new_capacity);
        if (new_object == NULL) {
            jsonlogic_decref(stringkey);
            JSONLOGIC_ERROR_MEMORY();
            return JSONLOGIC_ERROR_OUT_OF_MEMORY;
        }
        new_object->refcount = 1;
        new_object->size     = 0;

        buf->object   = new_object;
        buf->capacity = new_capacity;

        entry = new_object->entries;
    } else {
        JsonLogic_Object *object = buf->object;
        size_t left  = 0;
        size_t right = object->size;
        while (left < right) {
            size_t mid = (left + right - 1) / 2;
            entry = &object->entries[mid];
            const JsonLogic_String *entrykey = JSONLOGIC_CAST_STRING(entry->key);
            int cmp = jsonlogic_utf16_compare(
                entrykey->str,
                entrykey->size,
                strkey->str,
                strkey->size);
            if (cmp < 0) {
                left  = mid + 1;
            } else if (cmp > 0) {
                right = mid;
            } else {
                jsonlogic_incref(value);
                jsonlogic_decref(stringkey);
                jsonlogic_decref(entry->value);
                entry->value = value;
                return JSONLOGIC_ERROR_SUCCESS;
            }
        }

        assert(left <= object->size);

        if (buf->capacity == object->size) {
            size_t new_capacity = buf->capacity + JSONLOGIC_CHUNK_SIZE;
            JsonLogic_Object *new_object = realloc(object, sizeof(JsonLogic_Object) - sizeof(JsonLogic_Object_Entry) + sizeof(JsonLogic_Object_Entry) * new_capacity);
            if (new_object == NULL) {
                jsonlogic_decref(stringkey);
                JSONLOGIC_ERROR_MEMORY();
                return JSONLOGIC_ERROR_OUT_OF_MEMORY;
            }
            buf->object   = new_object;
            buf->capacity = new_capacity;
        }
        entry = &buf->object->entries[left];

        memmove(entry + 1, entry, sizeof(JsonLogic_Object_Entry) * (buf->object->size - left));
    }

    buf->object->size ++;

    entry->key   = stringkey;
    entry->value = jsonlogic_incref(value);

    return JSONLOGIC_ERROR_SUCCESS;
}

JsonLogic_Object *jsonlogic_objbuf_take(JsonLogic_ObjBuf *buf) {
    JsonLogic_Object *object = buf->object;
    if (object == NULL) {
        object = malloc(sizeof(JsonLogic_Object) - sizeof(JsonLogic_Object_Entry));
        if (object == NULL) {
            JSONLOGIC_ERROR_MEMORY();
        } else {
            object->refcount = 1;
            object->size     = 0;
        }
    } else {
        // shrink to fit
        object = realloc(object, sizeof(JsonLogic_Object) - sizeof(JsonLogic_Object_Entry) + sizeof(JsonLogic_Object_Entry) * object->size);
        if (object == NULL) {
            // should not happen
            JSONLOGIC_ERROR_MEMORY();
            object = buf->object;
        }
    }
    buf->capacity = 0;
    buf->object   = NULL;
    return object;
}

void jsonlogic_objbuf_free(JsonLogic_ObjBuf *buf) {
    jsonlogic_object_free(buf->object);
    buf->object   = NULL;
    buf->capacity = 0;
}

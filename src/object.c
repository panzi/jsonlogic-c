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
    object->used     = 0;
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
    JsonLogic_ObjBuf buf = JSONLOGIC_OBJBUF_INIT;

    va_list ap;
    va_start(ap, count);

    for (size_t index = 0; index < count; ++ index) {
        JsonLogic_Object_Entry entry = va_arg(ap, JsonLogic_Object_Entry);
        JsonLogic_Error error = jsonlogic_objbuf_set(&buf, entry.key, entry.value);
        if (error != JSONLOGIC_ERROR_SUCCESS) {
            jsonlogic_objbuf_free(&buf);
            return (JsonLogic_Handle){ .intptr = error };
        }
    }

    va_end(ap);

    return jsonlogic_object_into_handle(jsonlogic_objbuf_take(&buf));
}

JSONLOGIC_DEF_UTF16(JSONLOGIC_LENGTH, u"length")

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

size_t jsonlogic_object_get_index_utf16_with_hash(const JsonLogic_Object *object, uint64_t hash, const char16_t *key, size_t key_size) {
    if (object->size == 0) {
        return 0;
    }
    size_t size = object->size;
    size_t index = hash % size;
    size_t start_index = index;

    do {
        const JsonLogic_Object_Entry *entry = &object->entries[index];

        if (JSONLOGIC_IS_NULL(entry->key)) {
            break;
        }

        const JsonLogic_String *otherkey = JSONLOGIC_CAST_STRING(entry->key);
        if (jsonlogic_utf16_equals(key, key_size, otherkey->str, otherkey->size)) {
            return index;
        }

        index = (index + 1) % size;
    } while (index != start_index);

    return object->size;
}

size_t jsonlogic_object_get_index_utf16(const JsonLogic_Object *object, const char16_t *key, size_t key_size) {
    return jsonlogic_object_get_index_utf16_with_hash(object, jsonlogic_hash_fnv1a_utf16(key, key_size), key, key_size);
}

size_t jsonlogic_object_get_index(const JsonLogic_Object *object, JsonLogic_Handle key) {
    if (object->size == 0) {
        return 0;
    }

    JsonLogic_Handle keyhandle = jsonlogic_to_string(key);
    if (JSONLOGIC_IS_ERROR(keyhandle)) {
        return SIZE_MAX;
    }

    JsonLogic_String *stringkey = JSONLOGIC_CAST_STRING(keyhandle);
    if (stringkey->hash == JSONLOGIC_HASH_UNSET) {
        stringkey->hash = jsonlogic_hash_fnv1a_utf16(stringkey->str, stringkey->size);
    }
    size_t index = jsonlogic_object_get_index_utf16_with_hash(object, stringkey->hash, stringkey->str, stringkey->size);
    jsonlogic_decref(keyhandle);
    return index;
}

JsonLogic_Error jsonlogic_objbuf_set(JsonLogic_ObjBuf *buf, JsonLogic_Handle key, JsonLogic_Handle value) {
    JsonLogic_Handle stringkey = jsonlogic_to_string(key);
    if (JSONLOGIC_IS_ERROR(stringkey)) {
        return stringkey.intptr;
    }

    JsonLogic_String *strkey = JSONLOGIC_CAST_STRING(stringkey);

    if (strkey->hash == JSONLOGIC_HASH_UNSET) {
        strkey->hash = jsonlogic_hash_fnv1a_utf16(strkey->str, strkey->size);
    }

    uint64_t hash = strkey->hash;
    if (buf->object == NULL) {
        size_t new_size = 4;
        JsonLogic_Object *new_object = malloc(sizeof(JsonLogic_Object) - sizeof(JsonLogic_Object_Entry) + sizeof(JsonLogic_Object_Entry) * new_size);
        if (new_object == NULL) {
            jsonlogic_decref(stringkey);
            JSONLOGIC_ERROR_MEMORY();
            return JSONLOGIC_ERROR_OUT_OF_MEMORY;
        }
        new_object->refcount = 1;
        new_object->used     = 1;
        new_object->size     = new_size;

        for (size_t index = 0; index < new_size; ++ index) {
            new_object->entries[index] = (JsonLogic_Object_Entry) {
                .key   = JsonLogic_Null,
                .value = JsonLogic_Null,
            };
        }

        size_t index = hash % new_size;
        new_object->entries[index] = (JsonLogic_Object_Entry) {
            .key   = stringkey,
            .value = jsonlogic_incref(value),
        };

        buf->object = new_object;
    } else {
        JsonLogic_Object *object = buf->object;
        size_t size = object->size;
        size_t index = hash % size;
        size_t start_index = index;
        do {
            JsonLogic_Object_Entry *entry = &object->entries[index];

            if (JSONLOGIC_IS_NULL(entry->key)) {
                assert(JSONLOGIC_IS_NULL(entry->value));
                if (object->used + 1 > size / 2) {
                    // resize and insert instead
                    break;
                }
                *entry = (JsonLogic_Object_Entry) {
                    .key   = stringkey,
                    .value = jsonlogic_incref(value),
                };

                ++ object->used;
                return JSONLOGIC_ERROR_SUCCESS;
            }

            const JsonLogic_String *otherkey = JSONLOGIC_CAST_STRING(entry->key);
            if (jsonlogic_utf16_equals(strkey->str, strkey->size, otherkey->str, otherkey->size)) {
                jsonlogic_decref(entry->key);
                jsonlogic_decref(entry->value);

                *entry = (JsonLogic_Object_Entry) {
                    .key   = stringkey,
                    .value = jsonlogic_incref(value),
                };
                return JSONLOGIC_ERROR_SUCCESS;
            }

            index = (index + 1) % size;
        } while (index != start_index);

        size_t new_size = size * 2;
        JsonLogic_Object *new_object = malloc(sizeof(JsonLogic_Object) - sizeof(JsonLogic_Object_Entry) + sizeof(JsonLogic_Object_Entry) * new_size);
        if (new_object == NULL) {
            jsonlogic_decref(stringkey);
            JSONLOGIC_ERROR_MEMORY();
            return JSONLOGIC_ERROR_OUT_OF_MEMORY;
        }
        new_object->refcount = 1;
        new_object->used     = object->used;
        new_object->size     = new_size;

        for (size_t index = 0; index < new_size; ++ index) {
            new_object->entries[index] = (JsonLogic_Object_Entry) {
                .key   = JsonLogic_Null,
                .value = JsonLogic_Null,
            };
        }

        // move old entries to new hash-table
        for (size_t index = 0; index < size; ++ index) {
            JsonLogic_Object_Entry *entry = &object->entries[index];

            if (!JSONLOGIC_IS_NULL(entry->key)) {
                const JsonLogic_String *otherkey = JSONLOGIC_CAST_STRING(entry->key);
                size_t new_index = otherkey->hash % new_size;
                JSONLOGIC_DEBUG_CODE(size_t start_index = new_index;)

                for (;;) {
                    JsonLogic_Object_Entry *new_entry = &new_object->entries[new_index];
                    if (JSONLOGIC_IS_NULL(new_entry->key)) {
                        new_object->entries[new_index] = *entry;
                        break;
                    }

                    new_index = (new_index + 1) % new_size;
                    assert(new_index != start_index);
                }
            }
        }

        // add new entry
        index = strkey->hash % new_size;
        JSONLOGIC_DEBUG_CODE(start_index = index;)
        for (;;) {
            JsonLogic_Object_Entry *entry = &new_object->entries[index];
            if (JSONLOGIC_IS_NULL(entry->key)) {
                assert(JSONLOGIC_IS_NULL(entry->value));
                *entry = (JsonLogic_Object_Entry) {
                    .key   = stringkey,
                    .value = jsonlogic_incref(value),
                };
                break;
            }

            const JsonLogic_String *otherkey = JSONLOGIC_CAST_STRING(entry->key);
            if (jsonlogic_utf16_equals(strkey->str, strkey->size, otherkey->str, otherkey->size)) {
                jsonlogic_decref(entry->key);
                jsonlogic_decref(entry->value);

                *entry = (JsonLogic_Object_Entry) {
                    .key   = stringkey,
                    .value = jsonlogic_incref(value),
                };
                break;
            }

            index = (index + 1) % new_size;
            assert(index != start_index);
        }

        free(object);
        buf->object = new_object;
        ++ new_object->used;
    }

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
            object->used     = 0;
            object->size     = 0;
        }
    }
    buf->object = NULL;
    return object;
}

void jsonlogic_objbuf_free(JsonLogic_ObjBuf *buf) {
    jsonlogic_object_free(buf->object);
    buf->object = NULL;
}

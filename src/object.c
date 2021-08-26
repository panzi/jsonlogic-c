#include "jsonlogic_intern.h"

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

JsonLogic_Handle jsonlogic_empty_object() {
    JsonLogic_Object *object = malloc(sizeof(JsonLogic_Object) - sizeof(JsonLogic_Object_Entry));
    if (object == NULL) {
        return JsonLogic_Null;
    }

    object->refcount = 1;
    object->size     = 0;

    return (JsonLogic_Handle){ .intptr = ((uintptr_t)object) | JsonLogic_Type_Object };
}

void jsonlogic_object_free(JsonLogic_Object *object) {
    for (size_t index = 0; index < object->size; ++ index) {
        JsonLogic_Object_Entry *entry = &object->entries[index];
        jsonlogic_decref(entry->key);
        jsonlogic_decref(entry->value);
    }
    free(object);
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
        return JsonLogic_Null;
    }

    object->refcount = 1;
    object->size     = count;

    va_list ap;
    va_start(ap, count);

    for (size_t index = 0; index < count; ++ index) {
        JsonLogic_Object_Entry entry = va_arg(ap, JsonLogic_Object_Entry);
        JsonLogic_Handle key = jsonlogic_to_string(entry.key);
        if (!JSONLOGIC_IS_STRING(key)) {
            jsonlogic_decref(key);
            for (size_t free_index = 0; free_index < index; ++ free_index) {
                JsonLogic_Object_Entry *entry = &object->entries[free_index];
                jsonlogic_decref(entry->key);
                jsonlogic_decref(entry->value);
            }
            free(object);
            va_end(ap);
            // memory allocation failed
            assert(false);
            return JsonLogic_Null;
        }
        jsonlogic_incref(entry.value);
        object->entries[index] = (JsonLogic_Object_Entry){
            .key   = key,
            .value = entry.value,
        };
    }

    va_end(ap);

    qsort(object->entries, object->size, sizeof(JsonLogic_Object_Entry), jsonlogic_object_entry_compary);

    // check uniquness
    if (count > 1) {
        const JsonLogic_String *prev = JSONLOGIC_CAST_STRING(object->entries[0].key);
        for (size_t index = 1; index < count; ++ index) {
            const JsonLogic_String *key = JSONLOGIC_CAST_STRING(object->entries[index].key);
            if (jsonlogic_string_equals(prev, key)) {
                jsonlogic_object_free(object);
                // since qsort() isn't stable, wouldn't know what to delete
                assert(false);
                return JsonLogic_Null;
            }
            prev = key;
        }
    }

    return (JsonLogic_Handle){ .intptr = ((uintptr_t)object) | JsonLogic_Type_Object };
}

JSONLOGIC_DECL_UTF16(JSONLOGIC_LENGTH, 'l', 'e', 'n', 'g', 't', 'h')

JsonLogic_Handle jsonlogic_get_item(JsonLogic_Handle handle, JsonLogic_Handle key) {
    if (JSONLOGIC_IS_NUMBER(handle)) {
        return JsonLogic_Null;
    }

    switch (handle.intptr & JsonLogic_TypeMask) {
        case JsonLogic_Type_String:
        {
            size_t index = SIZE_MAX;
            if (JSONLOGIC_IS_STRING(key)) {
                const JsonLogic_String *stringkey = JSONLOGIC_CAST_STRING(key);
                if (stringkey->size == JSONLOGIC_LENGTH_SIZE && memcmp(stringkey->str, JSONLOGIC_LENGTH, JSONLOGIC_LENGTH_SIZE) == 0) {
                    return jsonlogic_number_from(JSONLOGIC_CAST_STRING(handle)->size);
                }
                index = jsonlogic_string_to_index(stringkey);
            } else {
                JsonLogic_Handle strkey = jsonlogic_to_string(key);
                if (JSONLOGIC_IS_NULL(strkey)) {
                    return JsonLogic_Null;
                }
                const JsonLogic_String *stringkey = JSONLOGIC_CAST_STRING(strkey);
                if (stringkey->size == JSONLOGIC_LENGTH_SIZE && memcmp(stringkey->str, JSONLOGIC_LENGTH, JSONLOGIC_LENGTH_SIZE) == 0) {
                    jsonlogic_decref(strkey);
                    return jsonlogic_number_from(JSONLOGIC_CAST_STRING(handle)->size);
                }
                index = jsonlogic_string_to_index(stringkey);
                jsonlogic_decref(strkey);
            }
            const JsonLogic_String *string = JSONLOGIC_CAST_STRING(handle);
            if (index < string->size) {
                return jsonlogic_string_from_utf16_sized(string->str + index, 1);
            }
            return JsonLogic_Null;
        }
        case JsonLogic_Type_Array:
        {
            size_t index = SIZE_MAX;
            if (JSONLOGIC_IS_STRING(key)) {
                const JsonLogic_String *stringkey = JSONLOGIC_CAST_STRING(key);
                if (stringkey->size == JSONLOGIC_LENGTH_SIZE && memcmp(stringkey->str, JSONLOGIC_LENGTH, JSONLOGIC_LENGTH_SIZE) == 0) {
                    return jsonlogic_number_from(JSONLOGIC_CAST_STRING(handle)->size);
                }
                index = jsonlogic_string_to_index(stringkey);
            } else {
                JsonLogic_Handle strkey = jsonlogic_to_string(key);
                if (JSONLOGIC_IS_NULL(strkey)) {
                    return JsonLogic_Null;
                }
                const JsonLogic_String *stringkey = JSONLOGIC_CAST_STRING(strkey);
                if (stringkey->size == JSONLOGIC_LENGTH_SIZE && memcmp(stringkey->str, JSONLOGIC_LENGTH, JSONLOGIC_LENGTH_SIZE) == 0) {
                    jsonlogic_decref(strkey);
                    return jsonlogic_number_from(JSONLOGIC_CAST_ARRAY(handle)->size);
                }
                index = jsonlogic_string_to_index(stringkey);
                jsonlogic_decref(strkey);
            }
            const JsonLogic_Array *array = JSONLOGIC_CAST_ARRAY(handle);
            if (index < array->size) {
                JsonLogic_Handle item = array->items[index];
                jsonlogic_incref(item);
                return item;
            }
            return JsonLogic_Null;
        }
        case JsonLogic_Type_Object:
        {
            JsonLogic_Object *object = JSONLOGIC_CAST_OBJECT(handle);
            size_t index = jsonlogic_object_get_index(object, key);
            if (index >= object->size) {
                return JsonLogic_Null;
            }
            return jsonlogic_incref(object->entries[index].value);
        }
        default:
            return JsonLogic_Null;
    }
}

size_t jsonlogic_object_get_index_utf16(JsonLogic_Object *object, const JsonLogic_Char *key, size_t key_size) {
    size_t left  = 0;
    size_t right = object->size - 1;
    while (left != right) {
        // (x + y - 1) / y
        // ceiling integer division (assumes this all isn't overlowing)
        size_t mid = (left + right + 1) / 2;
        const JsonLogic_String *entrykey = JSONLOGIC_CAST_STRING(object->entries[mid].key);
        if (jsonlogic_utf16_compare(
                entrykey->str,
                entrykey->size,
                key,
                key_size) > 0) {
            right = mid - 1;
        } else {
            left = mid;
        }
    }
    const JsonLogic_Object_Entry *entry = &object->entries[left];
    const JsonLogic_String *entrykey = JSONLOGIC_CAST_STRING(entry->key);
    if (jsonlogic_utf16_equals(
            entrykey->str,
            entrykey->size,
            key,
            key_size)) {
        return left;
    }
    return object->size;
}

size_t jsonlogic_object_get_index(JsonLogic_Object *object, JsonLogic_Handle key) {
    if (object->size == 0) {
        return 0;
    }
    JsonLogic_Handle strkey = jsonlogic_to_string(key);
    if (!JSONLOGIC_IS_STRING(strkey)) {
        // memory allocation failed
        assert(false);
        return SIZE_MAX;
    }
    const JsonLogic_String *stringkey = JSONLOGIC_CAST_STRING(strkey);
    const size_t index = jsonlogic_object_get_index_utf16(object, stringkey->str, stringkey->size);
    jsonlogic_decref(strkey);
    return index;
}

#include "jsonlogic_intern.h"

#include <assert.h>

JsonLogic_Iterator jsonlogic_iter(JsonLogic_Handle handle);

JsonLogic_Handle jsonlogic_iter_next(JsonLogic_Iterator *iter) {
    assert(iter != NULL);

    JsonLogic_Handle handle = iter->handle;
    if (JSONLOGIC_IS_NUMBER(handle)) {
        return JsonLogic_Error_IllegalArgument;
    }

    switch (handle.intptr & JsonLogic_TypeMask) {
        case JsonLogic_Type_Array:
        {
            const JsonLogic_Array *array = JSONLOGIC_CAST_ARRAY(handle);
            if (iter->index >= array->size) {
                return JsonLogic_Error_StopIteration;
            }
            return jsonlogic_incref(array->items[iter->index ++]);
        }
        case JsonLogic_Type_Object:
        {
            const JsonLogic_Object *object = JSONLOGIC_CAST_OBJECT(handle);
            if (iter->index >= object->size) {
                return JsonLogic_Error_StopIteration;
            }
            return jsonlogic_incref(object->entries[iter->index ++].key);
        }
        case JsonLogic_Type_String:
        {
            const JsonLogic_String *string = JSONLOGIC_CAST_STRING(handle);
            if (iter->index >= string->size) {
                return JsonLogic_Error_StopIteration;
            }
            return jsonlogic_string_from_utf16_sized(&string->str[iter->index ++], 1);
        }
        case JsonLogic_Type_Error:
            return handle;

        case JsonLogic_Type_Boolean:
        case JsonLogic_Type_Null:
            return JsonLogic_Error_IllegalArgument;

        default:
            assert(false);
            return JsonLogic_Error_InternalError;
    }
}

void jsonlogic_iter_free(JsonLogic_Iterator *iter) {
    assert(iter != NULL);
    jsonlogic_decref(iter->handle);
}

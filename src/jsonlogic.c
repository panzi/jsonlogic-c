#include "jsonlogic_intern.h"

#include <stdlib.h>
#include <assert.h>

const JsonLogic_Handle JsonLogic_Null  = { .intptr = JsonLogic_Type_Null };
const JsonLogic_Handle JsonLogic_True  = { .intptr = JsonLogic_Type_Boolean | 1 };
const JsonLogic_Handle JsonLogic_False = { .intptr = JsonLogic_Type_Boolean | 0 };

JsonLogic_Type jsonlogic_typeof(JsonLogic_Handle handle) {
    if (handle.intptr < JsonLogic_MaxNumber) {
        return JsonLogic_Type_Number;
    }

    return handle.intptr & JsonLogic_TypeMask;
}

void jsonlogic_incref(JsonLogic_Handle handle) {
    switch (handle.intptr & JsonLogic_TypeMask) {
        case JsonLogic_Type_String:
            JSONLOGIC_CAST_STRING(handle)->size ++;
            break;

        case JsonLogic_Type_Array:
            JSONLOGIC_CAST_ARRAY(handle)->size ++;
            break;

        case JsonLogic_Type_Object:
            JSONLOGIC_CAST_OBJECT(handle)->size ++;
            break;
    }
}

void jsonlogic_decref(JsonLogic_Handle handle) {
    switch (handle.intptr & JsonLogic_TypeMask) {
        case JsonLogic_Type_String:
        {
            JsonLogic_String *string = JSONLOGIC_CAST_STRING(handle);
            assert(string->size > 0);
            string->size --;
            if (string->size == 0) {
                jsonlogic_free_string(string);
            }
            break;
        }
        case JsonLogic_Type_Array:
        {
            JsonLogic_Array *array = JSONLOGIC_CAST_ARRAY(handle);
            assert(array->size > 0);
            array->size --;
            if (array->size == 0) {
                jsonlogic_free_array(array);
            }
            break;
        }
        case JsonLogic_Type_Object:
        {
            JsonLogic_Object *object = JSONLOGIC_CAST_OBJECT(handle);
            assert(object->size > 0);
            object->size --;
            if (object->size == 0) {
                jsonlogic_free_object(object);
            }
            break;
        }
    }
}

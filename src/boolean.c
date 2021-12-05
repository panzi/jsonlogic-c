#include "jsonlogic_intern.h"

#include <stdlib.h>
#include <math.h>
const JsonLogic_Handle JsonLogic_True  = JSONLOGIC_TURE;
const JsonLogic_Handle JsonLogic_False = JSONLOGIC_FALSE;

JsonLogic_Handle jsonlogic_to_boolean(JsonLogic_Handle handle) {
    if (JSONLOGIC_IS_NUMBER(handle)) {
        const double number = JSONLOGIC_HNDL_TO_NUM(handle);
        return number == 0.0 || isnan(number) ?
            JsonLogic_False :
            JsonLogic_True;
    }

    switch (handle & JsonLogic_TypeMask) {
        case JsonLogic_Type_Boolean:
            return handle;

        case JsonLogic_Type_String:
            return (JSONLOGIC_CAST_STRING(handle)->size > 0) | JsonLogic_Type_Boolean;

        case JsonLogic_Type_Null:
            return JsonLogic_False;

        case JsonLogic_Type_Array:
            // this is different to JavaScript
            return (JSONLOGIC_CAST_ARRAY(handle)->size > 0) | JsonLogic_Type_Boolean;

        case JsonLogic_Type_Error:
            return handle;

        default:
            return JsonLogic_True;
    }
}

bool jsonlogic_to_bool(JsonLogic_Handle handle) {
    // interpretes errors as false
    return jsonlogic_to_boolean(handle) == JSONLOGIC_TURE;
}

JsonLogic_Handle jsonlogic_not(JsonLogic_Handle handle) {
    if (JSONLOGIC_IS_NUMBER(handle)) {
        const double number = JSONLOGIC_HNDL_TO_NUM(handle);
        return number == 0.0 || isnan(number) ?
            JsonLogic_True :
            JsonLogic_False;
    }

    switch (handle & JsonLogic_TypeMask) {
        case JsonLogic_Type_Boolean:
            return (~handle & 1) | JsonLogic_Type_Boolean;

        case JsonLogic_Type_String:
            return (JSONLOGIC_CAST_STRING(handle)->size == 0) | JsonLogic_Type_Boolean;

        case JsonLogic_Type_Null:
            return JsonLogic_True;

        case JsonLogic_Type_Array:
            // this is different to JavaScript
            return (JSONLOGIC_CAST_ARRAY(handle)->size == 0) | JsonLogic_Type_Boolean;

        case JsonLogic_Type_Error:
            return handle;

        default:
            return JsonLogic_False;
    }
}

JsonLogic_Handle certlogic_to_boolean(JsonLogic_Handle handle) {
    if (JSONLOGIC_IS_NUMBER(handle)) {
        const double number = JSONLOGIC_HNDL_TO_NUM(handle);
        return number == 0.0 || isnan(number) ?
            JsonLogic_False :
            JsonLogic_True;
    }

    switch (handle & JsonLogic_TypeMask) {
        case JsonLogic_Type_Boolean:
            return handle;

        case JsonLogic_Type_String:
            return (JSONLOGIC_CAST_STRING(handle)->size > 0) | JsonLogic_Type_Boolean;

        case JsonLogic_Type_Null:
            return JsonLogic_False;

        case JsonLogic_Type_Array:
            // this is different to JavaScript
            return (JSONLOGIC_CAST_ARRAY(handle)->size > 0) | JsonLogic_Type_Boolean;

        case JsonLogic_Type_Error:
            return handle;

        case JsonLogic_Type_Object:
            // this is for CertLogic, conflicts with JsonLogic
            return (JSONLOGIC_CAST_OBJECT(handle)->used > 0) | JsonLogic_Type_Boolean;

        default:
            return JsonLogic_True;
    }
}

bool certlogic_to_bool(JsonLogic_Handle handle) {
    // interpretes errors as false
    return certlogic_to_boolean(handle) == JSONLOGIC_TURE;
}

JsonLogic_Handle certlogic_not(JsonLogic_Handle handle) {
    if (JSONLOGIC_IS_NUMBER(handle)) {
        const double number = JSONLOGIC_HNDL_TO_NUM(handle);
        return number == 0.0 || isnan(number) ?
            JsonLogic_True :
            JsonLogic_False;
    }

    switch (handle & JsonLogic_TypeMask) {
        case JsonLogic_Type_Boolean:
            return (~handle & 1) | JsonLogic_Type_Boolean;

        case JsonLogic_Type_String:
            return (JSONLOGIC_CAST_STRING(handle)->size == 0) | JsonLogic_Type_Boolean;

        case JsonLogic_Type_Null:
            return JsonLogic_True;

        case JsonLogic_Type_Array:
            // this is different to JavaScript
            return (JSONLOGIC_CAST_ARRAY(handle)->size == 0) | JsonLogic_Type_Boolean;

        case JsonLogic_Type_Error:
            return handle;

        case JsonLogic_Type_Object:
            // this is for CertLogic, conflicts with JsonLogic
            return (JSONLOGIC_CAST_OBJECT(handle)->used == 0) | JsonLogic_Type_Boolean;

        default:
            return JsonLogic_False;
    }
}

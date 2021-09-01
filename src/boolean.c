#include "jsonlogic_intern.h"

#include <stdlib.h>
#include <math.h>

const JsonLogic_Handle JsonLogic_True  = { .intptr = JSONLOGIC_TURE  };
const JsonLogic_Handle JsonLogic_False = { .intptr = JSONLOGIC_FALSE };

JsonLogic_Handle jsonlogic_boolean_from(bool value);

JsonLogic_Handle jsonlogic_to_boolean(JsonLogic_Handle handle) {
    if (JSONLOGIC_IS_NUMBER(handle)) {
        return handle.number == 0.0 || isnan(handle.number) ?
            JsonLogic_False :
            JsonLogic_True;
    }

    switch (handle.intptr & JsonLogic_TypeMask) {
        case JsonLogic_Type_Boolean:
            return handle;

        case JsonLogic_Type_String:
            return (JsonLogic_Handle){ .intptr = (JSONLOGIC_CAST_STRING(handle)->size > 0) | JsonLogic_Type_Boolean };

        case JsonLogic_Type_Null:
            return JsonLogic_False;

        case JsonLogic_Type_Array:
            // this is different to JavaScript
            return (JsonLogic_Handle){ .intptr = (JSONLOGIC_CAST_ARRAY(handle)->size > 0) | JsonLogic_Type_Boolean };

        case JsonLogic_Type_Error:
            return handle;

        default:
            return JsonLogic_True;
    }
}

bool jsonlogic_to_bool(JsonLogic_Handle handle) {
    // interpretes errors as false
    return jsonlogic_to_boolean(handle).intptr == JSONLOGIC_TURE;
}

JsonLogic_Handle jsonlogic_not(JsonLogic_Handle handle) {
    if (JSONLOGIC_IS_NUMBER(handle)) {
        return handle.number == 0.0 || isnan(handle.number) ?
            JsonLogic_True :
            JsonLogic_False;
    }

    switch (handle.intptr & JsonLogic_TypeMask) {
        case JsonLogic_Type_Boolean:
            return (JsonLogic_Handle){ .intptr = (~handle.intptr & 1) | JsonLogic_Type_Boolean };

        case JsonLogic_Type_String:
            return (JsonLogic_Handle){ .intptr = (JSONLOGIC_CAST_STRING(handle)->size == 0) | JsonLogic_Type_Boolean };

        case JsonLogic_Type_Null:
            return JsonLogic_True;

        case JsonLogic_Type_Array:
            // this is different to JavaScript
            return (JsonLogic_Handle){ .intptr = (JSONLOGIC_CAST_ARRAY(handle)->size == 0) | JsonLogic_Type_Boolean };

        case JsonLogic_Type_Error:
            return handle;

        default:
            return JsonLogic_False;
    }
}

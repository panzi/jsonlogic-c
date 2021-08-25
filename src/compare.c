#include "jsonlogic_intern.h"

#include <stdlib.h>
#include <assert.h>

JsonLogic_Handle jsonlogic_strict_equal(JsonLogic_Handle a, JsonLogic_Handle b) {
    JsonLogic_Type atype = a.intptr & JsonLogic_TypeMask;
    JsonLogic_Type btype = b.intptr & JsonLogic_TypeMask;
    if (atype != btype) {
        return JsonLogic_False;
    }

    if (a.intptr < JsonLogic_MaxNumber) {
        return a.number == b.number ? JsonLogic_True : JsonLogic_False;
    }

    switch (atype) {
        case JsonLogic_Type_Null:
            return JsonLogic_True;

        case JsonLogic_Type_String:
            return jsonlogic_string_equals(JSONLOGIC_CAST_STRING(a), JSONLOGIC_CAST_STRING(b)) ?
                JsonLogic_True :
                JsonLogic_False;

        case JsonLogic_Type_Array:
        case JsonLogic_Type_Boolean:
        case JsonLogic_Type_Object:
            return a.intptr == b.intptr ? JsonLogic_True : JsonLogic_False;

        default:
            assert(false);
            return JsonLogic_Null;
    }
}

JsonLogic_Handle jsonlogic_strict_not_equal(JsonLogic_Handle a, JsonLogic_Handle b) {
    JsonLogic_Type atype = a.intptr & JsonLogic_TypeMask;
    JsonLogic_Type btype = b.intptr & JsonLogic_TypeMask;
    if (atype != btype) {
        return JsonLogic_True;
    }

    if (a.intptr < JsonLogic_MaxNumber) {
        return a.number != b.number ? JsonLogic_True : JsonLogic_False;
    }

    switch (atype) {
        case JsonLogic_Type_Null:
            return JsonLogic_False;

        case JsonLogic_Type_String:
            return jsonlogic_string_equals(JSONLOGIC_CAST_STRING(a), JSONLOGIC_CAST_STRING(b)) ?
                JsonLogic_False :
                JsonLogic_True;

        case JsonLogic_Type_Boolean:
        case JsonLogic_Type_Array:
        case JsonLogic_Type_Object:
            return a.intptr != b.intptr ? JsonLogic_True : JsonLogic_False;

        default:
            assert(false);
            return JsonLogic_Null;
    }
}

// not sure if I got that right
JsonLogic_Handle jsonlogic_equal(JsonLogic_Handle a, JsonLogic_Handle b) {
    JsonLogic_Type atype = a.intptr & JsonLogic_TypeMask;
    JsonLogic_Type btype = b.intptr & JsonLogic_TypeMask;
    if (atype == btype) {
        return jsonlogic_strict_equal(a, b);
    }

    if (atype == JsonLogic_Type_Null || btype == JsonLogic_Type_Null) {
        return JsonLogic_False;
    }

    if (a.intptr < JsonLogic_MaxNumber) {
        double bnum = jsonlogic_to_double(b);
        return a.number == bnum ? JsonLogic_True : JsonLogic_False;
    }

    switch (atype) {
        case JsonLogic_Type_String:
            if (b.intptr < JsonLogic_MaxNumber) {
                double anum = jsonlogic_to_double(a);
                return anum == b.number ? JsonLogic_True : JsonLogic_False;
            }
            switch (btype) {
                case JsonLogic_Type_Boolean:
                {
                    double anum = jsonlogic_to_double(a);
                    double bnum = jsonlogic_to_double(b);
                    return anum == bnum ? JsonLogic_True : JsonLogic_False;
                }
                case JsonLogic_Type_Array:
                case JsonLogic_Type_Object:
                {
                    JsonLogic_Handle bstr   = jsonlogic_to_string(b);
                    JsonLogic_Handle result = jsonlogic_equal(a, bstr);
                    jsonlogic_decref(bstr);
                    return result;
                }
                default:
                    assert(false);
                    return JsonLogic_Null;
            }
        case JsonLogic_Type_Boolean:
            if (b.intptr < JsonLogic_MaxNumber) {
                double anum = jsonlogic_to_double(a);
                return anum == b.number ? JsonLogic_True : JsonLogic_False;
            }
            switch (btype) {
                case JsonLogic_Type_Array:
                case JsonLogic_Type_Object:
                case JsonLogic_Type_String:
                {
                    double anum = jsonlogic_to_double(a);
                    double bnum = jsonlogic_to_double(b);
                    return anum == bnum ? JsonLogic_True : JsonLogic_False;
                }
                default:
                    assert(false);
                    return JsonLogic_Null;
            }
        case JsonLogic_Type_Array:
        case JsonLogic_Type_Object:
            if (b.intptr < JsonLogic_MaxNumber) {
                double anum = jsonlogic_to_double(a);
                double bnum = jsonlogic_to_double(b);
                return anum == bnum ? JsonLogic_True : JsonLogic_False;
            }
            switch (btype) {
                case JsonLogic_Type_Array:
                case JsonLogic_Type_Object:
                    return JsonLogic_False;

                case JsonLogic_Type_String:
                {
                    JsonLogic_Handle astr   = jsonlogic_to_string(a);
                    JsonLogic_Handle result = jsonlogic_equal(astr, b);
                    jsonlogic_decref(astr);
                    return result;
                }
                case JsonLogic_Type_Boolean:
                {
                    double anum = jsonlogic_to_double(a);
                    double bnum = jsonlogic_to_double(b);
                    return anum == bnum ? JsonLogic_True : JsonLogic_False;
                }
                default:
                    assert(false);
                    return JsonLogic_Null;
            }
        default:
            assert(false);
            return JsonLogic_Null;
    }
}

JsonLogic_Handle jsonlogic_lt(JsonLogic_Handle a, JsonLogic_Handle b) {
    if (a.intptr < JsonLogic_MaxNumber) {
        double bnum = jsonlogic_to_double(b);
        return a.number < bnum ? JsonLogic_True : JsonLogic_False;
    }

    if (b.intptr < JsonLogic_MaxNumber) {
        double anum = jsonlogic_to_double(a);
        return anum < b.number ? JsonLogic_True : JsonLogic_False;
    }

    if ((a.intptr & JsonLogic_TypeMask) == JsonLogic_Type_String) {
        JsonLogic_Handle bstr = jsonlogic_to_string(b);
        if (JSONLOGIC_IS_NULL(bstr)) {
            // memory allocation failed
            return JsonLogic_Null;
        }
        JsonLogic_Handle result = jsonlogic_string_compare(JSONLOGIC_CAST_STRING(a), JSONLOGIC_CAST_STRING(bstr)) < 0 ?
            JsonLogic_True : JsonLogic_False;
        jsonlogic_decref(bstr);
        return result;
    }

    if ((b.intptr & JsonLogic_TypeMask) == JsonLogic_Type_String) {
        JsonLogic_Handle astr = jsonlogic_to_string(a);
        if (JSONLOGIC_IS_NULL(astr)) {
            // memory allocation failed
            return JsonLogic_Null;
        }
        JsonLogic_Handle result = jsonlogic_string_compare(JSONLOGIC_CAST_STRING(astr), JSONLOGIC_CAST_STRING(b)) < 0 ?
            JsonLogic_True : JsonLogic_False;
        jsonlogic_decref(astr);
        return result;
    }

    return JsonLogic_False;
}

JsonLogic_Handle jsonlogic_gt(JsonLogic_Handle a, JsonLogic_Handle b) {
    if (a.intptr < JsonLogic_MaxNumber) {
        double bnum = jsonlogic_to_double(b);
        return a.number > bnum ? JsonLogic_True : JsonLogic_False;
    }

    if (b.intptr < JsonLogic_MaxNumber) {
        double anum = jsonlogic_to_double(a);
        return anum > b.number ? JsonLogic_True : JsonLogic_False;
    }

    if ((a.intptr & JsonLogic_TypeMask) == JsonLogic_Type_String) {
        JsonLogic_Handle bstr = jsonlogic_to_string(b);
        if (JSONLOGIC_IS_NULL(bstr)) {
            // memory allocation failed
            return JsonLogic_Null;
        }
        JsonLogic_Handle result = jsonlogic_string_compare(JSONLOGIC_CAST_STRING(a), JSONLOGIC_CAST_STRING(bstr)) > 0 ?
            JsonLogic_True : JsonLogic_False;
        jsonlogic_decref(bstr);
        return result;
    }

    if ((b.intptr & JsonLogic_TypeMask) == JsonLogic_Type_String) {
        JsonLogic_Handle astr = jsonlogic_to_string(a);
        if (JSONLOGIC_IS_NULL(astr)) {
            // memory allocation failed
            return JsonLogic_Null;
        }
        JsonLogic_Handle result = jsonlogic_string_compare(JSONLOGIC_CAST_STRING(astr), JSONLOGIC_CAST_STRING(b)) > 0 ?
            JsonLogic_True : JsonLogic_False;
        jsonlogic_decref(astr);
        return result;
    }

    return JsonLogic_False;
}

JsonLogic_Handle jsonlogic_le(JsonLogic_Handle a, JsonLogic_Handle b) {
    if (a.intptr < JsonLogic_MaxNumber) {
        double bnum = jsonlogic_to_double(b);
        return a.number <= bnum ? JsonLogic_True : JsonLogic_False;
    }

    if (b.intptr < JsonLogic_MaxNumber) {
        double anum = jsonlogic_to_double(a);
        return anum <= b.number ? JsonLogic_True : JsonLogic_False;
    }

    if ((a.intptr & JsonLogic_TypeMask) == JsonLogic_Type_String) {
        JsonLogic_Handle bstr = jsonlogic_to_string(b);
        if (JSONLOGIC_IS_NULL(bstr)) {
            // memory allocation failed
            return JsonLogic_Null;
        }
        JsonLogic_Handle result = jsonlogic_string_compare(JSONLOGIC_CAST_STRING(a), JSONLOGIC_CAST_STRING(bstr)) <= 0 ?
            JsonLogic_True : JsonLogic_False;
        jsonlogic_decref(bstr);
        return result;
    }

    if ((b.intptr & JsonLogic_TypeMask) == JsonLogic_Type_String) {
        JsonLogic_Handle astr = jsonlogic_to_string(a);
        if (JSONLOGIC_IS_NULL(astr)) {
            // memory allocation failed
            return JsonLogic_Null;
        }
        JsonLogic_Handle result = jsonlogic_string_compare(JSONLOGIC_CAST_STRING(astr), JSONLOGIC_CAST_STRING(b)) <= 0 ?
            JsonLogic_True : JsonLogic_False;
        jsonlogic_decref(astr);
        return result;
    }

    return jsonlogic_equal(a, b);
}

JsonLogic_Handle jsonlogic_ge(JsonLogic_Handle a, JsonLogic_Handle b) {
    if (a.intptr < JsonLogic_MaxNumber) {
        double bnum = jsonlogic_to_double(b);
        return a.number >= bnum ? JsonLogic_True : JsonLogic_False;
    }

    if (b.intptr < JsonLogic_MaxNumber) {
        double anum = jsonlogic_to_double(a);
        return anum >= b.number ? JsonLogic_True : JsonLogic_False;
    }

    if ((a.intptr & JsonLogic_TypeMask) == JsonLogic_Type_String) {
        JsonLogic_Handle bstr = jsonlogic_to_string(b);
        if (JSONLOGIC_IS_NULL(bstr)) {
            // memory allocation failed
            return JsonLogic_Null;
        }
        JsonLogic_Handle result = jsonlogic_string_compare(JSONLOGIC_CAST_STRING(a), JSONLOGIC_CAST_STRING(bstr)) >= 0 ?
            JsonLogic_True : JsonLogic_False;
        jsonlogic_decref(bstr);
        return result;
    }

    if ((b.intptr & JsonLogic_TypeMask) == JsonLogic_Type_String) {
        JsonLogic_Handle astr = jsonlogic_to_string(a);
        if (JSONLOGIC_IS_NULL(astr)) {
            // memory allocation failed
            return JsonLogic_Null;
        }
        JsonLogic_Handle result = jsonlogic_string_compare(JSONLOGIC_CAST_STRING(astr), JSONLOGIC_CAST_STRING(b)) >= 0 ?
            JsonLogic_True : JsonLogic_False;
        jsonlogic_decref(astr);
        return result;
    }

    return jsonlogic_equal(a, b);
}

#include "jsonlogic_intern.h"

#include <stdlib.h>
#include <assert.h>

bool jsonlogic_deep_strict_equal(JsonLogic_Handle a, JsonLogic_Handle b) {
    if (a.intptr == b.intptr) {
        return true;
    }

    if (JSONLOGIC_IS_NUMBER(a)) {
        if (JSONLOGIC_IS_NUMBER(b)) {
            return a.number == b.number;
        } else {
            return false;
        }
    }

    JsonLogic_Type atype = a.intptr & JsonLogic_TypeMask;
    JsonLogic_Type btype = b.intptr & JsonLogic_TypeMask;
    if (atype != btype) {
        return false;
    }

    switch (atype) {
        case JsonLogic_Type_Null:
            return true;

        case JsonLogic_Type_String:
            return jsonlogic_string_equals(JSONLOGIC_CAST_STRING(a), JSONLOGIC_CAST_STRING(b));

        case JsonLogic_Type_Array:
        {
            const JsonLogic_Array *aarray = JSONLOGIC_CAST_ARRAY(a);
            const JsonLogic_Array *barray = JSONLOGIC_CAST_ARRAY(b);

            if (aarray->size != barray->size) {
                return false;
            }

            for (size_t index = 0; index < aarray->size; ++ index) {
                if (!jsonlogic_deep_strict_equal(aarray->items[index], barray->items[index])) {
                    return false;
                }
            }

            return true;
        }
        case JsonLogic_Type_Object:
        {
            const JsonLogic_Object *aobject = JSONLOGIC_CAST_OBJECT(a);
            const JsonLogic_Object *bobject = JSONLOGIC_CAST_OBJECT(b);

            if (aobject->used != bobject->used) {
                return false;
            }

            for (size_t aindex = aobject->first_index; aindex < aobject->size; ++ aindex) {
                const JsonLogic_Object_Entry *aentry = &aobject->entries[aindex];

                if (!JSONLOGIC_IS_NULL(aentry->key)) {
                    const JsonLogic_String *strkey = JSONLOGIC_CAST_STRING(aentry->key);
                    size_t bindex = jsonlogic_object_get_index_utf16_with_hash(bobject, strkey->hash, strkey->str, strkey->size);
                    if (bindex >= bobject->size) {
                        return false;
                    }
                    if (!jsonlogic_deep_strict_equal(aentry->value, bobject->entries[bindex].value)) {
                        return false;
                    }
                }
            }

            return true;
        }
        case JsonLogic_Type_Boolean:
        case JsonLogic_Type_Error:
            return a.intptr == b.intptr;

        default:
            assert(false);
            return false;
    }
}

JsonLogic_Handle jsonlogic_strict_equal(JsonLogic_Handle a, JsonLogic_Handle b) {
    if (JSONLOGIC_IS_NUMBER(a)) {
        if (JSONLOGIC_IS_NUMBER(b)) {
            return (JsonLogic_Handle){ .intptr = (a.number == b.number) | JsonLogic_Type_Boolean };
        } else {
            return JsonLogic_False;
        }
    }

    JsonLogic_Type atype = a.intptr & JsonLogic_TypeMask;
    JsonLogic_Type btype = b.intptr & JsonLogic_TypeMask;
    if (atype != btype) {
        return JsonLogic_False;
    }

    switch (atype) {
        case JsonLogic_Type_Null:
            return JsonLogic_True;

        case JsonLogic_Type_String:
            return (JsonLogic_Handle){ .intptr = jsonlogic_string_equals(JSONLOGIC_CAST_STRING(a), JSONLOGIC_CAST_STRING(b)) | JsonLogic_Type_Boolean };

        case JsonLogic_Type_Array:
        case JsonLogic_Type_Boolean:
        case JsonLogic_Type_Object:
            return (JsonLogic_Handle){ .intptr = (a.intptr == b.intptr) | JsonLogic_Type_Boolean };

        case JsonLogic_Type_Error:
            return a;

        default:
            assert(false);
            return JsonLogic_Error_InternalError;
    }
}

JsonLogic_Handle jsonlogic_strict_not_equal(JsonLogic_Handle a, JsonLogic_Handle b) {
    if (JSONLOGIC_IS_NUMBER(a)) {
        if (JSONLOGIC_IS_NUMBER(b)) {
            return (JsonLogic_Handle){ .intptr = (a.number != b.number) | JsonLogic_Type_Boolean };
        } else {
            return JsonLogic_True;
        }
    }

    JsonLogic_Type atype = a.intptr & JsonLogic_TypeMask;
    JsonLogic_Type btype = b.intptr & JsonLogic_TypeMask;
    if (atype != btype) {
        return JsonLogic_True;
    }

    switch (atype) {
        case JsonLogic_Type_Null:
            return JsonLogic_False;

        case JsonLogic_Type_String:
            return (JsonLogic_Handle){ .intptr = jsonlogic_string_equals(JSONLOGIC_CAST_STRING(a), JSONLOGIC_CAST_STRING(b)) | JsonLogic_Type_Boolean };

        case JsonLogic_Type_Boolean:
        case JsonLogic_Type_Array:
        case JsonLogic_Type_Object:
            return (JsonLogic_Handle){ .intptr = (a.intptr != b.intptr) | JsonLogic_Type_Boolean };

        case JsonLogic_Type_Error:
            return a;

        default:
            assert(false);
            return JsonLogic_Error_InternalError;
    }
}

// not sure if I got that right
JsonLogic_Handle jsonlogic_equal(JsonLogic_Handle a, JsonLogic_Handle b) {
    if (JSONLOGIC_IS_NUMBER(a)) {
        if (JSONLOGIC_IS_NUMBER(b)) {
            // shortcut probable case?
            return (JsonLogic_Handle){ .intptr = (a.number == b.number) | JsonLogic_Type_Boolean };
        }
        if (JSONLOGIC_IS_ERROR(b)) {
            return b;
        }
        double bnum = jsonlogic_to_double(b);
        return (JsonLogic_Handle){ .intptr = (a.number == bnum) | JsonLogic_Type_Boolean };
    }

    if (JSONLOGIC_IS_NUMBER(b)) {
        double anum = jsonlogic_to_double(a);
        return (JsonLogic_Handle){ .intptr = (anum == b.number) | JsonLogic_Type_Boolean };
    }

    if (JSONLOGIC_IS_ERROR(a)) {
        return a;
    }

    JsonLogic_Type atype = a.intptr & JsonLogic_TypeMask;
    JsonLogic_Type btype = b.intptr & JsonLogic_TypeMask;
    if (atype == btype) {
        return jsonlogic_strict_equal(a, b);
    }

    if (atype == JsonLogic_Type_Null || btype == JsonLogic_Type_Null) {
        return JsonLogic_False;
    }

    switch (atype) {
        case JsonLogic_Type_String:
            switch (btype) {
                case JsonLogic_Type_Boolean:
                {
                    double anum = jsonlogic_to_double(a);
                    double bnum = jsonlogic_to_double(b);
                    return (JsonLogic_Handle){ .intptr = (anum == bnum) | JsonLogic_Type_Boolean };
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
                    return JsonLogic_Error_InternalError;
            }
        case JsonLogic_Type_Boolean:
            switch (btype) {
                case JsonLogic_Type_Array:
                case JsonLogic_Type_Object:
                case JsonLogic_Type_String:
                {
                    double anum = jsonlogic_to_double(a);
                    double bnum = jsonlogic_to_double(b);
                    return (JsonLogic_Handle){ .intptr = (anum == bnum) | JsonLogic_Type_Boolean };
                }
                default:
                    assert(false);
                    return JsonLogic_Error_InternalError;
            }
        case JsonLogic_Type_Array:
        case JsonLogic_Type_Object:
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
                    return (JsonLogic_Handle){ .intptr = (anum == bnum) | JsonLogic_Type_Boolean };
                }
                default:
                    assert(false);
                    return JsonLogic_Error_InternalError;
            }
        case JsonLogic_Type_Error:
            return a;

        default:
            assert(false);
            return JsonLogic_Error_InternalError;
    }
}

JsonLogic_Handle jsonlogic_not_equal(JsonLogic_Handle a, JsonLogic_Handle b) {
    JsonLogic_Handle result = jsonlogic_equal(a, b);
    if (JSONLOGIC_IS_ERROR(result)) {
        return result;
    }
    uint64_t value = JSONLOGIC_IS_TRUE(result);
    return (JsonLogic_Handle){ .intptr = (~value & 1) | JsonLogic_Type_Boolean };
}

int jsonlogic_comapre(JsonLogic_Handle a, JsonLogic_Handle b) {
    if (JSONLOGIC_IS_NUMBER(a)) {
        double bnum = jsonlogic_to_double(b);
        return a.number < bnum ? -1 : a.number > bnum ? 1 : 0;
    }

    if (JSONLOGIC_IS_NUMBER(b)) {
        double anum = jsonlogic_to_double(a);
        return anum < b.number ? -1 : anum > b.number ? 1 : 0;
    }

    if (JSONLOGIC_IS_STRING(a)) {
        JsonLogic_Handle bstr = jsonlogic_to_string(b);
        if (JSONLOGIC_IS_ERROR(bstr)) {
            return 0;
        }
        int result = jsonlogic_string_compare(JSONLOGIC_CAST_STRING(a), JSONLOGIC_CAST_STRING(bstr));
        jsonlogic_decref(bstr);
        return result;
    }

    if (JSONLOGIC_IS_STRING(b)) {
        JsonLogic_Handle astr = jsonlogic_to_string(a);
        if (JSONLOGIC_IS_ERROR(astr)) {
            return 0;
        }
        int result = jsonlogic_string_compare(JSONLOGIC_CAST_STRING(astr), JSONLOGIC_CAST_STRING(b));
        jsonlogic_decref(astr);
        return result;
    }

    return a.intptr < b.intptr ? -1 : a.intptr > b.intptr ? 1 : 0;
}

JsonLogic_Handle jsonlogic_lt(JsonLogic_Handle a, JsonLogic_Handle b) {
    if (JSONLOGIC_IS_NUMBER(a)) {
        double bnum = jsonlogic_to_double(b);
        return (JsonLogic_Handle){ .intptr = (a.number < bnum) | JsonLogic_Type_Boolean };
    }

    if (JSONLOGIC_IS_NUMBER(b)) {
        double anum = jsonlogic_to_double(a);
        return (JsonLogic_Handle){ .intptr = (anum < b.number) | JsonLogic_Type_Boolean };
    }

    if (JSONLOGIC_IS_STRING(a)) {
        JsonLogic_Handle bstr = jsonlogic_to_string(b);
        if (JSONLOGIC_IS_ERROR(bstr)) {
            return bstr;
        }
        JsonLogic_Handle result = jsonlogic_string_compare(JSONLOGIC_CAST_STRING(a), JSONLOGIC_CAST_STRING(bstr)) < 0 ?
            JsonLogic_True : JsonLogic_False;
        jsonlogic_decref(bstr);
        return result;
    }

    if (JSONLOGIC_IS_STRING(b)) {
        JsonLogic_Handle astr = jsonlogic_to_string(a);
        if (JSONLOGIC_IS_ERROR(astr)) {
            return astr;
        }
        JsonLogic_Handle result = jsonlogic_string_compare(JSONLOGIC_CAST_STRING(astr), JSONLOGIC_CAST_STRING(b)) < 0 ?
            JsonLogic_True : JsonLogic_False;
        jsonlogic_decref(astr);
        return result;
    }

    if (JSONLOGIC_IS_ERROR(a)) {
        return a;
    }

    if (JSONLOGIC_IS_ERROR(b)) {
        return b;
    }

    return JsonLogic_False;
}

JsonLogic_Handle jsonlogic_gt(JsonLogic_Handle a, JsonLogic_Handle b) {
    if (JSONLOGIC_IS_NUMBER(a)) {
        double bnum = jsonlogic_to_double(b);
        return (JsonLogic_Handle){ .intptr = (a.number > bnum) | JsonLogic_Type_Boolean };
    }

    if (JSONLOGIC_IS_NUMBER(b)) {
        double anum = jsonlogic_to_double(a);
        return (JsonLogic_Handle){ .intptr = (anum > b.number) | JsonLogic_Type_Boolean };
    }

    if (JSONLOGIC_IS_STRING(a)) {
        JsonLogic_Handle bstr = jsonlogic_to_string(b);
        if (JSONLOGIC_IS_ERROR(bstr)) {
            return bstr;
        }
        JsonLogic_Handle result = jsonlogic_string_compare(JSONLOGIC_CAST_STRING(a), JSONLOGIC_CAST_STRING(bstr)) > 0 ?
            JsonLogic_True : JsonLogic_False;
        jsonlogic_decref(bstr);
        return result;
    }

    if (JSONLOGIC_IS_STRING(b)) {
        JsonLogic_Handle astr = jsonlogic_to_string(a);
        if (JSONLOGIC_IS_ERROR(astr)) {
            return astr;
        }
        JsonLogic_Handle result = jsonlogic_string_compare(JSONLOGIC_CAST_STRING(astr), JSONLOGIC_CAST_STRING(b)) > 0 ?
            JsonLogic_True : JsonLogic_False;
        jsonlogic_decref(astr);
        return result;
    }

    if (JSONLOGIC_IS_ERROR(a)) {
        return a;
    }

    if (JSONLOGIC_IS_ERROR(b)) {
        return b;
    }

    return JsonLogic_False;
}

JsonLogic_Handle jsonlogic_le(JsonLogic_Handle a, JsonLogic_Handle b) {
    if (JSONLOGIC_IS_NUMBER(a)) {
        double bnum = jsonlogic_to_double(b);
        return (JsonLogic_Handle){ .intptr = (a.number <= bnum) | JsonLogic_Type_Boolean };
    }

    if (JSONLOGIC_IS_NUMBER(b)) {
        double anum = jsonlogic_to_double(a);
        return (JsonLogic_Handle){ .intptr = (anum <= b.number) | JsonLogic_Type_Boolean };
    }

    if (JSONLOGIC_IS_STRING(a)) {
        JsonLogic_Handle bstr = jsonlogic_to_string(b);
        if (JSONLOGIC_IS_ERROR(bstr)) {
            return bstr;
        }
        JsonLogic_Handle result = jsonlogic_string_compare(JSONLOGIC_CAST_STRING(a), JSONLOGIC_CAST_STRING(bstr)) <= 0 ?
            JsonLogic_True : JsonLogic_False;
        jsonlogic_decref(bstr);
        return result;
    }

    if (JSONLOGIC_IS_STRING(b)) {
        JsonLogic_Handle astr = jsonlogic_to_string(a);
        if (JSONLOGIC_IS_ERROR(astr)) {
            return astr;
        }
        JsonLogic_Handle result = jsonlogic_string_compare(JSONLOGIC_CAST_STRING(astr), JSONLOGIC_CAST_STRING(b)) <= 0 ?
            JsonLogic_True : JsonLogic_False;
        jsonlogic_decref(astr);
        return result;
    }

    if (JSONLOGIC_IS_ERROR(a)) {
        return a;
    }

    if (JSONLOGIC_IS_ERROR(b)) {
        return b;
    }

    return jsonlogic_equal(a, b);
}

JsonLogic_Handle jsonlogic_ge(JsonLogic_Handle a, JsonLogic_Handle b) {
    if (JSONLOGIC_IS_NUMBER(a)) {
        double bnum = jsonlogic_to_double(b);
        return (JsonLogic_Handle){ .intptr = (a.number >= bnum) | JsonLogic_Type_Boolean };
    }

    if (JSONLOGIC_IS_NUMBER(b)) {
        double anum = jsonlogic_to_double(a);
        return (JsonLogic_Handle){ .intptr = (anum >= b.number) | JsonLogic_Type_Boolean };
    }

    if (JSONLOGIC_IS_STRING(a)) {
        JsonLogic_Handle bstr = jsonlogic_to_string(b);
        if (JSONLOGIC_IS_ERROR(bstr)) {
            return bstr;
        }
        JsonLogic_Handle result = jsonlogic_string_compare(JSONLOGIC_CAST_STRING(a), JSONLOGIC_CAST_STRING(bstr)) >= 0 ?
            JsonLogic_True : JsonLogic_False;
        jsonlogic_decref(bstr);
        return result;
    }

    if (JSONLOGIC_IS_STRING(b)) {
        JsonLogic_Handle astr = jsonlogic_to_string(a);
        if (JSONLOGIC_IS_ERROR(astr)) {
            return astr;
        }
        JsonLogic_Handle result = jsonlogic_string_compare(JSONLOGIC_CAST_STRING(astr), JSONLOGIC_CAST_STRING(b)) >= 0 ?
            JsonLogic_True : JsonLogic_False;
        jsonlogic_decref(astr);
        return result;
    }

    if (JSONLOGIC_IS_ERROR(a)) {
        return a;
    }

    if (JSONLOGIC_IS_ERROR(b)) {
        return b;
    }

    return jsonlogic_equal(a, b);
}

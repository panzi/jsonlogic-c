#include "jsonlogic_intern.h"

#include <stdlib.h>
#include <math.h>
#include <assert.h>

JsonLogic_Handle jsonlogic_to_number(JsonLogic_Handle handle) {
    for (;;) {
        if (handle.intptr < JsonLogic_MaxNumber) {
            return handle;
        }

        switch (handle.intptr & JsonLogic_TypeMask) {
            case JsonLogic_Type_String:
            {
                // TODO: more efficient method! Directly parse UTF-16?
                const JsonLogic_String *string = JSONLOGIC_CAST_STRING(handle);
                if (string->size == 0) {
                    return (JsonLogic_Handle){ .number = 0.0 };
                }
                for (size_t index = 0; index < string->size; ++ index) {
                    JsonLogic_Char ch = string->str[index];
                    if (ch != 'e' && ch != 'E' && ch != '.' && ch != '+' && ch != '-' && !(ch >= '0' && ch <= '9')) {
                        return (JsonLogic_Handle){ .number = NAN };
                    }
                }

                char *endptr = NULL;
                char buf[128];
                if (string->size < sizeof(buf)) {
                    for (size_t index = 0; index < string->size; ++ index) {
                        buf[index] = string->str[index];
                    }
                    buf[string->size] = 0;
                    double value = strtod(buf, &endptr);
                    if (*endptr) {
                        return (JsonLogic_Handle){ .number = NAN };
                    }
                    return (JsonLogic_Handle){ .number = value };
                } else {
                    char *buf = malloc(string->size + 1);
                    if (buf == NULL) {
                        // out of memory
                        assert(false);
                        return JsonLogic_Null;
                    }
                    for (size_t index = 0; index < string->size; ++ index) {
                        buf[index] = string->str[index];
                    }
                    buf[string->size] = 0;
                    double value = strtod(buf, &endptr);
                    free(buf);
                    if (*endptr) {
                        return (JsonLogic_Handle){ .number = NAN };
                    }
                    return (JsonLogic_Handle){ .number = value };
                }
            }
            case JsonLogic_Type_Array:
            {
                const JsonLogic_Array *array = JSONLOGIC_CAST_ARRAY(handle);
                if (array->size == 0) {
                    return (JsonLogic_Handle){ .number = 0.0 };
                } else if (array->size > 1) {
                    return (JsonLogic_Handle){ .number = NAN };
                }
                handle = array->items[0];
                break;
            }
            case JsonLogic_Type_Null:
                return (JsonLogic_Handle){ .number = 0.0 };

            case JsonLogic_Type_Boolean:
                return (JsonLogic_Handle){ .number = handle.intptr == JsonLogic_True.intptr ? 1.0 : 0.0 };

            default:
                return (JsonLogic_Handle){ .number = NAN };
        }
    }
}

inline double jsonlogic_to_double(JsonLogic_Handle handle) {
    // since all special values are some sort of NaN we can ignore here that there can be JsonLogic_Null
    return jsonlogic_to_number(handle).number;
}

JsonLogic_Handle jsonlogic_negative(JsonLogic_Handle value) {
    return (JsonLogic_Handle){ .number = -jsonlogic_to_number(value).number };
}

JsonLogic_Handle jsonlogic_add(JsonLogic_Handle a, JsonLogic_Handle b) {
    return (JsonLogic_Handle){ .number = jsonlogic_to_double(a) + jsonlogic_to_double(b) };
}

JsonLogic_Handle jsonlogic_sub(JsonLogic_Handle a, JsonLogic_Handle b) {
    return (JsonLogic_Handle){ .number = jsonlogic_to_double(a) - jsonlogic_to_double(b) };
}

JsonLogic_Handle jsonlogic_mul(JsonLogic_Handle a, JsonLogic_Handle b) {
    return (JsonLogic_Handle){ .number = jsonlogic_to_double(a) * jsonlogic_to_double(b) };
}

JsonLogic_Handle jsonlogic_div(JsonLogic_Handle a, JsonLogic_Handle b) {
    return (JsonLogic_Handle){ .number = jsonlogic_to_double(a) / jsonlogic_to_double(b) };
}

JsonLogic_Handle jsonlogic_mod(JsonLogic_Handle a, JsonLogic_Handle b) {
    return (JsonLogic_Handle){ .number = fmod(jsonlogic_to_double(a), jsonlogic_to_double(b)) };
}

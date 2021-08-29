#include "jsonlogic_intern.h"

#include <stdlib.h>
#include <math.h>
#include <assert.h>

#define JSONLOGIC_IS_NUM(ch) \
    ((ch) == u'e' || (ch) == u'E' || (ch) == u'.' || (ch) == u'+' || (ch) == u'-' || ((ch) >= u'0' && (ch) <= u'9'))

#define JSONLOGIC_IS_SPACE(ch) \
    ((ch) >= u'\t' && (ch) <= u'\r')

const JsonLogic_Handle JsonLogic_NaN = { .number = NAN };

JsonLogic_Handle jsonlogic_number_from(double value) {
    return (JsonLogic_Handle){ .number = value };
}

JsonLogic_Handle jsonlogic_to_number(JsonLogic_Handle handle) {
    for (;;) {
        if (JSONLOGIC_IS_NUMBER(handle)) {
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

                const char16_t *str = string->str;
                size_t size = string->size;

                while (size > 0 && JSONLOGIC_IS_SPACE(*str)) {
                    ++ str;
                    -- size;
                }

                while (size > 0 && JSONLOGIC_IS_SPACE(str[size - 1])) {
                    -- size;
                }

                if (jsonlogic_utf16_equals(str, size, JSONLOGIC_INFINITY_STRING, JSONLOGIC_INFINITY_STRING_SIZE)) {
                    return (JsonLogic_Handle){ .number = INFINITY };
                }

                if (jsonlogic_utf16_equals(str, size, JSONLOGIC_POS_INFINITY_STRING, JSONLOGIC_POS_INFINITY_STRING_SIZE)) {
                    return (JsonLogic_Handle){ .number = INFINITY };
                }

                if (jsonlogic_utf16_equals(str, size, JSONLOGIC_NEG_INFINITY_STRING, JSONLOGIC_NEG_INFINITY_STRING_SIZE)) {
                    return (JsonLogic_Handle){ .number = -INFINITY };
                }

                char *endptr = NULL;
                char buf[128];
                if (size < sizeof(buf)) {
                    for (size_t index = 0; index < size; ++ index) {
                        char16_t ch = str[index];
                        if (!JSONLOGIC_IS_NUM(ch)) {
                            return JsonLogic_NaN;
                        }
                        buf[index] = ch;
                    }
                    buf[size] = 0;
                    double value = strtod(buf, &endptr);
                    if (*endptr) {
                        return JsonLogic_NaN;
                    }
                    return (JsonLogic_Handle){ .number = value };
                } else {
                    char *buf = malloc(size + 1);
                    if (buf == NULL) {
                        JSONLOGIC_ERROR_MEMORY();
                        return JsonLogic_Error_OutOfMemory;
                    }
                    for (size_t index = 0; index < size; ++ index) {
                        char16_t ch = str[index];
                        if (!JSONLOGIC_IS_NUM(ch)) {
                            free(buf);
                            return JsonLogic_NaN;
                        }
                        buf[index] = ch;
                    }
                    buf[size] = 0;
                    double value = strtod(buf, &endptr);
                    free(buf);
                    if (*endptr) {
                        return JsonLogic_NaN;
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
                    return JsonLogic_NaN;
                }
                handle = array->items[0];
                break;
            }
            case JsonLogic_Type_Null:
                return (JsonLogic_Handle){ .number = 0.0 };

            case JsonLogic_Type_Boolean:
                return (JsonLogic_Handle){ .number = JSONLOGIC_IS_TRUE(handle) ? 1.0 : 0.0 };

            case JsonLogic_Type_Error:
                return handle;

            default:
                assert(false);
                return JsonLogic_Error_InternalError;
        }
    }
}

inline double jsonlogic_to_double(JsonLogic_Handle handle) {
    // since all special values are some sort of NaN we can ignore here that there can be JsonLogic_Error_*
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

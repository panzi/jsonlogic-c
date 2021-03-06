#include "jsonlogic_intern.h"

#include <stdlib.h>
#include <math.h>
#include <assert.h>

static bool JsonLogic_IsNumMap[256] = {
    ['e'] = true,
    ['E'] = true,
    ['.'] = true,
    ['+'] = true,
    ['-'] = true,
    ['0'] = true,
    ['1'] = true,
    ['2'] = true,
    ['3'] = true,
    ['4'] = true,
    ['5'] = true,
    ['6'] = true,
    ['7'] = true,
    ['8'] = true,
    ['9'] = true,
};

static bool JsonLogic_IsSpaceMap[256] = {
    ['\t'] = true,
    ['\n'] = true,
    ['\v'] = true,
    ['\f'] = true,
    ['\r'] = true,
    [' ']  = true,
};

#define JSONLOGIC_IS_NUM(ch) \
    ((ch) <= 0xFF && JsonLogic_IsNumMap[(ch)])

#define JSONLOGIC_IS_SPACE(ch) \
    ((ch) <= 0xFF && JsonLogic_IsSpaceMap[(ch)])

const JsonLogic_Handle JsonLogic_NaN = 0x7ff8000000000000;

JsonLogic_Handle jsonlogic_number_from(double value) {
    return JSONLOGIC_NUM_TO_HNDL(value);
}

JsonLogic_Handle jsonlogic_to_number(JsonLogic_Handle handle) {
    for (;;) {
        if (JSONLOGIC_IS_NUMBER(handle)) {
            return handle;
        }

        switch (handle & JsonLogic_TypeMask) {
            case JsonLogic_Type_String:
            {
                // TODO: more efficient method! Directly parse UTF-16?
                const JsonLogic_String *string = JSONLOGIC_CAST_STRING(handle);
                if (string->size == 0) {
                    return JSONLOGIC_NUM_TO_HNDL(0.0);
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
                    return JSONLOGIC_NUM_TO_HNDL(INFINITY);
                }

                if (jsonlogic_utf16_equals(str, size, JSONLOGIC_POS_INFINITY_STRING, JSONLOGIC_POS_INFINITY_STRING_SIZE)) {
                    return JSONLOGIC_NUM_TO_HNDL(INFINITY);
                }

                if (jsonlogic_utf16_equals(str, size, JSONLOGIC_NEG_INFINITY_STRING, JSONLOGIC_NEG_INFINITY_STRING_SIZE)) {
                    return JSONLOGIC_NUM_TO_HNDL(-INFINITY);
                }
                if (JsonLogic_C_Locale == NULL) {
                    JsonLogic_C_Locale = JSONLOGIC_CREATE_C_LOCALE();
                }

                char *endptr = NULL;
                char buf[128];
                if (size < sizeof(buf)) {
                    for (size_t index = 0; index < size; ++ index) {
                        char16_t ch = str[index];
                        if (!JSONLOGIC_IS_NUM(ch)) {
                            return JsonLogic_NaN;
                        }
                        buf[index] = (char) ch;
                    }
                    buf[size] = 0;
                    double value = JSONLOGIC_STRTOD_L(buf, &endptr, JsonLogic_C_Locale);
                    if (*endptr) {
                        return JsonLogic_NaN;
                    }
                    return JSONLOGIC_NUM_TO_HNDL(value);
                } else {
                    char *heap_buf = malloc(size + 1);
                    if (heap_buf == NULL) {
                        JSONLOGIC_ERROR_MEMORY();
                        return JsonLogic_Error_OutOfMemory;
                    }
                    for (size_t index = 0; index < size; ++ index) {
                        char16_t ch = str[index];
                        if (!JSONLOGIC_IS_NUM(ch)) {
                            free(heap_buf);
                            return JsonLogic_NaN;
                        }
                        heap_buf[index] = (char) ch;
                    }
                    heap_buf[size] = 0;
                    double value = JSONLOGIC_STRTOD_L(heap_buf, &endptr, JsonLogic_C_Locale);
                    free(heap_buf);
                    if (*endptr) {
                        return JsonLogic_NaN;
                    }
                    return JSONLOGIC_NUM_TO_HNDL(value);
                }
            }
            case JsonLogic_Type_Array:
            {
                const JsonLogic_Array *array = JSONLOGIC_CAST_ARRAY(handle);
                if (array->size == 0) {
                    return JSONLOGIC_NUM_TO_HNDL(0.0);
                } else if (array->size > 1) {
                    return JsonLogic_NaN;
                }
                handle = array->items[0];
                break;
            }
            case JsonLogic_Type_Null:
                return JSONLOGIC_NUM_TO_HNDL(0.0);

            case JsonLogic_Type_Boolean:
                return JSONLOGIC_NUM_TO_HNDL((double) (handle & 1));

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
    return JSONLOGIC_HNDL_TO_NUM(jsonlogic_to_number(handle));
}

JsonLogic_Handle jsonlogic_negative(JsonLogic_Handle value) {
    return JSONLOGIC_NUM_TO_HNDL(-JSONLOGIC_HNDL_TO_NUM(jsonlogic_to_number(value)));
}

JsonLogic_Handle jsonlogic_add(JsonLogic_Handle a, JsonLogic_Handle b) {
    return JSONLOGIC_NUM_TO_HNDL(jsonlogic_to_double(a) + jsonlogic_to_double(b));
}

JsonLogic_Handle jsonlogic_sub(JsonLogic_Handle a, JsonLogic_Handle b) {
    return JSONLOGIC_NUM_TO_HNDL(jsonlogic_to_double(a) - jsonlogic_to_double(b));
}

JsonLogic_Handle jsonlogic_mul(JsonLogic_Handle a, JsonLogic_Handle b) {
    return JSONLOGIC_NUM_TO_HNDL(jsonlogic_to_double(a) * jsonlogic_to_double(b));
}

JsonLogic_Handle jsonlogic_div(JsonLogic_Handle a, JsonLogic_Handle b) {
    return JSONLOGIC_NUM_TO_HNDL(jsonlogic_to_double(a) / jsonlogic_to_double(b));
}

JsonLogic_Handle jsonlogic_mod(JsonLogic_Handle a, JsonLogic_Handle b) {
    return JSONLOGIC_NUM_TO_HNDL(fmod(jsonlogic_to_double(a), jsonlogic_to_double(b)));
}

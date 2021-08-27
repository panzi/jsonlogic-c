#include "jsonlogic_intern.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <math.h>

JsonLogic_Handle jsonlogic_parse(const char *str) {
    return jsonlogic_parse_sized(str, strlen(str));
}
/*
typedef enum JsonLogic_ParserState {
    JsonLogic_ParserState_Start,
    JsonLogic_ParserState_End,
    JsonLogic_ParserState_String,
    JsonLogic_ParserState_StringEsc,
    JsonLogic_ParserState_StringUni1,
    JsonLogic_ParserState_StringUni2,
    JsonLogic_ParserState_StringUni3,
    JsonLogic_ParserState_StringUni4,
    JsonLogic_ParserState_StringUni5,
    JsonLogic_ParserState_StringUni6,
    JsonLogic_ParserState_Array,
    // ...
    JsonLogic_ParserState_Error,
    JsonLogic_ParserState_Max,
} JsonLogic_ParserState;

static const JsonLogic_ParserState JsonLogic_Parser[JsonLogic_ParserState_Max][256] = {
    [JsonLogic_ParserState_Error] = {
        JsonLogic_ParserState_Error
    }
};
*/
JsonLogic_Handle jsonlogic_parse_sized(const char *str, size_t size) {
    // TODO


    JSONLOGIC_ERROR("%s", "not implemented");

    return JsonLogic_Null;
}

static inline JsonLogic_Char jsonlogic_to_hex(uint8_t half_byte) {
    return half_byte > 9 ? 'a' + half_byte : '0' + half_byte;
}

static bool jsonlogic_stringify_intern(JsonLogic_Buffer *buf, JsonLogic_Handle handle) {
    if (JSONLOGIC_IS_NUMBER(handle)) {
        if (isnan(handle.number) || isinf(handle.number)) {
            return jsonlogic_buffer_append_utf16(buf, JSONLOGIC_NULL_STRING, JSONLOGIC_NULL_STRING_SIZE);
        }

        return jsonlogic_buffer_append_double(buf, handle.number);
    }

    switch (handle.intptr & JsonLogic_TypeMask) {
        case JsonLogic_Type_String:
            if (!jsonlogic_buffer_append_utf16(buf, (JsonLogic_Char[]){ '"' }, 1)) {
                return false;
            }

            const JsonLogic_String *string = JSONLOGIC_CAST_STRING(handle);

            for (size_t index = 0; index < string->size; ++ index) {
                JsonLogic_Char ch = string->str[index];

                switch (ch) {
                    case '"':
                        if (!jsonlogic_buffer_append_utf16(buf, (JsonLogic_Char[]){ '\\', '"' }, 2)) {
                            return false;
                        }
                        break;

                    case '\\':
                        if (!jsonlogic_buffer_append_utf16(buf, (JsonLogic_Char[]){ '\\', '\\' }, 2)) {
                            return false;
                        }
                        break;

                    case '\b':
                        if (!jsonlogic_buffer_append_utf16(buf, (JsonLogic_Char[]){ '\\', 'b' }, 2)) {
                            return false;
                        }
                        break;

                    case '\f':
                        if (!jsonlogic_buffer_append_utf16(buf, (JsonLogic_Char[]){ '\\', 'f' }, 2)) {
                            return false;
                        }
                        break;

                    case '\n':
                        if (!jsonlogic_buffer_append_utf16(buf, (JsonLogic_Char[]){ '\\', 'n' }, 2)) {
                            return false;
                        }
                        break;

                    case '\r':
                        if (!jsonlogic_buffer_append_utf16(buf, (JsonLogic_Char[]){ '\\', 'r' }, 2)) {
                            return false;
                        }
                        break;

                    case '\t':
                        if (!jsonlogic_buffer_append_utf16(buf, (JsonLogic_Char[]){ '\\', 't' }, 2)) {
                            return false;
                        }
                        break;

                    default:
                        if (ch >= 0x010000) {
                            JsonLogic_Char hex[6] = {
                                '\\', 'u',
                                jsonlogic_to_hex((ch >> 12) & 0xF),
                                jsonlogic_to_hex((ch >>  8) & 0xF),
                                jsonlogic_to_hex((ch >>  4) & 0xF),
                                jsonlogic_to_hex( ch        & 0xF),
                            };

                            if (!jsonlogic_buffer_append_utf16(buf, hex, sizeof(hex) / sizeof(hex[0]))) {
                                return false;
                            }
                        } else {
                            if (!jsonlogic_buffer_append_utf16(buf, (JsonLogic_Char[]){ ch }, 1)) {
                                return false;
                            }
                        }
                        break;
                }
            }

            if (!jsonlogic_buffer_append_utf16(buf, (JsonLogic_Char[]){ '"' }, 1)) {
                return false;
            }
            return false;

        case JsonLogic_Type_Boolean:
            if (handle.intptr == JsonLogic_False.intptr) {
                return jsonlogic_buffer_append_utf16(buf, JSONLOGIC_FALSE_STRING, JSONLOGIC_FALSE_STRING_SIZE);
            } else {
                return jsonlogic_buffer_append_utf16(buf, JSONLOGIC_TRUE_STRING, JSONLOGIC_TRUE_STRING_SIZE);
            }
        case JsonLogic_Type_Null:
            return jsonlogic_buffer_append_utf16(buf, JSONLOGIC_NULL_STRING, JSONLOGIC_NULL_STRING_SIZE);

        case JsonLogic_Type_Array:
        {
            if (!jsonlogic_buffer_append_utf16(buf, (JsonLogic_Char[]){ '[' }, 1)) {
                return false;
            }

            JsonLogic_Array *array = JSONLOGIC_CAST_ARRAY(handle);
            if (array->size > 0) {
                if (!jsonlogic_stringify_intern(buf, array->items[0])) {
                    return false;
                }
                for (size_t index = 1; index < array->size; ++ index) {
                    if (!jsonlogic_buffer_append_utf16(buf, (JsonLogic_Char[]){','}, 1)) {
                        return false;
                    }
                    JsonLogic_Handle item = array->items[index];
                    if (!jsonlogic_stringify_intern(buf, item)) {
                        return false;
                    }
                }
            }

            if (!jsonlogic_buffer_append_utf16(buf, (JsonLogic_Char[]){ ']' }, 1)) {
                return false;
            }
            return true;
        }
        case JsonLogic_Type_Object:
            if (!jsonlogic_buffer_append_utf16(buf, (JsonLogic_Char[]){ '{' }, 1)) {
                return false;
            }

            JsonLogic_Object *object = JSONLOGIC_CAST_OBJECT(handle);
            if (object->size > 0) {
                if (!jsonlogic_stringify_intern(buf, object->entries[0].key)) {
                    return false;
                }

                if (!jsonlogic_buffer_append_utf16(buf, (JsonLogic_Char[]){ ':' }, 1)) {
                    return false;
                }

                if (!jsonlogic_stringify_intern(buf, object->entries[0].value)) {
                    return false;
                }

                for (size_t index = 1; index < object->size; ++ index) {
                    if (!jsonlogic_stringify_intern(buf, object->entries[index].key)) {
                        return false;
                    }

                    if (!jsonlogic_buffer_append_utf16(buf, (JsonLogic_Char[]){ ':' }, 1)) {
                        return false;
                    }

                    if (!jsonlogic_stringify_intern(buf, object->entries[index].value)) {
                        return false;
                    }
                }
            }

            if (!jsonlogic_buffer_append_utf16(buf, (JsonLogic_Char[]){ '}' }, 1)) {
                return false;
            }
            return true;

        case JsonLogic_Type_Error:
            jsonlogic_buffer_append_latin1(buf, jsonlogic_get_error_message(jsonlogic_get_error(handle)));
            return false;

        default:
            assert(false);
            return false;
    }
}

JsonLogic_Handle jsonlogic_stringify(JsonLogic_Handle value) {
    if (JSONLOGIC_IS_ERROR(value)) {
        return value;
    }

    JsonLogic_Buffer buf = JSONLOGIC_BUFFER_INIT;
    
    if (!jsonlogic_stringify_intern(&buf, value)) {
        jsonlogic_buffer_free(&buf);
        return JsonLogic_Null;
    }

    return jsonlogic_string_into_handle(jsonlogic_buffer_take(&buf));
}


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

static JsonLogic_Error jsonlogic_stringify_intern(JsonLogic_StrBuf *buf, JsonLogic_Handle handle) {
    if (JSONLOGIC_IS_NUMBER(handle)) {
        if (isnan(handle.number) || isinf(handle.number)) {
            return jsonlogic_strbuf_append_utf16(buf, JSONLOGIC_NULL_STRING, JSONLOGIC_NULL_STRING_SIZE);
        }

        return jsonlogic_strbuf_append_double(buf, handle.number);
    }

    switch (handle.intptr & JsonLogic_TypeMask) {
        case JsonLogic_Type_String:
            TRY(jsonlogic_strbuf_append_utf16(buf, (JsonLogic_Char[]){ '"' }, 1));

            const JsonLogic_String *string = JSONLOGIC_CAST_STRING(handle);

            for (size_t index = 0; index < string->size; ++ index) {
                JsonLogic_Char ch = string->str[index];

                switch (ch) {
                    case '"':
                        TRY(jsonlogic_strbuf_append_utf16(buf, (JsonLogic_Char[]){ '\\', '"' }, 2));
                        break;

                    case '\\':
                        TRY(jsonlogic_strbuf_append_utf16(buf, (JsonLogic_Char[]){ '\\', '\\' }, 2));
                        break;

                    case '\b':
                        TRY(jsonlogic_strbuf_append_utf16(buf, (JsonLogic_Char[]){ '\\', 'b' }, 2));
                        break;

                    case '\f':
                        TRY(jsonlogic_strbuf_append_utf16(buf, (JsonLogic_Char[]){ '\\', 'f' }, 2));
                        break;

                    case '\n':
                        TRY(jsonlogic_strbuf_append_utf16(buf, (JsonLogic_Char[]){ '\\', 'n' }, 2));
                        break;

                    case '\r':
                        TRY(jsonlogic_strbuf_append_utf16(buf, (JsonLogic_Char[]){ '\\', 'r' }, 2));
                        break;

                    case '\t':
                        TRY(jsonlogic_strbuf_append_utf16(buf, (JsonLogic_Char[]){ '\\', 't' }, 2));
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

                            TRY(jsonlogic_strbuf_append_utf16(buf, hex, sizeof(hex) / sizeof(hex[0])));
                        } else {
                            TRY(jsonlogic_strbuf_append_utf16(buf, (JsonLogic_Char[]){ ch }, 1));
                        }
                        break;
                }
            }

            return jsonlogic_strbuf_append_utf16(buf, (JsonLogic_Char[]){ '"' }, 1);

        case JsonLogic_Type_Boolean:
            if (handle.intptr == JsonLogic_False.intptr) {
                return jsonlogic_strbuf_append_utf16(buf, JSONLOGIC_FALSE_STRING, JSONLOGIC_FALSE_STRING_SIZE);
            } else {
                return jsonlogic_strbuf_append_utf16(buf, JSONLOGIC_TRUE_STRING, JSONLOGIC_TRUE_STRING_SIZE);
            }
        case JsonLogic_Type_Null:
            return jsonlogic_strbuf_append_utf16(buf, JSONLOGIC_NULL_STRING, JSONLOGIC_NULL_STRING_SIZE);

        case JsonLogic_Type_Array:
        {
            TRY(jsonlogic_strbuf_append_utf16(buf, (JsonLogic_Char[]){ '[' }, 1));

            JsonLogic_Array *array = JSONLOGIC_CAST_ARRAY(handle);
            if (array->size > 0) {
                if (!jsonlogic_stringify_intern(buf, array->items[0])) {
                    return false;
                }
                for (size_t index = 1; index < array->size; ++ index) {
                    TRY(jsonlogic_strbuf_append_utf16(buf, (JsonLogic_Char[]){','}, 1));
                    JsonLogic_Handle item = array->items[index];
                    TRY(jsonlogic_stringify_intern(buf, item));
                }
            }

            return jsonlogic_strbuf_append_utf16(buf, (JsonLogic_Char[]){ ']' }, 1);
        }
        case JsonLogic_Type_Object:
            TRY(jsonlogic_strbuf_append_utf16(buf, (JsonLogic_Char[]){ '{' }, 1));

            JsonLogic_Object *object = JSONLOGIC_CAST_OBJECT(handle);
            if (object->size > 0) {
                TRY(jsonlogic_stringify_intern(buf, object->entries[0].key));
                TRY(jsonlogic_strbuf_append_utf16(buf, (JsonLogic_Char[]){ ':' }, 1));
                TRY(jsonlogic_stringify_intern(buf, object->entries[0].value));

                for (size_t index = 1; index < object->size; ++ index) {
                    TRY(jsonlogic_stringify_intern(buf, object->entries[index].key));
                    TRY(jsonlogic_strbuf_append_utf16(buf, (JsonLogic_Char[]){ ':' }, 1));
                    TRY(jsonlogic_stringify_intern(buf, object->entries[index].value));
                }
            }

            return jsonlogic_strbuf_append_utf16(buf, (JsonLogic_Char[]){ '}' }, 1);

        case JsonLogic_Type_Error:
            jsonlogic_strbuf_append_latin1(buf, jsonlogic_get_error_message(jsonlogic_get_error(handle)));
            return handle.intptr;

        default:
            assert(false);
            return JSONLOGIC_ERROR_INTERNAL_ERROR;
    }
}

JsonLogic_Handle jsonlogic_stringify(JsonLogic_Handle value) {
    if (JSONLOGIC_IS_ERROR(value)) {
        return value;
    }

    JsonLogic_StrBuf buf = JSONLOGIC_STRBUF_INIT;
    
    JsonLogic_Error error = jsonlogic_stringify_intern(&buf, value);
    if (error != JSONLOGIC_ERROR_SUCCESS) {
        jsonlogic_strbuf_free(&buf);
        return (JsonLogic_Handle){ .intptr = error };
    }

    return jsonlogic_string_into_handle(jsonlogic_strbuf_take(&buf));
}


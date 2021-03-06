#include "jsonlogic_intern.h"

// this file is included multiple times in json.c with function and type names #defined to make it generic
static JsonLogic_Error jsonlogic_stringify_intern(JsonLogic_StrBuf *buf, JsonLogic_Handle handle) {
    if (JSONLOGIC_IS_NUMBER(handle)) {
        const double number = JSONLOGIC_HNDL_TO_NUM(handle);
        if (isfinite(number)) {
            return jsonlogic_strbuf_append_double(buf, number);
        } else {
            return jsonlogic_strbuf_append_ascii(buf, "null");
        }
    }

    switch (handle & JsonLogic_TypeMask) {
        case JsonLogic_Type_String:
            TRY(jsonlogic_strbuf_append_ascii(buf, "\""));

            const JsonLogic_String *string = JSONLOGIC_CAST_STRING(handle);

            for (size_t index = 0; index < string->size; ++ index) {
                char16_t ch = string->str[index];

                switch (ch) {
                    case u'"':
                        TRY(jsonlogic_strbuf_append_ascii(buf, "\\\""));
                        break;

                    case u'\\':
                        TRY(jsonlogic_strbuf_append_ascii(buf, "\\\\"));
                        break;

                    case u'\b':
                        TRY(jsonlogic_strbuf_append_ascii(buf, "\\b"));
                        break;

                    case u'\f':
                        TRY(jsonlogic_strbuf_append_ascii(buf, "\\f"));
                        break;

                    case u'\n':
                        TRY(jsonlogic_strbuf_append_ascii(buf, "\\n"));
                        break;

                    case u'\r':
                        TRY(jsonlogic_strbuf_append_ascii(buf, "\\r"));
                        break;

                    case u'\t':
                        TRY(jsonlogic_strbuf_append_ascii(buf, "\\t"));
                        break;

                    default:
                        if (ch > 0xff) {
                            char hex[7] = {
                                '\\', 'u',
                                JsonLogic_ToHexMap[(ch >> 12) & 0xF],
                                JsonLogic_ToHexMap[(ch >>  8) & 0xF],
                                JsonLogic_ToHexMap[(ch >>  4) & 0xF],
                                JsonLogic_ToHexMap[ ch        & 0xF],
                                0
                            };

                            TRY(jsonlogic_strbuf_append_ascii(buf, hex));
                        } else {
                            TRY(jsonlogic_strbuf_append_utf16(buf, (char16_t[]){ ch }, 1));
                        }
                        break;
                }
            }

            return jsonlogic_strbuf_append_ascii(buf, "\"");

        case JsonLogic_Type_Boolean:
            if (handle == JSONLOGIC_FALSE) {
                return jsonlogic_strbuf_append_ascii(buf, "false");
            } else {
                return jsonlogic_strbuf_append_ascii(buf, "true");
            }
        case JsonLogic_Type_Null:
            return jsonlogic_strbuf_append_ascii(buf, "null");

        case JsonLogic_Type_Array:
        {
            TRY(jsonlogic_strbuf_append_ascii(buf, "["));

            JsonLogic_Array *array = JSONLOGIC_CAST_ARRAY(handle);
            if (array->size > 0) {
                if (!jsonlogic_stringify_intern(buf, array->items[0])) {
                    return false;
                }
                for (size_t index = 1; index < array->size; ++ index) {
                    TRY(jsonlogic_strbuf_append_ascii(buf, ","));
                    JsonLogic_Handle item = array->items[index];
                    TRY(jsonlogic_stringify_intern(buf, item));
                }
            }

            return jsonlogic_strbuf_append_ascii(buf, "]");
        }
        case JsonLogic_Type_Object:
            TRY(jsonlogic_strbuf_append_ascii(buf, "{"));

            JsonLogic_Object *object = JSONLOGIC_CAST_OBJECT(handle);
            size_t index = 0;
            while (index < object->size && JSONLOGIC_IS_NULL(object->entries[index].key)) {
                index ++;
            }
            if (index < object->size) {
                TRY(jsonlogic_stringify_intern(buf, object->entries[index].key));
                TRY(jsonlogic_strbuf_append_ascii(buf, ":"));
                TRY(jsonlogic_stringify_intern(buf, object->entries[index].value));

                for (++ index; index < object->size; ++ index) {
                    JsonLogic_Handle key = object->entries[index].key;
                    if (!JSONLOGIC_IS_NULL(key)) {
                        TRY(jsonlogic_strbuf_append_ascii(buf, ","));
                        TRY(jsonlogic_stringify_intern(buf, key));
                        TRY(jsonlogic_strbuf_append_ascii(buf, ":"));
                        TRY(jsonlogic_stringify_intern(buf, object->entries[index].value));
                    }
                }
            }

            return jsonlogic_strbuf_append_ascii(buf, "}");

        case JsonLogic_Type_Error:
            return jsonlogic_strbuf_append_ascii(buf, jsonlogic_get_error_message(jsonlogic_get_error(handle)));

        default:
            assert(false);
            return JSONLOGIC_ERROR_INTERNAL_ERROR;
    }
}

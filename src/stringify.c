#include "jsonlogic_intern.h"

// this file is twice included in json.c with functions and typed #defined to make it generic
static JsonLogic_Error jsonlogic_stringify_intern(JsonLogic_StrBuf *buf, JsonLogic_Handle handle) {
    if (JSONLOGIC_IS_NUMBER(handle)) {
        if (isfinite(handle.number)) {
            return jsonlogic_strbuf_append_double(buf, handle.number);
        } else {
            return jsonlogic_strbuf_append_ascii(buf, "null");
        }
    }

    switch (handle.intptr & JsonLogic_TypeMask) {
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
            if (handle.intptr == JsonLogic_False.intptr) {
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
            if (object->size > 0) {
                TRY(jsonlogic_stringify_intern(buf, object->entries[0].key));
                TRY(jsonlogic_strbuf_append_ascii(buf, ":"));
                TRY(jsonlogic_stringify_intern(buf, object->entries[0].value));

                for (size_t index = 1; index < object->size; ++ index) {
                    TRY(jsonlogic_stringify_intern(buf, object->entries[index].key));
                    TRY(jsonlogic_strbuf_append_ascii(buf, ":"));
                    TRY(jsonlogic_stringify_intern(buf, object->entries[index].value));
                }
            }

            return jsonlogic_strbuf_append_ascii(buf, "}");

        case JsonLogic_Type_Error:
            jsonlogic_strbuf_append_ascii(buf, jsonlogic_get_error_message(jsonlogic_get_error(handle)));
            return handle.intptr;

        default:
            assert(false);
            return JSONLOGIC_ERROR_INTERNAL_ERROR;
    }
}

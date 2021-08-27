#include "jsonlogic_intern.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#define JSONLOGIC_CODEPOINT_MAX 0x10FFFF

JsonLogic_Handle jsonlogic_string_from_latin1(const char *str) {
    return jsonlogic_string_from_latin1_sized(str, strlen(str));
}

JsonLogic_Handle jsonlogic_string_from_latin1_sized(const char *str, size_t size) {
    JsonLogic_String *string = malloc(sizeof(JsonLogic_String) - sizeof(JsonLogic_Char) + sizeof(JsonLogic_Char) * size);
    if (string == NULL) {
        return JsonLogic_Null;
    }
    string->refcount = 1;
    string->size     = size;

    for (size_t index = 0; index < size; ++ index) {
        string->str[index] = str[index];
    }

    return (JsonLogic_Handle){ .intptr = ((uintptr_t)string) | JsonLogic_Type_String };
}

JsonLogic_Handle jsonlogic_string_from_utf8(const char *str) {
    return jsonlogic_string_from_utf8_sized(str, strlen(str));
}

#define JSONLOGIC_DECODE_UTF8(STR, SIZE, CODE) \
    for (size_t index = 0; index < (SIZE);) { \
        uint8_t byte1 = (STR)[index ++]; \
        if (byte1 < 0x80) { \
            uint32_t codepoint = byte1; \
            CODE; \
        } else if (byte1 < 0xC2) { \
            /* unexpected continuation or overlong 2-byte sequence, ignored */ \
        } else if (byte1 < 0xE0) { \
            if (index < (SIZE)) { \
                uint8_t byte2 = (STR)[index ++]; \
                if ((byte2 & 0xC0) == 0x80) { \
                    uint32_t codepoint = (uint32_t)(byte1 & 0xF1) << 6 | \
                                         (uint32_t)(byte2 & 0x3F); \
                    CODE; \
                } /* else illegal byte sequence, ignored */ \
            } /* else unexpected end of multibyte sequence, ignored */ \
        } else if (byte1 < 0xF0) { \
            if (index + 1 < (SIZE)) { \
                uint8_t byte2 = (STR)[index ++]; \
                uint8_t byte3 = (STR)[index ++]; \
                \
                if ((byte1 != 0xE0 || byte2 >= 0xA0) && \
                    (byte2 & 0xC0) == 0x80 && \
                    (byte3 & 0xC0) == 0x80) { \
                    uint32_t codepoint = (uint32_t)(byte1 & 0x0F) << 12 | \
                                         (uint32_t)(byte2 & 0x3F) <<  6 | \
                                         (uint32_t)(byte3 & 0x3F); \
                    CODE; \
                } /* else illegal byte sequence, ignored */ \
            } /* else unexpected end of multibyte sequence, ignored */ \
        } else if (byte1 < 0xF8) { \
            if (index + 2 < (SIZE)) { \
                uint8_t byte2 = (STR)[index ++]; \
                uint8_t byte3 = (STR)[index ++]; \
                uint8_t byte4 = (STR)[index ++]; \
                \
                if ((byte1 != 0xF0 || byte2 >= 0x90) && \
                    (byte1 != 0xF4 || byte2 < 0x90) && \
                    (byte2 & 0xC0) == 0x80 && \
                    (byte3 & 0xC0) == 0x80 && \
                    (byte4 & 0xC0) == 0x80) { \
                    uint32_t codepoint = (uint32_t)(byte1 & 0x07) << 18 | \
                                         (uint32_t)(byte2 & 0x3F) << 12 | \
                                         (uint32_t)(byte3 & 0x3F) <<  6 | \
                                         (uint32_t)(byte4 & 0x3F); \
                    CODE; \
                } \
            } /* else unexpected end of multibyte sequence, ignored */ \
        } /* else illegal byte sequence, ignored */ \
    }

JsonLogic_Handle jsonlogic_string_from_utf8_sized(const char *str, size_t size) {
    size_t utf16_size = 0;
    JSONLOGIC_DECODE_UTF8(str, size, {
        if (codepoint < 0x10000) {
            utf16_size += 1;
        } else {
            utf16_size += 2;
        }
    });

    JsonLogic_String *string = malloc(sizeof(JsonLogic_String) - sizeof(JsonLogic_Char) + sizeof(JsonLogic_Char) * utf16_size);
    if (string == NULL) {
        JSONLOGIC_ERROR_MEMORY()
        return JsonLogic_Null;
    }
    string->refcount = 1;
    string->size     = utf16_size;

    size_t utf16_index = 0;
    JSONLOGIC_DECODE_UTF8(str, size, {
        if (codepoint < 0x10000) {
            string->str[utf16_index ++] = codepoint;
        } else {
            string->str[utf16_index ++] = 0xD800 | (codepoint >> 10);
            string->str[utf16_index ++] = 0xDC00 | (codepoint & 0x3FF);
        }
    });
    assert(utf16_index == utf16_size);

    return (JsonLogic_Handle){ .intptr = ((uintptr_t)string) | JsonLogic_Type_String };
}

JsonLogic_Handle jsonlogic_string_from_utf16(const JsonLogic_Char *str) {
    const JsonLogic_Char *ptr = str;
    while (*ptr) ++ ptr;
    size_t size = ptr - str;
    return jsonlogic_string_from_utf16_sized(str, size);
}

JsonLogic_Handle jsonlogic_string_from_utf16_sized(const JsonLogic_Char *str, size_t size) {
    JsonLogic_String *string = malloc(sizeof(JsonLogic_String) - sizeof(JsonLogic_Char) + sizeof(JsonLogic_Char) * size);
    if (string == NULL) {
        JSONLOGIC_ERROR_MEMORY();
        return JsonLogic_Null;
    }
    string->refcount = 1;
    string->size     = size;

    memcpy(string->str, str, size * sizeof(JsonLogic_Char));

    return (JsonLogic_Handle){ .intptr = ((uintptr_t)string) | JsonLogic_Type_String };
}

static JsonLogic_Handle jsonlogic_string_substr(const JsonLogic_String *string, JsonLogic_Handle index, JsonLogic_Handle size) {
    size_t sz_index;
    size_t sz_size;

    double dbl_index = jsonlogic_to_double(index);
    if (isnan(dbl_index)) {
        sz_index = 0;
    } else if (dbl_index < 0) {
        if (-dbl_index >= (double)string->size) {
            sz_index = 0;
        } else {
            sz_index = string->size + (size_t)dbl_index;
        }
    } else {
        if (dbl_index > (double)string->size) {
            sz_index = string->size;
        } else {
            sz_index = (size_t)dbl_index;
        }
    }

    if (JSONLOGIC_IS_NULL(size)) {
        sz_size = sz_index < string->size ? string->size - sz_index : 0;
    } else {
        double dbl_size = jsonlogic_to_double(size);

        if (isnan(dbl_size)) {
            sz_size = 0;
        } else if (dbl_size < 0) {
            if (-dbl_size >= (double)string->size) {
                sz_size = 0;
            } else {
                sz_size = string->size + (size_t)dbl_size;
            }
        } else {
            if (dbl_size > (double)(string->size - sz_index)) {
                sz_size = string->size - sz_index;
            } else {
                sz_size = (size_t)dbl_size;
            }
        }
    }

    JsonLogic_String *new_string = malloc(sizeof(JsonLogic_String) - sizeof(JsonLogic_Char) + sizeof(JsonLogic_Char) * sz_size);

    if (new_string == NULL) {
        JSONLOGIC_ERROR_MEMORY();
        return JsonLogic_Null;
    }

    new_string->refcount = 1;
    new_string->size     = sz_size;

    memcpy(new_string->str, string->str + sz_index, sz_size * sizeof(JsonLogic_Char));

    return (JsonLogic_Handle){ .intptr = ((uintptr_t)new_string) | JsonLogic_Type_String };
}

JsonLogic_Handle jsonlogic_substr(JsonLogic_Handle handle, JsonLogic_Handle index, JsonLogic_Handle size) {
    if (!JSONLOGIC_IS_STRING(handle)) {
        JsonLogic_Handle temp = jsonlogic_to_string(handle);
        if (!JSONLOGIC_IS_STRING(temp)) {
            JSONLOGIC_ERROR_MEMORY();
            return JsonLogic_Null;
        }
        JsonLogic_Handle result = jsonlogic_string_substr(JSONLOGIC_CAST_STRING(temp), index, size);
        jsonlogic_decref(temp);
        return result;
    }

    JsonLogic_String *string = JSONLOGIC_CAST_STRING(handle);

    return jsonlogic_string_substr(string, index, size);
}

const JsonLogic_Char *jsonlogic_get_utf16(JsonLogic_Handle handle, size_t *sizeptr) {
    if (!JSONLOGIC_IS_STRING(handle)) {
        return NULL;
    }

    JsonLogic_String *string = JSONLOGIC_CAST_STRING(handle);
    if (sizeptr != NULL) {
        *sizeptr = string->size;
    }

    return string->str;
}

bool jsonlogic_buffer_ensure(JsonLogic_Buffer *buf, size_t want_free_size) {
    size_t used = buf->data == NULL ? 0 : buf->data->size;
    size_t has_free_size = buf->size - used;

    if (has_free_size < want_free_size) {
        size_t add_size = want_free_size - has_free_size;
        size_t new_size = buf->size + (add_size < JSONLOGIC_CHUNK_SIZE ? JSONLOGIC_CHUNK_SIZE : add_size);
        JsonLogic_String *new_string = realloc(buf->data, sizeof(JsonLogic_String) - sizeof(JsonLogic_Char) + sizeof(JsonLogic_Char) * new_size);
        if (new_string == NULL) {
            return false;
        }
        buf->data = new_string;
        buf->size = new_size;
    }
    return true;
}

bool jsonlogic_buffer_append_latin1(JsonLogic_Buffer *buf, const char *str) {
    size_t size = strlen(str);
    if (!jsonlogic_buffer_ensure(buf, size)) {
        return false;
    }

    size_t outindex = buf->data->size;
    for (size_t index = 0; index < size; ++ index) {
        buf->data->str[outindex ++] = str[index];
    }
    buf->data->size += size;

    return false;
}

bool jsonlogic_buffer_append_utf16(JsonLogic_Buffer *buf, const JsonLogic_Char *str, size_t size) {
    if (!jsonlogic_buffer_ensure(buf, size)) {
        return false;
    }

    size_t outindex = buf->data->size;
    for (size_t index = 0; index < size; ++ index) {
        buf->data->str[outindex ++] = str[index];
    }
    buf->data->size += size;

    return true;
}

JSONLOGIC_DEF_UTF16(JSONLOGIC_OBJECT_STRING,       '[', 'o', 'b', 'j', 'e', 'c', 't', ' ', 'O', 'b', 'j', 'e', 'c', 't', ']')
JSONLOGIC_DEF_UTF16(JSONLOGIC_NULL_STRING,         'n', 'u', 'l', 'l')
JSONLOGIC_DEF_UTF16(JSONLOGIC_TRUE_STRING,         't', 'r', 'u', 'e')
JSONLOGIC_DEF_UTF16(JSONLOGIC_FALSE_STRING,        'f', 'a', 'l', 's', 'e')
JSONLOGIC_DEF_UTF16(JSONLOGIC_NAN_STRING,          'N', 'a', 'N')
JSONLOGIC_DEF_UTF16(JSONLOGIC_INFINITY_STRING,     'I', 'n', 'f', 'i', 'n', 'i', 't', 'y')
JSONLOGIC_DEF_UTF16(JSONLOGIC_POS_INFINITY_STRING, '+', 'I', 'n', 'f', 'i', 'n', 'i', 't', 'y')
JSONLOGIC_DEF_UTF16(JSONLOGIC_NEG_INFINITY_STRING, '-', 'I', 'n', 'f', 'i', 'n', 'i', 't', 'y')

bool jsonlogic_buffer_append_double(JsonLogic_Buffer *buf, double value) {
    if (isnan(value)) {
        return jsonlogic_buffer_append_utf16(buf, JSONLOGIC_NAN_STRING, JSONLOGIC_NAN_STRING_SIZE);
    }

    if (isinf(value)) {
        if (value > 0) {
            return jsonlogic_buffer_append_utf16(buf, JSONLOGIC_INFINITY_STRING, JSONLOGIC_INFINITY_STRING_SIZE);
        } else {
            return jsonlogic_buffer_append_utf16(buf, JSONLOGIC_NEG_INFINITY_STRING, JSONLOGIC_NEG_INFINITY_STRING_SIZE);
        }
    }

    char latin1[128];
    int count = snprintf(latin1, sizeof(buf), "%g", value);
    if (count >= sizeof(latin1)) {
        size_t size = (size_t)count + 1;
        char *latin1 = malloc(size);
        if (latin1 == NULL) {
            return false;
        }
        snprintf(latin1, size, "%g", value);
        bool result = jsonlogic_buffer_append_latin1(buf, latin1);
        free(latin1);
        return result;
    }
    return jsonlogic_buffer_append_latin1(buf, latin1);
}

bool jsonlogic_buffer_append(JsonLogic_Buffer *buf, JsonLogic_Handle handle) {
    if (handle.intptr < JsonLogic_MaxNumber) {
        return jsonlogic_buffer_append_double(buf, handle.number);
    }

    switch (handle.intptr & JsonLogic_TypeMask) {
        case JsonLogic_Type_String:
        {
            const JsonLogic_String *string = JSONLOGIC_CAST_STRING(handle);
            return jsonlogic_buffer_append_utf16(buf, string->str, string->size);
        }
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
            JsonLogic_Array *array = JSONLOGIC_CAST_ARRAY(handle);
            if (array->size > 0) {
                if (!jsonlogic_buffer_append(buf, array->items[0])) {
                    return false;
                }
                for (size_t index = 1; index < array->size; ++ index) {
                    if (!jsonlogic_buffer_append_utf16(buf, (JsonLogic_Char[]){','}, 1)) {
                        return false;
                    }
                    JsonLogic_Handle item = array->items[index];
                    if (!JSONLOGIC_IS_NULL(item) && !jsonlogic_buffer_append(buf, item)) {
                        return false;
                    }
                }
            }
            return true;
        }
        case JsonLogic_Type_Object:
            return jsonlogic_buffer_append_utf16(buf, JSONLOGIC_OBJECT_STRING, JSONLOGIC_OBJECT_STRING_SIZE);

        default:
            assert(false);
            return false;
    }
}

JsonLogic_String *jsonlogic_buffer_take(JsonLogic_Buffer *buf) {
    JsonLogic_String *string = buf->data;
    if (string == NULL) {
        string = malloc(sizeof(JsonLogic_String) - sizeof(JsonLogic_Char));
        if (string != NULL) {
            string->refcount = 1;
            string->size     = 0;
        }
    } else {
        // shrink to fit
        string = realloc(string, sizeof(JsonLogic_String) - sizeof(JsonLogic_Char) + sizeof(JsonLogic_Char) * string->size);
        if (string == NULL) {
            // should not happen
            string = buf->data;
        }
    }
    buf->size = 0;
    buf->data = NULL;
    return string;
}

void jsonlogic_buffer_free(JsonLogic_Buffer *buf) {
    free(buf->data);
    buf->data = NULL;
    buf->size = 0;
}

JsonLogic_Handle jsonlogic_to_string(JsonLogic_Handle handle) {
    if (JSONLOGIC_IS_STRING(handle)) {
        jsonlogic_incref(handle);
        return handle;
    }

    JsonLogic_Buffer buf = JSONLOGIC_BUFFER_INIT;
    if (!jsonlogic_buffer_append(&buf, handle)) {
        jsonlogic_buffer_free(&buf);
        JSONLOGIC_ERROR_MEMORY()
        return JsonLogic_Null;
    }

    JsonLogic_String *string = jsonlogic_buffer_take(&buf);
    jsonlogic_buffer_free(&buf);
    if (string == NULL) {
        JSONLOGIC_ERROR_MEMORY()
        return JsonLogic_Null;
    }

    return (JsonLogic_Handle){ .intptr = ((uintptr_t)string) | JsonLogic_Type_String };
}

bool jsonlogic_string_equals(const JsonLogic_String *a, const JsonLogic_String *b) {
    if (a->size != b->size) {
        return false;
    }

    return memcmp(a->str, b->str, a->size * sizeof(JsonLogic_Char)) == 0;
}


bool jsonlogic_utf16_equals(const JsonLogic_Char *a, size_t asize, const JsonLogic_Char *b, size_t bsize) {
    if (asize != bsize) {
        return false;
    }

    return memcmp(a, b, asize * sizeof(JsonLogic_Char)) == 0;
}

int jsonlogic_utf16_compare(const JsonLogic_Char *a, size_t asize, const JsonLogic_Char *b, size_t bsize) {
    size_t minsize = asize < bsize ? asize : bsize;

    for (size_t index = 0; index < minsize; ++ index) {
        JsonLogic_Char ach = a[index];
        JsonLogic_Char bch = b[index];

        int cmp = ach - bch;
        if (cmp != 0) {
            return cmp;
        }
    }

    return asize > bsize ? 1 : asize < bsize ? -1 : 0;
}

int jsonlogic_string_compare(const JsonLogic_String *a, const JsonLogic_String *b) {
    return jsonlogic_utf16_compare(a->str, a->size, b->str, b->size);
}

void jsonlogic_string_free(JsonLogic_String *string) {
    free(string);
}

size_t jsonlogic_string_to_index(const JsonLogic_String *string) {
    size_t value = 0;

    for (size_t index = 0; index < string->size; ++ index) {
        if (value >= SIZE_MAX / 10) {
            return SIZE_MAX;
        }
        JsonLogic_Char ch = string->str[index];
        if (ch < '0' || ch > '1') {
            return SIZE_MAX;
        }
        int ord = ch - '0';
        value *= 10;
        if (value > SIZE_MAX - ord) {
            return SIZE_MAX;
        }
        value += ord;
    }

    return value;
}

#define JSONLOGIC_DECODE_UTF16(STR, SIZE, CODE) \
    for (size_t index = 0; index < (SIZE);) { \
        JsonLogic_Char w1 = (STR)[index ++]; \
        switch (w1 & 0xfc00) { \
            case 0xd800: \
                if (index < (SIZE)) { \
                    JsonLogic_Char w2 = (STR)[index ++]; \
                    \
                    if ((w2 & 0xfc00) == 0xdc00) { \
                        uint32_t hi = w1 & 0x3FF; \
                        uint32_t lo = w2 & 0x3FF; \
                        uint32_t codepoint = (hi << 10) | lo | 0x10000; \
                        \
                        if (codepoint <= JSONLOGIC_CODEPOINT_MAX) { \
                            CODE; \
                        } /* else illegal code point, ignored */ \
                    } /* else illegal code unit, ignored */ \
                } /* else illegal code unit, ignored */ \
                break; \
            \
            case 0x0000: \
            { \
                uint32_t codepoint = w1; \
                CODE; \
                break; \
            } \
            default: \
                /* illegal code unit, ignored */ \
                break; \
        } \
    }

char *jsonlogic_utf16_to_utf8(const JsonLogic_Char *str, size_t size) {
    size_t utf8_size = 1;
    JSONLOGIC_DECODE_UTF16(str, size, {
        if (codepoint > 0x10000) {
            utf8_size += 4;
        } else if (codepoint >= 0x800) {
            utf8_size += 3;
        } else if (codepoint >= 0x80) {
            utf8_size += 2;
        } else {
            utf8_size += 1;
        }
    });

    char *utf8 = malloc(utf8_size);
    if (utf8 == NULL) {
        return NULL;
    }

    size_t utf8_index = 0;
    JSONLOGIC_DECODE_UTF16(str, size, {
        if (codepoint > 0x10000) {
            utf8[utf8_index ++] =  (codepoint >> 18)         | 0xF0;
            utf8[utf8_index ++] = ((codepoint >> 12) & 0x3F) | 0x80;
            utf8[utf8_index ++] = ((codepoint >>  6) & 0x3F) | 0x80;
            utf8[utf8_index ++] =  (codepoint        & 0x3F) | 0x80;
        } else if (codepoint >= 0x800) {
            utf8[utf8_index ++] =  (codepoint >> 12)         | 0xE0;
            utf8[utf8_index ++] = ((codepoint >>  6) & 0x3F) | 0x80;
            utf8[utf8_index ++] =  (codepoint        & 0x3F) | 0x80;
        } else if (codepoint >= 0x80) {
            utf8[utf8_index ++] = (codepoint >> 6)   | 0xC0;
            utf8[utf8_index ++] = (codepoint & 0x3F) | 0x80;
        } else {
            utf8[utf8_index ++] = codepoint;
        }
    });

    utf8[utf8_index] = 0;
    assert(utf8_index + 1 == utf8_size);

    return utf8;
}

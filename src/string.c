#include "jsonlogic_intern.h"

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#define JSONLOGIC_CODEPOINT_MAX 0x10FFFF

JsonLogic_Handle jsonlogic_string_into_handle(JsonLogic_String *string);
JsonLogic_Error jsonlogic_strbuf_append_ascii(JsonLogic_StrBuf *buf, const char *str);
JsonLogic_Error jsonlogic_utf8buf_append_ascii(JsonLogic_Utf8Buf *buf, const char *str);
int jsonlogic_string_compare(const JsonLogic_String *a, const JsonLogic_String *b);

size_t jsonlogic_utf16_len(const char16_t *key) {
    if (key == NULL) {
        return 0;
    }
    const char16_t *ptr = key;
    while (ptr < key) ++ ptr;
    return ptr - key;
}

JsonLogic_Handle jsonlogic_empty_string() {
    JsonLogic_String *string = malloc(sizeof(JsonLogic_String) - sizeof(char16_t));
    if (string == NULL) {
        JSONLOGIC_ERROR_MEMORY();
        return JsonLogic_Error_OutOfMemory;
    }

    string->refcount = 1;
    string->size     = 0;

    return (JsonLogic_Handle){ .intptr = ((uintptr_t)string) | JsonLogic_Type_String };
}

JsonLogic_Handle jsonlogic_string_from_latin1(const char *str) {
    return jsonlogic_string_from_latin1_sized(str, strlen(str));
}

JsonLogic_Handle jsonlogic_string_from_latin1_sized(const char *str, size_t size) {
    JsonLogic_String *string = malloc(sizeof(JsonLogic_String) - sizeof(char16_t) + sizeof(char16_t) * size);
    if (string == NULL) {
        JSONLOGIC_ERROR_MEMORY();
        return JsonLogic_Error_OutOfMemory;
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

    JsonLogic_String *string = malloc(sizeof(JsonLogic_String) - sizeof(char16_t) + sizeof(char16_t) * utf16_size);
    if (string == NULL) {
        JSONLOGIC_ERROR_MEMORY();
        return JsonLogic_Error_OutOfMemory;
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

JsonLogic_Handle jsonlogic_string_from_utf16(const char16_t *str) {
    const char16_t *ptr = str;
    while (*ptr) ++ ptr;
    size_t size = ptr - str;
    return jsonlogic_string_from_utf16_sized(str, size);
}

JsonLogic_Handle jsonlogic_string_from_utf16_sized(const char16_t *str, size_t size) {
    JsonLogic_String *string = malloc(sizeof(JsonLogic_String) - sizeof(char16_t) + sizeof(char16_t) * size);
    if (string == NULL) {
        JSONLOGIC_ERROR_MEMORY();
        return JsonLogic_Error_OutOfMemory;
    }
    string->refcount = 1;
    string->size     = size;

    memcpy(string->str, str, size * sizeof(char16_t));

    return (JsonLogic_Handle){ .intptr = ((uintptr_t)string) | JsonLogic_Type_String };
}

static JsonLogic_Handle jsonlogic_string_substr(const JsonLogic_String *string, JsonLogic_Handle index, JsonLogic_Handle size) {
    if (JSONLOGIC_IS_ERROR(index)) {
        return index;
    }

    if (JSONLOGIC_IS_ERROR(size)) {
        return size;
    }

    size_t sz_index;
    size_t sz_size;

    double dbl_index = jsonlogic_to_double(index);
    if (isnan(dbl_index)) {
        sz_index = 0;
    } else if (dbl_index < 0) {
        sz_index = (size_t)-dbl_index;
        if (sz_index >= string->size) {
            sz_index = 0;
        } else {
            sz_index = string->size - sz_index;
        }
    } else {
        sz_index = (size_t)dbl_index;
        if (sz_index > string->size) {
            sz_index = string->size;
        }
    }

    if (JSONLOGIC_IS_NULL(size)) {
        sz_size = sz_index < string->size ? string->size - sz_index : 0;
    } else {
        double dbl_size = jsonlogic_to_double(size);

        if (isnan(dbl_size)) {
            sz_size = 0;
        } else {
            size_t max_size = string->size - sz_index;
            if (dbl_size < 0) {
                sz_size = (size_t)-dbl_size;
                if (sz_size >= max_size) {
                    sz_size = 0;
                } else {
                    sz_size = max_size - sz_size;
                }
            } else {
                sz_size = (size_t)dbl_size;
                if (sz_size > max_size) {
                    sz_size = max_size;
                }
            }
        }
    }

    JsonLogic_String *new_string = malloc(sizeof(JsonLogic_String) - sizeof(char16_t) + sizeof(char16_t) * sz_size);
    if (new_string == NULL) {
        JSONLOGIC_ERROR_MEMORY();
        return JsonLogic_Error_OutOfMemory;
    }

    new_string->refcount = 1;
    new_string->size     = sz_size;

    memcpy(new_string->str, string->str + sz_index, sz_size * sizeof(char16_t));

    return (JsonLogic_Handle){ .intptr = ((uintptr_t)new_string) | JsonLogic_Type_String };
}

JsonLogic_Handle jsonlogic_substr(JsonLogic_Handle handle, JsonLogic_Handle index, JsonLogic_Handle size) {
    if (!JSONLOGIC_IS_STRING(handle)) {
        JsonLogic_Handle temp = jsonlogic_to_string(handle);
        if (JSONLOGIC_IS_ERROR(temp)) {
            return temp;
        }
        JsonLogic_Handle result = jsonlogic_string_substr(JSONLOGIC_CAST_STRING(temp), index, size);
        jsonlogic_decref(temp);
        return result;
    }

    JsonLogic_String *string = JSONLOGIC_CAST_STRING(handle);

    return jsonlogic_string_substr(string, index, size);
}

const char16_t *jsonlogic_get_string_content(JsonLogic_Handle handle, size_t *sizeptr) {
    if (!JSONLOGIC_IS_STRING(handle)) {
        return NULL;
    }

    JsonLogic_String *string = JSONLOGIC_CAST_STRING(handle);
    if (sizeptr != NULL) {
        *sizeptr = string->size;
    }

    return string->str;
}

JsonLogic_Error jsonlogic_strbuf_ensure(JsonLogic_StrBuf *buf, size_t want_free_size) {
    size_t used = buf->string == NULL ? 0 : buf->string->size;
    size_t has_free_size = buf->capacity - used;

    if (has_free_size < want_free_size) {
        size_t add_size = want_free_size - has_free_size;
        size_t new_size = buf->capacity + (add_size < JSONLOGIC_CHUNK_SIZE ? JSONLOGIC_CHUNK_SIZE : add_size);
        JsonLogic_String *new_string = realloc(buf->string, sizeof(JsonLogic_String) - sizeof(char16_t) + sizeof(char16_t) * new_size);
        if (new_string == NULL) {
            JSONLOGIC_ERROR_MEMORY();
            return JSONLOGIC_ERROR_OUT_OF_MEMORY;
        }
        if (buf->string == NULL) {
            new_string->refcount = 1;
            new_string->size     = 0;
        }
        buf->string   = new_string;
        buf->capacity = new_size;
    }
    return JSONLOGIC_ERROR_SUCCESS;
}

JsonLogic_Error jsonlogic_strbuf_append_latin1(JsonLogic_StrBuf *buf, const char *str) {
    size_t size = strlen(str);
    TRY(jsonlogic_strbuf_ensure(buf, size));

    size_t outindex = buf->string->size;
    for (size_t index = 0; index < size; ++ index) {
        buf->string->str[outindex ++] = str[index];
    }
    buf->string->size += size;

    return JSONLOGIC_ERROR_SUCCESS;
}

JsonLogic_Error jsonlogic_strbuf_append_utf16(JsonLogic_StrBuf *buf, const char16_t *str, size_t size) {
    TRY(jsonlogic_strbuf_ensure(buf, size));

    size_t outindex = buf->string->size;
    for (size_t index = 0; index < size; ++ index) {
        buf->string->str[outindex ++] = str[index];
    }
    buf->string->size += size;

    return JSONLOGIC_ERROR_SUCCESS;
}

JSONLOGIC_DEF_UTF16(JSONLOGIC_OBJECT_STRING,       u"[object Object]")
JSONLOGIC_DEF_UTF16(JSONLOGIC_NULL_STRING,         u"null")
JSONLOGIC_DEF_UTF16(JSONLOGIC_TRUE_STRING,         u"true")
JSONLOGIC_DEF_UTF16(JSONLOGIC_FALSE_STRING,        u"false")
JSONLOGIC_DEF_UTF16(JSONLOGIC_NAN_STRING,          u"NaN")
JSONLOGIC_DEF_UTF16(JSONLOGIC_INFINITY_STRING,     u"Infinity")
JSONLOGIC_DEF_UTF16(JSONLOGIC_POS_INFINITY_STRING, u"+Infinity")
JSONLOGIC_DEF_UTF16(JSONLOGIC_NEG_INFINITY_STRING, u"-Infinity")

JsonLogic_Error jsonlogic_strbuf_append_double(JsonLogic_StrBuf *buf, double value) {
    if (isnan(value)) {
        return jsonlogic_strbuf_append_utf16(buf, JSONLOGIC_NAN_STRING, JSONLOGIC_NAN_STRING_SIZE);
    }

    if (isinf(value)) {
        if (value > 0) {
            return jsonlogic_strbuf_append_utf16(buf, JSONLOGIC_INFINITY_STRING, JSONLOGIC_INFINITY_STRING_SIZE);
        } else {
            return jsonlogic_strbuf_append_utf16(buf, JSONLOGIC_NEG_INFINITY_STRING, JSONLOGIC_NEG_INFINITY_STRING_SIZE);
        }
    }

    char latin1[128];
    int count = snprintf(latin1, sizeof(buf), "%g", value);
    if (count < 0) {
        return JSONLOGIC_ERROR_INTERNAL_ERROR;
    } else if (count >= sizeof(latin1)) {
        size_t size = (size_t)count + 1;
        char *latin1 = malloc(size);
        if (latin1 == NULL) {
            JSONLOGIC_ERROR_MEMORY();
            return JSONLOGIC_ERROR_OUT_OF_MEMORY;
        }
        snprintf(latin1, size, "%g", value);
        JsonLogic_Error result = jsonlogic_strbuf_append_latin1(buf, latin1);
        free(latin1);
        return result;
    }
    return jsonlogic_strbuf_append_latin1(buf, latin1);
}

JsonLogic_Error jsonlogic_strbuf_append(JsonLogic_StrBuf *buf, JsonLogic_Handle handle) {
    if (JSONLOGIC_IS_NUMBER(handle)) {
        return jsonlogic_strbuf_append_double(buf, handle.number);
    }

    switch (handle.intptr & JsonLogic_TypeMask) {
        case JsonLogic_Type_String:
        {
            const JsonLogic_String *string = JSONLOGIC_CAST_STRING(handle);
            return jsonlogic_strbuf_append_utf16(buf, string->str, string->size);
        }
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
            JsonLogic_Array *array = JSONLOGIC_CAST_ARRAY(handle);
            for (size_t index = 0; index < array->size; ++ index) {
                JsonLogic_Handle item = array->items[index];
                if (!JSONLOGIC_IS_NULL(item)) {
                    TRY(jsonlogic_strbuf_append(buf, item));
                }
            }
            return true;
        }
        case JsonLogic_Type_Object:
            return jsonlogic_strbuf_append_utf16(buf, JSONLOGIC_OBJECT_STRING, JSONLOGIC_OBJECT_STRING_SIZE);

        case JsonLogic_Type_Error:
            jsonlogic_strbuf_append_latin1(buf, jsonlogic_get_error_message(jsonlogic_get_error(handle)));
            return handle.intptr;

        default:
            assert(false);
            return JSONLOGIC_ERROR_INTERNAL_ERROR;
    }
}

JsonLogic_String *jsonlogic_strbuf_take(JsonLogic_StrBuf *buf) {
    JsonLogic_String *string = buf->string;
    if (string == NULL) {
        string = malloc(sizeof(JsonLogic_String) - sizeof(char16_t));
        if (string == NULL) {
            JSONLOGIC_ERROR_MEMORY();
        } else {
            string->refcount = 1;
            string->size     = 0;
        }
    } else {
        // shrink to fit
        string = realloc(string, sizeof(JsonLogic_String) - sizeof(char16_t) + sizeof(char16_t) * string->size);
        if (string == NULL) {
            // should not happen
            JSONLOGIC_ERROR_MEMORY();
            string = buf->string;
        }
        assert(string->refcount == 1);
    }
    buf->capacity = 0;
    buf->string   = NULL;
    return string;
}

void jsonlogic_strbuf_free(JsonLogic_StrBuf *buf) {
    jsonlogic_string_free(buf->string);
    buf->string = NULL;
    buf->capacity = 0;
}

JsonLogic_Handle jsonlogic_to_string(JsonLogic_Handle handle) {
    if (JSONLOGIC_IS_STRING(handle)) {
        return jsonlogic_incref(handle);
    }

    if (JSONLOGIC_IS_ERROR(handle)) {
        return handle;
    }

    JsonLogic_StrBuf buf = JSONLOGIC_STRBUF_INIT;
    JsonLogic_Error error = jsonlogic_strbuf_append(&buf, handle);
    if (error != JSONLOGIC_ERROR_SUCCESS) {
        jsonlogic_strbuf_free(&buf);
        return (JsonLogic_Handle){ .intptr = error };
    }

    JsonLogic_String *string = jsonlogic_strbuf_take(&buf);
    jsonlogic_strbuf_free(&buf);
    if (string == NULL) {
        JSONLOGIC_ERROR_MEMORY();
        return JsonLogic_Error_OutOfMemory;
    }

    return (JsonLogic_Handle){ .intptr = ((uintptr_t)string) | JsonLogic_Type_String };
}

bool jsonlogic_string_equals(const JsonLogic_String *a, const JsonLogic_String *b) {
    if (a->size != b->size) {
        return false;
    }

    return memcmp(a->str, b->str, a->size * sizeof(char16_t)) == 0;
}


bool jsonlogic_utf16_equals(const char16_t *a, size_t asize, const char16_t *b, size_t bsize) {
    if (asize != bsize) {
        return false;
    }

    return memcmp(a, b, asize * sizeof(char16_t)) == 0;
}

int jsonlogic_utf16_compare(const char16_t *a, size_t asize, const char16_t *b, size_t bsize) {
    size_t minsize = asize < bsize ? asize : bsize;

    for (size_t index = 0; index < minsize; ++ index) {
        char16_t ach = a[index];
        char16_t bch = b[index];

        int cmp = (int)ach - (int)bch;
        if (cmp != 0) {
            return cmp;
        }
    }

    return asize > bsize ? 1 : asize < bsize ? -1 : 0;
}

void jsonlogic_string_free(JsonLogic_String *string) {
    free(string);
}

size_t jsonlogic_string_to_index(const JsonLogic_String *string);

size_t jsonlogic_utf16_to_index(const char16_t *str, size_t size) {
    size_t value = 0;

    for (size_t index = 0; index < size; ++ index) {
        if (value >= SIZE_MAX / 10) {
            return SIZE_MAX;
        }
        char16_t ch = str[index];
        if (ch < u'0' || ch > u'9') {
            return SIZE_MAX;
        }
        int ord = ch - u'0';
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
        char16_t w1 = (STR)[index ++]; \
        switch (w1 & 0xfc00) { \
            case 0xd800: \
                if (index < (SIZE)) { \
                    char16_t w2 = (STR)[index ++]; \
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

#define JSONLOGIC_ENCODE_UTF8(CODEPOINT, BUF, INDEX) \
    if ((CODEPOINT) > 0x10000) { \
        (BUF)[(INDEX) ++] =  ((CODEPOINT) >> 18)         | 0xF0; \
        (BUF)[(INDEX) ++] = (((CODEPOINT) >> 12) & 0x3F) | 0x80; \
        (BUF)[(INDEX) ++] = (((CODEPOINT) >>  6) & 0x3F) | 0x80; \
        (BUF)[(INDEX) ++] =  ((CODEPOINT)        & 0x3F) | 0x80; \
    } else if ((CODEPOINT) >= 0x800) { \
        (BUF)[(INDEX) ++] =  ((CODEPOINT) >> 12)         | 0xE0; \
        (BUF)[(INDEX) ++] = (((CODEPOINT) >>  6) & 0x3F) | 0x80; \
        (BUF)[(INDEX) ++] =  ((CODEPOINT)        & 0x3F) | 0x80; \
    } else if ((CODEPOINT) >= 0x80) { \
        (BUF)[(INDEX) ++] = ((CODEPOINT) >> 6)   | 0xC0; \
        (BUF)[(INDEX) ++] = ((CODEPOINT) & 0x3F) | 0x80; \
    } else { \
        (BUF)[(INDEX) ++] = (CODEPOINT); \
    }

static inline size_t jsonlogic_utf16_to_utf8_size(const char16_t *str, size_t size) {
    size_t utf8_size = 0;
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
    return utf8_size;
}

char *jsonlogic_utf16_to_utf8(const char16_t *str, size_t size) {
    size_t utf8_size = jsonlogic_utf16_to_utf8_size(str, size);

    char *utf8 = malloc(utf8_size + 1);
    if (utf8 == NULL) {
        JSONLOGIC_ERROR_MEMORY();
        return NULL;
    }

    size_t utf8_index = 0;
    JSONLOGIC_DECODE_UTF16(str, size, JSONLOGIC_ENCODE_UTF8(codepoint, utf8, utf8_index));

    utf8[utf8_index] = 0;
    assert(utf8_index == utf8_size);

    return utf8;
}

JsonLogic_Error jsonlogic_print_utf16(FILE *stream, const char16_t *str, size_t size) {
    JSONLOGIC_DECODE_UTF16(str, size, {
        char utf8[6];
        size_t utf8_index = 0;
        JSONLOGIC_ENCODE_UTF8(codepoint, utf8, utf8_index);
        if (fwrite(utf8, utf8_index, 1, stream) != 1) {
            return JSONLOGIC_ERROR_IO_ERROR;
        }
    });

    return JSONLOGIC_ERROR_SUCCESS;
}

JsonLogic_Error jsonlogic_println_utf16(FILE *stream, const char16_t *str, size_t size) {
    JsonLogic_Error result = jsonlogic_print_utf16(stream, str, size);
    if (result == JSONLOGIC_ERROR_SUCCESS) {
        if (fputc('\n', stream) == EOF) {
            return JSONLOGIC_ERROR_IO_ERROR;
        }
    }
    return result;
}

const char16_t *jsonlogic_find_char(const char16_t *str, size_t size, char16_t ch) {
    for (size_t index = 0; index < size; ++ index) {
        if (str[index] == ch) {
            return str + index;
        }
    }
    return NULL;
}

JsonLogic_Error jsonlogic_utf8buf_ensure(JsonLogic_Utf8Buf *buf, size_t want_free_size) {
    size_t has_free_size = buf->capacity - buf->used;

    if (has_free_size < want_free_size) {
        size_t add_size = want_free_size - has_free_size;
        size_t new_size = buf->capacity + (add_size < JSONLOGIC_CHUNK_SIZE ? JSONLOGIC_CHUNK_SIZE : add_size);
        char *new_string = realloc(buf->string, new_size);
        if (new_string == NULL) {
            JSONLOGIC_ERROR_MEMORY();
            return JSONLOGIC_ERROR_OUT_OF_MEMORY;
        }
        buf->string   = new_string;
        buf->capacity = new_size;
    }
    return JSONLOGIC_ERROR_SUCCESS;
}

JsonLogic_Error jsonlogic_utf8buf_append_utf8(JsonLogic_Utf8Buf *buf, const char *str) {
    size_t size = strlen(str);
    TRY(jsonlogic_utf8buf_ensure(buf, size));

    memcpy(buf->string + buf->used, str, size);
    buf->used += size;

    return JSONLOGIC_ERROR_SUCCESS;
}

JsonLogic_Error jsonlogic_utf8buf_append_utf16(JsonLogic_Utf8Buf *buf, const char16_t *str, size_t size) {
    size_t utf8_size = jsonlogic_utf16_to_utf8_size(str, size);
    TRY(jsonlogic_utf8buf_ensure(buf, utf8_size));

    char *utf8 = buf->string + buf->used;
    size_t utf8_index = 0;
    JSONLOGIC_DECODE_UTF16(str, size, JSONLOGIC_ENCODE_UTF8(codepoint, utf8, utf8_index));

    assert(utf8_index == utf8_size);
    buf->used += utf8_size;

    return JSONLOGIC_ERROR_SUCCESS;
}

JsonLogic_Error jsonlogic_utf8buf_append_double(JsonLogic_Utf8Buf *buf, double value) {
    if (isnan(value)) {
        return jsonlogic_utf8buf_append_utf8(buf, "NaN");
    }

    if (isinf(value)) {
        if (value > 0) {
            return jsonlogic_utf8buf_append_utf8(buf, "Infinity");
        } else {
            return jsonlogic_utf8buf_append_utf8(buf, "-Infinity");
        }
    }

    size_t has_free = buf->capacity - buf->used;
    int count = snprintf(buf->string + buf->used, has_free, "%g", value);
    if (count < 0) {
        JSONLOGIC_DEBUG("snprintf(buf, %" PRIuPTR ", \"%%g\", %g) error: %s", has_free, value, strerror(errno));
        return JSONLOGIC_ERROR_INTERNAL_ERROR;
    } else if (count >= has_free) {
        // snprintf() always writes the terminating NULL byte (if maxlen > 0)
        size_t need_free = (size_t)count + 1;
        TRY(jsonlogic_utf8buf_ensure(buf, need_free));
        snprintf(buf->string + buf->used, need_free, "%g", value);
    }
    buf->used += (size_t)count;

    return JSONLOGIC_ERROR_SUCCESS;
}

char *jsonlogic_utf8buf_take(JsonLogic_Utf8Buf *buf) {
    char *utf8;
    if (buf->string == NULL) {
        utf8 = malloc(1);
        if (utf8 == NULL) {
            JSONLOGIC_ERROR_MEMORY();
            return NULL;
        } else {
            utf8[0] = 0;
        }
    } else {
        if (jsonlogic_utf8buf_ensure(buf, 1) != JSONLOGIC_ERROR_SUCCESS) {
            return NULL;
        }
        buf->string[buf->used ++] = 0;

        // shrink to fit
        utf8 = realloc(buf->string, buf->used);
        if (utf8 == NULL) {
            // should not happen
            JSONLOGIC_ERROR_MEMORY();
            utf8 = buf->string;
        }
    }

    buf->string   = NULL;
    buf->capacity = 0;
    buf->used     = 0;

    return utf8;
}

void jsonlogic_utf8buf_free(JsonLogic_Utf8Buf *buf) {
    free(buf->string);
    buf->capacity = 0;
    buf->used     = 0;
}

JsonLogic_Error jsonlogic_print(FILE *stream, JsonLogic_Handle handle) {
    if (JSONLOGIC_IS_ERROR(handle)) {
        if (fputs(jsonlogic_get_error_message(handle.intptr), stream) < 0) {
            return JSONLOGIC_ERROR_IO_ERROR;
        }
        return JSONLOGIC_ERROR_SUCCESS;
    }
    return jsonlogic_stringify_file(stream, handle);
}

JsonLogic_Error jsonlogic_println(FILE *stream, JsonLogic_Handle handle) {
    JsonLogic_Error result = jsonlogic_print(stream, handle);
    if (result == JSONLOGIC_ERROR_SUCCESS) {
        if (fputc('\n', stream) == EOF) {
            return JSONLOGIC_ERROR_IO_ERROR;
        }
    }
    return result;
}

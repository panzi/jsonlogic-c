#include "jsonlogic_intern.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <inttypes.h>

JsonLogic_Handle jsonlogic_parse(const char *str, JsonLogic_LineInfo *infoptr) {
    return jsonlogic_parse_sized(str, strlen(str), infoptr);
}

JsonLogic_LineInfo jsonlogic_get_lineinfo(const char *str, size_t size, size_t index) {
    if (index > size) {
        index = size;
    }

    size_t lineno = 1;
    const char *end = str + index;
    const char *linestart = str;
    for (const char *ptr = str; ptr < end; ++ ptr) {
        if (*ptr == '\n') {
            ++ lineno;
            linestart = ptr + 1;
        }
    }

    size_t column = 1 + (size_t)(end - linestart);

    return (JsonLogic_LineInfo){
        .index  = index,
        .lineno = lineno,
        .column = column,
    };
}

typedef enum JsonLogic_RootParser {
    JsonLogic_ParserState_Error = 0, // for it to auto-initialize with error
    JsonLogic_ParserState_Start,
    JsonLogic_ParserState_End,
    JsonLogic_ParserState_String,
    JsonLogic_ParserState_Number,
    JsonLogic_ParserState_Null,
    JsonLogic_ParserState_True,
    JsonLogic_ParserState_False,
    JsonLogic_ParserState_ArrayStart,
    JsonLogic_ParserState_ArrayAfterStart,
    JsonLogic_ParserState_ArrayNext,
    JsonLogic_ParserState_ArrayEnd,
    JsonLogic_ParserState_ObjectStart,
    JsonLogic_ParserState_ObjectAfterStart,
    JsonLogic_ParserState_ObjectKey,
    JsonLogic_ParserState_ObjectValue,
    JsonLogic_ParserState_ObjectNext,
    JsonLogic_ParserState_ObjectEnd,
    JsonLogic_ParserState_Max,
} JsonLogic_RootParser;

#define JSONLOGIC_PARSE_EOF 256

static const JsonLogic_RootParser JsonLogic_Parser_Root[JsonLogic_ParserState_Max][JSONLOGIC_PARSE_EOF + 1] = {
    [JsonLogic_ParserState_Start] = {
        [' ']  = JsonLogic_ParserState_Start,
        ['\n'] = JsonLogic_ParserState_Start,
        ['\r'] = JsonLogic_ParserState_Start,
        ['\t'] = JsonLogic_ParserState_Start,
        ['\v'] = JsonLogic_ParserState_Start,
        ['"']  = JsonLogic_ParserState_String,
        ['n']  = JsonLogic_ParserState_Null,
        ['t']  = JsonLogic_ParserState_True,
        ['f']  = JsonLogic_ParserState_False,
        ['[']  = JsonLogic_ParserState_ArrayStart,
        ['{']  = JsonLogic_ParserState_ObjectStart,
        ['-']  = JsonLogic_ParserState_Number,
        ['0']  = JsonLogic_ParserState_Number,
        ['1']  = JsonLogic_ParserState_Number,
        ['2']  = JsonLogic_ParserState_Number,
        ['3']  = JsonLogic_ParserState_Number,
        ['4']  = JsonLogic_ParserState_Number,
        ['5']  = JsonLogic_ParserState_Number,
        ['6']  = JsonLogic_ParserState_Number,
        ['7']  = JsonLogic_ParserState_Number,
        ['8']  = JsonLogic_ParserState_Number,
        ['9']  = JsonLogic_ParserState_Number,
    },
    [JsonLogic_ParserState_End] = {
        [' ']  = JsonLogic_ParserState_End,
        ['\n'] = JsonLogic_ParserState_End,
        ['\r'] = JsonLogic_ParserState_End,
        ['\t'] = JsonLogic_ParserState_End,
        ['\v'] = JsonLogic_ParserState_End,
        [JSONLOGIC_PARSE_EOF] = JsonLogic_ParserState_End,
    },
    [JsonLogic_ParserState_ArrayStart] = {
        [' ']  = JsonLogic_ParserState_ArrayAfterStart,
        ['\n'] = JsonLogic_ParserState_ArrayAfterStart,
        ['\r'] = JsonLogic_ParserState_ArrayAfterStart,
        ['\t'] = JsonLogic_ParserState_ArrayAfterStart,
        ['\v'] = JsonLogic_ParserState_ArrayAfterStart,
        ['"']  = JsonLogic_ParserState_String,
        ['n']  = JsonLogic_ParserState_Null,
        ['t']  = JsonLogic_ParserState_True,
        ['f']  = JsonLogic_ParserState_False,
        ['[']  = JsonLogic_ParserState_ArrayStart,
        [']']  = JsonLogic_ParserState_ArrayEnd,
        ['{']  = JsonLogic_ParserState_ObjectStart,
        ['-']  = JsonLogic_ParserState_Number,
        ['0']  = JsonLogic_ParserState_Number,
        ['1']  = JsonLogic_ParserState_Number,
        ['2']  = JsonLogic_ParserState_Number,
        ['3']  = JsonLogic_ParserState_Number,
        ['4']  = JsonLogic_ParserState_Number,
        ['5']  = JsonLogic_ParserState_Number,
        ['6']  = JsonLogic_ParserState_Number,
        ['7']  = JsonLogic_ParserState_Number,
        ['8']  = JsonLogic_ParserState_Number,
        ['9']  = JsonLogic_ParserState_Number,
    },
    [JsonLogic_ParserState_ArrayAfterStart] = {
        [' ']  = JsonLogic_ParserState_ArrayAfterStart,
        ['\n'] = JsonLogic_ParserState_ArrayAfterStart,
        ['\r'] = JsonLogic_ParserState_ArrayAfterStart,
        ['\t'] = JsonLogic_ParserState_ArrayAfterStart,
        ['\v'] = JsonLogic_ParserState_ArrayAfterStart,
        ['"']  = JsonLogic_ParserState_String,
        ['n']  = JsonLogic_ParserState_Null,
        ['t']  = JsonLogic_ParserState_True,
        ['f']  = JsonLogic_ParserState_False,
        ['[']  = JsonLogic_ParserState_ArrayStart,
        [']']  = JsonLogic_ParserState_ArrayEnd,
        ['{']  = JsonLogic_ParserState_ObjectStart,
        ['-']  = JsonLogic_ParserState_Number,
        ['0']  = JsonLogic_ParserState_Number,
        ['1']  = JsonLogic_ParserState_Number,
        ['2']  = JsonLogic_ParserState_Number,
        ['3']  = JsonLogic_ParserState_Number,
        ['4']  = JsonLogic_ParserState_Number,
        ['5']  = JsonLogic_ParserState_Number,
        ['6']  = JsonLogic_ParserState_Number,
        ['7']  = JsonLogic_ParserState_Number,
        ['8']  = JsonLogic_ParserState_Number,
        ['9']  = JsonLogic_ParserState_Number,
    },
    [JsonLogic_ParserState_ArrayNext] = {
        [' ']  = JsonLogic_ParserState_ArrayNext,
        ['\n'] = JsonLogic_ParserState_ArrayNext,
        ['\r'] = JsonLogic_ParserState_ArrayNext,
        ['\t'] = JsonLogic_ParserState_ArrayNext,
        ['\v'] = JsonLogic_ParserState_ArrayNext,
        [',']  = JsonLogic_ParserState_ArrayAfterStart,
        [']']  = JsonLogic_ParserState_ArrayEnd,
    },
    [JsonLogic_ParserState_ObjectStart] = {
        [' ']  = JsonLogic_ParserState_ObjectAfterStart,
        ['\n'] = JsonLogic_ParserState_ObjectAfterStart,
        ['\r'] = JsonLogic_ParserState_ObjectAfterStart,
        ['\t'] = JsonLogic_ParserState_ObjectAfterStart,
        ['\v'] = JsonLogic_ParserState_ObjectAfterStart,
        ['"']  = JsonLogic_ParserState_String,
        ['}']  = JsonLogic_ParserState_ObjectEnd,
    },
    [JsonLogic_ParserState_ObjectAfterStart] = {
        [' ']  = JsonLogic_ParserState_ObjectAfterStart,
        ['\n'] = JsonLogic_ParserState_ObjectAfterStart,
        ['\r'] = JsonLogic_ParserState_ObjectAfterStart,
        ['\t'] = JsonLogic_ParserState_ObjectAfterStart,
        ['\v'] = JsonLogic_ParserState_ObjectAfterStart,
        ['"']  = JsonLogic_ParserState_String,
        ['}']  = JsonLogic_ParserState_ObjectEnd,
    },
    [JsonLogic_ParserState_ObjectKey] = {
        [' ']  = JsonLogic_ParserState_ObjectKey,
        ['\n'] = JsonLogic_ParserState_ObjectKey,
        ['\r'] = JsonLogic_ParserState_ObjectKey,
        ['\t'] = JsonLogic_ParserState_ObjectKey,
        ['\v'] = JsonLogic_ParserState_ObjectKey,
        [':']  = JsonLogic_ParserState_ObjectValue,
    },
    [JsonLogic_ParserState_ObjectValue] = {
        [' ']  = JsonLogic_ParserState_ObjectValue,
        ['\n'] = JsonLogic_ParserState_ObjectValue,
        ['\r'] = JsonLogic_ParserState_ObjectValue,
        ['\t'] = JsonLogic_ParserState_ObjectValue,
        ['\v'] = JsonLogic_ParserState_ObjectValue,
        ['"']  = JsonLogic_ParserState_String,
        ['n']  = JsonLogic_ParserState_Null,
        ['t']  = JsonLogic_ParserState_True,
        ['f']  = JsonLogic_ParserState_False,
        ['[']  = JsonLogic_ParserState_ArrayStart,
        ['{']  = JsonLogic_ParserState_ObjectStart,
        ['-']  = JsonLogic_ParserState_Number,
        ['0']  = JsonLogic_ParserState_Number,
        ['1']  = JsonLogic_ParserState_Number,
        ['2']  = JsonLogic_ParserState_Number,
        ['3']  = JsonLogic_ParserState_Number,
        ['4']  = JsonLogic_ParserState_Number,
        ['5']  = JsonLogic_ParserState_Number,
        ['6']  = JsonLogic_ParserState_Number,
        ['7']  = JsonLogic_ParserState_Number,
        ['8']  = JsonLogic_ParserState_Number,
        ['9']  = JsonLogic_ParserState_Number,
    },
    [JsonLogic_ParserState_ObjectNext] = {
        [' ']  = JsonLogic_ParserState_ObjectNext,
        ['\n'] = JsonLogic_ParserState_ObjectNext,
        ['\r'] = JsonLogic_ParserState_ObjectNext,
        ['\t'] = JsonLogic_ParserState_ObjectNext,
        ['\v'] = JsonLogic_ParserState_ObjectNext,
        [',']  = JsonLogic_ParserState_ObjectAfterStart,
        ['}']  = JsonLogic_ParserState_ObjectEnd,
    },
};

typedef enum JsonLogic_NumberParser {
    JsonLogic_NumberParser_Error = 0,
    JsonLogic_NumberParser_Start,
    JsonLogic_NumberParser_Negative,
    JsonLogic_NumberParser_LeadingZero,
    JsonLogic_NumberParser_Digit,
    JsonLogic_NumberParser_Decimal,
    JsonLogic_NumberParser_DecimalDigit,
    JsonLogic_NumberParser_Exp,
    JsonLogic_NumberParser_ExpSign,
    JsonLogic_NumberParser_ExpDigit,
    JsonLogic_NumberParser_End,
    JsonLogic_NumberParser_Max,
} JsonLogic_NumberParser;

static const JsonLogic_NumberParser JsonLogic_Parser_Number[JsonLogic_NumberParser_Max][JSONLOGIC_PARSE_EOF + 1] = {
    [JsonLogic_NumberParser_Start] = {
        ['-'] = JsonLogic_NumberParser_Negative,
        ['0'] = JsonLogic_NumberParser_LeadingZero,
        ['1'] = JsonLogic_NumberParser_Digit,
        ['2'] = JsonLogic_NumberParser_Digit,
        ['3'] = JsonLogic_NumberParser_Digit,
        ['4'] = JsonLogic_NumberParser_Digit,
        ['5'] = JsonLogic_NumberParser_Digit,
        ['6'] = JsonLogic_NumberParser_Digit,
        ['7'] = JsonLogic_NumberParser_Digit,
        ['8'] = JsonLogic_NumberParser_Digit,
        ['9'] = JsonLogic_NumberParser_Digit,
    },
    [JsonLogic_NumberParser_Negative] = {
        ['0'] = JsonLogic_NumberParser_LeadingZero,
        ['1'] = JsonLogic_NumberParser_Digit,
        ['2'] = JsonLogic_NumberParser_Digit,
        ['3'] = JsonLogic_NumberParser_Digit,
        ['4'] = JsonLogic_NumberParser_Digit,
        ['5'] = JsonLogic_NumberParser_Digit,
        ['6'] = JsonLogic_NumberParser_Digit,
        ['7'] = JsonLogic_NumberParser_Digit,
        ['8'] = JsonLogic_NumberParser_Digit,
        ['9'] = JsonLogic_NumberParser_Digit,
    },
    [JsonLogic_NumberParser_LeadingZero] = {
        ['.'] = JsonLogic_NumberParser_Decimal,
        ['e'] = JsonLogic_NumberParser_Exp,
        ['E'] = JsonLogic_NumberParser_Exp,
        [JSONLOGIC_PARSE_EOF] = JsonLogic_NumberParser_End,
    },
    [JsonLogic_NumberParser_Digit] = {
        ['0'] = JsonLogic_NumberParser_Digit,
        ['1'] = JsonLogic_NumberParser_Digit,
        ['2'] = JsonLogic_NumberParser_Digit,
        ['3'] = JsonLogic_NumberParser_Digit,
        ['4'] = JsonLogic_NumberParser_Digit,
        ['5'] = JsonLogic_NumberParser_Digit,
        ['6'] = JsonLogic_NumberParser_Digit,
        ['7'] = JsonLogic_NumberParser_Digit,
        ['8'] = JsonLogic_NumberParser_Digit,
        ['9'] = JsonLogic_NumberParser_Digit,
        ['.'] = JsonLogic_NumberParser_Decimal,
        ['e'] = JsonLogic_NumberParser_Exp,
        ['E'] = JsonLogic_NumberParser_Exp,
        [JSONLOGIC_PARSE_EOF] = JsonLogic_NumberParser_End,
    },
    [JsonLogic_NumberParser_Decimal] = {
        ['0'] = JsonLogic_NumberParser_DecimalDigit,
        ['1'] = JsonLogic_NumberParser_DecimalDigit,
        ['2'] = JsonLogic_NumberParser_DecimalDigit,
        ['3'] = JsonLogic_NumberParser_DecimalDigit,
        ['4'] = JsonLogic_NumberParser_DecimalDigit,
        ['5'] = JsonLogic_NumberParser_DecimalDigit,
        ['6'] = JsonLogic_NumberParser_DecimalDigit,
        ['7'] = JsonLogic_NumberParser_DecimalDigit,
        ['8'] = JsonLogic_NumberParser_DecimalDigit,
        ['9'] = JsonLogic_NumberParser_DecimalDigit,
    },
    [JsonLogic_NumberParser_DecimalDigit] = {
        ['0'] = JsonLogic_NumberParser_DecimalDigit,
        ['1'] = JsonLogic_NumberParser_DecimalDigit,
        ['2'] = JsonLogic_NumberParser_DecimalDigit,
        ['3'] = JsonLogic_NumberParser_DecimalDigit,
        ['4'] = JsonLogic_NumberParser_DecimalDigit,
        ['5'] = JsonLogic_NumberParser_DecimalDigit,
        ['6'] = JsonLogic_NumberParser_DecimalDigit,
        ['7'] = JsonLogic_NumberParser_DecimalDigit,
        ['8'] = JsonLogic_NumberParser_DecimalDigit,
        ['9'] = JsonLogic_NumberParser_DecimalDigit,
        ['e'] = JsonLogic_NumberParser_Exp,
        ['E'] = JsonLogic_NumberParser_Exp,
        [JSONLOGIC_PARSE_EOF] = JsonLogic_NumberParser_End,
    },
    [JsonLogic_NumberParser_Exp] = {
        ['-'] = JsonLogic_NumberParser_ExpSign,
        ['+'] = JsonLogic_NumberParser_ExpSign,
        ['0'] = JsonLogic_NumberParser_ExpDigit,
        ['1'] = JsonLogic_NumberParser_ExpDigit,
        ['2'] = JsonLogic_NumberParser_ExpDigit,
        ['3'] = JsonLogic_NumberParser_ExpDigit,
        ['4'] = JsonLogic_NumberParser_ExpDigit,
        ['5'] = JsonLogic_NumberParser_ExpDigit,
        ['6'] = JsonLogic_NumberParser_ExpDigit,
        ['7'] = JsonLogic_NumberParser_ExpDigit,
        ['8'] = JsonLogic_NumberParser_ExpDigit,
        ['9'] = JsonLogic_NumberParser_ExpDigit,
    },
    [JsonLogic_NumberParser_ExpSign] = {
        ['0'] = JsonLogic_NumberParser_ExpDigit,
        ['1'] = JsonLogic_NumberParser_ExpDigit,
        ['2'] = JsonLogic_NumberParser_ExpDigit,
        ['3'] = JsonLogic_NumberParser_ExpDigit,
        ['4'] = JsonLogic_NumberParser_ExpDigit,
        ['5'] = JsonLogic_NumberParser_ExpDigit,
        ['6'] = JsonLogic_NumberParser_ExpDigit,
        ['7'] = JsonLogic_NumberParser_ExpDigit,
        ['8'] = JsonLogic_NumberParser_ExpDigit,
        ['9'] = JsonLogic_NumberParser_ExpDigit,
    },
    [JsonLogic_NumberParser_ExpDigit] = {
        ['0'] = JsonLogic_NumberParser_ExpDigit,
        ['1'] = JsonLogic_NumberParser_ExpDigit,
        ['2'] = JsonLogic_NumberParser_ExpDigit,
        ['3'] = JsonLogic_NumberParser_ExpDigit,
        ['4'] = JsonLogic_NumberParser_ExpDigit,
        ['5'] = JsonLogic_NumberParser_ExpDigit,
        ['6'] = JsonLogic_NumberParser_ExpDigit,
        ['7'] = JsonLogic_NumberParser_ExpDigit,
        ['8'] = JsonLogic_NumberParser_ExpDigit,
        ['9'] = JsonLogic_NumberParser_ExpDigit,
        [JSONLOGIC_PARSE_EOF] = JsonLogic_NumberParser_End,
    },
    [JsonLogic_NumberParser_End] = {
        [JSONLOGIC_PARSE_EOF] = JsonLogic_NumberParser_End,
    },
};

static const int JsonLogic_EscMap[256] = {
    ['"']  = '"',
    ['\\'] = '\\',
    ['/']  = '/',
    ['b']  = '\b',
    ['f']  = '\f',
    ['n']  = '\n',
    ['r']  = '\r',
    ['t']  = '\t',
};

static const int JsonLogic_HexMap[256] = {
    ['0'] =  0 | 0xF00,
    ['1'] =  1 | 0xF00,
    ['2'] =  2 | 0xF00,
    ['3'] =  3 | 0xF00,
    ['4'] =  4 | 0xF00,
    ['5'] =  5 | 0xF00,
    ['6'] =  6 | 0xF00,
    ['7'] =  7 | 0xF00,
    ['8'] =  8 | 0xF00,
    ['9'] =  9 | 0xF00,
    ['a'] = 10 | 0xF00,
    ['b'] = 11 | 0xF00,
    ['c'] = 12 | 0xF00,
    ['d'] = 13 | 0xF00,
    ['e'] = 14 | 0xF00,
    ['f'] = 15 | 0xF00,
    ['A'] = 10 | 0xF00,
    ['B'] = 11 | 0xF00,
    ['C'] = 12 | 0xF00,
    ['D'] = 13 | 0xF00,
    ['E'] = 14 | 0xF00,
    ['F'] = 15 | 0xF00,
};

#define JSONLOGIC_PARSE_STRING(STR, SIZE, INDEX, ERROR, CODE) \
    while ((INDEX) < (SIZE)) { \
        uint8_t byte1 = (STR)[(INDEX) ++]; \
        if (byte1 < 0x80) { \
            if (byte1 == '"') \
                break; \
            if (byte1 < ' ') { \
                (ERROR) = JSONLOGIC_ERROR_SYNTAX_ERROR; \
                break; \
            } \
            uint32_t codepoint; \
            if (byte1 == '\\') { \
                if ((INDEX) >= (SIZE)) { \
                    (ERROR) = JSONLOGIC_ERROR_SYNTAX_ERROR; \
                    break; \
                } \
                byte1 = (STR)[(INDEX) ++]; \
                if (byte1 == 'u') { \
                    if ((INDEX) + 4 >= (SIZE)) { \
                        (ERROR) = JSONLOGIC_ERROR_SYNTAX_ERROR; \
                        break; \
                    } \
                    uint32_t x1 = JsonLogic_HexMap[(unsigned char)(STR)[(INDEX) ++]]; \
                    uint32_t x2 = JsonLogic_HexMap[(unsigned char)(STR)[(INDEX) ++]]; \
                    uint32_t x3 = JsonLogic_HexMap[(unsigned char)(STR)[(INDEX) ++]]; \
                    uint32_t x4 = JsonLogic_HexMap[(unsigned char)(STR)[(INDEX) ++]]; \
                    if ((~x1 | ~x2 | ~x3 | ~x4) & 0xF00) { \
                        (ERROR) = JSONLOGIC_ERROR_SYNTAX_ERROR; \
                        break; \
                    } \
                    codepoint = (x1 & 0xFF) << 12 | (x2 & 0xFF) << 8 | (x3 & 0xFF) << 4 | (x4 & 0xFF); \
                } else { \
                    codepoint = JsonLogic_EscMap[byte1]; \
                    if (codepoint == 0) { \
                        (ERROR) = JSONLOGIC_ERROR_SYNTAX_ERROR; \
                        break; \
                    } \
                } \
                \
            } else { \
                codepoint = byte1; \
            } \
            CODE; \
        } else if (byte1 < 0xC2) { \
            /* unexpected continuation or overlong 2-byte sequence */ \
            (ERROR) = JSONLOGIC_ERROR_SYNTAX_ERROR; \
            break; \
        } else if (byte1 < 0xE0) { \
            if ((INDEX) < (SIZE)) { \
                uint8_t byte2 = (STR)[(INDEX) ++]; \
                if ((byte2 & 0xC0) == 0x80) { \
                    uint32_t codepoint = (uint32_t)(byte1 & 0xF1) << 6 | \
                                         (uint32_t)(byte2 & 0x3F); \
                    CODE; \
                } else { \
                    /* else illegal byte sequence */ \
                    (ERROR) = JSONLOGIC_ERROR_SYNTAX_ERROR; \
                    break; \
                } \
            } else { \
                /* else unexpected end of multibyte sequence */ \
                (ERROR) = JSONLOGIC_ERROR_SYNTAX_ERROR; \
                break; \
            } \
        } else if (byte1 < 0xF0) { \
            if ((INDEX) + 1 < (SIZE)) { \
                uint8_t byte2 = (STR)[(INDEX) ++]; \
                uint8_t byte3 = (STR)[(INDEX) ++]; \
                \
                if ((byte1 != 0xE0 || byte2 >= 0xA0) && \
                    (byte2 & 0xC0) == 0x80 && \
                    (byte3 & 0xC0) == 0x80) { \
                    uint32_t codepoint = (uint32_t)(byte1 & 0x0F) << 12 | \
                                         (uint32_t)(byte2 & 0x3F) <<  6 | \
                                         (uint32_t)(byte3 & 0x3F); \
                    CODE; \
                } else { \
                    /* else illegal byte sequence */ \
                    (ERROR) = JSONLOGIC_ERROR_SYNTAX_ERROR; \
                    break; \
                } \
            } else { \
                /* else unexpected end of multibyte sequence */ \
                (ERROR) = JSONLOGIC_ERROR_SYNTAX_ERROR; \
                break; \
            } \
        } else if (byte1 < 0xF8) { \
            if ((INDEX) + 2 < (SIZE)) { \
                uint8_t byte2 = (STR)[(INDEX) ++]; \
                uint8_t byte3 = (STR)[(INDEX) ++]; \
                uint8_t byte4 = (STR)[(INDEX) ++]; \
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
            } else { \
                /* else unexpected end of multibyte sequence */ \
                (ERROR) = JSONLOGIC_ERROR_SYNTAX_ERROR; \
                break; \
             } \
        } else { \
            /* else illegal byte sequence */ \
            (ERROR) = JSONLOGIC_ERROR_SYNTAX_ERROR; \
            break; \
        } \
    }

typedef enum JsonLogic_ParseType {
    JsonLogic_ParseType_Value,
    JsonLogic_ParseType_Array,
    JsonLogic_ParseType_Object,
} JsonLogic_ParseType;

typedef struct JsonLogic_ParseItem_Object {
    JsonLogic_Handle key;
    JsonLogic_ObjBuf buf;
} JsonLogic_ParseItem_Object;

typedef struct JsonLogic_ParseItem {
    JsonLogic_ParseType type;
    union {
        JsonLogic_Handle value;
        JsonLogic_ArrayBuf arraybuf;
        JsonLogic_ParseItem_Object object;
    } data;
} JsonLogic_ParseItem;

typedef struct JsonLogic_ParseStack {
    size_t capacity;
    size_t used;
    JsonLogic_ParseItem *items;
} JsonLogic_ParseStack;

#define JSONLOGIC_PARSESTACK_CHUNK_SIZE 64
#define JSONLOGIC_PARSESTACK_INIT (JsonLogic_ParseStack){ .capacity = 0, .used = 0, .items = NULL }

static JsonLogic_ParseItem *jsonlogic_parsestack_push(JsonLogic_ParseStack *stack, JsonLogic_ParseType type) {
    if (stack->used == stack->capacity) {
        if (stack->capacity > SIZE_MAX - JSONLOGIC_PARSESTACK_CHUNK_SIZE) {
            JSONLOGIC_ERROR_MEMORY();
            errno = ENOMEM;
            return NULL;
        }
        size_t new_capacity = stack->capacity + JSONLOGIC_PARSESTACK_CHUNK_SIZE;
        JsonLogic_ParseItem *new_items = realloc(stack->items, sizeof(JsonLogic_ParseItem) * new_capacity);
        if (new_items == NULL) {
            JSONLOGIC_ERROR_MEMORY();
            return NULL;
        }
        stack->items    = new_items;
        stack->capacity = new_capacity;
    }
    JsonLogic_ParseItem *item = &stack->items[stack->used ++];
    item->type = type;
    switch (type) {
        case JsonLogic_ParseType_Value:
            item->data.value = JsonLogic_Null;
            break;

        case JsonLogic_ParseType_Array:
            item->data.arraybuf = JSONLOGIC_ARRAYBUF_INIT;
            break;

        case JsonLogic_ParseType_Object:
            item->data.object.key = JsonLogic_Null;
            item->data.object.buf = JSONLOGIC_OBJBUF_INIT;
            break;

        default:
            assert(false);
            return NULL;
    }
    return item;
}

static inline JsonLogic_Error jsonlogic_parsestack_push_value(JsonLogic_ParseStack *stack, JsonLogic_Handle value) {
    JsonLogic_ParseItem *item = jsonlogic_parsestack_push(stack, JsonLogic_ParseType_Value);
    if (item == NULL) {
        return JSONLOGIC_ERROR_OUT_OF_MEMORY;
    }
    item->data.value = jsonlogic_incref(value);
    return JSONLOGIC_ERROR_SUCCESS;
}

static inline JsonLogic_Error jsonlogic_parsestack_push_array(JsonLogic_ParseStack *stack) {
    JsonLogic_ParseItem *item = jsonlogic_parsestack_push(stack, JsonLogic_ParseType_Array);
    if (item == NULL) {
        return JSONLOGIC_ERROR_OUT_OF_MEMORY;
    }
    return JSONLOGIC_ERROR_SUCCESS;
}

static inline JsonLogic_Error jsonlogic_parsestack_push_object(JsonLogic_ParseStack *stack) {
    JsonLogic_ParseItem *item = jsonlogic_parsestack_push(stack, JsonLogic_ParseType_Object);
    if (item == NULL) {
        return JSONLOGIC_ERROR_OUT_OF_MEMORY;
    }
    return JSONLOGIC_ERROR_SUCCESS;
}

static inline JsonLogic_Error jsonlogic_parsestack_handle_value(JsonLogic_ParseStack *stack, JsonLogic_Handle value, JsonLogic_RootParser *stateptr) {
    if (stateptr == NULL) {
        return JSONLOGIC_ERROR_ILLEGAL_ARGUMENT;
    }

    if (stack->used == 0) {
        *stateptr = JsonLogic_ParserState_End;
        return jsonlogic_parsestack_push_value(stack, value);
    }

    JsonLogic_ParseItem *item = &stack->items[stack->used - 1];
    switch (item->type) {
        case JsonLogic_ParseType_Value:
            return JSONLOGIC_ERROR_INTERNAL_ERROR;

        case JsonLogic_ParseType_Array:
            *stateptr = JsonLogic_ParserState_ArrayNext;
            return jsonlogic_arraybuf_append(&item->data.arraybuf, value);

        case JsonLogic_ParseType_Object:
            if (JSONLOGIC_IS_NULL(item->data.object.key)) {
                *stateptr = JsonLogic_ParserState_ObjectKey;
                if (!JSONLOGIC_IS_STRING(value)) {
                    if (JSONLOGIC_IS_ERROR(value)) {
                        return value.intptr;
                    }
                    return JSONLOGIC_ERROR_ILLEGAL_ARGUMENT;
                }
                item->data.object.key = jsonlogic_incref(value);
            } else {
                *stateptr = JsonLogic_ParserState_ObjectNext;
                JsonLogic_Error error = jsonlogic_objbuf_set(&item->data.object.buf, item->data.object.key, value);
                if (error != JSONLOGIC_ERROR_SUCCESS) {
                    return error;
                }
                jsonlogic_decref(item->data.object.key);
                item->data.object.key = JsonLogic_Null;
            }
            return JSONLOGIC_ERROR_SUCCESS;

        default:
            assert(false);
            return JSONLOGIC_ERROR_INTERNAL_ERROR;
    }
}

static JsonLogic_Handle jsonlogic_parsestack_pop(JsonLogic_ParseStack *stack) {
    if (stack->used == 0) {
        return JsonLogic_Error_SyntaxError;
    }
    JsonLogic_ParseItem *item = &stack->items[-- stack->used];
    switch (item->type) {
        case JsonLogic_ParseType_Value:
            return item->data.value;

        case JsonLogic_ParseType_Array:
            return jsonlogic_array_into_handle(jsonlogic_arraybuf_take(&item->data.arraybuf));

        case JsonLogic_ParseType_Object:
            if (!JSONLOGIC_IS_NULL(item->data.object.key)) {
                jsonlogic_objbuf_free(&item->data.object.buf);
                jsonlogic_decref(item->data.object.key);
                return JsonLogic_Error_IllegalArgument;
            }
            return jsonlogic_object_into_handle(jsonlogic_objbuf_take(&item->data.object.buf));

        default:
            assert(false);
            return JsonLogic_Error_InternalError;
    }
}

static void jsonlogic_parsestack_free(JsonLogic_ParseStack *stack) {
    for (size_t index = 0; index < stack->used; ++ index) {
        JsonLogic_ParseItem *item = &stack->items[index];
        switch (item->type) {
            case JsonLogic_ParseType_Value:
                jsonlogic_decref(item->data.value);
                break;

            case JsonLogic_ParseType_Array:
                jsonlogic_arraybuf_free(&item->data.arraybuf);
                break;

            case JsonLogic_ParseType_Object:
                jsonlogic_objbuf_free(&item->data.object.buf);
                jsonlogic_decref(item->data.object.key);
                break;

            default:
                assert(false);
        }
    }
    free(stack->items);
    stack->capacity = 0;
    stack->used     = 0;
    stack->items    = NULL;
}

JsonLogic_Handle jsonlogic_parse_sized(const char *str, size_t size, JsonLogic_LineInfo *infoptr) {
    JsonLogic_ParseStack stack = JSONLOGIC_PARSESTACK_INIT;
    JsonLogic_RootParser state = JsonLogic_ParserState_Start;
    JsonLogic_Error error = JSONLOGIC_ERROR_SUCCESS;

    size_t index = 0;
    while (index < size) {
        const size_t ch = (unsigned char)str[index];
        state = JsonLogic_Parser_Root[state][ch];
        switch (state) {
            case JsonLogic_ParserState_Start:
                index ++;
                break;

            case JsonLogic_ParserState_End:
                index ++;
                if (index == size) {
                    goto loop_end;
                }
                break;

            case JsonLogic_ParserState_String:
            {
                size_t start_index = ++ index;
                size_t utf16_size = 0;
                JSONLOGIC_PARSE_STRING(str, size, index, error, {
                    if (codepoint < 0x10000) {
                        utf16_size += 1;
                    } else {
                        utf16_size += 2;
                    }
                });

                if (error != JSONLOGIC_ERROR_SUCCESS) {
                    state = JsonLogic_ParserState_Error;
                    goto loop_end;
                }

                JsonLogic_String *string = malloc(sizeof(JsonLogic_String) - sizeof(char16_t) + sizeof(char16_t) * utf16_size);
                if (string == NULL) {
                    JSONLOGIC_ERROR_MEMORY();
                    state = JsonLogic_ParserState_Error;
                    error = JSONLOGIC_ERROR_OUT_OF_MEMORY;
                    goto loop_end;
                }
                string->refcount = 1;
                string->size     = utf16_size;

                index = start_index;
                size_t utf16_index = 0;
                JSONLOGIC_PARSE_STRING(str, size, index, error, {
                    if (codepoint < 0x10000) {
                        string->str[utf16_index ++] = codepoint;
                    } else {
                        string->str[utf16_index ++] = 0xD800 | (codepoint >> 10);
                        string->str[utf16_index ++] = 0xDC00 | (codepoint & 0x3FF);
                    }
                });

                assert(utf16_index == utf16_size);
                assert(error == JSONLOGIC_ERROR_SUCCESS);

                JsonLogic_Handle handle = jsonlogic_string_into_handle(string);
                error = jsonlogic_parsestack_handle_value(&stack, handle, &state);
                jsonlogic_decref(handle);
                if (error != JSONLOGIC_ERROR_SUCCESS) {
                    state = JsonLogic_ParserState_Error;
                    goto loop_end;
                }
                break;
            }
            case JsonLogic_ParserState_Number:
            {
                size_t start_index = index;
                JsonLogic_NumberParser num_state = JsonLogic_NumberParser_Start;
                while (index < size) {
                    const size_t num_ch = (unsigned char)str[index ++];
                    JsonLogic_NumberParser prev_num_state = num_state;
                    num_state = JsonLogic_Parser_Number[num_state][num_ch];
                    switch (num_state) {
                        case JsonLogic_NumberParser_Start:
                            assert(false);
                            break;

                        case JsonLogic_NumberParser_Negative:
                        case JsonLogic_NumberParser_LeadingZero:
                        case JsonLogic_NumberParser_Digit:
                        case JsonLogic_NumberParser_Decimal:
                        case JsonLogic_NumberParser_DecimalDigit:
                        case JsonLogic_NumberParser_Exp:
                        case JsonLogic_NumberParser_ExpSign:
                        case JsonLogic_NumberParser_ExpDigit:
                            break;

                        case JsonLogic_NumberParser_End:
                            goto num_loop_end;
                            break;

                        case JsonLogic_NumberParser_Error:
                            -- index;
                            num_state = JsonLogic_Parser_Number[prev_num_state][JSONLOGIC_PARSE_EOF];
                            goto num_loop_end;
                            break;

                        case JsonLogic_NumberParser_Max:
                            assert(false);
                            goto num_loop_end;
                            break;
                    }
                }
            num_loop_end:

                num_state = JsonLogic_Parser_Number[num_state][JSONLOGIC_PARSE_EOF];

                if (num_state != JsonLogic_NumberParser_End) {
                    state = JsonLogic_ParserState_Error;
                    error = JSONLOGIC_ERROR_SYNTAX_ERROR;
                    goto loop_end;
                }

                char *endptr = NULL;
                char buf[128];
                size_t num_size = index - start_index;
                size_t clib_index = start_index;
                double number;
                if (num_size + 1 > sizeof(buf)) {
                    char *buf = malloc(num_size + 1);
                    if (buf == NULL) {
                        JSONLOGIC_ERROR_MEMORY();
                        state = JsonLogic_ParserState_Error;
                        error = JSONLOGIC_ERROR_OUT_OF_MEMORY;
                        goto loop_end;
                    }
                    memcpy(buf, str + start_index, num_size);
                    buf[num_size] = 0;
                    number = strtod(buf, &endptr);
                    clib_index += endptr - buf;
                    free(buf);
                } else {
                    memcpy(buf, str + start_index, num_size);
                    buf[num_size] = 0;
                    number = strtod(buf, &endptr);
                    clib_index += endptr - buf;
                }
                if (clib_index != index) {
                    JSONLOGIC_DEBUG(
                        "Clib disagrees about where number \"%s\" ends. Clib index: %" PRIuPTR ", JsonLogic index: %" PRIuPTR,
                        buf, clib_index, index);
                    state = JsonLogic_ParserState_Error;
                    error = JSONLOGIC_ERROR_INTERNAL_ERROR;
                    goto loop_end;
                }
                error = jsonlogic_parsestack_handle_value(&stack, jsonlogic_number_from(number), &state);
                if (error != JSONLOGIC_ERROR_SUCCESS) {
                    state = JsonLogic_ParserState_Error;
                    goto loop_end;
                }
                break;
            }
            case JsonLogic_ParserState_Null:
                if (size < index + 4) {
                    error = JSONLOGIC_ERROR_SYNTAX_ERROR;
                    state = JsonLogic_ParserState_Error;
                    goto loop_end;
                }
                if (memcmp(str + index, "null", 4) != 0) {
                    error = JSONLOGIC_ERROR_SYNTAX_ERROR;
                    state = JsonLogic_ParserState_Error;
                    goto loop_end;
                }
                index += 4;
                error = jsonlogic_parsestack_handle_value(&stack, JsonLogic_Null, &state);
                if (error != JSONLOGIC_ERROR_SUCCESS) {
                    state = JsonLogic_ParserState_Error;
                    goto loop_end;
                }
                break;

            case JsonLogic_ParserState_True:
                if (size < index + 4) {
                    error = JSONLOGIC_ERROR_SYNTAX_ERROR;
                    state = JsonLogic_ParserState_Error;
                    goto loop_end;
                }
                if (memcmp(str + index, "true", 4) != 0) {
                    error = JSONLOGIC_ERROR_SYNTAX_ERROR;
                    state = JsonLogic_ParserState_Error;
                    goto loop_end;
                }
                index += 4;
                error = jsonlogic_parsestack_handle_value(&stack, JsonLogic_True, &state);
                if (error != JSONLOGIC_ERROR_SUCCESS) {
                    state = JsonLogic_ParserState_Error;
                    goto loop_end;
                }
                break;

            case JsonLogic_ParserState_False:
                if (size < index + 5) {
                    error = JSONLOGIC_ERROR_SYNTAX_ERROR;
                    state = JsonLogic_ParserState_Error;
                    goto loop_end;
                }
                if (memcmp(str + index, "false", 5) != 0) {
                    error = JSONLOGIC_ERROR_SYNTAX_ERROR;
                    state = JsonLogic_ParserState_Error;
                    goto loop_end;
                }
                index += 5;
                error = jsonlogic_parsestack_handle_value(&stack, JsonLogic_False, &state);
                if (error != JSONLOGIC_ERROR_SUCCESS) {
                    state = JsonLogic_ParserState_Error;
                    goto loop_end;
                }
                break;

            case JsonLogic_ParserState_ArrayStart:
                error = jsonlogic_parsestack_push_array(&stack);
                if (error != JSONLOGIC_ERROR_SUCCESS) {
                    state = JsonLogic_ParserState_Error;
                    goto loop_end;
                }
                index ++;
                break;

            case JsonLogic_ParserState_ArrayAfterStart:
                index ++;
                break;

            case JsonLogic_ParserState_ArrayNext:
                index ++;
                break;

            case JsonLogic_ParserState_ArrayEnd:
            {
                JsonLogic_Handle handle = jsonlogic_parsestack_pop(&stack);
                if (!JSONLOGIC_IS_ARRAY(handle)) {
                    jsonlogic_decref(handle);
                    error = JSONLOGIC_ERROR_SYNTAX_ERROR;
                    state = JsonLogic_ParserState_Error;
                    goto loop_end;
                }
                error = jsonlogic_parsestack_handle_value(&stack, handle, &state);
                jsonlogic_decref(handle);
                if (error != JSONLOGIC_ERROR_SUCCESS) {
                    state = JsonLogic_ParserState_Error;
                    goto loop_end;
                }
                index ++;
                break;
            }
            case JsonLogic_ParserState_ObjectStart:
                error = jsonlogic_parsestack_push_object(&stack);
                if (error != JSONLOGIC_ERROR_SUCCESS) {
                    state = JsonLogic_ParserState_Error;
                    goto loop_end;
                }
                index ++;
                break;

            case JsonLogic_ParserState_ObjectAfterStart:
                index ++;
                break;

            case JsonLogic_ParserState_ObjectKey:
            case JsonLogic_ParserState_ObjectValue:
            case JsonLogic_ParserState_ObjectNext:
                index ++;
                break;

            case JsonLogic_ParserState_ObjectEnd:
            {
                JsonLogic_Handle handle = jsonlogic_parsestack_pop(&stack);
                if (!JSONLOGIC_IS_OBJECT(handle)) {
                    jsonlogic_decref(handle);
                    error = JSONLOGIC_ERROR_SYNTAX_ERROR;
                    state = JsonLogic_ParserState_Error;
                    goto loop_end;
                }
                error = jsonlogic_parsestack_handle_value(&stack, handle, &state);
                jsonlogic_decref(handle);
                if (error != JSONLOGIC_ERROR_SUCCESS) {
                    state = JsonLogic_ParserState_Error;
                    goto loop_end;
                }
                index ++;
                break;
            }
            case JsonLogic_ParserState_Error:
                error = JSONLOGIC_ERROR_SYNTAX_ERROR;
                goto loop_end;
                break;

            case JsonLogic_ParserState_Max:
                assert(false);
                state = JsonLogic_ParserState_Error;
                error = JSONLOGIC_ERROR_INTERNAL_ERROR;
                goto loop_end;
                break;
        }
    }
loop_end:

    if (error != JSONLOGIC_ERROR_SUCCESS) {
        JsonLogic_LineInfo info = jsonlogic_get_lineinfo(str, size, index);
        if (infoptr != NULL) {
            *infoptr = info;
        }
        jsonlogic_parsestack_free(&stack);
        return (JsonLogic_Handle){ .intptr = error };
    }

    state = JsonLogic_Parser_Root[state][JSONLOGIC_PARSE_EOF];

    if (state != JsonLogic_ParserState_End) {
        JsonLogic_LineInfo info = jsonlogic_get_lineinfo(str, size, index);
        if (infoptr != NULL) {
            *infoptr = info;
        }
        jsonlogic_parsestack_free(&stack);
        return JsonLogic_Error_SyntaxError;
    }

    JsonLogic_Handle value = jsonlogic_parsestack_pop(&stack);

    jsonlogic_parsestack_free(&stack);

    return value;
}

static const unsigned char JsonLogic_ToHexMap[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'a', 'b', 'c', 'd', 'e', 'f',
};

#include "stringify.c"

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

#define JsonLogic_StrBuf               JsonLogic_Utf8Buf
#define jsonlogic_strbuf_append_double jsonlogic_utf8buf_append_double
#define jsonlogic_strbuf_append_ascii  jsonlogic_utf8buf_append_ascii
#define jsonlogic_strbuf_append_utf16  jsonlogic_utf8buf_append_utf16
#define jsonlogic_stringify_intern     jsonlogic_stringify_utf8_intern
#include "stringify.c"
#undef JsonLogic_StrBuf
#undef jsonlogic_strbuf_append_double
#undef jsonlogic_strbuf_append_ascii
#undef jsonlogic_strbuf_append_utf16
#undef jsonlogic_stringify_intern

char *jsonlogic_stringify_utf8(JsonLogic_Handle value) {
    if (JSONLOGIC_IS_ERROR(value)) {
        errno = EINVAL;
        return NULL;
    }

    JsonLogic_Utf8Buf buf = JSONLOGIC_UTF8BUF_INIT;

    JsonLogic_Error error = jsonlogic_stringify_utf8_intern(&buf, value);
    if (error != JSONLOGIC_ERROR_SUCCESS) {
        JSONLOGIC_DEBUG("jsonlogic_stringify_utf8_intern(): %s", jsonlogic_get_error_message(error));
        jsonlogic_utf8buf_free(&buf);
        return NULL;
    }

    char *result = jsonlogic_utf8buf_take(&buf);

    // in error case the buffer is not freed
    jsonlogic_utf8buf_free(&buf);

    return result;
}

static inline JsonLogic_Error jsonlogic_print_double(FILE *file, double value) {
    int count = fprintf(file, "%g", value);
    if (count < 0) {
        return JSONLOGIC_ERROR_IO_ERROR;
    }
    return JSONLOGIC_ERROR_SUCCESS;
}

static inline JsonLogic_Error jsonlogic_print_ascii(FILE *file, const char *str) {
    if (fputs(str, file) < 0) {
        return JSONLOGIC_ERROR_IO_ERROR;
    }
    return JSONLOGIC_ERROR_SUCCESS;
}

#define JsonLogic_StrBuf               FILE
#define jsonlogic_strbuf_append_double jsonlogic_print_double
#define jsonlogic_strbuf_append_ascii  jsonlogic_print_ascii
#define jsonlogic_strbuf_append_utf16  jsonlogic_print_utf16
#define jsonlogic_stringify_intern     jsonlogic_stringify_file_intern
#include "stringify.c"
#undef JsonLogic_StrBuf
#undef jsonlogic_strbuf_append_double
#undef jsonlogic_strbuf_append_ascii
#undef jsonlogic_strbuf_append_utf16
#undef jsonlogic_stringify_intern

JsonLogic_Error jsonlogic_stringify_file(FILE *file, JsonLogic_Handle value) {
    if (JSONLOGIC_IS_ERROR(value)) {
        return value.intptr;
    }

    return jsonlogic_stringify_file_intern(file, value);
}

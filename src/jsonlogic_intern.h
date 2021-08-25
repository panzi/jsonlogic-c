#ifndef JSONLOGIC_INTERN_H
#define JSONLOGIC_INTERN_H
#pragma once

#include "jsonlogic_defs.h"

#define JsonLogic_PtrMask  (~(uintptr_t)0xffff000000000000)
#define JsonLogic_TypeMask  ((uintptr_t)0xffff000000000000)
#define JsonLogic_MaxNumber ((uintptr_t)0xfff8000000000000)

#ifdef __cplusplus
extern "C" {
#endif

union JsonLogic_Handle;
struct JsonLogic_Array;
struct JsonLogic_String;
struct JsonLogic_Object;

typedef union {
    uintptr_t intptr;
    double    number;
} JsonLogic_Handle;

#define JSONLOGIC_CAST_STRING(handle) ((JsonLogic_String*)((handle).intptr & JsonLogic_PtrMask))
#define JSONLOGIC_CAST_ARRAY( handle) ((JsonLogic_Array*) ((handle).intptr & JsonLogic_PtrMask))
#define JSONLOGIC_CAST_OBJECT(handle) ((JsonLogic_Object*)((handle).intptr & JsonLogic_PtrMask))

#define JSONLOGIC_IS_STRING(handle)  (((handle).intptr & JsonLogic_TypeMask) == JsonLogic_Type_String)
#define JSONLOGIC_IS_OBJECT(handle)  (((handle).intptr & JsonLogic_TypeMask) == JsonLogic_Type_Object)
#define JSONLOGIC_IS_ARRAY(handle)   (((handle).intptr & JsonLogic_TypeMask) == JsonLogic_Type_Array)
#define JSONLOGIC_IS_BOOLEAN(handle) (((handle).intptr & JsonLogic_TypeMask) == JsonLogic_Type_Boolean)
#define JSONLOGIC_IS_NULL(handle)    ((handle).intptr == JsonLogic_Type_Null)
#define JSONLOGIC_IS_NUMBER(handle)  ((handle).intptr < JsonLogic_MaxNumber)

typedef struct JsonLogic_String {
    size_t refcount;
    size_t size;
    JsonLogic_Char str[1];
} JsonLogic_String;

typedef struct JsonLogic_Array {
    size_t refcount;
    size_t size;
    JsonLogic_Handle items[1];
} JsonLogic_Array;

typedef struct JsonLogic_Object_Entry {
    JsonLogic_Handle key;
    JsonLogic_Handle value;
} JsonLogic_Object_Entry;

typedef struct JsonLogic_Object {
    size_t refcount;
    size_t size;
    JsonLogic_Object_Entry items[1];
} JsonLogic_Object;

#ifdef __cplusplus
}
#endif

#include "jsonlogic.h"

#ifdef __cplusplus
extern "C" {
#endif

JSONLOGIC_PRIVATE void jsonlogic_free_string(JsonLogic_String *string);
JSONLOGIC_PRIVATE void jsonlogic_free_array (JsonLogic_Array  *array);
JSONLOGIC_PRIVATE void jsonlogic_free_object(JsonLogic_Object *object);

typedef struct JsonLogic_Buffer {
    size_t size;
    JsonLogic_String *data;
} JsonLogic_Buffer;

#define JSONLOGIC_BUFFER_INIT ((JsonLogic_Buffer){ .size = 0, .data = NULL })
#define JSONLOGIC_CHUNK_SIZE 256

JSONLOGIC_PRIVATE bool jsonlogic_string_equals (JsonLogic_String *a, JsonLogic_String *b);
JSONLOGIC_PRIVATE int  jsonlogic_string_compare(JsonLogic_String *a, JsonLogic_String *b);

JSONLOGIC_PRIVATE bool jsonlogic_buffer_ensure(JsonLogic_Buffer *buf, size_t want_free_size);
JSONLOGIC_PRIVATE bool jsonlogic_buffer_append_latin1(JsonLogic_Buffer *buf, const char *str);
JSONLOGIC_PRIVATE bool jsonlogic_buffer_append_utf16(JsonLogic_Buffer *buf, JsonLogic_Char *str, size_t size);
JSONLOGIC_PRIVATE bool jsonlogic_buffer_append(JsonLogic_Buffer *buf, JsonLogic_Handle handle);
JSONLOGIC_PRIVATE JsonLogic_String *jsonlogic_buffer_take(JsonLogic_Buffer *buf);
JSONLOGIC_PRIVATE void jsonlogic_buffer_free(JsonLogic_Buffer *buf);

#ifdef __cplusplus
}
#endif

#endif

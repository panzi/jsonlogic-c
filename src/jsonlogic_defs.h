#ifndef JSONLOGIC_TYPEDEFS_H
#define JSONLOGIC_TYPEDEFS_H
#pragma once

#include <uchar.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t JsonLogic_Handle;

typedef uint64_t JsonLogic_Error;

typedef struct JsonLogic_Object_Entry {
    JsonLogic_Handle key;
    JsonLogic_Handle value;
} JsonLogic_Object_Entry;

typedef uint64_t JsonLogic_Type;

#define JsonLogic_Type_Number   ((JsonLogic_Type) 1)
#define JsonLogic_Type_Null     ((JsonLogic_Type) 0xfff9000000000000)
#define JsonLogic_Type_Boolean  ((JsonLogic_Type) 0xfffa000000000000)
#define JsonLogic_Type_String   ((JsonLogic_Type) 0xfffb000000000000)
#define JsonLogic_Type_Array    ((JsonLogic_Type) 0xfffc000000000000)
#define JsonLogic_Type_Object   ((JsonLogic_Type) 0xfffd000000000000)
#define JsonLogic_Type_Error    ((JsonLogic_Type) 0xfffe000000000000)

#define JSONLOGIC_NULL  JsonLogic_Type_Null
#define JSONLOGIC_TURE  (JsonLogic_Type_Boolean | 1)
#define JSONLOGIC_FALSE (JsonLogic_Type_Boolean | 0)

#define JSONLOGIC_ERROR_SUCCESS            (JsonLogic_Type_Error | 0)
#define JSONLOGIC_ERROR_OUT_OF_MEMORY      (JsonLogic_Type_Error | 1)
#define JSONLOGIC_ERROR_ILLEGAL_OPERATION  (JsonLogic_Type_Error | 2)
#define JSONLOGIC_ERROR_ILLEGAL_ARGUMENT   (JsonLogic_Type_Error | 3)
#define JSONLOGIC_ERROR_INTERNAL_ERROR     (JsonLogic_Type_Error | 4)
#define JSONLOGIC_ERROR_STOP_ITERATION     (JsonLogic_Type_Error | 5)
#define JSONLOGIC_ERROR_IO_ERROR           (JsonLogic_Type_Error | 6)
#define JSONLOGIC_ERROR_SYNTAX_ERROR       (JsonLogic_Type_Error | 7)
#define JSONLOGIC_ERROR_UNICODE_ERROR      (JsonLogic_Type_Error | 8)

#if defined(_WIN32) || defined(_WIN64)
    #define JSONLOGIC_WINDOWS
#endif

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    #ifdef JSONLOGIC_STATIC
        #define JSONLOGIC_EXPORT
    #else
        #ifdef JSONLOGIC_WIN_EXPORT
            // Exporting...
            #define JSONLOGIC_EXPORT __declspec(dllexport)
        #else
            #define JSONLOGIC_EXPORT __declspec(dllimport)
        #endif
    #endif
    #define JSONLOGIC_EXPORT_CONST JSONLOGIC_EXPORT extern
    #define JSONLOGIC_PRIVATE
#else
    #if (defined(__GNUC__) && __GNUC__ >= 4) || defined(__clang__)
        #define JSONLOGIC_EXPORT  __attribute__ ((visibility ("default")))
        #define JSONLOGIC_PRIVATE __attribute__ ((visibility ("hidden")))
        #define JSONLOGIC_EXPORT_CONST JSONLOGIC_EXPORT extern
    #else
        #define JSONLOGIC_EXPORT extern
        #define JSONLOGIC_PRIVATE
        #define JSONLOGIC_EXPORT_CONST extern
    #endif
#endif

#ifdef __cplusplus
}
#endif


#endif

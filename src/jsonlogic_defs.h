#ifndef JSONLOGIC_TYPEDEFS_H
#define JSONLOGIC_TYPEDEFS_H
#pragma once

#include <uchar.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(JSONLOGIC_INTERN_H)
typedef union {
    uint64_t intptr;
    double   number;
} JsonLogic_Handle;
#else
typedef uint64_t JsonLogic_Handle;
#endif

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

#define JSONLOGIC_ERROR_SUCCESS            (JsonLogic_Type_Error | 0)
#define JSONLOGIC_ERROR_OUT_OF_MEMORY      (JsonLogic_Type_Error | 1)
#define JSONLOGIC_ERROR_ILLEGAL_OPERATION  (JsonLogic_Type_Error | 2)
#define JSONLOGIC_ERROR_ILLEGAL_ARGUMENT   (JsonLogic_Type_Error | 3)
#define JSONLOGIC_ERROR_INTERNAL_ERROR     (JsonLogic_Type_Error | 4)

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    #ifdef WIN_EXPORT
        // Exporting...
        #ifdef __GNUC__
        #define JSONLOGIC_EXPORT __attribute__ ((dllexport))
        #else
        #define JSONLOGIC_EXPORT __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
        #endif
    #else
        #ifdef __GNUC__
        #define JSONLOGIC_EXPORT __attribute__ ((dllimport))
        #else
        #define JSONLOGIC_EXPORT __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
        #endif
    #endif
    #define JSONLOGIC_PRIVATE
#else
    #if (defined(__GNUC__) && __GNUC__ >= 4) || defined(__clang__)
        #define JSONLOGIC_EXPORT  __attribute__ ((visibility ("default")))
        #define JSONLOGIC_PRIVATE __attribute__ ((visibility ("hidden")))
    #else
        #define JSONLOGIC_EXPORT extern
        #define JSONLOGIC_PRIVATE
    #endif
#endif

#ifdef __cplusplus
}
#endif


#endif

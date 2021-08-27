#ifndef JSONLOGIC_TYPEDEFS_H
#define JSONLOGIC_TYPEDEFS_H
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(JSONLOGIC_INTERN_H)
typedef union {
    uintptr_t intptr;
    double    number;
} JsonLogic_Handle;
#else
typedef struct JsonLogic_Struct* JsonLogic_Handle;
#endif

typedef struct JsonLogic_Object_Entry {
    JsonLogic_Handle key;
    JsonLogic_Handle value;
} JsonLogic_Object_Entry;

typedef uint16_t JsonLogic_Char;
typedef uint64_t JsonLogic_Type;

#define JsonLogic_Type_Number   ((JsonLogic_Type) 1)
#define JsonLogic_Type_Null     ((JsonLogic_Type) 0xfff9000000000000)
#define JsonLogic_Type_Boolean  ((JsonLogic_Type) 0xfffa000000000000)
#define JsonLogic_Type_String   ((JsonLogic_Type) 0xfffb000000000000)
#define JsonLogic_Type_Array    ((JsonLogic_Type) 0xfffc000000000000)
#define JsonLogic_Type_Object   ((JsonLogic_Type) 0xfffd000000000000)
#define JsonLogic_Type_Error    ((JsonLogic_Type) 0xfffe000000000000)

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

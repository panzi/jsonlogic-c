#ifndef JSONLOGIC_EXTRAS_H
#define JSONLOGIC_EXTRAS_H
#pragma once

#include "jsonlogic.h"

#ifdef __cplusplus
extern "C" {
#endif

JSONLOGIC_EXPORT extern const JsonLogic_Operations JsonLogic_Extras;

JSONLOGIC_EXPORT double jsonlogic_now();
JSONLOGIC_EXPORT double jsonlogic_parse_time(JsonLogic_Handle handle);
JSONLOGIC_EXPORT double jsonlogic_parse_date_time(const char16_t *str, size_t size);
JSONLOGIC_EXPORT JsonLogic_Handle jsonlogic_format_date_time(double timestamp);

#ifdef __cplusplus
}
#endif

#endif

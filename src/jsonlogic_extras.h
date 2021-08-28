#ifndef JSONLOGIC_EXTRAS_H
#define JSONLOGIC_EXTRAS_H
#pragma once

#include "jsonlogic.h"

#ifdef __cplusplus
extern "C" {
#endif

JSONLOGIC_EXPORT double jsonlogic_now();
JSONLOGIC_EXPORT double jsonlogic_parse_time(JsonLogic_Handle handle);
JSONLOGIC_EXPORT double jsonlogic_parse_date_time(const char16_t *str, size_t size);

JSONLOGIC_EXPORT JsonLogic_Operation_Entry *jsonlogic_add_extras(const JsonLogic_Operation_Entry operations[], size_t *operations_size);

#ifdef __cplusplus
}
#endif

#endif

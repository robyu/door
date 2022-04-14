#ifndef JSON_CONFIG_H
#define JSON_CONFIG_H

#include <ArduinoJson.h>

void jc_get_config_from_file(const char *fname_ptr, JsonDocument *doc_ptr);

#endif


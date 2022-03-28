#ifndef JSON_CONFIG_H
#define JSON_CONFIG_H

#include <ArduinoJson.h>

void jc_get_config_from_file(const char *fname_ptr, DynamicJsonDocument *doc_ptr);

#endif


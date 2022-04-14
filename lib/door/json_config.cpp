#include <stdarg.h>
#include <stdlib.h>
#include <Arduino.h>
#include "utils.h"
#include "LittleFS.h"
#include "json_config.h"

void jc_get_config_from_file(const char *fname_ptr, JsonDocument *doc_ptr)
{
    String txt;
    File file;
    DeserializationError error;

    LittleFS.begin();

    // load file into buffer
    file = LittleFS.open(fname_ptr,"r");
    
    while (file.available()) {
        String line = file.readStringUntil('\n');
        txt = txt + line;
    }
    file.close();

    UTILS_ASSERT(doc_ptr->capacity() > strlen(txt.c_str()));
    error = deserializeJson(*doc_ptr, txt);
    UTILS_ASSERT(error==false);
}


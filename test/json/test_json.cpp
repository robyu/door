#include <Arduino.h>
#include <unity.h>
#include <ArduinoJson.h>
#include "LittleFS.h"

#define JSONFNAME "/unit_tests.json"
void setup() {
    bool b;
    UNITY_BEGIN();
    Serial.begin(115200);

    b = LittleFS.begin();
    if(!b)
    {
        Serial.println("An Error has occurred while mounting LittleFS");
    }
    TEST_ASSERT_TRUE(b);
}

void test_deserialize_string(void)
{
    String json = " \
{ \
    \"a\": [ \
        1, \
        2 \
    ], \
    \"b\": { \
        \"aa\": \"foo\", \
        \"bb\": 3 \
    } \
}";
    StaticJsonDocument<1024> doc;
    
    Serial.printf("%s\n", json.c_str());
    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, json);
    TEST_ASSERT_TRUE(error==false);

    TEST_ASSERT_TRUE(doc["a"][0] == 1);
    TEST_ASSERT_TRUE(doc["a"][1] == 2);
    TEST_ASSERT_TRUE(doc["b"]["aa"]=="foo");
    TEST_ASSERT_TRUE(doc["b"]["bb"]==3);
    
}


void test_deserialize_file(void)
{
    File file;
    String json;
    StaticJsonDocument<1024> doc0;
    StaticJsonDocument<1024> doc1;
    DeserializationError error;
    
    // load file into buffer
    bool exists = LittleFS.exists(JSONFNAME);
    TEST_ASSERT_TRUE(exists==true);

    file = LittleFS.open(JSONFNAME,"r");
    Serial.printf("------------------\n");
    while (file.available()) {
        String line = file.readStringUntil('\n');
        json = json + line;
        Serial.println(line);
    }
    Serial.printf("------------------\n");
    Serial.println(json);
    file.close();

    error = deserializeJson(doc0, json);
    TEST_ASSERT_TRUE(error==false);

    doc1 = doc0["test_json"];
    TEST_ASSERT_TRUE(doc1["a"][0] == 1);
}

void json_deserialize_from_file(const char *fname_ptr, DynamicJsonDocument *doc_ptr)
{
    String json;
    File file;
    DeserializationError error;

    // load file into buffer
    file = LittleFS.open(fname_ptr,"r");
    
    while (file.available()) {
        String line = file.readStringUntil('\n');
        json = json + line;
    }
    file.close();

    error = deserializeJson(*doc_ptr, json);
    TEST_ASSERT_TRUE(error==false);
}

void test_deserialize_file2(void)
{
    DynamicJsonDocument top_doc(1024);
    DynamicJsonDocument test_doc(1024);
    String fname(JSONFNAME);
    
    json_deserialize_from_file(fname.c_str(), &top_doc);
    TEST_ASSERT_TRUE(top_doc["test_json"]["a"][0]==1);
}


void loop() {
    RUN_TEST(test_deserialize_string);
    RUN_TEST(test_deserialize_file);
    RUN_TEST(test_deserialize_file2);
    LittleFS.end();
    UNITY_END();
}

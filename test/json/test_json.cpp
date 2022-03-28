#include <Arduino.h>
#include <unity.h>
#include <ArduinoJson.h>
#include "LittleFS.h"
#include "json_config.h"

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
    String txt;
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
        txt = txt + line;
        Serial.println(line);
    }
    Serial.printf("------------------\n");
    Serial.println(txt);
    file.close();

    error = deserializeJson(doc0, txt);
    TEST_ASSERT_TRUE(error==false);

    doc1 = doc0["test_json"];
    TEST_ASSERT_TRUE(doc1["a"][0] == 1);
}

void test_deserialize_file2(void)
{
    DynamicJsonDocument top_doc(1024);
    DynamicJsonDocument test_doc(1024);
    String fname(JSONFNAME);
    const char *str_ptr;
    String str_obj = "";
    float bb;
    
    jc_get_config_from_file(fname.c_str(), &top_doc);


    // get float value
    TEST_ASSERT_TRUE(top_doc["test_json"]["a"][0]==1.0);

    // assign a char ptr from a json array
    str_ptr = top_doc["test_json"]["c"];
    Serial.printf("100 %s\n",str_ptr);
    TEST_ASSERT_TRUE(strcmp(str_ptr, "the quick brown fox")==0);

    int len = strlen(top_doc["test_json"]["c"]);
    Serial.printf("101 strlen()=%d\n",len);
    

    /* note that printing the json array element directly DOES NOT work
     unless you cast it to (const char *)*/
    Serial.printf("200 %s\n",(const char *)top_doc["test_json"]["c"]);

    /* but you can use the gawdawful as<T>() syntax to perform a cast */
    Serial.printf("210 %s\n",top_doc["test_json"]["c"].as<const char *>());
    

    // assign a String obj from a json array
    str_obj += String(top_doc["test_json"]["c"]);
    Serial.printf("300 %s\n",str_obj.c_str());
    TEST_ASSERT_TRUE(str_obj == String("the quick brown fox"));


    // get 3rd level object
    bb = top_doc["test_json"]["b"]["bb"];
    Serial.printf("400 %f\n", bb);
    TEST_ASSERT_TRUE(bb==3.0);

}


void loop() {
    RUN_TEST(test_deserialize_string);
    RUN_TEST(test_deserialize_file);
    RUN_TEST(test_deserialize_file2);
    LittleFS.end();
    UNITY_END();
}

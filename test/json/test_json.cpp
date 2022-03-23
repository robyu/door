#include <Arduino.h>
#include <unity.h>
#include <ArduinoJson.h>


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
    TEST_ASSERT_TRUE(true);
}


void loop() {
    // RUN_TEST(test_write);
    RUN_TEST(test_serialize_string);
    LittleFS.end();
    UNITY_END();
}

#include <Arduino.h>
#include <unity.h>

#include "LittleFS.h"

#define TESTFNAME "/test_example.txt"

/*
see https://www.mischianti.org/2021/04/01/esp32-integrated-littlefs-filesystem-5/
*/
void setup() {
    bool b;
    Serial.begin(115200);

    b = LittleFS.begin();
    if(!b)
    {
        Serial.println("An Error has occurred while mounting LittleFS");
    }
    TEST_ASSERT_TRUE(b);

    LittleFS.remove(TESTFNAME);
    bool exists = LittleFS.exists(TESTFNAME);
    TEST_ASSERT_TRUE(exists==false);
    
    // File file = LittleFS.open("/test_example.txt", "r");
    // if(!file){
    //     Serial.println("Failed to open file for reading");
    //     return;
    // }

    // Serial.println("File Content:");
    // while(file.available()){
    //     Serial.write(file.read());
    // }
    // file.close();
}

void test_write(void)
{
    File file = LittleFS.open("/test_example.txt", "w");
    uint8_t pbuf[] = "helloworld\0";
    bool exists;
    
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    file.write(pbuf, sizeof(pbuf));
    file.close();
    TEST_ASSERT_EQUAL(1, 1);

    exists = LittleFS.exists(TESTFNAME);
    TEST_ASSERT_TRUE(exists==true);
    LittleFS.remove(TESTFNAME);
}

void test_write_read(void)
{
    File file;
    const char pwrite_buf[] = "helloworld\0";
    char pread_buf[255];
    int numbytes;
    
    file = LittleFS.open("/test_example.txt", "w");
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    file.write(pwrite_buf, sizeof(pwrite_buf));
    file.close();

    file = LittleFS.open(TESTFNAME,"r");
    numbytes = file.readBytesUntil('\n', pread_buf, sizeof(pread_buf));
    pread_buf[numbytes] = '\n';

    Serial.println("write (" + String(pwrite_buf) + ")");
    Serial.println("read (" + String(pread_buf) + ")");
    Serial.println("len pwrite_buf= " + String(strlen(pwrite_buf)));
    Serial.println("len pread_buf= " + String(strlen(pread_buf)));

    TEST_ASSERT_TRUE(String(pwrite_buf)==String(pread_buf));
    LittleFS.remove(TESTFNAME);
}

void loop() {
    RUN_TEST(test_write);
    RUN_TEST(test_write_read);
    LittleFS.end();
    UNITY_END();
}

#include <Arduino.h>
#include <unity.h>

#include "LittleFS.h"

#define TESTFNAME "/test_example.txt"
#define JSONFNAME "/unit_tests.json"

/*
see https://www.mischianti.org/2021/04/01/esp32-integrated-littlefs-filesystem-5/
*/
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
    pread_buf[numbytes] = '\0';

    // Serial.println("write (" + String(pwrite_buf) + ")");
    // Serial.println("read (" + String(pread_buf) + ")");
    // Serial.println("len pwrite_buf= " + String(strlen(pwrite_buf)));
    // Serial.println("len pread_buf= " + String(strlen(pread_buf)));
    Serial.printf("\n");
    Serial.printf("wrote (%s)\n", pwrite_buf);
    Serial.printf("read (%s)\n", pread_buf);
    Serial.printf("len pwrite_buf= %d\n", strlen(pwrite_buf));
    Serial.printf("len pread_buf=%d\n",strlen(pread_buf));

    TEST_ASSERT_TRUE(String(pwrite_buf)==String(pread_buf));
    LittleFS.remove(TESTFNAME);
}

/*
to upload door/data/test_littlefs_uploaded.txt,
first run:
dpio run -t uploadfs

then test_littlefs_uploaded.txt should appear on the esp8266 under /

Notes: 
you DO NOT need to set [platformio] data_dir. In fact, this setting seems to be ignored.

*/
void test_read_uploaded_file(void)
{
    File file;
    boolean found_flag = false;
    
    bool exists = LittleFS.exists(JSONFNAME);
    TEST_ASSERT_TRUE(exists==true);

    file = LittleFS.open(JSONFNAME,"r");
    Serial.printf("------------------\n");
    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        Serial.println(line);
        if (line.startsWith(String("\"wifi")))
        {
            found_flag |= true;
        }
    }
    TEST_ASSERT_TRUE(found_flag);
                                 
    Serial.printf("------------------\n");
    file.close();
}

void test_list_files(void)
{
    String str = "";

    Dir dir = LittleFS.openDir("/");
    while (dir.next()) {
        Serial.printf("%s | %d\n", dir.fileName().c_str(), dir.fileSize());
        // str += dir.fileName();
        // str += " / ";
        // str += dir.fileSize();
        // str += "\r\n";
    }
    Serial.print(str);
    TEST_ASSERT_TRUE(true);
}

void test_load_txt_into_buffer(void)
{
    File file;
    String txt;
    
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
}

void loop() {
     RUN_TEST(test_write);
    RUN_TEST(test_write_read);
    RUN_TEST(test_read_uploaded_file);
    RUN_TEST(test_list_files);
    //RUN_TEST(test_load_txt_into_buffer);
    LittleFS.end();
    UNITY_END();
}

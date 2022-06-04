#include <Arduino.h>
#include <unity.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "json_config.h"
#include "test_mqtt_common.h"

void get_wifi_credential(const char *fname, char *ssid_ptr, size_t len_ssid, char *pwd_ptr, size_t len_pwd)
{
    DynamicJsonDocument json(1024);
    jc_get_config_from_file(fname, &json);
    
    Serial.printf("ssid=(%s)\n",
                  (const char *)json["wifi_access"]["ssid"]);

    TEST_ASSERT_TRUE(strlen((const char *)json["wifi_access"]["ssid"]) < len_ssid);
    strncpy(ssid_ptr,
            json["wifi_access"]["ssid"].as<const char *>(),
            len_ssid);

    TEST_ASSERT_TRUE(strlen((const char *)json["wifi_access"]["pwd"]) < len_pwd);
    strncpy(pwd_ptr,
            json["wifi_access"]["pwd"].as<const char *>(),
            len_pwd);
}

void get_broker_params(const char *fname,
                       char *public_mqtt_broker,
                       char *local_mqtt_broker,
                       size_t max_len_broker,
                       char *topic,
                       size_t max_len_topic,
                       int *mqtt_port_pntr)
{
    DynamicJsonDocument json(1024);
    const char *pc;
    int i;
    
    jc_get_config_from_file(fname, &json);

    pc = json["test_mqtt"]["public_mqtt_broker"];
    if (public_mqtt_broker != NULL)
    {
        TEST_ASSERT_TRUE(strlen(pc) < max_len_broker);
        strncpy(public_mqtt_broker, pc,  max_len_broker);
        Serial.printf("public mqtt broker = (%s)\n", public_mqtt_broker);
    }

    pc = json["test_mqtt"]["local_mqtt_broker"];
    if (local_mqtt_broker != NULL)
    {
        TEST_ASSERT_TRUE(strlen(pc) < max_len_broker);
        strncpy(local_mqtt_broker, pc,  max_len_broker);
        Serial.printf("local mqtt broker = (%s)\n", local_mqtt_broker);
    }
    
    i = json["test_mqtt"]["mqtt_port"].as<int>();
    *mqtt_port_pntr = i;
    Serial.printf("mqtt port = %d\n", *mqtt_port_pntr);

    pc = json["test_mqtt"]["topic"];
    TEST_ASSERT_TRUE(strlen(pc) < max_len_topic);
    strncpy(topic, pc,  max_len_topic);
    Serial.printf("topic = (%s)\n", topic);
    
}

void setup_wifi(void)
{
    int max_attempts = 30;
    int count=0;
    char ssid_ptr[256];
    char pwd_ptr[256];
    get_wifi_credential(JSONFNAME, ssid_ptr,sizeof(ssid_ptr),pwd_ptr,sizeof(pwd_ptr));
    WiFi.begin(ssid_ptr, pwd_ptr);
    while ((WiFi.status() != WL_CONNECTED) && (count < max_attempts)) {
        delay(1000);
        Serial.printf("Connecting to WiFi (attempt %d)\n",count);
        count++;
    }
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi setup FAILED");
    }
    TEST_ASSERT_TRUE(WiFi.status() == WL_CONNECTED);

    Serial.println("Connected to the WiFi network");
}


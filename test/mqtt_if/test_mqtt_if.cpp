#include <Arduino.h>
#include <unity.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "json_config.h"
#include "test_mqtt_common.h"
#include "mqttif.h"
#include "utils.h"

// MQTT Broker Parameters
static char public_mqtt_broker[1024];
static char mqtt_topic[1024];
static int mqtt_port;

void setup(void) {
    Serial.begin(115200);
    UNITY_BEGIN();

    setup_wifi();

    if (WiFi.status() != WL_CONNECTED)
    {
        // no point in continuing
        TEST_ABORT();
    }

    get_broker_params(JSONFNAME,
                      public_mqtt_broker,
                      NULL,
                      sizeof(public_mqtt_broker),
                      mqtt_topic,
                      sizeof(mqtt_topic),
                      &mqtt_port);
                      
}

void test_connect_public_broker(void)
{
    mqttif_config_t config;
    mqttif_t mqttif;
    bool connected;

    TEST_ASSERT(WiFi.status()==WL_CONNECTED);

    
    Serial.printf("===================================================\n");
    mqttif_set_default_config(&config, public_mqtt_broker, mqtt_port);
    
    mqttif_init(&mqttif, &config);
    UTILS_ASSERT(0);

    Serial.printf("===================================================\n");

    connected = mqttif_is_connected(&mqttif);
    mqttif_disconnect(&mqttif);

    TEST_ASSERT_TRUE(connected==true);
}


void loop() {
    RUN_TEST(test_connect_public_broker);
    UNITY_END();
}



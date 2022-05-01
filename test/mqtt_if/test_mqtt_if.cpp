#include <Arduino.h>
#include <unity.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "json_config.h"
#include "test_mqtt_common.h"
#include "mqttif.h"
#include "utils.h"

// MQTT Broker Parameters
static char public_mqtt_broker[512];
static char mqtt_topic[MQTTIF_MAX_LEN_STR];
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

    Serial.printf("===================================================\n");

    connected = mqttif_is_connected(&mqttif);
    Serial.printf("connected=%d\n",(int)connected);
    TEST_ASSERT_TRUE(connected==true);

    mqttif_disconnect(&mqttif);

}

void test_pub_sub(void)
{
    mqttif_config_t config;
    mqttif_t mqttif;
    char topic[] = "home/garage/door";
    boolean retval;
    char prx_topic[MQTTIF_MAX_LEN_STR];
    char prx_payload[MQTTIF_MAX_LEN_STR];
    
    mqttif_set_default_config(&config, public_mqtt_broker, mqtt_port);
    mqttif_init(&mqttif, &config);

    // subscribe
    retval=mqttif_sub_topic(&mqttif, topic);
    Serial.printf("subscribed to (%s) returned %d\n",topic, int(retval));
    TEST_ASSERT_TRUE(retval);

    //
    // loop until I receive a topic message
    // publish to same topic on first iteration
    int count = 0;
    int max_attempts = 100;
    int num_msgs_rcvd = 0;
    while ((num_msgs_rcvd <= 0) && (count < max_attempts))
    {
        int msgs_in_queue;
        if (count==0)
        {
            retval = mqttif_publish(&mqttif, topic, "hello hello");
            TEST_ASSERT_TRUE(retval);
        }
        delay(100);
        mqttif_update(&mqttif);
        Serial.printf("count %d\n",count);
        count++;


        msgs_in_queue = mqttif_check_rx_msgs(&mqttif, prx_topic, prx_payload, MQTTIF_MAX_LEN_STR);
        num_msgs_rcvd += msgs_in_queue;
        if (msgs_in_queue)
        {
            Serial.printf("%s:%s\n",prx_topic, prx_payload);
        }
    }
    mqttif_disconnect(&mqttif);
}

void loop() {
    RUN_TEST(test_connect_public_broker);
    RUN_TEST(test_pub_sub);
    UNITY_END();
}



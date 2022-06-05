#include <Arduino.h>
#include <unity.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "json_config.h"
#include "test_mqtt_common.h"


// MQTT Broker Parameters
static char public_mqtt_broker[1024];
static char local_mqtt_broker[1024];
static char mqtt_topic[MQTT_MAX_PACKET_SIZE];
static int mqtt_port;

//
// pubsubclient api documentation here: https://pubsubclient.knolleary.net/api
static WiFiClient eth_client;
static PubSubClient *pmqtt_client = new PubSubClient(eth_client);

unsigned int rx_length = 0;
void rx_callback(char *topic, byte *payload, unsigned int length) {
    rx_length = length;
    Serial.print("Message arrived in topic: ");
    Serial.println(topic);
    Serial.print("Message 0:");

    // the payload is not null terminated, so you can't treat it like a string
    for (unsigned int i = 0; i < length; i++) {
        Serial.print((char) payload[i]);
    }
    Serial.println();
    Serial.println("-----------------------");
}


void connect_mqtt_broker(const char *broker_addr, int mqtt_port, int max_attempts)
{
    int count = 0;
    
    //connecting to a mqtt broker
    pmqtt_client->setServer(broker_addr, mqtt_port);
    pmqtt_client->setCallback(rx_callback);
    while (!pmqtt_client->connected() && (count < max_attempts)) {
        String client_id = "esp8266-client-";
        Serial.printf("attempt %d\n",count);  // printf() works?!
        count++;
        client_id += String(WiFi.macAddress());
        Serial.printf("The client %s connects to mqtt broker %s\n", client_id.c_str(), broker_addr);
        if (pmqtt_client->connect(client_id.c_str())) {
            Serial.println("mqtt broker connected");
        } else {
            Serial.print("failed with state ");
            Serial.println(pmqtt_client->state());
            delay(2000);
        }
    }
    Serial.printf("Connection state = (%s) after (%d) attempts\n", String(pmqtt_client->connected()).c_str(), count);
}

void setup(void) {
    Serial.begin(115200);
    UNITY_BEGIN();

    randomSeed(millis());
    setup_wifi();

    if (WiFi.status() != WL_CONNECTED)
    {
        // no point in continuing
        TEST_ABORT();
    }

    get_broker_params(JSONFNAME,
                      public_mqtt_broker,
                      local_mqtt_broker,
                      sizeof(public_mqtt_broker),
                      mqtt_topic,
                      sizeof(mqtt_topic),
                      &mqtt_port);
                      
}

void test_connect_public_broker(void)
{
    connect_mqtt_broker(public_mqtt_broker, mqtt_port, 5);
    TEST_ASSERT_TRUE(pmqtt_client->connected());
    pmqtt_client->disconnect();
}

void test_connect_local_broker(void)
{
    connect_mqtt_broker(local_mqtt_broker, mqtt_port, 5);
    if (pmqtt_client->connected()==false)
    {
        Serial.printf("\n");
        Serial.printf("===================================\n");
        Serial.printf("could not connected to mqtt broker at %s\n", local_mqtt_broker);
        Serial.printf("did you start the broker docker image in ./mqtt-test-broker ?");
    }

    TEST_ASSERT_TRUE(pmqtt_client->connected());
    pmqtt_client->disconnect();
}

void test_pub_sub(void)
{
    char topic[] = "home/unit/test";
    boolean retval;
    
    connect_mqtt_broker(local_mqtt_broker, mqtt_port, 5);
    TEST_ASSERT_TRUE(pmqtt_client->connected());

    // subscribe
    retval=pmqtt_client->subscribe(topic);
    Serial.printf("subscribed to (%s) returned %d\n",topic, int(retval));
    TEST_ASSERT_TRUE(retval);

    //
    // loop until I receive a topic message
    // msgs are received by rx_callback
    int count = 0;
    int max_attempts = 100;
    while ((rx_length <= 0) && (count < max_attempts))
    {
        int serialnum = random();
        String msg = "hello hello " + String(serialnum);
        
        // publish to same topic on first iteration
        if (count==0)
        {
            Serial.printf("publishing msg (%s)\n", msg.c_str());
            retval = pmqtt_client->publish(topic, msg.c_str());
            TEST_ASSERT_TRUE(retval);
        }
        pmqtt_client->loop();  // important!
        Serial.printf("count %d\n",count);
        count++;

        delay(100);
    }
    TEST_ASSERT_TRUE(rx_length > 0);
    pmqtt_client->disconnect();
}

void test_smoke(void)
{
    // verify network connection
    TEST_ASSERT_TRUE(true);
}


void loop() {
    RUN_TEST(test_smoke);
    // RUN_TEST(test_connect_public_broker);
    RUN_TEST(test_connect_local_broker);
    RUN_TEST(test_pub_sub);
    pmqtt_client->disconnect();
    UNITY_END();
    delay(1000);
}



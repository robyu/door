#include <Arduino.h>
#include <unity.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// WiFi
const char *ssid = "sleestak"; // Enter your WiFi name
const char *password = "P00pietou";  // Enter WiFi password

// MQTT Broker
const char *public_mqtt_broker = "broker.emqx.io";
const char *local_mqtt_broker = "ryu-mbp-2020.local";
const char *topic = "esp8266/test";
const int mqtt_port = 1883;

//
// pubsubclient api documentation here: https://pubsubclient.knolleary.net/api
WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi(void)
{
    int max_attempts = 20;
    int count=0;
    // connecting to a WiFi network
    WiFi.begin(ssid, password);
    while ((WiFi.status() != WL_CONNECTED) && (count < max_attempts)) {
        delay(500);
        Serial.printf("Connecting to WiFi (attempt %d)\n",count);
        count++;
    }
    Serial.println("Connected to the WiFi network");
}

void rx_callback(char *topic, byte *payload, unsigned int length) {
    Serial.print("Message arrived in topic: ");
    Serial.println(topic);
    Serial.print("Message:");
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
    client.setServer(broker_addr, mqtt_port);
    client.setCallback(rx_callback);
    while (!client.connected() && (count < max_attempts)) {
        String client_id = "esp8266-client-";
        Serial.printf("attempt %d\n",count);  // printf() works?!
        count++;
        client_id += String(WiFi.macAddress());
        Serial.printf("The client %s connects to mqtt broker %s\n", client_id.c_str(), broker_addr);
        if (client.connect(client_id.c_str())) {
            Serial.println("mqtt broker connected");
        } else {
            Serial.print("failed with state ");
            Serial.println(client.state());
            delay(2000);
        }
    }
}

void setup(void) {
    Serial.begin(115200);
    UNITY_BEGIN();

    setup_wifi();

    if (WiFi.status() != WL_CONNECTED)
    {
        // no point in continuing
        TEST_ABORT();
    }
}

void test_connect_public_broker(void)
{
    connect_mqtt_broker(public_mqtt_broker, mqtt_port, 5);
    TEST_ASSERT_TRUE(client.connected());
    client.disconnect();
}

void test_connect_local_broker(void)
{
    connect_mqtt_broker(local_mqtt_broker, mqtt_port, 5);
    if (client.connected()==false)
    {
        Serial.printf("\n");
        Serial.printf("could not connected to mqtt broker at %s\n", local_mqtt_broker);
        Serial.printf("did you start the broker docker image in ./mqtt-test-broker ?");
    }
    TEST_ASSERT_TRUE(client.connected());
    client.disconnect();
}

void test_pub_sub(void)
{
    // publish and subscribe
    client.publish(topic, "hello emqx");
    client.subscribe(topic);
    TEST_ASSERT_TRUE(1==1);
    client.disconnect();
}

void test_smoke(void)
{
    // verify network connection
    TEST_ASSERT_TRUE(true);
}


void loop() {
    RUN_TEST(test_smoke);
    RUN_TEST(test_connect_public_broker);
    RUN_TEST(test_connect_local_broker);
    //RUN_TEST(test_pub_sub);
    client.disconnect();
    UNITY_END();
}



#include <Arduino.h>
#include <unity.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "json_config.h"

#define JSONFNAME "/unit_tests.json"


// MQTT Broker Parameters
char public_mqtt_broker[1024];
char local_mqtt_broker[1024];
char mqtt_topic[1024];
int mqtt_port;

//
// pubsubclient api documentation here: https://pubsubclient.knolleary.net/api
WiFiClient eth_client;
PubSubClient *pmqtt_client = new PubSubClient(eth_client);

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
    TEST_ASSERT_TRUE(strlen(pc) < max_len_broker);
    strncpy(public_mqtt_broker, pc,  max_len_broker);
    Serial.printf("public mqtt broker = (%s)\n", public_mqtt_broker);

    pc = json["test_mqtt"]["local_mqtt_broker"];
    TEST_ASSERT_TRUE(strlen(pc) < max_len_broker);
    strncpy(local_mqtt_broker, pc,  max_len_broker);
    Serial.printf("local mqtt broker = (%s)\n", local_mqtt_broker);
    
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
    int max_attempts = 20;
    int count=0;
    char ssid_ptr[256];
    char pwd_ptr[256];
    get_wifi_credential(JSONFNAME, ssid_ptr,sizeof(ssid_ptr),pwd_ptr,sizeof(pwd_ptr));
    WiFi.begin(ssid_ptr, pwd_ptr);
    while ((WiFi.status() != WL_CONNECTED) && (count < max_attempts)) {
        delay(500);
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
    char topic[] = "home/garage/door";
    boolean retval;
    
    connect_mqtt_broker(public_mqtt_broker, mqtt_port, 5);
    TEST_ASSERT_TRUE(pmqtt_client->connected());

    // subscribe
    retval=pmqtt_client->subscribe(topic);
    Serial.printf("subscribed to (%s) returned %d\n",topic, int(retval));
    TEST_ASSERT_TRUE(retval);

    //
    // loop until I receive a topic message
    // publish to same topic on first iteration
    int count = 0;
    int max_attempts = 100;
    while ((rx_length <= 0) && (count < max_attempts))
    {
        if (count==0)
        {
            retval = pmqtt_client->publish(topic, "hello hello");
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
    RUN_TEST(test_connect_public_broker);
    RUN_TEST(test_connect_local_broker);
    RUN_TEST(test_pub_sub);
    pmqtt_client->disconnect();
    UNITY_END();
}



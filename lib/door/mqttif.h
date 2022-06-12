#ifndef MQTT_IF_H
#define MQTT_IF_H


#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define MQTTIF_MAX_LEN_STR 256

typedef struct
{
    char pbroker_addr[MQTTIF_MAX_LEN_STR];
    int mqtt_port;
    int max_num_connect_attempts;
} mqttif_config_t;

#define MQTTIF_NUM_RX 10

typedef struct
{
    char ptopic[MQTTIF_MAX_LEN_STR];
    byte ppayload[MQTTIF_MAX_LEN_STR];
    unsigned int len_payload; 
} mqttif_rx_msg_t;

typedef struct
{
    mqttif_config_t config;
    PubSubClient *pmqtt_client;

    int num_rx_msgs;
    mqttif_rx_msg_t prx_msgs[MQTTIF_NUM_RX];
} mqttif_t;

void mqttif_set_default_config(mqttif_config_t *pconfig, const char *pbroker_addr, int mqtt_port);
void mqttif_init(mqttif_t *p, const mqttif_config_t *pconfig);
bool mqttif_sub_topic(mqttif_t *p, const char *ptopic);
bool mqttif_is_connected(const mqttif_t *p);
bool mqttif_publish(mqttif_t *p, const char *ptopic, const char *ppayload);
bool mqttif_update(mqttif_t *p);

//int mqttif_get_num_rx_msgs(mqttif_t *p);
int mqttif_check_rx_msgs(mqttif_t *p, char *ptopic, char *ppayload, int len_str);
void mqttif_disconnect(mqttif_t *p);

#endif // MQTT_IF_H



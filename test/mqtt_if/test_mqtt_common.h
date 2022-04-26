#ifndef TEST_MQTT_COMMON_H
#define TEST_MQTT_COMMON_H

#include <Arduino.h>
#include <stdlib.h>

#define JSONFNAME "/unit_tests.json"


void get_wifi_credential(const char *fname, char *ssid_ptr, size_t len_ssid, char *pwd_ptr, size_t len_pwd);

void get_broker_params(const char *fname,
                       char *public_mqtt_broker,
                       char *local_mqtt_broker,
                       size_t max_len_broker,
                       char *topic,
                       size_t max_len_topic,
                       int *mqtt_port_pntr);

void setup_wifi(void);



#endif

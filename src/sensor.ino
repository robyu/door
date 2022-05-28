#include "Arduino.h"
#include <ESP8266WiFi.h>
#include "gpio_pins.h"
#include "doormon.h"
#include "wifimon.h"
#include "enum_descr.h"
#include "rebooter.h"
#include "utils.h"
#include "mqttif.h"

#define LOOP_DELAY_MS (1000*1)
#define FAIL_COUNT_THRESH 1000
#define DOOR_OPEN_THRESH_MS (1000*60)

typedef enum
{
    SENSOR_DONT_USE = 0,
    SENSOR_INIT,
    SENSOR_SETUP_WIFI,
    SENSOR_WIFI_OFFLINE,
    SENSOR_OFFLINE,
    SENSOR_ONLINE,
    SENSOR_REBOOT,
    SENSOR_LAST_DONT_USE
} sensor_state_t;

typedef struct
{
    doormon_t doormon;
    wifimon_t wifimon;
    rebooter_t rebooter;
    sensor_state_t sensor_state;
    mqttif_t mqttif;

    int mqtt_reboot_request;
    int wifi_fail_counter;
    float door_open_duration_ms;

    int reset_btn_longpress;
    wifimon_state_t wifimon_state;
    doormon_state_t doormon_state;
    doormon_state_t prev_doormon_state;
    
} sensor_t;
sensor_t sensor;

typedef enum
{
    TX_MSG_FIRST_DONT_USE = 0,
    TX_MSG_DOOR_IS_OPEN,
    TX_MSG_DOOR_HAS_CLOSED,
    TX_MSG_REBOOT,
    TX_MSG_LAST_DONT_USE
} msg_type_t;


static const char* state_to_string(sensor_state_t state)
{
    enum_descr_t sensor_state_descr[] = {
        ENUM_DESCR_DECLARE(SENSOR_DONT_USE),
        ENUM_DESCR_DECLARE(SENSOR_INIT),
        ENUM_DESCR_DECLARE(SENSOR_SETUP_WIFI),
        ENUM_DESCR_DECLARE(SENSOR_WIFI_OFFLINE),
        ENUM_DESCR_DECLARE(SENSOR_OFFLINE),
        ENUM_DESCR_DECLARE(SENSOR_ONLINE),
        ENUM_DESCR_DECLARE(SENSOR_REBOOT),
        ENUM_DESCR_DECLARE(SENSOR_LAST_DONT_USE),
        ENUM_DESCR_END
    };
    return sensor_state_descr[(int)state].pvalue_str;
}

void setup()
{
    Serial.begin(115200);
    delay(10); // need this?


    doormon_init(&sensor.doormon, REED_SWITCH0, BTN_DOOR_TEST, LED_DOOR);
    wifimon_init(&sensor.wifimon, LED_WIFI, BTN_RESET);
    rebooter_init(&sensor.rebooter, BTN_RESET, LED_RESET_BUTTON);

    sensor.sensor_state = SENSOR_INIT;

    Serial.println("setup complete");
    Serial.flush();
}

void init_mqttif(sensor_t *psensor)
{
    char pmqtt_server[WIFIMON_MAX_LEN_MQTT_SERVER];
    short mqtt_port;
    mqttif_config_t config;

    wifimon_read_mqtt_params_from_file(pmqtt_server,
                                       &mqtt_port);
    UTILS_ASSERT((mqtt_port >= 0) && (mqtt_port <= 1024));
    mqttif_set_default_config(&config, pmqtt_server, mqtt_port);
    mqttif_init(&psensor->mqttif, &config);
}

void transmit_msg(sensor_t *psensor, msg_type_t msg)
{
    ;
}

void handle_mqtt_rx(sensor_t *psensor)
{
    ;
}


static void do_steadystate(sensor_t *psensor)
{
    switch(psensor->sensor_state)
    {
    case SENSOR_INIT:
        break;

    case SENSOR_SETUP_WIFI:
        psensor->wifimon_state = wifimon_update(&psensor->wifimon);
        break;

    case SENSOR_WIFI_OFFLINE:
        psensor->wifimon_state = wifimon_update(&psensor->wifimon);
        psensor->reset_btn_longpress = reboot_update_state(&psensor->rebooter);
        psensor->wifi_fail_counter++;
        break;
        

    case SENSOR_OFFLINE:
        psensor->wifimon_state = wifimon_update(&psensor->wifimon);
        psensor->reset_btn_longpress = reboot_update_state(&psensor->rebooter);
        mqttif_update(&psensor->mqttif);
        break;
         
    case SENSOR_ONLINE:
        psensor->wifimon_state = wifimon_update(&psensor->wifimon);
        psensor->reset_btn_longpress = reboot_update_state(&psensor->rebooter);
        mqttif_update(&psensor->mqttif);
        handle_mqtt_rx(psensor);

        psensor->prev_doormon_state = psensor->doormon_state;
        psensor->doormon_state = doormon_update(&psensor->doormon);
        break;

    case SENSOR_REBOOT:
        if (mqttif_is_connected(&psensor->mqttif))
        {
            transmit_msg(psensor, TX_MSG_REBOOT);
            delay(500);
        }
        utils_restart();
        break;

    default:
        Serial.printf("unexpected state: %s\n",state_to_string(psensor->sensor_state));
        UTILS_ASSERT(0);
    }
}

sensor_state_t do_transitions(sensor_t *psensor)
{
    sensor_state_t next_state = psensor->sensor_state;
    switch(psensor->sensor_state)
    {
    case SENSOR_INIT:
        break;

    case SENSOR_SETUP_WIFI:
        if ((psensor->wifimon_state==WM_NOT_CONNECTED) || (psensor->wifimon_state==WM_CONNECTED))
        {
            next_state = SENSOR_WIFI_OFFLINE;
        }
        break;

    case SENSOR_WIFI_OFFLINE:
        if (psensor->reset_btn_longpress) 
        {
            Serial.printf("detected reset button long press\n");
            next_state = SENSOR_REBOOT;
        }
        else if (psensor->wifimon_state==WM_CONNECTED)
        {
            next_state = SENSOR_OFFLINE;
        }
        else if ((psensor->wifimon_state==WM_NOT_CONNECTED) && psensor->wifi_fail_counter > FAIL_COUNT_THRESH)
        {
            Serial.printf("SENSOR_WIFI_OFFLINE: exceeded fail_counter threshold; rebooting\n");
            next_state = SENSOR_REBOOT;
        }
        break;
        

    case SENSOR_OFFLINE:
        if (psensor->reset_btn_longpress)  
        {
            Serial.printf("detected reset button long press\n");
            next_state = SENSOR_REBOOT;
        }
        else if (psensor->wifimon_state==WM_NOT_CONNECTED)
        {
            next_state = SENSOR_WIFI_OFFLINE;
        }
        else if (mqttif_is_connected(&psensor->mqttif))
        {
            next_state = SENSOR_ONLINE;
        }
        break;
         
    case SENSOR_ONLINE:
        if (false==mqttif_is_connected(&psensor->mqttif))
        {
            next_state = SENSOR_OFFLINE;
        }
        else if ((psensor->reset_btn_longpress) || (psensor->mqtt_reboot_request))
        {
            next_state = SENSOR_REBOOT;
        }
        
        if (psensor->doormon_state==DM_DOOR_OPEN)
        {
            if (psensor->prev_doormon_state != psensor->doormon_state)
            {
                // door is newly opened
                transmit_msg(psensor, TX_MSG_DOOR_IS_OPEN);
            }
            else if (doormon_get_elapsed_state_time_ms(&psensor->doormon) > DOOR_OPEN_THRESH_MS)
            {
                // door has been open for a while
                transmit_msg(psensor, TX_MSG_DOOR_IS_OPEN);
            }
        }
        else if (psensor->doormon_state==DM_DOOR_CLOSED)
        {
            if (psensor->prev_doormon_state != psensor->doormon_state)
            {
                // door has just closed
                transmit_msg(psensor, TX_MSG_DOOR_HAS_CLOSED);
            }
        }
        break;

    case SENSOR_REBOOT:
        utils_restart();
        break;

    default:
        Serial.printf("unexpected state: %s\n",state_to_string(psensor->sensor_state));
        UTILS_ASSERT(0);
    }
    return next_state;
}

// the loop function runs over and over again forever
void loop() {
    sensor_state_t next_state = SENSOR_DONT_USE;
    
    Serial.printf("LOOP TOP\n");
    Serial.printf("entering state = (%s)\n",state_to_string(sensor.sensor_state));
    do_steadystate(&sensor);
    next_state = do_transitions(&sensor);

    if (next_state != sensor.sensor_state)
    {
        Serial.printf("state transition: (%s) -> (%s)\n",
                      state_to_string(sensor.sensor_state),
                      state_to_string(next_state));

        sensor.sensor_state = next_state;
    }
    delay(LOOP_DELAY_MS); 
}


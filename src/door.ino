#include "Arduino.h"
#include <ESP8266WiFi.h>
#include "gpio_pins.h"
#include "doormon.h"
#include "wifimon.h"
#include "enum_descr.h"
#include "rebooter.h"
#include "utils.h"
#include "mqttif.h"

#define LOOP_DELAY_MS (1000*5)
#define FAIL_COUNT_THRESH 1000
#define DOOR_OPEN_THRESH_MS (1000*60)

typedef enum
{
    DOOR_DONT_USE = 0,
    DOOR_INIT,
    DOOR_SETUP_WIFI,
    DOOR_WIFI_OFFLINE,
    DOOR_SENSOR_OFFLINE,
    DOOR_SENSOR_ONLINE,
    DOOR_REBOOT,
    DOOR_LAST_DONT_USE
} door_state_t;

typedef struct
{
    doormon_t doormon;
    wifimon_t wifimon;
    rebooter_t rebooter;
    door_state_t door_state;
    mqttif_t mqttif;
    int door_is_open;

    int mqtt_reboot_request;
    int wifi_fail_counter;
    float door_open_duration_ms;
} door_t;

door_t door;

typedef enum
{
    TX_MSG_FIRST_DONT_USE = 0,
    MSG_DOOR_IS_OPEN,
    MSG_DOOR_HAS_CLOSED,
    TX_MSG_LAST_DONT_USE
} msg_type_t;


static const char* state_to_string(door_state_t state)
{
    enum_descr_t door_state_descr[] = {
        ENUM_DESCR_DECLARE(DOOR_DONT_USE),
        ENUM_DESCR_DECLARE(DOOR_INIT),
        ENUM_DESCR_DECLARE(DOOR_SETUP_WIFI),
        ENUM_DESCR_DECLARE(DOOR_WIFI_OFFLINE),
        ENUM_DESCR_DECLARE(DOOR_SENSOR_OFFLINE),
        ENUM_DESCR_DECLARE(DOOR_SENSOR_ONLINE),
        ENUM_DESCR_DECLARE(DOOR_REBOOT),
        ENUM_DESCR_DECLARE(DOOR_LAST_DONT_USE),
        ENUM_DESCR_END
    };
    return door_state_descr[(int)state].pvalue_str;
}

void setup()
{
    Serial.begin(115200);
    delay(10); // need this?


    doormon_init(&door.doormon, REED_SWITCH0, BTN_DOOR_TEST, LED_DOOR);
    wifimon_init(&door.wifimon, LED_WIFI, BTN_RESET);
    rebooter_init(&door.rebooter, BTN_RESET, LED_RESET_BUTTON);

    door.door_state = DOOR_INIT;

    Serial.println("setup complete");
    Serial.flush();
}

void init_mqttif(door_t *pdoor)
{
    char pmqtt_server[WIFIMON_MAX_LEN_MQTT_SERVER];
    char pmqtt_port[WIFIMON_MAX_LEN_MQTT_PORT];
    int mqtt_port;
    mqttif_config_t config;

    wifimon_read_mqtt_params_from_file(pmqtt_server,
                                       WIFIMON_MAX_LEN_MQTT_SERVER,
                                       pmqtt_port,
                                       WIFIMON_MAX_LEN_MQTT_PORT);
    mqtt_port = atoi(pmqtt_port);
    UTILS_ASSERT((mqtt_port >= 0) && (mqtt_port <= 1024));
    mqttif_set_default_config(&config, pmqtt_server, mqtt_port);
    mqttif_init(&pdoor->mqttif, &config);
}

void transmit_registration_msgs(door_t *pdoor)
{
    ;
}

void transmit_reboot_msg(door_t *pdoor)
{
    ;
}

void handle_mqtt_rx(door_t *pdoor)
{
    ;
}


void transmit_door_msg(door_t *pdoor, msg_type_t door_msg)
{
    switch(door_msg)
    {
    case MSG_DOOR_IS_OPEN:
        break;

    case MSG_DOOR_HAS_CLOSED:
        break;

    default:
        UTILS_ASSERT(0);
    }
}

// the loop function runs over and over again forever
void loop() {
    int reset_btn_longpress = -1;
    door_state_t next_state = DOOR_DONT_USE;
    wifimon_state_t wifimon_state = WM_DONT_USE;
    doormon_state_t doormon_state;
    
    Serial.printf("loop() top\n");
    Serial.printf("    entering state = (%s)\n",state_to_string(door.door_state));
    delay(LOOP_DELAY_MS); 

    // steady state & transition logic
    switch(door.door_state)
    {
    case DOOR_INIT:
        next_state = DOOR_SETUP_WIFI;
        break;

    case DOOR_SETUP_WIFI:
        wifimon_state = wifimon_update(&door.wifimon);
        if (wifimon_state==WM_NOT_CONNECTED)
        {
            next_state = DOOR_WIFI_OFFLINE;
        }
        break;

    case DOOR_WIFI_OFFLINE:
        wifimon_state = wifimon_update(&door.wifimon);
        reset_btn_longpress = reboot_update_state(&door.rebooter);
        door.wifi_fail_counter++;

        if (reset_btn_longpress) 
        {
            Serial.printf("detected reset button long press\n");
            next_state = DOOR_REBOOT;
        }
        else if (wifimon_state==WM_CONNECTED)
        {
            next_state = DOOR_SENSOR_OFFLINE;
        }
        else if (door.wifi_fail_counter > FAIL_COUNT_THRESH)
        {
            UTILS_ASSERT(wifimon_state==WM_NOT_CONNECTED);
            Serial.printf("DOOR_WIFI_OFFLINE: exceeded fail_counter threshold; rebooting\n");
            next_state = DOOR_REBOOT;
        }
        else
        {
            UTILS_ASSERT(wifimon_state==WM_NOT_CONNECTED);
            Serial.printf("DOOR_WIFI_OFFLINE: fail_counter=%d\n",door.wifi_fail_counter);
        }
        break;
        

    case DOOR_SENSOR_OFFLINE:
        wifimon_state = wifimon_update(&door.wifimon);
        reset_btn_longpress = reboot_update_state(&door.rebooter);
        mqttif_update(&door.mqttif);
        
        if (reset_btn_longpress)  
        {
            Serial.printf("detected reset button long press\n");
            next_state = DOOR_REBOOT;
        }
        else if (wifimon_state==WM_NOT_CONNECTED)
        {
            next_state = DOOR_WIFI_OFFLINE;
        }
        else if (mqttif_is_connected(&door.mqttif))
        {
            next_state = DOOR_SENSOR_ONLINE;
        }
        break;
         
    case DOOR_SENSOR_ONLINE:
        wifimon_state = wifimon_update(&door.wifimon);
        reset_btn_longpress = reboot_update_state(&door.rebooter);
        mqttif_update(&door.mqttif);
        handle_mqtt_rx(&door);
        doormon_state = doormon_update(&door.doormon);

        if (false==mqttif_is_connected(&door.mqttif))
        {
            next_state = DOOR_SENSOR_OFFLINE;
        }
        else if ((reset_btn_longpress) || (door.mqtt_reboot_request))
        {
            next_state = DOOR_REBOOT;
        }
        
        if (doormon_state==DM_DOOR_OPEN)
        {
            if (door.door_is_open && (doormon_get_elapsed_state_time_ms(&door.doormon)) > DOOR_OPEN_THRESH_MS)
            {
                // door has been open for a while
                transmit_door_msg(&door, MSG_DOOR_IS_OPEN);
            }
            else if (door.door_is_open==0)
            {
                // door is newly open
                transmit_door_msg(&door, MSG_DOOR_IS_OPEN);
                door.door_is_open = 1;
            }
        }
        else if ((doormon_state==DM_DOOR_CLOSED) && door.door_is_open)
        {
            // door has closed
            door.door_is_open = 0;
            transmit_door_msg(&door, MSG_DOOR_HAS_CLOSED);
        }
        break;

    case DOOR_REBOOT:
        utils_restart();
        break;

    default:
        Serial.printf("unexpected state: %s\n",state_to_string(door.door_state));
        UTILS_ASSERT(0);
    }
    UTILS_ASSERT(next_state != DOOR_DONT_USE);

    if (next_state != door.door_state)
    {
        Serial.printf("state transition: (%s) -> (%s)\n",
                      state_to_string(door.door_state),
                      state_to_string(next_state));

        switch(next_state)
        {
        case DOOR_INIT:
            // should never transition into this state
            UTILS_ASSERT(0);  
            break;

        case DOOR_SETUP_WIFI:
            UTILS_ASSERT(door.door_state==DOOR_INIT);
            break;
            
        case DOOR_WIFI_OFFLINE:
            UTILS_ASSERT((door.door_state==DOOR_SETUP_WIFI) ||
                         (door.door_state==DOOR_SENSOR_OFFLINE));
            door.wifi_fail_counter = 0;
            break;

        case DOOR_SENSOR_OFFLINE:
            UTILS_ASSERT((door.door_state==DOOR_SENSOR_ONLINE) ||
                         (door.door_state==DOOR_WIFI_OFFLINE));
            if (door.door_state==DOOR_WIFI_OFFLINE)
            {
                init_mqttif(&door);
            }
            break;

        case DOOR_SENSOR_ONLINE:
            UTILS_ASSERT(door.door_state==DOOR_SENSOR_OFFLINE);
            transmit_registration_msgs(&door);
            break;

        case DOOR_REBOOT:
            UTILS_ASSERT((door.door_state==DOOR_WIFI_OFFLINE) ||
                         (door.door_state==DOOR_SENSOR_OFFLINE) ||
                         (door.door_state==DOOR_SENSOR_ONLINE));
            if (mqttif_is_connected(&door.mqttif))
            {
                transmit_reboot_msg(&door);
            }
            break;

        default:
            UTILS_ASSERT(0);
        }
        door.door_state = next_state;
    }
}


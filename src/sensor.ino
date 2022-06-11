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
#define THRESH_DOOR_OPEN_MS (1000*60)

// wifi typically it takes 8-10 sec to establish connection after calling WiFi.begin
#define THRESH_WAIT_FOR_WIFI_CONNECT_MS (1000 * 30)

// mqtt typically connect immediately
#define THRESH_WAIT_FOR_MQTT_CONNECT_MS (1000 * 5)

typedef enum
{
    SENSOR_DONT_USE = 0,
    SENSOR_INIT,
    SENSOR_SETUP_WIFI,
    SENSOR_WAIT_WIFI_CONNECT,
    SENSOR_WAIT_MQTT_CONNECT,
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
    long state_start_time;
    long state_elapsed_ms;
    unsigned long door_open_interval_count;
    long door_open_time;

    int reset_btn_longpress;
    wifimon_state_t wifimon_state;
    doormon_state_t doormon_state;
    doormon_state_t prev_doormon_state;
    
} sensor_t;
sensor_t sensor;

typedef enum
{
    TX_MSG_FIRST_DONT_USE = 0,
    TX_MSG_REGISTRATION,
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
        ENUM_DESCR_DECLARE(SENSOR_WAIT_WIFI_CONNECT),
        ENUM_DESCR_DECLARE(SENSOR_WAIT_MQTT_CONNECT),
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
    int mqtt_port;
    mqttif_config_t config;

    Serial.printf("  init_mqtt: initialize MQTT\n");
    wifimon_read_mqtt_params_from_file(pmqtt_server,
                                       &mqtt_port);
    Serial.printf("  mqtt_server: (%s), port (%d)\n", pmqtt_server, mqtt_port);
    UTILS_ASSERT((mqtt_port >= 0) && (mqtt_port <= 65535));
    mqttif_set_default_config(&config, pmqtt_server, mqtt_port);
    mqttif_init(&psensor->mqttif, &config);
    Serial.printf("  init_mqtt: exit\n");
}

void subscribe_mqtt(sensor_t *psensor)
{
}

void publish_mqtt(sensor_t *psensor, msg_type_t msg)
{
    mqttif_t *pmqttif = &psensor->mqttif;
    char pmsg[128];

    switch(msg)
    {
    case TX_MSG_REGISTRATION:
        mqttif_publish(pmqttif, "home/garage-sensor/state", "ready");

        snprintf(pmsg, 128, "%s", WiFi.localIP().toString().c_str());
        mqttif_publish(pmqttif, "home/garage-sensor/ip_addr", pmsg);
        break;
    case TX_MSG_DOOR_IS_OPEN:
    {
        float elapsed_min = (psensor->door_open_interval_count * THRESH_DOOR_OPEN_MS) / (1000.0 * 60);
        mqttif_publish(pmqttif, "home/garage-sensor/door-detector/door-status", "open");

        snprintf(pmsg, 128, "%8.2f min", elapsed_min);
        mqttif_publish(pmqttif, "home/garage-sensor/door-detector/open-duration", pmsg);

        snprintf(pmsg, 128, "door open for (%8.2f) min", elapsed_min);
        mqttif_publish(pmqttif, "home/broadcast/door", pmsg);
        break;
    }
    case TX_MSG_DOOR_HAS_CLOSED:
        mqttif_publish(pmqttif, "home/garage-sensor/door-detector/door-status", "closed");
        break;
        
    case TX_MSG_REBOOT:
        mqttif_publish(pmqttif, "home/broadcast/reset", "resetting now");
        break;
            
    default:
        UTILS_ASSERT(0);
    }
}

void handle_mqtt_rx(sensor_t *psensor)
{
    ;
}

static void do_steadystate(sensor_t *psensor)
{
    Serial.printf("do_steadystate: steady state = (%s)\n",state_to_string(sensor.sensor_state));
    switch(psensor->sensor_state)
    {
    case SENSOR_INIT:
        break;

    case SENSOR_SETUP_WIFI:
        psensor->wifimon_state = wifimon_update(&psensor->wifimon);
        break;

    case SENSOR_WAIT_WIFI_CONNECT:
        psensor->wifimon_state = wifimon_update(&psensor->wifimon);
        psensor->reset_btn_longpress = reboot_update_state(&psensor->rebooter);
        psensor->state_elapsed_ms = utils_get_elapsed_msec_and_reset(&psensor->state_start_time);
        break;
        

    case SENSOR_WAIT_MQTT_CONNECT:
        psensor->wifimon_state = wifimon_update(&psensor->wifimon);
        psensor->reset_btn_longpress = reboot_update_state(&psensor->rebooter);
        psensor->state_elapsed_ms = utils_get_elapsed_msec_and_reset(&psensor->state_start_time);
        mqttif_update(&psensor->mqttif);
        break;
         
    case SENSOR_ONLINE:
        psensor->wifimon_state = wifimon_update(&psensor->wifimon);
        psensor->reset_btn_longpress = reboot_update_state(&psensor->rebooter);
        psensor->state_elapsed_ms = utils_get_elapsed_msec_and_reset(&psensor->state_start_time);
        mqttif_update(&psensor->mqttif);

        handle_mqtt_rx(psensor);

        psensor->prev_doormon_state = psensor->doormon_state;
        psensor->doormon_state = doormon_update(&psensor->doormon);
        break;

    case SENSOR_REBOOT:
        if (mqttif_is_connected(&psensor->mqttif))
        {
            publish_mqtt(psensor, TX_MSG_REBOOT);
            delay(500);
        }
        utils_restart();
        break;

    default:
        Serial.printf("unexpected state: %s\n",state_to_string(psensor->sensor_state));
        UTILS_ASSERT(0);
    }
    Serial.printf("do_steadystate: exit\n");
}

sensor_state_t do_transitions(sensor_t *psensor)
{
    sensor_state_t next_state = psensor->sensor_state;
    Serial.printf("do_transitions\n");
    switch(psensor->sensor_state)
    {
    case SENSOR_INIT:
        next_state = SENSOR_SETUP_WIFI;
        break;

    case SENSOR_SETUP_WIFI:
        if ((psensor->wifimon_state==WM_NOT_CONNECTED) || (psensor->wifimon_state==WM_CONNECTED))
        {
            psensor->state_start_time = millis(); 
            next_state = SENSOR_WAIT_WIFI_CONNECT;
        }
        break;

    case SENSOR_WAIT_WIFI_CONNECT:
        if (psensor->reset_btn_longpress) 
        {
            next_state = SENSOR_REBOOT;
        }
        else if (psensor->wifimon_state==WM_CONNECTED)
        {
            //
            // wifi is up, so init mqtt interface
            init_mqttif(psensor);

            Serial.printf("== MQTTIF: RUNNING UPDATE RIGHT NOW\n");
            mqttif_update(&psensor->mqttif);
            Serial.printf("== MQTTIF: DONE\n");
            
            
            psensor->state_start_time = millis(); 
            next_state = SENSOR_WAIT_MQTT_CONNECT;

        }
        else //(psensor->wifimon_state==WM_NOT_CONNECTED)
        {
            UTILS_ASSERT(psensor->wifimon_state == WM_NOT_CONNECTED);
            
            Serial.printf("SENSOR_WAIT_WIFI_CONNECT: waiting for wifi connect for (%f) sec\n", psensor->state_elapsed_ms/1000.0);
            if (psensor->state_elapsed_ms > THRESH_WAIT_FOR_WIFI_CONNECT_MS)
            {
                Serial.printf("SENSOR_WAIT_WIFI_CONNECT: exceeded wait time threshold; rebooting\n");
                next_state = SENSOR_REBOOT;
            }
        }
        break;

    case SENSOR_WAIT_MQTT_CONNECT:
    {
        bool mqttif_connected = mqttif_is_connected(&psensor->mqttif);
        
        if (psensor->reset_btn_longpress)  
        {
            next_state = SENSOR_REBOOT;
        }
        else if (psensor->wifimon_state==WM_NOT_CONNECTED)
        {
            Serial.printf("lost wifi connection\n");
            psensor->state_start_time = millis(); 
            next_state = SENSOR_WAIT_WIFI_CONNECT;
        }
        else if (mqttif_connected)
        {
            Serial.printf("MQTT server connected\n");
            publish_mqtt(psensor, TX_MSG_REGISTRATION);
            subscribe_mqtt(psensor);
            next_state = SENSOR_ONLINE;
        }
        else // if ( !mqttif_connected)
        {
            UTILS_ASSERT( !mqttif_connected );
            Serial.printf("SENSOR_WAIT_MQTT_CONNECT: waiting for mqtt connect for (%f) sec\n", psensor->state_elapsed_ms/1000.0);
            if (psensor->state_elapsed_ms > THRESH_WAIT_FOR_MQTT_CONNECT_MS)
            {
                Serial.printf("SENSOR_WAIT_MQTT_CONNECT: exceeded wait time threshold; rebooting\n");
                next_state = SENSOR_REBOOT;
            }
        }
        break;
    }
         
    case SENSOR_ONLINE:
        if (false==mqttif_is_connected(&psensor->mqttif))
        {
            next_state = SENSOR_WAIT_MQTT_CONNECT;
        }
        else if (psensor->reset_btn_longpress)
        {
            next_state = SENSOR_REBOOT;
        }
        else if (psensor->mqtt_reboot_request)
        {
            Serial.printf("received MQTT reboot request\n");
            next_state = SENSOR_REBOOT;
        }

        Serial.printf("SENSOR_ONLINE =======================\n");
        Serial.printf("  doormon prev state = (%s)\n", doormon_state_to_string(psensor->prev_doormon_state).c_str());
        Serial.printf("  doormon state = (%s)\n", doormon_state_to_string(psensor->doormon_state).c_str());
        Serial.printf("=======================\n");
        if ((psensor->prev_doormon_state == DM_INIT) ||
            (psensor->prev_doormon_state == DM_DONT_USE))
        {
            // we're just starting up, so ignore this
        }
        else if (psensor->doormon_state==DM_DOOR_OPEN)
        {
            if (psensor->prev_doormon_state == DM_DOOR_OPENING) // respond only to this transition
            {
                psensor->door_open_interval_count = 0;
                psensor->door_open_time = millis();
                
                // door is newly opened
                publish_mqtt(psensor, TX_MSG_DOOR_IS_OPEN);
            }
            else if ( (millis() - psensor->door_open_time) > THRESH_DOOR_OPEN_MS)
            {
                // door has been open for a while
                psensor->door_open_interval_count++;
                psensor->door_open_time = millis();  // reset door_open_time
                publish_mqtt(psensor, TX_MSG_DOOR_IS_OPEN);
            }
        }
        else if (psensor->doormon_state==DM_DOOR_CLOSED) // respond only to this transition
        {
            if (psensor->prev_doormon_state == DM_DOOR_CLOSING)
            {
                // door has just closed
                publish_mqtt(psensor, TX_MSG_DOOR_HAS_CLOSED);
            }
        }
        break;

    default:
        Serial.printf("unexpected state: %s\n",state_to_string(psensor->sensor_state));
        UTILS_ASSERT(0);
    }
    Serial.printf("do_transitions: exit\n");
    return next_state;
}

// the loop function runs over and over again forever
void loop() {
    static int num_iter=0;
    sensor_state_t next_state = SENSOR_DONT_USE;
    
    Serial.printf("\nLOOP TOP (%d)\n", num_iter);
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

    if (num_iter > 9999)
    {
        utils_restart();
    }
    num_iter++;
}


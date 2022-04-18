#include "Arduino.h"
#include "mqttif.h"
#include "utils.h"

void mqttif_set_default_config(mqttif_config_t *pconfig, const char *pbroker_addr, int mqtt_port)
{
    UTILS_ZERO_STRUCT(pconfig);

    UTILS_ASSERT(strlen(pbroker_addr) < MQTTIF_MAX_LEN_STR);
    strncpy(pconfig->pbroker_addr, pbroker_addr, MQTTIF_MAX_LEN_STR);

    pconfig->mqtt_port = mqtt_port;
    pconfig->max_num_connect_attempts = 10;
}

// AGH, rx_callback needs a global
static mqttif_t *pmqttif_global = NULL;

/*
copy rx topic + payload into prx_msgs array
*/
void rx_callback(char *ptopic, byte *ppayload, unsigned int length)
{
    mqttif_t *p;
    UTILS_ASSERT(pmqttif_global!=NULL);
    p = pmqttif_global;
    int rx_msg_index = p->num_rx_msgs - 1;

    if (p->num_rx_msgs > MQTTIF_NUM_RX)
    {
        Serial.printf("MQTT RX BUFFER overflow\n");
    }
    else
    {
        int valid_len;
        mqttif_rx_msg_t *pcurr_rx_msg;

        UTILS_ASSERT((rx_msg_index >= 0) && (rx_msg_index < MQTTIF_NUM_RX));

        pcurr_rx_msg = &(p->prx_msgs[rx_msg_index]);
        memset(pcurr_rx_msg->ptopic, 0, MQTTIF_MAX_LEN_STR);
        Serial.printf("rcvd topic: (%s)\n", ptopic);
        strncpy(pcurr_rx_msg->ptopic, ptopic, MQTTIF_MAX_LEN_STR - 1);
        
        memset(pcurr_rx_msg->ppayload, 0, MQTTIF_MAX_LEN_STR);
        valid_len = (length < MQTTIF_MAX_LEN_STR) ? length : (MQTTIF_MAX_LEN_STR - 1);
        strncpy(pcurr_rx_msg->ppayload, (char *)ppayload, valid_len);

        p->num_rx_msgs++;
    }
    return;
}

static bool connect_mqtt_broker(mqttif_t *p)
{
    int count = 0;
    PubSubClient *pmqtt_client = p->pmqtt_client;
    mqttif_config_t *pconfig = &p->config;
    
    //connecting to a mqtt broker
    // usually we need only 1 attempt
    while (!pmqtt_client->connected() && (count < pconfig->max_num_connect_attempts)) {
        String client_id = "esp8266-client-";
        count++;
        client_id += String(WiFi.macAddress());
        Serial.printf("Attemping connection: client %s -> mqtt broker %s\n", client_id.c_str(), pconfig->pbroker_addr);
        if (pmqtt_client->connect(client_id.c_str())) {
            Serial.printf("mqtt broker connected after %d attempts\n", count);
        } else {
            Serial.printf("failed with state (%s)\n", String(pmqtt_client->state()).c_str());
            Serial.println(pmqtt_client->state());
            delay(2000);
        }
    }
    return pmqtt_client->connected();
}


void mqttif_init(mqttif_t *p, const mqttif_config_t *pconfig)
{
    UTILS_ZERO_STRUCT(p);
    p->config = *pconfig;

    p->pmqtt_client = new PubSubClient(p->eth_client);
    pmqttif_global = p;
    p->pmqtt_client->setServer(pconfig->pbroker_addr, pconfig->mqtt_port);
    p->pmqtt_client->setCallback(rx_callback);
    
    // try to connect, but it's not a big deal if connection fails
    connect_mqtt_broker(p);
}

bool mqttif_is_connected(const mqttif_t *p)
{
    return p->pmqtt_client->connected();
}

/*
subscribe to a topic

returns false if either the connection or subscription operation failed
*/
bool mqttif_sub_topic(mqttif_t *p, const char *ptopic)
{
    bool is_connected;
    bool retval;
    PubSubClient *pmqtt_client = p->pmqtt_client;
    is_connected = connect_mqtt_broker(p);
    if (false==is_connected)
    {
        return false;
    }
    UTILS_ASSERT(is_connected==true);

    retval = pmqtt_client->subscribe(ptopic);
    Serial.printf("subscribe to (%s) returned: %d\n", ptopic, int(retval));
    return retval;
}

/*
publish topic + payload
synchronous operation (I think)
returns false if either the connection or publish operation failed
*/
bool mqttif_publish(mqttif_t *p, const char *ptopic, const char *ppayload)
{
    bool is_connected;
    PubSubClient *pmqtt_client = p->pmqtt_client;
    bool retval;

    is_connected = connect_mqtt_broker(p);
    if (false==is_connected)
    {
        return false;
    }
    UTILS_ASSERT(is_connected==true);

    retval = pmqtt_client->publish(ptopic, ppayload);

    // call loop immediately to push it out (?)
    pmqtt_client->loop();

    return retval;
}

void mqttif_update(mqttif_t *p)
{
    p->pmqtt_client->loop();
}


/*
max length of ptopic and ppayload is MQTTIF_MAX_LEN_STR

returns:
number of msgs in rx buffer
*/
int  mqttif_check_rx_msgs(mqttif_t *p, char *ptopic, char *ppayload)
{
    int num_msgs_available = p->num_rx_msgs;
    if (num_msgs_available <= 0)
    {
        ptopic = NULL;
        ppayload = NULL;
        return 0;
    }
    else
    {
        int n;
        // dequeue msg 0
        mqttif_rx_msg_t *prx_msg = &(p->prx_msgs[0]);
        strncpy(ptopic,   prx_msg->ptopic,    MQTTIF_MAX_LEN_STR);
        strncpy(ppayload, prx_msg->ppayload,  MQTTIF_MAX_LEN_STR);

        //
        // move the remaining msgs
        for (n=1;n<num_msgs_available-1;n++)
        {
            mqttif_rx_msg_t *psrc_msg  = &(p->prx_msgs[ n ]);
            mqttif_rx_msg_t *pdest_msg = &(p->prx_msgs[ n-1 ]);

            strncpy(pdest_msg->ptopic,   psrc_msg->ptopic,   MQTTIF_MAX_LEN_STR);
            strncpy(pdest_msg->ppayload, psrc_msg->ppayload, MQTTIF_MAX_LEN_STR);
        }

        p->num_rx_msgs--;
        UTILS_ASSERT(p->num_rx_msgs >= 0);
    }
    return num_msgs_available;
}





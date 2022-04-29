#include "Arduino.h"
#include "mqttif.h"
#include "utils.h"
#include <ESP8266WiFi.h>

void mqttif_set_default_config(mqttif_config_t *pconfig, const char *pbroker_addr, int mqtt_port)
{
    UTILS_ZERO_STRUCT(pconfig);

    UTILS_ASSERT(strlen(pbroker_addr) < MQTTIF_MAX_LEN_STR);
    strncpy(pconfig->pbroker_addr, pbroker_addr, MQTTIF_MAX_LEN_STR);

    pconfig->mqtt_port = mqtt_port;
    pconfig->max_num_connect_attempts = 10;
}

/*
PubSubClient seems to want the object to be global
Otherwise it crashes w/ stack trace when I call connect()
*/
static WiFiClient wifi_client;
static PubSubClient *pmqtt_client = new PubSubClient(wifi_client);

/*
copy rx topic + payload into prx_msgs array
*/
mqttif_t *pmqttif_global;
void rx_callback(char *ptopic, byte *ppayload, unsigned int length)
{
    mqttif_t *p;
    UTILS_ASSERT(pmqtt_client!=NULL);
    p = pmqttif_global;

    int rx_msg_index = p->num_rx_msgs;

    Serial.printf("rx_callback: rcvd topic (%s)\n", ptopic);
    if (p->num_rx_msgs > MQTTIF_NUM_RX)
    {
        Serial.printf("MQTT RX BUFFER overflow\n");
    }
    else
    {
        mqttif_rx_msg_t *pcurr_rx_msg;

        UTILS_ASSERT((rx_msg_index >= 0) && (rx_msg_index < MQTTIF_NUM_RX));
        pcurr_rx_msg = &(p->prx_msgs[rx_msg_index]);

        memset(pcurr_rx_msg->ptopic, 0, MQTTIF_MAX_LEN_STR);
        strncpy(pcurr_rx_msg->ptopic, ptopic, MQTTIF_MAX_LEN_STR - 1);
        
        memset(pcurr_rx_msg->ppayload, 0, MQTTIF_MAX_LEN_STR);
        strncpy(pcurr_rx_msg->ppayload, (char *)ppayload, MQTTIF_MAX_LEN_STR - 1);

        p->num_rx_msgs++;
    }
    return;
}

#if 0
bool connect_mqtt_broker(mqttif_t *p)
{
    int count = 0;
    PubSubClient *pmqtt_client = p->pmqtt_client;
    mqttif_config_t *pconfig = &p->config;
    
    Serial.printf("connect_mqtt_broker: WiFi.status() = %d\n",(int)(WiFi.status()==WL_CONNECTED));

    Serial.printf("WiFi.macAddress() = %s\n",String(WiFi.macAddress()).c_str());

    /*--------------------------------------------------------*/
    Serial.printf("pmqtt_client=%p\n",(void *)pmqtt_client);
    Serial.printf("pmqttif_global->pmqtt_client=%p\n",(void *)pmqttif_global->pmqtt_client);


    Serial.printf("0 connected() = %d\n", (int) (pmqttif_global->pmqtt_client->connected()) );
    Serial.printf("1 connected() = %d\n", (int) (pmqtt_client->connected()) );
    
    UTILS_ASSERT(0);
    
    //connecting to a mqtt broker
    // usually we need only 1 attempt
    while (!pmqtt_client->connected() && (count < pconfig->max_num_connect_attempts)) {
        Serial.printf("attempt %d\n",count);  
        count++;
        UTILS_ASSERT(0);
        //Serial.printf("Attemping connection: client %s -> mqtt broker %s\n", client_id.c_str(), pconfig->pbroker_addr);
        if (pmqtt_client->connect("esp8266"))
        {
            Serial.printf("mqtt broker connected after %d attempts\n", count);
        } else {
            Serial.printf("failed with state (%s)\n", String(pmqtt_client->state()).c_str());
            Serial.println(pmqtt_client->state());
            delay(2000);
        }
    }
    return pmqtt_client->connected();
}
#endif

bool connect_mqtt_broker(PubSubClient *pmqtt_client)
{
    int count = 0;
    String client_name = "door-";
    Serial.printf("connect_mqtt_broker: WiFi.status() = %d\n",(int)(WiFi.status()==WL_CONNECTED));
    //Serial.printf("WiFi.macAddress() = %s\n",String(WiFi.macAddress()).c_str());
    client_name = client_name + WiFi.macAddress();
    Serial.printf("client name = %s\n",client_name.c_str());

    /*--------------------------------------------------------*/
    while (!pmqtt_client->connected() && (count < 5))
    {
        Serial.printf("attempt %d\n",count);  
        count++;
        //Serial.printf("Attemping connection: client %s -> mqtt broker %s\n", client_id.c_str(), pconfig->pbroker_addr);
        if (pmqtt_client->connect(client_name.c_str()))
        {
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

    UTILS_ASSERT(MQTT_MAX_PACKET_SIZE >= MQTTIF_MAX_LEN_STR);
    p->config = *pconfig;
    p->pmqtt_client = pmqtt_client;
    pmqttif_global = p;

    Serial.printf("mqttif_init: broker addr=%s\n",pconfig->pbroker_addr);
    Serial.printf("mqttif_init: port=%d\n",pconfig->mqtt_port);
    pmqtt_client->setServer(pconfig->pbroker_addr, pconfig->mqtt_port);
    pmqtt_client->setCallback(rx_callback);

    connect_mqtt_broker(p->pmqtt_client);
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
    is_connected = connect_mqtt_broker(p->pmqtt_client);
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

    is_connected = connect_mqtt_broker(p->pmqtt_client);
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
int  mqttif_check_rx_msgs(mqttif_t *p, char *ptopic, char *ppayload, int len_str)
{
    int num_msgs_available = p->num_rx_msgs;
    UTILS_ASSERT(len_str >= MQTTIF_MAX_LEN_STR);
    if (num_msgs_available <= 0)
    {
        memset(ptopic, 0, len_str);
        memset(ppayload, 0, len_str);
        return 0;
    }
    else
    {
        int n;
        // dequeue msg 0
        mqttif_rx_msg_t *prx_msg = &(p->prx_msgs[0]);
        strncpy(ptopic,   prx_msg->ptopic,    len_str);
        strncpy(ppayload, prx_msg->ppayload,  len_str);

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

void mqttif_disconnect(mqttif_t *p)
{
    p->pmqtt_client->disconnect();
}

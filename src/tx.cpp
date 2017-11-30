#include "WiFiClient.h"
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include "tx.h"

/*
  sanity check:  try HTTP GET from google
*/
void tx_send_google(void)
{
    char phost[] = "www.google.com";
    WiFiClient client;
    boolean connected;


    Serial.println("tx: >>> tx_send_google");
    Serial.println("tx: >>>>>>>>>> trying to connect");
    do {
        connected = client.connect(phost, 80);
        if (connected)
        {
            break;
        }
        client.stop();
        delay(500);
        Serial.println("tx: >>> retry connect");
    } while (connected==false);
    
    Serial.println("tx: >>> client.connected() =" + String(client.connected()));
    // We now create a URI for the request
  
    Serial.println("tx: >>> Requesting URL: ");

  
    client.print("GET / HTTP/1.1\r\n\r\n");
    //client.print(url);
    //client.print(" HTTP/1.1");

    Serial.println("tx: >>> Sent request");
    Serial.flush();

    Serial.println(String(">>> client.connected() = ") + String(client.connected()));
    Serial.println(String(">>> millis() = ") + String(millis()));
    Serial.flush();

    unsigned long timeout = millis();
    int count = 0;
    while (client.available() == 0) {
        count++;
        if ((count%1000)==0)
        {
            Serial.print("."); //String(count));
            Serial.flush();
        }
        if (millis() - timeout > 30000) {
            Serial.println("tx: >>> Client Timeout !");
            Serial.flush();
            client.stop();
            return;
        }
    }

    // Read all the lines of the reply from server and print them to Serial
    Serial.println("tx: >>> read response");
    Serial.flush();
    while(client.available()){
        String line = client.readStringUntil('\r');
        Serial.print(line);
    }
  
    Serial.println();
    Serial.println("tx: >>> closing connection");
    client.stop();
}

#define DEBUG 1
/*
  send msg via IFTTT webevent
  use HTTP POST

  RETURNS:
  0 - success
  1 - failed
*/
int tx_send(String descr, String val)
{
    char phost[] = "maker.ifttt.com";
    WiFiClient client;

    /*
      try to connect to ifttt
    */
    Serial.println("tx: transmitting message " + descr + ":" + val);
    Serial.println("tx: >>>>>>>>>> trying to connect");
    {
        int num_attempts = 0;
        int connected;
        do {
            connected = client.connect(phost, 80);
            if (connected)
            {
                break;
            }
            client.stop();
            delay(500);
            Serial.println("tx: >>> retry connect, attempt #" + String(num_attempts));
            num_attempts++;
        } while ((connected==false) && (num_attempts < 10));

        // if we got this far without connecting, then abort
        if (connected==false)
        {
            return 1;
        }
    }
    Serial.println("tx: >>> client.connected() =" + String(client.connected()));

    /*
      compose and send HTTP POST
    */
    {
        boolean connected;
        char pheader[255];
        String content = "{\"value1\" : \"" + descr + "\", \"value2\" : \"" + val + "\"}";

        /*
          compose message w/o newline,
          but use println to append newline
        */
        sprintf(pheader, "POST %s HTTP/1.1", WEBHOOK_URL);
        Serial.println(pheader);
        client.println(pheader);

        sprintf(pheader,
                "HOST: maker.ifttt.com");
        Serial.println(pheader);
        client.println(pheader);

        sprintf(pheader,
                "Content-Type: application/json");
        Serial.println(pheader);
        client.println(pheader);

        sprintf(pheader,"Content-Length: %d", content.length());
        Serial.println(pheader);
        client.println(pheader);

        client.println("");  // is this necessary?

        /*
          maker.ifttt.com expects json with format:
          {
              "value1": "foo",
              "value2": "bar"
          }
         */
        Serial.println(content);
        client.println(content);

        client.println("");   // finish GET by sending a final newline

        Serial.println("tx: >>> Sent request");
        Serial.flush();
    }

    unsigned long timeout = millis();
    int count = 0;
    while (client.available() == 0) {
        count++;
        if ((count%1000)==0)
        {
            Serial.print(".");
            Serial.flush();
        }
        if (millis() - timeout > 10000) {
            Serial.println("tx: >>> Client Timeout !");
            Serial.flush();
            client.stop();
            return 1;
        }
    }

    // Read all the lines of the reply from server and print them to Serial
    Serial.println("tx: >>> read response");
    Serial.flush();
    while(client.available()){
        String line = client.readStringUntil('\r');
        Serial.print(line);
    }
  
    Serial.println();
    Serial.println("tx: >>> closing connection");
    client.stop();

    return 0;
}

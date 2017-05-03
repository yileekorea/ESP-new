/*
 * -------------------------------------------------------------------
 * EmonESP Serial to Emoncms gateway
 * -------------------------------------------------------------------
 * Adaptation of Chris Howells OpenEVSE ESP Wifi
 * by Trystan Lea, Glyn Hudson, OpenEnergyMonitor
 * All adaptation GNU General Public License as below.
 *
 * -------------------------------------------------------------------
 *
 * This file is part of OpenEnergyMonitor.org project.
 * EmonESP is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * EmonESP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with EmonESP; see the file COPYING.  If not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "emonesp.h"
#include "mqtt.h"
#include "config.h"
#include "input.h"
#include "ota.h"

#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include <Arduino.h>
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <PubSubClient.h>             // MQTT https://github.com/knolleary/pubsubclient PlatformIO lib: 89
//#include <WiFiClient.h>
#include <WiFiClientSecure.h>

const char* fingerprint = "E6:E0:09:FD:2F:2F:31:85:54:F2:EA:22:14:42:D6:1A:9C:44:36:15";  //mac book
long lastMqttReconnectAttempt = 0;
int clientTimeout = 0;
int i = 0;
int MQTT_PORT = 8883;

const char* do_fw_update = "/do_fw_update";
const char* do_reboot = "/do_reboot";
const char* do_erase_all = "/do_erase_all";
const char* do_format = "/do_format";

String subCommand;
String topic_sub = "/S_";
String topic_pub = "/P_";
String MAC;


//WiFiClient espClient;                 // Create client for MQTT
WiFiClientSecure espClient;                 // Create client for MQTT
//PubSubClient mqttclient(espClient);   // Create client for MQTT
PubSubClient mqttclient(mqtt_server.c_str(), MQTT_PORT, mqttCallback, espClient);


// -------------------------------------------------------------------
// MQTT sendTempData
// -------------------------------------------------------------------
void sendTempData() {
    byte i;
    for ( i = 0; i < numSensor ; i++) {
		if(old_celsius[i] != celsius[i]){

			char pChrBuffer[5];
			String payload = "{\"tbl_name\":";
			payload += "\"";
			payload += MAC;
			payload += "\"";

			payload += ",\"id\":";
			payload += i+1;   // id
			
			payload += ",\"numSensor\":";
			payload += numSensor;   // numSensor
			
			payload += ",\"cTemps\":";
			if ( isnan(celsius[i]) )
				payload += "0";
			else {
				dtostrf(celsius[i] , 3, 1, pChrBuffer);
				payload += pChrBuffer;   // *C
			}
			
			payload += ",\"sName\":";
			payload += "\"";
			payload += sName[i];   // sensor name
			payload += "\"";

			payload += ",\"cStatus\":";
			payload += rStatus[i];   // room status
			payload += "}";

			sendmqttMsg((char *)topic_pub.c_str(), (char *)payload.c_str());
		}
		sName[i] = "";
		old_celsius[i] = celsius[i];
	}
}
// -------------------------------------------------------------------
// MQTT sendmqttMsg sending
// -------------------------------------------------------------------
void sendmqttMsg(char* topictosend, String payload)
{

  if (mqttclient.connected()) {
    if (DEBUG_PRINT) {
      Serial.print("Sending payload: ");
      Serial.print(payload);
    }

    unsigned int msg_length = payload.length();

    if (DEBUG_PRINT) {
      Serial.print(" length: ");
      Serial.println(msg_length);
    }

    byte* p = (byte*)malloc(msg_length);
    memcpy(p, (char*) payload.c_str(), msg_length);

    if ( mqttclient.publish(topictosend, p, msg_length)) {
      if (DEBUG_PRINT) {
        Serial.println("Publish length TEMP ok");
      }
      free(p);
      //return 1;
    } else {
      if (DEBUG_PRINT) {
        Serial.println("Publish length TEMP failed");
      }
      free(p);
      //return 0;
    }
  }
}

// -------------------------------------------------------------------
// MQTT mqttCallback
// -------------------------------------------------------------------
void mqttCallback(char* topic_sub, byte* payload, unsigned int length)
{
	int i = 0;
	int r_Sensor;
    char buffer[80];
    int len = length >= 79 ? 79 : length;
    memcpy(buffer, payload, len);
    buffer[length] = 0;
	
    DEBUG.print(">> Topic: ");
    DEBUG.print(topic_sub);

    DEBUG.print(">> Payload: ");
    DEBUG.println(buffer);
	
	char * pch=0;
	//printf ("Looking for the ':' chars in \"%s\"...\n",buffer);
	pch=strchr(buffer,':');
	if(pch){
		tempTry = 0;
		r_Sensor = atoi(buffer);	// number of Sensors
		printf ("number of sensor %d\n",r_Sensor);
	  
		while (pch!=NULL)
		{
			L_Temp[i] = atof(pch+1);
			DEBUG.println(L_Temp[i]);
			//printf ("found at %d\n",pch-buffer+1);
			pch=strchr(pch+1,':');
			old_celsius[i] += 0.5;
			++i;
		}
	}
	else {
		subCommand = String(buffer);
		if (subCommand == do_fw_update) {
			DEBUG.print(">> do_fw_update_exe ");
			io2LIFEhttpUpdate(updateServer, fwImage);
		}
		else if (subCommand == do_reboot) {
			DEBUG.print(">> do_reboot_exe ");
			do_reboot_exe();
		}
		else if (subCommand == do_format) {
			DEBUG.print(">> do_format_exe:SPIFFS.format() ");
			SPIFFS.format();
			do_reboot_exe();
		}

	}
	
}

// -------------------------------------------------------------------
// MQTT verifyFingerprint
// -------------------------------------------------------------------
void verifyFingerprint() {
  //const char* host = MQTT_SERVER;

  Serial.print("Connecting to ");
  Serial.println(mqtt_server);

  //WiFiManager wifiManager;

  if (! espClient.connect(mqtt_server.c_str(), MQTT_PORT)) 
  {
	  Serial.println(mqtt_server);
	  Serial.println(MQTT_PORT);

	  //Serial.println("MQTT Connection failed. Halting execution.");
	  //wifiManager.resetSettings();
	  //wifiManager.startConfigPortal("AutoConnectAP","password");
  
	  //while(1);
  }

  if (espClient.verify(fingerprint, mqtt_server.c_str())) 
  {
    Serial.println("Connection secure.");
  } else {
	Serial.println("verify Connection insecure! Halting execution.");
	//wifiManager.resetSettings();
	//wifiManager.startConfigPortal("AutoConnectAP","password");

    //while(1);
  }
}
// -------------------------------------------------------------------
// MQTT macToTopic
// -------------------------------------------------------------------
void macToTopic()
{
  String result;
  result = WiFi.macAddress();
  for (int i = 0; i < 16; ) {
  MAC += result.substring(i, i+2);
  i +=3;
  }
  topic_pub += MAC;
  topic_sub += MAC;
}
// -------------------------------------------------------------------
// MQTT Connect
// -------------------------------------------------------------------
boolean mqtt_connect()
{
  macToTopic();

  mqttclient.setServer(mqtt_server.c_str(), 8883);
  DEBUG.println("MQTT Connecting...");
  //String strID = String(ESP.getChipId());
  String strID = String(WiFi.macAddress());
  if (mqttclient.connect(strID.c_str(), mqtt_user.c_str(), mqtt_pass.c_str())) {  // Attempt to connect
    DEBUG.println("MQTT connected");
		// Publish
		if (mqttclient.publish((char *)topic_pub.c_str(), "hello world...")) {
			DEBUG.println("publish ok");
		}
		else {
			DEBUG.println("publish failed");
		}
		// Subscribe
		if (mqttclient.subscribe((char *)topic_sub.c_str())) {
			DEBUG.println("Subscribe ok");
		}
		else {
			DEBUG.println("Subscribe failed");
		}		
		//mqttclient.publish(mqtt_topic.c_str(), "connected"); // Once connected, publish an announcement..
  } else {
    DEBUG.print("MQTT failed: ");
    DEBUG.println(mqttclient.state());
    return(0);
  }
  return (1);
}

// -------------------------------------------------------------------
// Publish to MQTT
// Split up data string into sub topics: e.g
// data = CT1:3935,CT2:325,T1:12.5,T2:16.9,T3:11.2,T4:34.7
// base topic = emon/emonesp
// MQTT Publish: emon/emonesp/CT1 > 3935 etc..
// -------------------------------------------------------------------
void mqtt_publish(String data)
{
  String mqtt_data = "";
  String topic = mqtt_topic + "/" + mqtt_feed_prefix;
  int i=0;
  while (int(data[i])!=0)
  {
    // Construct MQTT topic e.g. <base_topic>/CT1 e.g. emonesp/CT1
    while (data[i]!=':'){
      topic+= data[i];
      i++;
      if (int(data[i])==0){
        break;
      }
    }
    i++;
    // Construct data string to publish to above topic
    while (data[i]!=','){
      mqtt_data+= data[i];
      i++;
      if (int(data[i])==0){
        break;
      }
    }
    // send data via mqtt
    //delay(100);
    DEBUG.printf("%s = %s\r\n", topic.c_str(), mqtt_data.c_str());
    mqttclient.publish(topic.c_str(), mqtt_data.c_str());
    topic = mqtt_topic + "/" + mqtt_feed_prefix;
    mqtt_data="";
    i++;
    if (int(data[i])==0) break;
  }

  String ram_topic = mqtt_topic + "/" + mqtt_feed_prefix + "freeram";
  String free_ram = String(ESP.getFreeHeap());
  mqttclient.publish(ram_topic.c_str(), free_ram.c_str());
  
  		if (mqttclient.publish((char *)topic_pub.c_str(), "hello world...")) {
			DEBUG.println("publish ok");
		}
}

// -------------------------------------------------------------------
// MQTT state management
//
// Call every time around loop() if connected to the WiFi
// -------------------------------------------------------------------
void mqtt_loop()
{
  if ((!mqttclient.connected())&& (mqtt_server)) {
    long now = millis();
    // try and reconnect continuously for first 5s then try again once every 10s
    if ( (now < 50000) || ((now - lastMqttReconnectAttempt)  > 100000) ) {
      lastMqttReconnectAttempt = now;
      if (mqtt_connect()) { // Attempt to reconnect
        lastMqttReconnectAttempt = 0;
		
      }
    }
  } else {
    // if MQTT connected
    mqttclient.loop();
  }
}

void mqtt_restart()
{
  if (mqttclient.connected()) {
    mqttclient.disconnect();
  }
}

boolean mqtt_connected()
{
  return mqttclient.connected();
}

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
#include "wifi.h"
#include "config.h"
#include "output.h"

#include <ESP8266WiFi.h>              // Connect to Wifi
#include <ESP8266mDNS.h>              // Resolve URL for update server etc.
#include <DNSServer.h>                // Required for captive portal
//#include <Ticker.h>

DNSServer dnsServer;                  // Create class DNS server, captive portal re-direct
const byte DNS_PORT = 53;

// Access Point SSID, password & IP address. SSID will be softAP_ssid + chipID to make SSID unique
const char *softAP_ssid = "io2lifeESP";
const char* softAP_password = "password";
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);

// hostname for mDNS. Should work at least on windows. Try http://emonesp.local
const char *esp_hostname = "emonesp";

// Wifi Network Strings
String connected_network = "";
String status_string = "";
String ipaddress = "";

unsigned long Timer;
String st, rssi;


// -------------------------------------------------------------------
int wifi_mode = WIFI_MODE_STA;


// -------------------------------------------------------------------
// Start Access Point
// Access point is used for wifi network selection
// -------------------------------------------------------------------
void startAP() {
  DEBUG.println("Starting AP");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  DEBUG.print("WiFi Scan: ");
  int n = WiFi.scanNetworks();
  DEBUG.print(n);
  DEBUG.println(" networks found");
  st = "";
  rssi = "";
  for (int i = 0; i < n; ++i){
    st += "\""+WiFi.SSID(i)+"\"";
    rssi += "\""+String(WiFi.RSSI(i))+"\"";
	
	DEBUG.print(WiFi.SSID(i));  
	DEBUG.println(" : ");
	DEBUG.println(String(WiFi.RSSI(i)));
  
    if (i<n-1) st += ",";
    if (i<n-1) rssi += ",";
  }
  delay(100);

  WiFi.softAPConfig(apIP, apIP, netMsk);
  // Create Unique SSID e.g "emonESP_XXXXXX"
  String softAP_ssid_ID = String(softAP_ssid)+"_"+String(ESP.getChipId());;
  WiFi.softAP(softAP_ssid_ID.c_str(), softAP_password);

  // Setup the DNS server redirecting all the domains to the apIP
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);

  IPAddress myIP = WiFi.softAPIP();
  char tmpStr[40];
  sprintf(tmpStr,"%d.%d.%d.%d",myIP[0],myIP[1],myIP[2],myIP[3]);
  DEBUG.print("AP IP Address: ");
  DEBUG.println(tmpStr);
  ipaddress = tmpStr;
  DEBUG.println("AP Server Started!");
}

// -------------------------------------------------------------------
// Start Client, attempt to connect to Wifi network
// -------------------------------------------------------------------
void startClient() {
  LED_setup(0.5);

  DEBUG.print("Connecting as client to ");
  DEBUG.print(esid.c_str());
  DEBUG.print(" epass:");
  DEBUG.println(epass.c_str());
  WiFi.hostname("emonesp");
  WiFi.begin(esid.c_str(), epass.c_str());

  delay(50);

  int t = 0;
  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    t++;
    // push and hold boot button after power on to skip stright to AP mode
    if (t >= 20 || digitalRead(0) == LOW){
      DEBUG.println(" ");
      DEBUG.println("Try Again...");
      delay(2000);
      WiFi.disconnect();
      WiFi.begin(esid.c_str(), epass.c_str());
      t = 0;
      attempt++;
      if (attempt >= 1 || digitalRead(0) == LOW){
		DEBUG.println();
		DEBUG.println("Terminate Client mode ... start AP mode");
        startAP();
        // AP mode with SSID in EEPROM, connection will retry in 5 minutes
        wifi_mode = WIFI_MODE_AP_STA_RETRY;
		LED_setup(0.2);
        break;
      }
    }
  }

  if (wifi_mode == WIFI_MODE_STA || wifi_mode == WIFI_MODE_AP_AND_STA){
    IPAddress myAddress = WiFi.localIP();
    char tmpStr[40];
    sprintf(tmpStr,"%d.%d.%d.%d",myAddress[0],myAddress[1],myAddress[2],myAddress[3]);
    DEBUG.print("Connected, IP: ");
    DEBUG.println(tmpStr);
    // Copy the connected network and ipaddress to global strings for use in status request
    connected_network = esid;
    ipaddress = tmpStr;
	LED_clear();
  }
}

void wifi_setup()
{
  WiFi.disconnect();
  // 1) If no network configured start up access point
  if (esid == 0 || esid == "")
  {
    startAP();
    wifi_mode = WIFI_MODE_AP_ONLY; // AP mode with no SSID in EEPROM
  }
  // 2) else try and connect to the configured network
  else
  {
    WiFi.mode(WIFI_STA);
    wifi_mode = WIFI_MODE_STA;
    startClient();
  }

  // Start hostname broadcast in STA mode
  if ((wifi_mode==WIFI_MODE_STA || wifi_mode==WIFI_MODE_AP_AND_STA)){
    if (MDNS.begin(esp_hostname)) {
      MDNS.addService("http", "tcp", 80);
    }
  }

  Timer = millis();
}

void wifi_loop()
{
  dnsServer.processNextRequest(); // Captive portal DNS re-dierct

  // Remain in AP mode for 9 Minutes before resetting
  if (wifi_mode == WIFI_MODE_AP_STA_RETRY){
     if ((millis() - Timer) >= 900000){
		Serial.println("WILL reboot ESP system soon!!!!");
		pinMode(0, OUTPUT);
		digitalWrite(0, HIGH);
		delay(5000);
		ESP.reset();
		DEBUG.println("WIFI Mode = 1, resetting");
     }
  }
}

void wifi_restart()
{
  // Startup in STA + AP mode
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(apIP, apIP, netMsk);

  // Create Unique SSID e.g "emonESP_XXXXXX"
  String softAP_ssid_ID = String(softAP_ssid)+"_"+String(ESP.getChipId());;
  WiFi.softAP(softAP_ssid_ID.c_str(), softAP_password);

  // Setup the DNS server redirecting all the domains to the apIP
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);
  wifi_mode = WIFI_MODE_AP_AND_STA;
  startClient();
}

void wifi_scan()
{
  DEBUG.println("WIFI Scan");
  int n = WiFi.scanNetworks();
  DEBUG.print(n);
  DEBUG.println(" networks found");
  st = "";
  rssi = "";
  for (int i = 0; i < n; ++i){
    st += "\""+WiFi.SSID(i)+"\"";
    rssi += "\""+String(WiFi.RSSI(i))+"\"";
    if (i<n-1) st += ",";
    if (i<n-1) rssi += ",";
  }
}

void wifi_disconnect()
{
  WiFi.disconnect();
}

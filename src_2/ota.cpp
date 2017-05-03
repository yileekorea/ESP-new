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
#include "ota.h"
#include "web_server.h"
#include "wifi.h"
#include "http.h"

#include <ArduinoOTA.h>               // local OTA update from Arduino IDE
#include <ESP8266httpUpdate.h>        // remote OTA update from server
#include <ESP8266HTTPUpdateServer.h>  // upload update
#include <FS.h>

ESP8266HTTPUpdateServer httpUpdater;  // Create class for webupdate handleWebUpdate()


// -------------------------------------------------------------------
//OTA UPDATE SETTINGS
// -------------------------------------------------------------------
//UPDATE SERVER strings and interfers for upate server
// Array of strings Used to check firmware version

String updateServer_fwImage;

const char* updateServer = "http://iot2better.iptime.org:8003/fwImage/";
const char* fwImage = "io2life.bin";

const char* u_host = "217.9.195.227";
const char* u_url = "/esp/firmware.php";
const char* firmware_update_path = "/upload";

extern const char *esp_hostname;

void ota_setup()
{
  // Start local OTA update server
  ArduinoOTA.setHostname(esp_hostname);
  ArduinoOTA.begin();

  // Setup firmware upload
  httpUpdater.setup(&server, firmware_update_path);
}

void ota_loop()
{
  ArduinoOTA.handle();
}

String ota_get_latest_version()
{
  // Get latest firmware version number
  String url = u_url;
  return get_http(u_host, url);
}

t_httpUpdate_return ota_http_update()
{
  SPIFFS.end(); // unmount filesystem
  t_httpUpdate_return ret = ESPhttpUpdate.update("http://" + String(u_host) + String(u_url) + "?tag=" + currentfirmware);
  SPIFFS.begin(); //mount-file system
  return ret;
}
void do_reboot_exe() 
{
	Serial.println("WILL reboot ESP system soon!!!!");
	pinMode(0, OUTPUT);
	digitalWrite(0, HIGH);
	delay(5000);
	ESP.reset();
}

void io2LIFEhttpUpdate(const char* updateServer, const char* fwImage)
{
  if (WiFi.status() == WL_CONNECTED) {
        updateServer_fwImage = updateServer;
        updateServer_fwImage += fwImage;
		
		String REMOTE_SERVER = updateServer;
		String SKETCH_BIN = fwImage;

		ESPhttpUpdate.rebootOnUpdate(BOOT_AFTER_UPDATE);
		//ESPhttpUpdate.rebootOnUpdate(true);

        t_httpUpdate_return ret = ESPhttpUpdate.update(updateServer_fwImage);
        //t_httpUpdate_return ret = ESPhttpUpdate.update(REMOTE_SERVER, 443, SKETCH_BIN);

        switch(ret) {
            case HTTP_UPDATE_FAILED:
                DEBUG.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
                break;

            case HTTP_UPDATE_NO_UPDATES:
                DEBUG.println("HTTP_UPDATE_NO_UPDATES");
                break;

            case HTTP_UPDATE_OK:
                DEBUG.println("HTTP_UPDATE_OK");
				do_reboot_exe();
                break;
        }
    }
}

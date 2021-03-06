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

#ifndef _EMONESP_MQTT_H
#define _EMONESP_MQTT_H

#define DEBUG_PRINT 1
// -------------------------------------------------------------------
// MQTT support
// -------------------------------------------------------------------

#include <Arduino.h>


// -------------------------------------------------------------------
// Perform the background MQTT operations. Must be called in the main
// loop function
// -------------------------------------------------------------------
extern void mqtt_loop();

// -------------------------------------------------------------------
// Publish values to MQTT
//
// data: a comma seperated list of name:value pairs to send
// -------------------------------------------------------------------
extern void mqtt_publish(String data);

// -------------------------------------------------------------------
// Restart the MQTT connection
// -------------------------------------------------------------------
extern void mqtt_restart();


// -------------------------------------------------------------------
// Return true if we are connected to an MQTT broker, false if not
// -------------------------------------------------------------------
extern boolean mqtt_connected();

extern void verifyFingerprint();

extern void mqttCallback(char* topic_sub, byte* payload, unsigned int length);

extern void sendmqttMsg(char* topictosend, String payload);

extern void sendTempData();

#endif // _EMONESP_MQTT_H

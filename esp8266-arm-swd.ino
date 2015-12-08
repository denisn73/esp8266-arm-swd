/*
 * Proof of concept ESP8266 Wifi + HTTP web interface, for remote control
 * of an ARM microcontroller via its Serial Wire Debug port.
 */

// Please edit wifi_config.h to set up your access point.

#include "wifi_config.h"

// ESP-01: GPIO0 = swdclk, GPIO2 = swdio
// NodeMCU devkit: D3 = swdclk, D4 = swdio
// SWD header: pin1 = 3.3v, pin2 = swdio, pin3 = gnd, pin4 = swdclk

const int swd_clock_pin = 0;
const int swd_data_pin = 2;

/* 
 * Copyright (c) 2015 Micah Elizabeth Scott
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "arm_debug.h"
#include "arm_kinetis_debug.h"

ESP8266WebServer server(80);
ARMKinetisDebug target(swd_clock_pin, swd_data_pin);

void handleWebRoot()
{
    server.send(200, "text/html", "<html><body>Ohai!</body></html>");
}

void setup(void)
{
    Serial.begin(115200);
    Serial.println("Starting up...");

    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(ssid, password);
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        WiFi.begin(ssid, password);
        Serial.println("Wifi retrying...");
    }
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    server.on("/", handleWebRoot);
    server.begin();

    MDNS.begin(host);
    MDNS.addService("http", "tcp", 80);

    Serial.printf("Server is running at http://%s.local/\n", host);
}

void loop(void)
{
    server.handleClient();
    delay(1);
}

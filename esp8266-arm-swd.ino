////////////////////////////////////////////////////////////////
//
//    Proof of Concept ESP8266 Wifi + HTTP web interface
//     for remote control of an ARM microcontroller
//      via its Serial Wire Debug port.
//
////////////////////////////////////////////////////////////////

// Copyright (c) 2015 Micah Elizabeth Scott
// Released with an MIT-style license; see LICENSE file

// Please edit wifi_config.h to set up your access point. 

#include "wifi_config.h"

// Default pins:
//    ESP-01          GPIO0 = swdclk, GPIO2 = swdio
//    NodeMCU devkit  D3 = swdclk, D4 = swdio
//
// And for reference:
//    SWD header      pin1 = 3.3v, pin2 = swdio, pin3 = gnd, pin4 = swdclk

const int swd_clock_pin = 0;
const int swd_data_pin = 2;

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
    server.send(200, "text/html", "<html><body>Ohai again!</body></html>");
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

////////////////////////////////////////////////////////////////
//
//    Proof of Concept ESP8266 Wifi + HTTP web interface
//     for remote control of an ARM microcontroller
//      via its Serial Wire Debug port.
//
//    Designed for the FRDM-KE04Z dev kit as an example target,
//    with its MKE04Z8VFK4 Cortex M0+ micro.
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
//    SWD over JTAG   TCLK = swdclk, TMS = swdio

const int swd_clock_pin = 0;
const int swd_data_pin = 2;

////////////////////////////////////////////////////////////////

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "arm_debug.h"
#include "arm_kinetis_debug.h"
#include "arm_kinetis_reg.h"

ESP8266WebServer server(80);
ARMKinetisDebug target(swd_clock_pin, swd_data_pin, ARMDebug::LOG_NORMAL);

void appendHex32(String &buffer, uint32_t word)
{
    char tmp[10];
    sprintf(tmp, "0x%08X", word);
    buffer += tmp;
}

void handleWebRoot()
{
    String output = "<html><body><pre>";

    output += "Howdy, neighbor!\n";
    output += "Nice to <a href=\"https://github.com/scanlime/esp8266-arm-swd\">meet you</a>.\n\n";

    if (!target.begin()) {
        output += "Unfortunately,\n";
        output += "I failed to connect to the debug port.\n";
        output += "Check your wiring maybe?\n";
        goto done;
    }
    output += "Connected to the ARM debug port.\n";

    uint32_t idcode;
    if (target.getIDCODE(idcode)) {
        output += "This processor has an IDCODE of ";
        appendHex32(output, idcode);
        output += "\n";
    }

    if (target.startup()) {
        output += "Putting the target into debug-halt, to keep things from getting too crazy just yet.\n";

        output += "\nSome memory...\n\n";
        uint32_t addr = 0x1000;
        uint32_t word;
        for (unsigned i = 0; i < 128; i++) {
            appendHex32(output, addr);
            output += ":";
            for (unsigned j = 0; j < 4; j++) {
                output += " ";
                if (target.memLoad(addr, word)) {
                    appendHex32(output, word);
                } else {
                    output += "--------";
                }
                addr += 4;
            }
            output += "\n";
        }
    }

done:
    output += "</pre></body></html>";
    server.send(200, "text/html", output);
}

void setup(void)
{
    Serial.begin(115200);
    Serial.println("\n\n~ Starting up ~\n");

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

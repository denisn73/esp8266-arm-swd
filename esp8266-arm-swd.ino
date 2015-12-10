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
#include "webapp.h"

ESP8266WebServer server(80);
ARMKinetisDebug target(swd_clock_pin, swd_data_pin, ARMDebug::LOG_NORMAL);

void appendHex32(String &buffer, uint32_t word)
{
    // Formatting utility for fixed-width hex integers, which don't come easily with just WString

    char tmp[16];
    snprintf(tmp, sizeof tmp, "0x%08X", word);
    buffer += tmp;
}

uint32_t intArg(const char *name)
{
    // Like server.arg(name).toInt(), but it handles integer bases other than 10
    // with C-style prefixes (0xNUMBER for hex, or 0NUMBER for octal)

    uint8_t tmp[16];
    server.arg(name).getBytes(tmp, sizeof tmp, 0);
    return strtol((char*) tmp, 0, 0);
}

void handleWebRoot()
{
    uint32_t addr = 0x1000;
    uint32_t word, idcode;

    String output = kWebAppHeader;

    output += "Howdy, neighbor!\n";
    output += "Nice to <a href='https://github.com/scanlime/esp8266-arm-swd'>meet you</a>.\n\n";

    if (!target.begin()) {
        output += "Unfortunately,\n";
        output += "I failed to connect to the debug port.\n";
        output += "Check your wiring maybe?\n";
        output += kWebAppFooter;
        server.send(200, "text/html", output);
        return;
    }
    output += "Connected to the ARM debug port.\n";

    if (target.getIDCODE(idcode)) {
        output += "This processor has an IDCODE of ";
        appendHex32(output, idcode);
        output += "\n";
    }

    if (target.detect()) {
        output += "And we have the Kinetis chip-specific extensions, neat:\n";
        output += " > <a href='#' onclick='targetReset()'>reset</a> <span id='targetResetResult'></span>\n";
        output += " > <a href='#' onclick='targetHalt()'>debug halt</a> <span id='targetHaltResult'></span>\n";
    } else {
        output += "We don't know this chip's specifics, so all you get is maybe memory access.\n";
    }

    output += "\nSome flash memory maybe:\n\n";
    output += "<script>hexDump(0x00000000, 0x1000);</script>";

    output += "\nAnd some RAM:\n\n";
    output += "<script>hexDump(0x20000000, 0x1000);</script>";

    output += kWebAppFooter;
    server.send(200, "text/html", output);
}

void handleMemLoad()
{
    // Read 'count' words starting at 'addr', returning a JSON array

    uint32_t addr = intArg("addr");
    uint32_t count = constrain(intArg("count"), 1, 1024);
    uint32_t value;
    String output = "[";

    while (count) {
        if (target.memLoad(addr, value)) {
            output += value;
        } else {
            output += "null";
        }
        addr += 4;
        count--;
        if (count) {
            output += ",";
        }
    }

    output += "]\n";
    server.send(200, "application/json", output);
}

void handleMemStore()
{
    // Interprets the argument list as a list of stores to make in order.
    // The key in the key=value pair consists of an address with an optional
    // width prefix ('b' = byte wide, 'h' = half width, default = word)
    // The address can be a '.' to auto-increment after the previous store.
    //
    // Returns some confirmation text, to make it easier to see what happened.
    // This is intended for interactive use, not really to be machine readable.

    uint32_t addr = -1;
    String output = "";

    for (int i = 0; server.argName(i).length() > 0; i++) {
        uint8_t arg[64];
        server.argName(i).getBytes(arg, sizeof arg, 0);
        
        uint8_t *addrString = &arg[arg[0] == 'b' || arg[0] == 'h'];
        if (addrString[0] != '.') {
            addr = strtol((char*) addrString, 0, 0);
        }

        uint8_t valueString[10];
        server.arg(i).getBytes(valueString, sizeof valueString, 0);
        uint32_t value = strtol((char*) valueString, 0, 0);

        char result[64];
        switch (arg[0]) {

            case 'b':
                value &= 0xff;
                snprintf(result, sizeof result,
                    "store byte %02x -> %08x%s\n", 
                    target.memStoreByte(addr, value) ? "" : " (failed!)");
                addr++;
                break;

            case 'h':
                value &= 0xffff;
                snprintf(result, sizeof result,
                    "store half %04x -> %08x%s\n", 
                    target.memStoreHalf(addr, value) ? "" : " (failed!)");
                addr += 2;
                break;

            default:
                snprintf(result, sizeof result,
                    "store %08x -> %08x%s\n", 
                    target.memStore(addr, value) ? "" : " (failed!)");
                addr += 4;
                break;
        }
        output += result;
    }

    server.send(200, "text/plain", output);
}

void setup()
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
    server.on("/load", handleMemLoad);
    server.on("/store", handleMemStore);

    server.on("/reset", [](){
        server.send(200, "application/json", target.reset() ? "true" : "false");});
    server.on("/halt", [](){
        server.send(200, "application/json", target.debugHalt() ? "true" : "false");});

    server.begin();

    MDNS.begin(host);
    MDNS.addService("http", "tcp", 80);

    Serial.printf("Server is running at http://%s.local/\n", host);
}

void loop()
{
    server.handleClient();
    delay(1);
}

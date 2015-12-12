////////////////////////////////////////////////////////////////
//
//    Proof of Concept ESP8266 Wifi + HTTP web interface
//     for remote control of an ARM microcontroller
//      via its Serial Wire Debug port.
//
//    Tested with the FRDM-KE04Z dev kit (KE04 Cortex M0+)
//    and the Fadecandy board (K20 Cortex M4)
//
////////////////////////////////////////////////////////////////

// Copyright (c) 2015 Micah Elizabeth Scott
// Released with an MIT-style license; see LICENSE file

// Default pins:
//    ESP-01          GPIO0 = swdclk, GPIO2 = swdio
//    NodeMCU devkit  D3 = swdclk, D4 = swdio
//
// And for reference:
//    SWD header      pin1 = 3.3v, pin2 = swdio, pin3 = gnd, pin4 = swdclk
//    SWD over JTAG   TCLK = swdclk, TMS = swdio

const int swd_clock_pin = 0;
const int swd_data_pin = 2;

// Edit these in the YOUR-WIFI-CONFIG tab
extern const char *host, *ssid, *password;

////////////////////////////////////////////////////////////////

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>

#include "arm_debug.h"
#include "arm_kinetis_debug.h"
#include "arm_kinetis_reg.h"

ESP8266WebServer server(80);

// Turn the log level back up for debugging; but by default, we have it
// completely off so that even failures happen quickly, to keep the web app responsive.
ARMKinetisDebug target(swd_clock_pin, swd_data_pin, ARMDebug::LOG_NONE);

uint32_t intArg(const char *name)
{
    // Like server.arg(name).toInt(), but it handles integer bases other than 10
    // with C-style prefixes (0xNUMBER for hex, or 0NUMBER for octal)

    uint8_t tmp[64];
    server.arg(name).getBytes(tmp, sizeof tmp, 0);
    return strtoul((char*) tmp, 0, 0);
}

const char *boolStr(bool x)
{
    // JSON compatible
    return x ? "true" : "false";
}

void handleLoad()
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

void handleStore()
{
    // Interprets the argument list as a list of stores to make in order.
    // The key in the key=value pair consists of an address with an optional
    // width prefix ('b' = byte wide, 'h' = half width, default = word)
    // The address can be a '.' to auto-increment after the previous store.
    //
    // Returns a confirmation and result for each store, as JSON.

    uint32_t addr = -1;
    String output = "[\n";

    for (int i = 0; server.argName(i).length() > 0; i++) {
        uint8_t arg[64];
        server.argName(i).getBytes(arg, sizeof arg, 0);

        uint8_t *addrString = &arg[arg[0] == 'b' || arg[0] == 'h'];
        if (addrString[0] != '.') {
            addr = strtoul((char*) addrString, 0, 0);
        }

        uint8_t valueString[64];
        server.arg(i).getBytes(valueString, sizeof valueString, 0);
        uint32_t value = strtoul((char*) valueString, 0, 0);

        bool result;
        const char *storeType = "word";

        switch (arg[0]) {
        case 'b':
            value &= 0xff;
            storeType = "byte";
            result = target.memStoreByte(addr, value);
            addr++;
            break;

        case 'h':
            storeType = "half";
            value &= 0xffff;
            result = target.memStoreHalf(addr, value);
            addr += 2;
            break;

        default:
            result = target.memStore(addr, value);
            addr += 4;
            break;
        }

        char buf[128];
        snprintf(buf, sizeof buf,
                "%s{\"store\": \"%s\", \"addr\": %lu, \"value\": %lu, \"result\": %s}",
                i ? "," : "", storeType, addr, value, boolStr(result));
        output += buf;
    }
    output += "\n]";

    server.send(200, "application/json", output);
}

void handleBegin()
{
    // See if we can communicate. If so, return information about the target.
    // This shouldn't reset the target, but it does need to communicate,
    // and the debug port itself will be reset.
    //
    // If all is well, this returns some identifying info about the target.

    uint32_t idcode;
    if (target.begin() && target.getIDCODE(idcode)) {
        char result[128];

        // Note the room left in the API for future platforms detected,
        // even though it requires refactoring a bit.

        snprintf(result, sizeof result,
            "{\"connected\": true, \"idcode\": %lu, \"detected\": %s}",
            idcode, target.detect() ? "\"kinetis\"" : "false");

        server.send(200, "application/json", result);
    } else {
        server.send(200, "application/json", "{\"connected\": false}");
    }
}

String getContentType(String filename)
{
    if (filename.endsWith(".html")) return "text/html";
    else if (filename.endsWith(".css")) return "text/css";
    else if (filename.endsWith(".js"))  return "application/javascript";
    else if (filename.endsWith(".png")) return "image/png";
    else if (filename.endsWith(".gif")) return "image/gif";
    else if (filename.endsWith(".jpg")) return "image/jpeg";
    else if (filename.endsWith(".ico")) return "image/x-icon";
    else if (filename.endsWith(".xml")) return "text/xml";
    else if (filename.endsWith(".pdf")) return "application/x-pdf";
    else if (filename.endsWith(".zip")) return "application/x-zip";
    else if (filename.endsWith(".gz"))  return "application/x-gzip";
    return "text/plain";
}

bool streamFileIfExists(String path)
{
    if (SPIFFS.exists(path)) {
        String contentType = getContentType(path);
        File f = SPIFFS.open(path, "r");
        server.streamFile(f, contentType);
        f.close();
        return true;
    }
    return false;
}

void handleStaticFile()
{
    String path = server.uri();
    if (path.endsWith("/")) {
        path += "index";
    }

    if (!streamFileIfExists(path) &&
        !streamFileIfExists(path + ".html")) {
        server.send(404, "text/plain",
            "File not found in ESP8266FS flash.\n"
            "Did you run Tools -> ESP8266 Sketch Data Upload?");
    }
}

void setup()
{
    Serial.begin(115200);
    Serial.println("\n\n~ Starting up ~\n");

    SPIFFS.begin();
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {    
        String fileName = dir.fileName();
        Serial.printf("FS File: %s\n", fileName.c_str());
    }
    Serial.println();

    do {
        Serial.println("\nGetting the wifi going...");
        WiFi.mode(WIFI_AP_STA);
        WiFi.begin(ssid, password);
    } while (WiFi.waitForConnectResult() != WL_CONNECTED);

    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    server.on("/api/begin", handleBegin);
    server.on("/api/load", handleLoad);
    server.on("/api/store", handleStore);

    server.on("/api/reset", [](){
        server.send(200, "application/json", boolStr(target.reset()));});

    server.on("/api/halt", [](){
        server.send(200, "application/json", boolStr(target.debugHalt()));});

    server.onNotFound(handleStaticFile);
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

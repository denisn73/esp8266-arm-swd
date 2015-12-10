# esp8266-arm-swd

Proof-of-concept ESP8266 web server and ARM Serial Wire Debug client!

This directory is an Arduino sketch.

The Serial Wire Debug client here is a slightly adapted version of the one I originally wrote for Fadecandy's factory test infrastructure. It has optional extensions for the Freescale Kinetis microcontrollers, but the lower-level SWD interface should be compatible with any ARM microcontroller.

Usage:

* Make sure to edit `wifi_config.h` with your WiFi name and password
* Hook it up to something with an ARM SWD port
* Visit [the web page](http://esp8266-swd.local), edit all the hexes

Hookups:

| Signal      | SWD Port | SWD over JTAG | ESP8266 pin | Equiv. NodeMCU pin |
| ----------- | -------- | ------------- | ----------- | ------------------ |
| 3.3v Power  | Pin 1    |               |             |                    |
| Data In/Out | Pin 2    | TMS           | GPIO2       | D4                 |
| Ground      | Pin 3    |               |             |                    |
| Clock       | Pin 4    | TCLK          | GPIO0       | D3                 |

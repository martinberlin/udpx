![udpx Logo](/examples/udpix-logo.png)

**udpx is a compression technology to transport data.**

This technology will be used in a future project in partnership with Hendrik Putzek but can be individually tested as mean to transport compressed data between devices.

### License

This repository is licensed with a ["CC Attribution-NonCommercial"](https://creativecommons.org/licenses/by-nc/4.0/legalcode) License.

It contains code and examples that can be used and copied freely for artistic works but in any way used for commercial projects. 

## Test after compiling

    cat examples/72-pix.json.br |nc -w1 -u ESP32_IP_ADDRESS 1234

Should turn on some Leds provided netcat is installed on your system.

    cat examples/72-pix-off.json.br |nc -w1 -u ESP32_IP_ADDRESS 1234

Should turn off all leds in the 72 addressable led stripe. Use the 144-pix version to try this on 144 leds.

## Important configuration

    /lib/Config/Config.h

Mandatory:
WIFI_SSID / WIFI_PASS 

UDP_PORT

MQTT_ENABLE is set to false by default. Only required if you need to register the ESP32 in an external controller.

    /lib/Output/Output.h

The number of leds in the stripe: 

#define PIXELCOUNT 72

The data pin of the addressable leds:

#define PIXELPIN 19
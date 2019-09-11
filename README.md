![udpx Logo](/examples/udpix-logo.png)

**udpx is a compression technology to transport data.**

This technology will be used in a future project in partnership with Hendrik Putzek but can be individually tested as mean to transport compressed data between devices.

This repository represents the Firmware part and it should be compiled using Platformio in a ESP32 board. 
[Pixelpusher API](https://github.com/IoTPanic/pixelpusher) is a work in progress by IoTPanic that will be a DMX like architecture to control LEDS

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

### History

**2019-06** Started working with Front-end developer [Hendrik Putzek](https://twitter.com/hputzek) to develop a firmware solution to transport a lot of frames per seconds from his Nodejs backend to a Led controller. first version used JSON and  [Brotli](http://manpages.ubuntu.com/manpages/bionic/man1/brotli.1.html) as a compression algorithm to transport data between frontend (nodejs) and the firmware (esp32)

**2019-07** Decided to name the Firmware UDPX and make it open-source to let another developers join forces and use it for their own needs

**2019-08** [Samuel Archibald](https://twitter.com/IoTPanic) Made his first contribution and started a new protocol to get rid of JSON and send this even faster. The new protocol receives bytes instead of JSON and it lives in develop branch

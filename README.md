![udpx Logo](/examples/udpix-logo.png)

**udpx is a compression technology to transport data.**

This technology will be used in a future project in partnership with Hendrik Putzek but can be individually tested as mean to transport compressed data between devices.
We are using [Brotli](http://manpages.ubuntu.com/manpages/bionic/man1/brotli.1.html) as a compression algorithm to transport data between frontend (nodejs) and the firmware (esp32)
This library represents the Firmware part and it should be compiled using Platformio in a ESP32 board. 

### Branch feature/examples

This branch is running the actual Pixels binary version and it's where we will create the examples section. Tools where we can measure speed, send different patterns and animations via UDP to the ESP32 Controller. Is testeable sending from linux bash:

cat examples/72-on.bin|nc -w1 -u 192.168.0.ESP32_IP 1234

cat examples/72-of.bin|nc -w1 -u 192.168.0.ESP32_IP 1234

This should send 72 pixels (Note that not all are on)
To understand the binary please read [the pixels section](https://github.com/martinberlin/udpx/tree/PIXELS%2Bs/lib/pixels)


### License

This repository is licensed with a ["CC Attribution-NonCommercial"](https://creativecommons.org/licenses/by-nc/4.0/legalcode) License.

It contains code and examples that can be used and copied freely for artistic works but in any way used for commercial projects. 

## Important configuration

    /lib/Config/Config.h

**Mandatory:**
WIFI_SSID / WIFI_PASS 

UDP_PORT

MQTT_ENABLE is set to false by default. Only required if you need to register the ESP32 in an external controller.

    /lib/pixels/src/pixels.h


    // The number of leds in the stripe:
    #define PIXELCOUNT 144
    // The data pin of the addressable leds:
    #define PIXELPIN 19


### History

**2019-06** Started working with Front-end developer [Hendrik Putzek](https://twitter.com/hputzek) to develop a firmware solution to transport a lot of frames per seconds from his Nodejs backend to a Led controller

**2019-07** Decided to name the Firmware UDPX and make it open-source to let another developers join forces and use it for their own needs

**2019-08** [Samuel Archibald](https://twitter.com/IoTPanic) Made his first contribution and started a new protocol to get rid of JSON and send this even faster

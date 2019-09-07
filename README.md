![udpx Logo](/examples/udpix-logo.png)

**udpx is a compression technology to transport data.**

This technology will be used in a future project in partnership with Hendrik Putzek but can be individually tested as mean to transport compressed data between devices.
We are using [Brotli](http://manpages.ubuntu.com/manpages/bionic/man1/brotli.1.html) as a compression algorithm to transport data between frontend (nodejs) and the firmware (esp32)
This library represents the Firmware part and it should be compiled using Platformio in a ESP32 board. 

### License

This repository is licensed with a ["CC Attribution-NonCommercial"](https://creativecommons.org/licenses/by-nc/4.0/legalcode) License.

It contains code and examples that can be used and copied freely for artistic works but in any way used for commercial projects. 

## Test after compiling

Main test that should pass before merging in develop: 

examples/test

    Open it in your browser
    nodejs middleware.js 
    Send UDP packets to the ESP32 IP in both RGB / RGBW to certify it works, does not crash and fullfills expected behaviour.

Please DO NOT merge in any stable branch before the tests pass. Keeping this methodology we will save the development team lot's of time that can be used wisely.

## Branching model

Please adhere to following branching model when working in this repository. Not strictly, just to mantein a certain order and avoid loosing time. 

    master  → Main stable branch (clean/no debugging)
      ↑↓
    develop → Where features are merged only *after testing* (serial debug is ok)
      ↓
    feature/1 → Merged back in develop *only* after testing 
    bug/1     → Merged back in develop (Same methodology)

    martin/myproject → Is no problem, but after some time let's
    hendrik/hisproject → fork it into your own project
    samuel/project 

    Please except of PIXELS+s no stale branches unless there is a real need to have them. 

Testing should pass when examples/test is open in the browser and two different ESP32 with RGB /RGBW stripe outputs match the expected results.

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

This line will enable RGBW mode sending 4 bytes per pixel. Comment to use a RGB stripe in /lib/src/pixels/src/pixels.h

#define RGBW

**Note:** Using a RGB / RGBW enforces you to update that line but also the [Neopixel class features](https://github.com/Makuna/NeoPixelBus/wiki/NeoPixelBus-object#neo-features) depending on the addressable leds you use (/lib/src/pixels/src/pixels.cpp)

### History

**2019-06** Started working with Front-end developer [Hendrik Putzek](https://twitter.com/hputzek) to develop a firmware solution to transport a lot of frames per seconds from his Nodejs backend to a Led controller

**2019-07** Decided to name the Firmware UDPX and make it open-source to let another developers join forces and use it for their own needs

**2019-08** [Samuel Archibald](https://twitter.com/IoTPanic) Made his first contribution and started a new protocol to get rid of JSON and send this even faster

**2019-09** Tests and fixes to achiebe a stable RGB, RGBW version

#### Old command line test

Use it only to check that compilation went right. For real tests please use the examples/test browser POST -> middleware.js -> UDP to ESP32

Command line test:

    cat examples/72-pix.json.br |nc -w1 -u ESP32_IP_ADDRESS 1234

Should turn on some Leds provided netcat is installed on your system.

    cat examples/72-pix-off.json.br |nc -w1 -u ESP32_IP_ADDRESS 1234

Should turn off all leds in the 72 addressable led stripe. Use the 144-pix version to try this on 144 leds.
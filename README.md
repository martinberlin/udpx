![udpx Logo](/examples/udpix-logo.png)

**udpx is a technology to transport data over WiFi to microcontrollers**


[Pixelpusher API](https://github.com/IoTPanic/pixelpusher) is a work in progress by IoTPanic that will use a DMX like architecture to control LEDS.
UDPX represents the ESP32 firmware where this Protocols will be tested and implemented. It's a collaborative effort of a team of makers that like tinkering with Espressif microcontrollers.

### TEAM

UDPX is a collaborative effort where a team of 3 have same access level to the repository:

[Hendrik Putzek](https://github.com/hputzek)   - Front end / Nodejs, VUE

[Samuel Archibald](https://github.com/IoTPanic) - C++ / GO Api backend

[Martin Fasani](https://github.com/martinberlin) - Firmware and testing

## Additional libraries

**S or "Little Stream"** is an an embedded streaming library for embedded devices. Is simple data transport layer that is meant to be used in the UDPX project that both is small as possible, and made for real-time applications, which has the ability to be compressed. And it also overcomes the maximum transport size limit of the ESP32 on Arduino framework, since you cannot receive an UDP bigger than 1470 bytes, S takes care of joining the data for you having a callback that get's called once it receives the last package.
S is being currently tested and implemented in the **testing/s** branch of this repository.

**Pixels** is a binary transport protocol. A way to send bytes to an ESP32 that are after arrival decoded and send to a RGB / RGBw Led strip. 

Both where developed by Samuel and are used by UDPX. This firmware a part of integrating this libraries has the MQTT part to register in Pixelpusher. Although all this is evolving and in the future they will be a [GO api](https://github.com/IoTPanic/pixelpusher) that may handle this part as well.

## Branches


**develop** Main branch only with Pixels protocol. 

**testing/s** Implementing here the Pixels+S Protocol (Aka "Little stream" https://github.com/IoTPanic/s)

**feature/tv-matrix** On this one we are experimenting sending compressed Pixels to a WS2812BALLPANEL Led matrix Panel 44x11 ([Demo with 484 pixels](https://twitter.com/martinfasani/status/1182244962395656192))

**feature/tcp** A TCP experiment since Martin had to do something for a client that wanted to use HTTP, interesting to see how fast is UDP compared to TCP, just left there for research reasons.

**feature/json-v0.1** Only stored for historical reasons, it our first JSON + Brotli compressed protocol, that was tested and even though the limitations [proved to be quite fast](https://twitter.com/martinfasani/status/1166106095858966529).


## Test after compiling

Main test that should pass before merging in develop: 

examples/test

    Open it in your browser
    nodejs middleware.js 
    Send UDP packets to the ESP32 IP in both RGB / RGBW to certify it works, does not crash and fullfills expected behaviour.

Please DO NOT merge in any stable branch before the tests pass. Keeping this methodology we will save the development team lot's of time that can be used wisely.

## Branching model

Please adhere to following branching model when working in this repository. Not strictly, just to maintain a certain order and avoid loosing time. 

    master  → Main stable branch (clean/no debugging)
      ↑↓
    develop → Where features are merged only *after testing* (serial debug is ok)
      ↓
    feature/1 → Merged back in develop *only* after testing 
    bug/1     → Merged back in develop
    

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

**2019-09** Tests and fixes to achieve a stable RGB, RGBW version

#### Old command line test

Use it only to check that compilation went right. For real tests please use the examples/test browser POST -> middleware.js -> UDP to ESP32

Command line test:

    cat examples/72-pix.json.br |nc -w1 -u ESP32_IP_ADDRESS 1234

Should turn on some Leds provided netcat is installed on your system.

    cat examples/72-pix-off.json.br |nc -w1 -u ESP32_IP_ADDRESS 1234

Should turn off all leds in the 72 addressable led stripe. Use the 144-pix version to try this on 144 leds.

### License

This repository is licensed with a ["CC Attribution-NonCommercial"](https://creativecommons.org/licenses/by-nc/4.0/legalcode) License.

It contains code and examples that can be used and copied freely for artistic works and it can be also used for commercial projects provided you write as a line to explain how is going to be used and our license and credits are maintened.
We put really a lot of time and effort building this Firmware and we would like to give proffesional support to integrate it in the future.

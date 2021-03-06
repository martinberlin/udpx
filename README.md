![udpx Logo](/examples/udpix-logo.png)

**udpx is a technology to transport data over WiFi to microcontrollers**

### What it can be used for

1. Receive frame by frame animations generated by any UDP [client application](https://play.google.com/store/apps/details?id=io.cordova.udpx)
2. Receive over the air Video frames
3. Be used as a Protocol base for a different mission like send on real time information from sensors

### How to test this

**Android App latest builds**
If you want to test this using Android just download and install the udpx app from Play store: <a href="https://play.google.com/store/apps/details?id=io.cordova.udpx" />
<img src="/examples/udpx-app-180x120.jpg" />
[udpx esp32](https://play.google.com/store/apps/details?id=io.cordova.udpx)

The [udpx Android App](https://github.com/martinberlin/udpx-app) is also open source. Feel free to explore and learn Cordova to make hybrid apps.

### Configuring WiFi using Bluetooth

Starting with version 1.1 defined in lib/Config.h the firmware supports receiving configuration over Bluetooth Serial.

1.- Firmware boots, opens Bluetooth serial and waits for 9 seconds as default. Serial output:  

    UDPX 1.x
    BTSerial active. Device name: udpx-3C71BF9D53DC_1234

2.- If in this time receives configuration it updates the SSID / Password and stores it on Preferences

3.- If it does not, after the wait time, tries to connect to the last known access point.

Serial output:

    Start connection to AP_NAME
    udpx-3C71BF9D53DC_1234.local is online
    UDP Listening on: 
    192.168.0.99:1234

If a fast start without wait is desired the best would be to add a hardware button, that only when pressed will wait for configuration.

First Pixel in the Led stripe or matrix will reflect the status

1. BLUE     Waiting for Bluetooth config 
2. GREEN    Connected successfully, got IP
3. RED      Connection failed, will turn to 1.Waiting for config after 4 fails

### udpx Android application steps:

| 1. Config Tab | 2. Select your device and pair |
| ------------- | -------------------------------|
| ![step 1](/examples/udpx_android/screen-1.png) | ![step 2](/examples/udpx_android/screen-2.png) |

| 3. Add your AP credentials and Send config | 4. Go to antenna tab and click to set the IP |
| ------------------------------------------ | -------------------------------------------- |
| ![step 3](/examples/udpx_android/screen-3.png) | ![step 5](/examples/udpx_android/screen-4.png) |

| 5. Done! Ready to receive UDP |
| ----------------------------- |
| ![step 6](/examples/udpx_android/screen-5.png) |

**Credits are due:** All logic doing the Firmware part is from [Bernd Giesecke](https://desire.giesecke.tk), since I followed his great example on ESP32WiFIBLE Android app, to make this configurable per Bluetooth serial. I implemented in a way that both [udpx Android](https://play.google.com/store/apps/details?id=io.cordova.udpx) and [ESP32 WiFi BLE](https://play.google.com/store/apps/details?id=tk.giesecke.esp32wifible) as a WiFi configuration mean are supported. 

### udpx Chrome App

In case you want to send test examples from a desktop computer try out our Chrome app in this branch. To install the it:

1. chrome://extensions -> Enable developer mode
2. Load unpacked extension and point to examples/chromeapp directory
3. That's it, you can open it on:
    chrome://apps

Please note that this app will be discontinued and not supported anymore starting on February 2020 since the preferred way to test this is the Android app. Zlib is not supported on Chrome App and it's very hard to port, so we will leave the existing code just as a proof-of-concept on how to send UDP packets to udpx.

### TEAM

UDPX is a collaborative effort where a team of 3 have same access level to the repository:

[Hendrik Putzek](https://github.com/hputzek)   - Front end / Nodejs, VUE

[Samuel Archibald](https://github.com/IoTPanic) - C++ / GO Api backend

[Martin Fasani](https://github.com/martinberlin) - Firmware and testing

## Additional libraries

**Pixels** is a binary transport protocol. A way to send bytes to an ESP32 that are after arrival decoded and send to a RGB / RGBw Led strip. 
 
**S or "Little Stream"** is an an embedded streaming library for embedded devices. Is simple data transport layer that is meant to be used in the udpx project that both is small as possible, and made for real-time applications, which has the ability to be compressed. Both protocols where developed by Samuel and are used by UDPX.

## Branches

**main** Supports Brotli compression when the first byte is not 'p' (80), is the only branch that has BTSerial configuration

**testing/testing-s** Implementing here the Pixels+S Protocol (Aka "Little stream" https://github.com/IoTPanic/s)

**feature/tcp** A TCP experiment since Martin had to do something for a client that wanted to use HTTP, interesting to see how fast is UDP compared to TCP, just left there for research reasons.

**feature/json-v0.1** Only stored for historical reasons, it our first JSON + Brotli compressed protocol, that was tested and even though the limitations [proved to be quite fast](https://twitter.com/martinfasani/status/1166106095858966529).

## Branching model

Please adhere to following branching model when working in this repository. Not strictly, just to maintain a certain order and avoid loosing time. 

    master  → Main stable branch (clean/no debugging)
      ↑↓
    develop → Where features are merged only *after testing* (serial debug is ok)
      ↓
    feature/1 → Merged back in develop *only* after testing 
    bug/1     → Merged back in develop
    

    Please except of the existing feature branches do not leave stale branches unless there is a real need to have them. Branches that diverge from master/develop and are stale for weeks will be deleted.

Testing should pass when examples/test is open in the browser and two different ESP32 with RGB /RGBW stripe outputs match the expected results.

### Experimental branches

**feature/multichannel-chunk** is using a experimental branch of pixels library that was recently merged in master. 
There the Neopixel out pins are defined like: 
const char pixelpin[] = { 19, 18 };

It's basically receiving 968 RGB pixels and sending the first chunk in channel 19 and the second 484 to channel 18. That sends them in parallel using 2 RMT channels, reducing the time serial communication is sent in a half and makes it gain framerate speed.  
Please check [Pixels library](https://github.com/martinberlin/udpx-app) to understand how send the right headers.

## Important configuration

    /lib/Config/Config.h

Mandatory only if you don't use WIFI_BLE configuration:
WIFI_SSID / WIFI_PASS 

UDP_PORT defaults to 1234


    .pio/libdeps/BOARD/PIXELS/src/pixels.h

The number of leds in the stripe or Led matrix, note that if you sent more than PIXELCOUNT it will simply not render it: 

#define PIXELCOUNT 1000

The data pin(s) of the addressable leds:

const char pixelpin[] = { 19, 18 };

// Uncomment if we sent per chunks in different channels
#define PIXELCHUNK 500 
If this is set, then there are 2 header bytes (16 bit unsigned) that will signalize it too, and then first 500 pixels will go to pixelpin[0] and the rest will go to pixelpin[1]. Note that this is experimental and at the moment is only working for two chunks.

Uncommenting this line will enable RGBW mode sending 4 bytes per pixel. It should be commented if you use RGB:

    #define RGBW

using RGB:

    //#define RGBW 
    // It defaults to this. We are using R,G,B on our led matrix part of the firmware since with White needs 4 bytes per pixel

**Note:** Switching between RGB to RGBW enforces you to update that line but also the [Neopixel class features](https://github.com/Makuna/NeoPixelBus/wiki/NeoPixelBus-object#neo-features) depending on the addressable leds you use (/lib/src/pixels/src/pixels.cpp) Please refer to the Neopixels wiki on the right way to do this correctly for your Led stripe.

### Limitations

This Firmware is limited by the MTU of the ESP32 that is 1470 bytes. If it receives a payload bigger than that the udp.onPacket callback will never take place. So in our experience we can send maximum about 1000 RGB pixels using PIX565 Zlib at a decent framerate.

### Hardware

This Firmware can work to stream pixel frame-per-frame animations to any Led stripe supported by the underlying library Neopixels. Any type of addresable LEDs should work if you select the right implementation in .libdeps/PIXELS
Please make sure to check our [Hardware buying guide](https://github.com/martinberlin/udpx/wiki/Hardware-buying-guide) if your are planning to stream video. Check the [Neopixels documentation](https://github.com/Makuna/NeoPixelBus/wiki/NeoPixelBus-object#neo-features) to select the right implementation to your LED Stripe.

### History

**2020-01** Implemented PIX565 Zlib compressed. The limit is now about 1000 Pixels using this protocol since it uses 2 bytes per pixel and Zlib compresses each frame, making it the fastest choice to send data so far

**2019-12** Martin converted his Chrome example into an Android cordova hibryd application. It features WiFi configuration, multicast DNS discovery and picture/video streaming

**2019-11** Martin worked out first a very raw Chrome App to send udp directly without middleware. This experiment lead to make a full pfledged [Android Application](http://udpx.fasani.de) in Dec. '19

**2019-10** Implementing and testing S library

**2019-09** Tests and fixes to achieve a stable RGB, RGBW version

**2019-08** [Samuel Archibald](https://twitter.com/IoTPanic) Made his first contribution and started a new protocol to get rid of JSON and send this even faster

**2019-07** Decided to name the Firmware UDPX and make it open-source to let another developers join forces and use it for their own needs

**2019-06** Started working with Front-end developer [Hendrik Putzek](https://twitter.com/hputzek) to develop a firmware solution to transport a lot of frames per seconds from his Nodejs backend to a Led controller

### License

This repository is licensed with a ["CC Attribution-NonCommercial"](https://creativecommons.org/licenses/by-nc/4.0/legalcode) License.

It contains code and examples that can be used and copied freely for artistic works and it can be also used for commercial projects provided you write as a line to explain how is going to be used and our license and credits are maintained.
We put really a lot of time and effort building this Firmware and we would like to give proffesional support to integrate it in the future.

### Companion Firmware

[Remora](https://github.com/martinberlin/Remora) Listens short UDP commands and triggers fast animations in ESP32/ESP8266. It supports udpx protocol (non compressed). So it can receive and render up to 484 RGB pixels as an animation frame.

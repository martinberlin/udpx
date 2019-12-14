![udpx Logo](/examples/udpix-logo.png)

**udpx is a technology to transport data over WiFi to microcontrollers**

### How to test this

**Android App latest builds**
If you want to test this fast using Android, just download and install the udpx app from Play store: 
<a href="https://play.google.com/store/apps/details?id=io.cordova.udpx" />
<img src="/examples/udpx-app-180x120.jpg" />
[udpx esp32](https://play.google.com/store/apps/details?id=io.cordova.udpx)

The [udpx Android App](https://github.com/martinberlin/udpx-app) is also open source. Feel free to explore and learn Cordova to make hybrid apps.

**Little stream protocol tester and nodejs libraries**
https://github.com/hputzek/little-stream-protocol This will let you experience much better Framerate and it also supports S as a protocol with compression.

### Configuring WiFi using Bluetooth

Starting with version 1.1 defined in lib/Config.h the firmware supports receiving configuration over Bluetooth Serial. It waits BLE_SECONDS_WAIT_FOR_CONFIG on every start. If you want to make the start faster just set this to a low number.

1.- Firmware boots, opens Bluetooth serial and waits for 9 seconds as default. Serial output:  

    UDPX 1.1
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

**develop** Main branch only with Pixels protocol. 
DEMO: https://www.youtube.com/watch?v=6ybJ6rIGSAo

**testing/s** Implementing here the Pixels+S Protocol (Aka "Little stream" https://github.com/IoTPanic/s)

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
    

    Please except of PIXELS+s no stale branches unless there is a real need to have them. 

Testing should pass when examples/test is open in the browser and two different ESP32 with RGB /RGBW stripe outputs match the expected results.

## Important configuration

    /lib/Config/Config.h

Mandatory:
WIFI_SSID / WIFI_PASS 

UDP_PORT

MQTT_ENABLE is set to false by default. Only required if you need to register the ESP32 in an external controller.

    .pio/libdeps/BOARD/PIXELS/src/pixels.h

The number of leds in the stripe: 

#define PIXELCOUNT 72

The data pin of the addressable leds:

#define PIXELPIN 19

Uncommenting this line will enable RGBW mode sending 4 bytes per pixel. It should be commented if you use RGB:

    #define RGBW

using RGB:

    //#define RGBW

**Note:** Using a RGB / RGBW enforces you to update that line but also the [Neopixel class features](https://github.com/Makuna/NeoPixelBus/wiki/NeoPixelBus-object#neo-features) depending on the addressable leds you use (/lib/src/pixels/src/pixels.cpp)

### History

**2019-12** Martin converted his Chrome App example into a Android cordova hibryd app that is running only on Android platform for now.

**2019-11** Martin worked out first a very raw Chrome App to send udp directly without middleware. This experiment lead to make a full pfledged [Android Application](http://udpx.fasani.de)

**2019-06** Started working with Front-end developer [Hendrik Putzek](https://twitter.com/hputzek) to develop a firmware solution to transport a lot of frames per seconds from his Nodejs backend to a Led controller

**2019-07** Decided to name the Firmware UDPX and make it open-source to let another developers join forces and use it for their own needs

**2019-08** [Samuel Archibald](https://twitter.com/IoTPanic) Made his first contribution and started a new protocol to get rid of JSON and send this even faster

**2019-09** Tests and fixes to achieve a stable RGB, RGBW version

**2019-10** Implementing and testing S library

#### Old command line test

Use it only to check that compilation went right. For real tests please use the examples/test browser POST -> middleware.js -> UDP to ESP32

Command line test:

    cat examples/72-pix.json.br |nc -w1 -u ESP32_IP_ADDRESS 1234

Should turn on some Leds provided netcat is installed on your system.

    cat examples/72-pix-off.json.br |nc -w1 -u ESP32_IP_ADDRESS 1234

Should turn off all leds in the 72 addressable led stripe. Use the 144-pix version to try this on 144 leds.

### License

This repository is licensed with a ["CC Attribution-NonCommercial"](https://creativecommons.org/licenses/by-nc/4.0/legalcode) License.

It contains code and examples that can be used and copied freely for artistic works and it can be also used for commercial projects provided you write as a line to explain how is going to be used and our license and credits are maintained.
We put really a lot of time and effort building this Firmware and we would like to give proffesional support to integrate it in the future.

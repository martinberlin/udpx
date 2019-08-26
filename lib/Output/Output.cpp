#ifndef neopixelbus_h
#include <NeoPixelBus.h>
#endif
#include "Output.h"

#define PIXELCOUNT 144
#define PIXELPIN 19
#ifdef ESP32
  NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PIXELCOUNT, PIXELPIN);
  // Four element pixels, RGBW
  //NeoPixelBus<NeoRgbwFeature, NeoEsp32I2s1800KbpsMethod> strip(144, 19);
#else
  NeoPixelBus<NeoGrbwFeature, NeoEsp8266Dma800KbpsMethod> strip(PIXELCOUNT, 3);
#endif
RgbwColor red(RgbColor(255, 0, 0));
Output::Output() {
}

void Output::setup() {
    // this resets all the neopixels to an off state
    strip.Begin();
    strip.Show();
}

void Output::setPixels(JsonArray *pixels) {
  // iterate over all received pixels
    int i = 0;
    for (JsonArray pixel : *pixels)
    {
         strip.SetPixelColor(i, RgbColor(pixel[0],pixel[1],pixel[2]));
         i++;
    }
    strip.Show();
}

#include <pixels.h>


// This block will select if the ESP32 or ESP8266 is selected. It is in the cpp file to stay private,
// if I were coding this library at work, I would do this, but it could easily be placed in the header
// file if you want to interact with it in main.cpp
#ifdef ESP32
    // This section selects whether we want to initialize the Neopixel lib in RGB or RGBW. Since I made
    /// a class, I can make it so you can hot select RGB or RGBW. I just don't see a use for that. ( I 
    // looked into putting it into class, we can if we use a different lib. This one is pretty inflexable
    // after compile time. But idc rn)
    #ifdef RGBW
        NeoPixelBus<NeoRgbwFeature, NeoEsp32I2s1800KbpsMethod> strip(PIXELCOUNT, 19);
    #else
        NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PIXELCOUNT, PIXELPIN);
    #endif
#elif ESP8266
    #ifndef RGBW
        NeoPixelBus<NeoRgbwFeature, NeoEsp8266Dma800KbpsMethod> strip(PIXELCOUNT, 3);
    #else
        NeoPixelBus<NeoGrbwFeature, NeoEsp8266Dma800KbpsMethod> strip(PIXELCOUNT, 3);
    #endif
#else
    // Place for someone who wants to support another platform to get started easily
    NeoPixelBus<, > strip(PIXELCOUNT, 3);
    #error This was written for ESPs, if you would like to use something else, define it above
#endif

PIXELS::PIXELS(){} // I'll do something with this, I swear.

void PIXELS::init(){
    // Starts the LEDs and blanks them out
    strip.Begin();
    strip.Show();
}

bool PIXELS::receive(uint8_t *pyld, unsigned length){
    uint16_t pixCnt = 0;
    pixel *pattern = unmarshal(pyld, length, &pixCnt);
    if(pixCnt==0){
        Serial.println("Returning from failed arshal");
        return false;
    }
    /*
    for(uint i=0; i<pixCnt; i++){
        Serial.print("Got LED value RGB(");
        Serial.print(pattern[i].R);
        Serial.print(",");
        Serial.print(pattern[i].G);
        Serial.print(",");
        Serial.print(pattern[i].B);
        Serial.println(")");
    }
    */
    this->show(pattern, pixCnt);
    delete pattern;
    return true;
}

void PIXELS::write(unsigned location, uint8_t R, uint8_t G, uint8_t B, uint8_t W){
    #ifdef RGBW
    strip.setPixelColor(location, RgbwColor(R,G,B,W));
    #else
    strip.SetPixelColor(location, RgbColor(R,G,B));
    #endif
}

void PIXELS::show(){
    strip.Show();
}

void PIXELS::show(pixel *pixels, unsigned cnt){
    for(unsigned i = 0; i<cnt; i++){
        #ifdef RGBW
        strip.setPixelColor(i, RgbwColor(pixels[i].R,pixels[i].G,pixels[i].B,pixels[i].W));
        #else
        strip.SetPixelColor(i, RgbColor(pixels[i].R,pixels[i].G,pixels[i].B));
        #endif
    }
    strip.Show();
}

pixel *PIXELS::unmarshal(uint8_t *pyld, unsigned len, uint16_t *pixCnt, uint8_t *channel){
    if(pyld[0]!=0x50){
        Serial.println("Missing checkvalue");
        // Set pixCnt to zero as we have not decoded any pixels and return NULL
        *pixCnt = 0;
        return NULL;
    }
    //if (pyld[1]!=syncWord||syncWord==0x0){
    //    *pixCnt = 0;
    //    return NULL;
    //} TODO UNCOMMENT REMOVED FOR EASY NETCAT USAGE QUICKLY
    if(channel!=NULL){
        *channel = pyld[2];
    }
    // Decode number of pixels, we don't have to send the entire strip if we don't want to
    uint16_t cnt = pyld[3] | pyld[4]<<8;
    if(cnt>PIXELCOUNT){
        // We got more pixels than the strip allows
        *pixCnt = 0;
        return NULL;
    }
    if (cnt ==0)
    {
        return false;
    }
    pixel *result = new pixel[cnt];
    // TODO Add logic to return if len is impossibly large or small
    for(uint16_t i = 0; i<cnt; i++){
        #ifdef RGBW
        result[i].R = pyld[5+(i*4)];
        result[i].G = pyld[5+(i*4)+1];
        result[i].B = pyld[5+(i*4)+2];
        result[i].W = pyld[5+(i*4)+3]
        #else
        result[i].R = pyld[5+(i*3)];
        result[i].G = pyld[5+(i*3)+1];
        result[i].B = pyld[5+(i*3)+2];
        #endif
    }


    // TODO Add CRC check before setting pixCnt
    *pixCnt = cnt;
    return result; 
}


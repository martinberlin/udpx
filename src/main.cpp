#include <WiFi.h>
#include <AsyncUDP.h>
#include <Config.h>
#include <pixels.h>
#include <brotli/decode.h>
extern "C" {
	#include "freertos/FreeRTOS.h"
	#include "freertos/timers.h"
}
#include <SimpleTimer.h>

#define BROTLI_DECOMPRESSION_BUFFER 6000
TaskHandle_t brotliTask;
size_t receivedLength;
TimerHandle_t wifiReconnectTimer;
unsigned long frameCounter = 0;
unsigned long frameLastCounter = frameCounter;
long decompressionFailed = 0;
// Debug mode prints to serial
bool debugMode = DEBUG_MODE;

// Message transport protocol
AsyncUDP udp;
SimpleTimer timer;
// Output class pixels
PIXELS pix;

typedef struct {
  unsigned size;
  uint8_t *pyld;
}taskParams;

/**
 * Generic message printer. Modify this if you want to send this messages elsewhere (Display)
 */
void printMessage(String message, bool newline = true)
{
  if (debugMode) {
    if (newline) {
      Serial.println(message);
    } else {
      Serial.print(message);
    }
   }
}

void timerCallback(){
  if (frameLastCounter != frameCounter) {
    Serial.printf("FPS: %d Frames received: %d\n", frameCounter-frameLastCounter, frameCounter);
    frameLastCounter = frameCounter;
  } 
}

// Task sent to the core to decompress + push to Output
void brTask(void * compressed){  
  
  uint8_t *brOutBuffer = new uint8_t[BROTLI_DECOMPRESSION_BUFFER];
  size_t bufferLength = BROTLI_DECOMPRESSION_BUFFER;
  BrotliDecoderResult brotli;

  int initMs = micros();

  brotli = BrotliDecoderDecompress(
    receivedLength,
    (const uint8_t *)compressed,
    &bufferLength,
    brOutBuffer);

  int brotliMs = micros()-initMs;
  
    if (brotli) {
      frameCounter++;      
      // Send the decompressed buffer to Pixel lib
      int neoMs = micros();
      pix.receive(brOutBuffer, bufferLength);

    if (debugMode) {
        Serial.printf("Neopixels:%d Brotli:%d Total:%d us\n", micros()-neoMs, brotliMs, micros()-initMs);
        Serial.printf("Decompressed %d bytes for frame %d Heap %d\n", bufferLength, frameCounter, ESP.getFreeHeap());
      }
    } else {
      decompressionFailed++;
      Serial.printf("Decompression failed %d times after frame: %d\n", decompressionFailed, frameCounter);
    }
    
    delete brOutBuffer;
    vTaskDelete(NULL);
}

void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

/**
 * Convert the IP to string 
 */
String IpAddress2String(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") +\
  String(ipAddress[1]) + String(".") +\
  String(ipAddress[2]) + String(".") +\
  String(ipAddress[3]);
}

void WiFiEvent(WiFiEvent_t event) {
    Serial.printf("[WiFi-event] event: %d\n", event);
    switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.println("WiFi connected: ");Serial.println(WiFi.localIP());

      if(udp.listen(UDP_PORT)) {
        Serial.println("UDP Listening on IP: ");
        Serial.print(IpAddress2String(WiFi.localIP())+":");
        Serial.println(UDP_PORT);

      // Interval to measure FPS  (millis, function called, times invoked for 1000ms around 1 hr and half)
      timer.setTimer(1000, timerCallback, 6000);

    // Executes on UDP receive
    udp.onPacket([](AsyncUDPPacket packet) {
      
        if ((int) packet.data()[0] == 80) {
        // Non compressed
          pix.receive(packet.data(), packet.length());
        } else {
          receivedLength = packet.length();

          xTaskCreatePinnedToCore(
                    brTask,        
                    "uncompress", 
                    10000,         
                    packet.data(),   
                    9,            
                    &brotliTask,  
                    0);           
          delay(1);  
        }

        }); 
 
    } else {
      Serial.println("UDP Lister could not be started");
    }
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        Serial.println("WiFi lost connection");
        
	    	xTimerStart(wifiReconnectTimer, 0);
        break;
        // Non used, just there to avoid warnings
        case SYSTEM_EVENT_WIFI_READY:
        case SYSTEM_EVENT_SCAN_DONE:
        case SYSTEM_EVENT_STA_START:
        case SYSTEM_EVENT_STA_STOP:
        case SYSTEM_EVENT_STA_CONNECTED:
        case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
        case SYSTEM_EVENT_STA_LOST_IP:
        case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
        case SYSTEM_EVENT_STA_WPS_ER_FAILED:
        case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
        case SYSTEM_EVENT_STA_WPS_ER_PIN:
        case SYSTEM_EVENT_AP_START:
        case SYSTEM_EVENT_AP_STOP:
        case SYSTEM_EVENT_AP_STACONNECTED:
        case SYSTEM_EVENT_AP_STADISCONNECTED:
        case SYSTEM_EVENT_AP_STAIPASSIGNED:
        case SYSTEM_EVENT_AP_PROBEREQRECVED:
        case SYSTEM_EVENT_GOT_IP6:
        case SYSTEM_EVENT_ETH_START:
        case SYSTEM_EVENT_ETH_STOP:
        case SYSTEM_EVENT_ETH_CONNECTED:
        case SYSTEM_EVENT_ETH_DISCONNECTED:
        case SYSTEM_EVENT_ETH_GOT_IP:
        case SYSTEM_EVENT_MAX:
        break;
    }
}

void setup()
{
  Serial.begin(115200);
  pix.init();
  
  Serial.printf("UDPX %s\n", UDPX_VERSION); 

  // Set up automatic reconnect timers
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));
  connectToWifi();

  WiFi.onEvent(WiFiEvent);

}

void loop() {
  timer.run();
}
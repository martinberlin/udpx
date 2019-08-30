#include <WiFi.h>
#include "AsyncUDP.h"
#include <Config.h>
#include <FS.h>
#include "SPIFFS.h"
#include <ArduinoJson.h>
#include <pixels.h>
extern "C" {
	#include "freertos/FreeRTOS.h"
	#include "freertos/timers.h"
}

TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;
String lastWill;
// Note DynamicJsonDocument should be created once otherwise eats your HEAP memory like cookies
const size_t capacity = 144 * JSON_ARRAY_SIZE(4) + JSON_ARRAY_SIZE(144);
DynamicJsonDocument doc(capacity);
char bssid[9];

// Debug mode prints to serial
bool debugMode = DEBUG_MODE;
// Turn on debug to enjoy seeing how ArduinoJson eats your heap memory

// Message transport protocol
AsyncUDP udp;

// Brotli decompression buffer
uint8_t * compressed;
TaskHandle_t brotliTask;
size_t receivedLength;
// Output class and Mqtt message buffer
PIXELS pix;
String payloadBuffer;
// SPIFFS to read presentation
File fsFile;
String presentation;

struct config {
  char title[140];
  bool compression = true;
} internalConfig;


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
/**
 * if (error)
        {
          printMessage("After uncompressing json: ",false);printMessage(error.c_str());
        } else { }

 Note this brTask is stripped to the bone to make it as fast as possible
 TODO: Research how to pass an array in notused Parameter (compressed data)
 */
// Task sent to the core to decompress + push to Output
void brTask(void * input){
  AsyncUDPPacket *p = static_cast<AsyncUDPPacket*>(input);  
  pix.receive(p->data(), p->length());

  if (debugMode) {
    Serial.println(" Heap: "+String(ESP.getFreeHeap())); 
  }
  vTaskDelete(NULL);
  // https://www.freertos.org/implementing-a-FreeRTOS-task.html
  // If it is necessary for a task to exit then have the task call vTaskDelete( NULL )
}

void WiFiEvent(WiFiEvent_t event) {
    Serial.printf("[WiFi-event] event: %d\n", event);
    switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.println("WiFi connected: ");Serial.println(WiFi.localIP());

      // Read presentation template from FS json only if MQTT_ENABLE is true
      if (MQTT_ENABLE) {
      if (SPIFFS.begin())
      {
        printMessage("SPIFFS started. Checking for presentation file: ", false);
        printMessage(FIRMWARE_PRESENTATION);
        if (SPIFFS.exists(FIRMWARE_PRESENTATION))
        {
          printMessage("Presentation file exists. Size in bytes: ", false);

          File configFile = SPIFFS.open(FIRMWARE_PRESENTATION, FILE_READ);
          presentation = configFile.readString();
          presentation.replace("<chipid>", bssid);
          presentation.replace("<udp_ip>", IpAddress2String(WiFi.localIP()));
          if (configFile)
          {
            size_t size = configFile.size();
            printMessage(String(size));
            DynamicJsonDocument doc(capacity);
            // Deserialize the JSON document
            DeserializationError error = deserializeJson(doc, presentation);
            if (error)
            {
              printMessage("ERR parsing presentation file");
              printMessage(error.c_str());
              return;
            }
            
            // TODO: Check if this properties exist in JSON otherwise die here with a proper message
            strcpy(internalConfig.title, doc["title"]);
            internalConfig.compression = doc["compression"];
            printMessage("Controller title: " + String(internalConfig.title));
            printMessage("=============== Presentation ready ===============");
            printMessage(presentation);
          }
          else
          {
            printMessage("ERR load config");
          }
          configFile.close();
        }
        else
        {
          printMessage("Config file not found");
        }
      }
      else
      {
        printMessage("SPIFFs cannot start. FFS Formatted?");
      }

      } else {
        Serial.println("MQTT_ENABLE is disabled per Config. Spiffs presentation read skipped");
      }

      if(udp.listen(UDP_PORT)) {
        Serial.println("UDP Listening on IP: ");
        Serial.print(IpAddress2String(WiFi.localIP())+":");
        Serial.println(UDP_PORT);

    // Executes on UDP receive
    udp.onPacket([](AsyncUDPPacket packet) {
        //printMessage("UDP packet from: ",false);printMessage(String(packet.remoteIP()));
        printMessage("Len: ", false);
        printMessage(String(packet.length()), false);
        unsigned long t = micros();
        pix.receive(packet.data(), packet.length());
        Serial.print("Took ");
        Serial.print(micros()-t);
        Serial.println("micro seconds to consume");
        delay(0);
        }); 
    } else {
      Serial.println("UDP Lister could not be started");
    }
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        Serial.println("WiFi lost connection");
        // Ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
        xTimerStop(mqttReconnectTimer, 0);
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
  // Set chip id in hex format
  itoa(ESP.getEfuseMac(), bssid, 16);
  strcpy(internalConfig.title, bssid);
  
  // Set up automatic reconnect timers
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));
  connectToWifi();

  WiFi.onEvent(WiFiEvent);
  // Pixels output
  pix.init();
}

void loop() {
  delay(0);
}
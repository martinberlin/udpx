#include <WiFi.h>
#include "AsyncUDP.h"
#include <Config.h>
#include <FS.h>
#include "SPIFFS.h"
#include <ArduinoJson.h>
#include <pixels.h>
#include <ESPmDNS.h>
#include "ESPAsyncWebServer.h"

extern "C" {
	#include "freertos/FreeRTOS.h"
	#include "freertos/timers.h"
}

AsyncWebServer server(SERVER_PORT);
// POST buffer
#define POST_BUFFER 3000

TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;
String lastWill;
// Note DynamicJsonDocument should be created once otherwise eats your HEAP memory like cookies
const size_t capacity = 144 * JSON_ARRAY_SIZE(4) + JSON_ARRAY_SIZE(144);
DynamicJsonDocument doc(capacity);
char bssid[9];

// Debug mode prints to serial
bool debugMode = DEBUG_MODE;
long start;
// Turn on debug to enjoy seeing how ArduinoJson eats your heap memory

// Message transport protocol
AsyncUDP udp;

size_t receivedLength;
// Output class and Mqtt message buffer
PIXELS pix;
String payloadBuffer;

long processedPosts = 0;

struct config {
  char title[140];
  bool compression = true;
} internalConfig;


typedef struct {
  unsigned size;
  uint8_t *pyld;
}taskParams;

uint8_t *collect; 
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

uint8_t *postBuffer;

void startWebserver()
{

  server.on(
      "/post",
      HTTP_POST,
      [](AsyncWebServerRequest *request) {},
      NULL,
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        printMessage("Len:" + String(len) + " Index:" + String(index) + " Total:" + String(total));

        if (!index)
        {
          postBuffer = new uint8_t[POST_BUFFER];
          processedPosts++;
          start = millis();
        }

        if (DEBUG_MODE)
        {
          printMessage("HEAP:" + String(ESP.getFreeHeap()) + " Job:" + String(processedPosts));
          /* for (size_t i = 0; i < len; i++)
          {
            Serial.print(postBuffer[i], HEX); Serial.print(' ');
          } */
        }
        // Acumular el Post en un buffer ya que esta funcion se llama en chunks de alrededor 1K
        for (size_t i = 0; i < len; i++)
        {
          postBuffer[index + i] = data[i];
        }

        if (index + len == total)
        {
          // Send to pix class
          pix.receive(postBuffer, total);
          delete (postBuffer);
          Serial.println();
          // Build response with CORs
          AsyncWebServerResponse *response = request->beginResponse(200, "application/json",
                                                                    "{\"status\":1,\"bytes\": " + String(total) + ",\"millis\": " + String(millis() - start) + "}");
          response->addHeader("Access-Control-Allow-Origin", "*");
          request->send(response);
        }
      });

  server.begin();
  printMessage("WebServer started");
}

void WiFiEvent(WiFiEvent_t event) {
    Serial.printf("[WiFi-event] event: %d\n", event);
    switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.println("WiFi connected: ");Serial.println(WiFi.localIP());
        if (!MDNS.begin("p")) {
          while(1) { 
          delay(100);
          }
        }
        MDNS.addService("http", "tcp", 80);
        printMessage("p.local mDns started");
        // Start httpServer
        startWebserver();

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
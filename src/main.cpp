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
#include <AsyncMqttClient.h>

AsyncMqttClient mqttClient;
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

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
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
  taskParams *p = static_cast<taskParams*>(input);   
  uint16_t ptr = (uint16_t)p->pyld;
  for(unsigned i =0; i<p->size; i++){
    Serial.print(p->pyld[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  pix.receive(p->pyld, p->size);
  Serial.println("Here 1");
  Serial.print("POINTER VALUE - ");
  Serial.println(ptr, HEX);
  delete p->pyld;
  Serial.println("Here 2");
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
    
        connectToMqtt(); 
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
        /* printMessage(" Data: ",false);
        if (debugMode) {
          Serial.write(packet.data(), packet.length());
        } */
        
        // Save compressed in memory instead of simply: uint8_t compressed[compressedBytes.size()];
        receivedLength = packet.length();

         uint8_t *buffer  = new uint8_t[receivedLength];
        
        for ( int i = 0; i < packet.length(); i++ ) {
            buffer[i] = packet.data()[i]; // Can be shortened to this right?
        }

        taskParams params = {receivedLength, buffer};

        uint16_t ptr = (uint16_t)buffer;
        Serial.print("POINTER VALUE - ");
        Serial.println(ptr, HEX);

          xTaskCreatePinnedToCore(
                    brTask,        /* Task function. */
                    "uncompress",  /* name of task. */
                    20000,         /* Stack size of task */
                    &params,          /* parameter of the task */
                    9,             /* priority of the task */
                    &brotliTask,   /* Task handle to keep track of created task */
                    0);            /* pin task to core 1 */
        //reply to the client (We don't need to ACK now)
        //packet.printf("1");
        delay(10);
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

void onMqttConnect(bool sessionPresent) {
  printMessage("Connected to MQTT. Sending presentation to topic: ", false);
  printMessage(PRESENTATION_TOPIC);
  printMessage("Last will: ", false);
  printMessage(lastWill);
  mqttClient.publish(lastWill.c_str(), 1, true, "true");
  // TODO: Read presentation from JSON and publish to PRESENTATION_TOPIC
  mqttClient.publish(PRESENTATION_TOPIC, 1, true, presentation.c_str());
  
  // Subscribe to receive topic (TODO: Later this should be dynamic)
  printMessage("Subscribing to IN_TOPIC, packetId: ", false);
  uint16_t packetIdSub = mqttClient.subscribe(IN_TOPIC, 0);
  printMessage(String(packetIdSub));
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");

  if (WiFi.isConnected()) {
    xTimerStart(mqttReconnectTimer, 0);
  }
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.println("Subscribe acknowledged.");
  printMessage("  packetId: ");
  printMessage(String(packetId));
  printMessage("  qos: ");
  printMessage(String(qos));
}

void onMqttUnsubscribe(uint16_t packetId) {
  Serial.println("Unsubscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, unsigned int len, size_t index, size_t total) {
// TODO: This topic should be dynamic 
   if (0 == strcmp(topic, "pixelcrasher/pixel-in/4ae5b9/hsl"))
  {
    // Enable buffering of partial messages. TODO: Do this with char not with String!
    if (index==0) {               // at start
    payloadBuffer = "";
    }
    payloadBuffer += payload;
    if (! (index + len == total)) { // at end
      return;
    }  
  }
}

void onMqttPublish(uint16_t packetId) {
  Serial.println("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void setup()
{
  Serial.begin(115200);
  // Set chip id in hex format
  itoa(ESP.getEfuseMac(), bssid, 16);
  strcpy(internalConfig.title, bssid);
  
  // Set up automatic reconnect timers
  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));
  connectToWifi();

  WiFi.onEvent(WiFiEvent);
  // TODO: Update this with new define STATUS_TOPIC
  lastWill = "pixelcrasher/online-status/"+String(bssid);

  // keepalive period, broker sets device offline after this period
  mqttClient.setKeepAlive(15);
  mqttClient.setMaxTopicLength(2000);
  mqttClient.setWill(lastWill.c_str(), 1, true, "false");
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  // Pixels output
  pix.init();
}

void loop() {
  delay(0);
}
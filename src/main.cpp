// Firmware in master with WiFi Configuration over Bluetooth serial
#include <WiFi.h>
#include <AsyncUDP.h>
#include <Config.h>
#include <pixels.h>
#include <brotli/decode.h>
#include <SimpleTimer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <miniz.c>

#define BROTLI_DECOMPRESSION_BUFFER 4000
TaskHandle_t brotliTask;
uLong receivedLength;
TimerHandle_t wifiReconnectTimer;
unsigned long frameCounter = 0;
unsigned long frameLastCounter = frameCounter;
unsigned long decompressionFailed = 0;
unsigned long frameTimerCalls = 0;
// DEBUG_MODE is compiled now and cannot be changed on runtime (Check lib/Config)
char apName[] = "udpx-xxxxxxxxxxxx_1234"; // Please follow naming as SERVICE_PORT with an underscore
bool usePrimAP = true;
/** Flag if stored AP credentials are available */
bool hasCredentials = false;
/** Connection status */
volatile bool isConnected = false;
bool connStatusChanged = false;
uint8_t lostConnectionCount = 1;
/** SSIDs of local WiFi networks */
String ssidPrim;
String ssidSec;
/** Password for local WiFi network */
String pwPrim;
String pwSec;
// Message transport protocol
AsyncUDP udp;
SimpleTimer timer;
// Output class pixels
PIXELS pix;
uint8_t timerEachSec = 2;

typedef struct {
  unsigned size;
  uint8_t *pyld;
}taskParams;
Preferences preferences; 

String ipAddress2String(const IPAddress& ipAddress){
  return String(ipAddress[0]) + String(".") +\
  String(ipAddress[1]) + String(".") +\
  String(ipAddress[2]) + String(".") +\
  String(ipAddress[3]);
}
void showStatus(uint8_t R,uint8_t G,uint8_t B, int blinkMs) {
    pix.write(0, R, G, B);
    pix.show();
    delay(blinkMs);
    pix.write(0, 0, 0, 0);
    pix.show();
}

#ifdef WIFI_BLE
  #include <nvs.h>
  #include <nvs_flash.h>
  #include "BluetoothSerial.h"

  // SerialBT class
  BluetoothSerial SerialBT;

  /** Buffer for JSON string */
  StaticJsonDocument<200> jsonBuffer;
#endif

void timerCallback(){
  if (frameLastCounter != frameCounter) {
    frameTimerCalls++;
    Serial.printf("FPS %lu / %lu / AVG %lu\n", (frameCounter-frameLastCounter)/timerEachSec, 
	frameCounter, frameCounter/(frameTimerCalls*timerEachSec));
    frameLastCounter = frameCounter;
  } 
}

// Task sent to the core to decompress + push to Output
void brTask(void * compressed){  
  
  uint8_t *brOutBuffer = new uint8_t[BROTLI_DECOMPRESSION_BUFFER];
  size_t bufferLength = BROTLI_DECOMPRESSION_BUFFER;
  BrotliDecoderResult brotli;
  #ifdef DEBUG_MODE
    int initMs = micros();
  #endif
  brotli = BrotliDecoderDecompress(
    receivedLength,
    (const uint8_t *)compressed,
    &bufferLength,
    brOutBuffer);
	#ifdef DEBUG_MODE
  	  int brotliMs = micros()-initMs;
    #endif
    if (brotli) {
      frameCounter++;      
      // Send the decompressed buffer to Pixel lib
	  #ifdef DEBUG_MODE
        int neoMs = micros();
	  #endif
      pix.receive(brOutBuffer, bufferLength);

    #ifdef DEBUG_MODE
	 //Neopixels: %u Brotli: %lu Total: %u us
        Serial.printf("Neopixels: %lu Brotli: %d Total: %lu us\n", micros()-neoMs, brotliMs, micros()-initMs);
        Serial.printf("Decompressed %u bytes for frame %lu Heap %u\n", bufferLength, frameCounter, ESP.getFreeHeap());
    #endif
    } else {
      decompressionFailed++;
      Serial.printf("Decompression failed %lu times after frame: %lu\n", decompressionFailed, frameCounter);
    }
    
    delete brOutBuffer;
    vTaskDelete(NULL);
}

void deleteWifiCredentials() {
	Serial.println("Clearing saved WiFi credentials");
	preferences.begin("WiFiCred", false);
	preferences.clear();
	preferences.end();
	delay(500);
}

/** Callback for receiving IP address from AP */
void gotIP(system_event_id_t event) {
	#ifdef WIFI_BLE
      //SerialBT.disconnect(); // In some configurations does not find this method
	  SerialBT.end();   
	#endif

  if (isConnected) return;
	// Interval to measure FPS  (millis, function called, times invoked for 1000ms around 1 hr and half)
	timer.setTimer(timerEachSec*1000, timerCallback, 6000);

  	isConnected = true;
	connStatusChanged = true;

	if (!MDNS.begin(apName)) {
		Serial.println("Error setting up MDNS responder!");
    }
	Serial.println((String(apName)+".local is online"));
    MDNS.addService("http", "tcp", 80);


  if(udp.listen(UDP_PORT)) {
      Serial.println("UDP Listening on: ");
      Serial.print(WiFi.localIP());Serial.println(":"+String(UDP_PORT)); 
	  showStatus(0,50,0,1000); // Green, 1 second

    // Executes on UDP receive
    udp.onPacket([](AsyncUDPPacket packet) {
		receivedLength = packet.length();
		#ifdef DEBUG_MODE
		Serial.printf("First BYTE:%d b[0]+b[1]:%d of %lu bytes HEAP: %d\n", packet.data()[0],packet.data()[0]+packet.data()[1], receivedLength,ESP.getFreeHeap());
        #endif
		/* Pixels: Not compressed */
		/* if (packet.data()[0] = 80) {
			
			return;
		} */

		switch (packet.data()[0]+packet.data()[1])
		{
		case 80:
		{
			/* Pixels: Not compressed */
			pix.receive(packet.data(), packet.length());
			frameCounter++;
			break;
		}
		case 121:
		{
			/* Zlib: miniz */
			#ifdef DEBUG_MODE
		      int initMs = micros();
			#endif
			uint8_t *outBuffer = new uint8_t[BROTLI_DECOMPRESSION_BUFFER];
			uLong uncomp_len;
			
			int cmp_status = uncompress(
				outBuffer, 
				&uncomp_len, 
				(const unsigned char*)packet.data(), 
				packet.length());
			
			if (cmp_status == 0) {
				pix.receive(outBuffer, uncomp_len);
				frameCounter++;
				}	
			#ifdef DEBUG_MODE
				/* Serial.println("HEX decompression dump");
				for (size_t i = 0; i<=20; i++){
					Serial.print(outBuffer[i], HEX);
					Serial.print(" ");
				}
 				*/
				// status:
				// { MZ_OK = 0, MZ_STREAM_END = 1, MZ_NEED_DICT = 2, MZ_ERRNO = -1, MZ_STREAM_ERROR = -2, MZ_DATA_ERROR = -3, MZ_MEM_ERROR = -4, MZ_BUF_ERROR = -5, MZ_VERSION_ERROR = -6, MZ_PARAM_ERROR = -10000 };
				int uncompressMs = micros()-initMs;
				Serial.printf
				("\nStatus:%d Decompressing miniz in %d us. Received: %d bytes, uncomp_length:%lu HEAP:%d\n",
				cmp_status, uncompressMs, packet.length(), uncomp_len, ESP.getFreeHeap());
			#endif
			delete outBuffer;
		}
		break;
		case 99: /*  Command: c <4 to make sure is a short command message */
			if (packet.length()<4) {
				pix.all_off();
			}
		break;
		#ifdef WIFI_BLE
		case 114: /* Command: r */
			if (packet.length()<4) {
				deleteWifiCredentials();
			}
			break;
		#endif
		default:
        {
			xTaskCreatePinnedToCore(
                    brTask,        
                    "uncompressBR", 
                    10000,         
                    packet.data(),   
                    9,            
                    &brotliTask,  
                    0);
			break;
		}
		}

        }); 
 
    } else {
      Serial.println("UDP Lister could not be started");
    }
}

/** Callback for connection loss */
void lostCon(system_event_id_t event) {
	isConnected = false;
	connStatusChanged = true;

    Serial.printf("WiFi lost connection try %d to connect again\n", lostConnectionCount);
	// Avoid trying to connect forever if the user made a typo in password
	if (lostConnectionCount>4) {
		deleteWifiCredentials();
		ESP.restart();
	} else if (lostConnectionCount>1) {
		showStatus(50,0,0,1000); // Red
	}
	lostConnectionCount++;
	#ifdef WIFI_BLE
	  WiFi.begin(ssidPrim.c_str(), pwPrim.c_str());
	#else
      WiFi.begin(WIFI_SSID, WIFI_PASS);
	#endif
}
  /**
 * Create unique device name from MAC address
 **/
void createName() {
	uint8_t baseMac[6];
	// Get MAC address for WiFi station
	esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
	// Write unique name into apName
	sprintf(apName, "udpx-%02X%02X%02X%02X%02X%02X_%d", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5], UDP_PORT);
}
// Bluetooth Serial configuration
#ifdef WIFI_BLE
/**
 * initBTSerial
 * Initialize Bluetooth Serial
 * Start BLE server and service advertising
 * @return <code>bool</code>
 * 			true if success
 * 			false if error occured
 */
bool initBTSerial() {
		if (!SerialBT.begin(apName)) {
			Serial.println("Failed to start BTSerial");
			return false;
		}
		Serial.println("BTSerial active. Device name: " + String(apName));
		return true;
}

/**
	 scanWiFi
	 Scans for available networks 
	 and decides if a switch between
	 allowed networks makes sense
	 @return <code>bool</code>
	        True if at least one allowed network was found
*/
bool scanWiFi() {
	/** RSSI for primary network */
	int8_t rssiPrim = -1;
	/** RSSI for secondary network */
	int8_t rssiSec = -1;
	/** Result of this function */
	bool result = false;

	Serial.println("Start scanning for networks");

	WiFi.disconnect(true);
	WiFi.enableSTA(true);
	WiFi.mode(WIFI_STA);

	// Scan for AP
	int apNum = WiFi.scanNetworks(false,true,false,1000);
	if (apNum == 0) {
		Serial.println("Found no networks?");
		return false;
	}
	
	byte foundAP = 0;
	bool foundPrim = false;

	for (int index=0; index<apNum; index++) {
		String ssid = WiFi.SSID(index);
		Serial.println("Found AP: " + ssid + " RSSI: " + WiFi.RSSI(index));
		if (!strcmp((const char*) &ssid[0], (const char*) &ssidPrim[0])) {
			Serial.println("Found primary AP");
			foundAP++;
			foundPrim = true;
			rssiPrim = WiFi.RSSI(index);
		}
		if (!strcmp((const char*) &ssid[0], (const char*) &ssidSec[0])) {
			Serial.println("Found secondary AP");
			foundAP++;
			rssiSec = WiFi.RSSI(index);
		}
	}

	switch (foundAP) {
		case 0:
			result = false;
			break;
		case 1:
			if (foundPrim) {
				usePrimAP = true;
			} else {
				usePrimAP = false;
			}
			result = true;
			break;
		default:
			Serial.printf("RSSI Prim: %d Sec: %d\n", rssiPrim, rssiSec);
			if (rssiPrim > rssiSec) {
				usePrimAP = true; // RSSI of primary network is better
			} else {
				usePrimAP = false; // RSSI of secondary network is better
			}
			result = true;
			break;
	}
	return result;
}

/**
 * Start connection to AP
 */
void connectWiFi() {
	// Setup callback function for successful connection
	WiFi.onEvent(gotIP, SYSTEM_EVENT_STA_GOT_IP);
	// Setup callback function for lost connection
	WiFi.onEvent(lostCon, SYSTEM_EVENT_STA_DISCONNECTED);

	Serial.println();
	Serial.print("Start connection to ");
	if (usePrimAP) {
		Serial.println(ssidPrim);
		WiFi.begin(ssidPrim.c_str(), pwPrim.c_str());
	} else {
		Serial.println(ssidSec);
		WiFi.begin(ssidSec.c_str(), pwSec.c_str());
	}
}

/**
 * readBTSerial
 * read all data from BTSerial receive buffer
 * parse data for valid WiFi credentials
 */
void readBTSerial() {
	if (!SerialBT.available()) return;
	uint64_t startTimeOut = millis();
	String receivedData;
	int msgSize = 0;
	// Read RX buffer into String
	
	while (SerialBT.available() != 0) {
		receivedData += (char)SerialBT.read();
		msgSize++;
		// Check for timeout condition
		if ((millis()-startTimeOut) >= 5000) break;
	}
	SerialBT.flush();
	showStatus(0,0,50,200);
	Serial.println("Received message " + receivedData + " over Bluetooth");

	// Decode the message only if it comes encoded (Like ESP32 WIFI Ble does)
	if (receivedData[0] != '{') {
		int keyIndex = 0;
		for (int index = 0; index < receivedData.length(); index ++) {
			receivedData[index] = (char) receivedData[index] ^ (char) apName[keyIndex];
			keyIndex++;
			if (keyIndex >= strlen(apName)) keyIndex = 0;
		}
		Serial.println("Decoded message: " + receivedData); 
	}
	
	/** Json object for incoming data */
	auto error = deserializeJson(jsonBuffer, receivedData);
	if (!error)
	{
		if (jsonBuffer.containsKey("ssidPrim") &&
			jsonBuffer.containsKey("pwPrim") &&
			jsonBuffer.containsKey("ssidSec") &&
			jsonBuffer.containsKey("pwSec"))
		{
			ssidPrim = jsonBuffer["ssidPrim"].as<String>();
			pwPrim = jsonBuffer["pwPrim"].as<String>();
			ssidSec = jsonBuffer["ssidSec"].as<String>();
			pwSec = jsonBuffer["pwSec"].as<String>();

			Preferences preferences;
			preferences.begin("WiFiCred", false);
			preferences.putString("ssidPrim", ssidPrim);
			preferences.putString("ssidSec", ssidSec);
			preferences.putString("pwPrim", pwPrim);
			preferences.putString("pwSec", pwSec);
			preferences.putBool("valid", true);
			preferences.end();

			Serial.println("Received over bluetooth:");
			Serial.println("primary SSID: "+ssidPrim+" password: "+pwPrim);
			Serial.println("secondary SSID: "+ssidSec+" password: "+pwSec);
			connStatusChanged = true;
			hasCredentials = true;
			delay(500);

			if (!scanWiFi()) {
				Serial.println("Could not find any AP");
				delay(500);
				ESP.restart();
			} else {
				// If AP was found, start connection
				connectWiFi();
			}

		}
		else if (jsonBuffer.containsKey("erase"))
		{ // {"erase":"true"}
			Serial.println("Received erase command");
			Preferences preferences;
			preferences.begin("WiFiCred", false);
			preferences.clear();
			preferences.end();
			connStatusChanged = true;
			hasCredentials = false;
			ssidPrim = "";
			pwPrim = "";
			ssidSec = "";
			pwSec = "";

			int err;
			err=nvs_flash_init();
			Serial.println("nvs_flash_init: " + err);
			err=nvs_flash_erase();
			Serial.println("nvs_flash_erase: " + err);
		}
		else if (jsonBuffer.containsKey("getip"))
		{ // {"getip":"true"}
			Serial.println("getip");
			String wifiCredentials;
			jsonBuffer.clear();
			jsonBuffer["status"] = 1;
			jsonBuffer["ip"] = ipAddress2String(WiFi.localIP());
			jsonBuffer["port"] = UDP_PORT;
			serializeJson(jsonBuffer, wifiCredentials);
			Serial.println(wifiCredentials);
			Serial.println();
			if (SerialBT.available()) {
				SerialBT.print(wifiCredentials);
				SerialBT.flush();
			} else {
				Serial.println("Cannot send IP request: Serial Bluetooth is not available");
			}
			
		} else if (jsonBuffer.containsKey("reset")) {
			WiFi.disconnect();
			esp_restart();
		}
	} else {
		Serial.println("Received invalid JSON");
	}
	jsonBuffer.clear();
}
#endif

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

void setup()
{
  Serial.begin(115200);
  pix.init();

  Serial.printf("UDPX %s\n", UDPX_VERSION); 
  createName();

  #ifdef WIFI_BLE

	preferences.begin("WiFiCred", false);
    //preferences.clear(); // Uncomment to force delete preferences

	bool hasPref = preferences.getBool("valid", false);
	if (hasPref) {
		ssidPrim = preferences.getString("ssidPrim","");
		ssidSec = preferences.getString("ssidSec","");
		pwPrim = preferences.getString("pwPrim","");
		pwSec = preferences.getString("pwSec","");

		if (ssidPrim.equals("") 
				|| pwPrim.equals("")
				|| ssidSec.equals("")
				|| pwPrim.equals("")) {
			Serial.println("Found preferences but credentials are invalid");
		} else {
			Serial.println("Read from preferences:");
			Serial.println("primary SSID: "+ssidPrim+" password: "+pwPrim);
			Serial.println("secondary SSID: "+ssidSec+" password: "+pwSec);
			hasCredentials = true;
		}
	} else {
		Serial.println("Could not find preferences, need send data over BLE");
		showStatus(0,0,100,1000);
	}
	preferences.end();

	if (hasCredentials) {
		//TODO: Think if it will be worth to leave 2 APs config, or only 1 in master:
	    connectWiFi();
		// Check for available AP's. Uncomment if the plan is to use 2 APs and select the best one
	    /* if (!scanWiFi()) {
			Serial.println("Could not find any AP");
		} else {
			connectWiFi();
		} */
	} else {
		// Start Bluetooth serial
		initBTSerial();
	}
	#else
	WiFi.onEvent(gotIP, SYSTEM_EVENT_STA_GOT_IP);
    WiFi.onEvent(lostCon, SYSTEM_EVENT_STA_DISCONNECTED);
    Serial.printf("Connecting to Wi-Fi using WIFI_AP %s\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
  #endif

}

void loop() {
  timer.run();

  #ifdef WIFI_BLE
  if (!WiFi.isConnected()) {
    readBTSerial();
  }
  #endif
}

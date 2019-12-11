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

// tdefl_compressor contains all the state needed by the low-level compressor so it's a pretty big struct (~300k).
// This example makes it a global vs. putting it on the stack, of course in real-world usage you'll probably malloc() or new it.
tinfl_decompressor decompressor;
#define BROTLI_DECOMPRESSION_BUFFER 3000
TaskHandle_t brotliTask;
uLong receivedLength;
TimerHandle_t wifiReconnectTimer;
unsigned long frameCounter = 0;
unsigned long frameLastCounter = frameCounter;
long decompressionFailed = 0;
// Debug mode prints to serial
bool debugMode = DEBUG_MODE;
// Please follow naming as SERVICE_PORT with an underscore (Android App needs this)
char apName[] = "udpx-xxxxxxxxxxxx_1234";
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
    Serial.printf("FPS: %lu Frames received: %lu\n", frameCounter-frameLastCounter, frameCounter);
    frameLastCounter = frameCounter;
  } 
}

String miniz_last_error = "";

void minizTask(void *compressed) {
  Serial.printf("Decompressing gzip. Received: %d bytes\n", receivedLength);
  uint8_t *outBuffer = new uint8_t[BROTLI_DECOMPRESSION_BUFFER];
  uLong uncomp_len;
  
  int cmp_status = uncompress(
	  outBuffer, 
	  &uncomp_len, 
  	  (const unsigned char*)compressed, 
      receivedLength);

//enum { MZ_OK = 0, MZ_STREAM_END = 1, MZ_NEED_DICT = 2, MZ_ERRNO = -1, MZ_STREAM_ERROR = -2, MZ_DATA_ERROR = -3, MZ_MEM_ERROR = -4, MZ_BUF_ERROR = -5, MZ_VERSION_ERROR = -6, MZ_PARAM_ERROR = -10000 };

  Serial.printf("MZ_OK=0 MZ_status:%d out_length:%lu uncomp_length:%lu\n",
  cmp_status,uncomp_len,uncomp_len);

  if (false) {
	for (size_t i = 0; uncomp_len < 8; i++)
	{
		Serial.print(outBuffer[i], HEX);
		Serial.print(" ");
	}
	}
	delete outBuffer;
    vTaskDelete(NULL);
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
        Serial.printf("Neopixels: %u Brotli: %lu Total: %u us\n", micros()-neoMs, brotliMs, micros()-initMs);
        Serial.printf("Decompressed %u bytes for frame %lu Heap %u\n", bufferLength, frameCounter, ESP.getFreeHeap());
      }
    } else {
      decompressionFailed++;
      Serial.printf("Decompression failed %lu times after frame: %lu\n", decompressionFailed, frameCounter);
    }
    
    delete brOutBuffer;
    vTaskDelete(NULL);
}
/** Callback for receiving IP address from AP */
void gotIP(system_event_id_t event) {
	#ifdef WIFI_BLE
	  SerialBT.end();delay(50);
	#endif

  if (isConnected) return;
	// Interval to measure FPS  (millis, function called, times invoked for 1000ms around 1 hr and half)
	timer.setTimer(1000, timerCallback, 6000);

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

    // Executes on UDP receive
    udp.onPacket([](AsyncUDPPacket packet) {
		receivedLength = packet.length();
		Serial.printf("First BYTE:%d b[1]+b[2]:%d of %d bytes\n", packet.data()[0],packet.data()[0]+packet.data()[1], receivedLength);
        
		switch (packet.data()[0]+packet.data()[1])
		{
		case 80:
		{
			/* Not compressed */
			pix.receive(packet.data(), packet.length());
			frameCounter++;
			break;
		}
		case 121:
		{
			/* GZIP miniz c file_in file_out */
		    
			uint8_t *outBuffer = new uint8_t[BROTLI_DECOMPRESSION_BUFFER];
			uLong uncomp_len;
			
			int cmp_status = uncompress(
				outBuffer, 
				&uncomp_len, 
				(const unsigned char*)packet.data(), 
				packet.length());
			// status:
			// { MZ_OK = 0, MZ_STREAM_END = 1, MZ_NEED_DICT = 2, MZ_ERRNO = -1, MZ_STREAM_ERROR = -2, MZ_DATA_ERROR = -3, MZ_MEM_ERROR = -4, MZ_BUF_ERROR = -5, MZ_VERSION_ERROR = -6, MZ_PARAM_ERROR = -10000 };
			Serial.printf
			("Status:%d Decompressing miniz. Received: %d bytes, uncomp_length:%lu\n",
			cmp_status, packet.length(), uncomp_len);

			if (cmp_status == 0) {
				pix.receive(outBuffer, uncomp_len);
				frameCounter++;
				if (DEBUG_MODE) {
					Serial.println("HEX decompression dump");
					for (size_t i = 0; i<=uncomp_len; i++){
						Serial.print(outBuffer[i], HEX);
						Serial.print(" ");
					}
				}	
			}
			delete outBuffer;
		}
		break;
		
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
	lostConnectionCount++;
	// Avoid trying to connect forever if the user made a typo in password
	if (lostConnectionCount>4) {
		Serial.println("Clearing saved WiFi credentials");
		preferences.begin("WiFiCred", false);
		preferences.clear();
		preferences.end();
		delay(500);
		ESP.restart();
	}
	WiFi.begin(ssidPrim.c_str(), pwPrim.c_str());
}

// Bluetooth Serial configuration
#ifdef WIFI_BLE
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
	Serial.println("Received message " + receivedData + " over Bluetooth");

	// decode the message | No need to do this, since we receive it as string already
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
			
		}
		else if (jsonBuffer.containsKey("read"))
		{ // {"read":"true"}
			Serial.println("BTSerial read request");
			String wifiCredentials;
			jsonBuffer.clear();

			/** Json object for outgoing data */
			jsonBuffer.clear();
			jsonBuffer["ssidPrim"] = ssidPrim;
			jsonBuffer["pwPrim"] = pwPrim;
			jsonBuffer["ssidSec"] = ssidSec;
			jsonBuffer["pwSec"] = pwSec;
			// Convert JSON object into a string
			serializeJson(jsonBuffer, wifiCredentials);

			// encode the data
			int keyIndex = 0;
			Serial.println("Stored settings: " + wifiCredentials);
			for (int index = 0; index < wifiCredentials.length(); index ++) {
				wifiCredentials[index] = (char) wifiCredentials[index] ^ (char) apName[keyIndex];
				keyIndex++;
				if (keyIndex >= strlen(apName)) keyIndex = 0;
			}
			Serial.println("Stored encrypted: " + wifiCredentials);

			delay(2000);
			SerialBT.print(wifiCredentials);
			SerialBT.flush();
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

  #ifdef WIFI_BLE
  	createName();

	// Start Bluetooth serial
	initBTSerial();
	int waitLoop = 0;
	delay(200);
	while (waitLoop < BLE_SECONDS_WAIT_FOR_CONFIG) {
		if (SerialBT.available() != 0) {
			readBTSerial();		
  		}
		waitLoop++;
		delay(1000);
	}
	preferences.begin("WiFiCred", false);
    //preferences.clear();

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
	}
	preferences.end();

	if (hasCredentials) {
		connectWiFi();
		// Check for available AP's
		/* if (!scanWiFi()) {
			Serial.println("Could not find any AP");
		} else {
			// If AP was found, start connection
			connectWiFi();
		} */
	}
	#else
	WiFi.onEvent(gotIP, SYSTEM_EVENT_STA_GOT_IP);
    WiFi.onEvent(lostCon, SYSTEM_EVENT_STA_DISCONNECTED);
    Serial.println("Connecting to Wi-Fi using WIFI_AP");
    WiFi.begin(WIFI_SSID, WIFI_PASS);
  #endif

}

void loop() {
  timer.run();

  #ifdef WIFI_BLE
  if (SerialBT.available() != 0) {
	readBTSerial();
  }
  #endif
}

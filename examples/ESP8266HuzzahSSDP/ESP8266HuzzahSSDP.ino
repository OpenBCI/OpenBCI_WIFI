#define ADS1299_VREF 4.5
#define MCP3912_VREF 1.2
#define ADC_24BIT_RES_NANO_VOLT 8388607000000000.0
#define BYTES_PER_SPI_PACKET 32
#define BYTES_PER_OBCI_PACKET 33
#define DEBUG 1
#define MAX_SRV_CLIENTS 2
// #define NUM_PACKETS_IN_RING_BUFFER 45
#define NUM_PACKETS_IN_RING_BUFFER 100
#define NUM_PACKETS_IN_RING_BUFFER_JSON 12
// #define MAX_PACKETS_PER_SEND_TCP 20
#define MAX_PACKETS_PER_SEND_TCP 50
#define WIFI_SPI_MSG_LAST 0x01
#define WIFI_SPI_MSG_MULTI 0x02
#define WIFI_SPI_MSG_GAINS 0x03
#define MAX_CHANNELS 16
#define MAX_CHANNELS_PER_PACKET 8
#define NUM_CHANNELS_CYTON 8
#define NUM_CHANNELS_CYTON_DAISY 16
#define NUM_CHANNELS_GANGLION 4
// Arduino JSON needs bytes for duplication
// to recalculate visit:
//   https://bblanchon.github.io/ArduinoJson/assistant/index.html
#define ARDUINOJSON_ADDITIONAL_BYTES_4_CHAN 115
#define ARDUINOJSON_ADDITIONAL_BYTES_8_CHAN 195
#define ARDUINOJSON_ADDITIONAL_BYTES_16_CHAN 355
#define ARDUINOJSON_ADDITIONAL_BYTES_24_CHAN 515
#define ARDUINOJSON_ADDITIONAL_BYTES_32_CHAN 675
#define MICROS_IN_SECONDS 1000000

#include <time.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266SSDP.h>
#include <WiFiManager.h>
#include "SPISlave.h"
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WebSocketsServer.h>
#include <Hash.h>

// ENUMS
typedef enum CLIENT_RESPONSE {
  CLIENT_RESPONSE_NONE,
  CLIENT_RESPONSE_OUTPUT_STRING
};

typedef enum OUTPUT_MODE {
  OUTPUT_MODE_RAW,
  OUTPUT_MODE_JSON
};

typedef enum OUTPUT_PROTOCOL {
  OUTPUT_PROTOCOL_NONE,
  OUTPUT_PROTOCOL_TCP,
  OUTPUT_PROTOCOL_MQTT,
  OUTPUT_PROTOCOL_WEB_SOCKETS
};

typedef enum CYTON_GAIN {
  CYTON_GAIN_1,
  CYTON_GAIN_2,
  CYTON_GAIN_4,
  CYTON_GAIN_6,
  CYTON_GAIN_8,
  CYTON_GAIN_12,
  CYTON_GAIN_24
};

// STRUCTS
typedef struct {
    long *        channelData;
    unsigned long timestamp;
} Sample;

boolean clientWaitingForResponse;
boolean clientWaitingForResponseFullfilled;
boolean ntpOffsetSet;
boolean syncingNtp;
boolean waitingOnNTP;
boolean waitingDaisyPacket;
boolean spiTXBufferLoaded;
boolean underSelfTest;

CLIENT_RESPONSE curClientResponse;

const char *serverCloudbrain;
const char *serverCloudbrainAuth;
const char *mqttUsername;
const char *mqttPassword;
const char *mqttBrokerAddress;
const char *tcpAddress;
const char *tcpPort;

ESP8266WebServer server(80);

double scaleFactors[MAX_CHANNELS];

int counter;
int latency;

OUTPUT_MODE curOutputMode;
OUTPUT_PROTOCOL curOutputProtocol;

Sample sampleBuffer[NUM_PACKETS_IN_RING_BUFFER_JSON];

size_t jsonBufferSize;

String jsonStr;
String outputString;

uint8_t numChannels;
uint8_t ntpTimeSyncAttempts;
uint8_t outputBuf[MAX_PACKETS_PER_SEND_TCP * BYTES_PER_OBCI_PACKET];
uint8_t passthroughBuffer[BYTES_PER_SPI_PACKET];
uint8_t passthroughPosition;
uint8_t ringBuf[NUM_PACKETS_IN_RING_BUFFER][BYTES_PER_OBCI_PACKET];
uint8_t sampleNumber;
uint8_t samplesLoaded;
uint8_t lastSampleNumber;

unsigned long lastSendToClient;
unsigned long lastHeadMove;
unsigned long lastMQTTConnectAttempt;
unsigned long lastTimeWasPolled;
unsigned long ntpOffset;
unsigned long ntpLastTimeSeconds;
unsigned long timeOfWifiTXBufferLoaded;

volatile uint8_t head;
volatile uint8_t tail;

WiFiClient clientTCP;
WiFiClient espClient;
PubSubClient clientMQTT(espClient);

///////////////////////////////////////////
// Utility functions
///////////////////////////////////////////
String getMac() {
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.softAPmacAddress(mac);
  String macID = String(mac[WL_MAC_ADDR_LENGTH - 2], HEX) +
                 String(mac[WL_MAC_ADDR_LENGTH - 1], HEX);
  macID.toUpperCase();
  return macID;
}

String getModelNumber() {
  String AP_NameString = "PTW-OBCI-0001-" + getMac();

  char AP_NameChar[AP_NameString.length() + 1];
  memset(AP_NameChar, 0, AP_NameString.length() + 1);

  for (int i=0; i<AP_NameString.length(); i++)
    AP_NameChar[i] = AP_NameString.charAt(i);

  return AP_NameString;
}

String getName() {
  String AP_NameString = "OpenBCI-" + getMac();

  char AP_NameChar[AP_NameString.length() + 1];
  memset(AP_NameChar, 0, AP_NameString.length() + 1);

  for (int i=0; i<AP_NameString.length(); i++)
    AP_NameChar[i] = AP_NameString.charAt(i);

  return AP_NameString;
}

String mqttGetInfo() {
  String json = "{";
  json += "\"broker_address\":\""+String(mqttBrokerAddress)+"\",";
  json += "\"connected\":"+String(clientMQTT.connected() ? "true" : "false")+",";
  json += "\"username\":\""+String(mqttUsername)+"\"";
  json += "}";
  return json;
}

String tcpGetInfo() {
  String json = "{";
  json += "\"connected\":"+String(clientTCP.connected() ? "true" : "false")+",";
  json += "\"ip_address\":\""+String(tcpAddress)+"\",";
  json += "\"port\":\""+String(tcpPort)+"\"";
  json += "}";
  return json;
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
}

///////////////////////////////////////////
// NTP BEGIN
///////////////////////////////////////////
/**
 * Check to see if SNTP is active
 * @type {Number}
 */
boolean ntpActive() {
  return time(nullptr) > 1000;
}

/**
 * Get ntp time in microseconds
 * @return [long] - The time in micro second
 */
unsigned long ntpGetTime() {
  return time(nullptr) * MICROS_IN_SECONDS;
}

/**
 * Safely get the time, defaults to micros() if ntp is not active.
 */
unsigned long getTime() {
  if (ntpActive()) {
    return ntpGetTime() + ((micros()%MICROS_IN_SECONDS) - (ntpOffset%MICROS_IN_SECONDS));
  } else {
    return micros();
  }
}

/**
 * Use this to start the sntp time sync
 * @type {Number}
 */
void ntpStart() {
#ifdef DEBUG
  Serial.println("Setting time using SNTP");
#endif
  configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");
}


///////////////////////////////////////////
// DATA PROCESSING BEGIN
///////////////////////////////////////////

/**
 * Return true if the channel data array is full
 * @param arr [uint8_t *] - 32 byte array from Cyton or Ganglion
 * @param sample [Sample *] - Sample struct to hold data before conversion to float
 * @param packetOffset [uint8_t] - The offset to shift loading channel data into.
 *   i.e. should be 1 on second packet for daisy
 */
void channelDataCompute(uint8_t *arr, Sample *sample, uint8_t packetOffset) {
  const uint8_t byteOffset = 2;
  if (packetOffset == 0) sample->timestamp = getTime();
  for (uint8_t i = 0; i < MAX_CHANNELS_PER_PACKET; i++) {
    // Zero out the new value
    uint32_t raw = 0;
    // Pull out 24bit number
    raw = arr[i*3 + byteOffset] << 16 | arr[i*3 + 1 + byteOffset] << 8 | arr[i*3 + 2 + byteOffset];
    // carry through the sign
    if(bitRead(raw,23) == 1){
      raw |= 0xFF000000;
    } else{
      raw &= 0x00FFFFFF;
    }

    sample->channelData[i + packetOffset] = (long)(scaleFactors[i] * ((double)raw));

    // Serial.printf("%d: %2.10f\n",i+1, farr[i]);
  }
}

/**
 * We want to max the size out to < 2000bytes per json chunk
 */
uint8_t sendJsonMaxPackets() {
  switch (numChannels) {
    case NUM_CHANNELS_GANGLION:
      return 8; // Size of
    case NUM_CHANNELS_CYTON_DAISY:
      return 3;
    case NUM_CHANNELS_CYTON:
    default:
      return 5;
  }
}

/**
 * The additional bytes needed for input duplication, follows max packets
 */
int sendJsonAdditionalBytes() {
  switch (numChannels) {
    case NUM_CHANNELS_GANGLION:
      return 1014;
    case NUM_CHANNELS_CYTON_DAISY:
      return 1062;
    case NUM_CHANNELS_CYTON:
    default:
      return 966;
  }
}

void gainReset() {
  for (uint8_t i = 0; i < MAX_CHANNELS; i++) {
    scaleFactors[i] = 0.0;
  }
}

float gainCyton(uint8_t b) {
  switch (b) {
    case CYTON_GAIN_1:
      return 1.0;
    case CYTON_GAIN_2:
      return 2.0;
    case CYTON_GAIN_4:
      return 4.0;
    case CYTON_GAIN_6:
      return 6.0;
    case CYTON_GAIN_8:
      return 8.0;
    case CYTON_GAIN_12:
      return 12.0;
    case CYTON_GAIN_24:
    default:
      return 24.0;
  }
}

float gainGanglion() {
  return 51.0;
}

/**
 * Used to set the gain for the JSON conversion
 * @param `raw` [uint8_t *] - The array from Pic or Cyton descirbing channels
 *   and gains.
 */
void gainSet(uint8_t *raw) {
  uint8_t byteCounter = 2;
  numChannels = raw[byteCounter++];

  if (numChannels < NUM_CHANNELS_GANGLION || numChannels > MAX_CHANNELS) {
    return;
  }
  for (uint8_t i = 0; i < numChannels; i++) {
    if (numChannels == NUM_CHANNELS_GANGLION) {
      // do gang related stuffs
      scaleFactors[i] = MCP3912_VREF / gainGanglion() / ADC_24BIT_RES_NANO_VOLT;
    } else {
      scaleFactors[i] = ADS1299_VREF / gainCyton(raw[byteCounter++]) / ADC_24BIT_RES_NANO_VOLT;
    }
#ifdef DEBUG
    Serial.printf("Channel: %d\n\tgain: %d\n\tscale factor: %.10f\n", i+1, scaleFactors[i]);
#endif
  }

  if (curOutputMode == OUTPUT_MODE_JSON) {
    jsonBufferSize = 0; // Reset to 0
    jsonBufferSize += JSON_OBJECT_SIZE(1); // For {"chunk":[...]}
    jsonBufferSize += JSON_ARRAY_SIZE(sendJsonMaxPackets()); // For the array of samples
    jsonBufferSize += sendJsonMaxPackets()*JSON_OBJECT_SIZE(2); // For each sample {"timestamp":0, "data":[...]}
    jsonBufferSize += sendJsonMaxPackets()*JSON_ARRAY_SIZE(numChannels); // For data array for each sample
    jsonBufferSize += sendJsonAdditionalBytes(); // The additional bytes needed for input duplication
  }
}
///////////////////////////////////////////////////
// MQTT
///////////////////////////////////////////////////

void callbackMQTT(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(0, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  } else {
    digitalWrite(0, HIGH);  // Turn the LED off by making the voltage HIGH
  }
}

void passthroughBufferClear() {
  for (uint8_t i = 0; i < BYTES_PER_OBCI_PACKET; i++) {
    passthroughBuffer[i] = 0;
  }
  passthroughPosition = 0;
}

/**
 * Used when
 */
void configModeCallback (WiFiManager *myWiFiManager) {
#ifdef DEBUG
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
#endif
}


///////////////////////////////////////////////////
// MQTT
///////////////////////////////////////////////////

void returnOK(int code, String s) {
  digitalWrite(5, HIGH);
  server.send(code, "text/plain", s);
  digitalWrite(5, LOW);
}

void returnOK(String s) {
  returnOK(200, s);
}

void returnOK(void) {
  returnOK("");
}

void returnFail(int code, String msg) {
  digitalWrite(5, HIGH);
  server.send(code, "text/plain", msg + "\r\n");
  digitalWrite(5, LOW);
}

bool readRequest(WiFiClient& client) {
  bool currentLineIsBlank = true;
  while (client.connected()) {
    if (client.available()) {
      char c = client.read();
      if (c == '\n' && currentLineIsBlank) {
        return true;
      } else if (c == '\n') {
        currentLineIsBlank = true;
      } else if (c != '\r') {
        currentLineIsBlank = false;
      }
    }
  }
  return false;
}

JsonObject& getArgFromArgs() {
  const size_t argBufferSize = JSON_OBJECT_SIZE(3) + 125;
  DynamicJsonBuffer jsonBuffer(argBufferSize);
  // const char* json = "{\"port\":13514}";
  JsonObject& root = jsonBuffer.parseObject(server.arg(0));
  return root;
}

// boolean cloudbrainAuthGetVhost(const char *serverAddr, const char *username) {
//   WiFiClientSecure client;
//
//   Serial.print("\nconnecting to "); Serial.println(serverAddr);
//
//   if (client.connect(serverAddr, 443)) {
//     StaticJsonBuffer<56> jsonBuffer;
//     JsonObject& root = jsonBuffer.createObject();
//     root["username"] = username;
//     Serial.println("Connected to server");
//     // Make a HTTP request
//     client.println("POST /rabbitmq/vhost/info HTTP/1.1");
//     client.println("Host: auth.getcloudbrain.com");
//     client.println("Accept: */*");
//     client.print("Content-Length: "); client.println(root.measureLength());
//     client.println("Content-Type: application/json");
//     client.println();
//     root.printTo(client);
//     // Serial.println("POST /rabbitmq/vhost/info HTTP/1.1");
//     // Serial.println("Host: auth.getcloudbrain.com");
//     // Serial.println("Accept: */*");
//     // Serial.print("Content-Length: "); Serial.println(root.measureLength());
//     // Serial.println("Content-Type: application/json");
//     // Serial.println();
//     // root.printTo(Serial);
//     int connectLoop = 0;
//     int inChar;
//
//     while(client.connected()) {
//       while(client.available()){
//         inChar = client.read();
//         Serial.write(inChar);
//         connectLoop = 0;
//         delay(1500);
//       }
//
//       delay(10);
//       connectLoop++;
//       if(connectLoop > 10000){
//         Serial.println();
//         Serial.println(F("Timeout"));
//         client.stop();
//       }
//     }
//
//     Serial.println();
//     Serial.println(F("disconnecting."));
//     client.stop();
//     returnOK("Sent command to client");
//     return true;
//   } else {
//     returnFail(500, "Unable to connect to client");
//     return false;
//   }
// }

/**
 * Used to set the latency of the system.
 */
boolean setLatency() {
  if(server.args() == 0) return false;

  JsonObject& root = getArgFromArgs();

  if (root.containsKey("latency")) {
    latency = root["latency"];
    return true;
  }
  return false;
}

/**
 * Used to set the latency of the system.
 */
uint8_t passThroughCommand() {
  if(server.args() == 0) return false;

  JsonObject& root = getArgFromArgs();

  if (root.containsKey("command")) {
    String cmds = root["command"];
    uint8_t numCmds = uint8_t(cmds.length());
    // const char* cmds = root["command"];
// #ifdef DEBUG
//     Serial.printf("Got %d chars: ", numCmds);
//     for (int i = 0; i < numCmds; i++) {
//       Serial.print(cmds.charAt(i));
//     }
//     Serial.println();Serial.print("cmds ");Serial.println(cmds);
// #endif
    if (numCmds > BYTES_PER_SPI_PACKET - 1) {
      return 2;
    }

    passthroughBuffer[0] = numCmds;
    passthroughPosition = 1;

    for (int i = 1; i < numCmds + 1; i++) {
      passthroughBuffer[i] = cmds.charAt(i-1);
    }
    passthroughPosition += numCmds;

    return 0;
  }
  return 1;
}

void setupSocketWithClient() {
  // Parse args
  if(server.args() == 0) return returnFail(501, "Error: No body in POST request"); // no body
  JsonObject& root = getArgFromArgs();
  if (!root.containsKey("ip")) return returnFail(502, "Error: No 'ip' in body");
  if (!root.containsKey("port")) return returnFail(503, "Error: No 'port' in body");

  const char *ip = root["ip"];
  int port = root["port"];

#ifdef DEBUG
  Serial.print("Got ip: "); Serial.println(ip);
  Serial.print("Got port: "); Serial.println(port);
  Serial.print("Current uri: "); Serial.println(server.uri());
  Serial.print("Starting socket to host: "); Serial.print(server.client().remoteIP().toString()); Serial.print(" on port: "); Serial.println(port);
#endif

  curOutputProtocol = OUTPUT_PROTOCOL_TCP;

  if (clientTCP.connect(ip, port)) {
#ifdef DEBUG
    Serial.println("Connected to server");
#endif
    clientTCP.setNoDelay(1);
    return server.send(200, "text/json", tcpGetInfo());
  } else {
#ifdef DEBUG
    Serial.println("Failed to connect to server");
#endif
    return server.send(504, "text/json", tcpGetInfo());
  }
}

boolean mqttConnect() {
  if (clientMQTT.connect(getName().c_str(), mqttUsername, mqttPassword)) {
#ifdef DEBUG
    Serial.println("connected");
#endif
    // Once connected, publish an announcement...
    clientMQTT.publish("openbci", "Will you Push The World?");
    return true;
  } else {
    // Wait 5 seconds before retrying
    lastMQTTConnectAttempt = millis();
    return false;
  }
}

/**
 * Function called on route `/mqtt` with HTTP_POST with body
 * {"username":"user_name", "password": "you_password", "broker_address": "/your.broker.com"}
 */
void mqttSetup() {
  // Parse args
  if(server.args() == 0) return returnFail(501, "Error: No body in POST request"); // no body
  size_t argBufferSize = JSON_OBJECT_SIZE(3) + 220;
  DynamicJsonBuffer jsonBuffer(argBufferSize);
  JsonObject& root = jsonBuffer.parseObject(server.arg(0));
  if (!root.containsKey("username")) return returnFail(502, "Error: No 'username' in body");
  if (!root.containsKey("password")) return returnFail(503, "Error: No 'password' in body");
  if (!root.containsKey("broker_address")) return returnFail(504, "Error: No 'broker_address' in body");

  mqttUsername = root["username"]; // "alongname.alonglastname@getcloudbrain.com"
  mqttPassword = root["password"]; // "that time when i had a big password"
  mqttBrokerAddress = root["broker_address"]; // "/the quick brown fox jumped over the lazy dog"

#ifdef DEBUG
  Serial.print("Got username: "); Serial.println(mqttUsername);
  Serial.print("Got password: "); Serial.println(mqttPassword);
  Serial.print("Got broker_address: "); Serial.println(mqttBrokerAddress);

  Serial.println("About to try and connect to cloudbrain MQTT server");
#endif

  clientMQTT.setServer(mqttBrokerAddress, 1883);

  curOutputProtocol = OUTPUT_PROTOCOL_MQTT;

  if (mqttConnect()) {
    return server.send(200, "text/json", mqttGetInfo());
  } else {
    return server.send(505, "text/json", mqttGetInfo());
  }
}

/**
 * Used to prepare a response in JSON
 */
// JsonObject& prepareSampleJSON(JsonBuffer& jsonBuffer, uint8_t packetsToSend) {
//   JsonObject& root = jsonBuffer.createObject();
//   JsonArray& chunk = root.createNestedArray("chunk");
//   for (uint8_t i = 0; i < packetsToSend; i++) {
//     JsonObject& sample = chunk.createNestedObject();
//     double curTime = ntpActive() ? ntpGetTime() : micros();
//     sample.set<double>("timestamp", curTime);
//     JsonArray& data = sample.createNestedArray("data");
//     for (uint8_t j = 0; i < numChannels; i++) {
//       data.add(channelData[i][j]);
//     }
//   }
//   return root;
// }

/**
 * Used to prepare a response in JSON with chunk
 */
JsonObject& intializeJSONChunk(JsonBuffer& jsonBuffer) {
  JsonObject& root = jsonBuffer.createObject();
  JsonArray& chunk = root.createNestedArray("chunk");
  return root;
}

// /**
//  * Convert sample to JSON, convert to string, return string
//  */
// void convertSampleToJSON(JsonObject& chunkObj, uint8_t *in, uint8_t nchan) {
//   // Get the chunk array
//   JsonArray& chunk = chunkObj["chunk"];
//
//   JsonObject& sample = chunk.createNestedObject();
//   sample["timestamp"] = ntpActive() ? ntpGetTime() : micros();
//   JsonArray& data = sample.createNestedArray("data");
//   for (uint8_t i = 0; i < numChannels; i++) {
//     data.add(channelData[i][j]);
//   }
// }

// void handleNotFound(){
//   String message = "File Not Found\n\n";
//   message += "URI: ";
//   message += server.uri();
//   message += "\nMethod: ";
//   message += (server.method() == HTTP_GET)?"GET":"POST";
//   message += "\nArguments: ";
//   message += server.args();
//   message += "\n";
//   for (uint8_t i=0; i<server.args(); i++){
//     message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
//   }
//   server.send(404, "text/plain", message);
// }
//



/**
* @description Test to see if a char follows the stream tail byte format
* @author AJ Keller (@pushtheworldllc)
*/
boolean isAStreamByte(uint8_t b) {
  return (b >> 4) == 0xC;
}

/**
 * Used to when the /data route is hit
 * @type {[type]}
//  */
// void getData() {
//
//   DynamicJsonBuffer jsonBuffer(bufferSize);
//   JsonObject& object = prepareResponse(jsonBuffer);
//   // object.printTo(Serial); Serial.println();
//
//   server.setContentLength(object.measureLength());
//   server.send(200, "application/json", "");
//
//   WiFiClientPrint<> p(server.client());
//   object.printTo(p);
//   p.stop();
// }

/**
 * Called when a sensor needs to be sent a command through SPI
 * @type {[type]}
 */
void handleSensorCommand() {
  if(server.args() == 0) return returnFail(500, "BAD ARGS");
  String path = server.arg(0);

  Serial.println(path);

  if(path == "/") {
    returnFail(501, "BAD PATH");
    return;
  }

  // SPISlave.setData(ip.toString().c_str());
}

//////////////////////////////////////////////
// WebSockets ////////////////////////////////
//////////////////////////////////////////////

WebSocketsServer webSocket = WebSocketsServer(81);

#define USE_SERIAL Serial

String fragmentBuffer = "";

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

	switch(type) {
		case WStype_DISCONNECTED:
			USE_SERIAL.printf("[%u] Disconnected!\n", num);
			break;
		case WStype_CONNECTED: {
			IPAddress ip = webSocket.remoteIP(num);
			USE_SERIAL.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

			// send message to client
			webSocket.sendTXT(num, "Connected");
		}
			break;
		case WStype_TEXT:
			USE_SERIAL.printf("[%u] get Text: %s\n", num, payload);
			break;
		case WStype_BIN:
			USE_SERIAL.printf("[%u] get binary length: %u\n", num, length);
			hexdump(payload, length);
			break;
	}

}

void initializeVariables() {
  clientWaitingForResponse = false;
  clientWaitingForResponseFullfilled = false;
  spiTXBufferLoaded = false;
  underSelfTest = false;
  syncingNtp = false;
  waitingOnNTP = false;
  ntpOffsetSet = false;
  waitingDaisyPacket = false;

  counter = 0;
  head = 0;
  jsonBufferSize = 0;
  lastHeadMove = 0;
  lastMQTTConnectAttempt = 0;
  lastSampleNumber = 0;
  lastSendToClient = 0;
  lastTimeWasPolled = 0;
  latency = 10000;
  ntpLastTimeSeconds = 0;
  ntpOffset = 0;
  ntpTimeSyncAttempts = 0;
  passthroughPosition = 0;
  sampleNumber = 0;
  tail = 0;
  timeOfWifiTXBufferLoaded = 0;
  samplesLoaded = 0;

  outputString = "";

  // curOutputMode = OUTPUT_MODE_RAW;
  curOutputMode = OUTPUT_MODE_JSON;
  // curOutputProtocol = OUTPUT_PROTOCOL_MQTT;
  curOutputProtocol = OUTPUT_PROTOCOL_TCP;
  curClientResponse = CLIENT_RESPONSE_NONE;

  passthroughBufferClear();
  gainReset();
}

void setup() {
  initializeVariables();

  pinMode(5, OUTPUT);
  pinMode(0, OUTPUT);
  digitalWrite(0, LOW);

#ifdef DEBUG
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println("Serial started");
#endif

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  WiFiManagerParameter custom_text("<p>Powered by Push The World</p>");
  wifiManager.addParameter(&custom_text);

  wifiManager.setAPCallback(configModeCallback);

  //and goes into a blocking loop awaiting configuration
#ifdef DEBUG
  Serial.println("Wifi manager started...");
#endif
  wifiManager.autoConnect(getName().c_str());

#ifdef DEBUG
  Serial.printf("Starting ntp...\n");
#endif
  ntpStart();

#ifdef DEBUG
  Serial.printf("Starting SSDP...\n");
#endif
  SSDP.setSchemaURL("description.xml");
  SSDP.setHTTPPort(80);
  SSDP.setName("PTW - OpenBCI Wifi Shield");
  SSDP.setSerialNumber(getName());
  SSDP.setURL("index.html");
  SSDP.setModelName(getModelNumber());
  SSDP.setModelNumber("929000226503");
  SSDP.setModelURL("http://www.openbci.com");
  SSDP.setManufacturer("Push The World LLC");
  SSDP.setManufacturerURL("http://www.pushtheworldllc.com");
  SSDP.begin();

  // pinMode(0, INPUT);
  // data has been received from the master. Beware that len is always 32
  // and the buffer is autofilled with zeroes if data is less than 32 bytes long
  // It's up to the user to implement protocol for handling data length
  SPISlave.onData([](uint8_t * data, size_t len) {
    if (isAStreamByte(data[0])) {
      if (curOutputMode == OUTPUT_MODE_RAW) {
        if (head >= NUM_PACKETS_IN_RING_BUFFER) {
          head = 0;
        }
        uint8_t stopByte = data[0];
        ringBuf[head][0] = 0xA0;
        // Serial.printf("-%d\n",ringBuf[head][1]);
        ringBuf[head][len] = stopByte;
        for (int i = 1; i < len; i++) {
          ringBuf[head][i] = data[i];
        }
        head++;
      } else {
        if (numChannels > MAX_CHANNELS_PER_PACKET) {
          // DO DAISY
          if (lastSampleNumber != data[1]) {
            lastSampleNumber = data[1];
            // this is first packet of new sample
            if (head >= NUM_PACKETS_IN_RING_BUFFER_JSON) {
              head = 0;
            }
            // this is the first packet, no offset
            channelDataCompute(data, sampleBuffer + head, 0);
          } else {
            // this is not first packet of new sample
            channelDataCompute(data, sampleBuffer + head, MAX_CHANNELS_PER_PACKET);
            head++;
          }
        } else { // Cyton or Ganglion
          if (head >= NUM_PACKETS_IN_RING_BUFFER_JSON) {
            head = 0;
          }
          // Convert sample immidiate, store to buffer and get out!
          channelDataCompute(data, sampleBuffer+head, 0);
          head++;
        }
      }

    } else {
      // save the client because we will need to send them some ish
      if (clientWaitingForResponse) {
        String newString = (char *)data;
        newString = newString.substring(1, len);

        switch (data[0]) {
          case WIFI_SPI_MSG_MULTI:
            // Serial.print("mulit:\n\toutputString:\n\t"); Serial.print(outputString); Serial.print("\n\tnewString\n\t"); Serial.println(newString);
            outputString.concat(newString);
            break;
          case WIFI_SPI_MSG_LAST:
            // Serial.println("last");
            // Serial.print("last:\n\toutputString:\n\t"); Serial.print(outputString); Serial.print("\n\tnewString\n\t"); Serial.println(newString);
            outputString.concat(newString);
            clientWaitingForResponse = false;
#ifdef DEBUG
            Serial.println(outputString);
#endif
            curClientResponse = CLIENT_RESPONSE_OUTPUT_STRING;
            clientWaitingForResponseFullfilled = true;
            break;
          default:
            curClientResponse = CLIENT_RESPONSE_NONE;
            clientWaitingForResponseFullfilled = true;
            clientWaitingForResponse = false;
            Serial.println("on data");
            break;
        }
      }
      if (data[0] > 0) {
        if (data[0] == data[1]) {
          switch (data[0]) {
            case WIFI_SPI_MSG_GAINS:
    #ifdef DEBUG
              Serial.println("gainSet");
    #endif
              gainSet(data);
              break;
            default:
              break;
          }
        }
      }
    }
  });

  SPISlave.onDataSent([]() {
    if (passthroughBuffer[0] > 0) {
      Serial.println("Sent data: 0x");
      for (uint8_t i = 0; i < BYTES_PER_OBCI_PACKET; i++) {
        Serial.print(passthroughBuffer[i],HEX);
      }
      Serial.println();
    }
    for (uint8_t i = 0; i < BYTES_PER_OBCI_PACKET; i++) {
      passthroughBuffer[i] = 0;
    }
    passthroughPosition = 0;
    SPISlave.setData(passthroughBuffer, BYTES_PER_OBCI_PACKET);
    // IPAddress ip = WiFi.localIP();
    // SPISlave.setData(ip.toString().c_str());
  });

  // The master has read the status register
  SPISlave.onStatusSent([]() {
#ifdef DEBUG
    Serial.println("Status Sent");
#endif
    SPISlave.setStatus(209);
  });

  passthroughBufferClear();

  // Setup SPI Slave registers and pins
  SPISlave.begin();

  // Set the status register (if the master reads it, it will read this value)
  SPISlave.setStatus(209);
  SPISlave.setData(passthroughBuffer, BYTES_PER_SPI_PACKET);

#ifdef DEBUG
  Serial.println("SPI Slave ready");
#endif

  ArduinoOTA.setHostname(getName().c_str());

#ifdef DEBUG
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
#endif
  ArduinoOTA.begin();

#ifdef DEBUG
  printWifiStatus();
  Serial.printf("Starting HTTP...\n");
#endif
  server.on("/index.html", HTTP_GET, [](){
    digitalWrite(5, HIGH);
    server.send(200, "text/plain", "Push The World - OpenBCI - Wifi bridge - is up and running woo");
    digitalWrite(5, LOW);
  });
  server.on("/description.xml", HTTP_GET, [](){
#ifdef DEBUG
    Serial.println("SSDP HIT");
#endif
    digitalWrite(5, HIGH);
    SSDP.schema(server.client());
    digitalWrite(5, LOW);
  });
  server.on("/yt", HTTP_GET, [](){
    digitalWrite(5, HIGH);
    server.send(200, "text/plain", "Keep going! Push The World!");
    digitalWrite(5, LOW);
  });

  server.on("/test/start", HTTP_GET, [](){
    underSelfTest = true;
    Serial.println("Under self test start");
    returnOK();
  });
  server.on("/test/stop", HTTP_GET, [](){
    underSelfTest = false;
    Serial.println("Under self test start");
    returnOK();
  });

  server.on("/output/json", HTTP_GET, [](){
    curOutputMode = OUTPUT_MODE_JSON;
    returnOK();
  });
  server.on("/output/raw", HTTP_GET, [](){
    curOutputMode = OUTPUT_MODE_RAW;
    returnOK();
  });

  server.on("/mqtt", HTTP_GET, []() {
    server.send(200, "text/json", mqttGetInfo());
  });
  server.on("/mqtt", HTTP_POST, mqttSetup);

  server.on("/tcp", HTTP_GET, []() {
    server.send(200, "text/json", tcpGetInfo());
  });
  server.on("/tcp", HTTP_POST, setupSocketWithClient);

  // These could be helpful...
  server.on("/stream/start", HTTP_GET, []() {
    passthroughBuffer[0] = 1;
    passthroughBuffer[1] = 'b';
    passthroughPosition = 2;
    returnOK();
  });
  server.on("/stream/stop", HTTP_GET, []() {
    passthroughBuffer[0] = 1;
    passthroughBuffer[1] = 's';
    passthroughPosition = 2;
    returnOK();
  });

  server.on("/version", HTTP_GET, [](){
    digitalWrite(5, HIGH);
    server.send(200, "text/plain", "v0.1.0");
    digitalWrite(5, LOW);
  });

  server.on("/command", HTTP_POST, [](){
    switch(passThroughCommand()) {
      case 0: // Command sent to board
        spiTXBufferLoaded = true;
        timeOfWifiTXBufferLoaded = millis();
        clientWaitingForResponse = true;
        break;
      case 1: // command not found
        returnFail(500, "Error: no \'command\' in json arg");
        break;
      case 2: // length to long
      default:
        returnFail(501, "Error: Sent more than 31 chars");
        break;
    }
  });
  server.on("/latency", HTTP_GET, [](){
    returnOK(String(latency).c_str());
  });
  server.on("/latency", HTTP_POST, [](){
    setLatency() ? returnOK() : returnFail(500, "Error: no \'latency\' in json arg");
  });

  if (!MDNS.begin(getName().c_str())) {
#ifdef DEBUG
    Serial.println("Error setting up MDNS responder!");
#endif
  } else {
#ifdef DEBUG
    Serial.print("Your ESP is called "); Serial.println(getName());
#endif
  }

  server.onNotFound([](){
    server.send(404, "text/plain", "FileNotFound");
  });
  // server.onNotFound(handleNotFound);
  //
  //get heap status, analog input value and all GPIO statuses in one json call
  server.on("/all", HTTP_GET, [](){
    String json = "{";
    json += "\"heap\":"+String(ESP.getFreeHeap())+",";
    json += "\"name\":\""+getName()+"\",";
    json += "\"num_channels\":"+String(numChannels);
    json += "}";
    server.send(200, "text/json", json);
    json = String();
  });

  server.begin();
  MDNS.addService("http", "tcp", 80);

#ifdef DEBUG
    Serial.printf("Ready!\n");
#endif

  // Test to see if ntp is good
  if (ntpActive()) {
    syncingNtp = true;
  } else {
#ifdef DEBUG
    Serial.println("Unable to get ntp sync");
#endif
    waitingOnNTP = true;
    ntpLastTimeSeconds = millis();
  }

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  clientMQTT.setCallback(callbackMQTT);

}
// unsigned long lastPrint = 0;
void loop() {
  ArduinoOTA.handle();
  server.handleClient();
  webSocket.loop();

  if (curOutputProtocol == OUTPUT_PROTOCOL_MQTT) {
    if (clientMQTT.connected()) {
      clientMQTT.loop();
    } else if (millis() > 5000 + lastMQTTConnectAttempt) {
      mqttConnect();
    }
  }

  if (syncingNtp) {
    unsigned long curTime = time(nullptr);
    if (ntpLastTimeSeconds == 0) {
      ntpLastTimeSeconds = curTime;
    } else if (ntpLastTimeSeconds < curTime) {
      ntpOffset = micros();
      syncingNtp = false;

#ifdef DEBUG
      Serial.print("Time set: "); Serial.println(ntpOffset);
#endif
    }
  }

  if (waitingOnNTP && (millis() > 3000 + ntpLastTimeSeconds)) {
    // Test to see if ntp is good
    if (ntpActive()) {
      waitingOnNTP = false;
      syncingNtp = true;
      ntpLastTimeSeconds = 0;
    }
    ntpTimeSyncAttempts++;
    if (ntpTimeSyncAttempts > 3) {
#ifdef DEBUG
      Serial.println("Unable to get ntp sync");
#endif
      waitingOnNTP = false;
    } else {
      ntpLastTimeSeconds = millis();
    }
  }

  // if (millis() > lastPrint + 20) {
  //   lastPrint = millis();
  //   for (int i = 0; i < 32; i++) {
  //     Serial.print(passthroughBuffer[i], HEX);
  //   }
  //   Serial.println();
  // }
  // Only try to do OTA if 'prog' button is held down
  if (digitalRead(0) == 0) {
    ArduinoOTA.handle();
  }

  if (clientWaitingForResponseFullfilled) {
    clientWaitingForResponseFullfilled = false;
    switch (curClientResponse) {
      case CLIENT_RESPONSE_OUTPUT_STRING:
        returnOK(outputString);
        outputString = "";
        break;
      case CLIENT_RESPONSE_NONE:
      default:
        returnOK();
        break;
    }
  }

  if (passthroughPosition > 0) {
    SPISlave.setData(passthroughBuffer, passthroughPosition);
    passthroughBufferClear();
  }

  if (underSelfTest) {
    int sampleIntervaluS = 1000; // micro seconds
    boolean daisy = false;
    if (micros() > (lastHeadMove + sampleIntervaluS)) {
      head += daisy ? 2 : 1;
      if (head >= NUM_PACKETS_IN_RING_BUFFER) {
        head -= NUM_PACKETS_IN_RING_BUFFER;
      }
      lastHeadMove = micros();
    }
  }

  if (clientWaitingForResponse && (millis() > (timeOfWifiTXBufferLoaded + 1000))) {
    clientWaitingForResponse = false;
    returnFail(502, "Error: timeout getting command response, be sure board is fully connected");
#ifdef DEBUG
    Serial.println("Failed to get response in 1000ms");
#endif
  }

  if((clientTCP.connected() || clientMQTT.connected()) && (micros() > (lastSendToClient + latency)) && head != tail) {
    // Serial.print("h: "); Serial.print(head); Serial.print(" t: "); Serial.print(tail); Serial.print(" cTCP: "); Serial.print(clientTCP.connected()); Serial.print(" cMQTT: "); Serial.println(clientMQTT.connected());

    digitalWrite(5, HIGH);

    int packetsToSend = head - tail;

    if (curOutputMode == OUTPUT_MODE_RAW) {
      if (packetsToSend < 0) {
        packetsToSend = NUM_PACKETS_IN_RING_BUFFER + packetsToSend; // for wrap around
      }
      if (packetsToSend > (MAX_PACKETS_PER_SEND_TCP - 5)) {
        packetsToSend = MAX_PACKETS_PER_SEND_TCP - 5;
      }
      int index = 0;
      for (uint8_t i = 0; i < packetsToSend; i++) {
        if (tail >= NUM_PACKETS_IN_RING_BUFFER) {
          tail = 0;
        }
        for (uint8_t j = 0; j < BYTES_PER_OBCI_PACKET; j++) {
          outputBuf[index++] = ringBuf[tail][j];
        }
        tail++;
      }
      if (curOutputProtocol == OUTPUT_PROTOCOL_TCP) {
        clientTCP.write(outputBuf, BYTES_PER_OBCI_PACKET * packetsToSend);
      } else {
        clientMQTT.publish("openbci",(const char*) outputBuf);

      }
    } else { // output mode is JSON
      if (packetsToSend < 0) {
        packetsToSend = NUM_PACKETS_IN_RING_BUFFER_JSON + packetsToSend; // for wrap around
      }
      if (packetsToSend > sendJsonMaxPackets()) {
        packetsToSend = sendJsonMaxPackets();
      }

      DynamicJsonBuffer jsonSampleBuffer(jsonBufferSize);

      JsonObject& root = jsonSampleBuffer.createObject();
      JsonArray& chunk = root.createNestedArray("chunk");

      for (uint8_t i = 0; i < packetsToSend; i++) {
        if (tail >= NUM_PACKETS_IN_RING_BUFFER_JSON) {
          tail = 0;
        }
        JsonObject& sample = chunk.createNestedObject();
        sample["timestamp"] = (sampleBuffer + tail)->timestamp;
        JsonArray& data = sample.createNestedArray("data");
        for (uint8_t j = 0; i < numChannels; i++) {
          data.add((sampleBuffer + tail)->channelData[j]);
        }
        tail++;
      }

      if (curOutputProtocol == OUTPUT_PROTOCOL_TCP) {
        // root.printTo(Serial);
        root.printTo(clientTCP);
      } else {
        root.prettyPrintTo(jsonStr);
        clientMQTT.publish("openbci", jsonStr.c_str());
        jsonStr = "";
      }
    }

    digitalWrite(5, LOW);

    // clientTCP.write(outputBuf, BYTES_PER_SPI_PACKET * packetsToSend);
    lastSendToClient = micros();
  }
}

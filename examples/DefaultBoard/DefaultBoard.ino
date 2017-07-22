#define ADS1299_VREF 4.5
#define MCP3912_VREF 1.2
#define ADC_24BIT_RES 8388607.0
#define ADC_24BIT_RES_NANO_VOLT 8388607000000000.0
#define BYTES_PER_SPI_PACKET 32
#define BYTES_PER_OBCI_PACKET 33
#define DEBUG 1
#define MAX_SRV_CLIENTS 2
// #define NUM_PACKETS_IN_RING_BUFFER 45
#define NUM_PACKETS_IN_RING_BUFFER 50
#define NUM_PACKETS_IN_RING_BUFFER_JSON 50
// #define MAX_PACKETS_PER_SEND_TCP 20
#define MAX_PACKETS_PER_SEND_TCP 25
#define WIFI_SPI_MSG_LAST 0x01
#define WIFI_SPI_MSG_MULTI 0x02
#define WIFI_SPI_MSG_GAINS 0x03
#define MAX_CHANNELS 16
#define MAX_CHANNELS_PER_PACKET 8
#define NUM_CHANNELS_CYTON 8
#define NUM_CHANNELS_CYTON_DAISY 16
#define NUM_CHANNELS_GANGLION 4
// #define bit(b) (1UL << (b)) // Taken directly from Arduino.h
// Arduino JSON needs bytes for duplication
// to recalculate visit:
//   https://bblanchon.github.io/ArduinoJson/assistant/index.html
// #define ARDUINOJSON_USE_DOUBLE 1
#define ARDUINOJSON_USE_LONG_LONG 1
#define ARDUINOJSON_ADDITIONAL_BYTES_4_CHAN 115
#define ARDUINOJSON_ADDITIONAL_BYTES_8_CHAN 195
#define ARDUINOJSON_ADDITIONAL_BYTES_16_CHAN 355
#define ARDUINOJSON_ADDITIONAL_BYTES_24_CHAN 515
#define ARDUINOJSON_ADDITIONAL_BYTES_32_CHAN 675
#define MICROS_IN_SECONDS 1000000
#define SPI_MASTER_POLL_TIMEOUT_MS 100
#define SPI_TIMEOUT_CLIENT_RESPONSE 400
#define SPI_NO_MASTER 401
#define CLIENT_RESPONSE_NO_BODY_IN_POST 402
#define CLIENT_RESPONSE_MISSING_REQUIRED_CMD 403
#define NANO_VOLTS_IN_VOLTS 1000000000.0

#define BOARD_TYPE_CYTON "cyton"
#define BOARD_TYPE_DAISY "daisy"
#define BOARD_TYPE_GANGLION "ganglion"
#define BOARD_TYPE_NONE "none"

#define JSON_BOARD_CONNECTED "board_connected"
#define JSON_BOARD_TYPE "board_type"
#define JSON_COMMAND "command"
#define JSON_CONNECTED "connected"
#define JSON_GAINS "gains"
#define JSON_HEAP "heap"
#define JSON_LATENCY "latency"
#define JSON_MAC "mac"
#define JSON_MQTT_BROKER_ADDR "broker_address"
#define JSON_MQTT_PASSWORD "password"
#define JSON_MQTT_USERNAME "username"
#define JSON_NAME "name"
#define JSON_NUM_CHANNELS "num_channels"
#define JSON_TCP_DELIMITER "delimiter"
#define JSON_TCP_IP "ip"
#define JSON_TCP_OUTPUT "output"
#define JSON_TCP_PORT "port"

#include <time.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
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
  OUTPUT_PROTOCOL_WEB_SOCKETS,
  OUTPUT_PROTOCOL_SERIAL
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
    long *channelData;
    double *raw;
    double *nano_volts;
    unsigned long long timestamp;
} Sample;

boolean clientWaitingForResponse;
boolean clientWaitingForResponseFullfilled;
boolean ntpOffsetSet;
boolean underSelfTest;
boolean spiTXBufferLoaded;
boolean syncingNtp;
boolean tcpDelimiter;
boolean waitingOnNTP;
boolean waitingDaisyPacket;

CLIENT_RESPONSE curClientResponse;

const char *mqttUsername;
const char *mqttPassword;
const char *mqttBrokerAddress;
const char *serverCloudbrain;
const char *serverCloudbrainAuth;

ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;

uint8_t gains[MAX_CHANNELS];
double scaleFactors[MAX_CHANNELS];

IPAddress tcpAddress;

int counter;
int latency;
int tcpPort;

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
///

/**
 * Has the SPI Master polled this device in the past SPI_MASTER_POLL_TIMEOUT_MS
 * @returns [boolean] True if SPI Master has polled within timeout.
 */
boolean hasSpiMaster() {
#ifdef DEBUG
  Serial.printf("millis(): %d < %d \nlastTimeWasPolled:%d\n", millis(), lastTimeWasPolled + SPI_MASTER_POLL_TIMEOUT_MS, lastTimeWasPolled);
#endif
  return millis() < lastTimeWasPolled + SPI_MASTER_POLL_TIMEOUT_MS;
}

String perfectPrintByteHex(uint8_t b) {
  if (b <= 0x0F) {
    return "0" + String(b, HEX);
  } else {
    return String(b, HEX);
  }
}

void printLLNumber(unsigned long long n, uint8_t base) {
  unsigned char buf[16 * sizeof(long)]; // Assumes 8-bit chars.
  unsigned long long i = 0;

  if (n == 0) {
    Serial.print('0');
    return;
  }

  while (n > 0) {
    buf[i++] = n % base;
    n /= base;
  }

  for (; i > 0; i--)
    Serial.print((char) (buf[i - 1] < 10 ?
      '0' + buf[i - 1] :
      'A' + buf[i - 1] - 10));
}

void print(long long n, int base) {
  if (n < 0) Serial.print("-");
  printLLNumber(n, base);
}

void println(long long n, int base) {
  print(n, base);
  Serial.println();
}

void print(unsigned long long n, int base) {
  printLLNumber(n, base);
}

void println(unsigned long long n, int base) {
  print(n, base);
  Serial.println();
}

String getMacLastFourBytes() {
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.softAPmacAddress(mac);
  String macID = perfectPrintByteHex(mac[WL_MAC_ADDR_LENGTH - 2]) +
                 perfectPrintByteHex(mac[WL_MAC_ADDR_LENGTH - 1]);
  macID.toUpperCase();
  return macID;
}

String getMac() {
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.softAPmacAddress(mac);
  String fullMac = perfectPrintByteHex(mac[WL_MAC_ADDR_LENGTH - 6]) + ":" +
                   perfectPrintByteHex(mac[WL_MAC_ADDR_LENGTH - 5]) + ":" +
                   perfectPrintByteHex(mac[WL_MAC_ADDR_LENGTH - 4]) + ":" +
                   perfectPrintByteHex(mac[WL_MAC_ADDR_LENGTH - 3]) + ":" +
                   perfectPrintByteHex(mac[WL_MAC_ADDR_LENGTH - 2]) + ":" +
                   perfectPrintByteHex(mac[WL_MAC_ADDR_LENGTH - 1]);
  fullMac.toUpperCase();
  return fullMac;
}

String getModelNumber() {
  String AP_NameString = "PTW-0001-" + getMacLastFourBytes();

  char AP_NameChar[AP_NameString.length() + 1];
  memset(AP_NameChar, 0, AP_NameString.length() + 1);

  for (int i=0; i<AP_NameString.length(); i++)
    AP_NameChar[i] = AP_NameString.charAt(i);

  return AP_NameString;
}

String getName() {
  String AP_NameString = "OpenBCI-" + getMacLastFourBytes();

  char AP_NameChar[AP_NameString.length() + 1];
  memset(AP_NameChar, 0, AP_NameString.length() + 1);

  for (int i=0; i<AP_NameString.length(); i++)
    AP_NameChar[i] = AP_NameString.charAt(i);

  return AP_NameString;
}

String getOutputMode(OUTPUT_MODE outputMode) {
  switch(outputMode) {
    case OUTPUT_MODE_JSON:
      return "json";
    case OUTPUT_MODE_RAW:
    default:
      return "raw";
  }
}

/**
 * Used to get a pretty description of the board connected to the shield
 */
String getBoardType(uint8_t numChan) {
  switch(numChan) {
    case NUM_CHANNELS_CYTON_DAISY:
      return BOARD_TYPE_DAISY;
    case NUM_CHANNELS_CYTON:
      return BOARD_TYPE_CYTON;
    case NUM_CHANNELS_GANGLION:
      return BOARD_TYPE_GANGLION;
    default:
      return BOARD_TYPE_NONE;
  }
}

String getCurOutputMode() {
  return getOutputMode(curOutputMode);
}

String mqttGetInfo() {
  const size_t bufferSize = JSON_OBJECT_SIZE(4) + 225;
  StaticJsonBuffer<bufferSize> jsonBuffer;
  String json;
  JsonObject& root = jsonBuffer.createObject();
  root[JSON_MQTT_BROKER_ADDR] = String(mqttBrokerAddress);
  root[JSON_CONNECTED] = clientMQTT.connected() ? true : false;
  root[JSON_MQTT_USERNAME] = String(mqttUsername);
  root[JSON_TCP_OUTPUT] = getCurOutputMode();
  root.printTo(json);
  return json;
}

String tcpGetInfo() {
  const size_t bufferSize = JSON_OBJECT_SIZE(5) + 100;
  StaticJsonBuffer<bufferSize> jsonBuffer;
  String json;
  JsonObject& root = jsonBuffer.createObject();
  root[JSON_CONNECTED] = clientTCP.connected() ? true : false;
  root[JSON_TCP_DELIMITER] = tcpDelimiter ? true : false;
  root[JSON_TCP_IP] = tcpAddress.toString();
  root[JSON_TCP_OUTPUT] = getCurOutputMode();
  root[JSON_TCP_PORT] = tcpPort;
  root.printTo(json);
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
unsigned long long ntpGetTime() {
  unsigned long long curTime = time(nullptr);
  return curTime * MICROS_IN_SECONDS;
}

/**
 * Safely get the time, defaults to micros() if ntp is not active.
 */
unsigned long long getTime() {
  if (ntpActive()) {
    unsigned long long boardTime_uS = micros() % MICROS_IN_SECONDS;
    unsigned long long adj = boardTime_uS - ntpOffset;
    if (boardTime_uS < ntpOffset) {
      boardTime_uS += MICROS_IN_SECONDS;
      adj = boardTime_uS - ntpOffset;
    }
    return ntpGetTime() + adj;
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
  if (packetOffset == 0) {
    // Serial.println(getTime());
    sample->timestamp = getTime();
    long temp[numChannels];
    double temp_raw[numChannels];
    double tmp[numChannels];
    sample->channelData = temp;
    sample->nano_volts = tmp;
    sample->raw = temp_raw;

    for (uint8_t i = 0; i < numChannels; i++) {
      sample->channelData[i] = 0;
      sample->nano_volts[i] = 0;
      sample->raw[i] = 0;
    }
  }
  for (uint8_t i = 0; i < MAX_CHANNELS_PER_PACKET; i++) {
    // Zero out the new value
    int raw = 0;
    // Pull out 24bit number
    raw = arr[i*3 + byteOffset] << 16 | arr[i*3 + 1 + byteOffset] << 8 | arr[i*3 + 2 + byteOffset];
    // carry through the sign
    if(bitRead(raw,23) == 1){
      raw |= 0xFF000000;
    } else{
      raw &= 0x00FFFFFF;
    }

    // Serial.println(raw, HEX);
    //

    // int raw_i = (int)raw;
    double raw_d = (double)raw;
    // Serial.printf("%4.10f %d ", raw_d, raw);

    double volts = raw_d * scaleFactors[i + packetOffset];
    double nano_volts = volts * NANO_VOLTS_IN_VOLTS;
    // Serial.printf("%4.10f ", nano_volts);
    // Serial.println((long)nano_volts);

    // if (nano_volts != 0.0 && nano_volts * 2 == nano_volts) {
      // Serial.printf("\nraw_d %4.10f \tvolts %4.10f", raw_d, nano_volts);
    // }
    // Serial.println(nano_volts);

    sample->channelData[i + packetOffset] = (long)nano_volts;
    sample->raw[i + packetOffset] = raw_d;
    sample->nano_volts[i + packetOffset] = nano_volts;

    // Serial.println(sample->channelData[i + packetOffset]);
    // sample->channelData[i + packetOffset] = nano_volts;

    // if (nano_volts < 0) {
    //   if (nano_volts < (-1 * 958994846070.5)) {
    //     Serial.printf("\nneg\nraw_d %4.10f \tvolts %4.10f", raw_d, nano_volts);
    //   }
    // } else {
    //   if (nano_volts > (958994846070.5)) {
    //     Serial.printf("\npos\nraw_d %4.10f \tvolts %4.10f", raw_d, nano_volts);
    //   }
    // }
    // print(sample->channelData[i + packetOffset], DEC);
    // Serial.printf(" %0.4f\n", nano_volts);

  }

  // Serial.printf("Channel 1: %12.4f", sample->channelData[0]); Serial.println(" nV");
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
  const int extraDoubleSpace = 500;
  switch (numChannels) {
    case NUM_CHANNELS_GANGLION:
      return 1014 + extraDoubleSpace;
    case NUM_CHANNELS_CYTON_DAISY:
      return 1062 + extraDoubleSpace;
    case NUM_CHANNELS_CYTON:
    default:
      return 966 + extraDoubleSpace;
  }
}

void gainReset() {
  for (uint8_t i = 0; i < MAX_CHANNELS; i++) {
    gains[i] = 0;
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
      gains[i] = gainGanglion();
      scaleFactors[i] = MCP3912_VREF / gainGanglion() / ADC_24BIT_RES;
    } else {
      gains[i] = gainCyton(raw[byteCounter]); // Save the gains for later sending in /all
      scaleFactors[i] = ADS1299_VREF / gainCyton(raw[byteCounter++]) / ADC_24BIT_RES;
    }
#ifdef DEBUG
    Serial.printf("Channel: %d\n\tscale factor: %.20f\n", i+1, scaleFactors[i]);
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

#ifdef DEBUG
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
#endif
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
// HTTP Rest Helpers
///////////////////////////////////////////////////

/**
 * Returns true if there is no args on the POST request.
 */
boolean noBodyInParam() {
  return server.args() == 0;
}

void serverReturn(int code, String s) {
  digitalWrite(5, HIGH);
  server.send(code, "text/plain", s + "\r\n");
  digitalWrite(5, LOW);
}

void returnOK(String s) {
  serverReturn(200, s);
}

void returnOK(void) {
  returnOK("OK");
}

/**
 * Used to send a response to the client that the board is not attached.
 */
void returnNoSPIMaster() {
  if (lastTimeWasPolled < 1) {
    serverReturn(SPI_NO_MASTER, "Error: No OpenBCI board attached");
  } else {
    serverReturn(SPI_TIMEOUT_CLIENT_RESPONSE, "Error: Lost communication with OpenBCI board");
  }
}

/**
 * Used to send a response to the client that there is no body in the post request.
 */
void returnNoBodyInPost() {
  serverReturn(CLIENT_RESPONSE_NO_BODY_IN_POST, "Error: No body in POST request");
}

/**
 * Return if there is a missing param in the required command
 */
void returnMissingRequiredParam(const char *err) {
  serverReturn(CLIENT_RESPONSE_MISSING_REQUIRED_CMD, String(err));
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

JsonObject& getArgFromArgs(int args) {
  // size_t argBufferSize = JSON_OBJECT_SIZE(args) + (40 * args);
  DynamicJsonBuffer jsonBuffer(JSON_OBJECT_SIZE(args) + (40 * args));
  // const char* json = "{\"port\":13514}";
  JsonObject& root = jsonBuffer.parseObject(server.arg(0));
  return root;
}

JsonObject& getArgFromArgs() {
  return getArgFromArgs(1);
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
void setLatency() {
  if (noBodyInParam()) return returnNoBodyInPost();

  JsonObject& root = getArgFromArgs();

  if (root.containsKey(JSON_LATENCY)) {
    latency = root[JSON_LATENCY];
    returnOK();
  } else {
    returnMissingRequiredParam(JSON_LATENCY);
  }
}

/**
 * Used to set the latency of the system.
 */
void passThroughCommand() {
  if (noBodyInParam()) return returnNoBodyInPost();
  if (!hasSpiMaster()) return returnNoSPIMaster();
  JsonObject& root = getArgFromArgs();

#ifdef DEBUG
  root.printTo(Serial);
#endif
  if (root.containsKey(JSON_COMMAND)) {
    String cmds = root[JSON_COMMAND];
    uint8_t numCmds = uint8_t(cmds.length());
#ifdef DEBUG
    Serial.printf("Got %d chars: ", numCmds);
    for (int i = 0; i < numCmds; i++) {
      Serial.print(cmds.charAt(i));
    }
    Serial.println();Serial.print("cmds ");Serial.println(cmds);
#endif
    if (numCmds > BYTES_PER_SPI_PACKET - 1) {
      return returnFail(501, "Error: Sent more than 31 chars");
    }

    passthroughBuffer[0] = numCmds;
    passthroughPosition = 1;

    for (int i = 1; i < numCmds + 1; i++) {
      passthroughBuffer[i] = cmds.charAt(i-1);
    }
    passthroughPosition += numCmds;

    spiTXBufferLoaded = true;
    timeOfWifiTXBufferLoaded = millis();
    clientWaitingForResponse = true;
    return;
  } else {
    return returnMissingRequiredParam(JSON_COMMAND);
  }
}

void setupSocketWithClient() {
  // Parse args
  if(noBodyInParam()) return returnNoBodyInPost(); // no body
  JsonObject& root = getArgFromArgs(5);
  if (!root.containsKey(JSON_TCP_IP)) return returnMissingRequiredParam(JSON_TCP_IP);
  String tempAddr = root[JSON_TCP_IP];
  if (!tcpAddress.fromString(tempAddr)) {
    return returnFail(505, "Error: unable to parse ip address. Please send as string in octets i.e. xxx.xxx.xxx.xxx");
  }
  if (!root.containsKey(JSON_TCP_PORT)) return returnMissingRequiredParam(JSON_TCP_PORT);
  tcpPort = root[JSON_TCP_PORT];

  if (root.containsKey(JSON_TCP_OUTPUT)) {
    String outputModeStr = root[JSON_TCP_OUTPUT];
    if (outputModeStr.equals(getOutputMode(OUTPUT_MODE_RAW))) {
      curOutputMode = OUTPUT_MODE_RAW;
    } else if (outputModeStr.equals(getOutputMode(OUTPUT_MODE_JSON))) {
      curOutputMode = OUTPUT_MODE_JSON;
    } else {
      return returnFail(506, "Error: '" + String(JSON_TCP_OUTPUT) +"' must be either "+getOutputMode(OUTPUT_MODE_RAW)+" or "+getOutputMode(OUTPUT_MODE_JSON));
    }
#ifdef DEBUG
    Serial.print("Set output mode to "); Serial.println(getCurOutputMode());
#endif
  }

  if (root.containsKey(JSON_LATENCY)) {
    latency = root[JSON_LATENCY];
#ifdef DEBUG
    Serial.print("Set latency to "); Serial.print(latency); Serial.println(" uS");
#endif
  }

  if (root.containsKey(JSON_TCP_DELIMITER)) {
    tcpDelimiter = root[JSON_TCP_DELIMITER];
#ifdef DEBUG
    Serial.print("Will use delimiter:"); Serial.println(tcpDelimiter ? "true" : "false");
#endif
  }

#ifdef DEBUG
  Serial.print("Got ip: "); Serial.println(tcpAddress);
  Serial.print("Got port: "); Serial.println(tcpPort);
  Serial.print("Current uri: "); Serial.println(server.uri());
  Serial.print("Starting socket to host: "); Serial.print(tcpAddress); Serial.print(" on port: "); Serial.println(tcpPort);
#endif

  curOutputProtocol = OUTPUT_PROTOCOL_TCP;

  if (clientTCP.connect(tcpAddress, tcpPort)) {
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
    Serial.println(JSON_CONNECTED);
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
  if(noBodyInParam()) return returnNoBodyInPost(); // no body
  JsonObject& root = getArgFromArgs(5);
  //
  // size_t argBufferSize = JSON_OBJECT_SIZE(3) + 220;
  // DynamicJsonBuffer jsonBuffer(argBufferSize);
  // JsonObject& root = jsonBuffer.parseObject(server.arg(0));
  if (!root.containsKey(JSON_MQTT_USERNAME)) return returnMissingRequiredParam(JSON_MQTT_USERNAME);
  if (!root.containsKey(JSON_MQTT_PASSWORD)) return returnMissingRequiredParam(JSON_MQTT_PASSWORD);
  if (!root.containsKey(JSON_MQTT_BROKER_ADDR)) return returnMissingRequiredParam(JSON_MQTT_BROKER_ADDR);

  if (root.containsKey(JSON_LATENCY)) {
    latency = root[JSON_LATENCY];
#ifdef DEBUG
    Serial.print("Set latency to "); Serial.print(latency); Serial.println(" uS");
#endif
  }

  mqttUsername = root[JSON_MQTT_USERNAME]; // "alongname.alonglastname@getcloudbrain.com"
  mqttPassword = root[JSON_MQTT_PASSWORD]; // "that time when i had a big password"
  mqttBrokerAddress = root[JSON_MQTT_BROKER_ADDR]; // "/the quick brown fox jumped over the lazy dog"

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
  if(noBodyInParam()) return returnNoBodyInPost();
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
    webSocket.sendTXT(num, JSON_CONNECTED);
			USE_SERIAL.printf("[%u] Disconnected!\n", num);
			break;
		case WStype_CONNECTED: {
			IPAddress ip = webSocket.remoteIP(num);
			USE_SERIAL.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

			// send message to client
			webSocket.sendTXT(num, JSON_CONNECTED);
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
  ntpOffsetSet = false;
  underSelfTest = false;
  spiTXBufferLoaded = false;
  syncingNtp = false;
  tcpDelimiter = false;
  waitingDaisyPacket = false;
  waitingOnNTP = false;

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

  jsonStr = "";
  outputString = "";
  mqttUsername = "";
  mqttPassword = "";
  mqttBrokerAddress = "";
  tcpPort = 3000;

  // curOutputMode = OUTPUT_MODE_RAW;
  curOutputMode = OUTPUT_MODE_JSON;
  // curOutputProtocol = OUTPUT_PROTOCOL_MQTT;
  curOutputProtocol = OUTPUT_PROTOCOL_SERIAL;
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
    lastTimeWasPolled = millis();
#ifdef DEBUG
    if (passthroughBuffer[0] > 0) {
      Serial.println("Sent data: 0x");
      for (uint8_t i = 0; i < BYTES_PER_OBCI_PACKET; i++) {
        Serial.print(passthroughBuffer[i],HEX);
      }
      Serial.println();
    }
#endif
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
#ifdef DEBUG
    Serial.println("Under self test start");
#endif
    returnOK();
  });
  server.on("/test/stop", HTTP_GET, [](){
    underSelfTest = false;
#ifdef DEBUG
    Serial.println("Under self test start");
#endif
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
  server.on("/tcp", HTTP_DELETE, []() {
    clientTCP.stop();
    server.send(200, "text/json", tcpGetInfo());
  });

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
    server.send(200, "text/plain", "v0.2.0");
    digitalWrite(5, LOW);
  });

  server.on("/command", HTTP_POST, passThroughCommand);

  server.on("/latency", HTTP_GET, [](){
    returnOK(String(latency).c_str());
  });
  server.on("/latency", HTTP_POST, setLatency);

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
    const size_t argBufferSize = JSON_OBJECT_SIZE(6) + 115;
    DynamicJsonBuffer jsonBuffer(argBufferSize);
    JsonObject& root = jsonBuffer.createObject();
    root[JSON_BOARD_CONNECTED] = hasSpiMaster() ? true : false;
    root[JSON_HEAP] = ESP.getFreeHeap();
    root[JSON_TCP_IP] = WiFi.localIP().toString();
    root[JSON_MAC] = getMac();
    root[JSON_NAME] = getName();
    root[JSON_NUM_CHANNELS] = numChannels;
    String output;
    root.printTo(output);
    server.send(200, "text/json", output);
#ifdef DEBUG
    root.printTo(Serial);
#endif
  });

  server.on("/board", HTTP_GET, [](){
    const size_t argBufferSize = JSON_OBJECT_SIZE(4) + 150 + JSON_ARRAY_SIZE(numChannels);
    DynamicJsonBuffer jsonBuffer(argBufferSize);
    JsonObject& root = jsonBuffer.createObject();
    root[JSON_BOARD_CONNECTED] = hasSpiMaster() ? true : false;
    root[JSON_BOARD_TYPE] = getBoardType(numChannels);
    root[JSON_NUM_CHANNELS] = numChannels;
    JsonArray& gainsArr = root.createNestedArray(JSON_GAINS);
    for (uint8_t i = 0; i < numChannels; i++) {
      gainsArr.add(gains[i]);
    }
    String output;
    root.printTo(output);
    server.send(200, "text/json", output);
#ifdef DEBUG
    root.printTo(Serial);
#endif
  });

  server.on("/wifi", HTTP_DELETE, []() {

#ifdef DEBUG
    Serial.println("Forgetting wifi and restarting");
#endif
    server.send(200, "text/plain", "Forgetting wifi credentials and rebooting");

#ifdef DEBUG
    Serial.println(ESP.eraseConfig());
#else
    ESP.eraseConfig();
#endif
    delay(5000);
    ESP.reset();
  });

  httpUpdater.setup(&server);

  server.begin();
  MDNS.addService("http", "tcp", 80);
  MDNS.addService("ws", "tcp", 81);

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

#ifdef DEBUG
  Serial.println("Spi has master: " + String(hasSpiMaster() ? "true" : "false"));
#endif

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
    unsigned long long curTime = time(nullptr);
    if (ntpLastTimeSeconds == 0) {
      ntpLastTimeSeconds = curTime;
    } else if (ntpLastTimeSeconds < curTime) {
      ntpOffset = micros() % MICROS_IN_SECONDS;
      syncingNtp = false;
      ntpOffsetSet = true;

#ifdef DEBUG
      Serial.print("\nTime set: "); Serial.println(ntpOffset);
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
    if (ntpTimeSyncAttempts > 10) {
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

  if((clientTCP.connected() || clientMQTT.connected() || curOutputProtocol == OUTPUT_PROTOCOL_SERIAL) && (micros() > (lastSendToClient + latency)) && head != tail) {
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
        if (tcpDelimiter) {
          clientTCP.write("\r\n");
        }
      } else if (curOutputProtocol == OUTPUT_PROTOCOL_MQTT) {
        clientMQTT.publish("openbci",(const char*) outputBuf);
      } else {
        Serial.println((const char*)outputBuf);
      }
    } else { // output mode is JSON
      if (packetsToSend < 0) {
        packetsToSend = NUM_PACKETS_IN_RING_BUFFER_JSON + packetsToSend; // for wrap around
      }
      if (packetsToSend > sendJsonMaxPackets()) {
        packetsToSend = sendJsonMaxPackets();
      }

      // Serial.printf("New: PTS: %d h: %d t: %d\n", packetsToSend, head, tail);

      DynamicJsonBuffer jsonSampleBuffer(jsonBufferSize);

      JsonObject& root = jsonSampleBuffer.createObject();
      JsonArray& chunk = root.createNestedArray("chunk");

      root["count"] = counter++;

      for (uint8_t i = 0; i < packetsToSend; i++) {
        if (tail >= NUM_PACKETS_IN_RING_BUFFER_JSON) {
          tail = 0;
        }
        JsonObject& sample = chunk.createNestedObject();
        // sample["timestamp"] = (sampleBuffer + tail)->timestamp;
        sample.set<unsigned long long>("timestamp", (sampleBuffer + tail)->timestamp);

        JsonArray& data = sample.createNestedArray("data");
        for (uint8_t j = 0; j < numChannels; j++) {
          if ((sampleBuffer + tail)->channelData[j] == 0) {
            Serial.printf("\nrawd %4.0f \tnv %4.10f", (sampleBuffer + tail)->raw[j], (sampleBuffer + tail)->nano_volts[j]);
          }
          data.add((sampleBuffer + tail)->channelData[j]);
        }
        tail++;
      }

      if (curOutputProtocol == OUTPUT_PROTOCOL_TCP) {
        // root.printTo(Serial);
        jsonStr = "";
        root.printTo(jsonStr);

        clientTCP.write(jsonStr.c_str());

        if (tcpDelimiter) {
          clientTCP.write("\r\n");
        }

      } else if (curOutputProtocol == OUTPUT_PROTOCOL_MQTT) {
        root.printTo(jsonStr);
        clientMQTT.publish("openbci", jsonStr.c_str());
        jsonStr = "";
      } else {
        root.printTo(jsonStr);
        Serial.println(jsonStr);
        jsonStr = "";
        // root.printTo(Serial);
        Serial.println();
      }
    }

    digitalWrite(5, LOW);

    // clientTCP.write(outputBuf, BYTES_PER_SPI_PACKET * packetsToSend);
    lastSendToClient = micros();
  }
}

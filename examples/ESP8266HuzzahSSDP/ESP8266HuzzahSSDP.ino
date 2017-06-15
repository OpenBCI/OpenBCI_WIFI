#define ADS1299_VREF 4.5
#define ADC_24BIT_RES 8388607.0
#define BYTES_PER_SPI_PACKET 32
#define BYTES_PER_OBCI_PACKET 33
#define DEBUG 1
#define MAX_SRV_CLIENTS 2
#define NUM_PACKETS_IN_RING_BUFFER 250
#define MAX_PACKETS_PER_SEND 150
#define WIFI_SPI_MSG_LAST 0x01
#define WIFI_SPI_MSG_MULTI 0x02
#define WIFI_SPI_MSG_GAINS 0x03
#define MAX_CHANNELS 16
#define NUM_CHANNELS_CYTON 8
#define NUM_CHANNELS_CYTON_DAISY 16
#define NUM_CHANNELS_GANGLION 4
#define ARDUINOJSON_USE_DOUBLE 1 // we need to store 64-bit doubles

#include <time.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266SSDP.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include "SPISlave.h"
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>

// ENUMS
typedef enum OUTPUT_MODE {
  OUTPUT_MODE_RAW,
  OUTPUT_MODE_JSON
};

typedef enum OUTPUT_PROTOCOL {
  OUTPUT_PROTOCOL_NONE,
  OUTPUT_PROTOCOL_TCP,
  OUTPUT_PROTOCOL_MQTT
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

boolean spiTXBufferLoaded;
boolean clientWaitingForResponse;
boolean underSelfTest;

const char serverCloudbrain[] = "mock.getcloudbrain.com";

ESP8266WebServer server(80);

float channelData[MAX_CHANNELS];
float scaleFactors[MAX_CHANNELS];

int counter;
int latency;

OUTPUT_MODE curOutputMode;
OUTPUT_PROTOCOL curOutputProtocol;

StaticJsonBuffer<295> jsonSampleBuffer;

String outputString;

uint8_t gains[MAX_CHANNELS];
uint8_t numChannels;
uint8_t channelsLoaded;
uint8_t outputBuf[MAX_PACKETS_PER_SEND * BYTES_PER_OBCI_PACKET];
uint8_t passthroughBuffer[BYTES_PER_SPI_PACKET];
uint8_t passthroughPosition;
uint8_t ringBuf[NUM_PACKETS_IN_RING_BUFFER][BYTES_PER_OBCI_PACKET];
uint8_t sampleNumber;

unsigned long lastSendToClient;
unsigned long lastHeadMove;
unsigned long lastMQTTConnectAttempt;
unsigned long lastTimeWasPolled;
unsigned long timeOfWifiTXBufferLoaded;

volatile uint8_t head;
volatile uint8_t tail;

WiFiClient clientTCP;
WiFiClient espClient;
PubSubClient clientMQTT(espClient);

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
 * @type {[type]}
 */
double ntpGetTime() {
  double tim = time(nullptr);
  tim *= 1000000;
  unsigned long microTime = micros();
  tim += microTime%1000000;
  return tim;
}

/**
 * Use this to start the sntp time sync
 * @type {Number}
 */
void ntpStart() {
#ifdef DEBUG
  Serial.print("Setting time using SNTP");
#endif
  configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");
}


///////////////////////////////////////////
// DATA PROCESSING BEGIN
///////////////////////////////////////////

void channelDataReset() {
  for (uint8_t i = 0; i < MAX_CHANNELS; i++) {
    channelData[i] = 0.0;
  }
  channelsLoaded = 0;
}

// TODO: finish 24 byte conversion

/**
 * Return true if the channel data array is full
 */
boolean channelDataCompute(uint8_t *arr) {
  const uint8_t byteOffset = 2;
  if (numChannels == NUM_CHANNELS_CYTON_DAISY) {
    // do something awesome
    return true;
  } else {
    for (uint8_t i = 0; i < numChannels; i++) {
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

      channelData[i] = scaleFactors[i] * ((float)raw);
    }
    return true;
  }
}

void gainReset() {
  for (uint8_t i = 0; i < MAX_CHANNELS; i++) {
    gains[i] = 0.0;
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

void gainSet(uint8_t *raw) {
  uint8_t byteCounter = 1;
  numChannels = raw[byteCounter++];

  if (numChannels < NUM_CHANNELS_GANGLION || numChannels > MAX_CHANNELS) {
    return;
  }

  for (uint8_t i = 0; i < numChannels; i++) {
    if (numChannels == NUM_CHANNELS_GANGLION) {
      // do gang related stuffs
      gains[i] = gainGanglion();
    } else {
      gains[i] = gainCyton(raw[byteCounter++]);
      scaleFactors[i] = ADS1299_VREF / gains[i] / ADC_24BIT_RES;
#ifdef DEBUG
      Serial.printf("Channel: %d\n\tgain: %d\n\tscale factor: %.10f\n", i+1, gains[i], scaleFactors[i]);
#endif
    }
  }
}
///////////////////////////////////////////////////
// MQTT
///////////////////////////////////////////////////

void mqttSetup() {
  client.setServer(serverCloudbrain, 1883);
  client.setCallback(callback);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!clientMQTT.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (clientMQTT.connect(getName(), "cloudbrain", "cloudbrain")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      clientMQTT.publish("amq.topic", "hello world");
      // ... and resubscribe
      clientMQTT.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(clientMQTT.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      lastMQTTConnectAttempt = millis();
    }
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

void returnOK(String s) {
  digitalWrite(5, HIGH);
  server.send(200, "text/plain", s);
  digitalWrite(5, LOW);
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
  const size_t argBufferSize = JSON_OBJECT_SIZE(1) + 20;
  DynamicJsonBuffer jsonBuffer(argBufferSize);
  // const char* json = "{\"port\":13514}";
  JsonObject& root = jsonBuffer.parseObject(server.arg(0));
  return root;
}

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

boolean setupSocketWithClient() {
  // Parse args
  if(server.args() == 0) return false;
  JsonObject& root = getArgFromArgs();
  if (!root.containsKey("port")) return false;
  int port = root["port"];

#ifdef DEBUG
  Serial.print("Got port: "); Serial.println(port);
  Serial.print("Current uri: "); Serial.println(server.uri());
  Serial.print("Starting socket to host: "); Serial.print(server.client().remoteIP().toString()); Serial.print(" on port: "); Serial.println(port);
#endif

  if (clientTCP.connect(server.client().remoteIP(), port)) {
#ifdef DEBUG
    Serial.println("Connected to server");
#endif
    clientTCP.setNoDelay(1);
    return true;
  } else {
#ifdef DEBUG
    Serial.println("Failed to connect to server");
#endif
    return false;
  }
}

/**
 * Used to prepare a response in JSON
 */
JsonObject& prepareSampleJSON(JsonBuffer& jsonBuffer) {
  JsonObject& root = jsonBuffer.createObject();
  root.set<double>("timestamp", ntpActive() ? ntpGetTime() : micros());
  JsonArray& data = root.createNestedArray("data");
  for (uint8_t i = 0; i < numChannels; i++) {
    data.add(channelData[i]);
  }
  return root;
}

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

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
}

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

void initializeVariables() {
  clientWaitingForResponse = false;
  spiTXBufferLoaded = false;
  underSelfTest = false;

  counter = 0;
  head = 0;
  lastHeadMove = 0;
  lastMQTTConnectAttempt = 0;
  lastSendToClient = 0;
  lastTimeWasPolled = 0;
  latency = 1000;
  passthroughPosition = 0;
  sampleNumber = 0;
  tail = 0;
  timeOfWifiTXBufferLoaded = 0;

  outputString = "";

  curOutputMode = OUTPUT_MODE_RAW;
  curOutputProtocol = OUTPUT_PROTOCOL_TCP;

  passthroughBufferClear();
  gainReset();
}

void setup() {
  initializeVariables();

  pinMode(5, OUTPUT);


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

  Serial.printf("Starting SSDP...\n");
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
            returnOK(outputString);
            outputString = "";
            break;
          default:
            break;
        }
      }
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
  });

  SPISlave.onDataSent([]() {
// #ifdef DEBUG
//     Serial.println("Sent data");
// #endif
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

  server.on("/output/mode/json", HTTP_GET, [](){
    curOutputMode = OUTPUT_MODE_JSON;
    returnOK();
  });
  server.on("/output/mode/raw", HTTP_GET, [](){
    curOutputMode = OUTPUT_MODE_RAW;
    returnOK();
  });

  server.on("/output/protocol/tcp", HTTP_GET, [](){
    curOutputProtocol = OUTPUT_PROTOCOL_TCP;
    returnOK();
  });
  server.on("/output/protocol/mqtt", HTTP_GET, [](){
    curOutputProtocol = OUTPUT_PROTOCOL_MQTT;
    returnOK();
  });

  // server.on("/data", HTTP_GET, getData);
  server.on("/websocket", HTTP_POST, [](){
    setupSocketWithClient() ? returnOK() : returnFail(500, "Error: Failed to connect to server");
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
  server.on("/latency", HTTP_POST, [](){
    setLatency() ? returnOK() : returnFail(500, "Error: no \'latency\' in json arg");
  });
  server.on("/latency", HTTP_GET, [](){
    server.send(200, "text/plain", String(latency).c_str());
  });
  // server.onNotFound(handleNotFound);
  server.begin();

#ifdef DEBUG
    Serial.printf("Ready!\n");
#endif

}
// unsigned long lastPrint = 0;
void loop() {
  server.handleClient();

  if (curOutputProtocol == OUTPUT_PROTOCOL_MQTT) {
    if (!clientMQTT.connected() && millis() > 5000 + lastMQTTConnectAttempt) {
      reconnect();
    }

    clientMQTT.loop();
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

  if(clientTCP.connected() && (micros() > (lastSendToClient + latency)) && head != tail) {
    // Serial.print("h: "); Serial.print(head); Serial.print(" t: "); Serial.print(tail); Serial.print(" cc: "); Serial.println(client.connected());

    int packetsToSend = head - tail;
    if (packetsToSend < 0) {
      packetsToSend = NUM_PACKETS_IN_RING_BUFFER + packetsToSend; // for wrap around
    }
    if (packetsToSend > (MAX_PACKETS_PER_SEND - 5)) {
      packetsToSend = MAX_PACKETS_PER_SEND - 5;
    }
    // Serial.print("Packets to send: "); Serial.println(packetsToSend);

    int index = 0;
    for (uint8_t i = 0; i < packetsToSend; i++) {
      if (tail >= NUM_PACKETS_IN_RING_BUFFER) {
        tail = 0;
      }
      for (uint8_t j = 0; j < BYTES_PER_OBCI_PACKET; j++) {
        if (curOutputMode == OUTPUT_MODE_JSON) {
          channelDataCompute(ringBuf[tail]);
          JsonObject& root = prepareSampleJSON(jsonSampleBuffer);
          digitalWrite(5, HIGH);
          root.printTo(clientTCP);
          digitalWrite(5, LOW);
        } else {
          outputBuf[index++] = ringBuf[tail][j];
        }
      }
      tail++;
    }

    if (curOutputMode == OUTPUT_MODE_RAW) {
      digitalWrite(5, HIGH);
      clientTCP.write(outputBuf, BYTES_PER_OBCI_PACKET * packetsToSend);
      digitalWrite(5, LOW);
    }

    // clientTCP.write(outputBuf, BYTES_PER_SPI_PACKET * packetsToSend);
    lastSendToClient = micros();
  }
}

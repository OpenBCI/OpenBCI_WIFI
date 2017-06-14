#define BYTES_PER_SPI_PACKET 32
#define BYTES_PER_OBCI_PACKET 33
// #define DEBUG 1
#define MAX_SRV_CLIENTS 2
#define NUM_PACKETS_IN_RING_BUFFER 250
#define MAX_PACKETS_PER_SEND 150
#define WIFI_SPI_MSG_LAST 0x01
#define WIFI_SPI_MSG_MULTI 0x02


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
// #include "WiFiClientPrint.h"

boolean spiTXBufferLoaded;
boolean clientWaitingForResponse;
boolean underSelfTest;

ESP8266WebServer server(80);

int counter;
int latency;

String outputString;

uint8_t ringBuf[NUM_PACKETS_IN_RING_BUFFER][BYTES_PER_OBCI_PACKET];
uint8_t outputBuf[MAX_PACKETS_PER_SEND * BYTES_PER_OBCI_PACKET];
uint8_t passthroughBuffer[BYTES_PER_SPI_PACKET];
uint8_t passthroughPosition;
uint8_t sampleNumber;

unsigned long lastSendToClient;
unsigned long lastHeadMove;
unsigned long lastTimeWasPolled;
unsigned long timeOfWifiTXBufferLoaded;

volatile uint8_t head;
volatile uint8_t tail;

WiFiClient client;


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

  if (client.connect(server.client().remoteIP(), port)) {
#ifdef DEBUG
    Serial.println("Connected to server");
#endif
    client.setNoDelay(1);
    return true;
  } else {
#ifdef DEBUG
    Serial.println("Failed to connect to server");
#endif
    return false;
  }
}

JsonObject& prepareResponse(JsonBuffer& jsonBuffer) {
  JsonObject& root = jsonBuffer.createObject();
  root["counter"] = counter++;
  root["sensor"] = "cyton";
  root["timestamp"] = millis();
  JsonArray& data = root.createNestedArray("data");
  uint8_t sampleCounter = 0;
  // Serial.print("head:"); Serial.print(head); Serial.print(" and tail: "); Serial.println(tail);

  while (tail != head) {
    if (tail >= NUM_PACKETS_IN_RING_BUFFER) {
      tail = 0;
    }

    if (sampleCounter >= MAX_PACKETS_PER_SEND) {
  #ifdef DEBUG
      Serial.print("b h: "); Serial.print(head); Serial.print(" t: "); Serial.println(tail);
  #endif
      return root;
    }
    JsonArray& nestedArray = data.createNestedArray();
    nestedArray.copyFrom(ringBuf[tail]);
    tail++;
    sampleCounter++;
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
  lastSendToClient = 0;
  lastTimeWasPolled = 0;
  latency = 1000;
  passthroughPosition = 0;
  sampleNumber = 0;
  tail = 0;
  timeOfWifiTXBufferLoaded = 0;

  outputString = "";

  passthroughBufferClear();
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
// #ifdef DEBUG
//         Serial.printf("outputString %s\n", outputString.c_str());
// #endif

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

  if(client.connected() && (micros() > (lastSendToClient + latency)) && head != tail) {
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
        outputBuf[index++] = ringBuf[tail][j];
      }
      tail++;
    }
    digitalWrite(5, HIGH);
    client.write(outputBuf, BYTES_PER_OBCI_PACKET * packetsToSend);
    digitalWrite(5, LOW);

    // client.write(outputBuf, BYTES_PER_SPI_PACKET * packetsToSend);
    lastSendToClient = micros();
  }
}

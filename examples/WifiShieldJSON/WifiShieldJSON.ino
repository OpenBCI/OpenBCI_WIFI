#define ARDUINOJSON_USE_LONG_LONG 1
#define ARDUINOJSON_USE_DOUBLE 1
#define ARDUINO_ARCH_ESP8266
#define ESP8266
// #define ARDUINOJSON_ENABLE_ARDUINO_STRING 1
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
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "OpenBCI_Wifi_Definitions.h"
#include "OpenBCI_Wifi.h"

boolean isWaitingOnResetConfirm;
boolean ntpOffsetSet;
boolean underSelfTest;
boolean syncingNtp;
boolean waitingOnNTP;
boolean wifiReset;

ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;

String jsonStr;

uint8_t ntpTimeSyncAttempts;
uint8_t samplesLoaded;

unsigned long lastSendToClient;
unsigned long lastHeadMove;
unsigned long lastMQTTConnectAttempt;
unsigned long ntpLastTimeSeconds;

WiFiClient clientTCP;
WiFiClient espClient;
PubSubClient clientMQTT(espClient);

///////////////////////////////////////////
// Utility functions
///////////////////////////////////////////

boolean mqttConnect(String username, String password) {
  if (clientMQTT.connect(wifi.getName().c_str(), username.c_str(), password.c_str())) {
#ifdef DEBUG
    Serial.println(JSON_CONNECTED);
#endif
    // Once connected, publish an announcement...
    clientMQTT.publish(MQTT_ROUTE_KEY, "{}");
    return true;
  } else {
    // Wait 5 seconds before retrying
    lastMQTTConnectAttempt = millis();
    return false;
  }
}

boolean mqttConnect() {
  if (clientMQTT.connect(wifi.getName().c_str())) {
#ifdef DEBUG
    Serial.println(JSON_CONNECTED);
#endif
    // Once connected, publish an announcement...
    clientMQTT.publish(MQTT_ROUTE_KEY, "{}");
    return true;
  } else {
    // Wait 5 seconds before retrying
    lastMQTTConnectAttempt = millis();
    return false;
  }
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
 * Use this to start the sntp time sync
 * @type {Number}
 */
void ntpStart() {
#ifdef DEBUG
  Serial.println("Setting time using SNTP");
#endif
  configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");
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

void sendHeadersForCORS() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
}

void sendHeadersForOptions() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "POST,DELETE,GET,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.send(200, "text/plain", "");
}

void serverReturn(int code, String s) {
  digitalWrite(LED_NOTIFY, LOW);
  sendHeadersForCORS();
  server.send(code, "text/plain", s + "\r\n");
  digitalWrite(LED_NOTIFY, HIGH);
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
  if (wifi.lastTimeWasPolled < 1) {
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
  serverReturn(code, msg);
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
  DynamicJsonBuffer jsonBuffer(JSON_OBJECT_SIZE(args) + (40 * args));
  JsonObject& root = jsonBuffer.parseObject(server.arg(0));
  return root;
}

JsonObject& getArgFromArgs() {
  return getArgFromArgs(1);
}

/**
 * Used to set the latency of the system.
 */
void setLatency() {
  if (noBodyInParam()) return returnNoBodyInPost();

  JsonObject& root = getArgFromArgs();

  if (root.containsKey(JSON_LATENCY)) {
    wifi.setLatency(root[JSON_LATENCY]);
    returnOK();
  } else {
    returnMissingRequiredParam(JSON_LATENCY);
  }
}

/**
 * Used to set the latency of the system.
 */
void passthroughCommand() {
  if (noBodyInParam()) return returnNoBodyInPost();
  if (!wifi.spiHasMaster()) return returnNoSPIMaster();
  JsonObject& root = getArgFromArgs();

#ifdef DEBUG
  root.printTo(Serial);
#endif
  if (root.containsKey(JSON_COMMAND)) {
    String cmds = root[JSON_COMMAND];
    uint8_t retVal = wifi.passthroughCommands(cmds);
    if (retVal < PASSTHROUGH_PASS) {
      switch(retVal) {
        case PASSTHROUGH_FAIL_TOO_MANY_CHARS:
          return returnFail(501, "Error: Sent more than 31 chars");
        case PASSTHROUGH_FAIL_NO_CHARS:
          return returnFail(505, "Error: No characters found for key 'command'");
        case PASSTHROUGH_FAIL_QUEUE_FILLED:
          return returnFail(503, "Error: Queue is full, please wait 20ms and try again.");
        default:
          return returnFail(504, "Error: Unknown error");
      }
    }
    return;
  } else {
    return returnMissingRequiredParam(JSON_COMMAND);
  }
}

void tcpSetup() {

  // Parse args
  if(noBodyInParam()) return returnNoBodyInPost(); // no body
  JsonObject& root = getArgFromArgs(7);
  if (!root.containsKey(JSON_TCP_IP)) return returnMissingRequiredParam(JSON_TCP_IP);
  String tempAddr = root[JSON_TCP_IP];
  IPAddress tempIPAddr;
  if (!tempIPAddr.fromString(tempAddr)) {
    return returnFail(505, "Error: unable to parse ip address. Please send as string in octets i.e. xxx.xxx.xxx.xxx");
  }
  if (!root.containsKey(JSON_TCP_PORT)) return returnMissingRequiredParam(JSON_TCP_PORT);
  int port = root[JSON_TCP_PORT];
  if (root.containsKey(JSON_TCP_OUTPUT)) {
    String outputModeStr = root[JSON_TCP_OUTPUT];
    if (outputModeStr.equals(wifi.getOutputModeString(wifi.OUTPUT_MODE_RAW))) {
      wifi.setOutputMode(wifi.OUTPUT_MODE_RAW);
    } else if (outputModeStr.equals(wifi.getOutputModeString(wifi.OUTPUT_MODE_JSON))) {
      wifi.setOutputMode(wifi.OUTPUT_MODE_JSON);
    } else {
      return returnFail(506, "Error: '" + String(JSON_TCP_OUTPUT) + "' must be either " + wifi.getOutputModeString(wifi.OUTPUT_MODE_RAW)+" or " + wifi.getOutputModeString(wifi.OUTPUT_MODE_JSON));
    }
#ifdef DEBUG
    Serial.print("Set output mode to "); Serial.println(wifi.getCurOutputModeString());
#endif
  }

  if (root.containsKey(JSON_LATENCY)) {
    int latency = root[JSON_LATENCY];
    wifi.setLatency(latency);
#ifdef DEBUG
    Serial.print("Set latency to "); Serial.print(wifi.getLatency()); Serial.println(" uS");
#endif
  }

  boolean tcpDelimiter = wifi.tcpDelimiter;
  if (root.containsKey(JSON_TCP_DELIMITER)) {
    tcpDelimiter = root[JSON_TCP_DELIMITER];
#ifdef DEBUG
    Serial.print("Will use delimiter:"); Serial.println(wifi.tcpDelimiter ? "true" : "false");
#endif
  }
  wifi.setInfoTCP(tempAddr, port, tcpDelimiter);

  if (root.containsKey(JSON_SAMPLE_NUMBERS)) {
    wifi.jsonHasSampleNumbers = root[JSON_SAMPLE_NUMBERS];
#ifdef DEBUG
    Serial.print("Set jsonHasSampleNumbers to "); Serial.println(wifi.jsonHasSampleNumbers ? String("true") : String("false"));
#endif
  }

  if (root.containsKey(JSON_TIMESTAMPS)) {
    wifi.jsonHasTimeStamps = root[JSON_TIMESTAMPS];
#ifdef DEBUG
    Serial.print("Set jsonHasTimeStamps to "); Serial.println(wifi.jsonHasTimeStamps ? String("true") : String("false"));
#endif
  }

#ifdef DEBUG
  Serial.print("Got ip: "); Serial.println(wifi.tcpAddress.toString());
  Serial.print("Got port: "); Serial.println(wifi.tcpPort);
  Serial.print("Current uri: "); Serial.println(server.uri());
  Serial.print("Starting socket to host: "); Serial.print(wifi.tcpAddress.toString()); Serial.print(" on port: "); Serial.println(wifi.tcpPort);
#endif

  wifi.setOutputProtocol(wifi.OUTPUT_PROTOCOL_TCP);

  // const size_t bufferSize = JSON_OBJECT_SIZE(6) + 40*6;
  // DynamicJsonBuffer jsonBuffer(bufferSize);
  // JsonObject& rootOut = jsonBuffer.createObject();
  sendHeadersForCORS();
  if (clientTCP.connect(wifi.tcpAddress, wifi.tcpPort)) {
#ifdef DEBUG
    Serial.println("Connected to server");
#endif
    clientTCP.setNoDelay(0);
    // wifiPrinter.setClient(clientTCP);
    jsonStr = wifi.getInfoTCP(true);
    // jsonStr = "";
    // rootOut.printTo(jsonStr);
    server.setContentLength(jsonStr.length());
    return server.send(200, RETURN_TEXT_JSON, jsonStr.c_str());
  } else {
#ifdef DEBUG
    Serial.println("Failed to connect to server");
#endif
    jsonStr = wifi.getInfoTCP(false);
    // jsonStr = "";
    // rootOut.printTo(jsonStr);
    server.setContentLength(jsonStr.length());
    return server.send(504, RETURN_TEXT_JSON, jsonStr.c_str());
  }
}

/**
 * Function called on route `/mqtt` with HTTP_POST with body
 * {"username":"user_name", "password": "you_password", "broker_address": "/your.broker.com"}
 */
void mqttSetup() {
  // Parse args
  if(noBodyInParam()) return returnNoBodyInPost(); // no body
  JsonObject& root = getArgFromArgs(25);
  //
  // size_t argBufferSize = JSON_OBJECT_SIZE(3) + 220;
  // DynamicJsonBuffer jsonBuffer(argBufferSize);
  // JsonObject& root = jsonBuffer.parseObject(server.arg(0));
  if (!root.containsKey(JSON_MQTT_BROKER_ADDR)) return returnMissingRequiredParam(JSON_MQTT_BROKER_ADDR);

  String mqttUsername = "";
  if (root.containsKey(JSON_MQTT_USERNAME)) {
    mqttUsername = root.get<String>(JSON_MQTT_USERNAME);
  }
  String mqttPassword = "";
  if (root.containsKey(JSON_MQTT_PASSWORD)) {
    mqttPassword = root.get<String>(JSON_MQTT_PASSWORD);
  }

  int mqttPort = wifi.mqttPort;
  if (root.containsKey(JSON_MQTT_PORT)) {
    mqttPort = root.get<int>(JSON_MQTT_PORT);
#ifdef DEBUG
    Serial.print("Set mqtt port to "); Serial.println(wifi.mqttPort);
#endif
  }

  if (root.containsKey(JSON_LATENCY)) {
    wifi.setLatency(root[JSON_LATENCY]);
#ifdef DEBUG
    Serial.print("Set latency to "); Serial.print(wifi.getLatency()); Serial.println(" uS");
#endif
  }

  if (root.containsKey(JSON_TCP_OUTPUT)) {
    String outputModeStr = root[JSON_TCP_OUTPUT];
    if (outputModeStr.equals(wifi.getOutputModeString(wifi.OUTPUT_MODE_RAW))) {
      wifi.setOutputMode(wifi.OUTPUT_MODE_RAW);
    } else if (outputModeStr.equals(wifi.getOutputModeString(wifi.OUTPUT_MODE_JSON))) {
      wifi.setOutputMode(wifi.OUTPUT_MODE_JSON);
    } else {
      return returnFail(506, "Error: '" + String(JSON_TCP_OUTPUT) + "' must be either " + wifi.getOutputModeString(wifi.OUTPUT_MODE_RAW)+" or " + wifi.getOutputModeString(wifi.OUTPUT_MODE_JSON));
    }
#ifdef DEBUG
    Serial.print("Set output mode to "); Serial.println(wifi.getCurOutputModeString());
#endif
  }

  if (root.containsKey(JSON_SAMPLE_NUMBERS)) {
    wifi.jsonHasSampleNumbers = root[JSON_SAMPLE_NUMBERS];
#ifdef DEBUG
    Serial.print("Set jsonHasSampleNumbers to "); Serial.println(wifi.jsonHasSampleNumbers ? String("true") : String("false"));
#endif
  }

  if (root.containsKey(JSON_TIMESTAMPS)) {
    wifi.jsonHasTimeStamps = root[JSON_TIMESTAMPS];
#ifdef DEBUG
    Serial.print("Set jsonHasTimeStamps to "); Serial.println(wifi.jsonHasTimeStamps ? String("true") : String("false"));
#endif
  }

  String mqttBrokerAddress = root[JSON_MQTT_BROKER_ADDR]; // "/the quick brown fox jumped over the lazy dog"

#ifdef DEBUG
  Serial.print("Got username: "); Serial.println(mqttUsername);
  Serial.print("Got password: "); Serial.println(mqttPassword);
  Serial.print("Got broker_address: "); Serial.println(mqttBrokerAddress);

  Serial.println("About to try and connect to cloudbrain MQTT server");
#endif

  wifi.setInfoMQTT(mqttBrokerAddress, mqttUsername, mqttPassword, mqttPort);
  clientMQTT.setServer(wifi.mqttBrokerAddress.c_str(), wifi.mqttPort);
  boolean connected = false;
  if (mqttUsername.equals("")) {
#ifdef DEBUG
    Serial.println("No auth approach");
#endif
    connected = mqttConnect();
  } else {
#ifdef DEBUG
    Serial.println("Auth approach being taken");
#endif
    connected = mqttConnect(mqttUsername, mqttPassword);
  }
  sendHeadersForCORS();
  if (connected) {
    return server.send(200, RETURN_TEXT_JSON, wifi.getInfoMQTT(true));
  } else {
    return server.send(505, RETURN_TEXT_JSON, wifi.getInfoMQTT(false));
  }
}

void removeWifiAPInfo() {
  // wifi.curClientResponse = wifi.CLIENT_RESPONSE_OUTPUT_STRING;
  // wifi.outputString = "Forgetting wifi credentials and rebooting";
  // wifi.clientWaitingForResponseFullfilled = true;

#ifdef DEBUG
  Serial.println(wifi.outputString);
  Serial.println(ESP.eraseConfig());
#else
  ESP.eraseConfig();
#endif
  delay(1000);
  ESP.reset();
  delay(1000);
}

void rawBufferFlush(uint8_t bufNum) {
  (wifi.rawBuffer + bufNum)->flushing = true;
  digitalWrite(LED_NOTIFY, LOW);
  if (wifi.curOutputProtocol == wifi.OUTPUT_PROTOCOL_TCP) {
    clientTCP.write((wifi.rawBuffer + bufNum)->data, (wifi.rawBuffer + bufNum)->positionWrite);
    if (wifi.tcpDelimiter) {
      clientTCP.write("\r\n");
    }
  } else if (wifi.curOutputProtocol == wifi.OUTPUT_PROTOCOL_MQTT) {
    clientMQTT.publish(MQTT_ROUTE_KEY, (const char*)(wifi.rawBuffer + bufNum)->data);
  } else {
#ifdef DEBUG
    for (int j = 0; j < (wifi.rawBuffer + bufNum)->positionWrite; j++) {
      Serial.write((wifi.rawBuffer + bufNum)->data[j]);
    }
#endif
  }
  wifi.rawBufferReset(wifi.rawBuffer + bufNum);
  lastSendToClient = micros();
  digitalWrite(LED_NOTIFY, HIGH);
  (wifi.rawBuffer + bufNum)->flushing = false;
}

void initializeVariables() {
  isWaitingOnResetConfirm = false;
  ntpOffsetSet = false;
  underSelfTest = false;
  syncingNtp = false;
  waitingOnNTP = false;
  wifiReset = false;

  lastHeadMove = 0;
  lastMQTTConnectAttempt = 0;
  lastSendToClient = 0;
  ntpLastTimeSeconds = 0;
  ntpTimeSyncAttempts = 0;
  samplesLoaded = 0;

  jsonStr = "";
}

void setup() {
  initializeVariables();

#ifdef DEBUG
  Serial.begin(230400);
  Serial.setDebugOutput(true);
  Serial.println("Serial started");
#endif

  wifi.begin();

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
  wifiManager.autoConnect(wifi.getName().c_str());

#ifdef DEBUG
  Serial.printf("Turning LED Notify light on\nStarting ntp...\n");
#endif

  digitalWrite(LED_NOTIFY, HIGH);

  wifi.ntpStart();

#ifdef DEBUG
  Serial.printf("Starting SSDP...\n");
#endif
  SSDP.setSchemaURL("description.xml");
  SSDP.setHTTPPort(80);
  SSDP.setName("PTW - OpenBCI Wifi Shield");
  SSDP.setSerialNumber(wifi.getName());
  SSDP.setURL("index.html");
  SSDP.setModelName(wifi.getModelNumber());
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
    wifi.spiProcessPacket(data);
  });

  SPISlave.onDataSent([]() {
    wifi.spiOnDataSent();
    SPISlave.setData(wifi.passthroughBuffer, BYTES_PER_SPI_PACKET);
  });

  // The master has read the status register
  SPISlave.onStatusSent([]() {
// #ifdef DEBUG
    // Serial.println("Status Sent");
// #endif
    SPISlave.setStatus(209);
  });

  // Setup SPI Slave registers and pins
  SPISlave.begin();

  // Set the status register (if the master reads it, it will read this value)
  SPISlave.setStatus(209);
  SPISlave.setData(wifi.passthroughBuffer, BYTES_PER_SPI_PACKET);

#ifdef DEBUG
  Serial.println("SPI Slave ready");
  printWifiStatus();
  Serial.printf("Starting HTTP...\n");
#endif
  server.on(HTTP_ROUTE, HTTP_GET, [](){
    returnOK("Push The World - Please visit https://app.swaggerhub.com/apis/pushtheworld/openbci-wifi-server/1.3.0 for the latest HTTP requests");
  });
  server.on(HTTP_ROUTE, HTTP_OPTIONS, sendHeadersForOptions);

  server.on(HTTP_ROUTE_CLOUD, HTTP_GET, [](){
    digitalWrite(LED_NOTIFY, LOW);
    sendHeadersForCORS();
    server.send(200, "text/html", "<!DOCTYPE html> html lang=\"en\"> <head><meta http-equiv=\"refresh\"content=\"0; url=https://app.exocortex.ai\"/><title>Redirecting ...</title></head></html>");
    digitalWrite(LED_NOTIFY, HIGH);
  });
  server.on(HTTP_ROUTE_CLOUD, HTTP_OPTIONS, sendHeadersForOptions);

  server.on("/index.html", HTTP_GET, [](){
    returnOK("Push The World - OpenBCI - Wifi bridge - is up and running woo");
  });

  server.on("/description.xml", HTTP_GET, [](){
#ifdef DEBUG
    Serial.println("SSDP HIT");
#endif
    digitalWrite(LED_NOTIFY, LOW);
    SSDP.schema(server.client());
    digitalWrite(LED_NOTIFY, HIGH);
  });
  server.on(HTTP_ROUTE_YT, HTTP_GET, [](){
    returnOK("Keep going! Push The World!");
  });
  server.on(HTTP_ROUTE_YT, HTTP_OPTIONS, sendHeadersForOptions);

  server.on(HTTP_ROUTE_OUTPUT_JSON, HTTP_GET, [](){
    wifi.setOutputMode(wifi.OUTPUT_MODE_JSON);
    returnOK();
  });
  server.on(HTTP_ROUTE_OUTPUT_JSON, HTTP_OPTIONS, sendHeadersForOptions);

  server.on(HTTP_ROUTE_OUTPUT_RAW, HTTP_GET, [](){
    wifi.setOutputMode(wifi.OUTPUT_MODE_RAW);
    returnOK();
  });
  server.on(HTTP_ROUTE_OUTPUT_RAW, HTTP_OPTIONS, sendHeadersForOptions);


  server.on(HTTP_ROUTE_MQTT, HTTP_GET, []() {
    sendHeadersForCORS();
    server.send(200, RETURN_TEXT_JSON, wifi.getInfoMQTT(clientMQTT.connected()));
  });
  server.on(HTTP_ROUTE_MQTT, HTTP_POST, mqttSetup);
  server.on(HTTP_ROUTE_MQTT, HTTP_OPTIONS, sendHeadersForOptions);

  server.on(HTTP_ROUTE_TCP, HTTP_GET, []() {
    sendHeadersForCORS();
    String out = wifi.getInfoTCP(clientTCP.connected());
    server.setContentLength(out.length());
    server.send(200, "application/json", out.c_str());
  });
  server.on(HTTP_ROUTE_TCP, HTTP_POST, tcpSetup);
  server.on(HTTP_ROUTE_TCP, HTTP_OPTIONS, sendHeadersForOptions);
  server.on(HTTP_ROUTE_TCP, HTTP_DELETE, []() {
    sendHeadersForCORS();
    clientTCP.stop();
    jsonStr = wifi.getInfoTCP(false);
    server.setContentLength(jsonStr.length());
    server.send(200, RETURN_TEXT_JSON, jsonStr.c_str());
    jsonStr = "";
  });

  // These could be helpful...
  server.on(HTTP_ROUTE_STREAM_START, HTTP_GET, []() {
    if (!wifi.spiHasMaster()) return returnNoSPIMaster();
    wifi.passthroughCommands("b");
    SPISlave.setData(wifi.passthroughBuffer, BYTES_PER_SPI_PACKET);
    returnOK();
  });
  server.on(HTTP_ROUTE_STREAM_START, HTTP_OPTIONS, sendHeadersForOptions);

  server.on(HTTP_ROUTE_STREAM_STOP, HTTP_GET, []() {
    if (!wifi.spiHasMaster()) return returnNoSPIMaster();
    wifi.passthroughCommands("s");
    SPISlave.setData(wifi.passthroughBuffer, BYTES_PER_SPI_PACKET);
    returnOK();
  });
  server.on(HTTP_ROUTE_STREAM_STOP, HTTP_OPTIONS, sendHeadersForOptions);

  server.on(HTTP_ROUTE_VERSION, HTTP_GET, [](){
    returnOK(wifi.getVersion());
  });
  server.on(HTTP_ROUTE_VERSION, HTTP_OPTIONS, sendHeadersForOptions);

  server.on(HTTP_ROUTE_COMMAND, HTTP_POST, passthroughCommand);
  server.on(HTTP_ROUTE_COMMAND, HTTP_OPTIONS, sendHeadersForOptions);

  server.on(HTTP_ROUTE_LATENCY, HTTP_GET, [](){
    returnOK(String(wifi.getLatency()).c_str());
  });
  server.on(HTTP_ROUTE_LATENCY, HTTP_POST, setLatency);
  server.on(HTTP_ROUTE_LATENCY, HTTP_OPTIONS, sendHeadersForOptions);

  if (!MDNS.begin(wifi.getName().c_str())) {
#ifdef DEBUG
    Serial.println("Error setting up MDNS responder!");
#endif
  } else {
#ifdef DEBUG
    Serial.print("Your ESP is called "); Serial.println(wifi.getName());
#endif
  }

  server.onNotFound([](){
    returnFail(404, "Route Not Found");
  });

  //get heap status, analog input value and all GPIO statuses in one json call
  server.on(HTTP_ROUTE_ALL, HTTP_GET, [](){
    sendHeadersForCORS();
    String output = wifi.getInfoAll();
    server.setContentLength(output.length());
    server.send(200, RETURN_TEXT_JSON, output);
#ifdef DEBUG
    Serial.println(output);
#endif
  });
  server.on(HTTP_ROUTE_ALL, HTTP_OPTIONS, sendHeadersForOptions);

  server.on(HTTP_ROUTE_BOARD, HTTP_GET, [](){
    sendHeadersForCORS();
    String output = wifi.getInfoBoard();
    server.setContentLength(output.length());
    server.send(200, RETURN_TEXT_JSON, output);
#ifdef DEBUG
    Serial.println(output);
#endif
  });
  server.on(HTTP_ROUTE_BOARD, HTTP_OPTIONS, sendHeadersForOptions);

  server.on(HTTP_ROUTE_WIFI, HTTP_DELETE, []() {
    returnOK("Reseting wifi. Please power cycle your board in 10 seconds");
    wifiReset = true;
  });
  server.on(HTTP_ROUTE_WIFI, HTTP_OPTIONS, sendHeadersForOptions);

  httpUpdater.setup(&server);

  server.begin();
  MDNS.addService("http", "tcp", 80);
  MDNS.addService("ws", "tcp", 81);

#ifdef DEBUG
    Serial.printf("Ready!\n");
#endif

  // Test to see if ntp is good
  if (wifi.ntpActive()) {
    syncingNtp = true;
  } else {
#ifdef DEBUG
    Serial.println("Unable to get ntp sync");
#endif
    waitingOnNTP = true;
    ntpLastTimeSeconds = millis();
  }

  clientMQTT.setCallback(callbackMQTT);

#ifdef DEBUG
  Serial.println("Spi has master: " + String(wifi.spiHasMaster() ? "true" : "false"));
#endif

}

/////////////////////////////////
/////////////////////////////////
// LOOP LOOP LOOP LOOP LOOP /////
/////////////////////////////////
/////////////////////////////////
void loop() {
  server.handleClient();

  if (wifiReset) {
    wifiReset = false;
    delay(1000);
    WiFi.disconnect();
    delay(1000);
    ESP.reset();
    delay(1000);
  }

  if (wifi.curOutputProtocol == wifi.OUTPUT_PROTOCOL_MQTT) {
    if (clientMQTT.connected()) {
      clientMQTT.loop();
    } else if (millis() > 5000 + lastMQTTConnectAttempt) {
      mqttConnect(wifi.mqttUsername, wifi.mqttPassword);
    }
  }

  if (syncingNtp) {
    unsigned long long curTime = time(nullptr);
    if (ntpLastTimeSeconds == 0) {
      ntpLastTimeSeconds = curTime;
    } else if (ntpLastTimeSeconds < curTime) {
      wifi.setNTPOffset(micros() % MICROS_IN_SECONDS);
      syncingNtp = false;
      ntpOffsetSet = true;

#ifdef DEBUG
      Serial.print("\nTime set: "); Serial.println(wifi.getNTPOffset());
#endif
    }
  }

  if (waitingOnNTP && (millis() > 3000 + ntpLastTimeSeconds)) {
    // Test to see if ntp is good
    if (wifi.ntpActive()) {
      waitingOnNTP = false;
      syncingNtp = true;
      ntpLastTimeSeconds = 0;
    }
    ntpTimeSyncAttempts++;
    if (ntpTimeSyncAttempts > 100) {
#ifdef DEBUG
      Serial.println("Unable to get ntp sync");
#endif
      waitingOnNTP = false;
    } else {
      ntpLastTimeSeconds = millis();
    }
  }

  if (wifi.clientWaitingForResponseFullfilled) {
    wifi.clientWaitingForResponseFullfilled = false;
    switch (wifi.curClientResponse) {
      case wifi.CLIENT_RESPONSE_OUTPUT_STRING:
        returnOK(wifi.outputString);
        wifi.outputString = "";
        break;
      case wifi.CLIENT_RESPONSE_NONE:
      default:
        returnOK();
        break;
    }
  }

  if (wifi.clientWaitingForResponse && (millis() > (wifi.timePassthroughBufferLoaded + 2000))) {
    wifi.clientWaitingForResponse = false;
    returnFail(502, "Error: timeout getting command response, be sure board is fully connected");
    wifi.outputString = "";
#ifdef DEBUG
    Serial.println("Failed to get response in 1000ms");
#endif
  }

  if((clientTCP.connected() || clientMQTT.connected() || wifi.curOutputProtocol == wifi.OUTPUT_PROTOCOL_SERIAL) && (micros() > (lastSendToClient + wifi.getLatency()))) {
    if (wifi.curOutputMode == wifi.OUTPUT_MODE_RAW) {
      // If the first buffer is being loaded
      unsigned long nowish = millis();
      if (wifi.curRawBuffer == wifi.rawBuffer) {
        // Does the other raw buffer have data in it? Maybe it's ready to flush?
        if (wifi.rawBufferHasData(wifi.rawBuffer + 1)) {
          rawBufferFlush(1);
          Serial.printf("t-1: %lu\n",millis()-nowish);
        }

        // Welp... might as well see if this buffer has data in it...
        // } else if (wifi.rawBufferHasData(wifi.rawBuffer)) {
          // rawBufferFlush(0);
        // }
      // So curRawBuffer is the second buffer...
      } else {
        // Does the first raw buffer have data in it?
        if (wifi.rawBufferHasData(wifi.rawBuffer)) {
          rawBufferFlush(0);
          Serial.printf("t-0: %lu\n",millis()-nowish);
        }

        // Crap, so the current buffer.. maybe there is data in it?
        // } else if (wifi.rawBufferHasData(wifi.rawBuffer)) {
          // rawBufferFlush(1);
        // }
      }
    } else { // output mode is JSON
      int packetsToSend = wifi.head - wifi.tail;
      if (packetsToSend < 0) {
        packetsToSend = NUM_PACKETS_IN_RING_BUFFER_JSON + packetsToSend; // for wrap around
      }
      if (packetsToSend > wifi.getJSONMaxPackets()) {
        packetsToSend = wifi.getJSONMaxPackets();
      }

      if (packetsToSend > 0) {
        digitalWrite(LED_NOTIFY, LOW);

        DynamicJsonBuffer jsonSampleBuffer(wifi.getJSONBufferSize());
        JsonObject& root = jsonSampleBuffer.createObject();

        wifi.getJSONFromSamples(root, wifi.getNumChannels(), packetsToSend);


        if (wifi.curOutputProtocol == wifi.OUTPUT_PROTOCOL_TCP) {
          root.printTo(jsonStr);
          clientTCP.write(jsonStr.c_str());
          if (wifi.tcpDelimiter) {
            clientTCP.write("\r\n");
          }
          jsonStr = "";
          // root.printTo(wifiPrinter);
          // if (wifi.tcpDelimiter) {
          //   wifiPrinter.write('\r');
          //   wifiPrinter.write('\n');
          // }
          // wifiPrinter.flush();
        } else if (wifi.curOutputProtocol == wifi.OUTPUT_PROTOCOL_MQTT) {
          jsonStr = "";
          root.printTo(jsonStr);
          clientMQTT.publish(MQTT_ROUTE_KEY, jsonStr.c_str());
          jsonStr = "";

          // root.printTo(wifiPrinter);
          // wifiPrinter.flush();
        } else {
          root.printTo(jsonStr);
          jsonStr = "";
          // root.printTo(Serial);
#ifdef DEBUG
          Serial.println();
#endif
        }
        lastSendToClient = micros();
        digitalWrite(LED_NOTIFY, HIGH);
      }
    }
  }
  // delay(0);
}

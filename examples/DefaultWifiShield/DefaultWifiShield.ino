#define ARDUINO_ARCH_ESP8266
#define ESP8266
#define BUFFER_SIZE 1440

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266SSDP.h>
#include <WiFiManager.h>
#include "SPISlave.h"
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include "OpenBCI_Wifi_Definitions.h"
#include "OpenBCI_Wifi.h"

boolean startWifiManager;
boolean underSelfTest;
boolean tryConnectToAP;
boolean wifiReset;
boolean ledState;

int ledFlashes;
int ledInterval;
unsigned long ledLastFlash;
int udpPort;
IPAddress udpAddress;

ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;

String jsonStr;

unsigned long lastSendToClient;
unsigned long lastHeadMove;
unsigned long wifiConnectTimeout;

WiFiUDP clientUDP;
WiFiClient clientTCP;

uint8_t buffer[1440];
uint32_t bufferPosition = 0;

///////////////////////////////////////////
// Utility functions
///////////////////////////////////////////

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
}

///////////////////////////////////////////////////
// MQTT
///////////////////////////////////////////////////


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

void debugPrintDelete() {
  Serial.println("HTTP DELETE " + server.uri() + " HEAP: " + ESP.getFreeHeap());
}

void debugPrintGet() {
  Serial.println("HTTP GET " + server.uri() + " HEAP: " + ESP.getFreeHeap());
}

void debugPrintPost() {
  Serial.println("HTTP POST " + server.uri() + " HEAP: " + ESP.getFreeHeap());
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

void requestWifiManagerStart() {
#ifdef DEBUG
  debugPrintGet();
#endif
  sendHeadersForCORS();
  // server.send(301, "text/html", "<meta http-equiv=\"refresh\" content=\"1; URL='/'\" />");

  String out = "<!DOCTYPE html><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><html lang=\"en\"><h1 style=\"margin:  auto\;width: 80%\;text-align: center\;\">Push The World</h1><br><p style=\"margin:  auto\;width: 80%\;text-align: center\;\"><a href='http://";
  if (WiFi.localIP().toString().equals("192.168.4.1") || WiFi.localIP().toString().equals("0.0.0.0")) {
    out += "192.168.4.1";
  } else {
    out += WiFi.localIP().toString();
  }
  out += HTTP_ROUTE;
  out += "'>Click to Go To WiFi Manager</a></p><html>";
  server.send(200, "text/html", out);

  ledFlashes = 5;
  ledInterval = 250;
  ledLastFlash = millis();

  startWifiManager = true;
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
  JsonObject& root = getArgFromArgs(8);
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

  if (root.containsKey(JSON_REDUNDANCY)) {
    wifi.redundancy = root[JSON_REDUNDANCY];
#ifdef DEBUG
    Serial.print("Set redundancy to "); Serial.println(wifi.redundancy);
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
    clientTCP.setNoDelay(1);
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

void udpSetup() {
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

  wifi.setOutputMode(wifi.OUTPUT_MODE_RAW);
  if (root.containsKey(JSON_TCP_OUTPUT)) {
    String outputModeStr = root[JSON_TCP_OUTPUT];
    if (outputModeStr.equals(wifi.getOutputModeString(wifi.OUTPUT_MODE_RAW))) {
    } else if (outputModeStr.equals(wifi.getOutputModeString(wifi.OUTPUT_MODE_JSON))) {
      wifi.setOutputMode(wifi.OUTPUT_MODE_JSON);
    } else {
      return returnFail(506, "Error: '" + String(JSON_TCP_OUTPUT) + "' must be either " + wifi.getOutputModeString(wifi.OUTPUT_MODE_RAW)+" or " + wifi.getOutputModeString(wifi.OUTPUT_MODE_JSON));
    }
  #ifdef DEBUG
    Serial.print("Set output mode to "); Serial.println(wifi.getCurOutputModeString());
  #endif
  }


  if (root.containsKey(JSON_REDUNDANCY)) {
    wifi.redundancy = root[JSON_REDUNDANCY];
#ifdef DEBUG
    Serial.print("Set redundancy to "); Serial.println(wifi.redundancy);
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
  wifi.setInfoUDP(tempAddr, port, tcpDelimiter);

#ifdef DEBUG
  Serial.print("Got ip: "); Serial.println(wifi.tcpAddress.toString());
  Serial.print("Got port: "); Serial.println(wifi.tcpPort);
  Serial.print("Current uri: "); Serial.println(server.uri());
  Serial.print("Ready to write to: "); Serial.print(wifi.tcpAddress.toString()); Serial.print(" on port: "); Serial.println(wifi.tcpPort);
#endif

  sendHeadersForCORS();
  return server.send(200, "text/json", jsonStr.c_str());
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

void initializeVariables() {
  ledState = false;
  startWifiManager = false;
  tryConnectToAP = false;
  underSelfTest = false;
  wifiReset = false;

  lastHeadMove = 0;
  lastSendToClient = 0;
  ledFlashes = 0;
  ledInterval = 300;
  ledLastFlash = millis();
  wifiConnectTimeout = millis();

  jsonStr = "";
}

void setup() {
  initializeVariables();

  WiFi.mode(WIFI_AP_STA);
  // WiFi.mode(WIFI_STA);
  // WiFi.mode(WIFI_AP);

  #ifdef DEBUG
  Serial.begin(230400);
  Serial.setDebugOutput(true);
  Serial.println("Serial started");
  Serial.println("Version: " + String(SOFTWARE_VERSION));
  #endif

  wifi.begin();

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
  // SPISlave.begin();

  // Set the status register (if the master reads it, it will read this value)
  SPISlave.setStatus(209);
  SPISlave.setData(wifi.passthroughBuffer, BYTES_PER_SPI_PACKET);

#ifdef DEBUG
  Serial.println("SPI Slave ready");
  printWifiStatus();
  Serial.printf("Starting HTTP...\n");
#endif

  server.on(HTTP_ROUTE, HTTP_GET, [](){
#ifdef DEBUG
    debugPrintGet();
#endif
    String ip = "192.168.4.1";
    String out = "<!DOCTYPE html><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><html lang=\"en\"><h1 style=\"margin:  auto\;width: 90%\;text-align: center\;\">Push The World</h1><br>";
    if (WiFi.localIP().toString().equals("192.168.4.1") || WiFi.localIP().toString().equals("0.0.0.0")) {
      if (WiFi.SSID().equals("")) {
        out += "<p style=\"margin:  auto\;width: 80%\;text-align: center\;\"><a href='http://";
        out += "192.168.4.1";
        out += HTTP_ROUTE_WIFI_CONFIG;
        out += "'>Click to Configure Wifi</a><br>If the above link does not work type 192.168.4.1/wifi in web browser and press Enter or Go.<br>See updates on issue <a href='https://github.com/OpenBCI/OpenBCI_WIFI/issues/62'>#62</a> on Github.</p><br>";
      } else {
        out += "<p style=\"margin:  auto\;width: 80%\;text-align: center\;\"><a href='http://";
        out += "192.168.4.1";
        out += HTTP_ROUTE_WIFI_CONFIG;
        out += "'>Click to Configure Wifi</a><br>If the above link does not work type 192.168.4.1/wifi in web browser and press Enter or Go.<br>See updates on issue <a href='https://github.com/OpenBCI/OpenBCI_WIFI/issues/62'>#62</a> on Github.</p><br>";
        out += "<p style=\"margin:  auto\;width: 80%\;text-align: center\;\"><a href='http://";
        out += "192.168.4.1";
        out += HTTP_ROUTE_WIFI_DELETE;
        out += "'>Click to Erase Wifi Credentials</a></p><br>";
      }
    } else {
      out += "<p style=\"margin:  auto\;width: 80%\;text-align: center\;\"><a href='http://";
      out += WiFi.localIP().toString();
      out += HTTP_ROUTE_WIFI_CONFIG;
      out += "'>Click to Configure Wifi</a><br>If the above link does not work type ";
      out += WiFi.localIP().toString();
      out += "/wifi in web browser and press Enter or Go.<br>See updates on issue <a href='https://github.com/OpenBCI/OpenBCI_WIFI/issues/62'>#62</a> on Github.</p><br>";
      out += "<p style=\"margin:  auto\;width: 80%\;text-align: center\;\"><a href='http://";
      out += WiFi.localIP().toString();
      out += HTTP_ROUTE_WIFI_DELETE;
      out += "'>Click to Erase Wifi Credentials</a></p><br>";
    }
    out += "<p style=\"margin:  auto\;width: 80%\;text-align: center\;\"><a href='http://";
    if (WiFi.localIP().toString().equals("192.168.4.1") || WiFi.localIP().toString().equals("0.0.0.0")) {
      out += "192.168.4.1/update";
    } else {
      out += WiFi.localIP().toString();
      out += "/update";
    }
    out += "'>Click to Update WiFi Firmware</a></p><br>";
    out += "<p style=\"margin:  auto\;width: 80%\;text-align: center\;\"> Please visit <a href='https://app.swaggerhub.com/apis/pushtheworld/openbci-wifi-server/2.0.0'>Swaggerhub</a> for the latest HTTP endpoints</p><br>";
    out += "<p style=\"margin:  auto\;width: 80%\;text-align: center\;\"> Shield Firmware: " + String(SOFTWARE_VERSION) + "</p></html>";

    server.send(200, "text/html", out);
  });
  server.on(HTTP_ROUTE, HTTP_OPTIONS, sendHeadersForOptions);

  server.on("/description.xml", HTTP_GET, [](){
#ifdef DEBUG
    Serial.println("SSDP HIT");
#endif
    digitalWrite(LED_NOTIFY, LOW);
    SSDP.schema(server.client());
    digitalWrite(LED_NOTIFY, HIGH);
  });
  server.on(HTTP_ROUTE_YT, HTTP_GET, [](){
#ifdef DEBUG
    debugPrintGet();
#endif
    returnOK("Keep going! Push The World!");
  });
  server.on(HTTP_ROUTE_YT, HTTP_OPTIONS, sendHeadersForOptions);

  server.on(HTTP_ROUTE_TCP, HTTP_GET, []() {
#ifdef DEBUG
    debugPrintGet();
#endif
    sendHeadersForCORS();
    String out = wifi.getInfoTCP(clientTCP.connected());
    server.setContentLength(out.length());
    server.send(200, "application/json", out.c_str());
  });
  server.on(HTTP_ROUTE_TCP, HTTP_POST, tcpSetup);
  server.on(HTTP_ROUTE_TCP, HTTP_OPTIONS, sendHeadersForOptions);
  server.on(HTTP_ROUTE_TCP, HTTP_DELETE, []() {
#ifdef DEBUG
    debugPrintDelete();
#endif
    sendHeadersForCORS();
    clientTCP.stop();
    wifi.setOutputProtocol(wifi.OUTPUT_PROTOCOL_NONE);
    jsonStr = wifi.getInfoTCP(false);
    server.setContentLength(jsonStr.length());
    server.send(200, "text/json", jsonStr.c_str());
    jsonStr = "";
  });

  server.on(HTTP_ROUTE_UDP, HTTP_POST, udpSetup);
  server.on(HTTP_ROUTE_UDP, HTTP_OPTIONS, sendHeadersForOptions);
  server.on(HTTP_ROUTE_UDP, HTTP_DELETE, []() {
#ifdef DEBUG
    debugPrintDelete();
#endif
    sendHeadersForCORS();
    wifi.setOutputProtocol(wifi.OUTPUT_PROTOCOL_NONE);
    jsonStr = wifi.getInfoTCP(false);
    server.setContentLength(jsonStr.length());
    server.send(200, RETURN_TEXT_JSON, jsonStr.c_str());
    jsonStr = "";
  });

  // These could be helpful...
  server.on(HTTP_ROUTE_STREAM_START, HTTP_GET, []() {
#ifdef DEBUG
    debugPrintGet();
#endif
    if (!wifi.spiHasMaster()) return returnNoSPIMaster();
    wifi.passthroughCommands("b");
    SPISlave.setData(wifi.passthroughBuffer, BYTES_PER_SPI_PACKET);
    returnOK();
  });
  server.on(HTTP_ROUTE_STREAM_START, HTTP_OPTIONS, sendHeadersForOptions);

  server.on(HTTP_ROUTE_STREAM_STOP, HTTP_GET, []() {
#ifdef DEBUG
    debugPrintGet();
#endif
    if (!wifi.spiHasMaster()) return returnNoSPIMaster();
    wifi.passthroughCommands("s");
    SPISlave.setData(wifi.passthroughBuffer, BYTES_PER_SPI_PACKET);
    returnOK();
  });
  server.on(HTTP_ROUTE_STREAM_STOP, HTTP_OPTIONS, sendHeadersForOptions);

  server.on(HTTP_ROUTE_VERSION, HTTP_GET, [](){
#ifdef DEBUG
    debugPrintGet();
#endif
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
#ifdef DEBUG
    Serial.println("HTTP NOT FOUND " + server.uri());
#endif
    returnFail(404, "Route Not Found");
  });

  //get heap status, analog input value and all GPIO statuses in one json call
  server.on(HTTP_ROUTE_ALL, HTTP_GET, [](){
#ifdef DEBUG
    debugPrintGet();
#endif
    sendHeadersForCORS();
    String output = wifi.getInfoAll();
    server.setContentLength(output.length());
    server.send(200, RETURN_TEXT_JSON, output);
  });
  server.on(HTTP_ROUTE_ALL, HTTP_OPTIONS, sendHeadersForOptions);

  server.on(HTTP_ROUTE_BOARD, HTTP_GET, [](){
#ifdef DEBUG
    debugPrintGet();
#endif
    sendHeadersForCORS();
    String output = wifi.getInfoBoard();
    server.setContentLength(output.length());
    server.send(200, RETURN_TEXT_JSON, output);
  });
  server.on(HTTP_ROUTE_BOARD, HTTP_OPTIONS, sendHeadersForOptions);

  server.on(HTTP_ROUTE_WIFI, HTTP_GET, requestWifiManagerStart);
  server.on(HTTP_ROUTE_WIFI, HTTP_DELETE, []() {
#ifdef DEBUG
    debugPrintDelete();
#endif
    returnOK("Reseting wifi. Please power cycle your board in 10 seconds");
    wifiReset = true;
  });
  server.on(HTTP_ROUTE_WIFI, HTTP_OPTIONS, sendHeadersForOptions);

  server.on(HTTP_ROUTE_WIFI_CONFIG, HTTP_GET, requestWifiManagerStart);
  server.on(HTTP_ROUTE_WIFI_CONFIG, HTTP_OPTIONS, sendHeadersForOptions);

  server.on(HTTP_ROUTE_WIFI_DELETE, HTTP_GET, []() {
#ifdef DEBUG
    debugPrintDelete();
#endif
    returnOK("Reseting wifi. Please power cycle your board in 10 seconds");
    wifiReset = true;
    digitalWrite(LED_NOTIFY, LOW);
  });
  server.on(HTTP_ROUTE_WIFI_DELETE, HTTP_OPTIONS, sendHeadersForOptions);

#ifdef DEBUG
  Serial.printf("Ready!\n");
  Serial.println("Spi has master: " + String(wifi.spiHasMaster() ? "true" : "false"));
#endif

  if (WiFi.SSID().equals("")) {
    WiFi.softAP(wifi.getName().c_str());
    WiFi.mode(WIFI_AP);
#ifdef DEBUG
    Serial.printf("No stored creds, turning wifi into access point with %d bytes on heap\n", ESP.getFreeHeap());
#endif
    httpUpdater.setup(&server);
    server.begin();
    MDNS.addService("http", "tcp", 80);
    SPISlave.begin();
    ledFlashes = 10;
    ledInterval = 100;
    ledLastFlash = millis();
    ledState = false;
    // digitalWrite(LED_NOTIFY, HIGH);
  } else {
    ledState = false;
    ledFlashes = 4;
    ledInterval = 250;
    wifiConnectTimeout = millis();
    tryConnectToAP = true;
#ifdef DEBUG
    Serial.printf("Stored creds, will try to connect for 10 seconds with %d bytes on heap\n", ESP.getFreeHeap());
#endif
  }
#ifdef DEBUG
  Serial.printf("END OF SETUP HEAP: %d\n", ESP.getFreeHeap());
#endif
}

/////////////////////////////////
/////////////////////////////////
// LOOP LOOP LOOP LOOP LOOP /////
/////////////////////////////////
/////////////////////////////////
void loop() {

  if (ledFlashes > 0) {
    if (millis() > (ledLastFlash + ledInterval)) {
      digitalWrite(LED_NOTIFY, ledState ? HIGH : LOW);
      if (ledState) {
        ledFlashes--;
      }
      ledState = !ledState;
      ledLastFlash = millis();
    }
  }

  if (!tryConnectToAP) {
    server.handleClient();
  }

  if (wifiReset) {
#ifdef DEBUG
    Serial.println("WiFi Reset");
#endif
    WiFi.mode(WIFI_STA);
    wifiReset = false;
    delay(1000);
    WiFi.disconnect();
    delay(1000);
    ESP.reset();
    delay(1000);
  }

  if (tryConnectToAP) {
    if (WiFi.status() == WL_CONNECTED) {
      tryConnectToAP = false;
      WiFi.mode(WIFI_STA);
      httpUpdater.setup(&server);
      server.begin();
      MDNS.addService("http", "tcp", 80);
      SPISlave.begin();
      // digitalWrite(LED_NOTIFY, HIGH);
#ifdef DEBUG
      Serial.println("Connected to network, switching to station mode.");
#endif
      ledState = false;
      ledFlashes = 2;
      ledInterval = 500;
      ledLastFlash = millis();
  } else if (millis() > (wifiConnectTimeout + 10000)) {
#ifdef DEBUG
      Serial.printf("Failed to connect to network with %d bytes on head\n", ESP.getFreeHeap());
#endif
      tryConnectToAP = false;
      WiFi.mode(WIFI_AP);
#ifdef DEBUG
      Serial.printf("Started AP with %d bytes on head\n", ESP.getFreeHeap());
#endif
      httpUpdater.setup(&server);
      server.begin();
      MDNS.addService("http", "tcp", 80);
      SPISlave.begin();
      ledFlashes = 10;
      ledInterval = 100;
      ledLastFlash = millis();
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
  if (startWifiManager) {
    startWifiManager = false;

#ifdef DEBUG
    Serial.printf("%d bytes on heap before stopping local server\n", ESP.getFreeHeap());
#endif
    server.stop();
    delay(1);
#ifdef DEBUG
    Serial.printf("%d bytes on after stopping local server\n", ESP.getFreeHeap());
#endif
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
    WiFiManagerParameter custom_text("<p>Powered by Push The World</p>");
    wifiManager.addParameter(&custom_text);
#ifdef DEBUG
    Serial.printf("Start WiFi Config Portal on WiFi Manager with %d bytes on heap\n" , ESP.getFreeHeap());
#endif
    boolean connected = wifiManager.startConfigPortal(wifi.getName().c_str());
#ifdef DEBUG
    if (connected) {
      Serial.printf("Connected to with WiFi Manager with %d bytes on heap\n" , ESP.getFreeHeap());
    } else {
      Serial.printf("Failed to connect with WiFi Manager with %d bytes on heap\n" , ESP.getFreeHeap());
    }
#endif
  }

  int packetsToSend = wifi.rawBufferHead - wifi.rawBufferTail;
  if (packetsToSend < 0) {
    packetsToSend = NUM_PACKETS_IN_RING_BUFFER_RAW + packetsToSend; // for wrap around
  }
  if (packetsToSend > MAX_PACKETS_PER_SEND_TCP) {
    packetsToSend = MAX_PACKETS_PER_SEND_TCP;
  }
  if((clientTCP.connected() || wifi.curOutputProtocol == wifi.OUTPUT_PROTOCOL_SERIAL || wifi.curOutputProtocol == wifi.OUTPUT_PROTOCOL_UDP) && (micros() > (lastSendToClient + wifi.getLatency()) || packetsToSend == MAX_PACKETS_PER_SEND_TCP) && (packetsToSend > 0)) {
    // Serial.printf("LS2C: %lums H: %u T: %u P2S: %d", (micros() - lastSendToClient)/1000, wifi.rawBufferHead, wifi.rawBufferTail, packetsToSend);
    digitalWrite(LED_NOTIFY, LOW);

    uint32_t taily = wifi.rawBufferTail;
    for (uint8_t i = 0; i < packetsToSend; i++) {
      if (taily >= NUM_PACKETS_IN_RING_BUFFER_RAW) {
        taily = 0;
      }
      uint8_t *buf = wifi.rawBuffer[taily];
      uint8_t stopByte = buf[0];
      buffer[bufferPosition++] = STREAM_PACKET_BYTE_START;
      for (int i = 1; i < BYTES_PER_SPI_PACKET; i++) {
        buffer[bufferPosition++] = buf[i];
      }
      buffer[bufferPosition++] = stopByte;
      taily += 1;
    }
    lastSendToClient = micros();
    if (wifi.curOutputProtocol == wifi.OUTPUT_PROTOCOL_TCP) {
      clientTCP.write(buffer, bufferPosition);
    } else if (wifi.curOutputProtocol == wifi.OUTPUT_PROTOCOL_UDP) {
      clientUDP.beginPacket(wifi.tcpAddress, wifi.tcpPort);
      clientUDP.write(buffer, bufferPosition);
      if (clientUDP.endPacket() == 1) {
        // Serial.println(" udp0");
      }
      if (wifi.redundancy) {
        clientUDP.beginPacket(wifi.tcpAddress, wifi.tcpPort);
        clientUDP.write(buffer, bufferPosition);
        if (clientUDP.endPacket() == 1) {
          // Serial.println(" udp1");
        }
        clientUDP.beginPacket(wifi.tcpAddress, wifi.tcpPort);
        clientUDP.write(buffer, bufferPosition);
        if (clientUDP.endPacket() == 1) {
          // Serial.println(" udp2");
        }
      }
    }
    bufferPosition = 0;
    wifi.rawBufferTail = taily;
    digitalWrite(LED_NOTIFY, HIGH);
  }
}

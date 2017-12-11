#define RAW_TO_JSON
#define MQTT
#define MQTT_SECURE
// #define UDP
// #define TCP
#define ARDUINOJSON_USE_LONG_LONG 1
#define ARDUINOJSON_USE_DOUBLE 1
#define ARDUINO_ARCH_ESP8266
#define ESP8266
#include <time.h>
#include <ESP8266WiFi.h>
// #include <DNSServer.h>
#include <ESP8266WebServer.h>
// #include <ESP8266HTTPUpdateServer.h>
#include "SPISlave.h"
// #include <ESP8266mDNS.h>
// #include <WiFiManager.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include "OpenBCI_Wifi_Definitions.h"
#include "OpenBCI_Wifi.h"

boolean isWaitingOnResetConfirm;
boolean ntpOffsetSet;
boolean tryConnectToMQTT;
boolean startWifiManager;
boolean syncingNtp;
boolean waitingOnNTP;
boolean wifiReset;

ESP8266WebServer server(80);
// ESP8266HTTPUpdateServer httpUpdater;

String jsonStr;

uint8_t ntpTimeSyncAttempts;
uint8_t samplesLoaded;

unsigned long lastSendToClient;
unsigned long lastHeadMove;
unsigned long lastMQTTConnectAttempt;
unsigned long ntpLastTimeSeconds;

WiFiClientSecure espClient;
PubSubClient clientMQTT(espClient);

///////////////////////////////////////////
// Utility functions
///////////////////////////////////////////

#ifdef MQTT
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
#endif

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

#ifdef RAW_TO_JSON
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
#endif

///////////////////////////////////////////////////
// HTTP Rest Helpers
///////////////////////////////////////////////////

/**
* Returns true if there is no args on the POST request.
*/
boolean noBodyInParam() {
  return server.args() == 0;
}

// void sendHeadersForCORS() {
//   server.sendHeader("Access-Control-Allow-Origin", "*");
// }

// void sendHeadersForOptions() {
//   server.sendHeader("Access-Control-Allow-Origin", "*");
//   server.sendHeader("Access-Control-Allow-Methods", "POST,DELETE,GET,OPTIONS");
//   server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
//   server.send(200, "text/plain", "");
// }

// void serverReturn(int code, String s) {
//   digitalWrite(LED_NOTIFY, LOW);
//   sendHeadersForCORS();
//   server.send(code, "text/plain", s + "\r\n");
//   digitalWrite(LED_NOTIFY, HIGH);
// }

// void returnOK(String s) {
//   serverReturn(200, s);
// }

// void returnOK(void) {
//   returnOK("OK");
// }

// /**
// * Used to send a response to the client that the board is not attached.
// */
// void returnNoSPIMaster() {
//   if (wifi.lastTimeWasPolled < 1) {
//     serverReturn(SPI_NO_MASTER, "Error: No OpenBCI board attached");
//   } else {
//     serverReturn(SPI_TIMEOUT_CLIENT_RESPONSE, "Error: Lost communication with OpenBCI board");
//   }
// }

/**
* Used to send a response to the client that there is no body in the post request.
*/
// void returnNoBodyInPost() {
//   serverReturn(CLIENT_RESPONSE_NO_BODY_IN_POST, "Error: No body in POST request");
// }

// void requestWifiManager() {
//   startWifiManager = true;
// #ifdef DEBUG
//   Serial.println("/wifi or /wifi/configure");
// #endif
//   sendHeadersForCORS();
//   server.send(301, "text/html", "<meta http-equiv=\"refresh\" content=\"1; URL='/'\" />");
// }

/**
* Return if there is a missing param in the required command
*/
// void returnMissingRequiredParam(const char *err) {
//   serverReturn(CLIENT_RESPONSE_MISSING_REQUIRED_CMD, String(err));
// }
//
// void returnFail(int code, String msg) {
//   serverReturn(code, msg);
// }

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

// JsonObject& getArgFromArgs(int args) {
//   DynamicJsonBuffer jsonBuffer(JSON_OBJECT_SIZE(args) + (40 * args));
//   JsonObject& root = jsonBuffer.parseObject(server.arg(0));
//   return root;
// }

// JsonObject& getArgFromArgs() {
//   return getArgFromArgs(1);
// }

/**
* Used to set the latency of the system.
*/
// void setLatency() {
//   if (noBodyInParam()) return returnNoBodyInPost();
//
//   JsonObject& root = getArgFromArgs();
//
//   if (root.containsKey(JSON_LATENCY)) {
//     wifi.setLatency(root[JSON_LATENCY]);
//     returnOK();
//   } else {
//     returnMissingRequiredParam(JSON_LATENCY);
//   }
// }

/**
* Used to set the latency of the system.
*/
// void passthroughCommand() {
//   if (noBodyInParam()) return returnNoBodyInPost();
//   if (!wifi.spiHasMaster()) return returnNoSPIMaster();
//   JsonObject& root = getArgFromArgs();
//
//   #ifdef DEBUG
//   root.printTo(Serial);
//   #endif
//   if (root.containsKey(JSON_COMMAND)) {
//     String cmds = root[JSON_COMMAND];
//     uint8_t retVal = wifi.passthroughCommands(cmds);
//     if (retVal < PASSTHROUGH_PASS) {
//       switch(retVal) {
//         case PASSTHROUGH_FAIL_TOO_MANY_CHARS:
//           return returnFail(501, "Error: Sent more than 31 chars");
//         case PASSTHROUGH_FAIL_NO_CHARS:
//           return returnFail(505, "Error: No characters found for key 'command'");
//         case PASSTHROUGH_FAIL_QUEUE_FILLED:
//           return returnFail(503, "Error: Queue is full, please wait 20ms and try again.");
//         default:
//           return returnFail(504, "Error: Unknown error");
//       }
//     }
//     return;
//   } else {
//     return returnMissingRequiredParam(JSON_COMMAND);
//   }
// }

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
#endif

  wifi.setInfoMQTT(mqttBrokerAddress, mqttUsername, mqttPassword, mqttPort);

  tryConnectToMQTT = true;
#ifdef DEBUG
  Serial.println("Will eventually try to connect to mqtt");
#endif
}

void removeWifiAPInfo() {
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
  isWaitingOnResetConfirm = false;
  ntpOffsetSet = false;
  tryConnectToMQTT = false;
  startWifiManager = false;
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

#ifdef DEBUG
  Serial.printf("Turning LED Notify light on\nStarting ntp...\n");
#endif

  digitalWrite(LED_NOTIFY, HIGH);

  wifi.ntpStart();

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

  if (WiFi.SSID().equals("")) {
    WiFi.mode(WIFI_AP);
#ifdef DEBUG
    Serial.println("Turning wifi into access point");
#endif
  }
//   server.on(HTTP_ROUTE, HTTP_GET, [](){
//     returnOK("Push The World - Please visit https://app.swaggerhub.com/apis/pushtheworld/openbci-wifi-server/1.3.0 for the latest HTTP requests");
//   });
//   server.on(HTTP_ROUTE, HTTP_OPTIONS, sendHeadersForOptions);
//
//   server.on(HTTP_ROUTE_CLOUD, HTTP_GET, [](){
//     digitalWrite(LED_NOTIFY, LOW);
//     sendHeadersForCORS();
//     server.send(200, "text/html", "<!DOCTYPE html> html lang=\"en\"> <head><meta http-equiv=\"refresh\"content=\"0; url=https://app.exocortex.ai\"/><title>Redirecting ...</title></head></html>");
//     digitalWrite(LED_NOTIFY, HIGH);
//   });
//   server.on(HTTP_ROUTE_CLOUD, HTTP_OPTIONS, sendHeadersForOptions);
//
//   server.on("/index.html", HTTP_GET, [](){
//     returnOK("Push The World - OpenBCI - Wifi bridge - is up and running woo");
//   });
//
  server.on(HTTP_ROUTE_MQTT, HTTP_GET, []() {
    sendHeadersForCORS();
    server.send(200, RETURN_TEXT_JSON, wifi.getInfoMQTT(clientMQTT.connected()));
  });
  server.on(HTTP_ROUTE_MQTT, HTTP_POST, mqttSetup);
  server.on(HTTP_ROUTE_MQTT, HTTP_OPTIONS, sendHeadersForOptions);

//   // These could be helpful...
//   server.on(HTTP_ROUTE_STREAM_START, HTTP_GET, []() {
//     if (!wifi.spiHasMaster()) return returnNoSPIMaster();
//     wifi.passthroughCommands("b");
//     SPISlave.setData(wifi.passthroughBuffer, BYTES_PER_SPI_PACKET);
//     returnOK();
//   });
//   server.on(HTTP_ROUTE_STREAM_START, HTTP_OPTIONS, sendHeadersForOptions);
//
//   server.on(HTTP_ROUTE_STREAM_STOP, HTTP_GET, []() {
//     if (!wifi.spiHasMaster()) return returnNoSPIMaster();
//     wifi.passthroughCommands("s");
//     SPISlave.setData(wifi.passthroughBuffer, BYTES_PER_SPI_PACKET);
//     returnOK();
//   });
//   server.on(HTTP_ROUTE_STREAM_STOP, HTTP_OPTIONS, sendHeadersForOptions);
//
//   server.on(HTTP_ROUTE_VERSION, HTTP_GET, [](){
//     returnOK(wifi.getVersion());
//   });
//   server.on(HTTP_ROUTE_VERSION, HTTP_OPTIONS, sendHeadersForOptions);
//
//   server.on(HTTP_ROUTE_COMMAND, HTTP_POST, passthroughCommand);
//   server.on(HTTP_ROUTE_COMMAND, HTTP_OPTIONS, sendHeadersForOptions);
//
//   server.on(HTTP_ROUTE_LATENCY, HTTP_GET, [](){
//     returnOK(String(wifi.getLatency()).c_str());
//   });
//   server.on(HTTP_ROUTE_LATENCY, HTTP_POST, setLatency);
//   server.on(HTTP_ROUTE_LATENCY, HTTP_OPTIONS, sendHeadersForOptions);
//
//   if (!MDNS.begin(wifi.getName().c_str())) {
// #ifdef DEBUG
//     Serial.println("Error setting up MDNS responder!");
// #endif
//   } else {
// #ifdef DEBUG
//     Serial.print("Your ESP is called "); Serial.println(wifi.getName());
// #endif
//   }
//
//   server.onNotFound([](){
//     returnFail(404, "Route Not Found");
//   });
//
//   //get heap status, analog input value and all GPIO statuses in one json call
//   server.on(HTTP_ROUTE_ALL, HTTP_GET, [](){
//     sendHeadersForCORS();
//     String output = wifi.getInfoAll();
//     server.setContentLength(output.length());
//     server.send(200, RETURN_TEXT_JSON, output);
// #ifdef DEBUG
//     Serial.println(output);
// #endif
//   });
//   server.on(HTTP_ROUTE_ALL, HTTP_OPTIONS, sendHeadersForOptions);
//
//   server.on(HTTP_ROUTE_BOARD, HTTP_GET, [](){
//     sendHeadersForCORS();
//     String output = wifi.getInfoBoard();
//     server.setContentLength(output.length());
//     server.send(200, RETURN_TEXT_JSON, output);
// #ifdef DEBUG
//     Serial.println(output);
// #endif
//   });
//   server.on(HTTP_ROUTE_BOARD, HTTP_OPTIONS, sendHeadersForOptions);
//
//   server.on(HTTP_ROUTE_WIFI, HTTP_DELETE, []() {
//     returnOK("Reseting wifi. Please power cycle your board in 10 seconds");
//     wifiReset = true;
//   });
//   server.on(HTTP_ROUTE_WIFI, HTTP_OPTIONS, sendHeadersForOptions);
//
//   server.on(HTTP_ROUTE_WIFI_CONFIG, HTTP_GET, requestWifiManager);
//   server.on(HTTP_ROUTE_WIFI_CONFIG, HTTP_OPTIONS, sendHeadersForOptions);
//
//   server.on(HTTP_ROUTE_WIFI_DELETE, HTTP_GET, []() {
//     returnOK("Reseting wifi. Please power cycle your board in 10 seconds");
//     wifiReset = true;
//   });
//   server.on(HTTP_ROUTE_WIFI_DELETE, HTTP_OPTIONS, sendHeadersForOptions);

  server.begin();
  // MDNS.addService("http", "tcp", 80);

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

  wifi.mqttUsername = "****";
  wifi.mqttPort = 8883;
  wifi.mqttBrokerAddress = "your.broker.com";
  wifi.mqttPassword = "****";

  wifi.setOutputProtocol(wifi.OUTPUT_PROTOCOL_MQTT);

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
  // server.handleClient();

  if (wifiReset) {
    wifiReset = false;
    delay(1000);
    WiFi.disconnect();
    delay(1000);
    ESP.reset();
    delay(1000);
  }

  // if (wifi.curOutputProtocol == wifi.OUTPUT_PROTOCOL_MQTT) {
  //   if (clientMQTT.connected()) {
  //     clientMQTT.loop();
  //   } else if (millis() > 5000 + lastMQTTConnectAttempt) {
  //     tryConnectToMQTT = true;
  //     Serial.println("About to try and start mqtt");
  //     // mqttConnect(wifi.mqttUsername, wifi.mqttPassword);
  //   }
  // }

  if (syncingNtp) {
    unsigned long long curTime = time(nullptr);
    if (ntpLastTimeSeconds == 0) {
      ntpLastTimeSeconds = curTime;
    } else if (ntpLastTimeSeconds < curTime) {
      wifi.setNTPOffset(micros() % MICROS_IN_SECONDS);
      syncingNtp = false;
      ntpOffsetSet = true;
      tryConnectToMQTT = true;

#ifdef DEBUG
      Serial.print("\nTime set: "); Serial.println(wifi.getNTPOffset());
#endif
    }
  }

  boolean restartServer = false;

  if (tryConnectToMQTT) {
    tryConnectToMQTT = false;

#ifdef DEBUG
    Serial.printf("%d bytes on heap before stopping local server\n", ESP.getFreeHeap());
#endif
    // server.stop();
#ifdef DEBUG
    Serial.printf("%d bytes on after stopping local server\n", ESP.getFreeHeap());
#endif
    clientMQTT.setServer(wifi.mqttBrokerAddress.c_str(), wifi.mqttPort);

    boolean connected = false;
  //   if (wifi.mqttUsername.equals("")) {
  // #ifdef DEBUG
  //     Serial.println("No auth approach");
  // #endif
  //     connected = mqttConnect();
  //   } else {
  // #ifdef DEBUG
  //     Serial.printf("Auth approach being taken with %d bytes in heap\n", ESP.getFreeHeap());
  // #endif
  //     connected = mqttConnect(wifi.mqttBrokerAddress.c_str(), wifi.mqttPassword.c_str());
  //   }
  //
    // const char *name = "/aj_pushtheworld.us:eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzI1NiIsImtpZCI6IlJURkZPRUZDUVRNNE1EWXlSVFpEUWpCRVJVUXlNekUxTkVNeU5VRTJRVUkxUWtWRk1rWXlNQSJ9.eyJuaWNrbmFtZSI6ImFqIiwibmFtZSI6ImFqQHB1c2h0aGV3b3JsZC51cyIsInBpY3R1cmUiOiJodHRwczovL3MuZ3JhdmF0YXIuY29tL2F2YXRhci84M2U1OGFhNjRmODBkNDU4ZmQ5M2YzOGI0NjQ1MjQwMj9zPTQ4MCZyPXBnJmQ9aHR0cHMlM0ElMkYlMkZjZG4uYXV0aDAuY29tJTJGYXZhdGFycyUyRmFqLnBuZyIsInVwZGF0ZWRfYXQiOiIyMDE3LTExLTIwVDAxOjI2OjU0LjY2M1oiLCJlbWFpbCI6ImFqQHB1c2h0aGV3b3JsZC51cyIsImVtYWlsX3ZlcmlmaWVkIjp0cnVlLCJpc3MiOiJodHRwczovL2Nsb3VkYnJhaW4uYXV0aDAuY29tLyIsInN1YiI6ImF1dGgwfDU5ODExOGRiMDM4ZDNiMWZkNjVhODVlYSIsImF1ZCI6ImozSkY3bGlFaXZ6MDlwU0VVeFRiRXBra09CemkzRWpGIiwiaWF0IjoxNTExMTQxMjE0LCJleHAiOjE1MTExNzcyMTR9.I2lMCKhUNMGjuqdZWaRVzC4s26WLWDntUN6Uxjg7g0JSSThkl9-IQ8JX7TTYK5HfWmxDXV-R-SfOCQVym278bptsiQBdkst1ZmbS5JKkRM8kGAOoib1F7UU5sb1uUHViJoF4O0MjETDF2980B71W_K8GjKE8ft5vU3RjpWnS1A9O8OOv1qSVFn6Jxz9P5yPu6IX_JQVLtWTuD3KPp77xuLqy2hOFxLJvrT1JmKr_AXAIPCbh0DAgeZQWwVBpJau78YC3VgK6PLuvzJ5XqJpw1M4BdkI1DXO2VadrBddDRx6CZhiPXYZiLR85Hig-AOlt7eGV5a9fg_Hw7_GO-33T1g";
    connected = clientMQTT.connect("openbci", wifi.mqttUsername.c_str(), wifi.mqttPassword.c_str());

    // sendHeadersForCORS();
    if (connected) {
#ifdef DEBUG
      Serial.printf("Connected to MQTT with %d bytes on heap\n" , ESP.getFreeHeap());
#endif
      // server.send(200, RETURN_TEXT_JSON, "{\"connected\":true}");
    } else {
#ifdef DEBUG
      Serial.printf("Failed to connect to MQTT with %d bytes on heap\n" , ESP.getFreeHeap());
#endif
      // server.send(505, RETURN_TEXT_JSON, "{\"connected\":false}");
    }
    // restartServer = true;
  }

//   if (startWifiManager) {
//     startWifiManager = false;
// #ifdef DEBUG
//     Serial.printf("%d bytes on heap before stopping local server\n", ESP.getFreeHeap());
// #endif
//     server.stop();
// #ifdef DEBUG
//     Serial.printf("%d bytes on after stopping local server\n", ESP.getFreeHeap());
// #endif
//
//     //Local intialization. Once its business is done, there is no need to keep it around
//     WiFiManager wifiManager;
//     WiFiManagerParameter custom_text("<p>Powered by Push The World</p>");
//     wifiManager.addParameter(&custom_text);
// #ifdef DEBUG
//     Serial.printf("Start WiFi Config Portal on WiFi Manager with %d bytes on heap\n" , ESP.getFreeHeap());
// #endif
//     if (!wifiManager.startConfigPortal(wifi.getName().c_str())) {
//       Serial.println("failed to connect and hit timeout");
//       delay(3000);
//       //reset and try again, or maybe put it to deep sleep
//       ESP.reset();
//       delay(5000);
//     }
//
// #ifdef DEBUG
//     Serial.printf("Stop WiFi Manager with %d bytes on heap\n" , ESP.getFreeHeap());
// #endif
//     restartServer = true;
//   }
//
//   if (restartServer) {
//     server.begin();
//   }

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
        // returnOK(wifi.outputString);
        wifi.outputString = "";
        break;
      case wifi.CLIENT_RESPONSE_NONE:
      default:
        // returnOK();
        break;
    }
  }

  if (wifi.clientWaitingForResponse && (millis() > (wifi.timePassthroughBufferLoaded + 2000))) {
    wifi.clientWaitingForResponse = false;
    // returnFail(502, "Error: timeout getting command response, be sure board is fully connected");
    wifi.outputString = "";
#ifdef DEBUG
    Serial.println("Failed to get response in 1000ms");
#endif
  }

  if ((clientMQTT.connected() || wifi.curOutputProtocol == wifi.OUTPUT_PROTOCOL_SERIAL) && (micros() > (lastSendToClient + wifi.getLatency()))) {
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

      if (wifi.curOutputProtocol == wifi.OUTPUT_PROTOCOL_MQTT) {
        jsonStr = "";
        root.printTo(jsonStr);
        clientMQTT.publish(MQTT_ROUTE_KEY, jsonStr.c_str());
      } else {
        jsonStr = "";
        root.printTo(jsonStr);
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

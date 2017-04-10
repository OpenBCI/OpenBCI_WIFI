#define BYTES_PER_PACKET 32
#define DEBUG 1
#define MAX_SRV_CLIENTS 2
#define MAX_PACKETS 80
#define MAX_PACKETS_PER_SEND 70

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266SSDP.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include "SPISlave.h"
#include "WiFiClientPrint.h"

ESP8266WebServer server(80);
WiFiClient client;

const size_t bufferSize = JSON_ARRAY_SIZE(26) + MAX_PACKETS_PER_SEND*JSON_ARRAY_SIZE(32) + JSON_OBJECT_SIZE(4) + 1706;

volatile uint8_t packetCount = 0;
volatile uint8_t maxPacketsPerWrite = 10;
volatile int position = 0;
volatile uint8_t head = 8;
volatile uint8_t tail = 0;

// int sendToClientRateHz = 25;
unsigned long packetIntervalUs = 25000; //(int)(1.0 / (float)sendToClientRateHz * 1000000.0);
unsigned long lastSendToClient = 0;
unsigned long lastFakeSPIRead = 0;
int counter = 0;

uint8_t ringBuf[MAX_PACKETS][BYTES_PER_PACKET];

void configModeCallback (WiFiManager *myWiFiManager) {
  // Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());

  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void returnOK() {
  server.send(200, "text/plain", "");
}

void returnFail(String msg) {
  server.send(500, "text/plain", msg + "\r\n");
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

boolean setupSocketWithClient() {
  if(server.args() == 0) return false;
#ifdef DEBUG
  Serial.print("Server has arg 0: "); Serial.println(server.arg(0));
#endif

  const size_t bufferSize = JSON_OBJECT_SIZE(1) + 20;
  DynamicJsonBuffer jsonBuffer(bufferSize);
  // const char* json = "{\"port\":13514}";
  JsonObject& root = jsonBuffer.parseObject(server.arg(0));
  int port = root["port"];

#ifdef DEBUG
  Serial.print("Got port: "); Serial.println(port);
#endif

  WiFiClient _client = server.client();
#ifdef DEBUG
  Serial.print("Current uri: "); Serial.println(server.uri());
  Serial.print("Starting socket to host: "); Serial.print(_client.remoteIP().toString()); Serial.print(" on port: "); Serial.println(port);
#endif

  if (_client.connect(_client.remoteIP(), port)) {
#ifdef DEBUG
    Serial.println("Connected to server");
    client = _client;
#endif
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
  uint8_t counter = 0;
  // Serial.print("head:"); Serial.print(head); Serial.print(" and tail: "); Serial.println(tail);

  while (tail != head) {
    if (tail >= MAX_PACKETS) {
      tail = 0;
    }

    if (counter >= MAX_PACKETS_PER_SEND) {
  #ifdef DEBUG
      Serial.print("b h: "); Serial.print(head); Serial.print(" t: "); Serial.println(tail);
  #endif
      return root;
    }
    JsonArray& nestedArray = data.createNestedArray();
    nestedArray.copyFrom(ringBuf[tail]);
    tail++;
    counter++;
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

String getName() {
  // WiFi.mode(WIFI_AP);

  // Do a little work to get a unique-ish name. Append the
  // last two bytes of the MAC (HEX'd) to "Thing-":
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.softAPmacAddress(mac);
  String macID = String(mac[WL_MAC_ADDR_LENGTH - 2], HEX) +
                 String(mac[WL_MAC_ADDR_LENGTH - 1], HEX);
  macID.toUpperCase();
  String AP_NameString = "OpenBCI-" + macID;

  char AP_NameChar[AP_NameString.length() + 1];
  memset(AP_NameChar, 0, AP_NameString.length() + 1);

  for (int i=0; i<AP_NameString.length(); i++)
    AP_NameChar[i] = AP_NameString.charAt(i);

  // WiFi.softAP(AP_NameChar, WiFiAPPSK);

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
 * Used to when the /data route is hit
 * @type {[type]}
 */
void getData() {

  DynamicJsonBuffer jsonBuffer(bufferSize);
  JsonObject& object = prepareResponse(jsonBuffer);
  // object.printTo(Serial); Serial.println();

  server.setContentLength(object.measureLength());
  server.send(200, "application/json", "");

  WiFiClientPrint<> p(server.client());
  object.printTo(p);
  p.stop();
}

/**
 * Called when a sensor needs to be sent a command through SPI
 * @type {[type]}
 */
void handleSensorCommand() {
  if(server.args() == 0) return returnFail("BAD ARGS");
  String path = server.arg(0);

  Serial.println(path);

  if(path == "/") {
    returnFail("BAD PATH");
    return;
  }

  // SPISlave.setData(ip.toString().c_str());
}

void setup() {

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
  printWifiStatus();
#endif

  Serial.printf("Starting HTTP...\n");
  server.on("/index.html", HTTP_GET, [](){
    server.send(200, "text/plain", "OpenBCI is in Wifi discovery mode");
  });
  server.on("/description.xml", HTTP_GET, [](){
#ifdef DEBUG
    Serial.println("SSDP HIT");
#endif
    SSDP.schema(server.client());
  });
  server.on("/you-there", HTTP_GET, [](){
    server.send(200, "text/plain", "Keep going AJ! Push The World!");
  });
  server.on("/data", HTTP_GET, getData);
  server.on("/websocket", HTTP_POST, [](){
    setupSocketWithClient() ? returnOK() : returnFail("Error: Failed to connect to server");
    // setupWebSocketWithClient();
    ;
  });
  // TODO: Add route for changing latency
  server.on("/sensor/command", HTTP_POST, [](){ returnOK(); }, handleSensorCommand);
  //
  // server.onNotFound(handleNotFound);
  server.begin();

  Serial.printf("Starting SSDP...\n");
  SSDP.setSchemaURL("description.xml");
  SSDP.setHTTPPort(80);
  SSDP.setName("PTW - OpenBCI Wifi Shield");
  SSDP.setSerialNumber(getName());
  SSDP.setURL("index.html");
  SSDP.setModelName("PTW - OpenBCI Wifi Shield Bridge 2017");
  SSDP.setModelNumber("929000226503");
  SSDP.setModelURL("http://www.openbci.com");
  SSDP.setManufacturer("Push The World LLC");
  SSDP.setManufacturerURL("http://www.pushtheworldllc.com");
  SSDP.begin();

#ifdef DEBUG
    Serial.printf("Ready!\n");
#endif
  // data has been received from the master. Beware that len is always 32
  // and the buffer is autofilled with zeroes if data is less than 32 bytes long
  // It's up to the user to implement protocol for handling data length
  SPISlave.onData([](uint8_t * data, size_t len) {
    if (client.connected()) {
      client.write(data, len);
    }
    // if (head >= MAX_PACKETS) {
    //   head = 0;
    // }
    // for (int i = 0; i < len; i++) {
    //   ringBuf[head][i] = data[i];
    //   // Serial.print(data[i]);
    // }
    // head++;
  });

  SPISlave.onDataSent([]() {
#ifdef DEBUG
    Serial.println("Sent ip");
#endif
    IPAddress ip = WiFi.localIP();
    SPISlave.setData(ip.toString().c_str());
  });

  // Setup SPI Slave registers and pins
  SPISlave.begin();

  // IPAddress ip = WiFi.localIP();
  // Serial.print("IP Address: ");
  // Serial.println(ip);


#ifdef DEBUG
  Serial.println("SPI Slave ready");
#endif

}

void loop() {
  // webSocket.loop();
  server.handleClient();

  if(client.connected()) {
    if (tail != head) {
      DynamicJsonBuffer jsonBuffer(bufferSize);
      JsonObject& object = prepareResponse(jsonBuffer);
      WiFiClientPrint<> p(client);
      object.printTo(p);
      p.stop();
    }
  }
}

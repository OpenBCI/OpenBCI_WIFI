#define BYTES_PER_PACKET 32
#define DEBUG 1
#define MAX_SRV_CLIENTS 2
#define NUM_PACKETS_IN_RING_BUFFER 80
#define MAX_PACKETS_PER_SEND 20

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266SSDP.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include "SPISlave.h"
// #include "WiFiClientPrint.h"

boolean underSelfTest = false;
const size_t bufferSize = JSON_ARRAY_SIZE(MAX_PACKETS_PER_SEND) + MAX_PACKETS_PER_SEND*JSON_ARRAY_SIZE(BYTES_PER_PACKET) + JSON_OBJECT_SIZE(4) + 302;

ESP8266WebServer server(80);

int counter = 0;
int latency = 100;

uint8_t ringBuf[NUM_PACKETS_IN_RING_BUFFER][BYTES_PER_PACKET];
uint8_t outputBuf[MAX_PACKETS_PER_SEND * BYTES_PER_PACKET];
uint8_t sampleNumber = 0;

unsigned long lastSendToClient = 0;
unsigned long lastHeadMove = 0;

volatile uint8_t head = 40;
volatile uint8_t tail = 0;

WiFiClient client;

/**
 * Used when
 */
void configModeCallback (WiFiManager *myWiFiManager) {
#ifdef DEBUG
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
#endif
}

void returnOK() {
  digitalWrite(5, HIGH);
  server.send(200, "text/plain", "");
  digitalWrite(5, LOW);
}

void returnFail(String msg) {
  digitalWrite(5, HIGH);
  server.send(500, "text/plain", msg + "\r\n");
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

boolean setupSocketWithClient() {
  // Parse args
  if(server.args() == 0) return false;
  JsonObject& root = getArgFromArgs();
  if (!root.containsKey("port")) return false;
  int port = root["port"];

#ifdef DEBUG
  Serial.print("Got port: "); Serial.println(port);
#endif

#ifdef DEBUG
  Serial.print("Current uri: "); Serial.println(server.uri());
  Serial.print("Starting socket to host: "); Serial.print(server.client().remoteIP().toString()); Serial.print(" on port: "); Serial.println(port);
#endif

  if (client.connect(server.client().remoteIP(), port)) {
#ifdef DEBUG
    Serial.println("Connected to server");
    client.setNoDelay(1);
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

#ifdef DEBUG
  printWifiStatus();
#endif

  Serial.printf("Starting HTTP...\n");
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
  server.on("/you-there", HTTP_GET, [](){
    digitalWrite(5, HIGH);
    server.send(200, "text/plain", "Keep going AJ! Push The World!");
    digitalWrite(5, LOW);
  });

#ifdef DEBUG
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
#endif

  // server.on("/data", HTTP_GET, getData);
  server.on("/websocket", HTTP_POST, [](){
    setupSocketWithClient() ? returnOK() : returnFail("Error: Failed to connect to server");
  });
  server.on("/latency", HTTP_GET, [](){
    setLatency() ? returnOK() : returnFail("Error: no \'latency\' in json arg");
  });
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
    if (head >= NUM_PACKETS_IN_RING_BUFFER) {
      head = 0;
    }
    for (int i = 0; i < len; i++) {
      ringBuf[head][i] = data[i];
      // Serial.print(data[i]);
    }
    head++;
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

#ifdef DEBUG
  Serial.println("SPI Slave ready");
#endif

}

void loop() {
  server.handleClient();

#ifdef DEBUG
  int sampleIntervaluS = 63; // micro seconds
  boolean daisy = true;
  if (underSelfTest) {
    if (micros() > (lastHeadMove + sampleIntervaluS)) {
      head += daisy ? 2 : 1;
      if (head >= NUM_PACKETS_IN_RING_BUFFER) {
        head -= NUM_PACKETS_IN_RING_BUFFER;
      }
      lastHeadMove = micros();
    }
  }
  // Serial.print("h: "); Serial.print(head); Serial.print(" t: "); Serial.print(tail); Serial.print(" cc: "); Serial.println(client.connected());
#endif

  if(client.connected() && (micros() > (lastSendToClient + latency)) && head != tail) {
    // Serial.print("h: "); Serial.print(head); Serial.print(" t: "); Serial.print(tail); Serial.print(" cc: "); Serial.println(client.connected());

    int packetsToSend = head - tail;
    if (packetsToSend < 0) {
      packetsToSend = NUM_PACKETS_IN_RING_BUFFER + packetsToSend; // for wrap around
    }
    if (packetsToSend > MAX_PACKETS_PER_SEND) {
      packetsToSend = MAX_PACKETS_PER_SEND;
    }
    // Serial.print("Packets to send: "); Serial.println(packetsToSend);

    int index = 0;
    for (uint8_t i = 0; i < packetsToSend; i++) {
      if (tail >= NUM_PACKETS_IN_RING_BUFFER) {
        tail = 0;
      }
      for (uint8_t j = 0; j < BYTES_PER_PACKET; j++) {
        if (j == 1) {
          outputBuf[index] = sampleNumber;
          sampleNumber++;
        } else {
          outputBuf[index] = ringBuf[tail][j];
        }
        index++;
      }
      tail++;
    }
    digitalWrite(5, HIGH);
    client.write(outputBuf, BYTES_PER_PACKET * packetsToSend);
    // Serial.println("YOYOYO");
    digitalWrite(5, LOW);

    // client.write(outputBuf, BYTES_PER_PACKET * packetsToSend);
    lastSendToClient = micros();

    // int sampleCounter = 0;
    // while (tail != head) {
    //   if (tail >= MAX_PACKETS) {
    //     tail = 0;
    //   }
    //   if (sampleCounter >= MAX_PACKETS_PER_SEND) {
    //     break;
    //   }
    //   // Serial.println((const char*)ringBuf[tail]);
    //   client.write(ringBuf[tail]);
    //   tail++;
    //   sampleCounter++;
    // }
    // lastSendToClient = micros();

    // if (tail != head) {
    //   DynamicJsonBuffer jsonBuffer(bufferSize);
    //   JsonObject& object = prepareResponse(jsonBuffer);
    //   WiFiClientPrint<> p(client);
    //   object.printTo(p);
    //   p.stop();
    //   lastSendToClient = micros();
    // }
  }
}

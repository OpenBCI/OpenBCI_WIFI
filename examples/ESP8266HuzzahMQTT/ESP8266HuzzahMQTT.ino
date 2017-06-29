/*
 * 31 mar 2015
 * This sketch display UDP packets coming from an UDP client.
 * On a Mac the NC command can be used to send UDP. (nc -u 192.168.1.101 2390).
 *
 * Configuration : Enter the ssid and password of your Wifi AP. Enter the port number your server is listening on.
 *
 */

 #include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

 //needed for library
 #include <DNSServer.h>
 #include <ESP8266WebServer.h>
 #include <WiFiManager.h>
 #include "SPISlave.h"
 #include <PubSubClient.h>
 // Check out https://github.com/knolleary/pubsubclient/blob/master/examples/mqtt_esp8266/mqtt_esp8266.ino
 // #include "OpenBCI_Wifi.h"

const char* mqtt_server = "mock.getcloudbrain.com";

// Create an instance of the server
// specify the port to listen on as an argument
// WiFiServer server(80);
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

volatile uint8_t packetCount = 0;
volatile uint8_t maxPacketsPerWrite = 10;
const int maxPackets = 100;
volatile int position = 0;
volatile uint8_t head = 0;
volatile uint8_t tail = 0;
const int bytesPerPacket = 32;

int sendToClientRateHz = 25;
unsigned long packetIntervalUs = 2000; //(int)(1.0 / (float)sendToClientRateHz * 1000000.0);
unsigned long lastSendToClient = 0;

uint8_t ringBuf[maxPackets][bytesPerPacket];

boolean packing = false;
boolean clientSet = false;

long lastReconnectAttempt = 0;

boolean reconnect() {
  if (client.connect(mqtt_server, "cloudbrain", "cloudbrain")) {
    Serial.println("MQTT connected");
    // Once connected, publish an announcement...
    client.publish("openbci/node","helloWorld");
    // ... and resubscribe
    client.subscribe("openbci/wifi");
  }
  return client.connected();
}


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void configModeCallback (WiFiManager *myWiFiManager) {
  // Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());

  Serial.println(myWiFiManager->getConfigPortalSSID());
}

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

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  WiFiManagerParameter custom_text("<p>Powered by Push The World</p>");
  wifiManager.addParameter(&custom_text);

  wifiManager.setAPCallback(configModeCallback);

  //fetches ssid and pass from eeprom and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  wifiManager.autoConnect("OpenBCI");
  printWifiStatus();
  // Start server
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // data has been received from the master. Beware that len is always 32
  // and the buffer is autofilled with zeroes if data is less than 32 bytes long
  // It's up to the user to implement protocol for handling data length
  SPISlave.onData([](uint8_t * data, size_t len) {
    if (head >= maxPackets) {
      head = 0;
    }
    for (int i = 0; i < len; i++) {
      ringBuf[head][i] = data[i];
      // Serial.print(data[i]);
    }
    head++;
  });

  // The master has read out outgoing data buffer
  // that buffer can be set with SPISlave.setData
  SPISlave.onDataSent([]() {
    // Serial.println("Answer Sent");
  });

  // status has been received from the master.
  // The status register is a special register that bot the slave and the master can write to and read from.
  // Can be used to exchange small data or status information
  SPISlave.onStatus([](uint32_t data) {
    // Serial.printf("Status: %u\n", data);
    // SPISlave.setStatus(millis()); //set next status
  });

  // The master has read the status register
  SPISlave.onStatusSent([]() {
    // Serial.println("Status Sent");
  });

  // Setup SPI Slave registers and pins
  SPISlave.begin();

  // Set the status register (if the master reads it, it will read this value)
  // SPISlave.setStatus(millis());

  // Sets the data registers. Limited to 32 bytes at a time.
  // SPISlave.setData(uint8_t * data, size_t len); is also available with the same limitation
  // SPISlave.setData("Ask me a question!");

  Serial.println("SPI Slave ready");

  lastReconnectAttempt = 0;

}

void loop() {
  if (!client.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      Serial.println("Attempting reconnect...");
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        Serial.println("Able to connect");
        lastReconnectAttempt = 0;
      } else {
        Serial.println("not able to connect");
      }
    }
  } else {
    // Client connected
    client.loop();
  }

}

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
 #include <WifiUdp.h>
 #include "SPISlave.h"
 // #include "OpenBCI_Wifi.h"

volatile byte pos;
volatile boolean process_it;


const char* ssid = "867";
const char* password = "penthouse867";

const uint8_t maxRand = 91;
const uint8_t minRand = 48;
const int WIFI_BUFFER_LENGTH = 512;

// A UDP instance to let us send and receive packets over UDP
WiFiUDP Udp;
IPAddress client;

// Create an instance of the server
// specify the port to listen on as an argument
// WiFiServer server(80);

// WiFiClient client;
unsigned int localUdpPort = 2390;
// WiFiServer server(80);

volatile uint8_t packetCount = 0;
volatile uint8_t maxPacketsPerWrite = 10;
const int maxPackets = 20;
volatile int position = 0;
volatile int head = 0;
volatile int tail = 0;
const int bytesPerPacket = 32;

uint8_t ringBuf[maxPackets][bytesPerPacket];

boolean packing = false;
boolean clientSet = false;

uint8_t lastSS = 0;
uint8_t lastVal = 0xFF;
uint8_t curOp = 0xFF;

void flushBufferToUDP() {
  Udp.beginPacket(client, 2391);
  for (int j = tail; j < head; j++) {
    Udp.write(ringBuf[j], bytesPerPacket);
  }
  Udp.endPacket();
  if (head == maxPackets) {
    tail = 0;
  } else {
    tail = head;
  }
}

void tryFlushBuffer() {
  if (tail == 0 && head == 10) {
    flushBufferToUDP();
  } else if (tail == 10 && head == 20) {
    flushBufferToUDP();
  }
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
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Udp.begin(localUdpPort);

  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.setDebugOutput(true);

}

void loop() {

  int packetSize = Udp.parsePacket();
  if (packetSize) {
    if (!clientSet) {
      client = Udp.remoteIP();
      clientSet = true;
      Serial.println("client connected");
      Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
      Udp.write("ready fredy");
      Udp.endPacket();
    }
  }
  // If the client is set then try to flush the buffer.
  if (clientSet) {
    tryFlushBuffer();
  }
  if (head >= maxPackets) {
    head = 0;
  } else {
    head++;
  }
}

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
 #include "SPI.h"
 #include "OpenBCI_Wifi.h"

char buf [100];
volatile byte pos;
volatile boolean process_it;

const uint8_t maxRand = 91;
const uint8_t minRand = 48;

unsigned int localPort = 2390;      // local port to listen for UDP packets

byte packetBuffer[512]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP Udp;

IPAddress client;

WiFiServer server(80);

boolean packing = false;
boolean clientSet = false;

uint8_t lastSS = 0;

void setup() {
  // pinMode(0, OUTPUT);
  // digitalWrite(0, LOW);

  Udp.begin(localPort);

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


}

void loop() {

  while (!digitalRead(WIFI_PIN_SLAVE_SELECT)) { // Is there data ready
    uint8_t inByte = xfer(0x00);
    // Mark the last SPI read as now;
    wifi.lastTimeSerialRead = micros();
    // Store it to serial buffer
    wifi.bufferSerialAddChar(inByte);
    // Get one char and process it
    wifi.bufferStreamAddChar((wifi.streamPacketBuffer + wifi.streamPacketBufferHead), inByte);
  }

  if ((wifi.streamPacketBuffer + wifi.streamPacketBufferHead)->state == wifi.STREAM_STATE_READY) { // Is there a stream packet waiting to get sent to the PC?
    if (wifi.bufferStreamTimeout()) {
      // We are sure this is a streaming packet.
      wifi.streamPacketBufferHead++;
      if (wifi.streamPacketBufferHead > (OPENBCI_NUMBER_STREAM_BUFFERS - 1)) {
        wifi.streamPacketBufferHead = 0;
      }
    }
  }

  if ((wifi.streamPacketBuffer + wifi.streamPacketBufferTail)->state == wifi.STREAM_STATE_READY) { // Is there a stream packet waiting to get sent to the Host?
    if (wifi.streamPacketBufferHead != wifi.streamPacketBufferTail) {
      // Try to add the tail to the TX buffer
      if (clientSet) {
        Udp.beginPacket(client,2391);
        Udp.write((wifi.streamPacketBuffer + wifi.streamPacketBufferTail)->data, (wifi.streamPacketBuffer + wifi.streamPacketBufferTail)->bytesIn);
        Udp.endPacket();
        wifi.bufferStreamReset(wifi.streamPacketBuffer + wifi.streamPacketBufferTail);
      }
      wifi.streamPacketBufferTail++;
      if (wifi.streamPacketBufferTail > (OPENBCI_NUMBER_STREAM_BUFFERS - 1)) {
        wifi.streamPacketBufferTail = 0;
      }
    }
  }

  int noBytes = Udp.parsePacket();
  if ( noBytes ) {

    if (!clientSet) {
      client = Udp.remoteIP();
      clientSet = true;
    }

    // Serial.println("client set");

    // We've received a packet, read the data from it
    Udp.read(packetBuffer,noBytes); // read the packet into the buffer

    // display the packet contents in HEX
    for (int i=1;i<=noBytes;i++){
      Serial.write(packetBuffer[i-1]);
    }
  }


}



//SPI communication method
byte xfer(byte _data)
{
    byte inByte;
    inByte = spi.transfer(_data);
    return inByte;
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

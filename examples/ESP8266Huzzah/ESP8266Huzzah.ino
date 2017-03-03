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
 // #include "OpenBCI_Wifi.h"

volatile byte pos;
volatile boolean process_it;

const uint8_t maxRand = 91;
const uint8_t minRand = 48;
//how many clients should be able to connect to this ESP8266
#define MAX_SRV_CLIENTS 2

// A UDP instance to let us send and receive packets over UDP
// WiFiUDP Udp;
// IPAddress client;

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);
WiFiClient serverClients[MAX_SRV_CLIENTS];

unsigned int localUdpPort = 2390;

volatile uint8_t packetCount = 0;
volatile uint8_t maxPacketsPerWrite = 10;
const int maxPackets = 100;
volatile int position = 0;
volatile uint8_t head = 0;
volatile uint8_t tail = 0;
const int bytesPerPacket = 32;

int sendToClientRateHz = 50;
unsigned long packetIntervalUs = (int)(1.0 / (float)sendToClientRateHz * 1000000.0);
unsigned long lastSendToClient = 0;

uint8_t ringBuf[maxPackets][bytesPerPacket];

boolean packing = false;
boolean clientSet = false;

uint8_t lastSS = 0;
uint8_t lastVal = 0xFF;
uint8_t curOp = 0xFF;

void setup() {
  // Start up wifi for OpenBCI
  // wifi.begin(true);
  //
  // Udp.begin(localUdpPort);

  // Boot up the library
  // OpenBCI_Wifi.begin();

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
  server.begin();
  server.setNoDelay(true);

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

}

void loop() {
  uint8_t i;
  //check if there are any new clients
  if (server.hasClient()){
    for(i = 0; i < MAX_SRV_CLIENTS; i++){
      //find free/disconnected spot
      if (!serverClients[i] || !serverClients[i].connected()){
        if(serverClients[i]) serverClients[i].stop();
        serverClients[i] = server.available();
        Serial.print("New client: "); Serial.print(i);
        continue;
      }
    }
    //no free/disconnected spot so reject
    WiFiClient serverClient = server.available();
    serverClient.stop();
  }
  //check clients for data
  for(i = 0; i < MAX_SRV_CLIENTS; i++){
    if (serverClients[i] && serverClients[i].connected()){
      if(serverClients[i].available()){
        //get data from the telnet client and push it to the UART
        while(serverClients[i].available()) Serial.write(serverClients[i].read());
      }
    }
  }
  unsigned long now = micros();
  if (now - lastSendToClient > packetIntervalUs) {
    //check SPI buffers for data
    if (head != tail) {
      uint8_t _head = head;
      lastSendToClient = now;
      // Serial.print("flushing buffer at "); Serial.print(now); Serial.print(" head: "); Serial.print(_head); Serial.print(" tail: "); Serial.println(tail);
      tail = flushSPIToAllClients(tail, _head);
      // tail = _head >= maxPackets ? 0 : ;
    }
  }
}

int flushSPIToAllClients(int _tail, int _head) {
  // size_t len = Serial.available();
  // uint8_t sbuf[len];
  // Serial.readBytes(sbuf, len);
  for(uint8_t i = 0; i < MAX_SRV_CLIENTS; i++){
    int __tail = _tail;
    if (serverClients[i] && serverClients[i].connected()){
      // Serial.print("flushing buffer at "); Serial.print(now); Serial.print(" head: "); Serial.print(_head); Serial.print(" tail: "); Serial.println(tail);
      while (__tail != _head) {
        if (__tail >= maxPackets) {
          __tail = 0;
        }
        // Serial.print("h "); Serial.print(_head); Serial.print("t: "); Serial.println(__tail);
        serverClients[i].write(ringBuf[__tail], bytesPerPacket);
        __tail++;
      }
      // for (int j = _tail; j < _head; j++) {
      //   Serial.write(ringBuf[j][1]);
      //   // serverClients[i].write(ringBuf[j], bytesPerPacket);
      // }
      delay(1);
    }
  }
  return _head;
  // Serial.println("Flushed");
}

// void udpLoop() {
//   int packetSize = Udp.parsePacket();
//   if (packetSize) {
//     if (!clientSet) {
//       client = Udp.remoteIP();
//       clientSet = true;
//       Serial.println("client connected");
//       Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
//       Udp.write("ready fredy");
//       Udp.endPacket();
//     }
//   }
//   // If the client is set then try to flush the buffer.
//   if (clientSet) {
//     tryFlushBuffer();
//   }
// }
//
// void flushBufferToUDP(int start, int stop) {
//   Udp.beginPacket(client, 2391);
//   for (int j = start; j < stop; j++) {
//     Udp.write(ringBuf[j], bytesPerPacket);
//   }
//   Udp.endPacket();
//   // Serial.println("Flushed");
//   if (stop == maxPackets) {
//     tail = 0;
//   } else {
//     tail = stop;
//   }
// }

// void tryFlushBuffer() {
//   if (tail == 0 && head == 10) {
//     flushBufferToUDP(tail, head);
//   } else if (tail == 10 && head == 20) {
//     flushBufferToUDP(tail, head);
//   }
// }

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

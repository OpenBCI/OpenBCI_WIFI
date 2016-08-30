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

const uint8_t maxRand = 91;
const uint8_t minRand = 48;

unsigned int localPort = 2390;      // local port to listen for UDP packets

byte packetBuffer[512]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP Udp;

IPAddress client;

WiFiServer server(80);

unsigned long lastSerialRead = 0;
boolean packing = false;
boolean clientSet = false;


void setup() {
  // pinMode(0, OUTPUT);
  // digitalWrite(0, LOW);

  Udp.begin(localPort);

  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  // data has been received from the master. Beware that len is always 32
  // and the buffer is autofilled with zeroes if data is less than 32 bytes long
  // It's up to the user to implement protocol for handling data length
  SPISlave.onData([](uint8_t * data, size_t len) {
      String message = String((char *)data);
      if(message.equals("Hello Slave!")) {
          SPISlave.setData("Hello Master!");
      } else if(message.equals("Are you alive?")) {
          char answer[33];
          sprintf(answer,"Alive for %u seconds!", millis() / 1000);
          SPISlave.setData(answer);
      } else {
          SPISlave.setData("Say what?");
      }
      Serial.printf("Question: %s\n", (char *)data);
  });

  // The master has read out outgoing data buffer
  // that buffer can be set with SPISlave.setData
  SPISlave.onDataSent([]() {
      Serial.println("Answer Sent");
  });

  // status has been received from the master.
  // The status register is a special register that bot the slave and the master can write to and read from.
  // Can be used to exchange small data or status information
  SPISlave.onStatus([](uint32_t data) {
      Serial.printf("Status: %u\n", data);
      SPISlave.setStatus(millis()); //set next status
  });

  // The master has read the status register
  SPISlave.onStatusSent([]() {
      Serial.println("Status Sent");
  });

  // Setup SPI Slave registers and pins
  SPISlave.begin();

  // Set the status register (if the master reads it, it will read this value)
  SPISlave.setStatus(millis());

  // Sets the data registers. Limited to 32 bytes at a time.
  // SPISlave.setData(uint8_t * data, size_t len); is also available with the same limitation
  SPISlave.setData("Ask me a question!");

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

  if (Serial.available() && clientSet) {
    if (packing == false) {
      Udp.beginPacket(client,2391);
      packing = true;
      // Serial.print("packing: ");
    }

    char c = Serial.read();
    Serial.print(c);
    Udp.write(c);

    lastSerialRead = micros();

  }

  if (micros() > (100 + lastSerialRead) && packing) {
    packing = false;
    Udp.endPacket();
    // Serial.println(" done!");
  }

  int noBytes = Udp.parsePacket();
  if ( noBytes ) {

    if (!clientSet) {
      client = Udp.remoteIP();
      clientSet = true;
      // digitalWrite(0, HIGH);
      // delay(500);
      // digitalWrite(0, LOW);
      // delay(500);
      // digitalWrite(0, HIGH);

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

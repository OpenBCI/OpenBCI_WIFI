/*
 * 31 mar 2015
 * This sketch display UDP packets coming from an UDP client.
 * On a Mac the NC command can be used to send UDP. (nc -u 192.168.1.101 2390).
 *
 * Configuration : Enter the ssid and password of your Wifi AP. Enter the port number your server is listening on.
 *
 */

#include <ESP8266WiFi.h>
#include <WiFiUDP.h>

int status = WL_IDLE_STATUS;
const char* ssid = "867";  //  your network SSID (name)
const char* pass = "penthouse867";       // your network password

unsigned int localPort = 2390;      // local port to listen for UDP packets

byte packetBuffer[512]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP Udp;

IPAddress client;

unsigned long lastSerialRead = 0;
boolean packing = false;

void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  // setting up Station AP
  WiFi.begin(ssid, pass);

  // Wait for connect to AP
//  Serial.print("[Connecting]");
//  Serial.print(ssid);
  int tries=0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
//    Serial.print(".");
    tries++;
    if (tries > 30){
      break;
    }
  }
//  Serial.println();


//printWifiStatus();

//  Serial.println("Connected to wifi");
//  Serial.print("Udp server started at port ");
//  Serial.println(localPort);
  Udp.begin(localPort);
}


void loop()
{
  if (Serial.available()) {
    if (packing == false) {
      Udp.beginPacket(client,2391);
      packing = true;
    }

    Udp.write(Serial.read());

    lastSerialRead = micros();

  }

  if (micros() > (3000 + lastSerialRead) && packing) {
    packing = false;
    Udp.endPacket();
  }

  int noBytes = Udp.parsePacket();
  if ( noBytes ) {
//    Serial.print(millis() / 1000);
//    Serial.print(":Packet of ");
//    Serial.print(noBytes);
//    Serial.print(" received from ");
//    Serial.print(Udp.remoteIP());
    client = Udp.remoteIP();
//    Serial.print(":");
//    Serial.println(Udp.remotePort());

    // We've received a packet, read the data from it
    Udp.read(packetBuffer,noBytes); // read the packet into the buffer

    // display the packet contents in HEX
    for (int i=1;i<=noBytes;i++){
      Serial.write(packetBuffer[i-1]);
//      Serial.print(packetBuffer[i-1],HEX);
//      if (i % 32 == 0){
//        Serial.println();
//      }
//      else Serial.print(' ');
    } // end for
//    Serial.println();
  } // end if


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

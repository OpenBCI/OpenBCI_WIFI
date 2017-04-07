#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266SSDP.h>

const char* ssid = "867";
const char* password = "penthouse867";

ESP8266WebServer HTTP(80);

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Starting WiFi...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if(WiFi.waitForConnectResult() == WL_CONNECTED){

    Serial.printf("Starting HTTP...\n");
    HTTP.on("/index.html", HTTP_GET, [](){
      HTTP.send(200, "text/plain", "OpenBCI is in Wifi discovery mode");
    });
    HTTP.on("/description.xml", HTTP_GET, [](){
      SSDP.schema(HTTP.client());
    });
    HTTP.begin();

    Serial.printf("Starting SSDP...\n");
    SSDP.setSchemaURL("description.xml");
    SSDP.setHTTPPort(80);
    SSDP.setName("PTW - OpenBCI Wifi Shield");
    SSDP.setSerialNumber(ESP.getChipID());
    SSDP.setURL("index.html");
    SSDP.setModelName("PTW - OpenBCI Wifi Shield Bridge 2017");
    SSDP.setModelNumber("929000226503");
    SSDP.setModelURL("http://www.openbci.com");
    SSDP.setManufacturer("Push The World LLC");
    SSDP.setManufacturerURL("http://www.pushtheworldllc.com");
    SSDP.begin();

    Serial.printf("Ready!\n");
  } else {
    Serial.printf("WiFi Failed\n");
    while(1) delay(100);
  }
}

void loop() {
  HTTP.handleClient();
  delay(1);
}

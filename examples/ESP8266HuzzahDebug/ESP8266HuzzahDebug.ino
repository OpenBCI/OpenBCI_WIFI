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
 #include "SPISlave.h"

void setup() {


  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  // data has been received from the master. Beware that len is always 32
  // and the buffer is autofilled with zeroes if data is less than 32 bytes long
  // It's up to the user to implement protocol for handling data length
  SPISlave.onData([](uint8_t * data, size_t len) {
    String message = String((char *)data);
    // if(message.equals("Hello Slave!")) {
    //   SPISlave.setData("Hello Master!");
    // } else if(message.equals("Are you alive?")) {
    //   char answer[33];
    //   sprintf(answer,"Alive for %u seconds!", millis() / 1000);
    //   SPISlave.setData(answer);
    // } else {
    //   SPISlave.setData("Say what?");
    // }
    Serial.printf("%s\n", (char *)data);
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

}

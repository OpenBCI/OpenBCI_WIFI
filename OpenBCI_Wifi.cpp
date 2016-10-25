/**
* Name: OpenBCI_Wifi.h
* Date: 8/30/2016
* Purpose: This is the header file for the OpenBCI radios. Let us define two
*   over arching paradigms: Host and Device, where:
*     Host is connected to PC via USB VCP (FTDI).
*     Device is connectedd to uC (PIC32MX250F128B with UDB32-MX2-DIP).
*
* Author: Push The World LLC (AJ Keller)
*/

#include "OpenBCI_Wifi.h"

// CONSTRUCTOR
OpenBCI_Wifi_Class::OpenBCI_Wifi_Class() {
  // Set defaults
  debugMode = false; // Set true if doing dongle-dongle sim
}

/**
* @description The function that the radio will call in setup()
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Wifi_Class::begin(void) {
  begin(false);
}

/**
 * Called to begin a new session but in Serial debug mode.
 * @param debug {boolean} True and will get serial debug print outs.
 */
void OpenBCI_Wifi_Class::begin(boolean debug) {
  initialize(debug);
  configure(debug)
}

void OpenBCI_Wifi_Class::configure(boolean debug) {
  debugMode = debug;
  lastTimeSpiRead = 0;
  lastChipSelectLevel = 0;
  streamPacketBufferHead = 0;
  streamPacketBufferTail = 0;
}

void OpenBCI_Wifi_Class::initialize() {
  initialize(false);
}

void OpenBCI_Wifi_Class::initialize(boolean debug) {
  initializeSerial(debug);
  initializeSPISlave(debug);
}

void OpenBCI_Wifi_Class::initializeSerial(boolean debug) {
  Serial.begin(115200);
  Serial.setDebugOutput(debug);
}

void OpenBCI_Wifi_Class::initializeSPISlave(boolean debug) {
  // data has been received from the master. Beware that len is always 32
  // and the buffer is autofilled with zeroes if data is less than 32 bytes long
  // It's up to the user to implement protocol for handling data length
  SPISlave.onData([](uint8_t * data, size_t len) {

    // Copy incoming data
    memcpy(packetBuffer[packetBufferHead], data, 5 );
    // Increment the head
    packetBufferHead++;
    if (packetBufferHead >= OPENBCI_NUMBER_STREAM_BUFFERS) packetBufferHead = 0;

    // If we are in debug mode then pring out the data to Serial
    if (debugMode) {
      Serial.printf("SPI Input: %s\n", (char *)data);
    }
  });

  // The master has read out outgoing data buffer
  // that buffer can be set with SPISlave.setData
  SPISlave.onDataSent([]() {
      Serial.println("Answer Sent");
  });

  // status has been received from the master.
  // The status register is a special register that both the slave and the
  // master can write to and read from. Can be used to exchange small data
  // or status information
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
}


OpenBCI_Wifi_Class wifi;

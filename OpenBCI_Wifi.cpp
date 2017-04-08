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

OpenBCI_Wifi_Class OpenBCI_Wifi;

/***************************************************/
/** PUBLIC METHODS *********************************/
/***************************************************/
// CONSTRUCTOR
OpenBCI_Wifi_Class::OpenBCI_Wifi_Class() {
  // Set defaults
}

/**
* @description The function that the radio will call in setup()
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Wifi_Class::begin(void) {
  initArduino();
  initArrays();
  initObjects();
  initVariables();
}

void OpenBCI_Wifi_Class::initialize(void) {
  // initialize();
}

/**
* @description Utility function to return `true` if the the streamPacketBuffer
*   is in the STREAM_STATE_READY. Normally used for determining if a stream
*   packet is ready to be sent.
* @param `buf` {StreamPacketBuffer *} - The stream packet buffer to send to the Host.
* @returns {boolean} - `true` is the `buf` is in the ready state, `false` otherwise.
* @author AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Wifi_Class::bufferStreamReadyToSend(StreamPacketBuffer *buf) {
  return false; //streamPacketBuffer->state == STREAM_STATE_READY;
}

void OpenBCI_Wifi_Class::initializeSPISlave(boolean debug) {
  // // data has been received from the master. Beware that len is always 32
  // // and the buffer is autofilled with zeroes if data is less than 32 bytes long
  // // It's up to the user to implement protocol for handling data length
  // SPISlave.onData([](uint8_t * data, size_t len) {
  //
  //   // Copy incoming data
  //   memcpy(packetBuffer[packetBufferHead], data, 5 );
  //   // Increment the head
  //   packetBufferHead++;
  //   if (packetBufferHead >= OPENBCI_NUMBER_STREAM_BUFFERS) packetBufferHead = 0;
  //
  //   // If we are in debug mode then pring out the data to Serial
  //   if (debugMode) {
  //     Serial.printf("SPI Input: %s\n", (char *)data);
  //   }
  // });
  //
  // // The master has read out outgoing data buffer
  // // that buffer can be set with SPISlave.setData
  // SPISlave.onDataSent([]() {
  //     Serial.println("Answer Sent");
  // });
  //
  // // status has been received from the master.
  // // The status register is a special register that both the slave and the
  // // master can write to and read from. Can be used to exchange small data
  // // or status information
  // SPISlave.onStatus([](uint32_t data) {
  //     Serial.printf("Status: %u\n", data);
  //     SPISlave.setStatus(millis()); //set next status
  // });
  //
  // // The master has read the status register
  // SPISlave.onStatusSent([]() {
  //     Serial.println("Status Sent");
  // });
  //
  // // Setup SPI Slave registers and pins
  // SPISlave.begin();
  //
  // // Set the status register (if the master reads it, it will read this value)
  // SPISlave.setStatus(millis());
  //
  // // Sets the data registers. Limited to 32 bytes at a time.
  // // SPISlave.setData(uint8_t * data, size_t len); is also available with the same limitation
  // SPISlave.setData("Ask me a question!");
}

/**
* @description Resets the stream packet buffer to default settings
* @param `buf` {StreamPacketBuffer *} - Pointer to a stream packet buffer to reset
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Wifi_Class::bufferStreamReset(StreamPacketBuffer *buf) {
  buf->bytesIn = 0;
  buf->typeByte = 0;
  buf->state = STREAM_STATE_INIT;
}

/**
* @description Strips and gets the packet number from a byteId
* @param byteId [char] a byteId (see ::byteIdMake for description of bits)
* @returns [byte] the packet type
* @author AJ Keller (@pushtheworldllc)
*/
byte OpenBCI_Wifi_Class::byteIdGetStreamPacketType(uint8_t byteId) {
  return (byte)((byteId & 0x78) >> 3);
}

boolean OpenBCI_Wifi_Class::dataReady(void) {
  return !digitalRead(WIFI_PIN_SLAVE_SELECT);
}

void OpenBCI_Wifi_Class::initArduino(void) {
  pinMode(WIFI_PIN_SLAVE_SELECT,INPUT);

}

/**
* @description Initalize arrays here
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Wifi_Class::initArrays(void) {
  for (int i = 0; i < OPENBCI_NUMBER_STREAM_BUFFERS; i++) {
    // bufferStreamReset(streamPacketBuffer + i);
  }
}

/**
* @description Initalize class objects here
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Wifi_Class::initObjects(void) {
  // SPI.begin();
  // SPI.setHwCs(true);
}

/**
* @description Initalize variables here
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Wifi_Class::initVariables(void) {
  lastTimeSpiRead = 0;
  lastChipSelectLevel = 0;
  streamPacketBufferHead = 0;
  streamPacketBufferTail = 0;
}

/**
* @description Test to see if a char follows the stream tail byte format
* @author AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Wifi_Class::isATailByte(uint8_t newChar) {
  return (newChar >> 4) == 0xC;
}

/**
* @description Takes a byteId and converts to a Stop Byte for a streaming packet
* @param `byteId` - [byte] - A byteId with packet type in bits 6-3
* @return - [byte] - A stop byte with 1100 as the MSBs with packet type in the
*          four LSBs
* @example byteId == 0b10111000 returns 0b11000111
* @author AJ Keller (@pushtheworldllc)
*/
byte OpenBCI_Wifi_Class::outputGetStopByteFromByteId(char byteId) {
  return byteIdGetStreamPacketType(byteId) | 0xC0;
}


//SPI communication method
byte OpenBCI_Wifi_Class::xfer(byte _data) {
    byte inByte;
    // inByte = SPI.transfer(_data);
    return inByte;
}

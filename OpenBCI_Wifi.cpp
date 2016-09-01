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

/**
* @description Process a char from the serial port on the Device. Enters the char
*  into the stream state machine.
* @param `buf` {StreamPacketBuffer *} - The stream packet buffer to add the char to.
* @param `newChar` {char} - A new char to process
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Wifi_Class::bufferStreamAddChar(StreamPacketBuffer *buf, char newChar) {
  // Process the new char
  switch (buf->state) {
    case STREAM_STATE_TAIL:
      // Is the current char equal to 0xCX where X is 0-F?
      if (isATailByte(newChar)) {
        // Set the type byte
        buf->typeByte = newChar;
        // Change the state to ready
        buf->state = STREAM_STATE_READY;
        // Serial.print(33); Serial.print(" state: "); Serial.print("READY-");
        // Serial.println((streamPacketBuffer + streamPacketBufferHead)->state);
      } else {
        // Reset the state machine
        buf->state = STREAM_STATE_INIT;
        // Set bytes in to 0
        buf->bytesIn = 0;
        // Test to see if this byte is a head byte, maybe if it's not a
        //  tail byte then that's because a byte was dropped on the way
        //  over from the Pic.
        if (newChar == OPENBCI_STREAM_PACKET_HEAD) {
          // Move the state
          buf->state = STREAM_STATE_STORING;
        }
      }
      break;
    case STREAM_STATE_STORING:
      // Store to the stream packet buffer
      buf->data[buf->bytesIn] = newChar;
      // Increment the number of bytes read in
      buf->bytesIn++;

      if (buf->bytesIn == OPENBCI_MAX_PACKET_SIZE_BYTES - 1) {
        buf->state = STREAM_STATE_TAIL;
      }

      break;
    // We have called the function before we were able to send the stream
    //  packet which means this is not a stream packet, it's part of a
    //  bigger message
    case STREAM_STATE_READY:
      // Got a 34th byte, go back to start
      buf->state = STREAM_STATE_INIT;
      // Set bytes in to 0
      buf->bytesIn = 0;

      break;
    case STREAM_STATE_INIT:
      if (newChar == OPENBCI_STREAM_PACKET_HEAD) {
        // Move the state
        buf->state = STREAM_STATE_STORING;
        // Do not store to the op code streamPacketBuffer
        buf->bytesIn = 0;
      }
      break;
    default:
      // // Reset the state
      buf->state = STREAM_STATE_INIT;
      break;

  }
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
  return streamPacketBuffer->state == STREAM_STATE_READY;
}

/**
* @description Resets the stream packet buffer to default settings
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Wifi_Class::bufferStreamReset(void) {
  for (int i = 0; i < OPENBCI_NUMBER_STREAM_BUFFERS; i++) {
    bufferStreamReset(streamPacketBuffer + i);
  }
  streamPacketBufferHead = 0;
  streamPacketBufferTail = 0;
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
    bufferStreamReset(streamPacketBuffer + i);
  }
}

/**
* @description Initalize class objects here
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Wifi_Class::initObjects(void) {
  SPI.begin();
  SPI.setHwCs(true);
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
    inByte = SPI.transfer(_data);
    return inByte;
}

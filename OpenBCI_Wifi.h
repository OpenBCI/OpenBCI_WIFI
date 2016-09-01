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


#ifndef __OpenBCI_Wifi__
#define __OpenBCI_Wifi__

#include <Arduino.h>
#include "SPI.h"
#include "OpenBCI_Wifi_Definitions.h"

class OpenBCI_Wifi_Class {

public:
  // ENUMS
  typedef enum STREAM_STATE {
    STREAM_STATE_INIT,
    STREAM_STATE_STORING,
    STREAM_STATE_TAIL,
    STREAM_STATE_READY
  };
  // STRUCTS

  typedef struct {
    uint8_t         typeByte;
    char            data[OPENBCI_MAX_PACKET_SIZE_BYTES];
    uint8_t         bytesIn;
    STREAM_STATE    state;
  } StreamPacketBuffer;

  // Functions and Methods
  OpenBCI_Wifi_Class();
  void begin(void);
  void bufferStreamAddChar(StreamPacketBuffer *, char);
  boolean bufferStreamAddData(char *);
  void bufferStreamFlush(StreamPacketBuffer *);
  void bufferStreamFlushBuffers(void);
  boolean bufferStreamReadyToSend(StreamPacketBuffer *);
  void bufferStreamReset(void);
  void bufferStreamReset(StreamPacketBuffer *);
  boolean bufferStreamTimeout(void);
  byte byteIdGetStreamPacketType(uint8_t);
  boolean dataReady(void);
  boolean isATailByte(uint8_t);
  byte outputGetStopByteFromByteId(char);
  byte xfer(byte);

  // Variables
  StreamPacketBuffer streamPacketBuffer[OPENBCI_NUMBER_STREAM_BUFFERS];
  uint8_t lastChipSelectLevel;
  uint8_t bufSpi[WIFI_BUFFER_LENGTH];
  uint8_t bufUdp[WIFI_BUFFER_LENGTH];
  uint8_t bufferTxPosition;
  uint8_t streamPacketBufferHead;
  uint8_t streamPacketBufferTail;
  unsigned long lastTimeSpiRead;

private:
  void initArduino(void);
  void initArrays(void);
  void initObjects(void);
  void initVariables(void);

};

extern OpenBCI_Wifi_Class OpenBCI_Wifi;

#endif // OPENBCI_WIFI_H

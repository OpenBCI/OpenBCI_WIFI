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

#include "OpenBCI_Wifi_Definitions.h"

class OpenBCI_Wifi_Class {

public:

  // Functions and Methods
  void begin(void);
  void begin(boolean);
  void configure(boolean);
  void initialize(void);
  void initialize(boolean);
  void initializeSerial(boolean);
  void initializeSPISlave(boolean);


  // Variables
  uint8_t packetBuffer[OPENBCI_NUMBER_STREAM_BUFFERS][OPENBCI_MAX_PACKET_SIZE_BYTES];
  uint8_t lastChipSelectLevel;
  volatile uint8_t packetBufferHead;
  volatile uint8_t packetBufferTail;
  unsigned long lastTimeSpiRead;


};

// Very important, major key to success #christmas
extern OpenBCI_Wifi_Class wifi;

#endif // OPENBCI_WIFI_H

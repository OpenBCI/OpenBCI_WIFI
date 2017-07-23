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
#include <time.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266SSDP.h>
#include <WiFiManager.h>
#include "SPISlave.h"
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WebSocketsServer.h>
#include <Hash.h>
#include "OpenBCI_Wifi_Definitions.h"

class OpenBCI_Wifi_Class {

public:

  // ENUMS
  typedef enum CLIENT_RESPONSE {
    CLIENT_RESPONSE_NONE,
    CLIENT_RESPONSE_OUTPUT_STRING
  };

  typedef enum OUTPUT_MODE {
    OUTPUT_MODE_RAW,
    OUTPUT_MODE_JSON
  };

  typedef enum OUTPUT_PROTOCOL {
    OUTPUT_PROTOCOL_NONE,
    OUTPUT_PROTOCOL_TCP,
    OUTPUT_PROTOCOL_MQTT,
    OUTPUT_PROTOCOL_WEB_SOCKETS,
    OUTPUT_PROTOCOL_SERIAL
  };

  typedef enum CYTON_GAIN {
    CYTON_GAIN_1,
    CYTON_GAIN_2,
    CYTON_GAIN_4,
    CYTON_GAIN_6,
    CYTON_GAIN_8,
    CYTON_GAIN_12,
    CYTON_GAIN_24
  };

  // Functions and Methods
  OpenBCI_Wifi_Class();
  void begin(void);
  void extractRaws(uint8_t *, int32_t *);
  String getOutputMode(OUTPUT_MODE);
  double getScaleFactorVoltsCyton(uint8_t);
  double getScaleFactorVoltsGanglion(void);
  int32_t int24To32(uint8_t *);
  double rawToScaled(int32_t, double);
  void transformRawsToScaledCyton(int32_t *, uint8_t *, uint8_t, double *);
  void transformRawsToScaledGanglion(int32_t *, double *);

  // Variables


private:
  void initArduino(void);
  void initArrays(void);
  void initObjects(void);
  void initVariables(void);

};

// Very important, major key to success #christmas
extern OpenBCI_Wifi_Class wifi;

#endif // OPENBCI_WIFI_H

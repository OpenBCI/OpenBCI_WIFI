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
#define ARDUINOJSON_USE_LONG_LONG 1
#define ARDUINOJSON_USE_DOUBLE 1


#ifndef __OpenBCI_Wifi__
#define __OpenBCI_Wifi__
#include <Arduino.h>
#include <time.h>
#include <ESP8266WiFi.h>
#include "SPISlave.h"
#include <ArduinoJson.h>
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
    OUTPUT_PROTOCOL_UDP,
    OUTPUT_PROTOCOL_MQTT,
    OUTPUT_PROTOCOL_WEB_SOCKETS,
    OUTPUT_PROTOCOL_SERIAL,
    OUTPUT_PROTOCOL_AZURE_EVENT_HUB
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

  // STRUCTS
  typedef struct {
    double channelData[NUM_CHANNELS_CYTON_DAISY];
    unsigned long long timestamp;
    uint8_t sampleNumber;
  } Sample;

  typedef struct {
    boolean flushing;
    boolean gotAllPackets;
    uint8_t data[BYTES_PER_RAW_BUFFER];
    int     positionWrite;
  } RawBuffer;

  // Functions and Methods
  OpenBCI_Wifi_Class();
  void begin(void);
  void channelDataCompute(uint8_t *, uint8_t *, Sample *, uint8_t, uint8_t);
  void debugPrintLLNumber(long long);
  void debugPrintLLNumber(long long, uint8_t);
  void debugPrintLLNumber(unsigned long long);
  void debugPrintLLNumber(unsigned long long, uint8_t);
  void extractRaws(uint8_t *, int32_t *, uint8_t);
  void gainReset(void);
  String getBoardTypeString(uint8_t);
  String getCurBoardTypeString();
  String getCurOutputModeString();
  String getCurOutputProtocolString();
  uint8_t *getGains(void);
  uint8_t getGainCyton(uint8_t b);
  uint8_t getGainGanglion(void);
  uint8_t getHead(void);
  String getInfoAll(void);
  String getInfoBoard(void);
  String getInfoMQTT(boolean);
  String getInfoTCP(boolean);
  int getJSONAdditionalBytes(uint8_t);
  size_t getJSONBufferSize(void);
  void getJSONFromSamples(JsonObject&, uint8_t, uint8_t);
  uint8_t getJSONMaxPackets(void);
  uint8_t getJSONMaxPackets(uint8_t);
  unsigned long getLatency(void);
  String getMacLastFourBytes(void);
  String getMac(void);
  String getModelNumber(void);
  String getName(void);
  uint8_t getNumChannels(void);
  unsigned long getNTPOffset(void);
  String getOutputModeString(OUTPUT_MODE);
  String getOutputProtocolString(OUTPUT_PROTOCOL);
  String getStringLLNumber(long long);
  String getStringLLNumber(long long, uint8_t);
  String getStringLLNumber(unsigned long long);
  String getStringLLNumber(unsigned long long, uint8_t);
  double getScaleFactorVoltsCyton(uint8_t);
  double getScaleFactorVoltsGanglion(void);
  uint8_t getTail(void);
  unsigned long long getTime(void);
  String getVersion();
  int32_t int24To32(uint8_t *);
  boolean isAStreamByte(uint8_t);
  void loop(void);
  boolean mqttConnect(void);
  boolean ntpActive(void);
  unsigned long long ntpGetPreciseAdjustment(unsigned long);
  unsigned long long ntpGetTime(void);
  void ntpStart(void);
  void passthroughBufferClear(void);
  uint8_t passthroughCommands(String);
  String perfectPrintByteHex(uint8_t);
  boolean rawBufferAddStreamPacket(RawBuffer *, uint8_t *);
  void rawBufferClean(RawBuffer *);
  boolean rawBufferHasData(RawBuffer *);
  byte rawBufferProcessPacket(uint8_t *);
  boolean rawBufferReadyForNewPage(RawBuffer *);
  void rawBufferReset(void);
  void rawBufferReset(RawBuffer *);
  boolean rawBufferSwitchToOtherBuffer(void);
  double rawToScaled(int32_t, double);
  void reset(void);
  void sampleReset(void);
  void sampleReset(Sample *);
  void sampleReset(Sample *, uint8_t);
  void setGains(uint8_t *);
  void setGains(uint8_t *, uint8_t *);
  void setInfoMQTT(String, String, String, int);
  void setInfoUDP(String, int, boolean);
  void setInfoTCP(String, int, boolean);
  void setLatency(unsigned long);
  void setNumChannels(uint8_t);
  void setNTPOffset(unsigned long);
  void setOutputMode(OUTPUT_MODE);
  void setOutputProtocol(OUTPUT_PROTOCOL);
  boolean spiHasMaster(void);
  void spiOnDataSent(void);
  void spiProcessPacket(uint8_t *);
  void spiProcessPacketGain(uint8_t *);
  void spiProcessPacketStream(uint8_t *);
  void spiProcessPacketStreamJSON(uint8_t *);
  void spiProcessPacketStreamRaw(uint8_t *);
  void spiProcessPacketResponse(uint8_t *);
  void transformRawsToScaledCyton(int32_t *, uint8_t *, uint8_t, double *);
  void transformRawsToScaledGanglion(int32_t *, double *);

  // Variables
  boolean clientWaitingForResponse;
  boolean clientWaitingForResponseFullfilled;
  boolean passthroughBufferLoaded;
  boolean jsonHasSampleNumbers;
  boolean jsonHasTimeStamps;
  boolean tcpDelimiter;

  CLIENT_RESPONSE curClientResponse;

  IPAddress tcpAddress;

  int mqttPort;
  int tcpPort;

  OUTPUT_MODE curOutputMode;
  OUTPUT_PROTOCOL curOutputProtocol;

  RawBuffer *curRawBuffer;
  RawBuffer rawBuffer[NUM_RAW_BUFFERS];

  Sample sampleBuffer[NUM_PACKETS_IN_RING_BUFFER_JSON];

  String mqttBrokerAddress;
  String mqttUsername;
  String mqttPassword;
  String outputString;

  uint8_t lastSampleNumber;
  uint8_t passthroughPosition;
  uint8_t passthroughBuffer[BYTES_PER_SPI_PACKET];

  unsigned long lastTimeWasPolled;
  unsigned long timePassthroughBufferLoaded;

  volatile uint8_t head;
  volatile uint8_t tail;

private:
  // Functions
  void initArduino(void);
  void initArrays(void);
  void initObjects(void);
  void initVariables(void);

  // Variables
  size_t _jsonBufferSize;

  uint8_t _gains[MAX_CHANNELS];
  uint8_t curNumChannels;

  unsigned long _counter;
  unsigned long _latency;
  unsigned long _ntpOffset;


};

// Very important, major key to success #christmas
extern OpenBCI_Wifi_Class wifi;

#endif // OPENBCI_WIFI_H

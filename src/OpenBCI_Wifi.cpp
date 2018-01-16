/**
* Name: OpenBCI_Wifi.h
* Date: 8/30/2016
* Purpose: This is the header file for the OpenBCI wifi chip.
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
  initVariables();
}

/**
* @description The function that the radio will call in setup()
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Wifi_Class::begin(void) {
  initVariables();
  initArrays();
  initObjects();
  initArduino();
}

void OpenBCI_Wifi_Class::initArduino(void) {
  pinMode(LED_NOTIFY, OUTPUT);
  pinMode(LED_PROG, OUTPUT);
  digitalWrite(LED_PROG, LOW);
}

/**
* @description Initalize arrays here
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Wifi_Class::initArrays(void) {
  gainReset();
}

/**
* @description Initalize class objects here
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Wifi_Class::initObjects(void) {
  setNumChannels(0);
#ifdef RAW_TO_JSON
  for (size_t i = 0; i < NUM_PACKETS_IN_RING_BUFFER_JSON; i++) {
    wifi.sampleReset(sampleBuffer + i);
  }
#endif
}

/**
* @description Initalize variables here
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Wifi_Class::initVariables(void) {
  clientWaitingForResponse = false;
  clientWaitingForResponseFullfilled = false;
  jsonHasSampleNumbers = false;
  jsonHasTimeStamps = true;
  passthroughBufferLoaded = false;
  redundancy = false;
  tcpDelimiter = false;

  curNumChannels = 0;
  head = 0;
  lastSampleNumber = 0;
  lastTimeWasPolled = 0;
  mqttPort = DEFAULT_MQTT_PORT;
  passthroughPosition = 0;
  rawBufferHead = 0;
  rawBufferTail = 0;
  tail = 0;
  tcpPort = 80;
  timePassthroughBufferLoaded = 0;
  _counter = 0;
  _latency = DEFAULT_LATENCY;
  _ntpOffset = 0;

#ifdef MQTT
  mqttBrokerAddress = "";
  mqttUsername = "";
  mqttPassword = "";
#endif
  outputString = "";

  tcpAddress = IPAddress();

  curOutputMode = OUTPUT_MODE_RAW;
  curOutputProtocol = OUTPUT_PROTOCOL_NONE;
}

void OpenBCI_Wifi_Class::reset(void) {
  // initArduino();
  initVariables();
  initArrays();
  initObjects();
}

////////////////////////////
////////////////////////////
// GETTERS /////////////////
////////////////////////////
////////////////////////////

/**
 * Used to get a pretty description of the board connected to the shield
 * @param  numChannels  {uint8_t} - The number of channels
 * @return              {String} - The string representation of given `numChannels`
 */
String OpenBCI_Wifi_Class::getBoardTypeString(uint8_t numChannels) {
  switch(numChannels) {
    case NUM_CHANNELS_CYTON_DAISY:
      return BOARD_TYPE_CYTON_DAISY;
    case NUM_CHANNELS_CYTON:
      return BOARD_TYPE_CYTON;
    case NUM_CHANNELS_GANGLION:
      return BOARD_TYPE_GANGLION;
    default:
      return BOARD_TYPE_NONE;
  }
}

/**
 * Use the internal `curNumChannels` value to get current board type.
 * @return {String} - A stringified version of the number of the board
 *                  type based on number of channels.
 */
String OpenBCI_Wifi_Class::getCurBoardTypeString() {
  return getBoardTypeString(curNumChannels);
}

String OpenBCI_Wifi_Class::getCurOutputModeString() {
  return getOutputModeString(curOutputMode);
}

String OpenBCI_Wifi_Class::getCurOutputProtocolString() {
  return getOutputProtocolString(curOutputProtocol);
}

uint8_t *OpenBCI_Wifi_Class::getGains() {
  return _gains;
}

/**
 * Used to decode coded gain value from Cyton into actual gain
 * @param  b {uint8_t} The encoded ADS1299 gain value. 0-6
 * @return   {uint8_t} The decoded gain value.
 */
uint8_t OpenBCI_Wifi_Class::getGainCyton(uint8_t b) {
  switch (b) {
    case CYTON_GAIN_1:
      return 1;
    case CYTON_GAIN_2:
      return 2;
    case CYTON_GAIN_4:
      return 4;
    case CYTON_GAIN_6:
      return 6;
    case CYTON_GAIN_8:
      return 8;
    case CYTON_GAIN_12:
      return 12;
    case CYTON_GAIN_24:
    default:
      return 24;
  }
}

/**
 * Returns the gain for the Ganglion.
 * @return {uint8_t} - The gain for the ganglion.
 */
uint8_t OpenBCI_Wifi_Class::getGainGanglion() {
  return 51;
}

/**
 * Returns the current head of the buffer
 * @return  [description]
 */
uint8_t OpenBCI_Wifi_Class::getHead(void) {
  return head;
}

String OpenBCI_Wifi_Class::getInfoAll(void) {
  const size_t argBufferSize = JSON_OBJECT_SIZE(8) + 135;
  DynamicJsonBuffer jsonBuffer(argBufferSize);
  JsonObject& root = jsonBuffer.createObject();
  root.set(JSON_BOARD_CONNECTED, (bool)spiHasMaster());
  // Serial.println("spiHasMaster: "); Serial.println(spiHasMaster() ? "true" : "false");
  root[JSON_HEAP] = ESP.getFreeHeap();
  root[JSON_TCP_IP] = WiFi.localIP().toString();
  root[JSON_MAC] = getMac();
  root[JSON_NAME] = getName();
  root[JSON_NUM_CHANNELS] = getNumChannels();
  root[JSON_VERSION] = getVersion();
  root[JSON_LATENCY] = getLatency();
  String output;
  root.printTo(output);
  return output;
}

String OpenBCI_Wifi_Class::getInfoBoard(void) {
  const size_t argBufferSize = JSON_OBJECT_SIZE(4) + 150 + JSON_ARRAY_SIZE(getNumChannels());
  DynamicJsonBuffer jsonBuffer(argBufferSize);
  JsonObject& root = jsonBuffer.createObject();
  root.set(JSON_BOARD_CONNECTED, (bool)spiHasMaster());
  // Serial.println("spiHasMaster: "); Serial.println(spiHasMaster() ? "true" : "false");
  root[JSON_BOARD_TYPE] = getCurBoardTypeString();
  root[JSON_NUM_CHANNELS] = getNumChannels();
  JsonArray& gainsArr = root.createNestedArray(JSON_GAINS);
  for (uint8_t i = 0; i < getNumChannels(); i++) {
    gainsArr.add(_gains[i]);
  }
  String output;
  root.printTo(output);
  return output;
}

#ifdef MQTT
String OpenBCI_Wifi_Class::getInfoMQTT(boolean clientMQTTConnected) {
  const size_t bufferSize = JSON_OBJECT_SIZE(7) + 2000;
  StaticJsonBuffer<bufferSize> jsonBuffer;
  String json;
  JsonObject& root = jsonBuffer.createObject();
  root[JSON_MQTT_BROKER_ADDR] = String(mqttBrokerAddress);
  root[JSON_CONNECTED] = clientMQTTConnected ? true : false;
  root[JSON_MQTT_USERNAME] = String(mqttUsername);
  root[JSON_TCP_OUTPUT] = getCurOutputModeString();
  root[JSON_LATENCY] = getLatency();
  root[JSON_MQTT_PORT] = mqttPort;
  root.printTo(json);
  return json;
}
#endif

String OpenBCI_Wifi_Class::getInfoTCP(boolean clientTCPConnected) {
  const size_t bufferSize = JSON_OBJECT_SIZE(6) + 40*6;
  StaticJsonBuffer<bufferSize> jsonBuffer;
  String json;
  JsonObject& root = jsonBuffer.createObject();
  root[JSON_CONNECTED] = clientTCPConnected ? true : false;
  root[JSON_TCP_DELIMITER] = tcpDelimiter ? true : false;
  root[JSON_TCP_IP] = tcpAddress.toString();
  root[JSON_TCP_OUTPUT] = getCurOutputModeString();
  root[JSON_TCP_PORT] = tcpPort;
  root[JSON_LATENCY] = getLatency();
  root.printTo(json);
  return json;
}

/////////////////////////
//// JSON ///////////////
/////////////////////////
/**
 * The additional bytes needed for input duplication, follows max packets
 */
int OpenBCI_Wifi_Class::getJSONAdditionalBytes(uint8_t numChannels) {
  const int extraDoubleSpace = 100;
  const int extraLongLongSpace = 50;
  switch (numChannels) {
    case NUM_CHANNELS_GANGLION:
      return 1014 + extraDoubleSpace + extraLongLongSpace;
    case NUM_CHANNELS_CYTON_DAISY:
      return 1062 + extraDoubleSpace + extraLongLongSpace;
    case NUM_CHANNELS_CYTON:
    default:
      return 966 + extraDoubleSpace + extraLongLongSpace;
  }
}

size_t OpenBCI_Wifi_Class::getJSONBufferSize() {
  return _jsonBufferSize;
}
#ifdef RAW_TO_JSON
/**
 * Used to pack Samples into a single JSON chunk to be sent out to client. The
 *
 * @param  numChannels {uint8_t} - The number of channels per sample
 * @param  numSamples  {uint8_t} - The number of samples to pack , should not
 *                      exceed the `::getJSONMaxPackets()` for `numChannels`.
 * @return             {String} - Strinigified version of the "chunk"
 */
void OpenBCI_Wifi_Class::getJSONFromSamples(JsonObject& root, uint8_t numChannels, uint8_t numPackets) {
  // DynamicJsonBuffer jsonSampleBuffer(_jsonBufferSize + 500);
  //
  // JsonObject& root = jsonSampleBuffer.createObject();
  JsonArray& chunk = root.createNestedArray("chunk");

  root["count"] = _counter++;
  // Serial.println("1" + outputString);
  for (uint8_t i = 0; i < numPackets; i++) {
    if (tail >= NUM_PACKETS_IN_RING_BUFFER_JSON) {
      tail = 0;
    }
    JsonObject& sample = chunk.createNestedObject();
    // Serial.printf("timestamp: "); debugPrintLLNumber((sampleBuffer + tail)->timestamp); Serial.println();
    if (jsonHasTimeStamps) sample.set<unsigned long long>("timestamp", (sampleBuffer + tail)->timestamp);
    // unsigned long long timestamp = sample.get<unsigned long long>("timestamp");
    // printLLNumber(timestamp); Serial.println();
    // Serial.printf("timestamp: %.0f\n", timestamp);
    if (jsonHasSampleNumbers) sample["sampleNumber"] = (sampleBuffer + tail)->sampleNumber;
    JsonArray& data = sample.createNestedArray("data");
    for (uint8_t j = 0; j < numChannels; j++) {
      // Serial.printf("%d %d ", i, j);
      if ((sampleBuffer + tail)->channelData[j] < 0) {
        data.add((long long)(sampleBuffer + tail)->channelData[j]);
        // long long tmp_data = data[j];
        // debugPrintLLNumber(tmp_data); Serial.println();
      } else {
        data.add((unsigned long long)(sampleBuffer + tail)->channelData[j]);
        // unsigned long long tmp_data = data[j];
        // debugPrintLLNumber(tmp_data); Serial.println();
      }
    }
    tail++;
  }

  // String returnStr = "";
  // root.printTo(returnStr);
  // return returnStr;
}
#endif
/**
 * Gets the latency
 * @return  {unsigned long} - The latency of the system
 */
unsigned long OpenBCI_Wifi_Class::getLatency(void) {
  return _latency;
}

/**
 * We want to max the size out to < 2000bytes per json chunk
 */
uint8_t OpenBCI_Wifi_Class::getJSONMaxPackets(uint8_t numChannels) {
  switch (numChannels) {
    case NUM_CHANNELS_GANGLION:
      return 10; // Size of
    case NUM_CHANNELS_CYTON_DAISY:
      return 6;
    case NUM_CHANNELS_CYTON:
    default:
      return 10;
  }
}

uint8_t OpenBCI_Wifi_Class::getJSONMaxPackets() {
  return getJSONMaxPackets(getNumChannels());
}

/**
 * Used to get the last two bytes of the max addresses
 * @return {String} The last two bytes, will always be four chars.
 */
String OpenBCI_Wifi_Class::getMacLastFourBytes(void) {
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.softAPmacAddress(mac);
  String macID = perfectPrintByteHex(mac[WL_MAC_ADDR_LENGTH - 2]) +
                 perfectPrintByteHex(mac[WL_MAC_ADDR_LENGTH - 1]);
  macID.toUpperCase();
  return macID;
}

/**
 * Returns the full mac address
 * @return  {String} Mac address with bytes separated with colons
 */
String OpenBCI_Wifi_Class::getMac(void) {
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.softAPmacAddress(mac);
  String fullMac = perfectPrintByteHex(mac[WL_MAC_ADDR_LENGTH - 6]) + ":" +
                   perfectPrintByteHex(mac[WL_MAC_ADDR_LENGTH - 5]) + ":" +
                   perfectPrintByteHex(mac[WL_MAC_ADDR_LENGTH - 4]) + ":" +
                   perfectPrintByteHex(mac[WL_MAC_ADDR_LENGTH - 3]) + ":" +
                   perfectPrintByteHex(mac[WL_MAC_ADDR_LENGTH - 2]) + ":" +
                   perfectPrintByteHex(mac[WL_MAC_ADDR_LENGTH - 1]);
  fullMac.toUpperCase();
  return fullMac;
}

/**
 * Model number has Push The World and their product number encoded with unqiue
 *  last four bytes of mac address.
 * @return  {String} The model number
 */
String OpenBCI_Wifi_Class::getModelNumber(void) {
  String AP_NameString = "PTW-0001-" + getMacLastFourBytes();

  char AP_NameChar[AP_NameString.length() + 1];
  memset(AP_NameChar, 0, AP_NameString.length() + 1);

  for (int i=0; i<AP_NameString.length(); i++)
    AP_NameChar[i] = AP_NameString.charAt(i);

  return AP_NameString;
}

/**
 * Each OpenBCI Wifi shield has a unique name
 * @return {String} - The name of the device.
 */
String OpenBCI_Wifi_Class::getName(void) {
  String AP_NameString = "OpenBCI-" + getMacLastFourBytes();

  char AP_NameChar[AP_NameString.length() + 1];
  memset(AP_NameChar, 0, AP_NameString.length() + 1);

  for (int i=0; i<AP_NameString.length(); i++)
    AP_NameChar[i] = AP_NameString.charAt(i);

  return AP_NameString;
}

unsigned long OpenBCI_Wifi_Class::getNTPOffset(void) {
  return _ntpOffset;
}

uint8_t OpenBCI_Wifi_Class::getNumChannels(void) {
  return curNumChannels;
}

/**
 * Get a string version of the output mode
 * @param  outputMode {OUTPUT_MODE} The output mode is either 'raw' or 'json'
 * @return            {String} String version of the output mode
 */
String OpenBCI_Wifi_Class::getOutputModeString(OUTPUT_MODE outputMode) {
  switch(outputMode) {
    case OUTPUT_MODE_JSON:
      return OUTPUT_JSON;
    case OUTPUT_MODE_RAW:
    default:
      return OUTPUT_RAW;
  }
}

/**
 * Get a string version of the output protocol
 * @param  outputProtocol {OUTPUT_PROTOCOL} The output protocol is either 'tcp',
 *                        'mqtt', 'serial', or 'none'
 * @return            {String} String version of the output protoocol
 */
String OpenBCI_Wifi_Class::getOutputProtocolString(OUTPUT_PROTOCOL outputProtocol) {
  switch(outputProtocol) {
    case OUTPUT_PROTOCOL_TCP:
      return OUTPUT_TCP;
    case OUTPUT_PROTOCOL_MQTT:
      return OUTPUT_MQTT;
    case OUTPUT_PROTOCOL_SERIAL:
      return OUTPUT_SERIAL;
    case OUTPUT_PROTOCOL_WEB_SOCKETS:
      return OUTPUT_WEB_SOCKETS;
    case OUTPUT_PROTOCOL_NONE:
    default:
      return OUTPUT_NONE;
  }
}
#ifdef RAW_TO_JSON
double OpenBCI_Wifi_Class::getScaleFactorVoltsGanglion() {
  return MCP_SCALE_FACTOR_VOLTS;
}

/**
 * Used to get a scale factor given a gain. Scale factors are hardcoded in
 *  file `OpenBCI_Wifi_Definitions.h`
 * @param  gain {uint8_t} - A gain value from cyton, so either 1, 2, 4, 6, 8,
 *              12, or 24
 * @return      {double} - The scale factor in volts
 */
double OpenBCI_Wifi_Class::getScaleFactorVoltsCyton(uint8_t gain) {
  switch (gain) {
    case ADS_GAIN_1:
      return ADS_SCALE_FACTOR_VOLTS_1;
    case ADS_GAIN_2:
      return ADS_SCALE_FACTOR_VOLTS_2;
    case ADS_GAIN_4:
      return ADS_SCALE_FACTOR_VOLTS_4;
    case ADS_GAIN_6:
      return ADS_SCALE_FACTOR_VOLTS_6;
    case ADS_GAIN_8:
      return ADS_SCALE_FACTOR_VOLTS_8;
    case ADS_GAIN_12:
      return ADS_SCALE_FACTOR_VOLTS_12;
    case ADS_GAIN_24:
      return ADS_SCALE_FACTOR_VOLTS_24;
    default:
      return 1.0;
  }
}
#endif

/**
 * Used to print out a long long number
 * @param n    {int64_t} The signed number
 * @param base {uint8_t} The base you want to print in. DEC, HEX, BIN
 */
String OpenBCI_Wifi_Class::getStringLLNumber(long long n, uint8_t base) {
  if (n < 0) {
    return "-" + getStringLLNumber((unsigned long long)(-1*n), base);
  } else {
    return getStringLLNumber((unsigned long long)n, base);
  }
}

/**
* Used to print out a long long number
* @param n    {int64_t} The signed number
*/
String OpenBCI_Wifi_Class::getStringLLNumber(long long n) {
  return getStringLLNumber(n, DEC);
}

/**
 * Used to print out an unsigned long long number
 * @param n    {uint64_t} The unsigned number
 * @param base {uint8_t} The base you want to print in. DEC, HEX, BIN
 */
String OpenBCI_Wifi_Class::getStringLLNumber(unsigned long long n, uint8_t base) {
  unsigned char buf[16 * sizeof(long)]; // Assumes 8-bit chars.
  unsigned long long i = 0;

  if (n == 0) {
    return "0";
  }
  String output;
  while (n > 0) {
    buf[i++] = n % base;
    n /= base;
  }

  for (; i > 0; i--) {
    output = output + String((char) (buf[i - 1] < 10 ?
      '0' + buf[i - 1] :
      'A' + buf[i - 1] - 10));
  }
  return output;
}

/**
 * Used to print out an unsigned long long number in base DEC
 * @param n    {uint64_t} The unsigned number
 */
String OpenBCI_Wifi_Class::getStringLLNumber(unsigned long long n) {
  return getStringLLNumber(n, DEC);
}

String OpenBCI_Wifi_Class::getVersion(void) {
  return SOFTWARE_VERSION;
}

void OpenBCI_Wifi_Class::gainReset(void) {
  for (uint8_t i = 0; i < MAX_CHANNELS; i++) {
    _gains[i] = 0;
  }
}

/**
 * Returns the current of the buffer
 * @return  [description]
 */
uint8_t OpenBCI_Wifi_Class::getTail(void) {
  return tail;
}

/**
 * Safely get the time, defaults to micros() if ntp is not active.
 */
unsigned long long OpenBCI_Wifi_Class::getTime() {
  if (ntpActive()) {
    return ntpGetTime() + ntpGetPreciseAdjustment(_ntpOffset);
  } else {
    return micros();
  }
}

////////////////////////////
////////////////////////////
// SETTERS ///////////////
////////////////////////////
////////////////////////////

/**
 * Used to set the gain for the JSON conversion
 * @param `raw` [uint8_t *] - The array from Pic or Cyton descirbing channels
 *   and gains.
 */
void OpenBCI_Wifi_Class::setGains(uint8_t *raw, uint8_t *gains) {
  uint8_t byteCounter = 2;
  uint8_t numChannels = raw[byteCounter++];

  if (numChannels < NUM_CHANNELS_GANGLION || numChannels > MAX_CHANNELS) {
    return;
  }
  for (uint8_t i = 0; i < numChannels; i++) {
    if (numChannels == NUM_CHANNELS_GANGLION) {
      // do gang related stuffs
      _gains[i] = getGainGanglion();
    } else {
      // Save the gains for later sending in /all
      _gains[i] = getGainCyton(raw[byteCounter]);
    }
  }
  setNumChannels(numChannels);
#ifdef DEBUG
  Serial.println("Gain set");
#endif
}

void OpenBCI_Wifi_Class::setGains(uint8_t *raw) {
  setGains(raw, _gains);
}

#ifdef MQTT
/**
 * Used to set the information required for a succesful MQTT communication
 * @param brokerAddress {String} - A string such as 'mock.getcloudbrain.com'
 * @param username      {String} - The username for the MQTT broker to user
 * @param password      {String} - The password for you to connect to
 */
void OpenBCI_Wifi_Class::setInfoMQTT(String brokerAddress, String username, String password, int port) {
  mqttBrokerAddress = brokerAddress;
  mqttUsername = username;
  mqttPassword = password;
  mqttPort = port;
  setOutputProtocol(OUTPUT_PROTOCOL_MQTT);
}
#endif
/**
 * Used to configure the requried internal variables for TCP communication
 * @param address   {IPAddress} - The ip address in string form: "192.168.0.1"
 * @param port      {int} - The port number as an int
 * @param delimiter {boolean} - Include the tcpDelimiter '\r\n'?
 */
void OpenBCI_Wifi_Class::setInfoTCP(String address, int port, boolean delimiter) {
  tcpAddress.fromString(address);
  tcpDelimiter = delimiter;
  tcpPort = port;
  setOutputProtocol(OUTPUT_PROTOCOL_TCP);
}

/**
 * Used to configure the requried internal variables for TCP communication
 * @param address   {IPAddress} - The ip address in string form: "192.168.0.1"
 * @param port      {int} - The port number as an int
 * @param delimiter {boolean} - Include the tcpDelimiter '\r\n'?
 */
void OpenBCI_Wifi_Class::setInfoUDP(String address, int port, boolean delimiter) {
  tcpAddress.fromString(address);
  tcpDelimiter = delimiter;
  tcpPort = port;
  setOutputProtocol(OUTPUT_PROTOCOL_UDP);
}

/**
 * Sets the latency
 * @param latency {unsigned long} - The latency of the system
 */
void OpenBCI_Wifi_Class::setLatency(unsigned long latency) {
  _latency = latency;
}

/**
 * Set the number of channels
 * @param numChannels {uint8_t} - The number of channels to set the system to
 */
void OpenBCI_Wifi_Class::setNumChannels(uint8_t numChannels) {
  curNumChannels = numChannels;
  // Used for JSON output mode
  _jsonBufferSize = 0; // Reset to 0
  _jsonBufferSize += JSON_OBJECT_SIZE(2); // For {"chunk":[...], "count":0}
  _jsonBufferSize += JSON_ARRAY_SIZE(getJSONMaxPackets(numChannels)); // For the array of samples
  _jsonBufferSize += getJSONMaxPackets(numChannels)*JSON_OBJECT_SIZE(3); // For each sample {"timestamp":0, "data":[...], "sampleNumber":0}
  _jsonBufferSize += getJSONMaxPackets(numChannels)*JSON_ARRAY_SIZE(numChannels); // For data array for each sample
  _jsonBufferSize += getJSONAdditionalBytes(numChannels); // The additional bytes needed for input duplication
}

/**
 * Set the ntp offset of the system
 * @param ntpOffset {unsigned long} - The micros mod with one million
 */
void OpenBCI_Wifi_Class::setNTPOffset(unsigned long ntpOffset) {
  _ntpOffset = ntpOffset;
}

/**
 * Used to set the current output mode
 * @param newOutputMode {OUTPUT_MODE} The output mode you want to switch to
 */
void OpenBCI_Wifi_Class::setOutputMode(OUTPUT_MODE newOutputMode) {
  curOutputMode = newOutputMode;
}

/**
 * Used to set the current output protocl
 * @param newOutputProtocol {OUTPUT_PROTOCOL} The output protocol you want to switch to
 */
void OpenBCI_Wifi_Class::setOutputProtocol(OUTPUT_PROTOCOL newOutputProtocol) {
  curOutputProtocol = newOutputProtocol;
}

////////////////////////////
////////////////////////////
// UTILITIES ///////////////
////////////////////////////
////////////////////////////

#ifdef RAW_TO_JSON
/**
 * Return true if the channel data array is full
 * @param arr           {uint8_t *] - 32 byte array from Cyton or Ganglion
 * @param gains         {uint8_t *} - Gain array
 * @param sample        {Sample *} - Sample struct to hold data before conversion to float
 * @param packetOffset  {uint8_t} - The offset to shift loading channel data into.
 *                      i.e. should be 1 on second packet for daisy
 * @param numChannels   {uint8_t} - The number of channels of the system, either 4, 8, or 16
 */
void OpenBCI_Wifi_Class::channelDataCompute(uint8_t *arr, uint8_t *gains, Sample *sample, uint8_t packetOffset, uint8_t numChannels) {
  const uint8_t byteOffset = 2;
  if (packetOffset == 0) {
    sample->timestamp = getTime();
    for (uint8_t i = 0; i < numChannels; i++) {
      sample->channelData[i] = 0.0;
    }
    sample->sampleNumber = arr[1];
  }

  if (numChannels == NUM_CHANNELS_GANGLION) {
    int32_t temp_raw[MAX_CHANNELS_PER_PACKET];
    // for (int i = 0; i < 24; i++) {
    //   Serial.printf("arr[%d]: %d\n", i+2, arr[i+2]);
    // }
    extractRaws(arr + 2, temp_raw, NUM_CHANNELS_GANGLION);
    transformRawsToScaledGanglion(temp_raw, sample->channelData);
  } else {
    int32_t temp_raw[MAX_CHANNELS_PER_PACKET];
    extractRaws(arr + 2, temp_raw, MAX_CHANNELS_PER_PACKET);
    transformRawsToScaledCyton(temp_raw, gains, packetOffset, sample->channelData);
  }
}
#endif

/**
 * Used to print out a long long number
 * @param n    {uint64_t} The signed number
 * @param base {uint8_t} The base you want to print in. DEC, HEX, BIN
 */
void OpenBCI_Wifi_Class::debugPrintLLNumber(long long n, uint8_t base) {
  Serial.print(getStringLLNumber(n, base));
}

/**
 * Used to print out a long long number. Forces the base to be DEC
 * @param n    {uint64_t} The unsigned number
 */
void OpenBCI_Wifi_Class::debugPrintLLNumber(long long n) {
  debugPrintLLNumber(n, DEC);
}

/**
 * Used to print out a long long number
 * @param n    {uint64_t} The unsigned number
 * @param base {uint8_t} The base you want to print in. DEC, HEX, BIN
 */
void OpenBCI_Wifi_Class::debugPrintLLNumber(unsigned long long n, uint8_t base) {
  Serial.print(getStringLLNumber(n, base));
}

/**
 * Used to print out a long long number. Forces the base to be DEC
 * @param n    {uint64_t} The unsigned number
 */
void OpenBCI_Wifi_Class::debugPrintLLNumber(unsigned long long n) {
  debugPrintLLNumber(n, DEC);
}

/**
 * Should extract raw int32_t array of length number of channels to get
 * @param arr         {uint8_t *} The array of 3byte signed 2's complement numbers
 * @param output      {int32_t *} An array of length `numChannels` to store
 *                    the extracted values
 * @param numChannels {uint8_t} The number of channels to pull out of `arr`
 */
void OpenBCI_Wifi_Class::extractRaws(uint8_t *arr, int32_t *output, uint8_t numChannels) {
  for (uint8_t i = 0; i < numChannels; i++) {
    output[i] = int24To32(arr + (i*3));
    // Serial.printf("%d\n", output[i]);
  }
}

/**
 * Convert an int24 into int32
 * @param  arr {uint8_t *} - 3-byte array of signed 24 bit number
 * @return     int32 - The converted number
 */
int32_t OpenBCI_Wifi_Class::int24To32(uint8_t *arr) {
  uint32_t raw = 0;
  raw = arr[0] << 16 | arr[1] << 8 | arr[2];
  // carry through the sign
  if(bitRead(raw,23) == 1){
    raw |= 0xFF000000;
  } else{
    raw &= 0x00FFFFFF;
  }
  return (int32_t)raw;
}

/**
* @description Test to see if a char follows the stream tail byte format
* @author AJ Keller (@pushtheworldllc)
*/
boolean OpenBCI_Wifi_Class::isAStreamByte(uint8_t b) {
  return (b >> 4) == 0xC;
}

String OpenBCI_Wifi_Class::perfectPrintByteHex(uint8_t b) {
  if (b <= 0x0F) {
    return "0" + String(b, HEX);
  } else {
    return String(b, HEX);
  }
}

void OpenBCI_Wifi_Class::loop(void) {

}

/**
 * Check to see if SNTP is active
 * @type {Number}
 */
boolean OpenBCI_Wifi_Class::ntpActive() {
  return time(nullptr) > 1000;
}

/**
 * Use this to start the sntp time sync
 */
void OpenBCI_Wifi_Class::ntpStart(void) {
#ifdef DEBUG
  Serial.println("Setting time using SNTP");
#endif
  configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");
}

/**
 * Calculate an adjusment based off an offset and the internal micros
 * @param  ntpOffset {unsigned long} A time in micro seconds
 * @return           {unsigned long long} The adjested offset.
 */
unsigned long long OpenBCI_Wifi_Class::ntpGetPreciseAdjustment(unsigned long ntpOffset) {
  unsigned long long boardTime_uS = micros() % MICROS_IN_SECONDS;
  unsigned long long adj = boardTime_uS - ntpOffset;
  if (boardTime_uS < ntpOffset) {
    boardTime_uS += MICROS_IN_SECONDS;
    adj = boardTime_uS - ntpOffset;
  }
  return adj;
}

/**
 * Get ntp time in microseconds
 * @return [long] - The time in micro second
 */
unsigned long long OpenBCI_Wifi_Class::ntpGetTime(void) {
  unsigned long long curTime = time(nullptr);
  return curTime * MICROS_IN_SECONDS;
}

void OpenBCI_Wifi_Class::passthroughBufferClear(void) {
  for (uint8_t i = 0; i < BYTES_PER_SPI_PACKET; i++) {
    passthroughBuffer[i] = 0;
  }
  passthroughPosition = 0;
}

uint8_t OpenBCI_Wifi_Class::passthroughCommands(String commands) {
  uint8_t numCmds = uint8_t(commands.length());
  if (numCmds > BYTES_PER_SPI_PACKET - 1) {
    return PASSTHROUGH_FAIL_TOO_MANY_CHARS;
  } else if (numCmds == 0) {
    return PASSTHROUGH_FAIL_NO_CHARS;
  }
  // Serial.printf("got %d commands | passthroughPosition: %d\n", numCmds, passthroughPosition);
  if (passthroughPosition > 0) {
    if (numCmds > BYTES_PER_SPI_PACKET - passthroughPosition-1) { // -1 because of numCmds as first byte
      return PASSTHROUGH_FAIL_QUEUE_FILLED;
    }
    passthroughBuffer[0] += numCmds;
  } else {
    passthroughBuffer[passthroughPosition++] = numCmds;
  }
  for (int i = 0; i < numCmds; i++) {
    // Serial.printf("cmd %c | passthroughPosition: %d\n", commands.charAt(i), passthroughPosition);
    passthroughBuffer[passthroughPosition++] = commands.charAt(i);
  }
  passthroughBufferLoaded = true;
  clientWaitingForResponse = true;
  timePassthroughBufferLoaded = millis();
  SPISlave.setData(passthroughBuffer, BYTES_PER_SPI_PACKET);
  return PASSTHROUGH_PASS;
}

/**
 * Used to convert the raw int32_t into a scaled double
 * @param  raw         {int32_t} - The raw value
 * @param  scaleFactor {double} - The scale factor to multiply by
 * @return             {double} - The raw value scaled by `scaleFactor`
 *                                converted into nano volts.
 */
double OpenBCI_Wifi_Class::rawToScaled(int32_t raw, double scaleFactor) {
  // Convert the three byte signed integer and convert it
  return scaleFactor * raw * NANO_VOLTS_IN_VOLTS;
}

#ifdef RAW_TO_JSON
/**
 * Resets all the samples assuming 16 channels
 */
void OpenBCI_Wifi_Class::sampleReset(void) {
  for (int i = 0; i < NUM_PACKETS_IN_RING_BUFFER_JSON; i++) {
    sampleReset(sampleBuffer + i);
  }
}

/**
 * Resets the sample assuming 16 channels
 * @param sample {Sample} - The sample struct
 */
void OpenBCI_Wifi_Class::sampleReset(Sample *sample) {
  sampleReset(sample, NUM_CHANNELS_CYTON_DAISY);
}

/**
 * Resets the sample
 * @param sample      {Sample} - The sample struct
 * @param numChannels {uint8_t} - The number of channels to clear to zero
 */
void OpenBCI_Wifi_Class::sampleReset(Sample *sample, uint8_t numChannels) {
  for (size_t i = 0; i < numChannels; i++) {
    sample->channelData[i] = 0.0;
  }
  sample->sampleNumber = 0;
  sample->timestamp = 0;
}
#endif
/**
 * Has the SPI Master polled this device in the past SPI_MASTER_POLL_TIMEOUT_MS
 * @returns [boolean] True if SPI Master has polled within timeout.
 */
boolean OpenBCI_Wifi_Class::spiHasMaster(void) {
  if (lastTimeWasPolled > 0) return millis() < lastTimeWasPolled + SPI_MASTER_POLL_TIMEOUT_MS;
  return false;
}

void OpenBCI_Wifi_Class::spiOnDataSent(void) {
  lastTimeWasPolled = millis();
  passthroughBufferClear();
}

void OpenBCI_Wifi_Class::spiProcessPacket(uint8_t *data) {
  if (isAStreamByte(data[0])) {
    spiProcessPacketStream(data);
  } else {
    switch (data[0]) {
      case WIFI_SPI_MSG_GAINS:
        spiProcessPacketGain(data);
        break;
      case WIFI_SPI_MSG_LAST:
      case WIFI_SPI_MSG_MULTI:
        spiProcessPacketResponse(data);
        break;
      default:
        break;
    }
  }
}

void OpenBCI_Wifi_Class::spiProcessPacketGain(uint8_t *data) {
  if (data[0] > 0) {
    if (data[0] == data[1]) {
      switch (data[0]) {
        case WIFI_SPI_MSG_GAINS:
          setGains(data);
          break;
        default:
          break;
      }
    }
  }
}

#ifdef RAW_TO_JSON
void OpenBCI_Wifi_Class::spiProcessPacketStreamJSON(uint8_t *data) {
  if (getNumChannels() > MAX_CHANNELS_PER_PACKET) {
    // DO DAISY
    if (lastSampleNumber != data[1]) {
      lastSampleNumber = data[1];
      // this is first packet of new sample
      if (head >= NUM_PACKETS_IN_RING_BUFFER_JSON) {
        head = 0;
      }
      // this is the first packet, no offset
      channelDataCompute(data, getGains(), sampleBuffer + head, 0, getNumChannels());
    } else {
      // this is not first packet of new sample
      channelDataCompute(data, getGains(), sampleBuffer + head, MAX_CHANNELS_PER_PACKET, getNumChannels());
      head++;
    }
  } else { // Cyton or Ganglion
    if (head >= NUM_PACKETS_IN_RING_BUFFER_JSON) {
      head = 0;
    }
    // Convert sample immidiate, store to buffer and get out!
    channelDataCompute(data, getGains(), sampleBuffer + head, 0, getNumChannels());
    head++;
  }
}
#endif

void OpenBCI_Wifi_Class::spiProcessPacketStreamRaw(uint8_t *data) {
  int newHead = rawBufferHead + 1;
  if (newHead >= NUM_PACKETS_IN_RING_BUFFER_RAW) {
    newHead = 0;
  }
  memcpy(rawBuffer + newHead, data, BYTES_PER_SPI_PACKET);
  rawBufferHead = newHead;
//   // Are we about to wrap around and over write the tail?
//   if (newHead == rawBufferTail) {
// #ifdef DEBUG
//     Serial.println("FAIL FAIL FAIL");
// #endif
//     return; // don't do that, ditch this packet
//   } else {
//     memcpy(rawBuffer + newHead, data, BYTES_PER_SPI_PACKET);
//     rawBufferHead = newHead;
//   }
}

void OpenBCI_Wifi_Class::spiProcessPacketStream(uint8_t *data) {
#ifdef RAW_TO_JSON
  if (curOutputMode == OUTPUT_MODE_JSON) {
    spiProcessPacketStreamJSON(data);
  } else {
    spiProcessPacketStreamRaw(data);
  }
#else
  spiProcessPacketStreamRaw(data);
#endif
}

/**
 * Used to process a packet from the SPI master then the server has a client
 *  waiting for a response. Occurs when a passthrough command occurs.
 * @param data {uint8_t *} - An array of data of length 32.
 */
void OpenBCI_Wifi_Class::spiProcessPacketResponse(uint8_t *data) {
  if (clientWaitingForResponse) {
    String newString = (char *)data;
    newString = newString.substring(1, BYTES_PER_SPI_PACKET);

    switch (data[0]) {
      case WIFI_SPI_MSG_MULTI:
        outputString.concat(newString);
        break;
      case WIFI_SPI_MSG_LAST:
        outputString.concat(newString);
        clientWaitingForResponse = false;
#ifdef DEBUG
        Serial.println(outputString);
#endif
        curClientResponse = CLIENT_RESPONSE_OUTPUT_STRING;
        clientWaitingForResponseFullfilled = true;
        break;
      default:
        curClientResponse = CLIENT_RESPONSE_NONE;
        clientWaitingForResponseFullfilled = true;
        clientWaitingForResponse = false;
        break;
    }
  }
}

/**
 * Used to convert extracted raw data and scale with gains for Cyton and Daisy
 * @param raw          {int32_t *} An array of int32_t of extracted raw values
 * @param gains        {uint8_t *} Should use the look up table to get scale
 *                     factor given the gain
 * @param packetOffset {uint8_t} The packet offset, always assume 8 channels
 *                     per packet. If this is a daisy for example, then
 *                     packetOffset should equal 8.
 * @param scaledOutput {double *} The calculated scaled value for each channel
 */
void OpenBCI_Wifi_Class::transformRawsToScaledCyton(int32_t *raw, uint8_t *gains, uint8_t packetOffset, double *scaledOutput) {
  for (uint8_t i = 0; i < MAX_CHANNELS_PER_PACKET; i++) {
    scaledOutput[i + packetOffset] = wifi.rawToScaled(raw[i], wifi.getScaleFactorVoltsCyton(gains[i + packetOffset]));
    // Serial.printf("[%d] raw: %d | gain: %d | scale: %.10f | output: %.0f\n", i + packetOffset, raw[i], gains[i + packetOffset], wifi.getScaleFactorVoltsCyton(gains[i + packetOffset]), scaledOutput[i + packetOffset]);
  }
}

/**
 * Used to convert extracted raw data and scale with gains for Ganglion. The
 *  Ganglion is fixed gain of 51.0
 * @param raw          {int32_t *} An array of int32_t of extracted raw values
 * @param scaledOutput {double *} The calculated scaled value for each channel
 */
void OpenBCI_Wifi_Class::transformRawsToScaledGanglion(int32_t *raw, double *scaledOutput) {
  for (uint8_t i = 0; i < NUM_CHANNELS_GANGLION; i++) {
    scaledOutput[i] = wifi.rawToScaled(raw[i], wifi.getScaleFactorVoltsGanglion());
    // Serial.printf("[%d] raw: %d | scale: %.10f | output: %.0f\n", i, raw[i], wifi.getScaleFactorVoltsGanglion(), scaledOutput[i]);
  }
}

OpenBCI_Wifi_Class wifi;

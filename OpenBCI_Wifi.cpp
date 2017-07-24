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

void OpenBCI_Wifi_Class::initArduino(void) {

}

/**
* @description Initalize arrays here
* @author AJ Keller (@pushtheworldllc)
*/
void OpenBCI_Wifi_Class::initArrays(void) {

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
  _ntpOffset = 0;
}

////////////////////////////
////////////////////////////
// GETTERS /////////////////
////////////////////////////
////////////////////////////

String OpenBCI_Wifi_Class::getOutputMode(OUTPUT_MODE outputMode) {
  switch(outputMode) {
    case OUTPUT_MODE_JSON:
      return "json";
    case OUTPUT_MODE_RAW:
    default:
      return "raw";
  }
}

double OpenBCI_Wifi_Class::getScaleFactorVoltsGanglion() {
  return MCP_SCALE_FACTOR_VOLTS;
}

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
// UTILITIES ///////////////
////////////////////////////
////////////////////////////

/**
 * Return true if the channel data array is full
 * @param arr           {uint8_t *] - 32 byte array from Cyton or Ganglion
 * @param gains         {uint8_t *} - 32 byte array from Cyton or Ganglion
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
  Serial.println();

  if (numChannels == NUM_CHANNELS_GANGLION) {
    int32_t temp_raw[NUM_CHANNELS_GANGLION];
    extractRaws(arr + 2, temp_raw, NUM_CHANNELS_GANGLION);
    transformRawsToScaledGanglion(temp_raw, sample->channelData);
  } else {
    int32_t temp_raw[MAX_CHANNELS_PER_PACKET];
    extractRaws(arr + 2, temp_raw, MAX_CHANNELS_PER_PACKET);
    transformRawsToScaledCyton(temp_raw, gains, packetOffset, sample->channelData);
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
 * Should extract raw int32_t array of length number of channels to get
 * @param arr         {uint8_t *} The array of 3byte signed 2's complement numbers
 * @param numChannels {uint8_t} The number of channels to pull out of `arr`
 * @param output      {int32_t *} An array of length `numChannels` to store
 *                    the extracted values
 */
void OpenBCI_Wifi_Class::extractRaws(uint8_t *arr, int32_t *output, uint8_t numChannels) {
  for (uint8_t i = 0; i < numChannels; i++) {
    output[i] = int24To32(arr + (i*3));
  }
}

/**
 * Check to see if SNTP is active
 * @type {Number}
 */
boolean OpenBCI_Wifi_Class::ntpActive() {
  return time(nullptr) > 1000;
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
  // for (size_t i = 0; i < numChannels; i++) {
  //   sample->channelData[i] = 0.0;
  // }
  // sample->sampleNumber = 0;
  // sample->timestamp = 0;
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
    scaledOutput[i + packetOffset] = wifi.rawToScaled(raw[i + packetOffset], wifi.getScaleFactorVoltsCyton(gains[i + packetOffset]));
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
  }
}

OpenBCI_Wifi_Class wifi;

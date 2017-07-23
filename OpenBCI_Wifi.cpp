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

}

String OpenBCI_Wifi_Class::getOutputMode(OUTPUT_MODE outputMode) {
  switch(outputMode) {
    case OUTPUT_MODE_JSON:
      return "json";
    case OUTPUT_MODE_RAW:
    default:
      return "raw";
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
 * Should extract raw int32_t array of length number of channels to get
 * @param arr         {uint8_t *} The array of 3byte signed 2's complement numbers
 * @param numChannels {uint8_t} The number of channels to pull out of `arr`
 * @param output      {int32_t *} An array of length `numChannels` to store
 *                    the extracted values
 */
void OpenBCI_Wifi_Class::extractRaws(uint8_t *arr, int32_t *output) {
  for (uint8_t i = 0; i < MAX_CHANNELS_PER_PACKET; i++) {
    output[i] = int24To32(arr + (i*3));
  }
}

/**
 * Used to convert extracted raw data and scale with gains for Cyton and Daisy
 * @param raw          {int32_t *} An array of int32_t of extracted raw values
 * @param gains        {uint8_t *} Should use the look up table to get scale
 *                     factor given the gain
 * @param numChannels  {uint8_t} The number of channels, size of `raw` and
 *                     `gains` and `scaledOutput` shall be of this length
 * @param scaledOutput {double *} The calculated scaled value for each channel
 */
void OpenBCI_Wifi_Class::transformRawsToScaledCyton(int32_t *raw, uint8_t *gains, uint8_t numChannels, double *scaledOutput) {
  for (uint8_t i = 0; i < numChannels; i++) {
    scaledOutput[i] = wifi.rawToScaled(raw[i], wifi.getScaleFactorVoltsCyton(gains[i]));
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

double OpenBCI_Wifi_Class::rawToScaled(int32_t raw, double scaleFactor) {
  // Convert the three byte signed integer and convert it
  return scaleFactor * raw * NANO_VOLTS_IN_VOLTS;
}

/**
 * Return true if the channel data array is full
 * @param arr [uint8_t *] - 32 byte array from Cyton or Ganglion
 * @param sample [Sample *] - Sample struct to hold data before conversion to float
 * @param packetOffset [uint8_t] - The offset to shift loading channel data into.
 *   i.e. should be 1 on second packet for daisy
 */
// void OpenBCI_Wifi_Class::channelDataCompute(uint8_t *arr, Sample *sample, uint8_t packetOffset) {
//   const uint8_t byteOffset = 2;
//   if (packetOffset == 0) {
//     // Serial.println(getTime());
//     sample->timestamp = getTime();
//     long temp[numChannels];
//     double temp_raw[numChannels];
//     double tmp[numChannels];
//     sample->channelData = temp;
//     sample->nano_volts = tmp;
//     sample->raw = temp_raw;
//
//     for (uint8_t i = 0; i < numChannels; i++) {
//       sample->channelData[i] = 0;
//       sample->nano_volts[i] = 0;
//       sample->raw[i] = 0;
//     }
//   }
//   for (uint8_t i = 0; i < MAX_CHANNELS_PER_PACKET; i++) {
//     // Zero out the new value
//     int raw = 0;
//     // Pull out 24bit number
//     raw = arr[i*3 + byteOffset] << 16 | arr[i*3 + 1 + byteOffset] << 8 | arr[i*3 + 2 + byteOffset];
//     // carry through the sign
//     if(bitRead(raw,23) == 1){
//       raw |= 0xFF000000;
//     } else{
//       raw &= 0x00FFFFFF;
//     }
//
//     double raw_d = (double)raw;
//
//     double volts = raw_d * scaleFactors[i + packetOffset];
//     double nano_volts = volts * NANO_VOLTS_IN_VOLTS;
//
//     sample->channelData[i + packetOffset] = (long)nano_volts;
//   }
//
//   // Serial.printf("Channel 1: %12.4f", sample->channelData[0]); Serial.println(" nV");
// }

OpenBCI_Wifi_Class wifi;

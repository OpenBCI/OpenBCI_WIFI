// #include <ESP8266HTTPUpdateServer.h>
// #include <ESP8266WiFi.h>
// #include <DNSServer.h>
// #include <ESP8266WebServer.h>
// #include <ESP8266SSDP.h>
// #include <WiFiManager.h>
// #include <ESP8266mDNS.h>
// #include <WiFiUdp.h>
// #include <ArduinoOTA.h>
// #include <PubSubClient.h>
// #include <ArduinoJson.h>
#include "OpenBCI_Wifi.h"
#include "PTW-Arduino-Assert.h"

int ledPin = 0;

void testGetGainCyton() {
  test.describe("getGainCyton");

  test.assertEqual(wifi.getGainCyton(wifi.CYTON_GAIN_1), 1, "should be able to get cyton gain of 1");
  test.assertEqual(wifi.getGainCyton(wifi.CYTON_GAIN_2), 2, "should be able to get cyton gain of 2");
  test.assertEqual(wifi.getGainCyton(wifi.CYTON_GAIN_4), 4, "should be able to get cyton gain of 4");
  test.assertEqual(wifi.getGainCyton(wifi.CYTON_GAIN_6), 6, "should be able to get cyton gain of 6");
  test.assertEqual(wifi.getGainCyton(wifi.CYTON_GAIN_8), 8, "should be able to get cyton gain of 8");
  test.assertEqual(wifi.getGainCyton(wifi.CYTON_GAIN_12), 12, "should be able to get cyton gain of 12");
  test.assertEqual(wifi.getGainCyton(wifi.CYTON_GAIN_24), 24, "should be able to get cyton gain of 24");

}

void testGetGainGanglion() {
  test.describe("getGainGanglion");

  test.assertEqual(wifi.getGainGanglion(), 51, "should get the gain of 51 for ganglion");
}

void testGetJSONAdditionalBytes() {
  test.describe("getJSONAdditionalBytes");

  test.assertEqual(wifi.getJSONAdditionalBytes(NUM_CHANNELS_GANGLION), 1164, "should get the right num of bytes for ganglion", __LINE__);
  test.assertEqual(wifi.getJSONAdditionalBytes(NUM_CHANNELS_CYTON), 1116, "should get the right num of bytes for cyton", __LINE__);
  test.assertEqual(wifi.getJSONAdditionalBytes(NUM_CHANNELS_CYTON_DAISY), 1212, "should get the right num of bytes for cyton daisy", __LINE__);
}

void testGetJSONBufferSize() {
  test.describe("getJSONBufferSize");

  wifi.reset();
  size_t intialSize = wifi.getJSONBufferSize();
  test.assertEqual(wifi.getJSONBufferSize(), (size_t)2836, "should initialize json buffer size to zero", __LINE__);
  wifi.setNumChannels(NUM_CHANNELS_CYTON_DAISY);
  test.assertGreaterThan(wifi.getJSONBufferSize(), intialSize, "should set the json buffer greater than zero or inital", __LINE__);
  test.assertLessThan(wifi.getJSONBufferSize(), (size_t)3000, "should be less than 3000bytes per chunk", __LINE__);

}

void testGetJSONFromSamplesCyton() {
  test.describe("getJSONFromSamplesCyton");

  wifi.reset(); // Clear everything
  uint8_t numChannels = NUM_CHANNELS_CYTON;
  uint8_t numSamples = 1;
  uint8_t expected_sampleNumber = 29;
  unsigned long long expected_timestamp = 1500916934017000;
  double expected_channelData[MAX_CHANNELS];
  for (int i = 0; i < MAX_CHANNELS; i++) {
    if (i%2 == 0) {
      expected_channelData[i] = ADC_24BIT_MAX_VAL_NANO_VOLT;
    } else {
      expected_channelData[i] = -1 * ADC_24BIT_MAX_VAL_NANO_VOLT;
    }
  }

  wifi.sampleBuffer->sampleNumber = expected_sampleNumber;
  wifi.sampleBuffer->timestamp = expected_timestamp;
  for (int i = 0; i < numChannels; i++) {
    wifi.sampleBuffer->channelData[i] = expected_channelData[i];
  }
  String actual_serializedOutput = wifi.getJSONFromSamples(numChannels, numSamples);
  const size_t bufferSize = JSON_ARRAY_SIZE(2) + JSON_ARRAY_SIZE(8) + JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(3) + 220;
  DynamicJsonBuffer jsonBuffer(bufferSize);
  JsonObject& root = jsonBuffer.parseObject(actual_serializedOutput);
  JsonObject& chunk0 = root["chunk"][0];
  double actual_timestamp = chunk0["timestamp"]; // 1500916934017000
  test.assertEqual((unsigned long long)actual_timestamp, expected_timestamp, "should be able to set timestamp", __LINE__);

  uint8_t actual_sampleNumber = chunk0["sampleNumber"]; // 255
  test.assertEqual(actual_sampleNumber, expected_sampleNumber, "should be able to set sampleNumber", __LINE__);

  JsonArray& chunk0_data = chunk0["data"];
  for (int i = 0; i < numChannels; i++) {
    double chunk0_tempData = chunk0_data[i];
    test.assertApproximately(chunk0_data[i], expected_channelData[i], 1.0, "should be able to code large numbers", __LINE__);
  }

  // Cyton with three packets
  wifi.reset(); // Clear everything
  wifi.setNumChannels(numChannels);
  numSamples = wifi.getJSONMaxPackets(numChannels);
  for (uint8_t i = 0; i < numSamples; i++) {
    wifi.sampleReset(wifi.sampleBuffer + i);
    (wifi.sampleBuffer + i)->sampleNumber = expected_sampleNumber + i;
    (wifi.sampleBuffer + i)->timestamp = expected_timestamp + i;
    for (uint8_t j = 0; j < numChannels; j++) {
      (wifi.sampleBuffer + i)->channelData[j] = expected_channelData[j];
      // Serial.printf("j:%d: i:%d %.0f\n", j, i, (wifi.sampleBuffer + i)->channelData[j]);
    }
  }
  actual_serializedOutput = wifi.getJSONFromSamples(numChannels, numSamples);
  const size_t bufferSize1 = JSON_ARRAY_SIZE(3) + 3*JSON_ARRAY_SIZE(8) + JSON_OBJECT_SIZE(2) + 3*JSON_OBJECT_SIZE(3) + 650;
  DynamicJsonBuffer jsonBuffer1(bufferSize1);

  JsonObject& root1 = jsonBuffer1.parseObject(actual_serializedOutput.c_str());
  JsonArray& chunk = root1["chunk"];

  for (uint8_t i = 0; i < numSamples; i++) {
    JsonObject& sample = chunk[i];
    unsigned long long actual_timestamp = sample["timestamp"]; // 1500916934017000
    test.assertEqual(actual_timestamp, expected_timestamp + i, String("should be able to set timestamp for " + String(i)).c_str(), __LINE__);

    uint8_t actual_sampleNumber = sample["sampleNumber"]; // 255
    test.assertEqual(actual_sampleNumber, expected_sampleNumber + i, String("should be able to set sampleNumber for " + String(i)).c_str(), __LINE__);

    JsonArray& sample_data = sample["data"];
    for (uint8_t j = 0; j < numChannels; j++) {
      double temp_d = sample_data[j];
      // Serial.printf("\nj[%d]: td:%0.0f e_cD:%.0f\n", j, temp_d, expected_channelData[j]-i);
      test.assertApproximately(temp_d, expected_channelData[j], 1.0, String("should be able to code large numbers "+String(j)+" for sample " + String(i)).c_str(), __LINE__);
    }
  }
}

void testGetJSONFromSamplesCytonDaisy() {
  test.describe("getJSONFromSamplesCytonDaisy");

  wifi.reset(); // Clear everything
  uint8_t numChannels = NUM_CHANNELS_CYTON_DAISY;
  wifi.setNumChannels(numChannels);
  uint8_t numSamples = 1;
  uint8_t expected_sampleNumber = 29;
  unsigned long long expected_timestamp = 1500916934017000;
  double expected_channelData[MAX_CHANNELS];
  for (int i = 0; i < MAX_CHANNELS; i++) {
    if (i%2 == 0) {
      expected_channelData[i] = ADC_24BIT_MAX_VAL_NANO_VOLT;
    } else {
      expected_channelData[i] = -1 * ADC_24BIT_MAX_VAL_NANO_VOLT;
    }
  }

  wifi.sampleBuffer->sampleNumber = expected_sampleNumber;
  wifi.sampleBuffer->timestamp = expected_timestamp;
  for (int i = 0; i < numChannels; i++) {
    wifi.sampleBuffer->channelData[i] = expected_channelData[i];
  }
  String actual_serializedOutput = wifi.getJSONFromSamples(numChannels, numSamples);
  const size_t bufferSize = JSON_ARRAY_SIZE(2) + JSON_ARRAY_SIZE(8) + JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(3) + 220;
  DynamicJsonBuffer jsonBuffer(bufferSize);
  JsonObject& root = jsonBuffer.parseObject(actual_serializedOutput);
  JsonObject& chunk0 = root["chunk"][0];
  double actual_timestamp = chunk0["timestamp"]; // 1500916934017000
  test.assertEqual((unsigned long long)actual_timestamp, expected_timestamp, "daisy should be able to set timestamp", __LINE__);

  uint8_t actual_sampleNumber = chunk0["sampleNumber"]; // 255
  test.assertEqual(actual_sampleNumber, expected_sampleNumber, "daisy should be able to set sampleNumber", __LINE__);

  JsonArray& chunk0_data = chunk0["data"];
  for (int i = 0; i < numChannels; i++) {
    double chunk0_tempData = chunk0_data[i];
    test.assertApproximately(chunk0_data[i], expected_channelData[i], 1.0, "daisy should be able to code large numbers", __LINE__);
  }

  // Cyton Daisy with three packets
  wifi.reset(); // Clear everything
  wifi.setNumChannels(numChannels);
  numSamples = wifi.getJSONMaxPackets(numChannels);
  for (uint8_t i = 0; i < numSamples; i++) {
    wifi.sampleReset(wifi.sampleBuffer + i);
    (wifi.sampleBuffer + i)->sampleNumber = expected_sampleNumber + i;
    (wifi.sampleBuffer + i)->timestamp = expected_timestamp + i;
    for (uint8_t j = 0; j < numChannels; j++) {
      (wifi.sampleBuffer + i)->channelData[j] = expected_channelData[j];
      // Serial.printf("j:%d: i:%d %.0f\n", j, i, (wifi.sampleBuffer + i)->channelData[j]);
    }
  }
  actual_serializedOutput = wifi.getJSONFromSamples(numChannels, numSamples);
  const size_t bufferSize1 = JSON_ARRAY_SIZE(3) + 3*JSON_ARRAY_SIZE(8) + JSON_OBJECT_SIZE(2) + 3*JSON_OBJECT_SIZE(3) + 650;
  DynamicJsonBuffer jsonBuffer1(bufferSize1);

  JsonObject& root1 = jsonBuffer1.parseObject(actual_serializedOutput.c_str());
  JsonArray& chunk = root1["chunk"];

  for (uint8_t i = 0; i < numSamples; i++) {
    JsonObject& sample = chunk[i];
    unsigned long long actual_timestamp = sample["timestamp"]; // 1500916934017000
    test.assertEqual(actual_timestamp, expected_timestamp + i, String("should be able to set timestamp for " + String(i)).c_str(), __LINE__);

    uint8_t actual_sampleNumber = sample["sampleNumber"]; // 255
    test.assertEqual(actual_sampleNumber, expected_sampleNumber + i, String("should be able to set sampleNumber for " + String(i)).c_str(), __LINE__);

    JsonArray& sample_data = sample["data"];
    for (uint8_t j = 0; j < numChannels; j++) {
      double temp_d = sample_data[j];
      // Serial.printf("\nj[%d]: td:%0.0f e_cD:%.0f\n", j, temp_d, expected_channelData[j]-i);
      test.assertApproximately(temp_d, expected_channelData[j], 1.0, String("should be able to code large numbers "+String(j)+" for sample " + String(i)).c_str(), __LINE__);
    }
  }
}

void testGetJSONFromSamplesGanglion() {
  test.describe("getJSONFromSamplesGanglion");

  wifi.reset(); // Clear everything
  uint8_t numChannels = NUM_CHANNELS_GANGLION;
  wifi.setNumChannels(numChannels);
  uint8_t numSamples = 1;
  uint8_t expected_sampleNumber = 29;
  unsigned long long expected_timestamp = 1500916934017000;
  double expected_channelData[MAX_CHANNELS];
  for (int i = 0; i < MAX_CHANNELS; i++) {
    if (i%2 == 0) {
      expected_channelData[i] = ADC_24BIT_MAX_VAL_NANO_VOLT;
    } else {
      expected_channelData[i] = -1 * ADC_24BIT_MAX_VAL_NANO_VOLT;
    }
  }

  wifi.sampleBuffer->sampleNumber = expected_sampleNumber;
  wifi.sampleBuffer->timestamp = expected_timestamp;
  for (int i = 0; i < numChannels; i++) {
    wifi.sampleBuffer->channelData[i] = expected_channelData[i];
  }
  String actual_serializedOutput = wifi.getJSONFromSamples(numChannels, numSamples);
  const size_t bufferSize = JSON_ARRAY_SIZE(2) + JSON_ARRAY_SIZE(4) + JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(3) + 220;
  DynamicJsonBuffer jsonBuffer(bufferSize);
  JsonObject& root = jsonBuffer.parseObject(actual_serializedOutput);
  JsonObject& chunk0 = root["chunk"][0];
  double actual_timestamp = chunk0["timestamp"]; // 1500916934017000
  test.assertEqual((unsigned long long)actual_timestamp, expected_timestamp, "ganglion should be able to set timestamp", __LINE__);

  uint8_t actual_sampleNumber = chunk0["sampleNumber"]; // 255
  test.assertEqual(actual_sampleNumber, expected_sampleNumber, "ganglion should be able to set sampleNumber", __LINE__);

  JsonArray& chunk0_data = chunk0["data"];
  for (int i = 0; i < numChannels; i++) {
    double chunk0_tempData = chunk0_data[i];
    test.assertApproximately(chunk0_data[i], expected_channelData[i], 1.0, "ganglion should be able to code large numbers", __LINE__);
  }

  // Ganglion with eight packets
  wifi.reset(); // Clear everything
  wifi.setNumChannels(numChannels);
  numSamples = wifi.getJSONMaxPackets(numChannels);
  for (uint8_t i = 0; i < numSamples; i++) {
    wifi.sampleReset(wifi.sampleBuffer + i);
    (wifi.sampleBuffer + i)->sampleNumber = expected_sampleNumber + i;
    (wifi.sampleBuffer + i)->timestamp = expected_timestamp + i;
    for (uint8_t j = 0; j < numChannels; j++) {
      (wifi.sampleBuffer + i)->channelData[j] = expected_channelData[j];
      // Serial.printf("j:%d: i:%d %.0f\n", j, i, (wifi.sampleBuffer + i)->channelData[j]);
    }
  }
  actual_serializedOutput = wifi.getJSONFromSamples(numChannels, numSamples);
  const size_t bufferSize1 = JSON_ARRAY_SIZE(3) + 3*JSON_ARRAY_SIZE(8) + JSON_OBJECT_SIZE(2) + 3*JSON_OBJECT_SIZE(3) + 650;
  DynamicJsonBuffer jsonBuffer1(bufferSize1);

  JsonObject& root1 = jsonBuffer1.parseObject(actual_serializedOutput.c_str());
  JsonArray& chunk = root1["chunk"];

  for (uint8_t i = 0; i < numSamples; i++) {
    JsonObject& sample = chunk[i];
    unsigned long long actual_timestamp = sample["timestamp"]; // 1500916934017000
    test.assertEqual(actual_timestamp, expected_timestamp + i, String("ganglion multi packet should be able to set timestamp for " + String(i)).c_str(), __LINE__);

    uint8_t actual_sampleNumber = sample["sampleNumber"]; // 255
    test.assertEqual(actual_sampleNumber, expected_sampleNumber + i, String("ganglion multi packet should be able to set sampleNumber for " + String(i)).c_str(), __LINE__);

    JsonArray& sample_data = sample["data"];
    for (uint8_t j = 0; j < numChannels; j++) {
      double temp_d = sample_data[j];
      // Serial.printf("\nj[%d]: td:%0.0f e_cD:%.0f\n", j, temp_d, expected_channelData[j]-i);
      test.assertApproximately(temp_d, expected_channelData[j], 1.0, String("ganglion multi packet should be able to code large numbers "+String(j)+" for sample " + String(i)).c_str(), __LINE__);
    }
  }
}

void testGetJSONFromSamples() {
  testGetJSONFromSamplesCyton();
  testGetJSONFromSamplesCytonDaisy();
  testGetJSONFromSamplesGanglion();
}

void testGetJSONMaxPackets() {
  test.describe("getJSONMaxPackets");

  test.assertEqual(wifi.getJSONMaxPackets(NUM_CHANNELS_GANGLION), 8, "should get the correct number for packets for ganglion", __LINE__);
  test.assertEqual(wifi.getJSONMaxPackets(NUM_CHANNELS_CYTON), 5, "should get the correct number for packets for cyton daisy", __LINE__);
  test.assertEqual(wifi.getJSONMaxPackets(NUM_CHANNELS_CYTON_DAISY), 3, "should get the correct number for packets for cyton", __LINE__);
}


void testGetOutputMode() {
  test.describe("getOutputMode");

  // RAW
  test.assertEqual(wifi.getOutputMode(wifi.OUTPUT_MODE_RAW),"raw","Should have gotten 'raw' string",__LINE__);

  // JSON
  test.assertEqual(wifi.getOutputMode(wifi.OUTPUT_MODE_JSON),"json","Should have gotten 'json' string",__LINE__);
}

void testGetStringLLNumber() {
  test.describe("getStringLLNumber");

  unsigned long long temp_ull = 8388607000000000;

  String actualString = wifi.getStringLLNumber(temp_ull);
  test.assertEqual(actualString, "8388607000000000", "should be able to convert unsigned long long", __LINE__);

  double temp_d = 8388607000000000.5;
  actualString = wifi.getStringLLNumber((unsigned long long)temp_d);
  test.assertEqual(actualString, "8388607000000000", "should be able to convert a positive double", __LINE__);

  temp_d = -8388606000000000.122;
  actualString = wifi.getStringLLNumber((long long)temp_d);
  test.assertEqual(actualString, "-8388606000000000", "should be able to convert a negative double", __LINE__);


  temp_d = ADC_24BIT_MAX_VAL_NANO_VOLT;
  actualString = wifi.getStringLLNumber((unsigned long long)temp_d);
  test.assertEqual(actualString, "8388607000000000", "should be able to convert a positive double again", __LINE__);

  temp_d = -1* ADC_24BIT_MAX_VAL_NANO_VOLT;
  actualString = wifi.getStringLLNumber((long long)temp_d);
  test.assertEqual(actualString, "-8388607000000000", "should be able to convert a negative double again", __LINE__);
}

void testGetScaleFactorVoltsCyton() {
  test.describe("getScaleFactorVoltsCyton");

  uint8_t expectedGain = 1;
  double epsilon = 0.000000001;
  double expectedScaleFactor = 4.5 / expectedGain / (pow(2,23) - 1);
  test.assertApproximately(wifi.getScaleFactorVoltsCyton(expectedGain), expectedScaleFactor, epsilon, "should be within 0.000001 with gain 1",__LINE__);

  expectedGain = 2;
  expectedScaleFactor = 4.5 / expectedGain / (pow(2,23) - 1);
  test.assertApproximately(wifi.getScaleFactorVoltsCyton(expectedGain), expectedScaleFactor, epsilon, "should be within 0.000001 with gain 2",__LINE__);

  expectedGain = 4;
  expectedScaleFactor = 4.5 / expectedGain / (pow(2,23) - 1);
  test.assertApproximately(wifi.getScaleFactorVoltsCyton(expectedGain), expectedScaleFactor, epsilon, "should be within 0.000001 with gain 4",__LINE__);

  expectedGain = 6;
  expectedScaleFactor = 4.5 / expectedGain / (pow(2,23) - 1);
  test.assertApproximately(wifi.getScaleFactorVoltsCyton(expectedGain), expectedScaleFactor, epsilon, "should be within 0.000001 with gain 6",__LINE__);

  expectedGain = 8;
  expectedScaleFactor = 4.5 / expectedGain / (pow(2,23) - 1);
  test.assertApproximately(wifi.getScaleFactorVoltsCyton(expectedGain), expectedScaleFactor, epsilon, "should be within 0.000001 with gain 8",__LINE__);

  expectedGain = 12;
  expectedScaleFactor = 4.5 / expectedGain / (pow(2,23) - 1);
  test.assertApproximately(wifi.getScaleFactorVoltsCyton(expectedGain), expectedScaleFactor, epsilon, "should be within 0.000001 with gain 12",__LINE__);

  expectedGain = 24;
  expectedScaleFactor = 4.5 / expectedGain / (pow(2,23) - 1);
  test.assertApproximately(wifi.getScaleFactorVoltsCyton(expectedGain), expectedScaleFactor, epsilon, "should be within 0.000001 with gain 24",__LINE__);
}

void testGetScaleFactorVoltsGanglion() {
  test.describe("getScaleFactorVoltsGanglion");

  double expectedScaleFactor = 1.2 / 51.0 / 1.5 / (pow(2,23) - 1);
  double epsilon = 0.0000000001;
  test.assertApproximately(wifi.getScaleFactorVoltsGanglion(), expectedScaleFactor, epsilon, "should be within 0.000001",__LINE__);

}

void testGetters() {
  testGetGainCyton();
  testGetGainGanglion();
  testGetJSONAdditionalBytes();
  testGetJSONBufferSize();
  testGetJSONFromSamples();
  testGetJSONMaxPackets();
  testGetOutputMode();
  testGetStringLLNumber();
  testGetScaleFactorVoltsCyton();
  testGetScaleFactorVoltsGanglion();
}

void testSetGain() {
  test.describe("setGains");

  uint8_t byteCounter = 0;
  uint8_t numChannels = NUM_CHANNELS_CYTON;
  uint8_t rawGainArray[numChannels];
  uint8_t actual_gains[numChannels];
  uint8_t expected_gains[numChannels];

  rawGainArray[byteCounter++] = WIFI_SPI_MSG_LAST;
  rawGainArray[byteCounter++] = WIFI_SPI_MSG_LAST;
  rawGainArray[byteCounter++] = numChannels;

  for (int i = 0; i < numChannels; i++) {
    rawGainArray[byteCounter++] = wifi.CYTON_GAIN_2;
    expected_gains[i] = wifi.getGainCyton(wifi.CYTON_GAIN_2);
  }

  wifi.setGains(rawGainArray, actual_gains);

  boolean allCorrect = true;

  for (int i = 0; i < numChannels; i++) {
    if (actual_gains[i] != expected_gains[i]) {
      allCorrect = false;
    }
  }

  test.assertTrue(true, "should be able to set all the gains correctly", __LINE__);
}

void testSetNumChannels() {
  test.describe("setNumChannels");

  uint8_t expected_numChannels = wifi.getNumChannels() + NUM_CHANNELS_CYTON;
  size_t expected_jsonBufferSize = wifi.getJSONBufferSize();
  wifi.setNumChannels(expected_numChannels);
  test.assertEqual(wifi.getNumChannels(), expected_numChannels, "should be able to set the number of channels",__LINE__);
  test.assertGreaterThan(wifi.getJSONBufferSize(), expected_jsonBufferSize, "should increase the size of the json buffer after raising the number of channels",__LINE__);
}

void testSetNTPOffset() {
  test.describe("setNTPOffset");
  unsigned long expected_ntpOffset = wifi.getNTPOffset() * 2;
  wifi.setNTPOffset(expected_ntpOffset);
  test.assertEqual(wifi.getNTPOffset(), expected_ntpOffset, "should be able to set the ntp offset", __LINE__);
}

void testReset() {
  test.describe("reset");

  wifi.setNTPOffset(123456);
  wifi.setNumChannels(NUM_CHANNELS_CYTON_DAISY);

  wifi.reset();
  test.assertFalse(wifi.tcpDelimiter, "should set tcpDelimiter to false", __LINE__);
  test.assertEqual((int)&wifi.curRawBuffer, (int)&wifi.rawBuffer, "should point cur raw buffer to head of buffer", __LINE__);
  test.assertEqual(wifi.getHead(), 0, "should reset head to 0", __LINE__);
  test.assertEqual(wifi.getTail(), 0, "should reset tail to 0", __LINE__);
  test.assertEqual(wifi.getJSONBufferSize(), (size_t)2836, "should reset jsonBufferSize to 0", __LINE__);
  test.assertEqual(wifi.getNTPOffset(), (unsigned long)0, "should reset ntpOffset to 0", __LINE__);
  test.assertEqual(wifi.getNumChannels(), 8, "should reset numChannels to 8", __LINE__);
}

void testSetters() {
  testReset();
  testSetGain();
  testSetNumChannels();
  testSetNTPOffset();
}

void testChannelDataComputeCyton() {
  test.describe("channelDataComputeCyton");

  uint8_t numChannels = NUM_CHANNELS_CYTON;
  uint8_t packetOffset = 0;
  uint8_t gains[NUM_CHANNELS_CYTON];
  uint8_t arr[BYTES_PER_SPI_PACKET];
  double expected_channelData[numChannels];
  uint8_t expected_sampleNumber = 20;
  uint8_t index = 0;
  for (int i = 0; i < 24; i++) {
    arr[i+2] = 0;
    if (i%3==2) {
      arr[i+2]=index+1;
      gains[index] = 24;
      int32_t raww = wifi.int24To32(arr + (index*3)+2);
      expected_channelData[index] = wifi.rawToScaled(raww, wifi.getScaleFactorVoltsCyton(gains[index]));
      index++;
    }
  }
  arr[1] = expected_sampleNumber;

  wifi.sampleReset(wifi.sampleBuffer);
  wifi.channelDataCompute(arr, gains, wifi.sampleBuffer, packetOffset, numChannels);
  for (uint8_t i = 0; i < numChannels; i++) {
    test.assertApproximately(wifi.sampleBuffer->channelData[i], expected_channelData[i], 1.0);
  }
  test.assertEqualHex(wifi.sampleBuffer->sampleNumber, expected_sampleNumber, "should be able to extract the sample number", __LINE__);
  test.assertGreaterThan(wifi.sampleBuffer->timestamp, (unsigned long long)0, "should be able to set the timestamp", __LINE__);
}

void testChannelDataComputeDaisy() {
  test.describe("channelDataComputeDaisy");

  uint8_t numChannels = NUM_CHANNELS_CYTON_DAISY;
  uint8_t packetOffset = 0;
  uint8_t gains[numChannels];
  uint8_t arr[BYTES_PER_SPI_PACKET];
  double expected_channelData[numChannels];
  uint8_t expected_sampleNumber = 20;
  uint8_t index = 0;
  for (int i = 0; i < 24; i++) {
    arr[i+2] = 0;
    if (i%3==2) {
      arr[i+2]=index+1;
      gains[index] = 24;
      int32_t raww = wifi.int24To32(arr + (index*3)+2);
      expected_channelData[index] = wifi.rawToScaled(raww, wifi.getScaleFactorVoltsCyton(gains[index]));
      index++;
    }
  }
  arr[1] = expected_sampleNumber;

  // Now for daisy
  wifi.sampleReset(wifi.sampleBuffer);
  wifi.channelDataCompute(arr, gains, wifi.sampleBuffer, packetOffset, numChannels);
  test.assertEqualHex(wifi.sampleBuffer->sampleNumber, expected_sampleNumber, "should be able to extract the sample number", __LINE__);
  test.assertGreaterThan(wifi.sampleBuffer->timestamp, (uint64_t)0, "should be able to set the timestamp", __LINE__);

  unsigned long long firstPacketTime = wifi.sampleBuffer->timestamp;
  packetOffset = MAX_CHANNELS_PER_PACKET;
  wifi.channelDataCompute(arr, gains, wifi.sampleBuffer, packetOffset, numChannels);
  for (uint8_t i = 0; i < numChannels; i++) {
    if (i >= packetOffset) {
      test.assertApproximately(wifi.sampleBuffer->channelData[i + packetOffset], expected_channelData[i], 1.0);
    } else {
      test.assertApproximately(wifi.sampleBuffer->channelData[i], expected_channelData[i], 1.0);
    }
  }
  test.assertEqualHex(wifi.sampleBuffer->sampleNumber, expected_sampleNumber, "should still have valid sample number", __LINE__);
  test.assertGreaterThan(wifi.sampleBuffer->timestamp, (uint64_t)0, "should be able to set the timestamp", __LINE__);
}

void testChannelDataComputeGanglion() {
  test.describe("channelDataComputeGanglion");

  uint8_t numChannels = NUM_CHANNELS_GANGLION;
  uint8_t packetOffset = 0;
  uint8_t gains[NUM_CHANNELS_GANGLION];
  uint8_t expected_sampleNumber = 20;
  uint8_t arr[MAX_CHANNELS_PER_PACKET * BYTES_PER_CHANNEL];
  double expected_channelData[numChannels];
  uint8_t index = 0;
  for (int i = 0; i < 24; i++) {
    arr[i] = 0;
    if (i%3==2) {
      arr[i]=index+1;
      gains[index] = 24;
      int32_t raww = wifi.int24To32(arr + (i*3));
      expected_channelData[index] = wifi.rawToScaled(raww, wifi.getScaleFactorVoltsCyton(24));
      index++;
    }
  }
  arr[1] = expected_sampleNumber;

  wifi.sampleReset(wifi.sampleBuffer);

  wifi.channelDataCompute(arr, gains, wifi.sampleBuffer, packetOffset, numChannels);

  for (uint8_t i = 0; i < numChannels; i++) {
    test.assertApproximately(wifi.sampleBuffer->channelData[i], expected_channelData[i], 1.0);
  }
  test.assertEqualHex(wifi.sampleBuffer->sampleNumber, expected_sampleNumber, "should be able to extract the sample number", __LINE__);
  test.assertGreaterThan(wifi.sampleBuffer->timestamp, (uint64_t)0, "should be able to set the timestamp", __LINE__);
}

void testExtractRaws() {
  test.describe("extractRaws");

  uint8_t expected_gain = 24;
  double channelScaleFactor = ADS1299_VREF / expected_gain / (2^23 - 1);
  uint8_t expectedNumChannels = MAX_CHANNELS_PER_PACKET;

  uint8_t arr[MAX_CHANNELS_PER_PACKET * BYTES_PER_CHANNEL];
  for (int i = 0; i < 24; i++) {
    arr[i] = 0;

    if (i%3==2) {
      arr[i]=(i/3)+1;
    }
  }

  int32_t actual_raws[MAX_CHANNELS_PER_PACKET];
  wifi.extractRaws(arr, actual_raws, MAX_CHANNELS_PER_PACKET);

  for (uint8_t i = 0; i < MAX_CHANNELS_PER_PACKET; i++) {
    test.assertEqual(actual_raws[i], (int32_t)i+1, "should match the index number", __LINE__);
  }

}

void testInt24To32() {
  test.describe("int24To32");

  uint8_t arr[3];
  arr[0] = 0x00; arr[1] = 0x06; arr[2] = 0x90;
  test.assertEqual(wifi.int24To32(arr), 1680, "Should convert a small positive number", __LINE__);

  arr[0] = 0x02; arr[1] = 0xC0; arr[2] = 0x01; // 0x02C001 === 180225
  test.assertEqual(wifi.int24To32(arr), 180225, "converts a large positive number", __LINE__);

  arr[0] = 0xFF; arr[1] = 0xFF; arr[2] = 0xFF; // 0xFFFFFF === -1
  test.assertEqual(wifi.int24To32(arr), -1, "converts a small negative number", __LINE__);

  arr[0] = 0x81; arr[1] = 0xA1; arr[2] = 0x01; // 0x81A101 === -8281855
  test.assertEqual(wifi.int24To32(arr), -8281855, "converts a large negative number", __LINE__);
}

void testTransformRawsToScaledCyton() {
  test.describe("transformRawsToScaledCyton");
  uint8_t numChannels = NUM_CHANNELS_CYTON_DAISY;
  double expected_scaledOutput[numChannels];
  double actual_scaledOutput[numChannels];
  int32_t raw[numChannels];
  uint8_t gains[numChannels];
  for (uint8_t i = 0; i < numChannels; i++) {
    actual_scaledOutput[i] = 0.0;
    raw[i] = i;
    if (i < 2) {
      gains[i] = 1;
    } else if (i < 4) {
      gains[i] = 8;
    } else if (i < 6) {
      gains[i] = 12;
    } else {
      gains[i] = 24;
    }

    expected_scaledOutput[i] = wifi.rawToScaled(raw[i], wifi.getScaleFactorVoltsCyton(gains[i]));
  }

  // Do Cyton first
  wifi.transformRawsToScaledCyton(raw, gains, 0, actual_scaledOutput);

  for (uint8_t i = 0; i < NUM_CHANNELS_CYTON; i++) {
    test.assertApproximately(actual_scaledOutput[i], expected_scaledOutput[i], 1.0, "should be able to transform raws to scaled for cyton", __LINE__);
    actual_scaledOutput[i] = 0.0; // clear
  }

  // Then daisy
  wifi.transformRawsToScaledCyton(raw, gains, MAX_CHANNELS_PER_PACKET, actual_scaledOutput);

  for (uint8_t i = NUM_CHANNELS_CYTON; i < NUM_CHANNELS_CYTON_DAISY; i++) {
    test.assertApproximately(actual_scaledOutput[i], expected_scaledOutput[i], 1.0, "should be able to transform raws to scaled for cyton daisy", __LINE__);
  }
}

void testTransformRawsToScaledGanglion() {
  test.describe("transformRawsToScaledGanglion");

  double expected_scaledOutput[NUM_CHANNELS_GANGLION];
  double actual_scaledOutput[NUM_CHANNELS_GANGLION];
  int32_t raw[NUM_CHANNELS_GANGLION];
  for (uint8_t i = 0; i < NUM_CHANNELS_GANGLION; i++) {
    actual_scaledOutput[i] = 0.0;
    raw[i] = i;

    expected_scaledOutput[i] = wifi.rawToScaled(raw[i], wifi.getScaleFactorVoltsGanglion());
  }

  // Do Ganglion
  wifi.transformRawsToScaledGanglion(raw, actual_scaledOutput);

  for (uint8_t i = 0; i < NUM_CHANNELS_GANGLION; i++) {
    test.assertApproximately(actual_scaledOutput[i], expected_scaledOutput[i], 1.0, "should be able to transform raws to scaled for cyton", __LINE__);
  }

}

void testRawToScaled() {
    test.describe("rawToScaled");

    int32_t raw = 1;
    double epsilon = 1.0;
    double scaleFactor = 5.0;
    double expected_output = 5000000000.0;
    double actual_output = wifi.rawToScaled(raw, scaleFactor);
    test.assertApproximately(actual_output, expected_output, epsilon, "should be able to convert to scaled small positive double", __LINE__);

    raw = 123456789;
    scaleFactor = 0.0000001234;
    epsilon = 50.0;
    expected_output = 15234567762.6; // on mac
    actual_output = wifi.rawToScaled(raw, scaleFactor);
    test.assertApproximately(actual_output, expected_output, epsilon, "should be able to convert to scaled large positive double", __LINE__);

    raw = -1;
    scaleFactor = 5.0;
    expected_output = -5000000000.0;
    epsilon = 1.0;
    actual_output = wifi.rawToScaled(raw, scaleFactor);
    test.assertApproximately(actual_output, expected_output, epsilon, "should be able to convert to scaled small negative double", __LINE__);

    raw = -123456789;
    scaleFactor = 0.0000001234;
    expected_output = -15234567762.6; // on mac
    epsilon = 50.0;
    actual_output = wifi.rawToScaled(raw, scaleFactor);
    test.assertApproximately(actual_output, expected_output, epsilon, "should be able to convert to scaled large positive double", __LINE__);
}

void testSampleReset() {
  test.describe("sampleReset");

  uint8_t numChannels = NUM_CHANNELS_CYTON_DAISY;
  wifi.sampleBuffer->timestamp = 20;
  wifi.sampleBuffer->sampleNumber = 21;
  for (int i = 0; i < numChannels; i++) {
    wifi.sampleBuffer->channelData[i] = 1099.0;
  }
  // Serial.printf("pre: wifi.sampleBuffer->sampleNumber %d\n", wifi.sampleBuffer->sampleNumber);

  wifi.sampleReset(wifi.sampleBuffer);

  test.assertEqual(wifi.sampleBuffer->timestamp, (uint64_t)0, String("should set timestamp to zero but got " + String((unsigned long)wifi.sampleBuffer->timestamp, DEC)).c_str());
  // Serial.printf("post: wifi.sampleBuffer->sampleNumber %d\n", wifi.sampleBuffer->sampleNumber);
  test.assertEqual(wifi.sampleBuffer->sampleNumber, 0, String("should set sampleNumber to zero but got " + String((uint8_t)wifi.sampleBuffer->sampleNumber, DEC)).c_str());
  boolean isAllClear = true;
  for (int i = 0; i < numChannels; i++) {
    if (wifi.sampleBuffer->channelData[i] > 0.001 || wifi.sampleBuffer->channelData[i] < -0.001) {
      isAllClear = false;
    }
  }
  test.assertBoolean(isAllClear, true, "should have cleared all values to 0.0", __LINE__);
}

void testUtilisForJSON() {
  testChannelDataComputeCyton();
  testChannelDataComputeDaisy();
  testChannelDataComputeGanglion();
  testExtractRaws();
  testInt24To32();
  testTransformRawsToScaledCyton();
  testTransformRawsToScaledGanglion();
  testSampleReset();
  testRawToScaled();
}

void testUtilisForRaw() {
    testRawBufferSetup();
    testRawBufferAddData();
    testRawBufferClean();
    testRawBufferHasData();
    testRawBufferProcessPacket();
    testRawBufferReadyForNewPage();
    testRawBufferReset();
    testRawBufferSwitchToOtherBuffer();
}

void testRawBufferSetup() {
    wifi.curRawBuffer = wifi.rawBuffer;
}

void testRawBufferAddData() {
    test.describe("bufferRadioAddData");

    char buffer[] = "AJ Keller is the best programmer";
    int expectedLength = 32; // Then length of the above buffer

    test.assertBoolean(wifi.rawBufferAddData(wifi.curRawBuffer, (char *)buffer, expectedLength, false),true,"should be able to add buffer to radioBuf", __LINE__);
    test.assertBoolean(wifi.curRawBuffer->gotAllPackets,false,"should not have all the packets", __LINE__);
    test.assertEqual(wifi.curRawBuffer->positionWrite,expectedLength,"should move positionWrite by 32", __LINE__);
    test.assertEqualBuffer(wifi.curRawBuffer->data,buffer,expectedLength,"should add the whole buffer", __LINE__);

    // Reset buffer
    wifi.curRawBuffer->positionWrite = 0;

    // Test how this will work in normal operations, i.e. ignoring the byte id
    test.assertBoolean(wifi.rawBufferAddData(wifi.curRawBuffer, buffer + 1, expectedLength - 1, true),true,"should be able to add buffer to radioBuf", __LINE__);
    test.assertBoolean(wifi.curRawBuffer->gotAllPackets,true,"should be able to set gotAllPackets to true", __LINE__);
    test.assertEqual(wifi.curRawBuffer->positionWrite,expectedLength - 1,"should set the positionWrite to 31", __LINE__);
    test.assertEqualBuffer(wifi.curRawBuffer->data,buffer + 1,expectedLength - 1,"should add the whole buffer", __LINE__);
}

void testRawBufferClean() {
    test.describe("bufferRadioClean");

    for (int i = 0; i < OPENBCI_BUFFER_LENGTH_MULTI; i++) {
        wifi.curRawBuffer->data[i] = 1;
    }

    // Call the function under test
    wifi.rawBufferClean(wifi.curRawBuffer);

    // Should fill the array with all zeros
    boolean allZeros = true;
    for (int j = 0; j < OPENBCI_BUFFER_LENGTH_MULTI; j++) {
        if (wifi.curRawBuffer->data[j] != 0) {
            allZeros = false;
        }
    }
    test.assertBoolean(true,allZeros,"should set all values to zero", __LINE__);
}

void testRawBufferHasData() {
    test.describe("bufferRadioHasData");

    wifi.curRawBuffer->positionWrite = 0;

    // Don't add any data
    test.assertBoolean(wifi.rawBufferHasData(wifi.curRawBuffer),false,"should have no data at first", __LINE__);
    // Add some data
    wifi.curRawBuffer->positionWrite = 69;
    // Verify!
    test.assertBoolean(wifi.rawBufferHasData(wifi.curRawBuffer),true,"should have data after moving positionWrite", __LINE__);
}

void testRawBufferProcessPacket() {

    test.describe("bufferRadioProcessPacket");

    testRawBuffer_OPENBCI_PROCESS_RADIO_PASS_LAST_SINGLE();

    testRawBuffer_OPENBCI_PROCESS_RADIO_PASS_LAST_MULTI();

    // Commented out test below because using only one radio buffer as of now
    //  and as such, will never be able to switch to the other radio buffer
    testRawBuffer_OPENBCI_PROCESS_RADIO_PASS_SWITCH_LAST();

    testRawBuffer_OPENBCI_PROCESS_RADIO_FAIL_SWITCH_LAST();

    testRawBuffer_OPENBCI_PROCESS_RADIO_FAIL_SWITCH_NOT_LAST();

    // Commented out test below because using only one radio buffer as of now
    //  and as such, will never be able to switch to the other radio buffer
    testRawBuffer_OPENBCI_PROCESS_RADIO_PASS_SWITCH_NOT_LAST();

    testRawBuffer_OPENBCI_PROCESS_RADIO_FAIL_MISSED_LAST();

    testRawBuffer_OPENBCI_PROCESS_RADIO_PASS_NOT_LAST_MIDDLE();

    testRawBuffer_OPENBCI_PROCESS_RADIO_FAIL_MISSED_NOT_LAST();

}

void testRawBuffer_OPENBCI_PROCESS_RADIO_PASS_LAST_SINGLE() {

    char bufferTaco[] = " taco";
    int bufferTacoLength = 5;

    testRawBufferCleanUp();
    test.detail("OPENBCI_PROCESS_RADIO_PASS_LAST_SINGLE");
    bufferTaco[0] = radio.byteIdMake(false,0,(char *)bufferTaco + 1, bufferTacoLength - 1);
    // Last packet
    //      Current buffer has no data
    //          Take it! Mark Last
    test.assertEqualHex(wifi.rawBufferProcessPacket((char *)bufferTaco, bufferTacoLength),OPENBCI_PROCESS_RADIO_PASS_LAST_SINGLE,"should add to radio buffer 1", __LINE__);
    test.assertBoolean(wifi.curRawBuffer->gotAllPackets,true,"should be able to set gotAllPackets to true", __LINE__);
    test.assertEqual(wifi.curRawBuffer->positionWrite,bufferTacoLength - 1,"should set the positionWrite to 4", __LINE__);
    test.assertEqualBuffer(wifi.curRawBuffer->data,bufferTaco + 1, bufferTacoLength - 1, "should have the taco buffer loaded into the first buffer", __LINE__);

    // Also verify that the buffer was loaded into the correct buffer
    test.assertBoolean(wifi.rawBuffer->gotAllPackets,true,"should be able to set gotAllPackets to true", __LINE__);
    test.assertEqual(wifi.rawBuffer->positionWrite,bufferTacoLength - 1,"should set the positionWrite to 4", __LINE__);
    test.assertEqualBuffer(wifi.rawBuffer->data,bufferTaco + 1, bufferTacoLength - 1, "currentRadioBuffer should have the taco buffer loaded into it", __LINE__);

}

void testRawBuffer_OPENBCI_PROCESS_RADIO_PASS_LAST_MULTI() {
    char buffer32[] = " AJ Keller is da best programmer";
    char bufferTaco[] = " taco";

    int buffer32Length = 32;
    int bufferTacoLength = 5;
    testRawBufferCleanUp();
    test.detail("OPENBCI_PROCESS_RADIO_PASS_LAST_MULTI");
    test.it("should be able to add a multi page packet");
    buffer32[0] = radio.byteIdMake(false,1,(char *)buffer32 + 1, buffer32Length - 1);
    bufferTaco[0] = radio.byteIdMake(false,0,(char *)bufferTaco + 1, bufferTacoLength - 1);
    test.assertEqualHex(wifi.rawBufferProcessPacket((char *)buffer32, buffer32Length),OPENBCI_PROCESS_RADIO_PASS_NOT_LAST_FIRST,"should add not the last packet", __LINE__);
    // Not last packet
    //      Current buffer has data
    //          Current buffer does not have all packets
    //              Previous packet number == packetNumber + 1
    //                  Take it! Mark last.
    test.assertEqualHex(wifi.rawBufferProcessPacket((char *)bufferTaco, bufferTacoLength),OPENBCI_PROCESS_RADIO_PASS_LAST_MULTI,"should add the last packet", __LINE__);
    test.assertBoolean(wifi.rawBuffer->gotAllPackets,true,"should set gotAllPackets to true on first buffer", __LINE__);
    test.assertEqual(wifi.rawBuffer->positionWrite,(bufferTacoLength + buffer32Length) - 2,"should set the positionWrite to size of both packets", __LINE__);
    test.assertEqualBuffer(wifi.rawBuffer->data,buffer32 + 1, buffer32Length - 1, "buffer32 loaded into the correct postion in first buffer", __LINE__);
    test.assertEqualBuffer(wifi.rawBuffer->data + (buffer32Length - 1),bufferTaco + 1, bufferTacoLength - 1, "taco buffer loaded into correct position in first buffer", __LINE__);
}

void testRawBuffer_OPENBCI_PROCESS_RADIO_PASS_SWITCH_LAST() {
    char buffer32[] = " AJ Keller is da best programmer";
    char bufferCali[] = " caliLucyMaggie";
    char bufferTaco[] = " taco";

    int buffer32Length = 32;
    int bufferCaliLength = 15;
    int bufferTacoLength = 5;
    testRawBufferCleanUp();
    test.detail("OPENBCI_PROCESS_RADIO_PASS_SWITCH_LAST");
    // Need the first buffer to be full
    test.it("should switch to second buffer when first buffer is full and id last packet");
    buffer32[0] = radio.byteIdMake(false,1,(char *)buffer32 + 1, buffer32Length - 1);
    bufferTaco[0] = radio.byteIdMake(false,0,(char *)bufferTaco + 1, bufferTacoLength - 1);
    test.assertEqualHex(wifi.rawBufferProcessPacket((char *)buffer32, buffer32Length),OPENBCI_PROCESS_RADIO_PASS_NOT_LAST_FIRST,"should add not the last packet", __LINE__);
    test.assertEqualHex(wifi.rawBufferProcessPacket((char *)bufferTaco, bufferTacoLength),OPENBCI_PROCESS_RADIO_PASS_LAST_MULTI,"should add the last packet", __LINE__);

    bufferCali[0] = radio.byteIdMake(false,0,(char *)bufferCali + 1, bufferCaliLength - 1);
    // Last packet
    //      Current buffer has data
    //          Current buffer has all packets
    //              Can swtich to other buffer
    //                  Take it! Mark Last
    test.assertEqualHex(wifi.rawBufferProcessPacket((char *)bufferCali, bufferCaliLength),OPENBCI_PROCESS_RADIO_PASS_SWITCH_LAST,"should switch and add the last packet", __LINE__);
    test.assertBoolean((wifi.rawBuffer + 1)->gotAllPackets,true,"should set gotAllPackets to true for second buffer", __LINE__);
    test.assertEqual((wifi.rawBuffer + 1)->positionWrite,bufferCaliLength - 1,"should set the positionWrite to size of cali buffer", __LINE__);
    test.assertEqualBuffer((wifi.rawBuffer + 1)->data,bufferCali + 1, bufferCaliLength - 1, "should have loaded cali buffer in the second buffer correctly", __LINE__);

    // Verify that both of the buffers are full
    test.assertBoolean(wifi.rawBuffer->gotAllPackets,true,"should still have a full first buffer after switch", __LINE__);
    test.assertEqual(wifi.rawBuffer->positionWrite,(bufferTacoLength + buffer32Length) - 2,"first buffer should still have correct size", __LINE__);

    test.assertBoolean(wifi.curRawBuffer->gotAllPackets,true,"should set got all packets full on currentRadioBuffer", __LINE__);
    test.assertEqual(wifi.curRawBuffer->positionWrite,bufferCaliLength - 1,"should set positionWrite of currentRadioBuffer to that of the second buffer", __LINE__);
    test.assertEqualBuffer(wifi.curRawBuffer->data,bufferCali + 1, bufferCaliLength - 1, "should have loaded cali buffer into the buffer currentRadioBuffer points to", __LINE__);


    // Do it again in reverse, where the second buffer is full
    // So clear the first buffer and point to the second
    test.it("should switch to first buffer when second buffer is full and id last packet");
    testRawBufferCleanUp();
    wifi.curRawBuffer = wifi.rawBuffer + 1;
    bufferCali[0] = radio.byteIdMake(false,0,(char *)bufferCali + 1, bufferCaliLength - 1);
    test.assertEqualHex(wifi.rawBufferProcessPacket((char *)bufferCali, bufferCaliLength),OPENBCI_PROCESS_RADIO_PASS_LAST_SINGLE,"should add the last packet", __LINE__);
    test.assertBoolean((wifi.rawBuffer + 1)->gotAllPackets,true,"should set gotAllPackets to true for second buffer", __LINE__);
    test.assertEqualBuffer((wifi.rawBuffer + 1)->data,bufferCali + 1, bufferCaliLength - 1, "should have loaded cali buffer in the second buffer correctly", __LINE__);
    test.assertEqualBuffer(wifi.curRawBuffer->data,bufferCali + 1, bufferCaliLength - 1, "should have loaded cali buffer into the buffer currentRadioBuffer points to", __LINE__);

    // point to the second buffer
    bufferTaco[0] = radio.byteIdMake(false,0,(char *)bufferTaco + 1, bufferTacoLength - 1);
    // Last packet
    //      Current buffer has data
    //          Current buffer has all packets
    //              Can swtich to other buffer
    //                  Take it! Mark Last
    test.assertEqualHex(wifi.rawBufferProcessPacket((char *)bufferTaco, bufferTacoLength),OPENBCI_PROCESS_RADIO_PASS_SWITCH_LAST,"should add the last packet", __LINE__);
    test.assertBoolean(wifi.rawBuffer->gotAllPackets,true,"should mark the first buffer full after switch", __LINE__);
    test.assertEqualBuffer(wifi.rawBuffer->data,bufferTaco + 1, bufferTacoLength - 1, "should have the taco buffer loaded into the first buffer", __LINE__);
    test.assertEqualBuffer(wifi.curRawBuffer->data,bufferTaco + 1, bufferTacoLength - 1, "should have the taco buffer loaded into currentRadioBuffer", __LINE__);
    // Verify the first buffer is still loaded with the cali buffer
    test.assertBoolean((wifi.rawBuffer + 1)->gotAllPackets,true,"should set gotAllPackets to true for second buffer", __LINE__);
    test.assertEqual((wifi.rawBuffer + 1)->positionWrite,bufferCaliLength - 1,"should set the positionWrite to size of cali buffer", __LINE__);
    test.assertEqualBuffer((wifi.rawBuffer + 1)->data,bufferCali + 1, bufferCaliLength - 1, "should have loaded cali buffer in the second buffer correctly", __LINE__);

    test.it("should switch to second buffer when first is flushing and id last packet");
    // First buffer flushing, second empty
    testRawBufferCleanUp();
    // Load the cali buffer into the first buffer
    bufferCali[0] = radio.byteIdMake(false,0,(char *)bufferCali + 1, bufferCaliLength - 1);
    test.assertEqualHex(wifi.rawBufferProcessPacket((char *)bufferCali, bufferCaliLength),OPENBCI_PROCESS_RADIO_PASS_LAST_SINGLE,"should add the last packet", __LINE__);
    test.assertBoolean(wifi.rawBuffer->gotAllPackets,true,"should set gotAllPackets to true for first buffer", __LINE__);
    test.assertEqualBuffer(wifi.rawBuffer->data,bufferCali + 1, bufferCaliLength - 1, "should have loaded cali buffer in the first buffer correctly", __LINE__);
    test.assertEqualBuffer(wifi.curRawBuffer->data,bufferCali + 1, bufferCaliLength - 1, "should have loaded cali buffer into the buffer currentRadioBuffer points to", __LINE__);
    // Test is to simulate the first one is being flushed as this new packet comes in
    // Set the first buffer to flushing
    wifi.rawBuffer->flushing = true;
    bufferTaco[0] = radio.byteIdMake(false,0,(char *)bufferTaco + 1, bufferTacoLength - 1);
    // Last packet
    //      Current buffer has data
    //          Current buffer has all packets
    //              Can swtich to other buffer
    //                  Take it! Mark Last
    test.assertEqualHex(wifi.rawBufferProcessPacket((char *)bufferTaco, bufferTacoLength),OPENBCI_PROCESS_RADIO_PASS_SWITCH_LAST,"should switch and add the last packet when first is flushing", __LINE__);
    test.assertBoolean((wifi.rawBuffer + 1)->gotAllPackets,true,"should mark the second buffer full", __LINE__);
    test.assertEqualBuffer((wifi.rawBuffer + 1)->data,bufferTaco + 1, bufferTacoLength - 1, "should have the taco buffer loaded into the second buffer", __LINE__);
    test.assertEqualBuffer(wifi.curRawBuffer->data,bufferTaco + 1, bufferTacoLength - 1, "should have the taco buffer loaded into currentRadioBuffer", __LINE__);
    // Verify the first buffer is still loaded with the cali buffer
    test.assertBoolean(wifi.rawBuffer->flushing,true,"should have flushing true for first buffer", __LINE__);
    test.assertBoolean(wifi.rawBuffer->gotAllPackets,true,"should still have gotAllPackets true for first buffer", __LINE__);
    test.assertEqual(wifi.rawBuffer->positionWrite,bufferCaliLength - 1,"should still have positionWrite to size of cali buffer in buffer 1", __LINE__);
    test.assertEqualBuffer(wifi.rawBuffer->data,bufferCali + 1, bufferCaliLength - 1, "should still have loaded cali buffer in the first buffer correctly", __LINE__);

    test.it("should switch to first buffer when second is flushing and id last packet");
    // Second buffer flushing, first empty
    testRawBufferCleanUp();
    // Load the cali buffer into the second buffer
    wifi.curRawBuffer = wifi.rawBuffer + 1;
    bufferCali[0] = radio.byteIdMake(false,0,(char *)bufferCali + 1, bufferCaliLength - 1);
    test.assertEqualHex(wifi.rawBufferProcessPacket((char *)bufferCali, bufferCaliLength),OPENBCI_PROCESS_RADIO_PASS_LAST_SINGLE,"should add the last packet", __LINE__);
    test.assertBoolean((wifi.rawBuffer + 1)->gotAllPackets,true,"should set gotAllPackets to true for second buffer", __LINE__);
    test.assertEqualBuffer((wifi.rawBuffer + 1)->data,bufferCali + 1, bufferCaliLength - 1, "should have loaded cali buffer in the second buffer correctly", __LINE__);
    test.assertEqualBuffer(wifi.curRawBuffer->data,bufferCali + 1, bufferCaliLength - 1, "should have loaded cali buffer into the buffer currentRadioBuffer points to", __LINE__);
    // Test is to simulate the second one is being flushed as this new packet comes in
    // Set the second buffer to flushing
    (wifi.rawBuffer + 1)->flushing = true;
    bufferTaco[0] = radio.byteIdMake(false,0,(char *)bufferTaco + 1, bufferTacoLength - 1);
    // Last packet
    //      Current buffer has data
    //          Current buffer has all packets
    //              Can swtich to other buffer
    //                  Take it! Mark Last
    test.assertEqualHex(wifi.rawBufferProcessPacket((char *)bufferTaco, bufferTacoLength),OPENBCI_PROCESS_RADIO_PASS_SWITCH_LAST,"should switch and add the last packet when second is flushing", __LINE__);
    test.assertBoolean(wifi.rawBuffer->gotAllPackets,true,"should mark the first buffer full", __LINE__);
    test.assertEqualBuffer(wifi.rawBuffer->data,bufferTaco + 1, bufferTacoLength - 1, "should have the taco buffer loaded into the first buffer", __LINE__);
    test.assertEqualBuffer(wifi.curRawBuffer->data,bufferTaco + 1, bufferTacoLength - 1, "should have the taco buffer loaded into currentRadioBuffer", __LINE__);
    // Verify the first buffer is still loaded with the cali buffer
    test.assertBoolean((wifi.rawBuffer + 1)->flushing,true,"should have flushing true for second buffer", __LINE__);
    test.assertBoolean((wifi.rawBuffer + 1)->gotAllPackets,true,"should still have gotAllPackets true for second buffer", __LINE__);
    test.assertEqual((wifi.rawBuffer + 1)->positionWrite,bufferCaliLength - 1,"should still have positionWrite to size of cali buffer in buffer 2", __LINE__);
    test.assertEqualBuffer((wifi.rawBuffer + 1)->data,bufferCali + 1, bufferCaliLength - 1, "should still have loaded cali buffer in the first buffer correctly", __LINE__);
}

void testRawBuffer_OPENBCI_PROCESS_RADIO_FAIL_SWITCH_LAST() {
    char bufferCali[] = " caliLucyMaggie";
    char bufferTaco[] = " taco";
    char bufferTomatoPotato[] = " tomatoPotato";

    int bufferCaliLength = 15;
    int bufferTacoLength = 5;
    int bufferTomatoPotatoLength = 13;
    test.detail("OPENBCI_PROCESS_RADIO_FAIL_SWITCH_LAST");
    test.it("should not be able to switch to other buffer when both are full");
    testRawBufferCleanUp();
    // Fill the two buffers
    bufferCali[0] = radio.byteIdMake(false,0,(char *)bufferCali + 1, bufferCaliLength - 1);
    bufferTaco[0] = radio.byteIdMake(false,0,(char *)bufferTaco + 1, bufferTacoLength - 1);
    test.assertEqualHex(wifi.rawBufferProcessPacket((char *)bufferCali, bufferCaliLength),OPENBCI_PROCESS_RADIO_PASS_LAST_SINGLE,"should add the last packet to buffer 1", __LINE__);
    test.assertEqualHex(wifi.rawBufferProcessPacket((char *)bufferTaco, bufferTacoLength),OPENBCI_PROCESS_RADIO_PASS_SWITCH_LAST,"should add the last packet to buffer 2", __LINE__);

    bufferTomatoPotato[0] = radio.byteIdMake(false,0,(char *)bufferTomatoPotato + 1, bufferTomatoPotatoLength - 1);
    // Last packet
    //      Current buffer has data
    //          Current buffer has all packets
    //              Cannot switch to other buffer
    //                  Reject it!
    test.assertEqualHex(wifi.rawBufferProcessPacket((char *)bufferTomatoPotato, bufferTomatoPotatoLength),OPENBCI_PROCESS_RADIO_FAIL_SWITCH_LAST,"should reject the addition of this buffer", __LINE__);

    test.it("should not be able to switch to other buffer when the buffers are flushing");
    testRawBufferCleanUp();
    wifi.rawBuffer->flushing = true;
    (wifi.rawBuffer+1)->flushing = true;
    bufferTomatoPotato[0] = radio.byteIdMake(false,0,(char *)bufferTomatoPotato + 1, bufferTomatoPotatoLength - 1);
    // Last packet
    //      Current buffer has data
    //          Current buffer has all packets
    //              Cannot switch to other buffer
    //                  Reject it!
    test.assertEqualHex(wifi.rawBufferProcessPacket((char *)bufferTomatoPotato, bufferTomatoPotatoLength),OPENBCI_PROCESS_RADIO_FAIL_SWITCH_LAST,"should reject the addition of this buffer", __LINE__);

}

void testRawBuffer_OPENBCI_PROCESS_RADIO_FAIL_SWITCH_NOT_LAST() {
    char buffer32Hey[] = " hey there, my name is AJ Keller";
    char bufferCali[] = " caliLucyMaggie";
    char bufferTaco[] = " taco";

    int buffer32Length = 32;
    int bufferCaliLength = 15;
    int bufferTacoLength = 5;
    int bufferTomatoPotatoLength = 13;
    testRawBufferCleanUp();
    test.detail("OPENBCI_PROCESS_RADIO_FAIL_SWITCH_NOT_LAST");
    // Fill both buffers
    bufferCali[0] = radio.byteIdMake(false,0,(char *)bufferCali + 1, bufferCaliLength - 1);
    bufferTaco[0] = radio.byteIdMake(false,0,(char *)bufferTaco + 1, bufferTacoLength - 1);
    test.assertEqualHex(wifi.rawBufferProcessPacket((char *)bufferCali, bufferCaliLength),OPENBCI_PROCESS_RADIO_PASS_LAST_SINGLE,"should add the last packet to buffer 1", __LINE__);
    test.assertEqualHex(wifi.rawBufferProcessPacket((char *)bufferTaco, bufferTacoLength),OPENBCI_PROCESS_RADIO_PASS_SWITCH_LAST,"should add the last packet to buffer 2", __LINE__);

    buffer32Hey[0] = radio.byteIdMake(false,1,(char *)buffer32Hey + 1, buffer32Length - 1);
    // Not last packet
    //      Current buffer has data
    //          Current buffer has all packets
    //              Cannot switch to other buffer
    //                  Reject it!
    test.assertEqualHex(wifi.rawBufferProcessPacket((char *)buffer32Hey, buffer32Length),OPENBCI_PROCESS_RADIO_FAIL_SWITCH_NOT_LAST,"should reject the addition of this multi page buffer", __LINE__);
    test.assertEqualBuffer(wifi.rawBuffer->data,bufferCali + 1, bufferCaliLength - 1, "should still have loaded cali buffer in the first buffer correctly", __LINE__);
    test.assertEqualBuffer((wifi.rawBuffer + 1)->data,bufferTaco + 1, bufferTacoLength - 1, "should still have loaded taco buffer in the second buffer correctly", __LINE__);

}

void testRawBuffer_OPENBCI_PROCESS_RADIO_PASS_SWITCH_NOT_LAST() {
    char bufferCali[] = " caliLucyMaggie";
    char buffer32Hey[] = " hey there, my name is AJ Keller";
    int bufferCaliLength = 15;
    int buffer32Length = 32;

    test.detail("OPENBCI_PROCESS_RADIO_PASS_SWITCH_NOT_LAST");
    testRawBufferCleanUp();
    // Clear the first buffer, second buffer still has stuff in it
    wifi.rawBufferReset(wifi.rawBuffer);
    // Make sure currentRadioBuffer pointer it on the second buffer
    wifi.curRawBuffer = wifi.rawBuffer + 1;
    bufferCali[0] = radio.byteIdMake(false,0,(char *)bufferCali + 1, bufferCaliLength - 1);
    test.assertEqualHex(wifi.rawBufferProcessPacket((char *)bufferCali, bufferCaliLength),OPENBCI_PROCESS_RADIO_PASS_LAST_SINGLE,"should add the last packet to buffer 1", __LINE__);

    // Load it
    buffer32Hey[0] = radio.byteIdMake(false,2,(char *)buffer32Hey + 1, buffer32Length - 1);
    // Not last packet
    //      Current buffer has data
    //          Current buffer has all packets
    //              Can switch to other buffer
    //                  Take it! Not last
    test.assertEqualHex(wifi.rawBufferProcessPacket((char *)buffer32Hey, buffer32Length),OPENBCI_PROCESS_RADIO_PASS_SWITCH_NOT_LAST,"should reject the addition of this multi page buffer", __LINE__);
}

void testRawBuffer_OPENBCI_PROCESS_RADIO_FAIL_MISSED_LAST() {
    char buffer32[] = " AJ Keller is da best programmer";
    char bufferTaco[] = " taco";

    int buffer32Length = 32;
    int bufferTacoLength = 5;
    // # CLEANUP
    testRawBufferCleanUp();

    test.detail("OPENBCI_PROCESS_RADIO_FAIL_MISSED_LAST");
    buffer32[0] = radio.byteIdMake(false,2,(char *)buffer32 + 1, buffer32Length - 1);
    bufferTaco[0] = radio.byteIdMake(false,0,(char *)bufferTaco + 1, bufferTacoLength - 1);
    test.assertEqualHex(wifi.rawBufferProcessPacket((char *)buffer32, buffer32Length),OPENBCI_PROCESS_RADIO_PASS_NOT_LAST_FIRST,"should add not the last packet", __LINE__);
    // Last packet
    //      Current buffer has data
    //          Current buffer does not have all packets
    //              Missed a packet
    //                  Reject it! Reset current buffer
    test.assertEqualHex(wifi.rawBufferProcessPacket((char *)bufferTaco, bufferTacoLength),OPENBCI_PROCESS_RADIO_FAIL_MISSED_LAST,"should not add the last packet because missed packet 1", __LINE__);
    test.assertBoolean(wifi.rawBuffer->gotAllPackets,false,"should not have gotAllPackets", __LINE__);
    test.assertEqual(wifi.rawBuffer->positionWrite,buffer32Length - 1,"should set the positionWrite to size of first packet", __LINE__);

}

void testRawBuffer_OPENBCI_PROCESS_RADIO_PASS_NOT_LAST_MIDDLE() {
    char buffer32[] = " AJ Keller is da best programmer";
    char buffer32Hey[] = " hey there, my name is AJ Keller";
    int buffer32Length = 32;

    // # CLEANUP
    testRawBufferCleanUp();
    test.detail("OPENBCI_PROCESS_RADIO_PASS_NOT_LAST_MIDDLE");
    buffer32[0] = radio.byteIdMake(false,2,(char *)buffer32 + 1, buffer32Length - 1);
    buffer32Hey[0] = radio.byteIdMake(false,1,(char *)buffer32Hey + 1, buffer32Length - 1);
    test.assertEqualHex(wifi.rawBufferProcessPacket((char *)buffer32, buffer32Length),OPENBCI_PROCESS_RADIO_PASS_NOT_LAST_FIRST,"should add first packet of several", __LINE__);
    // Not last packet
    //      Current buffer has data
    //          Current buffer does not have all packets
    //              Previous packet number == packetNumber + 1
    //                  Take it! Not last.
    test.assertEqualHex(wifi.rawBufferProcessPacket((char *)buffer32Hey, buffer32Length),OPENBCI_PROCESS_RADIO_PASS_NOT_LAST_MIDDLE,"should add middle packet", __LINE__);
    test.assertBoolean(wifi.rawBuffer->gotAllPackets,false,"should not have gotAllPackets", __LINE__);
    test.assertEqual(wifi.rawBuffer->positionWrite,buffer32Length - 1 + buffer32Length - 1,"should set the positionWrite to size of first packet", __LINE__);

}

void testRawBuffer_OPENBCI_PROCESS_RADIO_FAIL_MISSED_NOT_LAST() {
    char buffer32[] = " AJ Keller is da best programmer";
    char buffer32Hey[] = " hey there, my name is AJ Keller";
    int buffer32Length = 32;

    // # CLEANUP
    testRawBufferCleanUp();
    test.detail("OPENBCI_PROCESS_RADIO_FAIL_MISSED_NOT_LAST");
    buffer32[0] = radio.byteIdMake(false,3,(char *)buffer32 + 1, buffer32Length - 1);
    buffer32Hey[0] = radio.byteIdMake(false,1,(char *)buffer32Hey + 1, buffer32Length - 1);
    test.assertEqualHex(wifi.rawBufferProcessPacket((char *)buffer32, buffer32Length),OPENBCI_PROCESS_RADIO_PASS_NOT_LAST_FIRST,"should add first packet of several", __LINE__);
    // Not last packet
    //      Current buffer has data
    //          Current buffer does not have all packets
    //              Missed a packet
    //                  Reject it! Reset current buffer
    test.assertEqualHex(wifi.rawBufferProcessPacket((char *)buffer32Hey, buffer32Length),OPENBCI_PROCESS_RADIO_FAIL_MISSED_NOT_LAST,"should not be able to add middle packet because not last", __LINE__);
    test.assertBoolean(wifi.rawBuffer->gotAllPackets,false,"should not have gotAllPackets", __LINE__);
    test.assertEqual(wifi.rawBuffer->positionWrite,buffer32Length - 1,"should set the positionWrite to size of first packet", __LINE__);

}

void testRawBufferReadyForNewPage() {
    // # CLEANUP
    testRawBufferCleanUp();

    test.describe("bufferRadioReadyForNewPage");

    char bufferTomatoPotato[] = " tomatoPotato";
    int bufferTomatoPotatoLength = 13;
    bufferTomatoPotato[0] = radio.byteIdMake(false,0,(char *)bufferTomatoPotato + 1, bufferTomatoPotatoLength - 1);


    test.it("works with clean state");
    test.assertBoolean(wifi.rawBufferReadyForNewPage(wifi.rawBuffer),true,"should be ready to add new page in the first buffer", __LINE__);
    // test.assertBoolean(wifi.rawBufferReadyForNewPage(wifi.rawBuffer + 1),true,"should be ready to add new page in the second buffer", __LINE__);
    test.assertBoolean(wifi.rawBufferReadyForNewPage(wifi.curRawBuffer),true,"should be ready to add new page in the currentRadioBuffer", __LINE__);

    // Add data to buffer 1
    test.it("cannot add a page to first buffer but can the second when filled");
    wifi.rawBufferAddData(wifi.curRawBuffer, (char *)bufferTomatoPotato, bufferTomatoPotatoLength, true);
    test.assertBoolean(wifi.rawBufferReadyForNewPage(wifi.rawBuffer),false,"should not be ready to add new page in the first buffer", __LINE__);
    test.assertBoolean(wifi.rawBufferReadyForNewPage(wifi.rawBuffer + 1),true,"should be ready to add new page in the second buffer", __LINE__);
    test.assertBoolean(wifi.rawBufferReadyForNewPage(wifi.curRawBuffer),false,"should not be ready to add new page in the currentRadioBuffer", __LINE__);

    // Increment the currentRadioBuffer pointer
    // wifi.curRawBuffer++;
    // Clear the buffers
    // # CLEANUP
    testRawBufferCleanUp();

    // Add data to buffer 2
    test.it("cannot add a page to either the first buffer when filled");
    // test.it("cannot add a page to either the first or second buffer when both are filled");
    wifi.rawBufferAddData(wifi.curRawBuffer, (char *)bufferTomatoPotato, bufferTomatoPotatoLength, true);
    test.assertBoolean(wifi.rawBufferReadyForNewPage(wifi.rawBuffer),false,"should not be ready to add new page in the first buffer", __LINE__);
    // test.assertBoolean(wifi.rawBufferReadyForNewPage(wifi.rawBuffer + 1),false,"should not be ready to add new page in the second buffer", __LINE__);
    test.assertBoolean(wifi.rawBufferReadyForNewPage(wifi.curRawBuffer),false,"should not be ready to add new page in the currentRadioBuffer", __LINE__);

    // Clear the buffers
    // # CLEANUP
    testRawBufferCleanUp();

    // Mark first buffer as flushing
    test.it("cannot add a page to first buffer when when flushing");
    // test.it("cannot add a page to first buffer but can the second when flushing");
    wifi.rawBuffer->flushing = true;
    test.assertBoolean(wifi.rawBufferReadyForNewPage(wifi.rawBuffer),false,"should not be ready to add new page in the first buffer", __LINE__);
    // test.assertBoolean(wifi.rawBufferReadyForNewPage(wifi.rawBuffer + 1),true,"should be ready to add new page in the second buffer", __LINE__);
    test.assertBoolean(wifi.rawBufferReadyForNewPage(wifi.curRawBuffer),false,"should not be ready to add new page in the currentRadioBuffer", __LINE__);
    wifi.rawBuffer->flushing = false;
    // Mark second buffer as flushing
    test.it("cannot add a page to second buffer but can the first when flushing");
    (wifi.rawBuffer + 1)->flushing = true;
    test.assertBoolean(wifi.rawBufferReadyForNewPage(wifi.rawBuffer),true,"should be ready to add new page in the first buffer", __LINE__);
    test.assertBoolean(wifi.rawBufferReadyForNewPage(wifi.rawBuffer + 1),false,"should not be ready to add new page in the second buffer", __LINE__);
    test.assertBoolean(wifi.rawBufferReadyForNewPage(wifi.curRawBuffer),true,"should be ready to add new page in the currentRadioBuffer", __LINE__);
    (wifi.rawBuffer + 1)->flushing = true;

    // Both flushing
    test.it("cannot add a page to either when both flushing");
    wifi.rawBuffer->flushing = true;
    (wifi.rawBuffer + 1)->flushing = true;
    test.assertBoolean(wifi.rawBufferReadyForNewPage(wifi.rawBuffer),false,"should not be ready to add new page in the first buffer", __LINE__);
    test.assertBoolean(wifi.rawBufferReadyForNewPage(wifi.rawBuffer + 1),false,"should not be ready to add new page in the second buffer", __LINE__);
    test.assertBoolean(wifi.rawBufferReadyForNewPage(wifi.curRawBuffer),false,"should not be ready to add new page in the currentRadioBuffer", __LINE__);

    // # CLEANUP
    testRawBufferCleanUp();
}

void testRawBufferReset() {
    // Test the reset functions
    test.describe("bufferRadioReset");

    wifi.curRawBuffer->flushing = true;
    wifi.curRawBuffer->gotAllPackets = true;
    wifi.curRawBuffer->positionWrite = 60;
    wifi.curRawBuffer->previousPacketNumber = 3;

    // Reset the flags
    wifi.rawBufferReset(wifi.curRawBuffer);

    // Verify they got Reset
    test.assertBoolean(wifi.curRawBuffer->flushing,false,"should set flushing to false");
    test.assertBoolean(wifi.curRawBuffer->gotAllPackets,false,"should set got all packets to false");
    test.assertEqual(wifi.curRawBuffer->positionWrite,0,"should set positionWrite to 0");
    test.assertEqual(wifi.curRawBuffer->previousPacketNumber,0,"should set previousPacketNumber to 0");
}

void testRawBufferSwitchToOtherBuffer() {
    // # CLEANUP
    testRawBufferCleanUp();

    test.describe("bufferRadioSwitchToOtherBuffer");

    char bufferTomatoPotato[] = " tomatoPotato";
    int bufferTomatoPotatoLength = 13;

    test.it("should return true if buffer 2 does not have data and should move the pointer");
    wifi.curRawBuffer = wifi.rawBuffer;
    test.assertBoolean(wifi.rawBufferSwitchToOtherBuffer(),true,"can switch to other empty buffer", __LINE__);
    test.assertBoolean(wifi.curRawBuffer == (wifi.rawBuffer + 1), true, "currentRadioBuffer points to second buffer", __LINE__);

    test.it("should return true if buffer 1 does not have data and should move the pointer");
    wifi.curRawBuffer = wifi.rawBuffer + 1;
    test.assertBoolean(wifi.rawBufferSwitchToOtherBuffer(),true,"can switch to other empty buffer", __LINE__);
    test.assertBoolean(wifi.curRawBuffer == wifi.rawBuffer, true, "currentRadioBuffer points to first buffer", __LINE__);

    // # CLEANUP
    testRawBufferCleanUp();

    test.it("should return false when currently pointed at buf 1 and buf 2 has data");
    wifi.curRawBuffer = wifi.rawBuffer;
    wifi.rawBufferAddData(wifi.rawBuffer + 1, (char *)bufferTomatoPotato, bufferTomatoPotatoLength, true);
    test.assertBoolean(wifi.rawBufferSwitchToOtherBuffer(),false,"cannot switch to buffer with data", __LINE__);
    test.assertBoolean(wifi.curRawBuffer == wifi.rawBuffer, true, "currentRadioBuffer still points to first buffer", __LINE__);

    // # CLEANUP
    testRawBufferCleanUp();

    test.it("should return false when currently pointed at buf 2 and buf 1 has data");
    wifi.curRawBuffer = wifi.rawBuffer + 1;
    wifi.rawBufferAddData(wifi.rawBuffer, (char *)bufferTomatoPotato, bufferTomatoPotatoLength, true);
    test.assertBoolean(wifi.rawBufferSwitchToOtherBuffer(),false,"cannot switch to buffer with data", __LINE__);
    test.assertBoolean(wifi.curRawBuffer == wifi.rawBuffer + 1, true, "currentRadioBuffer still points to second buffer", __LINE__);

    // # CLEANUP
    testRawBufferCleanUp();

    test.it("should return false when both buffers have data");
    wifi.rawBufferAddData(wifi.rawBuffer, (char *)bufferTomatoPotato, bufferTomatoPotatoLength, true);
    wifi.rawBufferAddData(wifi.rawBuffer + 1, (char *)bufferTomatoPotato, bufferTomatoPotatoLength, true);
    test.assertBoolean(wifi.rawBufferSwitchToOtherBuffer(),false,"can't switch to second", __LINE__);
    wifi.curRawBuffer++;
    test.assertBoolean(wifi.rawBufferSwitchToOtherBuffer(),false,"can't switch back to first", __LINE__);

    // # CLEANUP
    testRawBufferCleanUp();

    test.it("should return false when other buffer is flushing");
    wifi.rawBufferAddData(wifi.rawBuffer, (char *)bufferTomatoPotato, bufferTomatoPotatoLength, true);
    (wifi.rawBuffer + 1)->flushing = true; // don't add data, just set it to flushing
    test.assertBoolean(wifi.rawBufferSwitchToOtherBuffer(),false,"can't switch to second because it's flushing", __LINE__);

    // # CLEANUP
    testRawBufferCleanUp();

    test.it("should not switch when buffers are flushing");
    wifi.rawBuffer->flushing = true; // don't add data, just set it to flushing
    (wifi.rawBuffer + 1)->flushing = true; // don't add data, just set it to flushing
    test.assertBoolean(wifi.rawBufferSwitchToOtherBuffer(),false,"can't switch to any buffer", __LINE__);
}

void testRawBufferCleanUp() {
    wifi.rawBufferReset(wifi.rawBuffer);
    // wifi.rawBufferReset(wifi.rawBuffer + 1);
    wifi.curRawBuffer = wifi.rawBuffer;
}

void testUtils() {
  // testUtilisForJSON();
  testUtilisForRaw();
}

void go() {
  // Start the test
  test.begin();
  digitalWrite(ledPin, HIGH);
  testGetters();
  testSetters();
  testUtils();
  test.end();
  digitalWrite(ledPin, LOW);
}

void setup() {
  pinMode(ledPin,OUTPUT);
  Serial.begin(115200);
  test.setSerial(Serial);
  test.failVerbosity = true;
}

void loop() {
  // put your main code here, to run repeatedly:
  if (Serial.available()) {
    Serial.read();
    go();
  }
}

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

void testGetOutputMode() {
  test.describe("getOutputMode");

  // RAW
  test.assertEqual(wifi.getOutputMode(wifi.OUTPUT_MODE_RAW),"raw","Should have gotten 'raw' string");

  // JSON
  test.assertEqual(wifi.getOutputMode(wifi.OUTPUT_MODE_JSON),"json","Should have gotten 'json' string");
}

void testGetScaleFactorVoltsCyton() {
  test.describe("getScaleFactorVoltsCyton");

  uint8_t expectedGain = 1;
  double epsilon = 0.000000001;
  double expectedScaleFactor = 4.5 / expectedGain / (pow(2,23) - 1);
  test.assertApproximately(wifi.getScaleFactorVoltsCyton(expectedGain), expectedScaleFactor, epsilon, "should be within 0.000001 with gain 1");

  expectedGain = 2;
  expectedScaleFactor = 4.5 / expectedGain / (pow(2,23) - 1);
  test.assertApproximately(wifi.getScaleFactorVoltsCyton(expectedGain), expectedScaleFactor, epsilon, "should be within 0.000001 with gain 2");

  expectedGain = 4;
  expectedScaleFactor = 4.5 / expectedGain / (pow(2,23) - 1);
  test.assertApproximately(wifi.getScaleFactorVoltsCyton(expectedGain), expectedScaleFactor, epsilon, "should be within 0.000001 with gain 4");

  expectedGain = 6;
  expectedScaleFactor = 4.5 / expectedGain / (pow(2,23) - 1);
  test.assertApproximately(wifi.getScaleFactorVoltsCyton(expectedGain), expectedScaleFactor, epsilon, "should be within 0.000001 with gain 6");

  expectedGain = 8;
  expectedScaleFactor = 4.5 / expectedGain / (pow(2,23) - 1);
  test.assertApproximately(wifi.getScaleFactorVoltsCyton(expectedGain), expectedScaleFactor, epsilon, "should be within 0.000001 with gain 8");

  expectedGain = 12;
  expectedScaleFactor = 4.5 / expectedGain / (pow(2,23) - 1);
  test.assertApproximately(wifi.getScaleFactorVoltsCyton(expectedGain), expectedScaleFactor, epsilon, "should be within 0.000001 with gain 12");

  expectedGain = 24;
  expectedScaleFactor = 4.5 / expectedGain / (pow(2,23) - 1);
  test.assertApproximately(wifi.getScaleFactorVoltsCyton(expectedGain), expectedScaleFactor, epsilon, "should be within 0.000001 with gain 24");
}

void testGetScaleFactorVoltsGanglion() {
  test.describe("getScaleFactorVoltsGanglion");

  double expectedScaleFactor = 1.2 / 51.0 / 1.5 / (pow(2,23) - 1);
  double epsilon = 0.0000000001;
  test.assertApproximately(wifi.getScaleFactorVoltsGanglion(), expectedScaleFactor, epsilon, "should be within 0.000001");

}

void testGetters() {
  testGetOutputMode();
  testGetScaleFactorVoltsCyton();
  testGetScaleFactorVoltsGanglion();
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

  // Now for cyton

  wifi.sampleReset(wifi.sampleBuffer);
  wifi.channelDataCompute(arr, gains, wifi.sampleBuffer, packetOffset, numChannels);
  for (uint8_t i = 0; i < numChannels; i++) {
    test.assertApproximately(wifi.sampleBuffer->channelData[i], expected_channelData[i], 1.0);
  }
  test.assertEqualHex(wifi.sampleBuffer->sampleNumber, expected_sampleNumber, "should be able to extract the sample number");
  test.assertGreaterThan((uint32_t)wifi.sampleBuffer->timestamp, 0, "should be able to set the timestamp");
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
  test.assertEqualHex(wifi.sampleBuffer->sampleNumber, expected_sampleNumber, "should be able to extract the sample number");
  test.assertGreaterThan(wifi.sampleBuffer->timestamp, (uint64_t)0, "should be able to set the timestamp");

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
  test.assertEqualHex(wifi.sampleBuffer->sampleNumber, expected_sampleNumber, "should still have valid sample number");
  test.assertGreaterThan(wifi.sampleBuffer->timestamp, (uint64_t)0, "should be able to set the timestamp");
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
  test.assertEqualHex(wifi.sampleBuffer->sampleNumber, expected_sampleNumber, "should be able to extract the sample number");
  test.assertGreaterThan(wifi.sampleBuffer->timestamp, (uint64_t)0, "should be able to set the timestamp");
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
    test.assertEqual(actual_raws[i], (int32_t)i+1, "should match the index number");
  }

}

void testInt24To32() {
  test.describe("int24To32");

  uint8_t arr[3];
  arr[0] = 0x00; arr[1] = 0x06; arr[2] = 0x90;
  test.assertEqual(wifi.int24To32(arr), 1680, "Should convert a small positive number");

  arr[0] = 0x02; arr[1] = 0xC0; arr[2] = 0x01; // 0x02C001 === 180225
  test.assertEqual(wifi.int24To32(arr), 180225, "converts a large positive number");

  arr[0] = 0xFF; arr[1] = 0xFF; arr[2] = 0xFF; // 0xFFFFFF === -1
  test.assertEqual(wifi.int24To32(arr), -1, "converts a small negative number");

  arr[0] = 0x81; arr[1] = 0xA1; arr[2] = 0x01; // 0x81A101 === -8281855
  test.assertEqual(wifi.int24To32(arr), -8281855, "converts a large negative number");
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
    test.assertApproximately(actual_scaledOutput[i], expected_scaledOutput[i], 1.0, "should be able to transform raws to scaled for cyton");
    actual_scaledOutput[i] = 0.0; // clear
  }

  // Then daisy
  wifi.transformRawsToScaledCyton(raw, gains, MAX_CHANNELS_PER_PACKET, actual_scaledOutput);

  for (uint8_t i = NUM_CHANNELS_CYTON; i < NUM_CHANNELS_CYTON_DAISY; i++) {
    test.assertApproximately(actual_scaledOutput[i], expected_scaledOutput[i], 1.0, "should be able to transform raws to scaled for cyton daisy");
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
    test.assertApproximately(actual_scaledOutput[i], expected_scaledOutput[i], 1.0, "should be able to transform raws to scaled for cyton");
  }

}

void testRawToScaled() {
    test.describe("rawToScaled");

    int32_t raw = 1;
    double epsilon = 1.0;
    double scaleFactor = 5.0;
    double expected_output = 5000000000.0;
    double actual_output = wifi.rawToScaled(raw, scaleFactor);
    test.assertApproximately(actual_output, expected_output, epsilon, "should be able to convert to scaled small positive double");

    raw = 123456789;
    scaleFactor = 0.0000001234;
    epsilon = 50.0;
    expected_output = 15234567762.6; // on mac
    actual_output = wifi.rawToScaled(raw, scaleFactor);
    test.assertApproximately(actual_output, expected_output, epsilon, "should be able to convert to scaled large positive double");

    raw = -1;
    scaleFactor = 5.0;
    expected_output = -5000000000.0;
    epsilon = 1.0;
    actual_output = wifi.rawToScaled(raw, scaleFactor);
    test.assertApproximately(actual_output, expected_output, epsilon, "should be able to convert to scaled small negative double");

    raw = -123456789;
    scaleFactor = 0.0000001234;
    expected_output = -15234567762.6; // on mac
    epsilon = 50.0;
    actual_output = wifi.rawToScaled(raw, scaleFactor);
    test.assertApproximately(actual_output, expected_output, epsilon, "should be able to convert to scaled large positive double");
}

void testSampleReset() {
  test.describe("sampleReset");

  uint8_t numChannels = NUM_CHANNELS_CYTON_DAISY;
  wifi.sampleBuffer->timestamp = 20;
  wifi.sampleBuffer->sampleNumber = 21;
  for (int i = 0; i < numChannels; i++) {
    wifi.sampleBuffer->channelData[i] = 1099.0;
  }
  wifi.sampleReset(wifi.sampleBuffer);

  test.assertEqual(wifi.sampleBuffer->timestamp, (uint64_t)0, String("should set timestamp to zero but got " + String((unsigned long)wifi.sampleBuffer->timestamp, DEC)).c_str());
  test.assertEqualHex(wifi.sampleBuffer->sampleNumber, 0, "should clear the sample number");
  boolean isAllClear = true;
  for (int i = 0; i < numChannels; i++) {
    if (wifi.sampleBuffer->channelData[i] > 0.001 || wifi.sampleBuffer->channelData[i] < -0.001) {
      isAllClear = false;
    }
  }
  test.assertBoolean(isAllClear, true, "should have cleared all values to 0.0");
}

void testUtils() {
  testChannelDataComputeCyton();
  testExtractRaws();
  testInt24To32();
  testTransformRawsToScaledCyton();
  testTransformRawsToScaledGanglion();
  testSampleReset();
  testRawToScaled();
}

void go() {
  // Start the test
  test.begin();
  digitalWrite(ledPin, HIGH);
  testGetters();
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

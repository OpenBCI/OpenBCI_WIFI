#define ARDUINOJSON_USE_LONG_LONG 1
#define ARDUINOJSON_USE_DOUBLE 1
#include "OpenBCI_Wifi.h"
#include "PTW-Arduino-Assert.h"

int ledPin = 0;

uint8_t *giveMeASPIStreamPacket(uint8_t *data, uint8_t sampleNumber) {
  data[0] = STREAM_PACKET_BYTE_STOP;
  data[1] = sampleNumber;
  uint8_t index = 1;
  for (int i = 2; i < BYTES_PER_SPI_PACKET; i++) {
    data[i] = 0;
    if (i%3==2) {
      data[i] = index;
      index++;
    }
  }
  return data;
}

uint8_t *giveMeASPIStreamPacket(uint8_t *data) {
  return giveMeASPIStreamPacket(data, 0);
}

uint8_t *giveMeASPIPacketGainSet(uint8_t *data, uint8_t numChannels) {
  uint8_t byteCounter = 0;
  data[byteCounter++] = WIFI_SPI_MSG_GAINS;
  data[byteCounter++] = WIFI_SPI_MSG_GAINS;
  data[byteCounter++] = numChannels;

  for (int i = byteCounter; i < BYTES_PER_SPI_PACKET; i++) {
    if (i < numChannels + byteCounter) {
      if (numChannels == NUM_CHANNELS_GANGLION) {
        data[i] = GANGLION_GAIN;
      } else {
        data[i] = wifi.CYTON_GAIN_24;
      }
    } else {
      data[i] = 0;
    }
  }
}

void testGetBoardTypeString() {
  test.describe("getBoardTypeString");

  test.it("needs to get the right string name for the board given the number of channels inputed to the function.");

  wifi.reset();

  test.assertEqual(wifi.getBoardTypeString(NUM_CHANNELS_GANGLION), BOARD_TYPE_GANGLION, "should be able to get 'ganglion' for 4 channels", __LINE__);
  test.assertEqual(wifi.getBoardTypeString(NUM_CHANNELS_CYTON), BOARD_TYPE_CYTON, "should be able to get 'cyton' for 8 channels", __LINE__);
  test.assertEqual(wifi.getBoardTypeString(NUM_CHANNELS_CYTON_DAISY), BOARD_TYPE_CYTON_DAISY, "should be able to get 'daisy' for 16 channels", __LINE__);
  test.assertEqual(wifi.getBoardTypeString(NUM_CHANNELS_CYTON_DAISY * 2), BOARD_TYPE_NONE, "should be able to get 'none' for 32 channels", __LINE__);
  test.assertEqual(wifi.getBoardTypeString(0), BOARD_TYPE_NONE, "should be able to get 'none' for 0 channels", __LINE__);
}

void testGetCurBoardTypeString() {
  test.describe("getCurBoardTypeString");

  wifi.reset();
  test.it("needs to get the right string name for the board given the number of channels in the system.");
  test.assertEqual(wifi.getCurBoardTypeString(), BOARD_TYPE_NONE, "should get 'none' for 0 channels", __LINE__);

  test.it("should get 'daisy' after setting the channels to 16");
  wifi.setNumChannels(NUM_CHANNELS_CYTON_DAISY);
  test.assertEqual(wifi.getCurBoardTypeString(), BOARD_TYPE_CYTON_DAISY, "should get 'daisy' for 16 channels", __LINE__);

  test.it("should get 'ganglion' after setting the channels to 4");
  wifi.setNumChannels(NUM_CHANNELS_GANGLION);
  test.assertEqual(wifi.getCurBoardTypeString(), BOARD_TYPE_GANGLION, "should get 'ganglion' for 4 channels", __LINE__);

  test.it("should get 'cyton' after setting the channels to 8");
  wifi.setNumChannels(NUM_CHANNELS_CYTON);
  test.assertEqual(wifi.getCurBoardTypeString(), BOARD_TYPE_CYTON, "should get 'cyton' for 8 channels", __LINE__);
}

void testGetCurOutputModeString() {
  test.describe("getCurOutputModeString");

  wifi.reset();

  test.it("should get raw output mode at startup");
  test.assertEqual(wifi.getCurOutputModeString(), OUTPUT_RAW, "should have output mode of 'raw'", __LINE__);

  test.it("should be able to switch to 'json' mode");
  wifi.setOutputMode(wifi.OUTPUT_MODE_JSON);
  test.assertEqual(wifi.getCurOutputModeString(), OUTPUT_JSON, "should have output mode of 'json'", __LINE__);

  test.it("should be able to switch back to 'raw' after 'json'");
  wifi.setOutputMode(wifi.OUTPUT_MODE_RAW);
  test.assertEqual(wifi.getCurOutputModeString(), OUTPUT_RAW, "should have output mode of 'raw'", __LINE__);
}

void testGetCurOutputProtocolString() {
  test.describe("getCurOutputProtocolString");

  wifi.reset();

  test.it("should get none output protocol at startup");
  test.assertEqual(wifi.getCurOutputProtocolString(), OUTPUT_NONE, "should have output protocol of 'none'", __LINE__);

  test.it("should be able to switch to 'tcp' protocol");
  wifi.setOutputProtocol(wifi.OUTPUT_PROTOCOL_TCP);
  test.assertEqual(wifi.getCurOutputProtocolString(), OUTPUT_TCP, "should have output protocol of 'tcp'", __LINE__);

  test.it("should be able to switch to 'mqtt' protocol");
  wifi.setOutputProtocol(wifi.OUTPUT_PROTOCOL_MQTT);
  test.assertEqual(wifi.getCurOutputProtocolString(), OUTPUT_MQTT, "should have output protocol of 'mqtt'", __LINE__);

  test.it("should be able to switch to 'serial' protocol");
  wifi.setOutputProtocol(wifi.OUTPUT_PROTOCOL_WEB_SOCKETS);
  test.assertEqual(wifi.getCurOutputProtocolString(), OUTPUT_WEB_SOCKETS, "should have output protocol of 'ws'", __LINE__);

  test.it("should be able to switch to 'serial' protocol");
  wifi.setOutputProtocol(wifi.OUTPUT_PROTOCOL_SERIAL);
  test.assertEqual(wifi.getCurOutputProtocolString(), OUTPUT_SERIAL, "should have output protocol of 'serial'", __LINE__);

  test.it("should be able to switch to 'none' protocol");
  wifi.setOutputProtocol(wifi.OUTPUT_PROTOCOL_NONE);
  test.assertEqual(wifi.getCurOutputProtocolString(), OUTPUT_NONE, "should have output protocol of 'none'", __LINE__);

}

void testGetGainCyton() {
  test.detail("Cyton");

  test.assertEqual(wifi.getGainCyton(wifi.CYTON_GAIN_1), 1, "should be able to get cyton gain of 1");
  test.assertEqual(wifi.getGainCyton(wifi.CYTON_GAIN_2), 2, "should be able to get cyton gain of 2");
  test.assertEqual(wifi.getGainCyton(wifi.CYTON_GAIN_4), 4, "should be able to get cyton gain of 4");
  test.assertEqual(wifi.getGainCyton(wifi.CYTON_GAIN_6), 6, "should be able to get cyton gain of 6");
  test.assertEqual(wifi.getGainCyton(wifi.CYTON_GAIN_8), 8, "should be able to get cyton gain of 8");
  test.assertEqual(wifi.getGainCyton(wifi.CYTON_GAIN_12), 12, "should be able to get cyton gain of 12");
  test.assertEqual(wifi.getGainCyton(wifi.CYTON_GAIN_24), 24, "should be able to get cyton gain of 24");

}

void testGetGainGanglion() {
  test.detail("Ganglion");

  test.assertEqual(wifi.getGainGanglion(), 51, "should get the gain of 51 for ganglion");
}

void testGetGain() {
  test.describe("getGain");
  testGetGainCyton();
  testGetGainGanglion();
}

void testGetInfoAll() {
  test.detail("GET /all");
  wifi.reset();

  // Serial.print("spi has master: "); Serial.println(wifi.spiHasMaster() ?)

  String actual_infoAll = wifi.getInfoAll();

  // Serial.println(actual_infoAll);
  const size_t bufferSize = JSON_OBJECT_SIZE(8) + 150 + 200;
  DynamicJsonBuffer jsonBuffer(bufferSize);

  JsonObject& root = jsonBuffer.parseObject(actual_infoAll);
  if (!root.success()) {
    Serial.println("unable to parse /all root");
  }
  boolean board_connected = root.get<boolean>("board_connected"); // false
  test.assertBoolean(board_connected, false, "should not have board connected");
  int heap = root["heap"]; // 1234
  test.assertGreaterThan(heap, 0, "should get heap greater than 0", __LINE__);
  String ip = root["ip"]; // "255.255.255.255"
  test.assertEqual(ip, WiFi.localIP().toString(), "should be local ip", __LINE__);
  String mac = root["mac"]; //
  test.assertEqual(mac, wifi.getMac(), "should get the full mac address", __LINE__);
  String name = root["name"]; //
  test.assertEqual(name, wifi.getName(), "should get the full name", __LINE__);
  String version = root["version"]; // v1.0.0
  test.assertEqual(version, wifi.getVersion(), "should get the software version", __LINE__);
  uint8_t numChannels = root["num_channels"];
  test.assertEqual(numChannels, 0, "should get 0 channels", __LINE__);

  uint8_t expected_numChannels = NUM_CHANNELS_CYTON;
  uint8_t gainPacket[BYTES_PER_SPI_PACKET];
  giveMeASPIPacketGainSet(gainPacket, expected_numChannels);
  wifi.setGains(gainPacket);
  wifi.lastTimeWasPolled = millis();

  actual_infoAll = wifi.getInfoAll();
  // Serial.println(actual_infoAll);

  DynamicJsonBuffer jsonBuffer1(bufferSize);

  JsonObject& root1 = jsonBuffer1.parseObject(actual_infoAll);
  if (!root1.success()) {
    Serial.println("unable to parse /all root1");
  }
  board_connected = root1.get<boolean>("board_connected"); // true
  test.assertTrue(board_connected, "should think a board is attached");
  heap = root1.get<int>("heap"); // 1234
  test.assertGreaterThan(heap, 0, "should get heap more than 0");
  ip = root1.get<String>("ip"); // "255.255.255.255"
  test.assertEqual(ip, WiFi.localIP().toString(), "should be local ip", __LINE__);
  mac = root1.get<String>("mac"); //
  test.assertEqual(mac, wifi.getMac(), "should get the full mac address", __LINE__);
  name = root1.get<String>("name"); //
  test.assertEqual(name, wifi.getName(), "should get the full name", __LINE__);
  version = root.get<String>("version"); // v1.0.0
  test.assertEqual(version, wifi.getVersion(), "should get the software version", __LINE__);
  numChannels = root1.get<uint8_t>("num_channels");
  test.assertEqual(numChannels, expected_numChannels, "should get 8 channels", __LINE__);
}

void testGetInfoBoard() {
  test.detail("GET /board");
  wifi.reset();

  String actual_infoBoard = wifi.getInfoBoard();
  Serial.println(actual_infoBoard);

  const size_t bufferSize = JSON_OBJECT_SIZE(4) + 350 + JSON_ARRAY_SIZE(16);
  DynamicJsonBuffer jsonBuffer(bufferSize);

  JsonObject& root = jsonBuffer.parseObject(actual_infoBoard);
  if (!root.success()) {
    Serial.println("unable to parse /board root");
  }
  boolean board_connected = root.get<boolean>("board_connected"); // false
  test.assertBoolean(board_connected, false, "should not have board connected");
  String board_type = root["board_type"]; // "255.255.255.255"
  test.assertEqual(board_type, "none", "should get none for board type", __LINE__);
  uint8_t numChannels = root["num_channels"];
  test.assertEqual(numChannels, 0, "should get 0 channels", __LINE__);
  JsonArray& gains = root["gains"];
  test.assertEqual(gains.size(), (size_t)0, "should have zero elemenets inside the array");

  uint8_t expected_numChannels = NUM_CHANNELS_CYTON_DAISY;
  uint8_t gainPacket[BYTES_PER_SPI_PACKET];
  giveMeASPIPacketGainSet(gainPacket, expected_numChannels);
  wifi.setGains(gainPacket);
  wifi.lastTimeWasPolled = millis();

  actual_infoBoard = wifi.getInfoBoard();
  Serial.println(actual_infoBoard);
  const size_t bufferSize1 = JSON_ARRAY_SIZE(16) + JSON_OBJECT_SIZE(4) + 120;
  DynamicJsonBuffer jsonBuffer1(bufferSize1);

  JsonObject& root1 = jsonBuffer1.parseObject(actual_infoBoard);
  if (!root1.success()) {
    Serial.println("unable to parse /board root1");
  }
  board_connected = root1.get<boolean>("board_connected"); // true
  test.assertBoolean(board_connected, true, "should think a board is attached");
  board_type = root1.get<String>("board_type"); // "255.255.255.255"
  test.assertEqual(board_type, "daisy", "should be daisy", __LINE__);
  numChannels = root1.get<uint8_t>("num_channels");
  test.assertEqual(numChannels, expected_numChannels, "should get 16 channels", __LINE__);
  JsonArray& gains1 = root1["gains"];
  // boolean all24 = true;
  for (int i = 0; i < numChannels; i++) {
    int gain1 = gains1[i];
    test.assertEqual(gain1, (int)24, String("should be able get gain for channel " + String(i+1)).c_str(), __LINE__);
  }
}

void testGetInfoTCP() {
  test.detail("TCP");
  wifi.reset();
  boolean expected_connected = false;
  // String actual_infoTCP = "";
  String actual_infoTCP = wifi.getInfoTCP(expected_connected);

  const size_t bufferSize = JSON_OBJECT_SIZE(6) + 40*6;
  DynamicJsonBuffer jsonBuffer(bufferSize);

  JsonObject& root = jsonBuffer.parseObject(actual_infoTCP);
  IPAddress tempIPAddr;

  boolean connected = root["connected"]; // false
  test.assertBoolean(connected, expected_connected, "should not be connected");
  boolean delimiter = root["delimiter"]; // false
  test.assertBoolean(delimiter, false, "should not be using delimiter");
  String ip = root["ip"]; // "255.255.255.255"
  test.assertEqual(ip, tempIPAddr.toString(), "should be an empty ip address", __LINE__);
  String output = root["output"]; // "raw"
  test.assertEqual(output, "raw", "should be default to raw", __LINE__);
  int port = root["port"]; // 12345
  test.assertEqual(port, 80, "should be at port 80", __LINE__);
  unsigned long latency = root["latency"];
  test.assertEqual(latency, (unsigned long)DEFAULT_LATENCY, "should get the default latency", __LINE__);

  boolean expected_delimiter = true;
  String expected_ip = "192.168.0.1";
  int expected_port = 12345;
  expected_connected = true;
  wifi.setInfoTCP(expected_ip, expected_port, expected_delimiter);
  wifi.setOutputMode(wifi.OUTPUT_MODE_JSON);

  actual_infoTCP = wifi.getInfoTCP(expected_connected);
  DynamicJsonBuffer jsonBuffer1(bufferSize);
  JsonObject& root1 = jsonBuffer1.parseObject(actual_infoTCP);

  connected = root1.get<boolean>("connected"); // false
  test.assertBoolean(connected, expected_connected, "should not be connected");
  delimiter = root1.get<boolean>("delimiter"); // true
  test.assertBoolean(delimiter, true, "should be using delimiter");
  ip = root1.get<String>("ip"); // "255.255.255.255"
  test.assertEqual(ip, expected_ip, "should be valid ip address", __LINE__);
  output = root1.get<String>("output"); // "json"
  test.assertEqual(output, "json", "should be set to json output mode", __LINE__);
  port = root1["port"]; // 12345
  test.assertEqual(port, expected_port, "should be at port 12345", __LINE__);
  latency = root.get<unsigned long>("latency");
  test.assertEqual(latency, (unsigned long)DEFAULT_LATENCY, "should get the default latency", __LINE__);
}

void testGetInfo() {
  test.describe("getInfo");
  testGetInfoAll();
  testGetInfoBoard();
  testGetInfoTCP();
}

void testGetLatency() {
  test.describe("getLatency");

  wifi.reset();
  unsigned long expected_latency = DEFAULT_LATENCY;
  test.assertEqual(wifi.getLatency(), expected_latency, "should get default latency", __LINE__);

  expected_latency = 5000;
  wifi.setLatency(expected_latency);

  test.assertEqual(wifi.getLatency(), expected_latency, "should get new latency", __LINE__);
}

void testGetVersion() {
  test.describe("getVersion");
  test.assertEqual(wifi.getVersion(), SOFTWARE_VERSION, "should get the correct version number", __LINE__);
}

void testGetters() {
  testGetBoardTypeString();
  testGetCurBoardTypeString();
  testGetCurOutputModeString();
  testGetCurOutputProtocolString();
  testGetGain();
  testGetInfo();
  testGetLatency();
}

void testSetInfoTCP() {
  test.detail("TCP");
  String addr = "129.0.0.23";
  IPAddress expected_address;
  expected_address.fromString(addr);
  boolean expected_delimiter = true;
  int expected_port = 99;

  wifi.setInfoTCP(addr, expected_port, expected_delimiter);
  test.assertEqual((int)wifi.curOutputProtocol, (int)wifi.OUTPUT_PROTOCOL_TCP, "should have set the output protocol to TCP", __LINE__);
  test.assertEqual(wifi.tcpAddress.toString(), expected_address.toString(), "should have set tcpAddress", __LINE__);
  test.assertTrue(wifi.tcpDelimiter, "should have set tcpDelimiter", __LINE__);
  test.assertEqual(wifi.tcpPort, expected_port, "should be set tcpPort", __LINE__);
}

void testSetInfo() {
  test.describe("setInfo");
  testSetInfoTCP();
}

void testSetLatency() {
  test.describe("setLatency");

  wifi.reset();

  unsigned long expected_latency = 5000;
  wifi.setLatency(expected_latency);

  test.assertEqual(wifi.getLatency(), expected_latency, "should set the new latency", __LINE__);
}


void testSetters() {
  testSetInfo();
  testSetLatency();
}

void testIsAStreamByte() {
  test.describe("isAStreamByte");

  test.assertTrue(wifi.isAStreamByte(0xC0), "should find a stream byte", __LINE__);
  test.assertTrue(wifi.isAStreamByte(0xC5), "should find a stream byte", __LINE__);
  test.assertFalse(wifi.isAStreamByte(0x05), "should not find a stream byte", __LINE__);
}


void testUtilisForJSON() {
  testIsAStreamByte();
}

void testPassthroughCommands() {
  test.detail("Commands");

  String cmds = "pushtheworld";
  uint8_t expected_length = cmds.length();
  wifi.reset();
  test.assertEqualHex(wifi.passthroughCommands(""), PASSTHROUGH_FAIL_NO_CHARS, "should not pass through the commands", __LINE__);
  test.assertEqual(wifi.passthroughPosition, 0, "should not have put the cmds into the pass through buffer offset by one for storage of num cmds byte", __LINE__);

  wifi.reset();
  test.assertEqualHex(wifi.passthroughCommands("aj keller is the best programmer in the world woo!!!!"), PASSTHROUGH_FAIL_TOO_MANY_CHARS, "should not pass through the commands", __LINE__);
  test.assertEqual(wifi.passthroughPosition, 0, "should not have put the cmds into the pass through buffer offset by one for storage of num cmds byte", __LINE__);

  wifi.reset();
  test.assertEqualHex(wifi.passthroughCommands(cmds), PASSTHROUGH_PASS, "should be able to pass through the commands", __LINE__);
  test.assertApproximately((float)millis(), (float)wifi.timePassthroughBufferLoaded, (float)10.0, "should have just set the timePassthroughBufferLoaded", __LINE__);
  test.assertTrue(wifi.clientWaitingForResponse, "should have set clientWaitingForResponse to true", __LINE__);
  test.assertTrue(wifi.passthroughBufferLoaded, "should have set passthroughBufferLoaded to true", __LINE__);
  test.assertEqual((int)wifi.passthroughBuffer[0], (int)cmds.length(), "should have loaded 12 commands into the buffer", __LINE__);
  test.assertEqualBuffer((char *)wifi.passthroughBuffer+1, (char *)cmds.c_str(), expected_length, "should have put the cmds into the pass through buffer", __LINE__);
  test.assertEqual(wifi.passthroughPosition, expected_length+1, "should have put the cmds into the pass through buffer offset by one for storage of num cmds byte", __LINE__);

  wifi.reset();
  wifi.passthroughCommands(cmds);
  test.assertEqualHex(wifi.passthroughCommands(cmds), PASSTHROUGH_PASS, "should be able to pass through the commands", __LINE__);
  test.assertApproximately((float)millis(), (float)wifi.timePassthroughBufferLoaded, (float)10.0, "should have just set the timePassthroughBufferLoaded", __LINE__);
  test.assertTrue(wifi.clientWaitingForResponse, "should have set clientWaitingForResponse to true", __LINE__);
  test.assertTrue(wifi.passthroughBufferLoaded, "should have set passthroughBufferLoaded to true", __LINE__);
  test.assertEqual((int)wifi.passthroughBuffer[0], (int)(cmds.length() + cmds.length()), "should have loaded 24 commands into the buffer", __LINE__);
  test.assertEqualBuffer((char *)wifi.passthroughBuffer+1, (char *)String(cmds + cmds).c_str(), expected_length*2, "should have put the cmds into the pass through buffer", __LINE__);
  test.assertEqual(wifi.passthroughPosition, expected_length*2+1, "should have put the cmds into the pass through buffer offset by one for storage of num cmds byte", __LINE__);

  wifi.reset();
  wifi.passthroughCommands(cmds);
  wifi.passthroughCommands(cmds);
  test.assertEqualHex(wifi.passthroughCommands(cmds), PASSTHROUGH_FAIL_QUEUE_FILLED, "should not be able to pass through the commands", __LINE__);
  test.assertApproximately((float)millis(), (float)wifi.timePassthroughBufferLoaded, (float)10.0, "should have just set the timePassthroughBufferLoaded", __LINE__);
  test.assertTrue(wifi.clientWaitingForResponse, "should have set clientWaitingForResponse to true", __LINE__);
  test.assertTrue(wifi.passthroughBufferLoaded, "should have set passthroughBufferLoaded to true", __LINE__);
  test.assertEqual((int)wifi.passthroughBuffer[0], (int)(cmds.length() + cmds.length()), "should have loaded 24 commands into the buffer", __LINE__);
  test.assertEqualBuffer((char *)wifi.passthroughBuffer+1, (char *)String(cmds + cmds).c_str(), expected_length*2, "should not have put the cmds into the pass through buffer", __LINE__);
  test.assertEqual(wifi.passthroughPosition, expected_length*2+1, "should not have put the cmds into the pass through buffer offset by one for storage of num cmds byte", __LINE__);
}

void testPassthroughBufferClear() {
  test.detail("Buffer Clear");

  wifi.reset();

  wifi.passthroughCommands("tacos");

  wifi.passthroughBufferClear();

  test.assertEqual(wifi.passthroughPosition, 0, "should have zeroed out the passthroughPosition", __LINE__);
  boolean allZeros = true;
  for (int i = 0; i < BYTES_PER_SPI_PACKET; i++) {
    if (wifi.passthroughBuffer[i] > 0) {
      allZeros = false;
    }
  }
  test.assertTrue(allZeros, "should have been able to set each val in passthrough buffer to 0", __LINE__);
}

void testUtilsForPassthrough() {
  test.describe("testPassthrough");
  testPassthroughCommands();
  testPassthroughBufferClear();
}

void testRawBufferCleanUp() {
  wifi.rawBufferHead = 0;
  for (int i = 0; i < NUM_RAW_BUFFERS; i++) {
    wifi.rawBufferReset(wifi.rawBuffer + i);
    wifi.rawBufferClean(wifi.rawBuffer + i);
  }
}

void testRawBufferAddStreamPacket() {
  test.describe("rawBufferAddStreamPacket");
  testRawBufferCleanUp();
  uint8_t buffer[BYTES_PER_SPI_PACKET];
  giveMeASPIStreamPacket(buffer);
  int expectedLength = BYTES_PER_OBCI_PACKET; // Then length of the above buffer
  test.it("should take a 32byte spi packet, add start byte to curRawBuffer, write 31 data bytes, and add stop byte of first spi byte");
  uint8_t expected_buffer[BYTES_PER_OBCI_PACKET];
  expected_buffer[0] = STREAM_PACKET_BYTE_START;
  for (int i = 1; i < BYTES_PER_SPI_PACKET; i++) {
    expected_buffer[i] = buffer[i];
  }
  expected_buffer[BYTES_PER_SPI_PACKET] = buffer[0];
  test.assertTrue(wifi.rawBufferAddStreamPacket((wifi.rawBuffer + wifi.rawBufferHead), buffer), "should be able to add buffer to radioBuf", __LINE__);
  test.assertFalse((wifi.rawBuffer + wifi.rawBufferHead)->gotAllPackets, "should not have all the packets", __LINE__);
  test.assertEqual((wifi.rawBuffer + wifi.rawBufferHead)->positionWrite, expectedLength, "should move positionWrite by 33", __LINE__);
  test.assertEqualBuffer((wifi.rawBuffer + wifi.rawBufferHead)->data, expected_buffer, expectedLength, "should add the whole buffer", __LINE__);

  // Reset buffer
  (wifi.rawBuffer + wifi.rawBufferHead)->positionWrite = 0;

  // Test how this will work in normal operations, i.e. ignoring the byte id
  test.assertTrue(wifi.rawBufferAddStreamPacket((wifi.rawBuffer + wifi.rawBufferHead), buffer),"should be able to add buffer to radioBuf", __LINE__);
  test.assertFalse((wifi.rawBuffer + wifi.rawBufferHead)->gotAllPackets, "should be still have room", __LINE__);
  test.assertEqual((wifi.rawBuffer + wifi.rawBufferHead)->positionWrite , expectedLength,"should set the positionWrite to 33", __LINE__);
  test.assertEqualBuffer((wifi.rawBuffer + wifi.rawBufferHead)->data, expected_buffer, expectedLength,"should add the whole buffer", __LINE__);
}

void testRawBufferClean() {
  test.describe("rawBufferClean");
  testRawBufferCleanUp();
  for (int i = 0; i < BYTES_PER_RAW_BUFFER; i++) {
    (wifi.rawBuffer + wifi.rawBufferHead)->data[i] = 1;
  }

  // Call the function under test
  wifi.rawBufferClean((wifi.rawBuffer + wifi.rawBufferHead));

  test.it("should be able to initialize the raw buffer to all zeros");
  // Should fill the array with all zeros
  boolean allZeros = true;
  for (int j = 0; j < BYTES_PER_RAW_BUFFER; j++) {
    if ((wifi.rawBuffer + wifi.rawBufferHead)->data[j] != 0) {
      allZeros = false;
    }
  }
  test.assertTrue(allZeros, "should set all values to zero", __LINE__);
}

void testRawBufferHasData() {
  test.describe("rawBufferHasData");
  testRawBufferCleanUp();
  (wifi.rawBuffer + wifi.rawBufferHead)->positionWrite = 0;

  // Don't add any data
  test.assertBoolean(wifi.rawBufferHasData((wifi.rawBuffer + wifi.rawBufferHead)),false,"should have no data at first", __LINE__);
  // Add some data
  (wifi.rawBuffer + wifi.rawBufferHead)->positionWrite = 69;
  // Verify!
  test.assertBoolean(wifi.rawBufferHasData((wifi.rawBuffer + wifi.rawBufferHead)),true,"should have data after moving positionWrite", __LINE__);
}

void testRawBuffer_PROCESS_RAW_PASS_FIRST() {
  uint8_t bufferRaw[BYTES_PER_SPI_PACKET];
  giveMeASPIStreamPacket(bufferRaw);
  uint8_t expected_buffer[BYTES_PER_OBCI_PACKET];
  expected_buffer[0] = STREAM_PACKET_BYTE_START;
  for (int i = 1; i < BYTES_PER_SPI_PACKET; i++) {
    expected_buffer[i] = bufferRaw[i];
  }
  expected_buffer[BYTES_PER_SPI_PACKET] = bufferRaw[0];

  testRawBufferCleanUp();
  test.detail("PROCESS_RAW_PASS_FIRST");
  // Last packet
  //      Current buffer has no data
  //          Take it!
  test.assertEqualHex(wifi.rawBufferProcessPacket(bufferRaw), PROCESS_RAW_PASS_FIRST, "should add to raw buffer 1", __LINE__);
  test.assertFalse((wifi.rawBuffer + wifi.rawBufferHead)->gotAllPackets, "should leave gotAllPackets to false", __LINE__);
  test.assertEqual((wifi.rawBuffer + wifi.rawBufferHead)->positionWrite, BYTES_PER_OBCI_PACKET, "should set the positionWrite to 33", __LINE__);
  test.assertEqualBuffer((wifi.rawBuffer + wifi.rawBufferHead)->data, expected_buffer, BYTES_PER_OBCI_PACKET, "should have the packet loaded into the first buffer", __LINE__);

  // Also verify that the buffer was loaded into the correct buffer
  test.assertFalse(wifi.rawBuffer->gotAllPackets, "should be able to set gotAllPackets to true", __LINE__);
  test.assertEqual(wifi.rawBuffer->positionWrite, BYTES_PER_OBCI_PACKET, "should set the positionWrite to 4", __LINE__);
  test.assertEqualBuffer(wifi.rawBuffer->data, expected_buffer, BYTES_PER_OBCI_PACKET, "curRawBuffer should have the taco buffer loaded into it", __LINE__);
}

void testRawBuffer_PROCESS_RAW_PASS_SWITCH() {
  uint8_t bufferRaw[BYTES_PER_SPI_PACKET];
  giveMeASPIStreamPacket(bufferRaw);
  uint8_t expected_buffer[BYTES_PER_OBCI_PACKET];
  expected_buffer[0] = STREAM_PACKET_BYTE_START;
  for (int i = 1; i < BYTES_PER_SPI_PACKET; i++) {
    expected_buffer[i] = bufferRaw[i];
  }
  expected_buffer[BYTES_PER_SPI_PACKET] = bufferRaw[0];

  testRawBufferCleanUp();
  test.detail("PROCESS_RAW_PASS_SWITCH");
  // Need the first buffer to be full
  test.it("should switch to second buffer when first buffer is full and id last packet");
  for (uint8_t i = 0; i < MAX_PACKETS_PER_SEND_TCP; i++) {
    // Serial.printf("[%d]: | retVal: %d | pos: %d\n", i, wifi.rawBufferProcessPacket(bufferRaw, BYTES_PER_OBCI_PACKET), (wifi.rawBuffer + wifi.rawBufferHead)->positionWrite);
    wifi.rawBufferProcessPacket(bufferRaw);
  }

  // Current buffer has data
  //     Current buffer has all packets
  //         Can swtich to other buffer
  //             Take it!
  test.assertEqualHex(wifi.rawBufferProcessPacket(bufferRaw), PROCESS_RAW_PASS_SWITCH, "should add the last packet", __LINE__);
  test.assertFalse((wifi.rawBuffer + 1)->gotAllPackets, "should set gotAllPackets to false for second buffer", __LINE__);
  test.assertEqual((wifi.rawBuffer + 1)->positionWrite, BYTES_PER_OBCI_PACKET, "should set the positionWrite to size of cali buffer", __LINE__);
  test.assertEqualBuffer((wifi.rawBuffer + 1)->data, expected_buffer, BYTES_PER_OBCI_PACKET, "should have loaded raw buffer in the second buffer correctly", __LINE__);

  // Verify that both of the buffers are full
  test.assertTrue(wifi.rawBuffer->gotAllPackets, "should still have a full first buffer after switch", __LINE__);
  test.assertEqual(wifi.rawBuffer->positionWrite,BYTES_PER_RAW_BUFFER,"first buffer should still have correct size", __LINE__);

  test.assertFalse((wifi.rawBuffer + wifi.rawBufferHead)->gotAllPackets, "should set got all packets false on curRawBuffer", __LINE__);
  test.assertEqual((wifi.rawBuffer + wifi.rawBufferHead)->positionWrite, BYTES_PER_OBCI_PACKET, "should set positionWrite of curRawBuffer to that of the second buffer", __LINE__);
  test.assertEqualBuffer((wifi.rawBuffer + wifi.rawBufferHead)->data, expected_buffer, BYTES_PER_OBCI_PACKET, "should have loaded cali buffer into the buffer curRawBuffer points to", __LINE__);


  // Do it again in reverse, where the last buffer is full
  // So clear the first buffer and point to the second
  test.it("should switch to first buffer when last buffer is full and id last packet");
  testRawBufferCleanUp();
  wifi.rawBufferHead = NUM_RAW_BUFFERS - 1;
  for (uint8_t i = 0; i < MAX_PACKETS_PER_SEND_TCP; i++) {
    wifi.rawBufferProcessPacket(bufferRaw);
  }

  // Current buffer has data
  //     Current buffer has all packets
  //         Can swtich to other buffer
  //             Take it!
  test.assertEqualHex(wifi.rawBufferProcessPacket(bufferRaw), PROCESS_RAW_PASS_SWITCH, "should add the last packet", __LINE__);
  test.assertBoolean(wifi.rawBuffer->gotAllPackets, false, "should set gotAllPackets to false for second buffer", __LINE__);
  test.assertEqual(wifi.rawBuffer->positionWrite, BYTES_PER_OBCI_PACKET, "should set the positionWrite to size of cali buffer", __LINE__);
  test.assertEqualBuffer(wifi.rawBuffer->data, expected_buffer, BYTES_PER_OBCI_PACKET, "should have loaded cali buffer in the second buffer correctly", __LINE__);

  // Verify that both of the buffers have data
  test.assertBoolean((wifi.rawBuffer + NUM_RAW_BUFFERS - 1)->gotAllPackets, true, "should still have a full first buffer after switch", __LINE__);
  test.assertEqual((wifi.rawBuffer + NUM_RAW_BUFFERS - 1)->positionWrite,BYTES_PER_RAW_BUFFER,"first buffer should still have correct size", __LINE__);

  test.assertBoolean((wifi.rawBuffer + wifi.rawBufferHead)->gotAllPackets, false, "should set got all packets false on curRawBuffer", __LINE__);
  test.assertEqual((wifi.rawBuffer + wifi.rawBufferHead)->positionWrite, BYTES_PER_OBCI_PACKET, "should set positionWrite of curRawBuffer to that of the second buffer", __LINE__);
  test.assertEqualBuffer((wifi.rawBuffer + wifi.rawBufferHead)->data, expected_buffer, BYTES_PER_OBCI_PACKET, "should have loaded cali buffer into the buffer curRawBuffer points to", __LINE__);

  test.it("should switch to second buffer when first is flushing");
  // First buffer flushing, second empty
  testRawBufferCleanUp();
  test.assertEqualHex(wifi.rawBufferProcessPacket(bufferRaw), PROCESS_RAW_PASS_FIRST);
  test.assertFalse(wifi.rawBuffer->gotAllPackets, "should set gotAllPackets to false for first buffer", __LINE__);
  test.assertEqualBuffer(wifi.rawBuffer->data, expected_buffer, BYTES_PER_OBCI_PACKET, "should have loaded data buffer in the first buffer correctly", __LINE__);
  test.assertEqualBuffer((wifi.rawBuffer + wifi.rawBufferHead)->data, expected_buffer, BYTES_PER_OBCI_PACKET, "should have loaded cali buffer into the buffer curRawBuffer points to", __LINE__);
  // Test is to simulate the first one is being flushed as this new packet comes in
  // Set the first buffer to flushing
  wifi.rawBuffer->flushing = true;
  // Current buffer has data
  //     Current buffer has all packets
  //         Can swtich to other buffer
  //             Take it! Mark Last
  test.assertEqualHex(wifi.rawBufferProcessPacket(bufferRaw), PROCESS_RAW_PASS_SWITCH,"should switch and add the last packet when first is flushing", __LINE__);
  test.assertFalse((wifi.rawBuffer + 1)->gotAllPackets, "should mark the second buffer not full", __LINE__);
  test.assertEqualBuffer((wifi.rawBuffer + 1)->data, expected_buffer, BYTES_PER_OBCI_PACKET, "should have the taco buffer loaded into the second buffer", __LINE__);
  test.assertEqualBuffer((wifi.rawBuffer + wifi.rawBufferHead)->data, expected_buffer, BYTES_PER_OBCI_PACKET, "should have the taco buffer loaded into curRawBuffer", __LINE__);
  // Verify the first buffer is still loaded with the cali buffer
  test.assertTrue(wifi.rawBuffer->flushing, "should have flushing true for first buffer", __LINE__);
  test.assertFalse(wifi.rawBuffer->gotAllPackets, "should still have gotAllPackets false for first buffer", __LINE__);
  test.assertEqual(wifi.rawBuffer->positionWrite, BYTES_PER_OBCI_PACKET,"should still have positionWrite to size of cali buffer in buffer 1", __LINE__);
  test.assertEqualBuffer(wifi.rawBuffer->data, expected_buffer, BYTES_PER_OBCI_PACKET, "should still have loaded cali buffer in the first buffer correctly", __LINE__);

  test.it("should switch to first buffer when second is flushing and id last packet");
  // Second buffer flushing, first empty
  testRawBufferCleanUp();
  // Load the cali buffer into the second buffer
  wifi.rawBufferHead = 1;
  test.assertEqualHex(wifi.rawBufferProcessPacket(bufferRaw), PROCESS_RAW_PASS_FIRST);
  test.assertFalse((wifi.rawBuffer + 1)->gotAllPackets, "should set gotAllPackets to false for first buffer", __LINE__);
  test.assertEqualBuffer((wifi.rawBuffer + 1)->data, expected_buffer, BYTES_PER_OBCI_PACKET, "should have loaded data buffer in the first buffer correctly", __LINE__);
  test.assertEqualBuffer((wifi.rawBuffer + wifi.rawBufferHead)->data, expected_buffer, BYTES_PER_OBCI_PACKET, "should have loaded cali buffer into the buffer curRawBuffer points to", __LINE__);
  // Test is to simulate the second one is being flushed as this new packet comes in
  // Set the second buffer to flushing
  (wifi.rawBuffer + 1)->flushing = true;
  // Current buffer has data
  //     Current buffer has all packets
  //         Can swtich to other buffer
  //             Take it!
  test.assertEqualHex(wifi.rawBufferProcessPacket(bufferRaw), PROCESS_RAW_PASS_SWITCH,"should switch and add the last packet when first is flushing", __LINE__);
  test.assertFalse(wifi.rawBuffer->gotAllPackets, "should mark the second buffer not full", __LINE__);
  test.assertEqualBuffer(wifi.rawBuffer->data, expected_buffer, BYTES_PER_OBCI_PACKET, "should have the taco buffer loaded into the second buffer", __LINE__);
  test.assertEqualBuffer((wifi.rawBuffer + wifi.rawBufferHead)->data, expected_buffer, BYTES_PER_OBCI_PACKET, "should have the taco buffer loaded into curRawBuffer", __LINE__);
  // Verify the first buffer is still loaded with the cali buffer
  test.assertTrue((wifi.rawBuffer + 1)->flushing, "should have flushing true for first buffer", __LINE__);
  test.assertFalse((wifi.rawBuffer + 1)->gotAllPackets, "should still have gotAllPackets false for first buffer", __LINE__);
  test.assertEqual((wifi.rawBuffer + 1)->positionWrite, BYTES_PER_OBCI_PACKET,"should still have positionWrite to size of cali buffer in buffer 1", __LINE__);
  test.assertEqualBuffer((wifi.rawBuffer + 1)->data, expected_buffer, BYTES_PER_OBCI_PACKET, "should still have loaded cali buffer in the first buffer correctly", __LINE__);

}

void testRawBuffer_PROCESS_RAW_PASS_MIDDLE() {
  uint8_t bufferRaw[BYTES_PER_SPI_PACKET];
  giveMeASPIStreamPacket(bufferRaw);
  uint8_t bufferRaw2[BYTES_PER_SPI_PACKET];
  uint8_t expected_sampleNumber = 1;
  giveMeASPIStreamPacket(bufferRaw2, expected_sampleNumber);
  uint8_t numSamples = 2;
  uint8_t expected_buffer[BYTES_PER_OBCI_PACKET*numSamples];
  uint8_t byteCounter = 0;
  expected_buffer[byteCounter++] = STREAM_PACKET_BYTE_START;
  for (int i = 1; i < BYTES_PER_SPI_PACKET; i++) {
    expected_buffer[byteCounter++] = bufferRaw[i];
  }
  expected_buffer[byteCounter++] = bufferRaw[0];
  expected_buffer[byteCounter++] = STREAM_PACKET_BYTE_START;
  for (int i = 1; i < BYTES_PER_SPI_PACKET; i++) {
    expected_buffer[byteCounter++] = bufferRaw2[i];
  }
  expected_buffer[byteCounter++] = bufferRaw2[0];

  testRawBufferCleanUp();
  test.detail("PROCESS_RAW_PASS_MIDDLE");
  test.it("should return middle pass when packet entered");
  wifi.rawBufferProcessPacket(bufferRaw);

  // Current buffer has data
  //     Current buffer does not have all packets
  //         Previous packet number == packetNumber + 1
  //             Take it!
  test.assertEqualHex(wifi.rawBufferProcessPacket(bufferRaw2), PROCESS_RAW_PASS_MIDDLE);
  test.assertFalse(wifi.rawBuffer->gotAllPackets, "should not have gotAllPackets", __LINE__);
  test.assertEqual(wifi.rawBuffer->positionWrite, BYTES_PER_OBCI_PACKET * 2, "should set the positionWrite to size of two packets", __LINE__);
  test.assertEqualBuffer(wifi.rawBuffer->data, expected_buffer, BYTES_PER_OBCI_PACKET*2, "should have two buffers loaded into rawBuffer", __LINE__);

}

void testRawBuffer_PROCESS_RAW_FAIL_SWITCH() {
  uint8_t bufferRaw[BYTES_PER_SPI_PACKET];
  giveMeASPIStreamPacket(bufferRaw);
  uint8_t expected_buffer[BYTES_PER_OBCI_PACKET];
  expected_buffer[0] = STREAM_PACKET_BYTE_START;
  for (int i = 1; i < BYTES_PER_SPI_PACKET; i++) {
    expected_buffer[i] = bufferRaw[i];
  }
  expected_buffer[BYTES_PER_SPI_PACKET] = bufferRaw[0];

  testRawBufferCleanUp();
  test.detail("PROCESS_RAW_FAIL_SWITCH");
  test.it("should not be able to switch to other buffer when both are full");

  // Fill the two buffers
  for (uint8_t i = 0; i < MAX_PACKETS_PER_SEND_TCP * 2; i++) {
    wifi.rawBufferProcessPacket(bufferRaw);
  }

  // Current buffer has data
  //     Current buffer has all packets
  //         Cannot switch to other buffer
  //             Reject it!
  test.assertEqualHex(wifi.rawBufferProcessPacket(bufferRaw),PROCESS_RAW_FAIL_SWITCH,"should reject the addition of this buffer", __LINE__);

  test.it("should not be able to switch to other buffer when the buffers are flushing");
  testRawBufferCleanUp();
  wifi.rawBuffer->flushing = true;
  (wifi.rawBuffer+1)->flushing = true;
  // Current buffer has data
  //     Current buffer has all packets
  //         Cannot switch to other buffer
  //             Reject it!
  test.assertEqualHex(wifi.rawBufferProcessPacket(bufferRaw),PROCESS_RAW_FAIL_SWITCH,"should reject the addition of this buffer", __LINE__);
}

void testRawBufferProcessPacket() {
  test.describe("rawBufferProcessPacket");

  testRawBuffer_PROCESS_RAW_PASS_FIRST();

  testRawBuffer_PROCESS_RAW_PASS_SWITCH();

  testRawBuffer_PROCESS_RAW_PASS_MIDDLE();

  testRawBuffer_PROCESS_RAW_FAIL_SWITCH();

  // testRawBuffer_PROCESS_RAW_FAIL_OVERFLOW_FIRST();
  //
  // testRawBuffer_PROCESS_RAW_FAIL_OVERFLOW_FIRST_AFTER_SWITCH();
  //
  // testRawBuffer_PROCESS_RAW_FAIL_OVERFLOW_MIDDLE();
}

void testRawBufferReadyForNewPage() {
  uint8_t bufferRaw[BYTES_PER_SPI_PACKET];
  giveMeASPIStreamPacket(bufferRaw);
  // # CLEANUP
  testRawBufferCleanUp();

  test.describe("rawBufferReadyForNewPage");

  test.it("works with clean state");
  test.assertTrue(wifi.rawBufferReadyForNewPage(wifi.rawBuffer), "should be ready to add new page in the first buffer", __LINE__);
  test.assertTrue(wifi.rawBufferReadyForNewPage(wifi.rawBuffer + 1), "should be ready to add new page in the second buffer", __LINE__);
  test.assertTrue(wifi.rawBufferReadyForNewPage((wifi.rawBuffer + wifi.rawBufferHead)), "should be ready to add new page in the curRawBuffer", __LINE__);

  // Add data to buffer 1
  test.it("not ready for a new page when data is in the buffer");
  wifi.rawBufferAddStreamPacket((wifi.rawBuffer + wifi.rawBufferHead), bufferRaw);
  test.assertFalse(wifi.rawBufferReadyForNewPage(wifi.rawBuffer), "should not be ready to add new page in the first buffer", __LINE__);
  test.assertTrue(wifi.rawBufferReadyForNewPage(wifi.rawBuffer + 1), "should be ready to add new page in the second buffer", __LINE__);
  test.assertFalse(wifi.rawBufferReadyForNewPage((wifi.rawBuffer + wifi.rawBufferHead)), "should not be ready to add new page in the curRawBuffer", __LINE__);

  // Increment the curRawBuffer pointer
  // (wifi.rawBuffer + wifi.rawBufferHead)++;
  // Clear the buffers
  // # CLEANUP
  testRawBufferCleanUp();

  // Add data to buffer 2
  //
  for (uint8_t i = 0; i < MAX_PACKETS_PER_SEND_TCP * 2; i++) {
    wifi.rawBufferProcessPacket(bufferRaw);
  }
  test.it("cannot add a page to either the first or second buffer when both are filled");
  wifi.rawBufferAddStreamPacket((wifi.rawBuffer + wifi.rawBufferHead), bufferRaw);
  test.assertFalse(wifi.rawBufferReadyForNewPage(wifi.rawBuffer), "should not be ready to add new page in the first buffer", __LINE__);
  test.assertFalse(wifi.rawBufferReadyForNewPage(wifi.rawBuffer + 1),"should not be ready to add new page in the second buffer", __LINE__);
  test.assertFalse(wifi.rawBufferReadyForNewPage((wifi.rawBuffer + wifi.rawBufferHead)), "should not be ready to add new page in the curRawBuffer", __LINE__);

  // Clear the buffers
  // # CLEANUP
  testRawBufferCleanUp();

  // Mark first buffer as flushing
  test.it("cannot add a page to first buffer but can the second when flushing");
  wifi.rawBuffer->flushing = true;
  test.assertFalse(wifi.rawBufferReadyForNewPage(wifi.rawBuffer), "should not be ready to add new page in the first buffer", __LINE__);
  test.assertTrue(wifi.rawBufferReadyForNewPage(wifi.rawBuffer + 1), "should be ready to add new page in the second buffer", __LINE__);
  test.assertFalse(wifi.rawBufferReadyForNewPage((wifi.rawBuffer + wifi.rawBufferHead)), "should not be ready to add new page in the curRawBuffer", __LINE__);
  wifi.rawBuffer->flushing = false;
  // Mark second buffer as flushing
  test.it("cannot add a page to second buffer but can the first when flushing");
  (wifi.rawBuffer + 1)->flushing = true;
  test.assertTrue(wifi.rawBufferReadyForNewPage(wifi.rawBuffer), "should be ready to add new page in the first buffer", __LINE__);
  test.assertFalse(wifi.rawBufferReadyForNewPage(wifi.rawBuffer + 1), "should not be ready to add new page in the second buffer", __LINE__);
  test.assertTrue(wifi.rawBufferReadyForNewPage((wifi.rawBuffer + wifi.rawBufferHead)), "should be ready to add new page in the curRawBuffer", __LINE__);
  (wifi.rawBuffer + 1)->flushing = true;

  // Both flushing
  test.it("cannot add a page to either when both flushing");
  wifi.rawBuffer->flushing = true;
  (wifi.rawBuffer + 1)->flushing = true;
  test.assertFalse(wifi.rawBufferReadyForNewPage(wifi.rawBuffer), "should not be ready to add new page in the first buffer", __LINE__);
  test.assertFalse(wifi.rawBufferReadyForNewPage(wifi.rawBuffer + 1), "should not be ready to add new page in the second buffer", __LINE__);
  test.assertFalse(wifi.rawBufferReadyForNewPage((wifi.rawBuffer + wifi.rawBufferHead)), "should not be ready to add new page in the curRawBuffer", __LINE__);

  // # CLEANUP
  testRawBufferCleanUp();
}

void testRawBufferReset() {
  // Test the reset functions
  test.describe("rawBufferReset");
  wifi.reset();
  test.it("should reset the passed in raw buffer");
  (wifi.rawBuffer + wifi.rawBufferHead)->flushing = true;
  (wifi.rawBuffer + wifi.rawBufferHead)->gotAllPackets = true;
  (wifi.rawBuffer + wifi.rawBufferHead)->positionWrite = 60;

  // Reset the flags
  wifi.rawBufferReset((wifi.rawBuffer + wifi.rawBufferHead));

  // Verify they got Reset
  test.assertFalse((wifi.rawBuffer + wifi.rawBufferHead)->flushing, "should set flushing to false");
  test.assertFalse((wifi.rawBuffer + wifi.rawBufferHead)->gotAllPackets, "should set got all packets to false");
  test.assertEqual((wifi.rawBuffer + wifi.rawBufferHead)->positionWrite, 0, "should set positionWrite to 0");

  test.it("should reset all the raw buffers");

  wifi.rawBuffer->flushing = true;
  wifi.rawBuffer->gotAllPackets = true;
  wifi.rawBuffer->positionWrite = 60;
  (wifi.rawBuffer + 1)->flushing = true;
  (wifi.rawBuffer + 1)->gotAllPackets = true;
  (wifi.rawBuffer + 1)->positionWrite = 60;

  // Reset the flags
  wifi.rawBufferReset();

  // Verify they got Reset
  test.assertFalse(wifi.rawBuffer->flushing, "should set flushing to false");
  test.assertFalse(wifi.rawBuffer->gotAllPackets, "should set got all packets to false");
  test.assertEqual(wifi.rawBuffer->positionWrite, 0, "should set positionWrite to 0");
  test.assertFalse((wifi.rawBuffer + 1)->flushing, "should set flushing to false");
  test.assertFalse((wifi.rawBuffer + 1)->gotAllPackets, "should set got all packets to false");
  test.assertEqual((wifi.rawBuffer + 1)->positionWrite, 0, "should set positionWrite to 0");


}

void testRawBufferSwitchToOtherBuffer() {
  // # CLEANUP
  testRawBufferCleanUp();

  uint8_t bufferRaw[BYTES_PER_SPI_PACKET];
  giveMeASPIStreamPacket(bufferRaw);

  test.describe("rawBufferSwitchToOtherBuffer");

  test.it("should return true if buffer 2 does not have data and should move the pointer");
  wifi.rawBufferHead = 0;
  test.assertTrue(wifi.rawBufferSwitchToOtherBuffer(), "can switch to other empty buffer", __LINE__);
  test.assertEqual(wifi.rawBufferHead, 1, "head points to second buffer", __LINE__);

  testRawBufferCleanUp();

  test.it("should return true if buffer 1 does not have data and should move the pointer");
  wifi.rawBufferHead = NUM_RAW_BUFFERS - 2;
  test.assertTrue(wifi.rawBufferSwitchToOtherBuffer(), "can switch to 3rd other empty buffer", __LINE__);
  test.assertEqual(wifi.rawBufferHead, NUM_RAW_BUFFERS - 1, "head points to last buffer", __LINE__);

  testRawBufferCleanUp();

  test.it("should return true if first buffer does not have data");
  wifi.rawBufferHead = NUM_RAW_BUFFERS - 1;
  test.assertTrue(wifi.rawBufferSwitchToOtherBuffer(), "can switch to 1st buffer", __LINE__);
  test.assertEqual(wifi.rawBufferHead, 0, "head points to first buffer", __LINE__);

  // # CLEANUP
  testRawBufferCleanUp();

  test.it("should return false when currently pointed at 1st buf AND next buf has data");
  wifi.rawBufferHead = 0;
  wifi.rawBufferAddStreamPacket(wifi.rawBuffer + 1, bufferRaw);
  test.assertFalse(wifi.rawBufferSwitchToOtherBuffer(), "cannot switch to buffer with data", __LINE__);
  test.assertEqual(wifi.rawBufferHead, 0, "head still points to first buffer", __LINE__);

  // # CLEANUP
  testRawBufferCleanUp();

  test.it("should return false when currently pointed at last buf and 1st buf has data");
  wifi.rawBufferHead = NUM_RAW_BUFFERS - 1;
  wifi.rawBufferAddStreamPacket(wifi.rawBuffer, bufferRaw);
  test.assertFalse(wifi.rawBufferSwitchToOtherBuffer(),"cannot switch to buffer with data", __LINE__);
  test.assertEqual(wifi.rawBufferHead, NUM_RAW_BUFFERS - 1, "head still points to last buffer", __LINE__);

  // # CLEANUP
  testRawBufferCleanUp();

  test.it("should return false when all buffers have data");
  for (int i = 0; i < NUM_RAW_BUFFERS; i++) {
    wifi.rawBufferAddStreamPacket(wifi.rawBuffer + i, bufferRaw);
  }
  test.assertFalse(wifi.rawBufferSwitchToOtherBuffer(),"can't switch to second", __LINE__);

  // # CLEANUP
  testRawBufferCleanUp();

  test.it("should return false when other buffer is flushing");
  wifi.rawBufferAddStreamPacket(wifi.rawBuffer, bufferRaw);
  (wifi.rawBuffer + 1)->flushing = true; // don't add data, just set it to flushing
  test.assertFalse(wifi.rawBufferSwitchToOtherBuffer(),"can't switch to second because it's flushing", __LINE__);

  // # CLEANUP
  testRawBufferCleanUp();

  test.it("should not switch when buffers are flushing");
  wifi.rawBuffer->flushing = true; // don't add data, just set it to flushing
  (wifi.rawBuffer + 1)->flushing = true; // don't add data, just set it to flushing
  test.assertFalse(wifi.rawBufferSwitchToOtherBuffer(),"can't switch to other buffer", __LINE__);
}

void testUtilisForRaw() {
  testRawBufferCleanUp();
  testRawBufferAddStreamPacket();
  testRawBufferClean();
  testRawBufferHasData();
  testRawBufferProcessPacket();
  testRawBufferReadyForNewPage();
  testRawBufferReset();
  testRawBufferSwitchToOtherBuffer();
}

void testSpiHasMaster() {
  test.describe("spiHasMaster");

  wifi.reset();

  test.assertFalse(wifi.spiHasMaster(), "should not have spi master", __LINE__);

  wifi.lastTimeWasPolled = millis();

  test.assertTrue(wifi.spiHasMaster(), "should have spi master", __LINE__);
}

void testSPIOnDataSent() {
  test.describe("spiOnDataSent");
  wifi.reset();

  wifi.spiOnDataSent();

  test.assertApproximately((float)millis(), (float)wifi.lastTimeWasPolled, (float)5.0, "should have just set the lastTimeWasPolled", __LINE__);
  test.assertEqual(wifi.passthroughPosition, (uint8_t)0, "should have reset the passthrough buffer", __LINE__);
}

void testSPIProcessPacketStreamRaw() {
  test.detail("Raw");

  wifi.reset();

  uint8_t expected_sampleNumber = 1; // Jordan
  uint8_t data[BYTES_PER_SPI_PACKET];
  giveMeASPIStreamPacket(data, expected_sampleNumber);

  wifi.setOutputMode(wifi.OUTPUT_MODE_RAW);
  test.it("should add 33 bytes to the output buffer");
  wifi.spiProcessPacketStreamRaw(data);

  test.assertEqualHex((wifi.rawBuffer + wifi.rawBufferHead)->data[0], STREAM_PACKET_BYTE_START, "should set first byte to start byte", __LINE__);
  test.assertEqualBuffer((wifi.rawBuffer + wifi.rawBufferHead)->data + 1, data + 1, BYTES_PER_SPI_PACKET-1, "should have the same 31 data bytes");
  test.assertEqualHex((wifi.rawBuffer + wifi.rawBufferHead)->data[BYTES_PER_SPI_PACKET], 0xC0, "should set the stop byte", __LINE__);

}

void testSPIProcessPacketStream() {
  test.describe("spiProcessPacketStream");
  testSPIProcessPacketStreamRaw();
}

void testSPIProcessPacketGain() {
  test.detail("Gain");

  wifi.reset();

  uint8_t expected_numChannels = NUM_CHANNELS_CYTON_DAISY;
  uint8_t spiPacket[BYTES_PER_SPI_PACKET];

  giveMeASPIPacketGainSet(spiPacket, expected_numChannels);

  uint8_t expected_gains[expected_numChannels];
  for(int i = 0; i < expected_numChannels; i++) {
    expected_gains[i] = wifi.getGainCyton(spiPacket[i+2]);
  }

  test.it("should set for cyton daisy the gains and num of channels in system");
  wifi.spiProcessPacketGain(spiPacket);

  test.assertEqualBuffer(wifi.getGains(), expected_gains, expected_numChannels, "should have been able to extract the right gains", __LINE__);
  test.assertEqual(wifi.getNumChannels(), expected_numChannels, "should have gotten the right number of channels", __LINE__);
  test.assertEqual(wifi.getCurBoardTypeString(), "daisy", "should have gotten daisy for cur board type", __LINE__);
}

void testSPIProcessPacketResponse() {
  test.detail("Client Response");

  wifi.reset();

  test.it("should add the message to outputString, and not change client waiting or fufilled and set cur client response");
  uint8_t message[BYTES_PER_SPI_PACKET];
  for (int i = 1; i < BYTES_PER_SPI_PACKET; i++) {
    message[i] = 0;
  }
  message[0] = WIFI_SPI_MSG_MULTI;
  message[1] = 'a'; message[2] = 'j'; message[3] = ' ';
  message[4] = 'p'; message[5] = 't'; message[6] = 'w';

  wifi.clientWaitingForResponse = true;
  wifi.clientWaitingForResponseFullfilled = false;

  wifi.spiProcessPacketResponse(message);

  String expected_output = (char *)message+1;
  test.assertEqual(wifi.outputString, expected_output, "should have been able to put message into string", __LINE__);
  test.assertTrue(wifi.clientWaitingForResponse, "should have kept clientWaitingForResponse to true", __LINE__);
  test.assertFalse(wifi.clientWaitingForResponseFullfilled, "should have kept clientWaitingForResponseFullfilled to false", __LINE__);
  test.assertEqualHex((uint8_t)wifi.curClientResponse, (uint8_t)wifi.CLIENT_RESPONSE_NONE, "should leave output string to none");

  test.it("should load the last message and set flags");
  message[0] = WIFI_SPI_MSG_LAST;

  wifi.spiProcessPacketResponse(message);
  expected_output.concat((char *)message+1);
  test.assertEqual(wifi.outputString, expected_output, "should have been able to put message into output string", __LINE__);
  test.assertFalse(wifi.clientWaitingForResponse, "should change clientWaitingForResponse to false", __LINE__);
  test.assertTrue(wifi.clientWaitingForResponseFullfilled, "should change clientWaitingForResponseFullfilled to true", __LINE__);
  test.assertEqualHex((uint8_t)wifi.curClientResponse, (uint8_t)wifi.CLIENT_RESPONSE_OUTPUT_STRING, "should set to output string for response type");

  test.it("should default to a none client response and set flags to send ok response");

  wifi.clientWaitingForResponse = true;
  wifi.clientWaitingForResponseFullfilled = false;

  message[0] = 0x23;

  wifi.spiProcessPacketResponse(message);

  test.assertEqualHex((uint8_t)wifi.curClientResponse, (uint8_t)wifi.CLIENT_RESPONSE_NONE, "should set output string to none");
  test.assertFalse(wifi.clientWaitingForResponse, "should change clientWaitingForResponse to false", __LINE__);
  test.assertTrue(wifi.clientWaitingForResponseFullfilled, "should change clientWaitingForResponseFullfilled to true", __LINE__);
}

void testSPIProcessPacketOthers() {
  test.describe("testSPIProcessPacketOther");
  testSPIProcessPacketGain();
  testSPIProcessPacketResponse();
}

void testSPIProcessPacket() {
  test.describe("spiProcessPacket");
  testSPIProcessPacketStream();
  testSPIProcessPacketOthers();
}

void testUtilisForSPI() {
  testSpiHasMaster();
  testSPIOnDataSent();
  testSPIProcessPacket();
}

void testUtils() {
  testIsAStreamByte();
  testUtilsForPassthrough();
  testUtilisForRaw();
  testUtilisForSPI();
}

void go() {
  // Start the test
  test.begin();
  digitalWrite(ledPin, HIGH);
  testGetters();
  delay(500);
  digitalWrite(ledPin, LOW);
  testSetters();
  delay(500);
  digitalWrite(ledPin, HIGH);
  testUtils();
  test.end();
  digitalWrite(ledPin, LOW);
}

void setup() {
  pinMode(ledPin,OUTPUT);
  Serial.begin(230400);
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

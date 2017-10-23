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

void testGetInfoMQTT() {
  test.detail("MQTT");
  wifi.reset();

  boolean expected_connected = false;
  String actual_infoMqtt = wifi.getInfoMQTT(expected_connected);

  const size_t bufferSize = JSON_OBJECT_SIZE(7) + 1400;
  DynamicJsonBuffer jsonBuffer(bufferSize);

  // const char* json = "{\"broker_address\":\"mock.getcloudbrain.com\",\"connected\":false,\"username\":\"/a253c7a141daca0dc6bfe5f51bee7ef5f1ca4b9cb9807ff0ea1f1737f771f573:a253c7a141daca0dc6bfe5f51bee7ef5f1ca4b9cb9807ff0ea1f1737f771f573\",\"password\":\"\"}";
  JsonObject& root = jsonBuffer.parseObject(actual_infoMqtt);
  String brokerAddress = root["broker_address"]; // "mock.getcloudbrain.com"
  test.assertEqual(brokerAddress, "", "should be an empty string");
  boolean connected = root["connected"]; // false
  test.assertBoolean(connected, expected_connected, "should not be connected");
  String output = root["output"]; // "raw"
  test.assertEqual(output, "raw", "should be default to raw", __LINE__);
  String username = root["username"]; // "/a253c7a141daca0dc6bfe5f51bee7ef5f1ca4b9cb9807ff0ea1f1737f771f573:a253c7a141daca0dc6bfe5f51bee7ef5f1ca4b9cb9807ff0ea1f1737f771f573"
  test.assertEqual(username, "", "should be an empty username", __LINE__);
  String password = root["password"]; // ""
  test.assertEqual(password, "", "should be an empty password", __LINE__);
  unsigned long latency = root["latency"];
  test.assertEqual(latency, (unsigned long)DEFAULT_LATENCY, "should get the default latency", __LINE__);
  int port = root["port"];
  test.assertEqual(port, (int)DEFAULT_MQTT_PORT, "should get the default port", __LINE__);

  String expected_brokerAddress = "mock.getcloudbrain.com";
  String expected_username = "/a253c7a141daca0dc6bfe5f51bee7ef5f1ca4b9cb9807ff0ea1f1737f771f573:a253c7a141daca0dc6bfe5f51bee7ef5f1ca4b9cb9807ff0ea1f1737f771f573";
  String expected_password = "";
  int expected_port = 687;

  Serial.print("expected username1 "); Serial.println(expected_username);


  wifi.setInfoMQTT(expected_brokerAddress, expected_username, expected_password, expected_port);
  wifi.setOutputMode(wifi.OUTPUT_MODE_JSON);
  expected_connected = true;
  actual_infoMqtt = wifi.getInfoMQTT(expected_connected);

  DynamicJsonBuffer jsonBuffer1(bufferSize);

  JsonObject& root1 = jsonBuffer1.parseObject(actual_infoMqtt);
  brokerAddress = root1.get<String>("broker_address"); // "mock.getcloudbrain.com"
  test.assertEqual(brokerAddress, expected_brokerAddress, "should get brokerAddress");
  connected = root1.get<boolean>("connected"); // false
  test.assertBoolean(connected, expected_connected, "should be connected");
  output = root1.get<String>("output"); // "json"
  test.assertEqual(output, "json", "should switch to json", __LINE__);
  username = root1.get<String>("username"); // "/a253c7a141daca0dc6bfe5f51bee7ef5f1ca4b9cb9807ff0ea1f1737f771f573:a253c7a141daca0dc6bfe5f51bee7ef5f1ca4b9cb9807ff0ea1f1737f771f573"
  Serial.print("expected username "); Serial.println(expected_username);
  test.assertEqual(username, expected_username, "should get username", __LINE__);
  password = root1.get<String>("password"); // ""
  test.assertEqual(password, expected_password, "should get password", __LINE__);
  latency = root1.get<unsigned long>("latency");
  test.assertEqual(latency, (unsigned long)DEFAULT_LATENCY, "should get the default latency", __LINE__);
  port = root1.get<int>("port");
  test.assertEqual(port, (int)expected_port, "should get the new port number", __LINE__);
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
  testGetInfoMQTT();
  testGetInfoTCP();
}

void testGetJSONAdditionalBytes() {
  test.detail("getJSONAdditionalBytes");

  test.assertEqual(wifi.getJSONAdditionalBytes(NUM_CHANNELS_GANGLION), 1164, "should get the right num of bytes for ganglion", __LINE__);
  test.assertEqual(wifi.getJSONAdditionalBytes(NUM_CHANNELS_CYTON), 1116, "should get the right num of bytes for cyton", __LINE__);
  test.assertEqual(wifi.getJSONAdditionalBytes(NUM_CHANNELS_CYTON_DAISY), 1212, "should get the right num of bytes for cyton daisy", __LINE__);
}

void testGetJSONBufferSize() {
  test.detail("getJSONBufferSize");

  wifi.reset();

  test.it("should set all channel possibilities and verify json max buffer is less than 3000 bytes");
  test.assertLessThan(wifi.getJSONBufferSize(), (size_t)MAX_JSON_BUFFER_SIZE, "should initialize json buffer size with 0 channels", __LINE__);
  wifi.setNumChannels(NUM_CHANNELS_CYTON);
  test.assertLessThan(wifi.getJSONBufferSize(), (size_t)MAX_JSON_BUFFER_SIZE, "cyton json buffer size less than 3000 bytes per chunk", __LINE__);
  wifi.setNumChannels(NUM_CHANNELS_CYTON_DAISY);
  test.assertLessThan(wifi.getJSONBufferSize(), (size_t)MAX_JSON_BUFFER_SIZE, "cyton daisy json buffer size less than 3000 bytes per chunk", __LINE__);
  wifi.setNumChannels(NUM_CHANNELS_GANGLION);
  test.assertLessThan(wifi.getJSONBufferSize(), (size_t)MAX_JSON_BUFFER_SIZE, "ganglion json buffer size less than 3000 bytes per chunk", __LINE__);
}

void testGetJSONFromSamplesCytonMax() {
  wifi.reset(); // Clear everything
  wifi.jsonHasSampleNumbers = true;
  test.it("should work to get JSON from max samples Cyton. mem stress");
  uint8_t numChannels = NUM_CHANNELS_CYTON;
  wifi.setNumChannels(numChannels);
  uint8_t numPackets = wifi.getJSONMaxPackets(numChannels);
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

  for (uint8_t i = 0; i < numPackets; i++) {
    wifi.sampleReset(wifi.sampleBuffer + i);
    (wifi.sampleBuffer + i)->sampleNumber = expected_sampleNumber + i;
    (wifi.sampleBuffer + i)->timestamp = expected_timestamp + i;
    for (uint8_t j = 0; j < numChannels; j++) {
      (wifi.sampleBuffer + i)->channelData[j] = expected_channelData[j];
      // Serial.printf("j:%d: i:%d %.0f\n", j, i, (wifi.sampleBuffer + i)->channelData[j]);
    }
  }

  DynamicJsonBuffer jsonBuffer(wifi.getJSONBufferSize());
  JsonObject& root = jsonBuffer.createObject();
  wifi.getJSONFromSamples(root, numChannels, numPackets);

  JsonArray& chunk = root["chunk"];

  for (uint8_t i = 0; i < numPackets; i++) {
    JsonObject& sample = chunk[i];
    unsigned long long actual_timestamp = sample["timestamp"]; // 1500916934017000
    test.assertEqual(actual_timestamp, expected_timestamp + i, String("should be able to set timestamp for " + String(i)).c_str(), __LINE__);

    uint8_t actual_sampleNumber = sample["sampleNumber"]; // 255
    test.assertEqual(actual_sampleNumber, expected_sampleNumber + i, String("cyton multi packet should be able to set sampleNumber for " + String(i)).c_str(), __LINE__);

    JsonArray& sample_data = sample["data"];
    for (uint8_t j = 0; j < numChannels; j++) {
      double temp_d = sample_data[j];
      // Serial.printf("\nj[%d]: td:%0.0f e_cD:%.0f\n", j, temp_d, expected_channelData[j]-i);
      test.assertApproximately(temp_d, expected_channelData[j], 1.0, String("should be able to code large numbers "+String(j)+" for packet " + String(i)).c_str(), __LINE__);
    }
  }
}

void testGetJSONFromSamplesCytonSingle() {
  test.it("should work to get a single JSON from samples Cyton");

  wifi.reset(); // Clear everything
  uint8_t numChannels = NUM_CHANNELS_CYTON;
  uint8_t numPackets = 1;
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

  DynamicJsonBuffer jsonBuffer(wifi.getJSONBufferSize());
  JsonObject& root = jsonBuffer.createObject();
  wifi.getJSONFromSamples(root, numChannels, numPackets);

  JsonObject& chunk0 = root["chunk"][0];
  double actual_timestamp = chunk0["timestamp"]; // 1500916934017000
  test.assertEqual((unsigned long long)actual_timestamp, expected_timestamp, "should be able to set timestamp", __LINE__);

  test.assertFalse(root.containsKey(JSON_SAMPLE_NUMBER), "should not have sample numbers in json", __LINE__);

  JsonArray& chunk0_data = chunk0["data"];
  for (int i = 0; i < numChannels; i++) {
    double chunk0_tempData = chunk0_data[i];
    test.assertApproximately(chunk0_data[i], expected_channelData[i], 1.0, "should be able to code large numbers", __LINE__);
  }
}

void testGetJSONFromSamplesCytonDaisyMax() {
  // Cyton Daisy with three packets
  wifi.reset(); // Clear everything
  wifi.jsonHasSampleNumbers = true;
  uint8_t expected_sampleNumber = 29;
  unsigned long long expected_timestamp = 1500916934017000;
  double expected_channelData[MAX_CHANNELS];
  uint8_t numChannels = NUM_CHANNELS_CYTON_DAISY;
  wifi.setNumChannels(numChannels);
  uint8_t numPackets = wifi.getJSONMaxPackets(numChannels);
  for (uint8_t i = 0; i < numPackets; i++) {
    wifi.sampleReset(wifi.sampleBuffer + i);
    (wifi.sampleBuffer + i)->sampleNumber = expected_sampleNumber + i;
    (wifi.sampleBuffer + i)->timestamp = expected_timestamp + i;
    for (uint8_t j = 0; j < numChannels; j++) {
      (wifi.sampleBuffer + i)->channelData[j] = expected_channelData[j];
      // Serial.printf("j:%d: i:%d %.0f\n", j, i, (wifi.sampleBuffer + i)->channelData[j]);
    }
  }

  DynamicJsonBuffer jsonBuffer(wifi.getJSONBufferSize());
  JsonObject& root = jsonBuffer.createObject();
  wifi.getJSONFromSamples(root, numChannels, numPackets);

  JsonArray& chunk = root["chunk"];

  for (uint8_t i = 0; i < numPackets; i++) {
    JsonObject& sample = chunk[i];
    unsigned long long actual_timestamp = sample["timestamp"]; // 1500916934017000
    test.assertEqual(actual_timestamp, expected_timestamp + i, String("should be able to set timestamp for " + String(i)).c_str(), __LINE__);

    uint8_t actual_sampleNumber = sample["sampleNumber"]; // 255
    test.assertEqual(actual_sampleNumber, expected_sampleNumber + i, String("cyton daisy multi packet should be able to set sampleNumber for " + String(i)).c_str(), __LINE__);

    JsonArray& sample_data = sample["data"];
    for (uint8_t j = 0; j < numChannels; j++) {
      double temp_d = sample_data[j];
      // Serial.printf("\nj[%d]: td:%0.0f e_cD:%.0f\n", j, temp_d, expected_channelData[j]-i);
      test.assertApproximately(temp_d, expected_channelData[j], 1.0, String("should be able to code large numbers "+String(j)+" for sample " + String(i)).c_str(), __LINE__);
    }
  }
}

void testGetJSONFromSamplesCytonDaisySingle() {
  test.it("should work to get JSON from samples Cyton Daisy");

  wifi.reset(); // Clear everything
  uint8_t numChannels = NUM_CHANNELS_CYTON_DAISY;
  wifi.setNumChannels(numChannels);
  uint8_t numPackets = 1;
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

  DynamicJsonBuffer jsonBuffer(wifi.getJSONBufferSize());
  JsonObject& root = jsonBuffer.createObject();
  wifi.getJSONFromSamples(root, numChannels, numPackets);

  JsonObject& chunk0 = root["chunk"][0];
  double actual_timestamp = chunk0["timestamp"]; // 1500916934017000
  test.assertEqual((unsigned long long)actual_timestamp, expected_timestamp, "daisy should be able to set timestamp", __LINE__);

  test.assertFalse(root.containsKey(JSON_SAMPLE_NUMBER), "should not have sample numbers in json", __LINE__);

  JsonArray& chunk0_data = chunk0["data"];
  for (int i = 0; i < numChannels; i++) {
    double chunk0_tempData = chunk0_data[i];
    test.assertApproximately(chunk0_data[i], expected_channelData[i], 1.0, "daisy should be able to code large numbers", __LINE__);
  }
}

void testGetJSONFromSamplesGanglionMax() {
  // Ganglion with eight packets
  wifi.reset(); // Clear everything
  wifi.jsonHasSampleNumbers = true;
  uint8_t expected_sampleNumber = 29;
  unsigned long long expected_timestamp = 1500916934017000;
  double expected_channelData[MAX_CHANNELS];
  uint8_t numChannels = NUM_CHANNELS_GANGLION;
  wifi.setNumChannels(numChannels);
  uint8_t numPackets = wifi.getJSONMaxPackets(numChannels);
  for (uint8_t i = 0; i < numPackets; i++) {
    wifi.sampleReset(wifi.sampleBuffer + i);
    (wifi.sampleBuffer + i)->sampleNumber = expected_sampleNumber + i;
    (wifi.sampleBuffer + i)->timestamp = expected_timestamp + i;
    for (uint8_t j = 0; j < numChannels; j++) {
      (wifi.sampleBuffer + i)->channelData[j] = expected_channelData[j];
      // Serial.printf("j:%d: i:%d %.0f\n", j, i, (wifi.sampleBuffer + i)->channelData[j]);
    }
  }

  DynamicJsonBuffer jsonBuffer(wifi.getJSONBufferSize());
  JsonObject& root = jsonBuffer.createObject();
  wifi.getJSONFromSamples(root, numChannels, numPackets);

  JsonArray& chunk = root["chunk"];
  for (uint8_t i = 0; i < numPackets; i++) {
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

void testGetJSONFromSamplesGanglionSingle() {
  test.it("should work to get JSON from samples Ganglion");

  wifi.reset(); // Clear everything
  uint8_t numChannels = NUM_CHANNELS_GANGLION;
  wifi.setNumChannels(numChannels);
  uint8_t numPackets = 1;
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

  DynamicJsonBuffer jsonBuffer(wifi.getJSONBufferSize());
  JsonObject& root = jsonBuffer.createObject();
  wifi.getJSONFromSamples(root, numChannels, numPackets);

  JsonObject& chunk0 = root["chunk"][0];
  double actual_timestamp = chunk0["timestamp"]; // 1500916934017000
  test.assertEqual((unsigned long long)actual_timestamp, expected_timestamp, "ganglion should be able to set timestamp", __LINE__);

  test.assertFalse(root.containsKey(JSON_SAMPLE_NUMBER), "should not have sample numbers in json", __LINE__);

  JsonArray& chunk0_data = chunk0["data"];
  for (int i = 0; i < numChannels; i++) {
    double chunk0_tempData = chunk0_data[i];
    test.assertApproximately(chunk0_data[i], expected_channelData[i], 1.0, "ganglion should be able to code large numbers", __LINE__);
  }
}

void testGetJSONFromSamplesNoTimestamp() {
  test.it("should work to get a single JSON from samples with no timestamp Cyton");

  wifi.reset(); // Clear everything
  wifi.jsonHasTimeStamps = false;
  uint8_t numChannels = NUM_CHANNELS_CYTON;
  uint8_t numPackets = 1;
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

  DynamicJsonBuffer jsonBuffer(wifi.getJSONBufferSize());
  JsonObject& root = jsonBuffer.createObject();
  wifi.getJSONFromSamples(root, numChannels, numPackets);

  test.assertFalse(root.containsKey("timestamp"), "should not have timestamp in json", __LINE__);

  uint8_t actual_sampleNumber = root["sampleNumber"]; // 255
  test.assertEqual(actual_sampleNumber, expected_sampleNumber, "should be able to set sampleNumber", __LINE__);

  JsonArray& chunk0_data = root["data"];
  for (int i = 0; i < numChannels; i++) {
    double chunk0_tempData = chunk0_data[i];
    test.assertApproximately(chunk0_data[i], expected_channelData[i], 1.0, "should be able to code large numbers", __LINE__);
  }
}

void testGetJSONFromSamples() {
  test.detail("getJSONFromSamples");
  testGetJSONFromSamplesCytonSingle();
  testGetJSONFromSamplesCytonMax();
  delay(100);
  testGetJSONFromSamplesCytonDaisySingle();
  testGetJSONFromSamplesCytonDaisyMax();
  delay(100);
  testGetJSONFromSamplesGanglionSingle();
  testGetJSONFromSamplesGanglionMax();
  delay(100);
}

void testGetJSONMaxPackets() {
  test.describe("getJSONMaxPackets");

  test.assertEqual(wifi.getJSONMaxPackets(NUM_CHANNELS_GANGLION), 10, "should get the correct number for packets for ganglion", __LINE__);
  test.assertEqual(wifi.getJSONMaxPackets(NUM_CHANNELS_CYTON), 10, "should get the correct number for packets for cyton", __LINE__);
  test.assertEqual(wifi.getJSONMaxPackets(NUM_CHANNELS_CYTON_DAISY), 6, "should get the correct number for packets for daisy cyton", __LINE__);
}

void testGetJSON() {
  test.describe("getJSON");
  testGetJSONAdditionalBytes();
  testGetJSONBufferSize();
  testGetJSONFromSamples();
  testGetJSONMaxPackets();
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

void testGetMacLastFourBytes() {
  test.describe("getMacLastFourBytes");

  test.assertEqual((int)wifi.getMacLastFourBytes().length(), 4, "mac should be of length of 4", __LINE__);
}

void testGetMac() {
  test.describe("getMac");

  test.assertEqual((int)wifi.getMac().length(), 17, "length of mac address should be 14", __LINE__);
}

void testGetModelNumber() {
  test.describe("getModelNumber");

  test.assertTrue(wifi.getModelNumber().startsWith("PTW-0001-"), "should be able to get model number", __LINE__);
  test.assertEqual((int)wifi.getModelNumber().length(), 13, "length of model number should be 14", __LINE__);
}

void testGetName() {
  test.describe("getName");

  test.assertTrue(wifi.getName().startsWith("OpenBCI-"), "should be able to get name", __LINE__);
  test.assertEqual((int)wifi.getName().length(), 12, "length of name should be 12", __LINE__);
}

void testGetOutputModeString() {
  test.describe("getOutputModeString");
  wifi.reset();
  // RAW
  test.assertEqual(wifi.getOutputModeString(wifi.OUTPUT_MODE_RAW), "raw", "Should have gotten 'raw' string",__LINE__);

  // JSON
  test.assertEqual(wifi.getOutputModeString(wifi.OUTPUT_MODE_JSON), "json", "Should have gotten 'json' string",__LINE__);
}

void testGetOutputProtocolString() {
  test.describe("getOutputProtocolString");
  wifi.reset();

  // NONE
  test.assertEqual(wifi.getOutputProtocolString(wifi.OUTPUT_PROTOCOL_NONE), "none", "Should have gotten 'none' string",__LINE__);

  // TCP
  test.assertEqual(wifi.getOutputProtocolString(wifi.OUTPUT_PROTOCOL_TCP), "tcp", "Should have gotten 'tcp' string",__LINE__);

  // MQTT
  test.assertEqual(wifi.getOutputProtocolString(wifi.OUTPUT_PROTOCOL_MQTT), "mqtt", "Should have gotten 'mqtt' string",__LINE__);

  // Serial
  test.assertEqual(wifi.getOutputProtocolString(wifi.OUTPUT_PROTOCOL_SERIAL), "serial", "Should have gotten 'serial' string",__LINE__);

  // Web sockets
  test.assertEqual(wifi.getOutputProtocolString(wifi.OUTPUT_PROTOCOL_WEB_SOCKETS), "ws", "Should have gotten 'ws' string",__LINE__);
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
  testGetJSON();
  testGetLatency();
  testGetMacLastFourBytes();
  testGetMac();
  testGetModelNumber();
  testGetName();
  testGetOutputModeString();
  testGetOutputProtocolString();
  testGetStringLLNumber();
  testGetScaleFactorVoltsCyton();
  testGetScaleFactorVoltsGanglion();
}

///////////////////////////
// SETTERS ////////////////
///////////////////////////

void testReset() {
  test.describe("reset");

  wifi.setNTPOffset(123456);
  wifi.setLatency(1234);
  wifi.setNumChannels(NUM_CHANNELS_CYTON_DAISY);

  wifi.reset();
  test.assertFalse(wifi.tcpDelimiter, "should set tcpDelimiter to false", __LINE__);
  test.assertFalse(wifi.clientWaitingForResponse, "should set clientWaitingForResponse to false", __LINE__);
  test.assertFalse(wifi.clientWaitingForResponseFullfilled, "should set clientWaitingForResponseFullfilled to false", __LINE__);
  test.assertTrue(wifi.curRawBuffer == wifi.rawBuffer, "should point cur raw buffer to head of buffer", __LINE__);
  test.assertEqual(wifi.curOutputMode, wifi.OUTPUT_MODE_RAW, "should initialize to 'raw' output mode", __LINE__);
  test.assertEqual(wifi.curOutputProtocol, wifi.OUTPUT_PROTOCOL_NONE, "should initialize 'none' for output protocol", __LINE__);
  test.assertEqual(wifi.lastTimeWasPolled, (unsigned long)0, "should initialize 'lastTimeWasPolled' for 0", __LINE__);
  test.assertFalse(wifi.jsonHasSampleNumbers, "should not have sample numbers by default in JSON");
  test.assertTrue(wifi.jsonHasTimeStamps, "should have timestamps by default in JSON");
  test.assertEqual(wifi.mqttBrokerAddress, "", "should initialize mqttBrokerAddress to empty string", __LINE__);
  test.assertEqual(wifi.mqttUsername, "", "should initialize mqttUsername to empty string", __LINE__);
  test.assertEqual(wifi.mqttPassword, "", "should initialize mqttPassword to empty string", __LINE__);
  test.assertEqual(wifi.mqttPort, DEFAULT_MQTT_PORT, "should initialize mqtt port to 1883", __LINE__);
  test.assertEqual(wifi.outputString, "", "should initialize outputString to empty string", __LINE__);
  test.assertEqual(wifi.passthroughPosition, 0, "should initialize the passthroughPosition to 0", __LINE__);
  test.assertEqual(wifi.tcpAddress.toString(), "0.0.0.0", "should initialize tcpAddress to empty string", __LINE__);
  test.assertFalse(wifi.tcpDelimiter, "should initialize tcpDelimiter to false", __LINE__);
  test.assertEqual(wifi.tcpPort, 80, "should initialize tcpPort to 80", __LINE__);
  test.assertEqual(wifi.timePassthroughBufferLoaded, (unsigned long)0, "should initialize the timeOfWifiTXBufferLoaded to 0");
  test.assertFalse(wifi.passthroughBufferLoaded, "should initialize the passthroughBuffer loaded", __LINE__);
  test.assertEqual(wifi.getHead(), 0, "should reset head to 0", __LINE__);
  test.assertEqual(wifi.getJSONBufferSize(), (size_t)2556, "should reset jsonBufferSize to size with 8 channels", __LINE__);
  test.assertEqual(wifi.getLatency(), (unsigned long)DEFAULT_LATENCY, "should reset latency to default", __LINE__);
  test.assertEqual(wifi.getNTPOffset(), (unsigned long)0, "should reset ntpOffset to 0", __LINE__);
  test.assertEqual(wifi.getNumChannels(), 0, "should set numChannels to 0", __LINE__);
  test.assertEqual(wifi.getTail(), 0, "should reset tail to 0", __LINE__);
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

void testSetInfoMQTT() {
  test.detail("MQTT");

  String expected_brokerAddress = "mock.getcloudbrain.com";
  String expected_username = "/a253c7a141daca0dc6bfe5f51bee7ef5f1ca4b9cb9807ff0ea1f1737f771f573:a253c7a141daca0dc6bfe5f51bee7ef5f1ca4b9cb9807ff0ea1f1737f771f573";
  String expected_password = "the password is not password";
  int expected_port = 883;

  wifi.setInfoMQTT(expected_brokerAddress, expected_username, expected_password, expected_port);
  test.assertEqual((int)wifi.curOutputProtocol, (int)wifi.OUTPUT_PROTOCOL_MQTT, "should have set the output protocol to MQTT", __LINE__);
  test.assertEqual(wifi.mqttBrokerAddress, expected_brokerAddress, "should set mqttBrokerAddress", __LINE__);
  test.assertEqual(wifi.mqttUsername, expected_username, "should be able to set mqttUsername", __LINE__);
  test.assertEqual(wifi.mqttPassword, expected_password, "should be able to set mqttPassword", __LINE__);
  test.assertEqual(wifi.mqttPort, (int)expected_port, "should be able to set mqttPort", __LINE__);
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
  testSetInfoMQTT();
  testSetInfoTCP();
}

void testSetLatency() {
  test.describe("setLatency");

  wifi.reset();

  unsigned long expected_latency = 5000;
  wifi.setLatency(expected_latency);

  test.assertEqual(wifi.getLatency(), expected_latency, "should set the new latency", __LINE__);
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

void testSetOutputMode() {
  test.describe("setOutputMode");

  wifi.reset();

  test.assertEqual(wifi.curOutputMode, wifi.OUTPUT_MODE_RAW, "should have 'raw' mode initially", __LINE__);

  test.it("should be able to switch to json output mode");
  wifi.setOutputMode(wifi.OUTPUT_MODE_JSON);
  test.assertEqual(wifi.curOutputMode, wifi.OUTPUT_MODE_JSON, "should have 'json' mode", __LINE__);

  test.it("should be able to switch to raw output mode");
  wifi.setOutputMode(wifi.OUTPUT_MODE_RAW);
  test.assertEqual(wifi.curOutputMode, wifi.OUTPUT_MODE_RAW, "should have 'raw' mode", __LINE__);
}

void testSetOutputProtocol() {
  test.describe("setOutputProtocol");

  wifi.reset();

  test.assertEqual(wifi.curOutputProtocol, wifi.OUTPUT_PROTOCOL_NONE, "should have 'none' protocol initially", __LINE__);

  test.it("should be able to switch to tcp output protocol");
  wifi.setOutputProtocol(wifi.OUTPUT_PROTOCOL_TCP);
  test.assertEqual(wifi.curOutputProtocol, wifi.OUTPUT_PROTOCOL_TCP, "should have 'tcp' protocol", __LINE__);

  test.it("should be able to switch to mqtt output protocol");
  wifi.setOutputProtocol(wifi.OUTPUT_PROTOCOL_MQTT);
  test.assertEqual(wifi.curOutputProtocol, wifi.OUTPUT_PROTOCOL_MQTT, "should have 'mqtt' protocol", __LINE__);

  test.it("should be able to switch to web sockets output protocol");
  wifi.setOutputProtocol(wifi.OUTPUT_PROTOCOL_WEB_SOCKETS);
  test.assertEqual(wifi.curOutputProtocol, wifi.OUTPUT_PROTOCOL_WEB_SOCKETS, "should have 'ws' protocol", __LINE__);

  test.it("should be able to switch to serial output protocol");
  wifi.setOutputProtocol(wifi.OUTPUT_PROTOCOL_SERIAL);
  test.assertEqual(wifi.curOutputProtocol, wifi.OUTPUT_PROTOCOL_SERIAL, "should have 'serial' protocol", __LINE__);
}

void testSetters() {
  testReset();
  testSetGain();
  testSetInfo();
  testSetLatency();
  testSetNumChannels();
  testSetNTPOffset();
  testSetOutputMode();
  testSetOutputProtocol();
}

void testChannelDataComputeCyton() {
  test.detail("Cyton");

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
  test.it("should be able to compute and build a sample object");
  wifi.channelDataCompute(arr, gains, wifi.sampleBuffer, packetOffset, numChannels);
  for (uint8_t i = 0; i < numChannels; i++) {
    test.assertApproximately(wifi.sampleBuffer->channelData[i], expected_channelData[i], 1.0, "should compute data for cyton", __LINE__);
  }
  test.assertEqualHex(wifi.sampleBuffer->sampleNumber, expected_sampleNumber, "should be able to extract the sample number", __LINE__);
  test.assertGreaterThan(wifi.sampleBuffer->timestamp, (unsigned long long)0, "should be able to set the timestamp", __LINE__);
}

void testChannelDataComputeDaisy() {
  test.detail("Cyton Daisy");

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
  test.it("should be able to build a daisy sample");
  wifi.channelDataCompute(arr, gains, wifi.sampleBuffer, packetOffset, numChannels);
  test.assertEqualHex(wifi.sampleBuffer->sampleNumber, expected_sampleNumber, "should be able to extract the sample number", __LINE__);
  test.assertGreaterThan(wifi.sampleBuffer->timestamp, (uint64_t)0, "should be able to set the timestamp", __LINE__);

  unsigned long long firstPacketTime = wifi.sampleBuffer->timestamp;
  packetOffset = MAX_CHANNELS_PER_PACKET;
  for (int i = 0; i < MAX_CHANNELS_PER_PACKET; i++) {
    gains[i + packetOffset] = 24;
    int32_t raww = wifi.int24To32(arr + (i*3)+2);
    expected_channelData[i + packetOffset] = wifi.rawToScaled(raww, wifi.getScaleFactorVoltsCyton(gains[i + packetOffset]));
  }
  // for (uint8_t i = 0; i < numChannels; i++) {
  //   Serial.printf("[%d] gain: %d | e_cD: %0.8f\n", i, gains[i], expected_channelData[i]);
  // }
  wifi.channelDataCompute(arr, gains, wifi.sampleBuffer, packetOffset, numChannels);
  // for (uint8_t i = 0; i < numChannels; i++) {
  //   Serial.printf("[%d] gain: %d | e_cD: %0.8f | output: %.0f\n", i, gains[i], expected_channelData[i], wifi.sampleBuffer->channelData[i]);
  // }
  // Serial.printf("packetOffset: %d\n", packetOffset);
  for (uint8_t i = 0; i < numChannels; i++) {
    if (i >= packetOffset) {
      test.assertApproximately(wifi.sampleBuffer->channelData[i], expected_channelData[i], 1.0, "should get data for the upper 8 channels of daisy", __LINE__);
    } else {
      test.assertApproximately(wifi.sampleBuffer->channelData[i], expected_channelData[i], 1.0, "should get data for lower 8 channels of daisy", __LINE__);
    }
  }
  test.assertEqualHex(wifi.sampleBuffer->sampleNumber, expected_sampleNumber, "should still have valid sample number", __LINE__);
  test.assertGreaterThan(wifi.sampleBuffer->timestamp, (uint64_t)0, "should be able to set the timestamp", __LINE__);
}

void testChannelDataComputeGanglion() {
  test.detail("Ganglion");

  uint8_t numChannels = NUM_CHANNELS_GANGLION;
  uint8_t packetOffset = 0;
  uint8_t expected_sampleNumber = 20;
  uint8_t arr[BYTES_PER_SPI_PACKET];
  double expected_channelData[MAX_CHANNELS_PER_PACKET];
  uint8_t index = 0;
  for (int i = 0; i < 24; i++) {
    arr[i+2] = 0;
    if (i%3==2) {
      arr[i+2]=index+1;
      int32_t raww = wifi.int24To32(arr + (index*3)+2);
      // Serial.printf("raww: %d\n", raww);
      expected_channelData[index] = wifi.rawToScaled(raww, wifi.getScaleFactorVoltsGanglion());
      index++;
    }
    // Serial.printf("i: %d | arr[%d]: %d\n", i, i+2, arr[i+2]);

  }
  // for (uint8_t i = 0; i < 24; i++) {
    // Serial.printf("arr[%d]: %d\n", i+2, arr[i+2]);
  // }
  arr[1] = expected_sampleNumber;
  test.it("should be able to create a ganglion sample object");

  wifi.sampleReset(wifi.sampleBuffer);

  // for (uint8_t i = 0; i < numChannels; i++) {
    // Serial.printf("[%d] e_cD: %0.8f\n", i, expected_channelData[i]);
  // }
  uint8_t gains[numChannels];
  wifi.channelDataCompute(arr, gains, wifi.sampleBuffer, packetOffset, numChannels);
  // for (uint8_t i = 0; i < numChannels; i++) {
    // Serial.printf("[%d] e_cD: %0.8f | output: %.0f\n", i, expected_channelData[i], wifi.sampleBuffer->channelData[i]);
  // }
  // Serial.printf("packetOffset: %d\n", packetOffset);

  for (uint8_t i = 0; i < numChannels; i++) {
    test.assertApproximately(wifi.sampleBuffer->channelData[i], expected_channelData[i], 1.0, "should get data for 4 channels of ganglion", __LINE__);
  }
  test.assertEqualHex(wifi.sampleBuffer->sampleNumber, expected_sampleNumber, "should be able to extract the sample number", __LINE__);
  test.assertGreaterThan(wifi.sampleBuffer->timestamp, (uint64_t)0, "should be able to set the timestamp", __LINE__);
}

void testChannelDataCompute() {
  test.describe("channelDataCompute");
  testChannelDataComputeCyton();
  testChannelDataComputeDaisy();
  testChannelDataComputeGanglion();
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

void testIsAStreamByte() {
  test.describe("isAStreamByte");

  test.assertTrue(wifi.isAStreamByte(0xC0), "should find a stream byte", __LINE__);
  test.assertTrue(wifi.isAStreamByte(0xC5), "should find a stream byte", __LINE__);
  test.assertFalse(wifi.isAStreamByte(0x05), "should not find a stream byte", __LINE__);
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
    // Serial.printf("[%d] raw: %d | scale: %.10f | output: %.1f\n", i, raw[i], wifi.getScaleFactorVoltsCyton(gains[i]), expected_scaledOutput[i]);

  }

  // Do Cyton first
  test.it("should be able to work for cyton");
  wifi.transformRawsToScaledCyton(raw, gains, 0, actual_scaledOutput);

  for (uint8_t i = 0; i < NUM_CHANNELS_CYTON; i++) {
    test.assertApproximately(actual_scaledOutput[i], expected_scaledOutput[i], 1.0, "should be able to transform raws to scaled for cyton", __LINE__);
    // actual_scaledOutput[i] = 0.0; // clear
  }

  // Then daisy
  test.it("should be able to work for cyton daisy");
  wifi.transformRawsToScaledCyton(raw+MAX_CHANNELS_PER_PACKET, gains, MAX_CHANNELS_PER_PACKET, actual_scaledOutput);
  // for (uint8_t i = 0; i < NUM_CHANNELS_CYTON_DAISY; i++) {
  //   Serial.printf("[%d] actual_scaledOutput[%d]: %.10f | expected_scaledOutput[%d]: %.10f\n", i, i, actual_scaledOutput[i], i, expected_scaledOutput[i]);
  // }
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

  test.it("should be able to reset the given sample");
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

  test.it("should reset all samples");
  for (int i = 0; i < NUM_PACKETS_IN_RING_BUFFER_JSON; i++) {
    (wifi.sampleBuffer + i)->timestamp = i;
    (wifi.sampleBuffer + i)->sampleNumber = i;
    for (int i = 0; i < numChannels; i++) {
      (wifi.sampleBuffer + i)->channelData[i] = 1099.0 + i;
    }
  }
  wifi.sampleReset();

  for (int i = 0; i < NUM_PACKETS_IN_RING_BUFFER_JSON; i++) {
    boolean isAllClear = true;
    if ((wifi.sampleBuffer + i)->timestamp > 0) {
      isAllClear = false;
    }
    if ((wifi.sampleBuffer + i)->sampleNumber > 0) {
      isAllClear = false;
    }
    for (int j = 0; j < numChannels; j++) {
      if ((wifi.sampleBuffer + i)->channelData[j] > 0.001 || (wifi.sampleBuffer + i)->channelData[j] < -0.001) {
        isAllClear = false;
      }
    }
    if (!isAllClear) {
      test.assertBoolean(isAllClear, true, "should have cleared all values to 0(", __LINE__);
    }
  }
}

void testUtilisForJSON() {
  testChannelDataCompute();
  testExtractRaws();
  testInt24To32();
  testIsAStreamByte();
  testTransformRawsToScaledCyton();
  testTransformRawsToScaledGanglion();
  testSampleReset();
  testRawToScaled();
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
  wifi.rawBufferReset(wifi.rawBuffer);
  wifi.rawBufferClean(wifi.rawBuffer);
  wifi.rawBufferReset(wifi.rawBuffer + 1);
  wifi.rawBufferClean(wifi.rawBuffer + 1);
  wifi.curRawBuffer = wifi.rawBuffer;
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
  test.assertTrue(wifi.rawBufferAddStreamPacket(wifi.curRawBuffer, buffer), "should be able to add buffer to radioBuf", __LINE__);
  test.assertFalse(wifi.curRawBuffer->gotAllPackets, "should not have all the packets", __LINE__);
  test.assertEqual(wifi.curRawBuffer->positionWrite, expectedLength, "should move positionWrite by 33", __LINE__);
  test.assertEqualBuffer(wifi.curRawBuffer->data, expected_buffer, expectedLength, "should add the whole buffer", __LINE__);

  // Reset buffer
  wifi.curRawBuffer->positionWrite = 0;

  // Test how this will work in normal operations, i.e. ignoring the byte id
  test.assertTrue(wifi.rawBufferAddStreamPacket(wifi.curRawBuffer, buffer),"should be able to add buffer to radioBuf", __LINE__);
  test.assertFalse(wifi.curRawBuffer->gotAllPackets, "should be still have room", __LINE__);
  test.assertEqual(wifi.curRawBuffer->positionWrite , expectedLength,"should set the positionWrite to 33", __LINE__);
  test.assertEqualBuffer(wifi.curRawBuffer->data, expected_buffer, expectedLength,"should add the whole buffer", __LINE__);
}

void testRawBufferClean() {
  test.describe("rawBufferClean");
  testRawBufferCleanUp();
  for (int i = 0; i < BYTES_PER_RAW_BUFFER; i++) {
    wifi.curRawBuffer->data[i] = 1;
  }

  // Call the function under test
  wifi.rawBufferClean(wifi.curRawBuffer);

  test.it("should be able to initialize the raw buffer to all zeros");
  // Should fill the array with all zeros
  boolean allZeros = true;
  for (int j = 0; j < BYTES_PER_RAW_BUFFER; j++) {
    if (wifi.curRawBuffer->data[j] != 0) {
      allZeros = false;
    }
  }
  test.assertTrue(allZeros, "should set all values to zero", __LINE__);
}

void testRawBufferHasData() {
  test.describe("rawBufferHasData");
  testRawBufferCleanUp();
  wifi.curRawBuffer->positionWrite = 0;

  // Don't add any data
  test.assertBoolean(wifi.rawBufferHasData(wifi.curRawBuffer),false,"should have no data at first", __LINE__);
  // Add some data
  wifi.curRawBuffer->positionWrite = 69;
  // Verify!
  test.assertBoolean(wifi.rawBufferHasData(wifi.curRawBuffer),true,"should have data after moving positionWrite", __LINE__);
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
  test.assertFalse(wifi.curRawBuffer->gotAllPackets, "should leave gotAllPackets to false", __LINE__);
  test.assertEqual(wifi.curRawBuffer->positionWrite, BYTES_PER_OBCI_PACKET, "should set the positionWrite to 33", __LINE__);
  test.assertEqualBuffer(wifi.curRawBuffer->data, expected_buffer, BYTES_PER_OBCI_PACKET, "should have the packet loaded into the first buffer", __LINE__);

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
    // Serial.printf("[%d]: | retVal: %d | pos: %d\n", i, wifi.rawBufferProcessPacket(bufferRaw, BYTES_PER_OBCI_PACKET), wifi.curRawBuffer->positionWrite);
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

  test.assertFalse(wifi.curRawBuffer->gotAllPackets, "should set got all packets false on curRawBuffer", __LINE__);
  test.assertEqual(wifi.curRawBuffer->positionWrite, BYTES_PER_OBCI_PACKET, "should set positionWrite of curRawBuffer to that of the second buffer", __LINE__);
  test.assertEqualBuffer(wifi.curRawBuffer->data, expected_buffer, BYTES_PER_OBCI_PACKET, "should have loaded cali buffer into the buffer curRawBuffer points to", __LINE__);


  // Do it again in reverse, where the second buffer is full
  // So clear the first buffer and point to the second
  test.it("should switch to first buffer when second buffer is full and id last packet");
  testRawBufferCleanUp();
  wifi.curRawBuffer = wifi.rawBuffer + 1;
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

  // Verify that both of the buffers are full
  test.assertBoolean((wifi.rawBuffer + 1)->gotAllPackets, true, "should still have a full first buffer after switch", __LINE__);
  test.assertEqual((wifi.rawBuffer + 1)->positionWrite,BYTES_PER_RAW_BUFFER,"first buffer should still have correct size", __LINE__);

  test.assertBoolean(wifi.curRawBuffer->gotAllPackets, false, "should set got all packets false on curRawBuffer", __LINE__);
  test.assertEqual(wifi.curRawBuffer->positionWrite, BYTES_PER_OBCI_PACKET, "should set positionWrite of curRawBuffer to that of the second buffer", __LINE__);
  test.assertEqualBuffer(wifi.curRawBuffer->data, expected_buffer, BYTES_PER_OBCI_PACKET, "should have loaded cali buffer into the buffer curRawBuffer points to", __LINE__);

  test.it("should switch to second buffer when first is flushing");
  // First buffer flushing, second empty
  testRawBufferCleanUp();
  test.assertEqualHex(wifi.rawBufferProcessPacket(bufferRaw), PROCESS_RAW_PASS_FIRST);
  test.assertFalse(wifi.rawBuffer->gotAllPackets, "should set gotAllPackets to false for first buffer", __LINE__);
  test.assertEqualBuffer(wifi.rawBuffer->data, expected_buffer, BYTES_PER_OBCI_PACKET, "should have loaded data buffer in the first buffer correctly", __LINE__);
  test.assertEqualBuffer(wifi.curRawBuffer->data, expected_buffer, BYTES_PER_OBCI_PACKET, "should have loaded cali buffer into the buffer curRawBuffer points to", __LINE__);
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
  test.assertEqualBuffer(wifi.curRawBuffer->data, expected_buffer, BYTES_PER_OBCI_PACKET, "should have the taco buffer loaded into curRawBuffer", __LINE__);
  // Verify the first buffer is still loaded with the cali buffer
  test.assertTrue(wifi.rawBuffer->flushing, "should have flushing true for first buffer", __LINE__);
  test.assertFalse(wifi.rawBuffer->gotAllPackets, "should still have gotAllPackets false for first buffer", __LINE__);
  test.assertEqual(wifi.rawBuffer->positionWrite, BYTES_PER_OBCI_PACKET,"should still have positionWrite to size of cali buffer in buffer 1", __LINE__);
  test.assertEqualBuffer(wifi.rawBuffer->data, expected_buffer, BYTES_PER_OBCI_PACKET, "should still have loaded cali buffer in the first buffer correctly", __LINE__);

  test.it("should switch to first buffer when second is flushing and id last packet");
  // Second buffer flushing, first empty
  testRawBufferCleanUp();
  // Load the cali buffer into the second buffer
  wifi.curRawBuffer = wifi.rawBuffer + 1;
  test.assertEqualHex(wifi.rawBufferProcessPacket(bufferRaw), PROCESS_RAW_PASS_FIRST);
  test.assertFalse((wifi.rawBuffer + 1)->gotAllPackets, "should set gotAllPackets to false for first buffer", __LINE__);
  test.assertEqualBuffer((wifi.rawBuffer + 1)->data, expected_buffer, BYTES_PER_OBCI_PACKET, "should have loaded data buffer in the first buffer correctly", __LINE__);
  test.assertEqualBuffer(wifi.curRawBuffer->data, expected_buffer, BYTES_PER_OBCI_PACKET, "should have loaded cali buffer into the buffer curRawBuffer points to", __LINE__);
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
  test.assertEqualBuffer(wifi.curRawBuffer->data, expected_buffer, BYTES_PER_OBCI_PACKET, "should have the taco buffer loaded into curRawBuffer", __LINE__);
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
  test.assertTrue(wifi.rawBufferReadyForNewPage(wifi.curRawBuffer), "should be ready to add new page in the curRawBuffer", __LINE__);

  // Add data to buffer 1
  test.it("not ready for a new page when data is in the buffer");
  wifi.rawBufferAddStreamPacket(wifi.curRawBuffer, bufferRaw);
  test.assertFalse(wifi.rawBufferReadyForNewPage(wifi.rawBuffer), "should not be ready to add new page in the first buffer", __LINE__);
  test.assertTrue(wifi.rawBufferReadyForNewPage(wifi.rawBuffer + 1), "should be ready to add new page in the second buffer", __LINE__);
  test.assertFalse(wifi.rawBufferReadyForNewPage(wifi.curRawBuffer), "should not be ready to add new page in the curRawBuffer", __LINE__);

  // Increment the curRawBuffer pointer
  // wifi.curRawBuffer++;
  // Clear the buffers
  // # CLEANUP
  testRawBufferCleanUp();

  // Add data to buffer 2
  //
  for (uint8_t i = 0; i < MAX_PACKETS_PER_SEND_TCP * 2; i++) {
    wifi.rawBufferProcessPacket(bufferRaw);
  }
  test.it("cannot add a page to either the first or second buffer when both are filled");
  wifi.rawBufferAddStreamPacket(wifi.curRawBuffer, bufferRaw);
  test.assertFalse(wifi.rawBufferReadyForNewPage(wifi.rawBuffer), "should not be ready to add new page in the first buffer", __LINE__);
  test.assertFalse(wifi.rawBufferReadyForNewPage(wifi.rawBuffer + 1),"should not be ready to add new page in the second buffer", __LINE__);
  test.assertFalse(wifi.rawBufferReadyForNewPage(wifi.curRawBuffer), "should not be ready to add new page in the curRawBuffer", __LINE__);

  // Clear the buffers
  // # CLEANUP
  testRawBufferCleanUp();

  // Mark first buffer as flushing
  test.it("cannot add a page to first buffer but can the second when flushing");
  wifi.rawBuffer->flushing = true;
  test.assertFalse(wifi.rawBufferReadyForNewPage(wifi.rawBuffer), "should not be ready to add new page in the first buffer", __LINE__);
  test.assertTrue(wifi.rawBufferReadyForNewPage(wifi.rawBuffer + 1), "should be ready to add new page in the second buffer", __LINE__);
  test.assertFalse(wifi.rawBufferReadyForNewPage(wifi.curRawBuffer), "should not be ready to add new page in the curRawBuffer", __LINE__);
  wifi.rawBuffer->flushing = false;
  // Mark second buffer as flushing
  test.it("cannot add a page to second buffer but can the first when flushing");
  (wifi.rawBuffer + 1)->flushing = true;
  test.assertTrue(wifi.rawBufferReadyForNewPage(wifi.rawBuffer), "should be ready to add new page in the first buffer", __LINE__);
  test.assertFalse(wifi.rawBufferReadyForNewPage(wifi.rawBuffer + 1), "should not be ready to add new page in the second buffer", __LINE__);
  test.assertTrue(wifi.rawBufferReadyForNewPage(wifi.curRawBuffer), "should be ready to add new page in the curRawBuffer", __LINE__);
  (wifi.rawBuffer + 1)->flushing = true;

  // Both flushing
  test.it("cannot add a page to either when both flushing");
  wifi.rawBuffer->flushing = true;
  (wifi.rawBuffer + 1)->flushing = true;
  test.assertFalse(wifi.rawBufferReadyForNewPage(wifi.rawBuffer), "should not be ready to add new page in the first buffer", __LINE__);
  test.assertFalse(wifi.rawBufferReadyForNewPage(wifi.rawBuffer + 1), "should not be ready to add new page in the second buffer", __LINE__);
  test.assertFalse(wifi.rawBufferReadyForNewPage(wifi.curRawBuffer), "should not be ready to add new page in the curRawBuffer", __LINE__);

  // # CLEANUP
  testRawBufferCleanUp();
}

void testRawBufferReset() {
  // Test the reset functions
  test.describe("rawBufferReset");
  wifi.reset();
  test.it("should reset the passed in raw buffer");
  wifi.curRawBuffer->flushing = true;
  wifi.curRawBuffer->gotAllPackets = true;
  wifi.curRawBuffer->positionWrite = 60;

  // Reset the flags
  wifi.rawBufferReset(wifi.curRawBuffer);

  // Verify they got Reset
  test.assertFalse(wifi.curRawBuffer->flushing, "should set flushing to false");
  test.assertFalse(wifi.curRawBuffer->gotAllPackets, "should set got all packets to false");
  test.assertEqual(wifi.curRawBuffer->positionWrite, 0, "should set positionWrite to 0");

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
  wifi.curRawBuffer = wifi.rawBuffer;
  test.assertTrue(wifi.rawBufferSwitchToOtherBuffer(), "can switch to other empty buffer", __LINE__);
  test.assertTrue(wifi.curRawBuffer == (wifi.rawBuffer + 1), "curRawBuffer points to second buffer", __LINE__);

  test.it("should return true if buffer 1 does not have data and should move the pointer");
  wifi.curRawBuffer = wifi.rawBuffer + 1;
  test.assertTrue(wifi.rawBufferSwitchToOtherBuffer(), "can switch to other empty buffer", __LINE__);
  test.assertTrue(wifi.curRawBuffer == wifi.rawBuffer, "curRawBuffer points to first buffer", __LINE__);

  // # CLEANUP
  testRawBufferCleanUp();

  test.it("should return false when currently pointed at buf 1 and buf 2 has data");
  wifi.curRawBuffer = wifi.rawBuffer;
  wifi.rawBufferAddStreamPacket(wifi.rawBuffer + 1, bufferRaw);
  test.assertFalse(wifi.rawBufferSwitchToOtherBuffer(), "cannot switch to buffer with data", __LINE__);
  test.assertTrue(wifi.curRawBuffer == wifi.rawBuffer, "curRawBuffer still points to first buffer", __LINE__);

  // # CLEANUP
  testRawBufferCleanUp();

  test.it("should return false when currently pointed at buf 2 and buf 1 has data");
  wifi.curRawBuffer = wifi.rawBuffer + 1;
  wifi.rawBufferAddStreamPacket(wifi.rawBuffer, bufferRaw);
  test.assertFalse(wifi.rawBufferSwitchToOtherBuffer(),"cannot switch to buffer with data", __LINE__);
  test.assertTrue(wifi.curRawBuffer == wifi.rawBuffer + 1, "curRawBuffer still points to second buffer", __LINE__);

  // # CLEANUP
  testRawBufferCleanUp();

  test.it("should return false when both buffers have data");
  wifi.rawBufferAddStreamPacket(wifi.rawBuffer, bufferRaw);
  wifi.rawBufferAddStreamPacket(wifi.rawBuffer + 1, bufferRaw);
  test.assertFalse(wifi.rawBufferSwitchToOtherBuffer(),"can't switch to second", __LINE__);
  wifi.curRawBuffer++;
  test.assertFalse(wifi.rawBufferSwitchToOtherBuffer(),"can't switch back to first", __LINE__);

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
  test.assertFalse(wifi.rawBufferSwitchToOtherBuffer(),"can't switch to any buffer", __LINE__);
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

void testSPIProcessPacketStreamJSONGanglion() {
  test.detail("JSON Ganglion");

  wifi.reset();

  wifi.setOutputMode(wifi.OUTPUT_MODE_JSON);

  test.it("for ganglion should add scaled data to sample struct");
  uint8_t expected_sampleNumber = 23; // Jordan
  uint8_t numChannels = NUM_CHANNELS_GANGLION;
  uint8_t data[BYTES_PER_SPI_PACKET];
  giveMeASPIStreamPacket(data, expected_sampleNumber);
  uint8_t gainPacket[BYTES_PER_SPI_PACKET];
  giveMeASPIPacketGainSet(gainPacket, numChannels);
  wifi.setGains(gainPacket);
  double expected_channelData[numChannels];
  for (int i = 0; i < numChannels; i++) {
    int32_t raww = wifi.int24To32(data + (i*3)+2);
    expected_channelData[i] = wifi.rawToScaled(raww, wifi.getScaleFactorVoltsGanglion());
  }
  wifi.spiProcessPacketStreamJSON(data);

  test.assertGreaterThan(wifi.sampleBuffer->timestamp, (unsigned long long)0, "should set timestamp greater than 0", __LINE__);
  test.assertEqual(wifi.sampleBuffer->sampleNumber, expected_sampleNumber, "should set the sample number", __LINE__);
  for (uint8_t i = 0; i < numChannels; i++) {
    test.assertApproximately(wifi.sampleBuffer->channelData[i], expected_channelData[i], 1.0, "should compute data for ganglion", __LINE__);
  }
}

void testSPIProcessPacketStreamJSONCyton() {
  test.detail("JSON Cyton");

  wifi.reset();

  wifi.setOutputMode(wifi.OUTPUT_MODE_JSON);

  test.it("for cyton should add scaled data to sample struct");
  uint8_t expected_sampleNumber = 23; // Jordan
  uint8_t numChannels = NUM_CHANNELS_CYTON;
  uint8_t data[BYTES_PER_SPI_PACKET];
  giveMeASPIStreamPacket(data, expected_sampleNumber);
  uint8_t gainPacket[BYTES_PER_SPI_PACKET];
  giveMeASPIPacketGainSet(gainPacket, numChannels);
  wifi.setGains(gainPacket);
  double expected_channelData[numChannels];
  for (int i = 0; i < numChannels; i++) {
    int32_t raww = wifi.int24To32(data + (i*3)+2);
    expected_channelData[i] = wifi.rawToScaled(raww, wifi.getScaleFactorVoltsCyton(wifi.getGains()[i]));
  }
  wifi.spiProcessPacketStreamJSON(data);

  test.assertGreaterThan(wifi.sampleBuffer->timestamp, (unsigned long long)0, "should set timestamp greater than 0", __LINE__);
  test.assertEqual(wifi.sampleBuffer->sampleNumber, expected_sampleNumber, "should set the sample number", __LINE__);
  for (uint8_t i = 0; i < numChannels; i++) {
    test.assertApproximately(wifi.sampleBuffer->channelData[i], expected_channelData[i], 1.0, "should compute data for cyton", __LINE__);
  }
}

void testSPIProcessPacketStreamJSONDaisy() {
  test.detail("JSON Cyton Daisy");

  wifi.reset();

  wifi.setOutputMode(wifi.OUTPUT_MODE_JSON);

  test.it("for cyton daisy should add scaled data to sample struct");
  uint8_t expected_sampleNumber = 23; // Jordan
  uint8_t numChannels = NUM_CHANNELS_CYTON_DAISY;
  uint8_t data[BYTES_PER_SPI_PACKET];
  giveMeASPIStreamPacket(data, expected_sampleNumber);
  uint8_t data_daisy[BYTES_PER_SPI_PACKET];
  giveMeASPIStreamPacket(data_daisy, expected_sampleNumber);
  uint8_t gainPacket[BYTES_PER_SPI_PACKET];
  giveMeASPIPacketGainSet(gainPacket, numChannels);
  wifi.setGains(gainPacket);
  double expected_channelData[numChannels];
  int32_t raws[MAX_CHANNELS_PER_PACKET];

  wifi.extractRaws(data_daisy + 2, raws, MAX_CHANNELS_PER_PACKET);
  for (int i = 0; i < MAX_CHANNELS_PER_PACKET; i++) {
    expected_channelData[i] = wifi.rawToScaled(raws[i], wifi.getScaleFactorVoltsCyton(wifi.getGains()[i]));
    expected_channelData[i+MAX_CHANNELS_PER_PACKET] = wifi.rawToScaled(raws[i], wifi.getScaleFactorVoltsCyton(wifi.getGains()[i+MAX_CHANNELS_PER_PACKET]));
  }
  wifi.spiProcessPacketStreamJSON(data);
  wifi.spiProcessPacketStreamJSON(data_daisy);

  test.assertGreaterThan(wifi.sampleBuffer->timestamp, (unsigned long long)0, "should set timestamp greater than 0", __LINE__);
  test.assertEqual(wifi.sampleBuffer->sampleNumber, expected_sampleNumber, "should set the sample number", __LINE__);
  for (uint8_t i = 0; i < numChannels; i++) {
    test.assertApproximately(wifi.sampleBuffer->channelData[i], expected_channelData[i], 1.0, "should compute data for cyton daisy", __LINE__);
  }
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

  test.assertEqualHex(wifi.curRawBuffer->data[0], STREAM_PACKET_BYTE_START, "should set first byte to start byte", __LINE__);
  test.assertEqualBuffer(wifi.curRawBuffer->data + 1, data + 1, BYTES_PER_SPI_PACKET-1, "should have the same 31 data bytes");
  test.assertEqualHex(wifi.curRawBuffer->data[BYTES_PER_SPI_PACKET], 0xC0, "should set the stop byte", __LINE__);

}

void testSPIProcessPacketStream() {
  test.describe("spiProcessPacketStream");
  testSPIProcessPacketStreamRaw();
  testSPIProcessPacketStreamJSONCyton();
  testSPIProcessPacketStreamJSONGanglion();
  testSPIProcessPacketStreamJSONDaisy();
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
  testUtilisForJSON();
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
  // testSetters();
  delay(500);
  digitalWrite(ledPin, HIGH);
  // testUtils();
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

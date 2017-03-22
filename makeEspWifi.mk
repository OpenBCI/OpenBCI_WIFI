# My makefile
LIBARAIES_DIR = $(HOME)/Documents/Arduino/libraries
SKETCH = $(LIBARAIES_DIR)/OpenBCI_Wifi/examples/ESP8266HuzzahMQTT/ESP8266HuzzahMQTT.ino

UPLOAD_PORT = /dev/cu.usbserial-A104JV88
ESP_ROOT = $(HOME)/esp8266
ESP_LIBS = $(ESP_ROOT)/libraries

LIBS = $(ESP_LIBS)/Wire $(ESP_LIBS)/ESP8266WiFi $(ESP_LIBS)/DNSServer $(ESP_LIBS)/ESP8266WebServer $(ESP_LIBS)/SPISlave $(LIBARAIES_DIR)/WiFiManager $(LIBARAIES_DIR)/PubSubClient

EXCLUDE_DIRS = $(LIBARAIES_DIR)/PubSubClient/tests

include $(HOME)/makeEspArduino/makeEspArduino.mk

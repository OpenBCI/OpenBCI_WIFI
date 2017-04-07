# My makefile
LIBARAIES_DIR = $(HOME)/Documents/Arduino/libraries
SKETCH = $(LIBARAIES_DIR)/OpenBCI_Wifi/examples/ESP8266HuzzahSSDP/ESP8266HuzzahSSDP.ino

UPLOAD_PORT = /dev/cu.usbserial-A104JV88
ESP_ROOT = $(HOME)/esp8266
ESP_LIBS = $(ESP_ROOT)/libraries

LIBS = $(ESP_LIBS)/Wire $(ESP_LIBS)/ESP8266WiFi $(ESP_LIBS)/ESP8266SSDP $(ESP_LIBS)/ESP8266WebServer

include $(HOME)/makeEspArduino/makeEspArduino.mk

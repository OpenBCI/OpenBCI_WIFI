# My makefile
LIBARAIES_DIR = $(HOME)/Documents/Arduino/libraries
SKETCH = $(LIBARAIES_DIR)/OpenBCI_Wifi/examples/OpenBCIWifiQC/OpenBCIWifiQC.ino
OPENBCI_WIFI_DIR = $(LIBARAIES_DIR)/OpenBCI_Wifi

UPLOAD_PORT = /dev/cu.usbserial-A104JV88
ESP_ROOT = $(HOME)/esp8266
ESP_LIBS = $(ESP_ROOT)/libraries

LIBS = $(ESP_LIBS)/Wire $(ESP_LIBS)/ESP8266WiFi $(ESP_LIBS)/SPISlave

include $(HOME)/makeEspArduino/makeEspArduino.mk

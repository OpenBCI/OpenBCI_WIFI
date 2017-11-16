# My makefile
LIBARAIES_DIR = $(HOME)/Arduino/libraries
SKETCH = $(LIBARAIES_DIR)/OpenBCI_WIFI/examples/DefaultWifiShield/DefaultWifiShield.ino
OPENBCI_WIFI_DIR = $(LIBARAIES_DIR)/OpenBCI_WIFI

UPLOAD_PORT = /dev/ttyUSB0
ESP_ROOT = $(HOME)/esp8266
ESP_LIBS = $(ESP_ROOT)/libraries

HTTP_ADDR=OpenBCI-2114.local
# BOARD = openbci
BOARD = huzzah

FLASH_DEF = 4M1M
# F_CPU = 160000000l

LIBS = $(LIBARAIES_DIR)/OpenBCI_WIFI $(ESP_LIBS)/Wire $(ESP_LIBS)/GDBStub $(ESP_LIBS)/ESP8266WiFi  $(ESP_LIBS)/DNSServer $(ESP_LIBS)/ESP8266SSDP $(ESP_LIBS)/ESP8266WebServer $(ESP_LIBS)/ESP8266HTTPClient $(LIBARAIES_DIR)/WiFiManager $(ESP_LIBS)/SPISlave $(LIBARAIES_DIR)/ArduinoJson $(ESP_LIBS)/ESP8266mDNS $(ESP_LIBS)/ESP8266HTTPUpdateServer $(ESP_LIBS)/ArduinoOTA $(LIBARAIES_DIR)/PubSubClient $(LIBARAIES_DIR)/Time $(LIBARAIES_DIR)/NTPClient
EXCLUDE_DIRS = $(LIBARAIES_DIR)/ArduinoJson/test $(LIBARAIES_DIR)/ArduinoJson/third-party $(LIBARAIES_DIR)/ArduinoJson/fuzzing $(LIBARAIES_DIR)/WebSockets/examples $(LIBARAIES_DIR)/PubSubClient/tests $(LIBARAIES_DIR)/OpenBCI_WIFI/test $(LIBARAIES_DIR)/OpenBCI_WIFI/tests-ptw-assert $(LIBARAIES_DIR)/OpenBCI_WIFI/extras

include $(HOME)/makeEspArduino/makeEspArduino.mk

# My makefile
DUINO_LIBS_DIR = $(HOME)/Arduino/libraries
SKETCH = $(DUINO_LIBS_DIR)/OpenBCI_WIFI/examples/DefaultWifiShield/DefaultWifiShield.ino
OPENBCI_WIFI_DIR = $(DUINO_LIBS_DIR)/OpenBCI_WIFI

UPLOAD_PORT = /dev/ttyUSB0
ESP_ROOT = $(HOME)/esp8266
ESP_LIBS = $(ESP_ROOT)/libraries

HTTP_ADDR=OpenBCI-2114.local
# BOARD = openbci
BOARD = huzzah

FLASH_DEF = 4M1M
# F_CPU = 160000000l

LIBS = $(DUINO_LIBS_DIR)/OpenBCI_WIFI \
       $(DUINO_LIBS_DIR)/WiFiManager \
       $(DUINO_LIBS_DIR)/ArduinoJson \
       $(DUINO_LIBS_DIR)/PubSubClient \
       $(DUINO_LIBS_DIR)/Time \
       $(DUINO_LIBS_DIR)/NTPClient \
       $(ESP_LIBS)/Wire \
       $(ESP_LIBS)/GDBStub \
       $(ESP_LIBS)/ESP8266WiFi  \
       $(ESP_LIBS)/DNSServer \
       $(ESP_LIBS)/ESP8266SSDP \
       $(ESP_LIBS)/ESP8266WebServer \
       $(ESP_LIBS)/ESP8266HTTPClient \
       $(ESP_LIBS)/SPISlave \
       $(ESP_LIBS)/ESP8266mDNS \
       $(ESP_LIBS)/ESP8266HTTPUpdateServer \
       $(ESP_LIBS)/ArduinoOTA 

EXCLUDE_DIRS = $(DUINO_LIBS_DIR)/ArduinoJson/test \
	       $(DUINO_LIBS_DIR)/ArduinoJson/third-party \
	       $(DUINO_LIBS_DIR)/ArduinoJson/fuzzing \
	       $(DUINO_LIBS_DIR)/WebSockets/examples \
	       $(DUINO_LIBS_DIR)/PubSubClient/tests \
	       $(DUINO_LIBS_DIR)/OpenBCI_WIFI/test \
	       $(DUINO_LIBS_DIR)/OpenBCI_WIFI/tests-ptw-assert \
	       $(DUINO_LIBS_DIR)/OpenBCI_WIFI/extras

include $(HOME)/makeEspArduino/makeEspArduino.mk

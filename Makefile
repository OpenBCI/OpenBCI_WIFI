#########################################
#                                       #
#      Make OpenBCI Wifi Module         #
#                                       #
#########################################

#########################################
#            makeEspArduino Source      #
#########################################
# from: https://github.com/plerup/makeEspArduino
makeEspArduino ?= $(HOME)/makeEspArduino/makeEspArduino.mk

#########################################
#            Arduino Lib                #
#########################################
ARDUINO_LIBS ?= $(HOME)/Documents/Arduino/libraries

#########################################
#            Arduino Sketch             #
#########################################
SKETCH ?= $(ARDUINO_LIBS)/OpenBCI_WIFI/examples/DefaultWifiShield/DefaultWifiShield.ino

#########################################
#            OpenBCI Lib                #
#########################################
OPENBCI_WIFI_DIR ?= $(ARDUINO_LIBS)/OpenBCI_WIFI
OPENBCI_WIFI_LIB ?= $(OPENBCI_WIFI_DIR)/src

#########################################
#            ESP8266 Lib                #
#########################################
ESP_ROOT ?= $(HOME)/esp8266
ESP_LIBS ?= $(ESP_ROOT)/libraries

#########################################
#            Board Info                 #
#########################################
HTTP_ADDR ?= OpenBCI-2114.local
UPLOAD_PORT ?= /dev/ttyUSB0
# BOARD ?= openbci
BOARD ?= huzzah
FLASH_DEF ?= 4M1M
F_CPU ?= 80000000l


#########################################
#            Include/Exclude Libs       #
#########################################
LIBS ?= $(OPENBCI_WIFI_LIB) \
       $(ARDUINO_LIBS)/WiFiManager \
       $(ARDUINO_LIBS)/ArduinoJson \
       $(ARDUINO_LIBS)/PubSubClient \
       $(ARDUINO_LIBS)/Time \
       $(ARDUINO_LIBS)/NTPClient \
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

EXCLUDE_DIRS ?= $(ARDUINO_LIBS)/ArduinoJson/test \
	       $(ARDUINO_LIBS)/ArduinoJson/third-party \
	       $(ARDUINO_LIBS)/ArduinoJson/fuzzing \
	       $(ARDUINO_LIBS)/WebSockets/examples \
	       $(ARDUINO_LIBS)/PubSubClient/tests \
	       $(ARDUINO_LIBS)/OpenBCI_WIFI/test \
	       $(ARDUINO_LIBS)/OpenBCI_WIFI/tests-ptw-assert \
	       $(ARDUINO_LIBS)/OpenBCI_WIFI/extras

include $(makeEspArduino)

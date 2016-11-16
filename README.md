# OpenBCI_Wifi

Yup. Wifi is coming to the OpenBCI.

## Getting Started

You need to clone the [ESP8266 library from github](https://github.com/esp8266/Arduino#using-git-version).

For the firmware that runs on ESP8266 see [OpenBCI_Wifi](https://github.com/PushTheWorld/OpenBCI_Wifi) on PushTheWorld’s github repository.

For the software that runs on OpenBCI, we will start from scratch with an ino file that just does SPI. 

The ESP8266 runs arduino as well so there are several installs that need to happen in order to make that work; repo: [ESP8266 Arduino](https://github.com/esp8266/Arduino) and follow the instructions for [Using Git Version](https://github.com/esp8266/Arduino/blob/master/README.md#using-git-version) to install properly. We have to do this because we need the “SPISlave” feature that has not been published in their latest release.

The library we are interested in using is the [`SPISlave`](https://github.com/esp8266/Arduino/tree/master/libraries/SPISlave). 


The pin mappings for the ESP8266 are:
Pin Name | GPIO # | HSPI Function
--- | --- | ---
MTDI | GPIO12 | MISO (DIN)
MTCK | GPIO13 | MOSI (DOUT)
MTMS | GPIO14 | CLOCK
MTDO | GPIO15 | CS / SS

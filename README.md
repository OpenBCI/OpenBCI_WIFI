# OpenBCI WiFi Shield Library

<p align="center">
  <img alt="banner" src="/images/WiFi_front_product.jpg/" width="400">
</p>
<p align="center" href="">
  Make programming with OpenBCI reliable, easy, research grade and fun!
</p>

## Welcome!

First and foremost, Welcome! :tada: Willkommen! :confetti_ball: Bienvenue! :balloon::balloon::balloon:

Thank you for visiting the OpenBCI WiFi Shield Library repository.

This document (the README file) is a hub to give you some information about the project. Jump straight to one of the sections below, or just scroll down to find out more.

* [What are we doing? (And why?)](#what-are-we-doing)
* [Who are we?](#who-are-we)
* [What do we need?](#what-do-we-need)
* [How can you get involved?](#get-involved)
* [Get in touch](#contact-us)
* [Find out more](#find-out-more)
* [Installation](#install)
* [Building](#build)
* [Running](#running)
* [License](#license)

## What are we doing?

### The problem

* Users continuously struggle to get prerequisites properly installed to get current OpenBCI Cyton and Ganglion, hours/days/weeks are wasted just _trying to get the data_.
* Bluetooth requires you to stay close to your computer, if you go to far, data is lost and the experiment is over.
* Bluetooth is too slow for transmitting research grade EEG, researchers want 1000Hz (samples per second), bluetooth with 8 channels is limited to 250Hz and with 16 channels limited to 125Hz.
* Bluetooth is unreliable when many other Bluetooth devices are around, demo device or use in busy real life experiment is not reliable. (think grand central station at rush hour)
* Bluetooth requires data to be sent to desktops in raw or compressed form, must use other node modules to parse complex byte streams, prevents from running in browser.
* OpenBCI Cyton (8 and 16 channel) with Bluetooth cannot go to any mobile device because of required Bluetooth-to-USB "Dongle". Must use USB port on Desktop/Laptop computers.
* Bluetooth on Ganglion requires low level drivers to use computers bluetooth hardware.
* The OpenBCI Cyton and Ganglion must transmit data to another computer over Bluetooth before going to the cloud for storage or analytics
* OpenBCI Cyton Dongle FTDI virtual comm port drivers have high latency by default which limits the rate at which new data is made available to your application to twice a second when it should get data as close to 250 times a second.

So, if even the very best developers integrate the current easy to use Cyton and Ganglion, they are still burdened by the limitations of the physical hardware on the OpenBCI system.

### The solution

The OpenBCI WiFi Shield Library will:

* Find, connect, sync, and configure (e.g. set sample rate) with carrier boards (Ganglion or Cyton or Cyton with Daisy) automatically
* Use TCP over WiFi to prevent packet loss
* When using UDP, offer a way to use send packets redundantly
* Enable streaming of high speed (samples rates over 1000Hz), low latency (by default, send data every 10ms), research grade EEG (no lost data) directly to any internet connected device (i.e. iPhone, Android, macOS, Windows, Linux, Raspberry Pi 3)
* Use WiFi direct to create a stable wireless transmission system even in crowded areas

Using WiFi physically solves limitations with the current state-of-the-art open source bio sensor. The goal for the WiFi Shield firmware was to create a [_one up_](https://www.google.com/url?sa=t&rct=j&q=&esrc=s&source=web&cd=1&cad=rja&uact=8&ved=0ahUKEwjF7pax67PWAhUH6oMKHdfJAcgQFggoMAA&url=https%3A%2F%2Fwww.electroimpact.com%2FWhitePapers%2F2008-01-2297.pdf&usg=AFQjCNHSyVXxRNtkFrmPiRqM5WqHWdO9-g) data pipeline, where scientific data in JSON is sent instead of raw/compressed ADC counts (yuk!) to ***make programming with OpenBCI reliable, easy, research grade and fun!***

## Who are we?

The founder of the OpenBCI WiFi Shield Library is [OpenBCI][link_openbci]. As we continue to stabilize the hardware, the people contributing to this repo is the [OpenBCI][link_openbci] at large. WiFi library contributors want to stream data _fast_ from their bio-sensors.

<a href="https://www.mozillascience.org/about">
  <img
    src="http://mozillascience.github.io/working-open-workshop/assets/images/science-fox.svg"
    align="right"
    width=140
  </img>
</a>

## What do we need?

**You**! In whatever way you can help.

We need expertise in programming, user experience, software sustainability, documentation and technical writing and project management.

We'd love your feedback along the way.

Our primary goal is to make programming with OpenBCI reliable, easy, research grade and fun, and we're excited to support the professional development of any and all of our contributors. If you're looking to learn to code, try out working collaboratively, or translate you skills to the digital domain, we're here to help.

## Get involved

If you think you can help in any of the areas listed above (and we bet you can) or in any of the many areas that we haven't yet thought of (and here we're *sure* you can) then please check out our [contributors' guidelines](CONTRIBUTING.md) and our [roadmap](ROADMAP.md).

Please note that it's very important to us that we maintain a positive and supportive environment for everyone who wants to participate. When you join us we ask that you follow our [code of conduct](CODE_OF_CONDUCT.md) in all interactions both on and offline.


## Contact us

If you want to report a problem or suggest an enhancement we'd love for you to [open an issue](../../issues) at this github repository because then we can get right on it. But you can also contact [OpenBCI][link_openbci] by email (contact@openbci.com).

You can also hang out, ask questions and share stories in the [OpenBCI NodeJS room](https://gitter.im/OpenBCI/OpenBCI_NodeJS?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge) on Gitter.

## Find out more

You might be interested in:

* Purchase a [WiFi Shield from OpenBCI](https://shop.openbci.com/collections/frontpage/products/wifi-shield?variant=44534009550)
* An example of the WiFi Shield: [DefaultWifiShield][link_wifi_default]

And of course, you'll want to know our:

* [Contributors' guidelines](CONTRIBUTING.md)
* [Roadmap](ROADMAP.md)

## Thank you

Thank you so much (Danke schön! Merci beaucoup!) for visiting the project and we do hope that you'll join us on this amazing journey to make programming with OpenBCI fun and easy.

## <a name="install"></a> Installation:

Use the `DefaultWifiShield.ino` in examples.

You need to clone the [ESP8266 library from github](https://github.com/esp8266/Arduino#using-git-version).

For the firmware that runs on ESP8266 see [OpenBCI_Wifi](https://github.com/OpenBCI/OpenBCI_WIFI) on the OpenBCI github repository.

For the software that runs on OpenBCI, we will start from scratch with an ino file that just does SPI.

The ESP8266 runs arduino as well so there are several installs that need to happen in order to make that work; repo: [ESP8266 Arduino](https://github.com/esp8266/Arduino) and follow the instructions for [Using Git Version](https://github.com/esp8266/Arduino/blob/master/README.md#using-git-version) to install properly. We have to do this because we need the “SPISlave” feature that has not been published in their latest release.

The library we are interested in using is the [`SPISlave`](https://github.com/esp8266/Arduino/tree/master/libraries/SPISlave).


## <a name="build"></a> Building and Flashing:

There are two steps to flashing code. Compiling the codebase, and then flashing it onto the wifi module.
Makefiles make it a lot easier. Especially if you use [makeEspArduino](https://github.com/plerup/makeEspArduino). Follow the instructions from the project's github.

The `make` command runs the default commands from `makeEspArduino` with OpenBCI_Wifi specific variables provided in the `Makefile`. By default builds the default sketch in the `examples/` directory. (**Note**: Run `source env/linux.env` to override `make` variables for the build.

-   To build *and* flash run:

    `make flash`

-   To build a different sketch, provide the (full or relative) path to the sketch file, e.g.:

    `make SKETCH=examples/WifiShieldJSON/WifiShieldJSON.ino`

-   To build *and* flash with a specific sketch file:

    `make SKETCH=examples/WifiShieldJSON/WifiShieldJSON.ino flash`

-   If your board is *not using* port `/dev/ttyUSB0`, then pass the `UPLOAD_PORT` variable, e.g.:

    `make UPLOAD_PORT=/dev/cu.usbserial-AI02BAAJ flash`

-   To build *and* flash if your board is *not using* port `/dev/ttyUSB0`, then pass the `UPLOAD_PORT` variable, e.g.:

    `make SKETCH=examples/WifiShieldJSON/WifiShieldJSON.ino UPLOAD_PORT=/dev/cu.usbserial-AI02BAAJ flash`

## <a name="running"></a> Running:

There are two modes that the OpenBCI WiFi shield can run in

<p align="center">
  <img alt="soft-ap" src="/images/wifi_flow_softap.png/" width="500">
</p>
<p align="center" href="">
  WiFi Direct Mode (WiFi<->Computer)
</p>
<p align="center">
  <img alt="soft-ap" src="/images/wifi_flow_station.png/" width="500">
</p>
<p align="center" href="">
  WiFi Station Mode (WiFi<->Router<->Any Internet Device)
</p>

### <a name="license"></a> License:

MIT

[link_aj_keller]: https://github.com/aj-ptw
[link_shop_wifi_shield]: https://shop.openbci.com/collections/frontpage/products/wifi-shield?variant=44534009550
[link_shop_ganglion]: https://shop.openbci.com/collections/frontpage/products/pre-order-ganglion-board
[link_shop_cyton]: https://shop.openbci.com/collections/frontpage/products/cyton-biosensing-board-8-channel
[link_shop_cyton_daisy]: https://shop.openbci.com/collections/frontpage/products/cyton-daisy-biosensing-boards-16-channel
[link_ptw]: https://www.pushtheworldllc.com
[link_openbci]: http://www.openbci.com
[link_mozwow]: http://mozillascience.github.io/working-open-workshop/index.html
[link_wifi_default]: examples/DefaultWifiShield/DefaultWifiShield.ino
[link_openleaderscohort]: https://medium.com/@MozOpenLeaders
[link_mozsci]: https://science.mozilla.org

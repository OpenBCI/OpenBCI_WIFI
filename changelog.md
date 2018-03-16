# v2.0.5

### Bug Fixes

* Setting up SPI slave too early could lead to crash

# v2.0.4

### Bug Fixes

* Access point name not set until wifi manager was ran for the first time.

# v2.0.3

### Enhancements

* Reduce buffer size for better wifi manager
* Fix LED flash when there are stored creds and shield attempts to connect
* Add update wifi link to home page

### Work In Progress

* Hitting /wifi on some devices did not trigger wifi manager

### Third party

* Now building with v2.4.0 from ESP8266/Arduino

# v2.0.2

* Reduce buffer size for better OTA Updates
* Add firmware version to homepage

# v2.0.1

* Updates the swaggerhub to 2.0.0 to match new routes.
* Removed ability to change output on UDP to JSON because not supporting JSON right now.

# v2.0.0

### New Features

* UDP support
* WiFi Direct default support (thanks @jnaulty)
* New docs (code of conduct, roadmap, contributing, readme)
* When in WiFi direct mode, connect PC to WiFi Shield hotspot, and navigate to `192.168.4.1`
* Now visit `192.168.4.1/wifi` in your browser to connect wifi shield to a new network
* To erase your network credentials and use wifi direct, use GUI or go to ip address of wifi shield and go to /wifi/delete

### Breaking Changes

* No longer doing captive portal by default, must join network and go to 192.168.4.1

### Work In Progress

* MQTT Secure
* Stability of wifi shield #48

## Beta 2

### Bug Fixes

* Switch to Station if connect to wifi network success.

## Beta 1

### Bug Fixes

* Can't connect to shield #51

# v1.3.0

At this time, can no longer support both RAW and JSON modes with a single binary file. Need optimized builds. We ended up having two modes that didnt work that well instead of highly optimized builds for the different operating modes.

### Breaking Changes

* For RAW only (can't do Raw to JSON or MQTT) - upload the binary for `DefaultWifiShield_v1.3.0.bin`
  * This is the default firmware to be shipped with WiFi Shields
  * Sample rates of 1000Hz with Cyton (with or without Daisy) possible
  * Use the OpenBCI_GUI/OpenBCIHub
* For JSON over (TCP/MQTT) (can't use OpenBCI GUI) - upload the binary for `WifiShield_RawToJSON_v1.3.0.bin`
  * Only tested with 200Hz with Ganglion
  * Only tested with 250Hz with Cyton

# v1.2.0

### Bug Fixes

* Allow for CORS on HTTP server for every call. Closes #41
* Extend max tcp packets per send
* Optimize JSON packet streaming for better performance, allows for latency up to 39ms for 8 channel and 4 channel and 25ms for 16 channel when streaming out JSON
* Changed return of route of /cloud

# v1.1.4

### Bug Fixes

* Calling to /tcp with POST could result in a 502 error when no characters found in _command_ key and also when there was a timeout waiting for the attached board to respond. Changed error when no characters found in _command_ key to 505.

# v1.1.3

### Enhancements

* Spurious code clean up

### Bug Fixes

* Fixed potential issue with gains
* Fixed issue where `outputString` was not cleared after timeout.

# v1.1.2

### Bug Fixes

* Fix bug where DELETE /wifi was not working on some modules.

# v1.1.1

### Bug Fixes

* Extend number of packets per TCP send for Cyton with Daisy board.

# v1.1.0

### Bug Fixes

* Extend raw ring buffer by 1900 packets to 2000. Really helped with dropped packets.

# v1.0.1

### Bug Fixes

* Extended timeout for command passthrough to allow for long messages such as '?' on cyton to be passed through in one message.
* Extended the length of raw buffer to allow for more packets to be sent when at high sample rate.

# v1.0.0

Add /cloud route for cloudbrain redirect.

Lots of stability. Added 596 tests to ensure the core wifi engine core works as expected every time. This is very close to what will be released with the initial production run.

# v0.2.1

### Bug Fixes

* Timestamp was not correct due to offset issue.

# v0.2.0

### Bug Fixes

* When mac address was less than 16 i.e. 0x0X where X is 0-15 the device name would be like "OpenBCI-CF1" instead of "OpenBCI-0CF1".

### Enhancements

* Over the air uploading working! Woo yess go to `/update` and boom! Victory!

# v0.1.2

### Enhancements

* Removed WifiClientPrint dependency from make file.

### New Features

* Add route `/board` to get board type, connection status, number of channels and gains for each channel

# v0.1.1

### Bug Fixes

* Timestamps in JSON are the right unit now. They are 64bit integers.
* Using root.printTo(clientTCP) takes like half a second, switched to print to string and that seemed to speed everything up.
* Getting close to having the JSON working better.

### Known Issues

* Timestamps fall out of sync:
```bash
{"chunk":[{"timestamp":1498731994785376,"data":[3718123.25,3720872.5,3718749.25,3720179.75,3718101,3715262.25,Infinity,3724404]},{"timestamp":1498731994789373,"data":[3718078.5,0,1.942749,3717989.25,3720872.5,3718659.75,3720246.75,3718056.25]},{"timestamp":1498731994793373,"data":[3717989.25,3720872.5,3718659.75,3720246.75,3718056.25,3715262.25,Infinity,3724560.5]}],"count":17488}
{"chunk":[{"timestamp":1498731994797365,"data":[3717855,3721051.25,3718570.25,3720179.75,3718391.5,3715329.25,Infinity,3724337]},{"timestamp":1498731994801403,"data":[Infinity,3724605.25,0,1.942734,1.942711,4.014413,4.050549,0]},{"timestamp":1498731994805362,"data":[3717899.75,3720760.75,3718548,3720492.5,3718190.25,3715217.5,Infinity,3724605.25]}],"count":17489}
{"chunk":[{"timestamp":1498731994809357,"data":[3718011.5,3720895,3718570.25,3720202,3718391.5,3715351.75,Infinity,3724426.5]},{"timestamp":1498731994813353,"data":[1.954601,8.110682,0,3720312,0,0,1.919922,0]},{"timestamp":1498731994817347,"data":[3717966.75,3720515,3718324.5,3720246.75,3718235,3715485.75,Infinity,3724448.75]},{"timestamp":1498731994821341,"data":[3717832.75,3720805.5,3718525.5,3720068,3718011.5,3715374,Infinity,3724538.25]}],"count":17490}
{"chunk":[{"timestamp":1498731994825337,"data":[0,0,0,1.933907,4.01032,0,4.007946,1.933792]},{"timestamp":1498731994829335,"data":[3718123.25,3721319.5,3718793.75,3720626.75,3717989.25,3715217.5,Infinity,3724717]},{"timestamp":1498736288800626,"data":[3718123.25,3721319.5,3718793.75,3720626.75,3717989.25,3715217.5,Infinity,3724717]}],"count":17491}
{"chunk":[{"timestamp":1498736288804624,"data":[3718190.25,3721073.75,3718771.5,Infinity,1.954601,-Infinity,0,0]},{"timestamp":1498736288808667,"data":[3718056.25,3720850.25,3718816.25,3720425.5,3717989.25,3715329.25,Infinity,3724627.75]},{"timestamp":1498736288812619,"data":[3718145.75,3720850.25,3718637.25,3720626.75,3718078.5,3715485.75,Infinity,3724471.25]}],"count":17492}
{"chunk":[{"timestamp":1498736288816610,"data":[3718056.25,3720895,3718637.25,3720515,3718190.25,3715195.25,Infinity,3724337]},{"timestamp":1498736288820618,"data":[3718056.25,3720850.25,3718525.5,3720537.25,3718346.75,3715262.25,Infinity,3724426.5]},{"timestamp":1498736288824602,"data":[3718011.5,3720984.25,3718682,3720671.5,3718302,3715150.5,Infinity,3724471.25]}],"count":17493}
{"chunk":[{"timestamp":1498736288828596,"data":[3718257.5,3720939.5,3718726.75,3720403.25,3718078.5,3715351.75,Infinity,3724404]},{"timestamp":1498736288832596,"data":[3718235,3721029,3718592.75,3720023.25,3718235,3715396.25,0,4.014412]},{"timestamp":1498736288836596,"data":[3718190.25,3720805.5,3718816.25,3720425.5,3718324.5,3715150.5,Infinity,3724180.5]}],"count":17494}
{"chunk":[{"timestamp":1498736288840583,"data":[3718056.25,3720872.5,3718548,3720269,3718212.75,3714815.25,Infinity,3724136]},{"timestamp":1498736288844585,"data":[3718145.75,3721029,3718436.25,3720380.75,3718190.25,3715172.75,Infinity,3724270]},{"timestamp":1498736288848626,"data":[Infinity,3724314.75,0,1.942734,1.942711,3715284.5,Infinity,0]},{"timestamp":1498736288852574,"data":[3718302,3720827.75,3718369.25,3720358.5,3718123.25,3715150.5,Infinity,3724314.75]}],"count":17495}
{"chunk":[{"timestamp":1498736288856576,"data":[3718123.25,3720850.25,3718391.5,3720269,3718123.25,3715240,Infinity,3724381.75]},{"timestamp":1498736288860567,"data":[1.942734,3718279.75,3720693.75,3718615,3720246.75,3717922,3715038.75,Infinity]},{"timestamp":1498736288864563,"data":[3718279.75,3720693.75,3718615,3720246.75,3717922,3715038.75,Infinity,3724091.25]}],"count":17496}
{"chunk":[{"timestamp":1498736288868561,"data":[3718078.5,0,0,4.014412,4.011159,0,0,0]},{"timestamp":1498736288872561,"data":[3718168,3720895,3718570.25,3720291.5,3718279.75,3715150.5,Infinity,3724337]},{"timestamp":1498736288876554,"data":[3718101,3720805.5,3718503.25,3720068,3717966.75,3715172.75,Infinity,3724270]}],"count":17497}
{"chunk":[{"timestamp":1498736288880548,"data":[3718101,3720738.5,3718726.75,3720179.75,3718123.25,3715485.75,Infinity,3724538.25]},{"timestamp":1498736288884589,"data":[3718324.5,3720760.75,3718704.5,3720224.25,3718145.75,3715329.25,Infinity,3724650]},{"timestamp":1498736288888543,"data":[3718302,3720738.5,3718503.25,3720403.25,3718235,3715351.75,Infinity,3724605.25]}],"count":17498}
{"chunk":[{"timestamp":1498736288892539,"data":[3718011.5,3721006.75,3718503.25,3720358.5,3718324.5,3715284.5,Infinity,3724493.5]},{"timestamp":1498736288896531,"data":[3717989.25,3720962,3718413.75,3720179.75,3718168,3714949.25,Infinity,3724605.25]},{"timestamp":1498736288900531,"data":[3717989.25,3720962,3718413.75,3720179.75,3718168,3714949.25,Infinity,3724605.25]}],"count":17499}
{"chunk":[{"timestamp":1498736288904530,"data":[3718078.5,3720760.75,3718413.75,3720179.75,3717966.75,3715150.5,Infinity,3724448.75]},{"timestamp":1498736288908524,"data":[0,1.999527,3718101,3720559.5,3718905.5,3720202,3718011.5,3715396.25]},{"timestamp":1498736288912520,"data":[3718101,3720559.5,3718905.5,3720202,3718011.5,3715396.25,Infinity,3724426.5]},{"timestamp":1498736288916518,"data":[3718391.5,3720850.25,3718838.5,3720425.5,3718279.75,3715463.5,Infinity,3724448.75]}],"count":17500}
{"chunk":[{"timestamp":1498736288920512,"data":[3718078.5,3720962,3718682,3720336,3718168,3715150.5,Infinity,3724404]},{"timestamp":1498736288924559,"data":[3718056.25,3720939.5,3718637.25,3720269,3718078.5,3715217.5,0,4.014412]},{"timestamp":1498736288928504,"data":[3718056.25,3720962,3718615,3720224.25,3717944.5,3715038.75,Infinity,3724448.75]}],"count":17501}
{"chunk":[{"timestamp":1498736288932502,"data":[3718257.5,3720805.5,3718726.75,3720269,3718145.75,3715083.5,Infinity,3724538.25]},{"timestamp":1498736289936498,"data":[0,1.942749,3717989.25,3721453.75,3718883.25,3720269,3718235,3715172.75]},{"timestamp":1498736289940494,"data":[3717989.25,3721453.75,3718883.25,3720269,3718235,3715172.75,Infinity,3724493.5]}],"count":17502}
{"chunk":[{"timestamp":1498736289944491,"data":[3718123.25,3721342,3718637.25,3720068,3718101,3715284.5,Infinity,3724381.75]},{"timestamp":1498736289948488,"data":[3717899.75,3721073.75,3718726.75,3719933.75,3718056.25,3715329.25,0,4.014412]},{"timestamp":1498736289952483,"data":[3717966.75,3721185.5,3718793.75,3720135,3718034,3715172.75,Infinity,3724516]}],"count":17503}
{"chunk":[{"timestamp":1498736289956481,"data":[3718190.25,3721230.25,3718928,3720246.75,3718056.25,3714949.25,Infinity,3724471.25]},{"timestamp":1498736289960477,"data":[3718302,3720984.25,3718995,3720202,3718011.5,3715217.5,Infinity,3724560.5]},{"timestamp":1498736289964515,"data":[3718235,3720447.75,3718659.75,3720179.75,3718168,3715217.5,Infinity,3724359.5]},{"timestamp":1498731995001169,"data":[3718190.25,3720760.75,3718637.25,3720269,3718123.25,3715262.25,Infinity,3724314.75]}],"count":17504}
{"chunk":[{"timestamp":1498731995005171,"data":[0,8.435183,0,4.106922,4.106492,2.002465,0,NaN]},{"timestamp":1498731995009163,"data":[3718078.5,3721185.5,3718592.75,3720291.5,3718078.5,3715262.25,Infinity,3724381.75]},{"timestamp":1498731995013161,"data":[3718235,3721096,3718793.75,3720336,3718279.75,3715262.25,Infinity,3724493.5]}],"count":17505}
{"chunk":[{"timestamp":1498731995017158,"data":[3718056.25,3721006.75,3718793.75,3720425.5,3718346.75,3715150.5,0,1.942734]},{"timestamp":1498731995021155,"data":[3718056.25,3721006.75,3718458.5,3720358.5,3718391.5,3715105.75,Infinity,0]},{"timestamp":1498731995025150,"data":[3718078.5,3721118.5,3718235,3720269,3718078.5,3714949.25,Infinity,3724426.5]}],"count":17506}
```
* Infinity in JSON? What's up with that?

# v0.1.0

### New Features

* Direct to JSON

# v0.0.1

Initial release.

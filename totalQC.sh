echo "Plug in wifi shield to UART to USB"
read -p "Press enter to continue:"
echo "Power the wifi shield and flip the switch to the on position"
read -p "Press enter to continue:"
echo "Press & hold RESET, press & hold PROG, release RESET, then release PROG"
read -p "Press enter to continue:"
echo "Starting flash of wifi shield"
make -f makeEspWifiDefault.mk flash
echo "Flash complete, now use your computer to get the shield on your local network"
read -p "Press enter to continue once your computer is back on the network:"
echo "Starting verification testing..."
node test/js/qc.js

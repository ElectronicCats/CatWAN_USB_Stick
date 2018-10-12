# LoRA USB Stick

<a href="https://www.tindie.com/stores/electroniccats/?ref=offsite_badges&utm_source=sellers_electroniccats&utm_medium=badges&utm_campaign=badge_large"><img src="https://d2ss6ovg47m0r5.cloudfront.net/badges/tindie-larges.png" alt="I sell on Tindie" width="200" height="104"></a>

**WARNING: This is a beta device, use it at your own risk**

Are you interested in learning how LoRa works at the package level? Debugging your own LoRa hardware and trying to detect where something is wrong? Or maybe you're writing a custom application for your Internet of Things (IoT) network with LoraWAN? We have the perfect tool for you!

This CatWAN USB Stick is programmed with a special firmware image that makes it an easy-to-use LoRa sniffer. You can passively capture the data exchanges between two LoRa devices, capturing with our "LoRa Sniffer" the open source network analysis tool that we have created to use together.

This device can work in networks LoRaWAN compatible with classes A, B and C, although currently we do not have a firmware for this way of working, the CatWAN firmware is completely open source and you can find it in our repository along with the schematic. If you want to reprogram this device you can do it through Arduino IDE and its USB port or if you do not have to use a J-Link. ATMEL-ICE or a DIY SWD programmer

This device has a SAMD21 ARM Cortex microcontroller at 48Mhz with native USB 2.1, with 256Kb for programming, compatible with Arduino and Circuit Python.

Check our repository for more details and software downloads!


### Main characteristics

- Works with any PC, Raspberry Pi or BeagleBone, even a smartphone or tablet
- Supports packet mode LoRa® (package mode) or LoRaWAN ™ Class A, B and C
- Compatible with The Things Network and other LoRaWAN networks
- Based on the RFM95
- RX LED as reception indicator, programmable by the user
- Easy reprogramming compatible with Arduino and Circuit Python
- Compatible with the [App LoRa Sniffer](https://github.com/ElectronicCats/LoRa_Sniffer)
- Open Source

### Specifications

- Connectivity: USB 2.1
- Power Consumption: 140 ma typical TX, 20 ma idle (with power LED)
- Receiver Sensitivity: down to -146 dBm
- TX Power: adjustable up to +18.5 dBm
- Range: up to 15 km coverage in suburban and up to 5 km coverage in urban areas


### For update firmware with Arduino IDE: MattairTech D|L|C Core Installation

* To update from a previous version, click on MattairTech SAM D|L|C Core for Arduino in Boards Manager, then click Update.

1. The MattairTech SAM D|L|C Core requires Arduino IDE 1.6.7 or above (including 1.8.x).
2. In the Arduino IDE, click File->Preferences.
3. Click the button next to Additional Boards Manager URLs.
4. Add `https://www.mattairtech.com/software/arduino/beta/package_MattairTech_index.json`.
5. Save preferences, then open the Boards Manager.
6. Install the Arduino SAMD Boards package. Use version 1.6.2 or higher.
7. Install the MattairTech SAM D|L|C Core for Arduino package.
8. Close Boards Manager, then click Tools->Board->MattairTech MT-D21E (rev B).
9. Select the MCU with the now visible Tools->Microcontroller menu->SAMD21E18A).
10. Select the clock source "INTERNAL_USB_CALIBRATED_OSCILLATOR"*
11. Plug in the board. The blink sketch should be running.
12. Click Tools->Port and choose the COM port. Note that the board indicated may not match the chosen board*
13. You can now upload your own sketch.

*Forgetting to set the clock source will stop the device from booting. It can saved by booting with safe mode, (press the reset button twice during startup).


Electronic Cats invests time and resources providing this open source design, please support Electronic Cats and open-source hardware by purchasing products from Electronic Cats!

Designed by Electronic Cats.

Firmware released under an GNU AGPL v3.0 license. See the LICENSE file for more information.

Hardware released under an CERN Open Hardware Licence v1.2. See the LICENSE_HARDWARE file for more information.

Electronic Cats is a registered trademark, please do not use if you sell these PCBs.

29 July 2018

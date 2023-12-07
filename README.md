# CatWAN USB Stick

<p align="center">
    <img src="https://github.com/ElectronicCats/CatWAN_USB_Stick/assets/44976441/51508bc4-9d92-4624-85f6-da037400540e" width="60%">
    <br><strong>CatWAN USB Stick</strong>
</p>

## Find yours in our official store!
<p align="center">
    <a href="https://electroniccats.com/store/catwan-usb-stick/">
        <img src="https://electroniccats.com/wp-content/uploads/badge_store.png" alt="CatWANShield" height="104">
        <br>
        <strong>Get yours here!</strong>
    </a>
</p>

## What is CatWAN USB Stick?
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

### Pin connections

|Arduino |   Port    |   RFM Function|
|--------|-----------|---------------|
|0       |   PORTA4  |   DIO0   |
|1       |   PORTA23 |   DIO1   |
|2       |   PORTA27 |   DIO2   |
|13      |   PORTA14 |   LED    |
|15      |   PORTA15 |   DIO5   |
|17      |   PORTA22 |   MISO   |
|18      |   PORTA18 |   MOSI   |
|19      |   PORTA19 |   SCK    |
|20      |   PORTA17 |   SS     |

##  Wiki and Getting Started

[**Getting Started in our Wiki**](https://github.com/ElectronicCats/CatWAN_USB_Stick/wiki)

## Open Source certified.
<p align="center">
    <a href="https://www.oshwa.org/cert">
        <img src="https://github.com/ElectronicCats/CatWAN_USB_Stick/raw/master/MX000006.png" alt="CatWANShield" width="10%">
        <br>
        <strong>[OSHW] MX000006 | Certified open source hardware |</strong>
    </a>
</p>

Electronic Cats invests time and resources in providing this open-source design, please support Electronic Cats and open-source hardware by purchasing products from Electronic Cats!

Designed by Electronic Cats.

Firmware released under an GNU AGPL v3.0 license. See the LICENSE file for more information.

Hardware released under an CERN Open Hardware Licence v1.2. See the LICENSE_HARDWARE file for more information.

Electronic Cats is a registered trademark, please do not use if you sell these PCBs.

27 May 2019

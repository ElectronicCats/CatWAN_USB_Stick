# Firmware examples
This folder contains 3 sub-folders:

- LoRaWAN
- LoRa-PHY
- bootloader_uf2 for only SAMD21 version

The sub-folder bootloader_uf2 has inside all the files that can be use to re-flash your CatWAN USB-Stick in case of hard brick it.

The sub-folders LoRa and LoRaWAN have inside its own examples about a posible implementation of LoRa from LoRa physical layer to some LoRaWAN implementations. 

## LoRaWAN Folder
Here you will find three possible ways to implement a LoRaWAN Stack:

- LoRa_WAN_Sniffer_REG
	Which is an example where you only require the SPI library and all the registers inside the radio are modified just as the LoRaWAN stack says.

- LMIC_library 
	In this folder you'll find two examples about an implementation of the LoRaWAN Stack with the LMIC library with the two types of activations, ABP and OTAA, both examples are a class A node, for this examples you MUST install the [LMIC library](https://github.com/things-nyc/arduino-lmic).
	
- ArduinoLoRaWAN_library 
	In this folder you'll find an example of the LoRaWAN stack implementation with the ArduinoLoRaWAN library which is simple than the LMIC implementation, for this examples you MUST install the [Arduino LoRaWAN library](https://github.com/BeelanMX/arduino-LoRaWAN)
	
## LoRa_PHY 
Here you'
ll find the simplest examples to use the LoRa radio just using the physical layer of LoRa 

- "LoRaReceiverCallback" 
   Will receive any LoRa data from an interrupt
- "LoRaSender" 
	Will send data on the physical layer of LoRa
- "LoRa_PHY_Sniffer"
	Will receive any packet an is under development to configure the radio over serial communication
- "Relay Stick"
	Will communicate with the CatWAN Relay board to control and monitor relays.
- "LoRaReceiver"
    Will receive any LoRa data without an interrupt
- "LoRaSniffer"



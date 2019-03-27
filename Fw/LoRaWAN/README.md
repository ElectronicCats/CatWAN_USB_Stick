# LoRaWAN Folder
Here you will find three possible ways to implement a LoRaWAN Stack:

- LoRa_WAN_Sniffer_REG
	Which is an example where you only require the SPI library and all the registers inside the radio are modified just as the LoRaWAN stack says.

- LMIC_library 
	In this folder you'll find two examples about an implementation of the LoRaWAN Stack with the LMIC library with the two types of activations, ABP and OTAA, both examples are a class A node, for this examples you MUST install the [LMIC library](https://github.com/things-nyc/arduino-lmic).
	
- ArduinoLoRaWAN_library 
	In this folder you'll find an example of the LoRaWAN stack implementation with the ArduinoLoRaWAN library which is simple than the LMIC implementation, for this examples you MUST install the [Arduino LoRaWAN library](https://github.com/BeelanMX/arduino-LoRaWAN)
	


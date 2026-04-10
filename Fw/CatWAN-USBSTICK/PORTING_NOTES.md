# CatWAN USB Stick Porting Notes

This Zephyr app started as a CatSniffer-derived firmware for an RP2040 board
with an SX1262 and a CC1352 coprocessor. The CatWAN USB Stick hardware is
different:

- RP2040
- RFM95 / SX1276
- no CC1352 coprocessor

## What this first port enables

- Real RP2040 to RFM95/SX1276 pin mapping in the Zephyr overlay
- LoRa RX/TX over the USB control/data ports
- Runtime LoRa configuration from the shell
- RP2040 UF2 reboot flow

## What is intentionally disabled in this stage

- CC1352 bridge, bootloader, and JTAG workflows
- SX1262-specific FSK/GFSK workflow
- Custom LoRa sync-word programming through the current Zephyr SX1276 backend

## Important implication for Meshtastic

Meshtastic compatibility is not just "LoRa packets over serial". To make this
stick behave like a real Meshtastic node/dongle we will likely need one of
these paths:

1. Port Meshtastic itself to this RP2040 + RFM95 board variant.
2. Add the missing radio features and protocol framing needed by a host-side
   bridge mode.

The current port is a good foundation for a flexible USB LoRa tool, but it is
not yet a Meshtastic-compatible firmware.

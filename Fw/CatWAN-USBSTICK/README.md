# CatSniffer Firmware (RP2040/Zephyr)

Multi-protocol wireless sniffer and LoRa/FSK communication device based on RP2040, CC1352P7, and SX1262 radios.

[![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)](VERSION)
[![Zephyr](https://img.shields.io/badge/Zephyr-4.1.99-green.svg)](https://zephyrproject.org/)
[![Platform](https://img.shields.io/badge/platform-RP2040-red.svg)](https://www.raspberrypi.com/products/rp2040/)

## Table of Contents

- [Features](#features)
- [Hardware](#hardware)
- [To-Do](#to-do)
- [Quick Start](#quick-start)
- [USB Endpoints](#usb-endpoints)
- [Command Reference](#command-reference)
- [Usage Examples](#usage-examples)
- [Building from Source](#building-from-source)
- [Development](#development)
- [Troubleshooting](#troubleshooting)

---

## Features

### Dual Radio Support
- **CC1352P7 (TI)**: Zigbee, Thread, Matter, BLE 5.2, 2.4 GHz / Sub-GHz protocols
- **SX1262 (Semtech)**: LoRa and FSK/GFSK long-range communication

### Triple USB CDC-ACM Endpoints
- **Cat-Bridge**: Transparent UART bridge to CC1352P7 — no driver needed
- **Cat-LoRa**: Binary or text interface to the SX1262 radio
- **Cat-Shell**: Live configuration and control shell

### LoRa Capabilities (SX1262)
- **Dual-mode operation**:
  - **Stream mode** (default): Raw binary TX/RX for programmatic access
  - **Command mode**: Text-based interface for debugging and testing
- **Full runtime configuration** — no recompilation needed:
  - Frequency: 137–1020 MHz
  - Spreading factor: SF7–SF12
  - Bandwidth: 125 / 250 / 500 kHz
  - Coding rate: 4/5, 4/6, 4/7, 4/8
  - TX power: −9 to 22 dBm
  - Preamble length: 6–65535
  - Sync word: private, public, or custom (e.g. `0x2D` for Meshtastic)
  - IQ inversion: normal / inverted
- **Atomic configuration apply**: stage multiple changes, apply in one shot with `lora_apply`

### FSK / GFSK Capabilities (SX1262)
- Switch modulation at runtime with `modulation fsk`
- **Full runtime configuration**:
  - Frequency: 137–1020 MHz
  - Bit rate: 600–300000 bps
  - Frequency deviation: 600–200000 Hz
  - RX bandwidth: 4.8–467.0 kHz (21 discrete steps)
  - TX power: −9 to 22 dBm
  - Preamble length: 0–65535 bytes
  - Sync word: up to 8 bytes (hex)
  - CRC: on / off
  - Data whitening: on / off
  - Packet length mode: fixed / variable
  - Payload length: 1–255 bytes
  - GFSK Gaussian BT shaping: off / 0.3 / 0.5 / 0.7 / 1.0
- **Atomic configuration apply** with `fsk_apply`

### System Features
- RF band switching (2.4 GHz / Sub-GHz / LoRa) via hardware RF switch
- CC1352P7 bootloader mode for in-field firmware updates
- CC1352 firmware ID tracking — store and recall which image is flashed, persistent across reboots
- RP2040 USB bootloader for easy firmware updates (drag-and-drop `.uf2`)
- **Board identification**: `identify` command blinks all LEDs so you can tell boards apart when multiple CatSniffers are connected to the same host
- **Packet loss monitoring**: UART bridge tracks overrun and ring-buffer overflow events in real time
- Concurrent radio operation — CC1352 and SX1262 work independently at the same time

---

## Hardware

### RP2040 Microcontroller
- **Dual ARM Cortex-M0+ cores @ 133 MHz**
- **264 KB RAM**
- **2 MB Flash**
- **USB Full-Speed Device**

### CC1352P7 Radio (Texas Instruments)
- **Protocols**: Zigbee, Thread, Matter, BLE 5.2, IEEE 802.15.4, Wi-SUN, MIOTY, Amazon Sidewalk, Wireless M-Bus, 6LoWPAN, proprietary Sub-GHz
- **Frequencies**: 2.4 GHz + Sub-GHz (863–928 MHz)
- **Interface**: UART @ 921600 baud (passthrough), 500000 baud (bootloader)
- **Control pins**: GPIO2 (boot), GPIO3 (reset)
- **JTAG pins**: GPIO11–14 (cJTAG — for CC1352 flash erase recovery)
- **Wireshark**: v4.0.x compatible

### SX1262 LoRa / FSK Radio (Semtech)
- **Modulations**: LoRa (SF5–SF12), FSK, GFSK
- **Frequency range**: 137–1020 MHz
- **Interface**: SPI @ 1 MHz (GPIO16–19)
- **Control pins**: GPIO24 (reset), GPIO4 (busy), GPIO5 (DIO1)

### RF Switching
- **3 hardware RF paths** via GPIO-controlled switch (GPIO8–10)
  - `band1` → 2.4 GHz (CC1352P7)
  - `band2` → Sub-GHz (CC1352P7)
  - `band3` → LoRa / FSK (SX1262)

### LEDs
- **LED1** (GPIO27): USB enumeration status
- **LED2** (GPIO26): Mode indicator
- **LED3** (GPIO28): Mode indicator
  - Slow blink (1 s): Normal passthrough / Stream mode
  - Fast cycle (200 ms): Boot mode / LoRa Command mode
  - Rapid simultaneous blink (all 3, 100 ms): `identify` command active

---

## TO-DO

- **LoRa TX-only stream**: Reduce the ~5 ms RX→TX turnaround on the SX1262 for high-throughput transmit-only use cases.
- **Add support to save CC1352 images**: Store multiple CC1352 firmware images in the RP2040 flash and flash them on demand.
- **Add automated testing with 2 boards**: Fully automate RF link testing with two CatSniffers including CC1352 programming.
- **Add CC1352 serial programmer**: Allow the RP2040 to reprogram the CC1352 over the internal UART, replacing the need for an external `cc2538-bsl` host tool.
- **Add JTAG direct support to erase CC1352 flash**: Use the cJTAG pins (GPIO11–14) to erase a bricked CC1352 directly from the RP2040, without needing an external JTAG probe.

---

## Quick Start

### Prerequisites

1. **Zephyr SDK** (0.16.8 or later)
2. **Python 3.10+**
3. **Build tools**: CMake, Ninja, GCC ARM (`arm-zephyr-eabi`)

### Setup Environment

```bash
# Install west and dependencies
pip install west jsonschema pyelftools PyYAML

# Initialize workspace
west init -l RP2040/catsniffer --mf west.yml
west update

# Set Zephyr base
export ZEPHYR_BASE=$PWD/RP2040/zephyr
export ZEPHYR_SDK_INSTALL_DIR=$HOME/zephyr-sdk-0.16.8
```

### Build and Flash

```bash
# Build
west build -s catsniffer -b rpi_pico -d build/rpi_pico

# Output files
build/rpi_pico/zephyr/zephyr.uf2   # drag-and-drop flash
build/rpi_pico/zephyr/zephyr.hex
build/rpi_pico/zephyr/zephyr.elf
```

**Flash via USB bootloader:**
1. Hold **BOOTSEL** on the RP2040 while connecting USB
2. Release — a `RPI-RP2` drive appears
3. Copy `zephyr.uf2` onto the drive
4. Board reboots automatically

### Connect

The board enumerates as 3 serial ports with no drivers required on macOS and Linux:

```
macOS:
  /dev/cu.usbmodem*1   →  Cat-Bridge  (CC1352P7 passthrough)
  /dev/cu.usbmodem*3   →  Cat-LoRa    (SX1262 data)
  /dev/cu.usbmodem*5   →  Cat-Shell   (configuration shell)

Linux:
  /dev/ttyACM0   →  Cat-Bridge
  /dev/ttyACM1   →  Cat-LoRa
  /dev/ttyACM2   →  Cat-Shell
```

---

## USB Endpoints

### CDC0: Cat-Bridge (CC1352P7 Passthrough)

**Purpose**: Transparent UART bridge between USB and the CC1352P7 co-processor.

| Mode | Baud rate | When |
|------|-----------|------|
| Passthrough | 921600 | Normal sniffing operation |
| Bootloader | 500000 | After `boot` command — CC1352 BSL ready |

**Use cases**:
- Wireless protocol sniffing (Zigbee, Thread, BLE, IEEE 802.15.4, Matter, Sub-GHz)
- CC1352P7 firmware updates via `cc2538-bsl`
- Integration with Wireshark, TI Packet Sniffer, Sniffle, etc.

**Example** (Python):
```python
import serial

bridge = serial.Serial('/dev/ttyACM0', 921600)
bridge.write(b'\x00\x01\x02...')  # transparent passthrough
data = bridge.read(100)
```

---

### CDC1: Cat-LoRa (SX1262 Data Interface)

Carries both LoRa and FSK data depending on the active modulation. Operates in two modes selectable from Cat-Shell.

#### Stream Mode (default — binary, for applications)

**TX**: write raw bytes → transmitted as a single radio packet (max 255 bytes)

**RX**: length-prefixed binary frame:
```
[length: 1 byte][payload: N bytes][RSSI offset: 1 byte][SNR offset: 1 byte]
```

RSSI and SNR are offset by 128 — subtract 128 to get the signed dBm / dB value.

```python
import serial

lora = serial.Serial('/dev/ttyACM1', 115200)

# Transmit
lora.write(b'Hello LoRa!')

# Receive
while True:
    if lora.in_waiting:
        length = ord(lora.read(1))
        payload = lora.read(length)
        rssi = ord(lora.read(1)) - 128
        snr  = ord(lora.read(1)) - 128
        print(f"RX: {payload.hex()} | RSSI: {rssi} dBm | SNR: {snr} dB")
```

#### Command Mode (text, for debugging)

Switch via Cat-Shell: `lora_mode command`

Commands sent on Cat-LoRa:

| Command | Description |
|---------|-------------|
| `TEST` | Initialize SX1262 and report status |
| `TXTEST` | Send a test `PING` packet |
| `TX <hex>` | Send a hex-encoded packet (e.g. `TX 48656C6C6F`) |

RX format: `RX: <hex> | RSSI: -45 | SNR: 8\r\n`

```bash
# On Cat-Shell — switch to command mode
> lora_mode command

# On Cat-LoRa
> TEST
LoRa: Starting initialization...
LoRa: Device ready

> TX 48656C6C6F
TX Result: 0 (Success)

> TXTEST
TX Result: 0 (Success)
```

Switch back: `lora_mode stream`

---

### CDC2: Cat-Shell (Configuration Shell)

**Purpose**: Live configuration, control, and diagnostics for all subsystems.

**Baud rate**: 115200

Type `help` for a full command listing. All commands end with `\n` or `\r\n`.

---

## Command Reference

### General

| Command | Description |
|---------|-------------|
| `help` | List all available commands with descriptions |
| `status` | Show mode, band, radio state, modulation, and packet-loss counters |
| `fw_version` | Show firmware version, git SHA, build timestamp, and compiler |
| `identify` | Blink all three LEDs rapidly (~2 s) to identify this board among multiple connected units |

### RP2040 Control

| Command | Description |
|---------|-------------|
| `reboot` | Enter RP2040 USB bootloader — board appears as `RPI-RP2` drive for flashing a new `.uf2` |

### CC1352P7 Control

| Command | Description |
|---------|-------------|
| `boot` | Enter CC1352P7 bootloader mode — holds BSL pin and switches UART to 500000 baud, ready for `cc2538-bsl` |
| `exit` | Exit CC1352P7 bootloader, return to passthrough mode at 921600 baud |
| `cc1352_fw_id set <id>` | Store the CC1352 firmware name in persistent flash (e.g. `cc1352_fw_id set sniffle-v1.10`) |
| `cc1352_fw_id get` | Read back the stored CC1352 firmware name |
| `cc1352_fw_id clear` | Erase the stored CC1352 firmware name |
| `cc1352_fw_id list` | List known official CC1352 firmware identifiers |

### Band Selection

| Command | Radio | Frequency |
|---------|-------|-----------|
| `band1` | CC1352P7 | 2.4 GHz |
| `band2` | CC1352P7 | Sub-GHz (863–928 MHz) |
| `band3` | SX1262 | 137–1020 MHz (LoRa / FSK) |

### Modulation (SX1262)

| Command | Description |
|---------|-------------|
| `modulation lora` | Switch SX1262 to LoRa modulation |
| `modulation fsk` | Switch SX1262 to FSK / GFSK modulation |

### LoRa Configuration (SX1262)

Changes are staged until `lora_apply` is called. View the current config with `lora_config`.

| Command | Parameters | Description |
|---------|-----------|-------------|
| `lora_freq` | `<Hz>` | Set frequency — 137 to 1020 MHz (e.g. `lora_freq 915000000`) |
| `lora_sf` | `<7-12>` | Set spreading factor SF7–SF12 |
| `lora_bw` | `<125\|250\|500>` | Set bandwidth in kHz |
| `lora_cr` | `<5\|6\|7\|8>` | Set coding rate — 4/5, 4/6, 4/7, 4/8 |
| `lora_power` | `<-9 to 22>` | Set TX power in dBm |
| `lora_preamble` | `<6-65535>` | Set preamble length |
| `lora_syncword` | `<private\|public\|0xNN>` | Set sync word — use `0x2D` for Meshtastic |
| `lora_iq` | `<normal\|inverted>` | Set IQ inversion |
| `lora_mode` | `<stream\|command>` | Switch Cat-LoRa port between binary and text mode |
| `lora_config` | — | Display current LoRa configuration |
| `lora_apply` | — | Apply all staged LoRa changes atomically |

### FSK / GFSK Configuration (SX1262)

Changes are staged until `fsk_apply` is called. View the current config with `fsk_config`.

| Command | Parameters | Description |
|---------|-----------|-------------|
| `fsk_freq` | `<Hz>` | Set frequency — 137 to 1020 MHz |
| `fsk_bitrate` | `<bps>` | Set bit rate — 600 to 300000 bps |
| `fsk_fdev` | `<Hz>` | Set frequency deviation — 600 to 200000 Hz |
| `fsk_bw` | `<kHz>` | Set RX bandwidth — 4.8 to 467.0 kHz (21 steps) |
| `fsk_power` | `<-9 to 22>` | Set TX power in dBm |
| `fsk_preamble` | `<0-65535>` | Set preamble length in bytes |
| `fsk_syncword` | `<hex>` | Set sync word as hex bytes (e.g. `fsk_syncword 12AD`) |
| `fsk_crc` | `<on\|off>` | Enable or disable CRC |
| `fsk_whitening` | `<on\|off>` | Enable or disable data whitening |
| `fsk_pktlen` | `<fixed\|variable>` | Set packet length mode |
| `fsk_payload` | `<1-255>` | Set payload length (fixed mode) or max length (variable mode) |
| `fsk_bt` | `<off\|0.3\|0.5\|0.7\|1.0>` | Set GFSK Gaussian BT shaping filter |
| `fsk_config` | — | Display current FSK configuration |
| `fsk_apply` | — | Apply all staged FSK changes atomically |

### Diagnostics

| Command | Parameters | Description |
|---------|-----------|-------------|
| `loss_reset` | — | Reset CC1352P7 UART bridge packet-loss counters (overruns + ring-buffer drops) |
| `radio` | `<TEST\|FSKRX\|FSKTEST\|FSKTX\|TX <hex>>` | Forward a raw command directly to the radio subsystem |

### Example Shell Session

```bash
$ screen /dev/ttyACM2 115200

> fw_version
FW: 1.0.0
Git: 2a4302a (clean)
Built: 2026-02-26T17:18:18Z
Compiler: GNU 12.2.0

> status
Mode: PASSTHROUGH | Band: SUB-GHz | LoRa: initialized | LoRa Mode: Stream
UART overruns: 0 | Ring overflows: 0

> lora_config
LoRa Configuration:
  Frequency:      915000000 Hz
  Spreading Factor: SF7
  Bandwidth:      125 kHz
  Coding Rate:    4/5
  TX Power:       20 dBm
  Preamble:       12
  Sync Word:      private
  IQ:             normal
  Mode:           Stream

> lora_freq 868000000
Frequency set to 868000000 Hz (pending)

> lora_sf 10
Spreading Factor set to SF10 (pending)

> lora_apply
Applying LoRa configuration...
LoRa configuration applied successfully

> cc1352_fw_id set sniffle-v1.10
CC1352 FW ID set: sniffle-v1.10

> identify
Identifying board...

> loss_reset
CC1352 loss counters reset
```

---

## Usage Examples

### Example 1: Zigbee / Thread Sniffing with Wireshark

```bash
# Connect Cat-Bridge to Wireshark
cat /dev/ttyACM0 | wireshark -k -i -
```

### Example 2: LoRa Point-to-Point Link

**Transmitter:**
```python
import serial, time

lora = serial.Serial('/dev/ttyACM1', 115200)
while True:
    lora.write(b'Hello from CatSniffer')
    time.sleep(2)
```

**Receiver:**
```python
import serial

lora = serial.Serial('/dev/ttyACM1', 115200)
while True:
    if lora.in_waiting:
        length  = ord(lora.read(1))
        payload = lora.read(length)
        rssi    = ord(lora.read(1)) - 128
        snr     = ord(lora.read(1)) - 128
        print(f"RX: {payload.decode()} | RSSI: {rssi} dBm | SNR: {snr} dB")
```

### Example 3: EU868 LoRa Configuration

```bash
> lora_freq 868100000
> lora_sf 12
> lora_bw 125
> lora_cr 5
> lora_power 14
> lora_apply
LoRa configuration applied successfully
```

### Example 4: Meshtastic-Compatible Sync Word

```bash
> lora_syncword 0x2D
> lora_apply
```

### Example 5: FSK Link at 50 kbps

```bash
> modulation fsk
> fsk_freq 915000000
> fsk_bitrate 50000
> fsk_fdev 25000
> fsk_bw 78.2
> fsk_syncword 12AD
> fsk_crc on
> fsk_apply
```

### Example 6: Identifying a Board Among Multiple Units

```bash
# On the Cat-Shell port of the board you want to locate
> identify
Identifying board...
# All three LEDs blink rapidly for ~2 seconds
```

### Example 7: Concurrent CC1352 + LoRa Operation

```bash
# Terminal 1 — Zigbee sniffing via CC1352
screen /dev/ttyACM0 921600

# Terminal 2 — LoRa link via SX1262 (Python)
python3 lora_receive.py
```

Both radios operate fully independently.

### Example 8: CC1352 Firmware Update

```bash
# 1. From Cat-Shell, enter bootloader mode
> boot

# 2. Flash new CC1352 firmware from host
cc2538-bsl.py -p /dev/ttyACM0 -evw sniffle.hex

# 3. Return to passthrough
> exit

# 4. Record what was flashed
> cc1352_fw_id set sniffle-v1.10
```

---

## Building from Source

### Directory Structure

```
catsniffer/
├── boards/
│   └── rpi_pico.overlay     # Device tree: GPIO, CDC, SPI, flash
├── include/
│   ├── catsniffer.h          # State structures, enums, prototypes
│   ├── catsniffer_usbd.h     # USB device interface
│   ├── fw_metadata.h         # CC1352 FW ID storage API
│   ├── shell_commands.h      # Shell interface
│   └── fw_version.h.in       # Build-time version template
├── src/
│   ├── main.c                # USB, CDC handlers, LED animation, radio management
│   ├── shell_commands.c      # All 44 command handlers
│   ├── fw_metadata.c         # NVS persistent storage for CC1352 FW ID
│   └── USB/
│       └── usbd_init.c       # USB device initialization
├── scripts/
│   ├── catsniffer_build_flash_test.sh
│   ├── verify_endpoints.py
│   └── README.md
├── west.yml                  # Full Zephyr manifest
├── prj.conf                  # Zephyr Kconfig
└── CMakeLists.txt
```

### Key Kconfig Options (`prj.conf`)

```
CONFIG_USB_DEVICE_STACK_NEXT=y
CONFIG_USB_DEVICE_MANUFACTURER="Electronic Cats"
CONFIG_USB_DEVICE_PRODUCT="Catsniffer"
CONFIG_USB_DEVICE_VID=0x1209
CONFIG_USB_DEVICE_PID=0xBABB

CONFIG_LORA=y
CONFIG_LORA_SX126X=y

CONFIG_UART_INTERRUPT_DRIVEN=y
CONFIG_RING_BUFFER=y

CONFIG_NVS=y              # Persistent CC1352 FW ID storage
CONFIG_FLASH=y
CONFIG_FLASH_MAP=y
```

### Build Steps

```bash
# Full build
west build -s catsniffer -b rpi_pico -d build/rpi_pico

# Clean rebuild
west build -s catsniffer -b rpi_pico -d build/rpi_pico -p

# Flash (drag-and-drop)
cp build/rpi_pico/zephyr/zephyr.uf2 /Volumes/RPI-RP2/
```

### Firmware Size

Typical build:
- **Flash**: ~63 KB / 2 MB (3% usage)
- **RAM**: ~29 KB / 264 KB (11% usage)

---

## Development

### LED Indicators

| State | LED pattern |
|-------|-------------|
| Passthrough / Stream mode | LED2 slow blink — 1 s period |
| Boot mode / LoRa Command mode | LED1–3 fast cycle — 200 ms period |
| `identify` command active | All three LEDs rapid simultaneous blink — 100 ms on/off, ~2 s total |

### Testing

```bash
# Basic endpoint test
python3 scripts/verify_endpoints.py

# Full test suite
python3 scripts/verify_endpoints.py --test-all

# Test specific device
python3 scripts/verify_endpoints.py --device 1
```

### Adding New Shell Commands

1. Add a forward declaration in `shell_commands.c`:
```c
static void cmd_mycommand(char *args);
```

2. Add an entry to the command table:
```c
{ "mycommand", cmd_mycommand, "My command description", false },
```
Set the last field to `true` if the command takes arguments.

3. Implement the handler:
```c
static void cmd_mycommand(char *args)
{
    shell_reply("OK\r\n");
}
```

### Debugging

```bash
# Open Cat-Shell
screen /dev/ttyACM2 115200

# Check USB enumeration (macOS)
system_profiler SPUSBDataType | grep -A 10 "1209:babb"

# Check USB enumeration (Linux)
lsusb | grep 1209:babb

# Run endpoint verification
python3 scripts/verify_endpoints.py
```

---

## Troubleshooting

### Device Not Detected

```bash
# macOS
system_profiler SPUSBDataType | grep -i catsniffer

# Linux
lsusb | grep 1209:babb
```

**Manual reset to bootloader:**
1. Disconnect USB
2. Hold BOOTSEL button
3. Connect USB — `RPI-RP2` drive appears
4. Drop `zephyr.uf2` onto the drive

### Serial Port Permissions (Linux)

```bash
sudo usermod -a -G dialout $USER
# Log out and back in for the change to take effect
```

### LoRa Not Initializing

```bash
> status          # check: LoRa: initialized
> band3           # ensure LoRa RF path is selected
> lora_mode command
# then on Cat-LoRa:
> TEST            # should print: LoRa: Device ready
```

### CC1352 Not Responding

```bash
> exit            # ensure passthrough mode (not bootloader)
> status          # check mode and band
```

### FSK Not Working

```bash
> modulation fsk  # switch SX1262 to FSK
> band3           # ensure SX1262 RF path is selected
> fsk_config      # review current configuration
> fsk_apply       # ensure configuration is applied
```

### Commands Not Responding

1. Confirm baud rate: 115200 on all three ports
2. Line endings: `\n` or `\r\n` both accepted
3. Confirm you are on the correct port — use `fw_version` to verify Cat-Shell

### Build Errors

```bash
# Clean and rebuild
rm -rf build/rpi_pico
west build -s catsniffer -b rpi_pico -d build/rpi_pico

# Verify environment
west --version
echo $ZEPHYR_BASE
```

---

## Performance

### Throughput

- **CC1352 UART bridge**: up to 921600 baud (~115 KB/s)
- **LoRa** (SF7, 125 kHz): ~5.5 kbps
- **LoRa** (SF12, 125 kHz): ~250 bps
- **USB**: Full-speed (12 Mbps)

### Latency

- **USB → UART (CC1352)**: < 1 ms
- **LoRa air time** — SF7, 20 bytes: ~30 ms
- **LoRa air time** — SF12, 20 bytes: ~1.5 s
- **LoRa RX→TX turnaround**: ~5 ms (SX1262 hardware limitation)

### Power Consumption

- All radios active: ~150 mA @ 5 V
- CC1352 only: ~30 mA
- SX1262 TX at 22 dBm: ~120 mA

---

## Contributing

Contributions are welcome! Please ensure:

1. Code follows existing style (see `shell_commands.c` for patterns)
2. All tests pass: `python3 scripts/verify_endpoints.py --test-all`
3. Build succeeds without warnings
4. Documentation is updated to reflect any new commands or behavior

See [CONTRIBUTING.md](https://github.com/ElectronicCats/CatSniffer/blob/master/CONTRIBUTING.md) for the full contribution guide.

---

## License

Firmware: GNU AGPL v3.0
Hardware: CERN Open Hardware Licence v1.2
Electronic Cats is a registered trademark.

---

## References

- [Zephyr Project Documentation](https://docs.zephyrproject.org/)
- [RP2040 Datasheet](https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf)
- [CC1352P7 Product Page](https://www.ti.com/product/CC1352P7)
- [SX1262 Datasheet](https://www.semtech.com/products/wireless-rf/lora-core/sx1262)
- [Electronic Cats CatSniffer](https://github.com/ElectronicCats/CatSniffer)
- [CatSniffer Tools](https://github.com/ElectronicCats/CatSniffer-Tools)

---

## Support

- **Issues**: [GitHub Issues](https://github.com/ElectronicCats/CatSniffer-Firmware/issues)
- **Wiki**: [CatSniffer Wiki](https://github.com/ElectronicCats/CatSniffer/wiki)
- **Scripts**: See [scripts/README.md](scripts/README.md)

---

**Version**: 1.0.0
**Maintainer**: Electronic Cats

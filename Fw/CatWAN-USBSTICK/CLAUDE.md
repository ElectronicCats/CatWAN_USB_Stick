# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

CatSniffer firmware for RP2040 (Zephyr RTOS). Multi-protocol wireless sniffer with dual radios: CC1352P7 (Zigbee/Thread/BLE via UART passthrough) and SX1262 (LoRa/FSK via SPI). Exposes three USB CDC-ACM endpoints: Cat-Bridge (CDC0, CC1352 passthrough), Cat-LoRa (CDC1, SX1262 data), Cat-Shell (CDC2, configuration shell).

## Build Commands

```bash
# Environment setup (one-time per session)
source ~/zephyrproject/.venv/bin/activate
export ZEPHYR_BASE=$HOME/zephyrproject/zephyr

# Build (incremental)
west build -b rpi_pico

# Clean rebuild
west build -b rpi_pico -p always

# Build with custom firmware version label
CATSNIFFER_FW_VERSION=v3.1.0.0 west build -b rpi_pico -p always

# Flash via USB bootloader (copy .uf2 to RPI-RP2 mass storage drive)
cp build/zephyr/zephyr.uf2 /Volumes/RPI-RP2/

# All-in-one: build + flash + test
./scripts/catsniffer_build_flash_test.sh
# Individual stages: -c (compile), -f (flash), -t (test), --cjtag-test
# Pristine build: -p
```

## Testing

```bash
# Basic endpoint verification (requires device connected via USB)
python3 scripts/verify_endpoints.py

# Full test suite (all LoRa config commands + communication)
python3 scripts/verify_endpoints.py --test-all

# Test specific device when multiple connected
python3 scripts/verify_endpoints.py --device 1

# CC1352 JTAG smoke test
python3 scripts/test_cc1352_jtag.py

# Validate release firmware version format (vA.X.Y.Z)
./scripts/validate_fw_version.sh v3.1.0.0
```

All tests require physical hardware connected via USB. There are no unit tests that run without hardware.

## Architecture

### Source Layout

- `src/main.c` — Entry point, USB/CDC handlers, UART bridge (CC1352<->CDC0), LoRa RX/TX thread, LED animation, ring buffer plumbing, RF band switching
- `src/shell_commands.c` — Command table and all ~44 shell command handlers for CDC2. Commands are registered in a static table as `{name, handler, help, prefix_match}`
- `src/fw_metadata.c` — NVS-based persistent storage for CC1352 firmware ID tracking
- `src/cc1352_jtag.c` — Bit-banged cJTAG interface for CC1352 flash erase recovery
- `src/flash_compat.c` — Flash compatibility layer
- `src/USB/usbd_init.c` — Zephyr USB device stack initialization

### Key Patterns

- **Global state**: Single `catsniffer_t catsniffer` struct (defined in `catsniffer.h`) holds all runtime state — mode, band, LoRa/FSK config, packet loss counters
- **Ring buffers**: 6 ring buffers (16KB each) shuttle data between USB CDC endpoints and hardware UART/radio. Named `rb_{source}_to_{dest}` (e.g. `rb_cc1352_to_usb`)
- **Staged config**: LoRa and FSK parameters are staged in `lora_config_t`/`fsk_config_t` structs with a `config_pending` flag, then atomically applied via `apply_lora_config()`/`apply_fsk_config()`
- **LoRa thread**: Dedicated Zephyr thread (`LORA_THREAD_PRIORITY = K_PRIO_COOP(5)`) handles async RX/TX for both LoRa and FSK
- **Shell reply**: All shell command output goes through `shell_reply()` which writes to the CDC2 ring buffer

### Device Tree

`boards/rpi_pico.overlay` defines all hardware: 3 CDC-ACM UARTs, SPI0 for SX1262, GPIO pins for CC1352 control (reset/boot), RF switch (CTF1-3), LEDs, cJTAG pins. The SX1262 is aliased as `lora0`.

### West Manifest

`west.yml` (development) and `west-ci.yml` (CI, pinned revisions) pull a custom Zephyr fork (`wero1414/zephyr@fsk-native-driver`) that adds the native FSK driver for SX126x.

### Adding Shell Commands

1. Add forward declaration in `shell_commands.c`
2. Add entry to the command table: `{ "name", handler, "description", has_args }`
3. Implement handler — use `shell_reply()` for output

### Firmware Version

Controlled by `VERSION` file (semver) and CMake. Git SHA and dirty status are embedded at build time via `include/fw_version.h.in`. Override with `CATSNIFFER_FW_VERSION` env var or CMake cache variable. Release tags must match `vA.X.Y.Z` format (validated by `scripts/validate_fw_version.sh`).

# CatSniffer Verification Scripts

## Firmware Version Format

`scripts/validate_fw_version.sh` is the release/CI validator. It enforces release tags in
`vA.X.Y.Z` format.

The local build helper `scripts/catsniffer_build_flash_test.sh --fw-version <label>` is less
strict: it accepts any non-empty ASCII label made of letters, numbers, `.`, `_`, and `-`.
Examples: `jtag_test`, `dev.local`, `v3.1.0.0`

Release firmware versions must follow:

`vA.X.Y.Z`

- `A`: Compatible board category
- `X`: Major
- `Y`: Minor
- `Z`: Patch

Validation helper:

```bash
./scripts/validate_fw_version.sh v1.0.0.0
```

Current CI policy:

- `A` must be `3` (board family 3.1/3.2/3.3).
- First firmware release must be exactly `v3.1.0.0`.
- Subsequent releases must be `>= v3.1.0.0`.

## verify_endpoints.py

Multi-device verification tool for CatSniffer firmware testing.

### Features

- **Multi-device detection**: Automatically detects all connected CatSniffers
- **Endpoint verification**: Identifies Cat-Bridge, Cat-LoRa, and Cat-Shell ports
- **Command testing**: Tests all shell commands on each device
- **LoRa configuration**: Tests all new LoRa configuration commands
- **Parallel device support**: Can test multiple CatSniffers simultaneously

### Prerequisites

```bash
# Activate Zephyr virtual environment
source ~/zephyrproject/.venv/bin/activate

# Install required packages (already installed in venv)
pip install pyusb pyserial
```

### Usage

**Basic test (quick verification):**
```bash
python3 scripts/verify_endpoints.py
```

Tests:
- `help` command
- `status` command
- `lora_config` command
- `lora_mode` switching (stream/command)

**Full test suite:**
```bash
python3 scripts/verify_endpoints.py --test-all
```

Additional tests:
- `lora_freq` - Frequency configuration
- `lora_sf` - Spreading factor configuration
- `lora_bw` - Bandwidth configuration
- `lora_cr` - Coding rate configuration
- `lora_power` - TX power configuration
- LoRa communication (TEST, TXTEST commands)

**Test specific device:**
```bash
python3 scripts/verify_endpoints.py --device 1
python3 scripts/verify_endpoints.py --device 2 --test-all
```

### Example Output

```
======================================================================
CATSNIFFER MULTI-DEVICE VERIFICATION TOOL
======================================================================

Searching for CatSniffers (VID:1209 PID:BABB)...
Found 1 CatSniffer device(s)

======================================================================
DETECTED CATSNIFFERS
======================================================================

CatSniffer #1
  Cat-Bridge (CC1352): /dev/cu.usbmodem2101
  Cat-LoRa (SX1262):   /dev/cu.usbmodem2103
  Cat-Shell (Config):  /dev/cu.usbmodem2105

======================================================================
RUNNING TESTS
======================================================================

[1/4] Testing 'help' command...
  ✓ PASS: Help command works

[2/4] Testing 'status' command...
  ✓ PASS: Status command works
  Response: Mode: 0, Band: 0, LoRa: initialized, LoRa Mode: Stream

[3/4] Testing 'lora_config' command...
  ✓ PASS: LoRa config command works

[4/4] Testing 'lora_mode' command...
  ✓ PASS: LoRa mode switch to stream works
  ✓ PASS: LoRa mode switch to command works

======================================================================
TEST SUMMARY
======================================================================

CatSniffer #1:
  Basic Commands:        ✓ PASS

======================================================================
✓ ALL TESTS PASSED
======================================================================
```

### Tested Commands

#### Shell Commands (CDC2)
- [x] `help` - List all commands
- [x] `status` - Show device status
- [x] `boot` - Enter CC1352 bootloader
- [x] `exit` - Return to passthrough
- [x] `band1` - Switch to 2.4GHz
- [x] `band2` - Switch to Sub-GHz
- [x] `band3` - Switch to LoRa
- [x] `reboot` - Enter RP2040 bootloader

#### LoRa Configuration Commands (CDC2)
- [x] `lora_freq <Hz>` - Set frequency
- [x] `lora_sf <7-12>` - Set spreading factor
- [x] `lora_bw <125|250|500>` - Set bandwidth
- [x] `lora_cr <5|6|7|8>` - Set coding rate
- [x] `lora_power <-9 to 22>` - Set TX power
- [x] `lora_mode <stream|command>` - Switch LoRa mode
- [x] `lora_config` - Display configuration
- [x] `lora_apply` - Apply pending changes

#### LoRa Commands (CDC1 in command mode)
- [x] `TEST` - Initialize LoRa
- [x] `TXTEST` - Send test packet
- [x] `TX <hex>` - Send hex-encoded data

### Multi-Device Support

The script automatically detects multiple CatSniffers connected via USB:

```bash
# Test all connected devices
python3 scripts/verify_endpoints.py --test-all

# Output:
# CatSniffer #1: ✓ PASS
# CatSniffer #2: ✓ PASS
# CatSniffer #3: ✓ PASS
```

### Exit Codes

- `0` - All tests passed
- `1` - One or more tests failed or no devices found

### Notes

- LoRa communication tests require LoRa to be initialized first
- The script resets LoRa configuration to defaults after testing
- All serial operations use 115200 baud rate
- Timeout values are optimized for reliable communication

### Troubleshooting

**No devices found:**
```bash
# Check USB connection
system_profiler SPUSBDataType | grep -A 10 "1209:babb"

# Check serial ports
ls /dev/cu.usbmodem*
```

**Permission errors (Linux):**
```bash
sudo usermod -a -G dialout $USER
# Log out and back in
```

**Import errors:**
```bash
pip install pyusb pyserial
```

## test_cc1352_jtag.py

Automates the CC1352 JTAG smoke sequence on the shell endpoint (CDC2), with retries.

### Usage

```bash
python3 scripts/test_cc1352_jtag.py
python3 scripts/test_cc1352_jtag.py --device 1 --attempts 5
python3 scripts/test_cc1352_jtag.py --expect-fw cjtag-test
python3 scripts/test_cc1352_jtag.py --quiet
python3 scripts/test_cc1352_jtag.py --destructive-erase
```

### What it runs

1. `fw_version`
2. `cc1352_jtag diag`
3. `cc1352_jtag status`
4. `cc1352_jtag init`
5. `cc1352_jtag id`
6. `cc1352_jtag halt`
7. `cc1352_jtag read32 0xE000ED00`
8. `cc1352_jtag read32 0xE000EDF0`
9. `cc1352_jtag regs`
10. `cc1352_jtag status`
11. Optional: `cc1352_jtag erase_all confirm`

### Pass criteria

- `diag`, `init`, `id`, `halt`, `read32`, and `regs` return no `ERR`.
- `id` returns `OK JRC IDCODE`.
- `halt` returns `OK CPU halted`.
- `read32 0xE000ED00` returns a plausible Cortex-M CPUID.
- `regs` returns at least `SP` and `PC`.
- final `status` reports chain detected.

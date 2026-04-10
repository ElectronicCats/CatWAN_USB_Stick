#!/bin/bash
# Catsniffer staged build/flash/test script

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
UF2_FILE="$PROJECT_DIR/build/zephyr/zephyr.uf2"

DO_COMPILE=0
DO_FLASH=0
DO_TEST=0
DO_CJTAG_TEST=0
PRISTINE_BUILD=0
SEND_REBOOT=1
VERBOSE=0
SHELL_PORT_OVERRIDE=""
ALL_DEVICES=0
FW_VERSION_OVERRIDE=""
CJTAG_ATTEMPTS=""
CJTAG_DEVICE=""
CJTAG_QUIET=0

usage() {
    cat <<'EOF'
Usage: catsniffer_build_flash_test.sh [flags]

Main flags:
  -c    Compile only
  -f    Flash only
  -t    Test only (runs verify_endpoints.py)
  --cjtag-test  Run CC1352 JTAG smoke test (scripts/test_cc1352_jtag.py)

If no -c/-f/-t is provided, script runs all three stages: compile + flash + test.

Useful flags:
  --all Flash all detected CatSniffers (all RPI-RP2* mounts)
  --fw-version <ver> Set firmware version label (local labels like jtag_test are allowed)
  --cjtag-attempts <n> Retry count for --cjtag-test
  --cjtag-device <n> Device index for --cjtag-test
  --cjtag-quiet Quiet mode for --cjtag-test
  -p    Pristine build (west build -p always)
  -n    Do not send "reboot" command before flashing
  -s <port>  Override shell serial port (example: /dev/cu.usbmodem1205)
  -u <path>  Override UF2 path
  -v    Verbose shell tracing
  -h    Show this help
EOF
}

validate_firmware_label() {
    local version="$1"

    if [[ -z "$version" ]]; then
        echo "ERROR: --fw-version requires a non-empty value"
        exit 1
    fi

    if [[ "$version" =~ ^[A-Za-z0-9][A-Za-z0-9._-]*$ ]]; then
        return 0
    fi

    echo "ERROR: Invalid firmware label: $version"
    echo "Allowed characters: letters, numbers, dot (.), underscore (_), dash (-)"
    echo "Examples: jtag_test, dev.local, v3.1.0.0"
    exit 1
}

# Parse long flags first
ARGS=()
while [[ $# -gt 0 ]]; do
    case "$1" in
        --all)
            ALL_DEVICES=1
            shift
            ;;
        --fw-version)
            if [[ $# -lt 2 ]]; then
                echo "ERROR: --fw-version requires a value"
                usage
                exit 1
            fi
            FW_VERSION_OVERRIDE="$2"
            shift 2
            ;;
        --cjtag-test)
            DO_CJTAG_TEST=1
            shift
            ;;
        --cjtag-attempts)
            if [[ $# -lt 2 ]]; then
                echo "ERROR: --cjtag-attempts requires a value"
                usage
                exit 1
            fi
            CJTAG_ATTEMPTS="$2"
            shift 2
            ;;
        --cjtag-device)
            if [[ $# -lt 2 ]]; then
                echo "ERROR: --cjtag-device requires a value"
                usage
                exit 1
            fi
            CJTAG_DEVICE="$2"
            shift 2
            ;;
        --cjtag-quiet)
            CJTAG_QUIET=1
            shift
            ;;
        *)
            ARGS+=("$1")
            shift
            ;;
    esac
done
set -- "${ARGS[@]}"

while getopts ":cftpns:u:vh" opt; do
    case "$opt" in
        c) DO_COMPILE=1 ;;
        f) DO_FLASH=1 ;;
        t) DO_TEST=1 ;;
        p) PRISTINE_BUILD=1 ;;
        n) SEND_REBOOT=0 ;;
        s) SHELL_PORT_OVERRIDE="$OPTARG" ;;
        u) UF2_FILE="$OPTARG" ;;
        v) VERBOSE=1 ;;
        h) usage; exit 0 ;;
        \?) echo "ERROR: Invalid option -$OPTARG"; usage; exit 1 ;;
        :) echo "ERROR: Option -$OPTARG requires an argument"; usage; exit 1 ;;
    esac
done

if [[ $VERBOSE -eq 1 ]]; then
    set -x
fi

if [[ $DO_COMPILE -eq 0 && $DO_FLASH -eq 0 && $DO_TEST -eq 0 && $DO_CJTAG_TEST -eq 0 ]]; then
    DO_COMPILE=1
    DO_FLASH=1
    DO_TEST=1
fi

if [[ -n "$FW_VERSION_OVERRIDE" ]]; then
    validate_firmware_label "$FW_VERSION_OVERRIDE"
fi

LINUX_USER="${USER:-$(id -un)}"

if [[ "$OSTYPE" == "darwin"* ]]; then
    MOUNT_POINT="/Volumes/RPI-RP2"
    MOUNT_GLOB="/Volumes/RPI-RP2*"
    SERIAL_PATTERN="/dev/cu.usbmodem*"
elif [[ "$OSTYPE" == "linux"* ]]; then
    MOUNT_POINT="/media/$LINUX_USER/RPI-RP2"
    MOUNT_GLOB="/media/$LINUX_USER/RPI-RP2*"
    SERIAL_PATTERN="/dev/ttyACM*"
else
    echo "ERROR: Unsupported OS: $OSTYPE"
    exit 1
fi

list_shell_ports() {
    ls $SERIAL_PATTERN 2>/dev/null | sort || true
}

find_shell_port() {
    if [[ -n "$SHELL_PORT_OVERRIDE" ]]; then
        echo "$SHELL_PORT_OVERRIDE"
    else
        ls $SERIAL_PATTERN 2>/dev/null | sort | tail -1 || true
    fi
}

list_mount_points() {
    compgen -G "$MOUNT_GLOB" || true
}

read_lines_into_array() {
    local __var_name="$1"
    local __line
    eval "$__var_name=()"
    while IFS= read -r __line; do
        [[ -n "$__line" ]] || continue
        eval "$__var_name+=(\"\$__line\")"
    done
}

wait_for_mount() {
    local timeout=30
    local elapsed=0
    while [[ ! -d "$MOUNT_POINT" ]]; do
        sleep 1
        elapsed=$((elapsed + 1))
        if [[ $elapsed -ge $timeout ]]; then
            echo "ERROR: Timeout waiting for $MOUNT_POINT"
            echo "       Try: hold BOOTSEL and plug USB manually"
            return 1
        fi
        printf "\r      Waiting... %ds" "$elapsed"
    done
    echo ""
}

wait_for_any_mount() {
    local timeout=30
    local elapsed=0
    while [[ -z "$(list_mount_points)" ]]; do
        sleep 1
        elapsed=$((elapsed + 1))
        if [[ $elapsed -ge $timeout ]]; then
            echo "ERROR: Timeout waiting for any mount matching $MOUNT_GLOB"
            echo "       Try: hold BOOTSEL and plug USB manually"
            return 1
        fi
        printf "\r      Waiting for any board... %ds" "$elapsed"
    done
    echo ""
}

wait_for_serial_ports() {
    local needed=3
    local timeout=15
    local elapsed=0
    while [[ $(ls $SERIAL_PATTERN 2>/dev/null | wc -l | tr -d ' ') -lt $needed ]]; do
        sleep 1
        elapsed=$((elapsed + 1))
        if [[ $elapsed -ge $timeout ]]; then
            echo "ERROR: Device did not enumerate properly"
            return 1
        fi
    done
}

echo "=== Catsniffer Build/Flash/Test ==="
echo "Platform: $OSTYPE"
echo "Compile: $DO_COMPILE | Flash: $DO_FLASH | Test: $DO_TEST"
echo "cJTAG test: $DO_CJTAG_TEST"
echo "All devices: $ALL_DEVICES"
if [[ -n "$FW_VERSION_OVERRIDE" ]]; then
    echo "FW version override: $FW_VERSION_OVERRIDE"
fi
cd "$PROJECT_DIR"

if [[ $DO_COMPILE -eq 1 ]]; then
    echo ""
    echo "[compile] Building firmware..."
    source "$HOME/zephyrproject/.venv/bin/activate"
    export ZEPHYR_BASE="$HOME/zephyrproject/zephyr"
    mkdir -p "$PROJECT_DIR/.cache/zephyr"
    mkdir -p "$PROJECT_DIR/.cache/ccache/tmp"
    export CCACHE_DIR="$PROJECT_DIR/.cache/ccache"
    export CCACHE_TEMPDIR="$PROJECT_DIR/.cache/ccache/tmp"
    CMAKE_ARGS=("-DUSER_CACHE_DIR=$PROJECT_DIR/.cache/zephyr")
    if [[ -n "$FW_VERSION_OVERRIDE" ]]; then
        export CATSNIFFER_FW_VERSION="$FW_VERSION_OVERRIDE"
        CMAKE_ARGS+=("-DCATSNIFFER_FW_VERSION=$FW_VERSION_OVERRIDE")
    fi

    if [[ $PRISTINE_BUILD -eq 1 ]]; then
        echo "      Mode: pristine rebuild"
        west build -p always -b rpi_pico -- "${CMAKE_ARGS[@]}"
    else
        if [[ -d "$PROJECT_DIR/build" && ! -f "$PROJECT_DIR/build/build.ninja" ]]; then
            echo "      Build dir is incomplete (missing build.ninja). Regenerating pristine..."
            west build -p always -b rpi_pico -- "${CMAKE_ARGS[@]}"
        else
            echo "      Mode: incremental rebuild"
            west build -b rpi_pico -- "${CMAKE_ARGS[@]}"
        fi
    fi

    if [[ ! -f "$UF2_FILE" ]]; then
        echo "ERROR: Build failed - UF2 not found at $UF2_FILE"
        exit 1
    fi
    echo "Build successful: $(ls -lh "$UF2_FILE" | awk '{print $5}')"
fi

if [[ $DO_FLASH -eq 1 ]]; then
    if [[ ! -f "$UF2_FILE" ]]; then
        echo "ERROR: UF2 not found: $UF2_FILE"
        echo "       Run with -c first or pass -u <uf2 path>"
        exit 1
    fi

    echo ""
    echo "[flash] Preparing board..."

    if [[ $ALL_DEVICES -eq 1 ]]; then
        if [[ -n "$SHELL_PORT_OVERRIDE" ]]; then
            echo "WARNING: -s/--shell-port is ignored when using --all"
        fi

        if [[ $SEND_REBOOT -eq 1 ]]; then
            read_lines_into_array ALL_PORTS < <(list_shell_ports)
            if [[ ${#ALL_PORTS[@]} -gt 0 ]]; then
                echo "      Sending 'reboot' to ${#ALL_PORTS[@]} detected serial ports..."
                for port in "${ALL_PORTS[@]}"; do
                    echo "        -> $port"
                    echo "reboot" > "$port" 2>/dev/null || true
                done
                sleep 2
            else
                echo "      No serial ports detected. Continuing..."
            fi
        fi

        echo "[flash] Waiting for any RPI-RP2 mount..."
        wait_for_any_mount

        read_lines_into_array MOUNTS < <(list_mount_points)
        if [[ ${#MOUNTS[@]} -eq 0 ]]; then
            echo "ERROR: No mount points found for pattern: $MOUNT_GLOB"
            exit 1
        fi

        echo "      Flashing ${#MOUNTS[@]} detected board mount(s)..."
        for mp in "${MOUNTS[@]}"; do
            echo "        -> $mp"
            cp "$UF2_FILE" "$mp/"
        done
        echo "      Flash complete for all detected mounts."
    else
        if [[ $SEND_REBOOT -eq 1 ]]; then
            SHELL_PORT="$(find_shell_port)"
            if [[ -n "$SHELL_PORT" ]]; then
                echo "      Sending 'reboot' to $SHELL_PORT"
                echo "reboot" > "$SHELL_PORT" 2>/dev/null || true
                sleep 1
            else
                echo "      No serial shell port found. Continuing..."
            fi
        fi

        echo "[flash] Waiting for $MOUNT_POINT..."
        wait_for_mount

        echo "      Copying $(basename "$UF2_FILE") to boot drive..."
        cp "$UF2_FILE" "$MOUNT_POINT/"
        echo "      Flash complete."
    fi
fi

if [[ $DO_TEST -eq 1 ]]; then
    echo ""
    echo "[test] Running endpoint verification..."
    if [[ $DO_FLASH -eq 1 ]]; then
        echo "      Waiting for reboot..."
        sleep 3
    fi

    if [[ $ALL_DEVICES -eq 1 ]]; then
        echo "      WARNING: --all test runs a single verify pass (not per-device)."
    fi

    wait_for_serial_ports
    sleep 2

    SHELL_PORT="$(find_shell_port)"
    if [[ -n "$SHELL_PORT" ]]; then
        for _ in 1 2 3; do
            echo "" > "$SHELL_PORT" 2>/dev/null || true
            sleep 1
        done
    fi

    python3 "$SCRIPT_DIR/verify_endpoints.py"
fi

if [[ $DO_CJTAG_TEST -eq 1 ]]; then
    echo ""
    echo "[cjtag-test] Running CC1352 JTAG smoke test..."

    if [[ $DO_FLASH -eq 1 ]]; then
        echo "      Waiting for reboot..."
        sleep 3
    fi

    wait_for_serial_ports

    CJTAG_ARGS=()
    if [[ -n "$CJTAG_ATTEMPTS" ]]; then
        CJTAG_ARGS+=("--attempts" "$CJTAG_ATTEMPTS")
    fi
    if [[ -n "$CJTAG_DEVICE" ]]; then
        CJTAG_ARGS+=("--device" "$CJTAG_DEVICE")
    fi
    if [[ $CJTAG_QUIET -eq 1 ]]; then
        CJTAG_ARGS+=("--quiet")
    fi
    if [[ -n "$FW_VERSION_OVERRIDE" ]]; then
        CJTAG_ARGS+=("--expect-fw" "$FW_VERSION_OVERRIDE")
    fi

    python3 "$SCRIPT_DIR/test_cc1352_jtag.py" "${CJTAG_ARGS[@]}"
fi

echo ""
echo "=== Done ==="

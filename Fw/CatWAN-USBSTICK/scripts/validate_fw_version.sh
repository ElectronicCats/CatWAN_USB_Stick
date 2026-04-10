#!/bin/bash
# Validate CatSniffer firmware version format: vA.X.Y.Z

set -euo pipefail

usage() {
    cat <<'EOF'
Usage: validate_fw_version.sh <version> [--require-board <A>] [--require-exact <vA.X.Y.Z>] [--min-version <vA.X.Y.Z>]

Expected format:
  vA.X.Y.Z
Where:
  A = Compatible board category
  X = Major
  Y = Minor
  Z = Patch

Examples:
  v1.0.0.0
  v2.3.4.5
EOF
}

if [[ $# -lt 1 ]]; then
    echo "ERROR: Missing firmware version argument."
    usage
    exit 1
fi

VERSION="$1"
shift

REQUIRE_BOARD=""
REQUIRE_EXACT=""
MIN_VERSION=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --require-board)
            if [[ $# -lt 2 ]]; then
                echo "ERROR: --require-board requires a value."
                exit 1
            fi
            REQUIRE_BOARD="$2"
            shift 2
            ;;
        --require-exact)
            if [[ $# -lt 2 ]]; then
                echo "ERROR: --require-exact requires a value."
                exit 1
            fi
            REQUIRE_EXACT="$2"
            shift 2
            ;;
        --min-version)
            if [[ $# -lt 2 ]]; then
                echo "ERROR: --min-version requires a value."
                exit 1
            fi
            MIN_VERSION="$2"
            shift 2
            ;;
        *)
            echo "ERROR: Unknown argument: $1"
            usage
            exit 1
            ;;
    esac
done

if [[ "$VERSION" =~ ^v([0-9]+)\.([0-9]+)\.([0-9]+)\.([0-9]+)$ ]]; then
    BOARD_COMPAT="${BASH_REMATCH[1]}"
    MAJOR="${BASH_REMATCH[2]}"
    MINOR="${BASH_REMATCH[3]}"
    PATCH="${BASH_REMATCH[4]}"

    if [[ -n "$REQUIRE_BOARD" && "$BOARD_COMPAT" != "$REQUIRE_BOARD" ]]; then
        echo "ERROR: Invalid board compatibility category in $VERSION"
        echo "Expected A=$REQUIRE_BOARD but got A=$BOARD_COMPAT"
        exit 1
    fi

    if [[ -n "$REQUIRE_EXACT" && "$VERSION" != "$REQUIRE_EXACT" ]]; then
        echo "ERROR: Version must be exactly $REQUIRE_EXACT for this release stage"
        echo "Got: $VERSION"
        exit 1
    fi

    if [[ -n "$MIN_VERSION" ]]; then
        if [[ ! "$MIN_VERSION" =~ ^v[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
            echo "ERROR: --min-version must use vA.X.Y.Z format"
            exit 1
        fi
        if [[ "$(printf '%s\n%s\n' "$MIN_VERSION" "$VERSION" | sort -V | head -n1)" != "$MIN_VERSION" ]]; then
            echo "ERROR: Version $VERSION is lower than minimum allowed $MIN_VERSION"
            exit 1
        fi
    fi

    echo "Firmware version is valid: $VERSION"
    echo "  A (board compat): $BOARD_COMPAT"
    echo "  X (major):        $MAJOR"
    echo "  Y (minor):        $MINOR"
    echo "  Z (patch):        $PATCH"
    exit 0
fi

echo "ERROR: Invalid firmware version: $VERSION"
echo "Expected format: vA.X.Y.Z (example: v1.0.0.0)"
exit 1

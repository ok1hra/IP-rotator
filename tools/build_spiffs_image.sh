#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

DATA_DIR="${ROOT_DIR}/data"
OUTPUT_DIR="${ROOT_DIR}/build"
OUTPUT_FILE="${OUTPUT_DIR}/spiffs.bin"
PARTITIONS_CSV="${ROOT_DIR}/partitions.csv"
SKETCH_FILE="${ROOT_DIR}/IP-rotator.ino"
ESP32_CORE_ROOT="${ESP32_CORE_ROOT:-}"
MKSPIFFS_BIN="${MKSPIFFS_BIN:-}"

detect_esp32_core_root() {
  local candidates=()
  if [[ -n "${ESP32_CORE_ROOT}" ]]; then
    candidates+=("${ESP32_CORE_ROOT}")
  fi
  if [[ -n "${HOME:-}" ]]; then
    candidates+=("${HOME}/Arduino/hardware/espressif/esp32")
    candidates+=("${HOME}/.arduino15/packages/esp32/hardware/esp32")
  fi

  local candidate=""
  local version_dir=""

  for candidate in "${candidates[@]}"; do
    if [[ -x "${candidate}/tools/mkspiffs/mkspiffs" ]]; then
      echo "${candidate}"
      return 0
    fi
    if [[ -d "${candidate}" ]]; then
      version_dir="$(find "${candidate}" -mindepth 1 -maxdepth 1 -type d | sort | tail -n 1)"
      if [[ -n "${version_dir}" && -x "${version_dir}/tools/mkspiffs/mkspiffs" ]]; then
        echo "${version_dir}"
        return 0
      fi
    fi
  done

  return 1
}

detect_mkspiffs_bin() {
  local candidates=()
  if [[ -n "${MKSPIFFS_BIN}" ]]; then
    candidates+=("${MKSPIFFS_BIN}")
  fi
  if [[ -n "${ESP32_CORE_ROOT}" ]]; then
    candidates+=("${ESP32_CORE_ROOT}/tools/mkspiffs/mkspiffs")
  fi
  if [[ -n "${HOME:-}" ]]; then
    candidates+=("${HOME}/Arduino/hardware/espressif/esp32/tools/mkspiffs/mkspiffs")
  fi

  local candidate=""
  for candidate in "${candidates[@]}"; do
    if [[ -x "${candidate}" ]]; then
      echo "${candidate}"
      return 0
    fi
  done

  if [[ -n "${HOME:-}" ]]; then
    candidate="$(find "${HOME}/.arduino15/packages" -path '*/tools/mkspiffs/*/mkspiffs' -type f -executable 2>/dev/null | sort -V | tail -n 1)"
    if [[ -n "${candidate}" ]]; then
      echo "${candidate}"
      return 0
    fi

    candidate="$(find "${HOME}/.platformio/packages" -path '*/tool-mkspiffs/mkspiffs*' -type f -executable 2>/dev/null | sort -V | tail -n 1)"
    if [[ -n "${candidate}" ]]; then
      echo "${candidate}"
      return 0
    fi
  fi

  return 1
}

usage() {
  cat <<'EOF'
Usage: tools/build_spiffs_image.sh [options]

Build a SPIFFS image from the local data/ directory for ESP32 OTA upload.

Options:
  --data-dir PATH        Source directory to pack (default: ./data)
  --output PATH          Output image path (default: ./build/spiffs.bin)
  --partitions PATH      Partition CSV to read SPIFFS size/offset from
  --mkspiffs PATH        mkspiffs binary path
  -h, --help             Show this help
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --data-dir)
      DATA_DIR="$2"
      shift 2
      ;;
    --output)
      OUTPUT_FILE="$2"
      OUTPUT_DIR="$(dirname "$OUTPUT_FILE")"
      shift 2
      ;;
    --partitions)
      PARTITIONS_CSV="$2"
      shift 2
      ;;
    --mkspiffs)
      MKSPIFFS_BIN="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

if [[ -z "${ESP32_CORE_ROOT}" ]]; then
  ESP32_CORE_ROOT="$(detect_esp32_core_root || true)"
fi

if [[ -z "${MKSPIFFS_BIN}" ]]; then
  MKSPIFFS_BIN="$(detect_mkspiffs_bin || true)"
fi

if [[ ! -d "$DATA_DIR" ]]; then
  echo "Data directory not found: $DATA_DIR" >&2
  exit 1
fi

if [[ ! -f "$PARTITIONS_CSV" ]]; then
  echo "Partition CSV not found: $PARTITIONS_CSV" >&2
  exit 1
fi

if [[ ! -x "$MKSPIFFS_BIN" ]]; then
  echo "mkspiffs binary not found or not executable: $MKSPIFFS_BIN" >&2
  exit 1
fi

if [[ ! -f "$SKETCH_FILE" ]]; then
  echo "Sketch file not found: $SKETCH_FILE" >&2
  exit 1
fi

SPIFFS_ROW="$(
  awk -F',' '
    $0 !~ /^[[:space:]]*#/ {
      name=$1; type=$2; subtype=$3; offset=$4; size=$5;
      gsub(/[[:space:]]/, "", name);
      gsub(/[[:space:]]/, "", type);
      gsub(/[[:space:]]/, "", subtype);
      gsub(/[[:space:]]/, "", offset);
      gsub(/[[:space:]]/, "", size);
      if (type == "data" && subtype == "spiffs") {
        print name "," offset "," size;
        exit 0;
      }
    }
  ' "$PARTITIONS_CSV"
)"

if [[ -z "$SPIFFS_ROW" ]]; then
  echo "No SPIFFS partition found in: $PARTITIONS_CSV" >&2
  exit 1
fi

IFS=',' read -r SPIFFS_NAME SPIFFS_OFFSET SPIFFS_SIZE <<< "$SPIFFS_ROW"

FW_REV="$(
  awk -F'"' '
    /const char\* REV = "/ {
      print $2;
      exit 0;
    }
  ' "$SKETCH_FILE"
)"

if [[ -z "$FW_REV" ]]; then
  echo "Could not read REV from: $SKETCH_FILE" >&2
  exit 1
fi

mkdir -p "$OUTPUT_DIR"

TMP_DATA_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DATA_DIR"' EXIT

cp -R "${DATA_DIR}/." "$TMP_DATA_DIR/"
cat > "${TMP_DATA_DIR}/fs_build.txt" <<EOF
REV=$FW_REV
SPIFFS_OFFSET=$SPIFFS_OFFSET
SPIFFS_SIZE=$SPIFFS_SIZE
EOF

"$MKSPIFFS_BIN" -c "$TMP_DATA_DIR" -b 4096 -p 256 -s "$SPIFFS_SIZE" "$OUTPUT_FILE"

echo "SPIFFS image created: $OUTPUT_FILE"
echo "Partition: $SPIFFS_NAME"
echo "Offset:    $SPIFFS_OFFSET"
echo "Size:      $SPIFFS_SIZE"
echo "FW REV:    $FW_REV"
echo "Upload via web OTA as filesystem image."

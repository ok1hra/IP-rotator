#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

OUTPUT_DIR="${ROOT_DIR}/build/recovery-web"
FIRMWARE_BIN="${ROOT_DIR}/IP-rotator.ino.esp32-poe.bin"
SPIFFS_BIN="${ROOT_DIR}/build/spiffs.bin"
PARTITIONS_CSV="${ROOT_DIR}/partitions.csv"
REV_FILE="${ROOT_DIR}/IP-rotator.ino"

ESP32_CORE_ROOT="${ESP32_CORE_ROOT:-}"
BOOTLOADER_BIN="${BOOTLOADER_BIN:-}"
BOOT_APP0_BIN="${BOOT_APP0_BIN:-}"
GEN_PART_BIN="${GEN_PART_BIN:-}"
ESPTOOL_BIN="${ESPTOOL_BIN:-}"

FLASH_MODE="dio"
FLASH_FREQ="40m"
FLASH_SIZE="4MB"

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
    if [[ -f "${candidate}/tools/esptool.py" ]]; then
      echo "${candidate}"
      return 0
    fi
    if [[ -d "${candidate}" ]]; then
      version_dir="$(find "${candidate}" -mindepth 1 -maxdepth 1 -type d | sort | tail -n 1)"
      if [[ -n "${version_dir}" && -f "${version_dir}/tools/esptool.py" ]]; then
        echo "${version_dir}"
        return 0
      fi
    fi
  done

  return 1
}

usage() {
  cat <<'EOF'
Usage: tools/build_recovery_package.sh [options]

Build a complete ESP32 recovery package for blank-chip flashing and browser-based install.

Options:
  --output-dir PATH      Output directory (default: ./build/recovery-web)
  --firmware PATH        Sketch firmware binary (default: ./IP-rotator.ino.esp32-poe.bin)
  --spiffs PATH          SPIFFS image (default: ./build/spiffs.bin)
  --partitions PATH      Partition CSV (default: ./partitions.csv)
  --bootloader PATH      Bootloader binary
  --boot-app0 PATH       boot_app0.bin path
  --gen-part PATH        gen_esp32part.py path
  --esptool PATH         esptool.py path
  -h, --help             Show this help
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --output-dir)
      OUTPUT_DIR="$2"
      shift 2
      ;;
    --firmware)
      FIRMWARE_BIN="$2"
      shift 2
      ;;
    --spiffs)
      SPIFFS_BIN="$2"
      shift 2
      ;;
    --partitions)
      PARTITIONS_CSV="$2"
      shift 2
      ;;
    --bootloader)
      BOOTLOADER_BIN="$2"
      shift 2
      ;;
    --boot-app0)
      BOOT_APP0_BIN="$2"
      shift 2
      ;;
    --gen-part)
      GEN_PART_BIN="$2"
      shift 2
      ;;
    --esptool)
      ESPTOOL_BIN="$2"
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

if [[ -z "${BOOTLOADER_BIN}" && -n "${ESP32_CORE_ROOT}" ]]; then
  BOOTLOADER_BIN="${ESP32_CORE_ROOT}/tools/sdk/esp32/bin/bootloader_dio_40m.bin"
fi
if [[ -z "${BOOT_APP0_BIN}" && -n "${ESP32_CORE_ROOT}" ]]; then
  BOOT_APP0_BIN="${ESP32_CORE_ROOT}/tools/partitions/boot_app0.bin"
fi
if [[ -z "${GEN_PART_BIN}" && -n "${ESP32_CORE_ROOT}" ]]; then
  GEN_PART_BIN="${ESP32_CORE_ROOT}/tools/gen_esp32part.py"
fi
if [[ -z "${ESPTOOL_BIN}" && -n "${ESP32_CORE_ROOT}" ]]; then
  ESPTOOL_BIN="${ESP32_CORE_ROOT}/tools/esptool.py"
fi

require_file() {
  local path="$1"
  local label="$2"
  if [[ ! -f "$path" ]]; then
    echo "$label not found: $path" >&2
    exit 1
  fi
}

require_file "$FIRMWARE_BIN" "Firmware binary"
require_file "$SPIFFS_BIN" "SPIFFS binary"
require_file "$PARTITIONS_CSV" "Partition CSV"
require_file "$BOOTLOADER_BIN" "Bootloader binary"
require_file "$BOOT_APP0_BIN" "boot_app0 binary"
require_file "$GEN_PART_BIN" "Partition generator"
require_file "$ESPTOOL_BIN" "esptool"
require_file "$REV_FILE" "Sketch file"

FW_REV="$(
  awk -F'"' '
    /const char\* REV = "/ {
      print $2;
      exit 0;
    }
  ' "$REV_FILE"
)"

if [[ -z "$FW_REV" ]]; then
  echo "Could not read REV from: $REV_FILE" >&2
  exit 1
fi

REV_FIRMWARE_NAME="ip-rotator-${FW_REV}-esp32-poe-firmware.bin"
REV_SPIFFS_NAME="ip-rotator-${FW_REV}-esp32-poe-spiffs.bin"
REV_MERGED_NAME="ip-rotator-${FW_REV}-esp32-poe-recovery-merged.bin"

SPIFFS_INFO="$(
  awk -F',' '
    $0 !~ /^[[:space:]]*#/ {
      name=$1; type=$2; subtype=$3; offset=$4; size=$5;
      gsub(/[[:space:]]/, "", name);
      gsub(/[[:space:]]/, "", type);
      gsub(/[[:space:]]/, "", subtype);
      gsub(/[[:space:]]/, "", offset);
      gsub(/[[:space:]]/, "", size);
      if (type == "data" && subtype == "spiffs") {
        print offset "," size;
        exit 0;
      }
    }
  ' "$PARTITIONS_CSV"
)"

if [[ -z "$SPIFFS_INFO" ]]; then
  echo "No SPIFFS partition found in: $PARTITIONS_CSV" >&2
  exit 1
fi

IFS=',' read -r SPIFFS_OFFSET SPIFFS_SIZE_HEX <<< "$SPIFFS_INFO"
SPIFFS_OFFSET_DEC="$((SPIFFS_OFFSET))"

mkdir -p "$OUTPUT_DIR"

PARTITIONS_BIN="${OUTPUT_DIR}/partitions.bin"
python3 "$GEN_PART_BIN" "$PARTITIONS_CSV" "$PARTITIONS_BIN"

cp "$BOOTLOADER_BIN" "${OUTPUT_DIR}/bootloader.bin"
cp "$BOOT_APP0_BIN" "${OUTPUT_DIR}/boot_app0.bin"
cp "$FIRMWARE_BIN" "${OUTPUT_DIR}/firmware.bin"
cp "$SPIFFS_BIN" "${OUTPUT_DIR}/spiffs.bin"
cp "$FIRMWARE_BIN" "${OUTPUT_DIR}/${REV_FIRMWARE_NAME}"
cp "$SPIFFS_BIN" "${OUTPUT_DIR}/${REV_SPIFFS_NAME}"

python3 "$ESPTOOL_BIN" --chip esp32 merge_bin \
  -o "${OUTPUT_DIR}/recovery-merged.bin" \
  --flash_mode "$FLASH_MODE" \
  --flash_freq "$FLASH_FREQ" \
  --flash_size "$FLASH_SIZE" \
  0x1000 "${OUTPUT_DIR}/bootloader.bin" \
  0x8000 "${OUTPUT_DIR}/partitions.bin" \
  0xe000 "${OUTPUT_DIR}/boot_app0.bin" \
  0x10000 "${OUTPUT_DIR}/firmware.bin" \
  "$SPIFFS_OFFSET" "${OUTPUT_DIR}/spiffs.bin"

cp "${OUTPUT_DIR}/recovery-merged.bin" "${OUTPUT_DIR}/${REV_MERGED_NAME}"

cat > "${OUTPUT_DIR}/manifest.json" <<EOF
{
  "name": "IP Rotator Recovery",
  "version": "${FW_REV}",
  "new_install_prompt_erase": true,
  "builds": [
    {
      "chipFamily": "ESP32",
      "parts": [
        { "path": "bootloader.bin", "offset": 4096 },
        { "path": "partitions.bin", "offset": 32768 },
        { "path": "boot_app0.bin", "offset": 57344 },
        { "path": "firmware.bin", "offset": 65536 },
        { "path": "spiffs.bin", "offset": ${SPIFFS_OFFSET_DEC} }
      ]
    }
  ]
}
EOF

cat > "${OUTPUT_DIR}/index.html" <<EOF
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta http-equiv="Cache-Control" content="no-cache, no-store, must-revalidate">
  <meta http-equiv="Pragma" content="no-cache">
  <meta http-equiv="Expires" content="0">
  <title>IP Rotator firmware</title>
  <script
    type="module"
    src="https://unpkg.com/esp-web-tools@10/dist/web/install-button.js?module"
  ></script>
  <style>
    :root {
      --esp-tools-button-color: #d97706;
      --esp-tools-button-text-color: #111827;
      --esp-tools-button-border-radius: 999px;
    }
    body {
      margin: 0;
      font-family: "Trebuchet MS", "Segoe UI", sans-serif;
      color: #f8fafc;
      background:
        radial-gradient(circle at top left, rgba(245, 158, 11, 0.28), transparent 28rem),
        linear-gradient(135deg, #0f172a 0%, #111827 45%, #1f2937 100%);
      min-height: 100vh;
    }
    main {
      width: min(46rem, calc(100% - 2rem));
      margin: 0 auto;
      padding: 3rem 0 4rem;
    }
    .panel {
      background: rgba(15, 23, 42, 0.76);
      border: 1px solid rgba(251, 191, 36, 0.24);
      border-radius: 1.25rem;
      padding: 1.5rem;
      box-shadow: 0 2rem 4rem rgba(0, 0, 0, 0.26);
      backdrop-filter: blur(12px);
    }
    .paths {
      display: grid;
      gap: 1rem;
      margin-top: 1.5rem;
    }
    .path-card {
      background: rgba(15, 23, 42, 0.55);
      border: 1px solid rgba(148, 163, 184, 0.2);
      border-radius: 1rem;
      padding: 1rem 1.1rem;
    }
    .path-card.ota-card {
      border-color: rgba(245, 158, 11, 0.45);
      box-shadow: inset 0 0 0 1px rgba(245, 158, 11, 0.08);
    }
    .path-card.usb-card {
      border-color: rgba(34, 197, 94, 0.42);
      box-shadow: inset 0 0 0 1px rgba(34, 197, 94, 0.08);
    }
    .path-title {
      display: flex;
      align-items: center;
      gap: 0.7rem;
      flex-wrap: wrap;
      margin-bottom: 0.7rem;
    }
    .path-card h2 {
      margin: 0;
      font-size: 1.2rem;
      color: #fb923c;
    }
    .path-badge {
      display: inline-block;
      width: 0.72rem;
      height: 0.72rem;
      border-radius: 999px;
      background: #f97316;
      box-shadow: 0 0 0 0.22rem rgba(249, 115, 22, 0.18);
      flex: 0 0 auto;
    }
    h1 {
      margin: 0 0 0.75rem;
      font-size: clamp(2rem, 4vw, 3.2rem);
      line-height: 1;
    }
    p, li {
      font-size: 1rem;
      line-height: 1.6;
      color: #dbe4f0;
    }
    code {
      background: rgba(148, 163, 184, 0.18);
      padding: 0.1rem 0.35rem;
      border-radius: 0.35rem;
    }
    .cta {
      margin: 1.5rem 0;
    }
    .note {
      color: #fde68a;
    }
    .downloads {
      margin-top: 1.5rem;
      padding-top: 1rem;
      border-top: 1px solid rgba(251, 191, 36, 0.2);
    }
    .downloads p {
      margin: 0 0 0.65rem;
    }
    .downloads-links {
      display: flex;
      flex-wrap: wrap;
      gap: 0.85rem 1.25rem;
    }
    .muted {
      color: #cbd5e1;
    }
    a {
      color: #fbbf24;
    }
  </style>
</head>
<body>
  <main>
    <div class="panel">
      <p class="note">Two options - update or restore the entire device</p>
      <h1>IP Rotator firmware</h1>
      <p>Firmware version: <code>${FW_REV}</code> for <a href="https://remoteqth.com/w/doku.php?id=simple_rotator_interface_v" target="_blank">Hardware</a></p>
      <div class="paths">
        <section class="path-card ota-card">
          <div class="path-title">
            <span class="path-badge" aria-hidden="true"></span>
            <h2>Update via the EasyOTA web service</h2>
          </div>
          <p class="muted">Download the two <code>.bin</code> files and upload them on the device web update OTA page <code>http://[ip]:82/update</code> in this order: first firmware, then filesystem.</p>
          <div class="downloads">
            <p>Files for EasyOTA update:</p>
            <div class="downloads-links">
              <a href="${REV_FIRMWARE_NAME}">Firmware ${FW_REV}.bin</a>
              <a href="${REV_SPIFFS_NAME}">SPIFFS ${FW_REV}.bin</a>
            </div>
          </div>
        </section>
        <section class="path-card usb-card">
          <div class="path-title">
            <span class="path-badge" aria-hidden="true"></span>
            <h2>Complete recovery via USB</h2>
          </div>
          <p class="muted">Use this path for device recovery. Open this page in Google Chrome or Microsoft Edge over <code>https://</code>, connect the IP Rotator to your computer using USB-C, then start the automated install below.</p>
          <ul class="note">
            <li>Before recovery, back up your configuration from the Setup page <code>http://[ip]:88/setup</code>, Backup and restore part</li>
            <li>After pressing the Connect button, select the <code>CP2104 USB to UART Bridge Controller</code> device</li>
            <li>Then select the <code>Install IP Rotator Recovery</code> option</li>
            <li>After uploading, select the USB device again and the <code>Logs & Console</code> option, where after pressing <code>Reset Device</code> you will see the boot log, including the device's <strong>IP address</strong></li>
          </ul>
          <div class="cta">
            <esp-web-install-button manifest="manifest.json"></esp-web-install-button>
          </div>
          <p class="muted">The installer flashes <code>bootloader</code>, <code>partition table</code>, <code>boot_app0</code>, <code>firmware</code>, and <code>spiffs</code> automatically.</p>
        </div>
      </div>
    </div>
  </main>
</body>
</html>
EOF

cat > "${OUTPUT_DIR}/README.txt" <<EOF
IP Rotator recovery package
==========================

Firmware REV: ${FW_REV}
Flash mode:   ${FLASH_MODE}
Flash freq:   ${FLASH_FREQ}
Flash size:   ${FLASH_SIZE}
SPIFFS off.:  ${SPIFFS_OFFSET}
SPIFFS size:  ${SPIFFS_SIZE_HEX}

Files:
- bootloader.bin
- partitions.bin
- boot_app0.bin
- firmware.bin
- spiffs.bin
- recovery-merged.bin
- ${REV_FIRMWARE_NAME}
- ${REV_SPIFFS_NAME}
- ${REV_MERGED_NAME}
- manifest.json
- index.html

Web installer:
- Host this directory over HTTPS.
- Open index.html in Chrome or Edge.
- ESP Web Tools docs/demo: https://esphome.github.io/esp-web-tools/

Serial/manual flashing:
- Flash recovery-merged.bin at 0x0, or flash the individual images at the offsets from manifest.json.
EOF

echo "Recovery package created in: $OUTPUT_DIR"
echo "Firmware REV: $FW_REV"
echo "SPIFFS offset: $SPIFFS_OFFSET"

IP Rotator recovery package
==========================

Firmware REV: 20260430
Flash mode:   dio
Flash freq:   40m
Flash size:   4MB
SPIFFS off.:  0x290000
SPIFFS size:  0x160000

Files:
- bootloader.bin
- partitions.bin
- boot_app0.bin
- firmware.bin
- spiffs.bin
- recovery-merged.bin
- ip-rotator-20260430-esp32-poe-firmware.bin
- ip-rotator-20260430-esp32-poe-spiffs.bin
- ip-rotator-20260430-esp32-poe-recovery-merged.bin
- manifest.json
- index.html

Web installer:
- Host this directory over HTTPS.
- Open index.html in Chrome or Edge.
- ESP Web Tools docs/demo: https://esphome.github.io/esp-web-tools/

Serial/manual flashing:
- Flash recovery-merged.bin at 0x0, or flash the individual images at the offsets from manifest.json.

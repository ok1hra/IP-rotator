# IP-rotator firmware
- Manual available on the [Wiki page](https://remoteqth.com/w/doku.php?id=simple_rotator_interface_v) | [copy on GitHub](Assembly-manual.md)
- MQTT based IP control firmware, for 3D printed rotator
- Web preview
- Main repository [Parameterizable 3D print Antenna rotator in OpenScad](https://github.com/ok1hra/Parameterizable-3D-print-Antenna-rotator-in-OpenScad)

<img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/ajax-server.png" height="200"><img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/ip-rotator.jpg" height="200">

<img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/control-zoom0.png" height="400"><img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/control-zoom1.png" height="400"><img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/control-zoom2.png" height="400">

Keyboard shortcuts on the main control page:

| Key | Action |
| --- | --- |
| `+` / `-` | Change map zoom |
| `p` | Decrease SPEED |
| `P` | Increase SPEED |
| `G` | Toggle Grayline |
| `S` | Toggle State borders |
| `D` | Toggle DXCC prefixes |
| `T` | Focus or unfocus the **TO DXCC** input |

<img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/setup.png" height="600"><img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/calibrate.png" height="600">

<img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/elevation.png" height="400">

## OTA artifacts

The project now uses SPIFFS for the web UI assets in [`data/`](/home/dan/inst/IP-rotator/data), so OTA/web updates may require two separate images:

- `firmware.bin` for the sketch
- `spiffs.bin` for the filesystem

### Build `firmware.bin`

In Arduino IDE 1.8.19 use:

- `Sketch > Export compiled Binary`

For this project select:

- Board: `OLIMEX ESP32-PoE`
- Partition Scheme: `Default`

The sketch includes a local [`partitions.csv`](/home/dan/inst/IP-rotator/partitions.csv), so Arduino will prefer that partition table during build/upload. This keeps USB upload, `ESP32 Sketch Data Upload`, and `tools/build_spiffs_image.sh` aligned.

### Build `spiffs.bin`

Use the helper script from the repo root:

```bash
tools/build_spiffs_image.sh
```

Default output:

- `build/spiffs.bin`

The script reads the SPIFFS size and offset from the project's local [`partitions.csv`](/home/dan/inst/IP-rotator/partitions.csv) and packs the current `data/` directory with `mkspiffs`.
It also writes `/fs_build.txt` into the image so the firmware can report whether the uploaded SPIFFS build matches the running firmware revision.

### Upload options

- USB/serial: use Arduino IDE upload for firmware and `Tools > ESP32 Sketch Data Upload` for SPIFFS
- Web OTA: upload `firmware.bin` as firmware image and `spiffs.bin` as filesystem image

### Notes

- If you change only files in `data/`, you only need to rebuild and upload `spiffs.bin`
- If you change only `IP-rotator.ino`, you only need a new `firmware.bin`
- If the board uses a different partition scheme than `Default`, pass a different CSV to the script with `--partitions PATH`

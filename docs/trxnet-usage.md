# TrxNet — Configuration and Usage

**Applies to:** IP-rotator firmware rev 20260522+

TrxNet integrates the rotator into a **broker-free, P2P UDP network** shared with
other ham radio devices (IC-705 Interface, k3ng CW keyer). Devices discover each
other automatically via UDP broadcast — no router configuration, no broker required.
Runs alongside MQTT independently.

---

## How it works

```
IP-rotator (ESP32-PoE, Ethernet)
    │  publishes /azimuth, /elevation
    │  subscribes /s-azimuth, /s-elevation   ← rotation commands from peers
    │
    └── LAN (UDP broadcast port 5683)
            │
            ├── ROT.02    (second IP-rotator)
            ├── 705.01    (IC-705 Interface)
            └── OI3.ff    (k3ng CW keyer)
```

---

## Published topics (IP-rotator → network)

| Topic | Type | Trigger | Description |
|-------|------|---------|-------------|
| `/azimuth` | `uint16_t` LE | Moving: change >= 1°. Stopped: change >= 2° or every 60 s | Current compass azimuth in degrees (0-359) |
| `/elevation` | `uint16_t` LE | Same as above | Current elevation 0–90°. Sent only when elevation mode is enabled. |

## Subscribed topics (network → IP-rotator)

| Topic | Type | Action | Condition |
|-------|------|--------|-----------|
| `/s-azimuth` | `uint16_t` LE | Rotate to this compass azimuth (0-359) | Subscribe must be enabled in Setup |
| `/s-elevation` | `uint16_t` LE | Set elevation target | Subscribe enabled + elevation mode on |

---

## Payload format

All values are **raw `uint16_t`, 2 bytes, little-endian** (native ESP32 byte order).
Serialise and deserialise with `memcpy` — never with a direct pointer cast.

```
/azimuth 180°  →  bytes: B4 00
                         ── ──
                         LSB  MSB    (0x00B4 = 180)
```

### Sending a rotation command from another TrxNet device

```cpp
// Rotate IP-rotator to 270°
uint16_t az = 270;
net.publish("/s-azimuth", (const uint8_t*)&az, sizeof(az), TRX_NON);
```

### Receiving azimuth in another device

```cpp
void onAzimuth(const char* from, const uint8_t* data, size_t len) {
    if (len < sizeof(uint16_t)) return;
    uint16_t az;
    memcpy(&az, data, sizeof(az));
    // az = azimuth in degrees
}

net.subscribe("/azimuth", onAzimuth);
```

---

## Web configuration

Open **`http://<ip>/setup`** → **TrxNet** section.

| Field | Format | Default | Description |
|-------|--------|---------|-------------|
| **NET_ID** | 1–2 chars, e.g. `01`, `A` | *(empty)* | Own device identifier. Forms the device name **`ROT.01`**. Empty = TrxNet disabled. Set in the **Station and rotor** section. |
| **UDP port** | 1024–65535 | `5683` | Must match all TrxNet devices on the network. Change takes effect after restart. |
| **Subscribe** | checkbox | off | Accept `/s-azimuth` (and `/s-elevation`) commands from peers. Change takes effect after restart. |
| **Active peers** | live list | — | Devices currently visible on the network. Refreshed every second. |

### Choosing NET_ID

- Format: 1–2 printable characters (`01`, `02`, `A`, `ff`).
- Empty NET_ID = TrxNet **disabled** — no UDP traffic is generated.
- ID only needs to be unique within the `ROT` device type. `ROT.01` and `705.01` can coexist.
- The same NET_ID is also used as the second part of the MQTT topic.

---

## Device name format

```
ROT.{NET_ID}
```

| NET_ID | Device name on network |
|--------|----------------------|
| `01` | `ROT.01` |
| `A` | `ROT.A` |
| `ff` | `ROT.ff` |
| *(empty)* | TrxNet disabled |

---

## Connecting with IC-705 Interface or k3ng keyer

IP-rotator **does not exchange frequency or mode** with other devices. The only
cross-device use case is rotating the antenna to a bearing commanded by another device.

**Example — DX station auto-rotate from a custom ESP32 client:**

```cpp
// On any TrxNet device — command all IP-rotators with Subscribe enabled
uint16_t bearing = 312;
net.publish("/s-azimuth", (const uint8_t*)&bearing, sizeof(bearing), TRX_NON);
```

**Example — read azimuth on IC-705 Interface for logging:**

```cpp
// On IC-705 Interface firmware
net.subscribe("/azimuth", onAzimuth);

void onAzimuth(const char* from, const uint8_t* data, size_t len) {
    if (len < sizeof(uint16_t)) return;
    uint16_t az;
    memcpy(&az, data, sizeof(az));
    // display or log az
}
```

---

## Azimuth coordinate system

IP-rotator publishes the **real compass azimuth** (0-359), referenced to North.
Internally, the firmware converts between this value and the rotator position relative
to the CCW end-stop by applying the `StartAzimuth` calibration offset.

---

## Peer discovery timing

| Parameter | Value |
|-----------|-------|
| Discovery probe on boot | Immediate (after Ethernet link up) |
| Keepalive broadcast interval | 30 s |
| Peer timeout (no keepalive) | ~95 s (~3 missed keepalives) |
| Time to first peer visible | < 100 ms under normal conditions |

After an Ethernet reconnect, `trxNetBegin()` is not called again (guard flag prevents
double-init). If TrxNet stops responding after a link flap, restart the device.

---

## Network requirements

- All devices must be on the **same Layer-2 broadcast domain** (same switch/VLAN).
- UDP broadcast `255.255.255.255` must reach all devices.
- All devices must use the **same UDP port** (default 5683).
- IP-rotator uses **wired Ethernet** (Olimex ESP32-PoE) — not affected by WiFi AP
  client isolation. WiFi-only peers on networks with AP isolation will not be discovered.

---

## Disabling TrxNet

Clear the **Rotator ID** field in the **Station and rotor** section of Setup and save.
The firmware will not call `net.begin()` and no UDP traffic is generated. MQTT and all
other features remain fully functional.

---

## Troubleshooting

| Symptom | Likely cause | Fix |
|---------|-------------|-----|
| No peers appear | AP isolation on WiFi peer | Use wired Ethernet on the peer, or check router settings |
| Peer appears then disappears after ~95 s | Network segment separation | Check VLAN or switch config between devices |
| `/s-azimuth` received but rotator does not move | Subscribe not enabled | Enable Subscribe checkbox in TrxNet section and restart |
| `/s-azimuth` value ignored | Value exceeds MaxRotateDegree | Check the rotation range configured for this rotator |
| TrxNet inactive after restart | NET_ID empty | Set NET_ID in Station and rotor section |
| Port mismatch — peers not visible | Devices on different ports | Verify all devices use port 5683 (or the same custom port) |

---

*Library source and documentation: [https://github.com/ok1hra/TrxNet](https://github.com/ok1hra/TrxNet)*  
*Full technical reference: [trxnet.md](trxnet.md)*

# TrxNet — Integration Manual for IP-rotator

**Library version:** 0.1.0  
**Firmware:** IP-rotator (ESP32-PoE / Olimex)  
**Protocol:** P2P telemetry over local network (CoAP/UDP, no broker)

---

## Table of contents

1. [What is TrxNet](#1-what-is-trxnet)
2. [How it works](#2-how-it-works)
3. [Library API](#3-library-api)
4. [IP-rotator integration](#4-ip-rotator-integration)
5. [Topics](#5-topics)
6. [Configuration — Setup page](#6-configuration--setup-page)
7. [EEPROM map](#7-eeprom-map)
8. [Backup and restore](#8-backup-and-restore)
9. [Device ecosystem](#9-device-ecosystem)
10. [Network requirements](#10-network-requirements)
11. [Tunable limits](#11-tunable-limits)
12. [Common pitfalls](#12-common-pitfalls)

---

## 1. What is TrxNet

TrxNet is a lightweight P2P telemetry protocol for ham radio devices on a local network.  
It requires **no broker, no server, no mDNS, and no router**. Devices discover each other
via UDP broadcast and exchange data using a minimal CoAP framing.

In IP-rotator, TrxNet runs **alongside MQTT** — both can be active at the same time.

| Feature | MQTT | TrxNet |
|---------|------|--------|
| Broker required | Yes | No |
| Internet access needed | Yes (for remote broker) | No |
| Latency | Depends on broker | LAN-local (~1 ms) |
| Discovery | Manual (fixed topic) | Automatic (broadcast) |
| Use case | Remote monitoring / control | Local device-to-device |

---

## 2. How it works

### Discovery

On startup, the device broadcasts a **PROBE** packet to `255.255.255.255` on the configured
UDP port (default 5683). Every TrxNet device on the same subnet that receives the probe
replies with a unicast **ANNOUNCE**. From this point both devices know each other.

Every 30 seconds, each device re-broadcasts an **ANNOUNCE** as a keepalive. A peer is
removed from the internal table after 95 seconds of silence (~3 missed keepalives).

### Messaging

Once peers are known, the device sends CoAP-framed UDP unicast messages to each peer
individually. Two delivery modes are available:

| Mode | Constant | Behaviour |
|------|----------|-----------|
| Non-confirmable | `TRX_NON` | Fire-and-forget. No ACK. |
| Confirmable | `TRX_CON` | Retransmits every 2 s, up to 3 times, until ACKed. |

IP-rotator uses `TRX_NON` for all publish topics (azimuth telemetry — a lost packet
is superseded by the next one) and `TRX_CON` is available for command topics that
must not be lost.

---

## 3. Library API

### Installation

The library is located at `~/Arduino/libraries/TrxNet/`. Arduino IDE finds it
automatically — no additional steps needed for IP-rotator.

### Constructor

```cpp
WiFiUDP udp;
TrxNet  net(udp);          // port defaults to 5683
TrxNet  net(udp, 5683);    // explicit port
```

The device name is **not** passed at construction time — it is loaded from EEPROM
and not yet known when global objects are initialised.

### `setPort(uint16_t port)`

Sets the UDP port before calling `begin()`. Call this when the port is loaded from
EEPROM at runtime rather than fixed at compile time.

```cpp
net.setPort(TrxNetPort);   // TrxNetPort loaded from EEPROM
net.begin("ROT.01");
```

### `begin(const char* name)`

Starts the UDP socket and broadcasts the initial discovery probe.  
**Call after the network is up** (Ethernet link obtained, IP assigned).

```cpp
net.begin("ROT.01");
```

The `name` string is the device identity visible to all peers. Maximum 31 characters.
Format used in IP-rotator: `ROT.{NET_ID}` — e.g. `ROT.01`, `ROT.A`.

If `NET_ID` is empty, `begin()` must not be called (TrxNet disabled).

### `loop()`

Processes incoming packets, sends keepalive broadcasts, and retransmits unACKed CON
messages. **Must be called every iteration of `loop()` without blocking delays.**

```cpp
void loop() {
    net.loop();
    // ...
}
```

### `subscribe(const char* path, TrxNetCallback cb)`

Registers a callback invoked when a message arrives on `path`.

```cpp
typedef void (*TrxNetCallback)(const char* from, const uint8_t* data, size_t len);
```

| Parameter | Description |
|-----------|-------------|
| `from` | Sender's device name string, e.g. `"705.01"` |
| `data` | Raw payload bytes |
| `len` | Payload length |

Keep callbacks short — they are called synchronously inside `loop()`. No `delay()`,
no blocking I/O. Use a volatile flag or queue to hand off work to `loop()`.

### `unsubscribe(const char* path)`

Removes the callback for `path`. Safe to call even if the path was never subscribed.

### `publish(const char* path, const uint8_t* data, size_t len, TrxMsgType type)`

Sends `data` to all currently known peers on `path`.

```cpp
uint16_t az = 180;
net.publish("/azimuth", (const uint8_t*)&az, sizeof(az), TRX_NON);
```

Does nothing if no peers are known. Oversized payloads (> `TRXNET_MAX_PAYLOAD` = 64 bytes)
are silently truncated.

### `peerCount()`

Returns the number of active peers (discovered and not timed out).

### `peer(int index)`

Returns a read-only pointer to a `TrxPeer` struct, or `NULL` if index is out of range.

```cpp
struct TrxPeer {
    char      name[32];    // device name, e.g. "705.01"
    IPAddress ip;
    uint16_t  port;
    uint32_t  lastSeen;    // millis() timestamp of last discovery packet
    bool      active;
};
```

Usage example (iterate all peers):

```cpp
for (int i = 0; i < net.peerCount(); i++) {
    const TrxPeer* p = net.peer(i);
    uint32_t ageSec = (millis() - p->lastSeen) / 1000;
    Serial.printf("%s  active=%d  last=%lus ago\n", p->name, p->active, ageSec);
}
```

---

## 4. IP-rotator integration

### Activation

TrxNet is disabled by default. It activates when `NET_ID` is set to a non-empty value
in the **Station and rotor** section of the setup page.

- `NET_ID` empty → TrxNet inactive (no `begin()` called, no UDP traffic)
- `NET_ID` = `"01"` → device name `ROT.01`, TrxNet active after next restart

### Lifecycle

```
Boot → EEPROM read (NET_ID, TrxNetPort, TrxNetSubEnabled)
     → Ethernet link up (EthEvent ARDUINO_EVENT_ETH_GOT_IP)
     → trxNetBegin()   ← net.setPort() + net.begin() + optional subscribe()
     → loop():  net.loop() + trxNetPublish() + trxNetProcessPending()
```

`trxNetBegin()` includes a guard flag (`trxNetStarted`) so it is safe even if
`EthEvent` fires multiple times (e.g. after a reconnect).

### Publish — throttle logic

`trxNetPublish()` runs every loop iteration but sends a packet only when:

- The azimuth changed by ≥ 1° since the last publish, **or**
- At least 10 seconds have passed since the last publish (keepalive).

This prevents flooding the network during rapid rotation while ensuring peers receive
a heartbeat even when the antenna is stationary.

### Subscribe — command handling

Incoming commands from peers are processed in two steps to keep callbacks short:

1. **Callback** (`onSetAzimuth`) — validates the value and sets a volatile flag.
2. **`trxNetProcessPending()`** — called from `loop()`, reads the flag and executes
   the same code path as the MQTT `/ROT/Target` handler:
   sets `AzimuthTarget`, updates `UiTargetAzimuth`, calls `RotCalculate()`.

Subscribe is disabled by default. Enable it with the **Subscribe** checkbox in the
TrxNet section of the setup page.

### Coexistence with MQTT

Both systems set `AzimuthTarget` and call `RotCalculate()` using identical code paths.
Whichever command arrives last wins. No priority arbitration is implemented — last
writer takes effect.

---

## 5. Topics

All payload values use the platform's native byte order (little-endian on ESP32).
Use `memcpy` to deserialise — do not cast pointers directly.

### Published by IP-rotator

| Topic | Type | Value | Trigger |
|-------|------|-------|---------|
| `/azimuth` | `uint16_t` | Current azimuth in degrees (0 – MaxRotateDegree) | Change ≥ 1° or 10 s keepalive |
| `/elevation` | `uint16_t` | Current elevation in degrees (0 – 90) | Same as above — only if elevation mode is enabled |

### Subscribed by IP-rotator (when Subscribe is enabled)

| Topic | Type | Value | Action |
|-------|------|-------|--------|
| `/s-azimuth` | `uint16_t` | Target azimuth (0 – MaxRotateDegree) | Rotate to this azimuth |
| `/s-elevation` | `uint16_t` | Target elevation (0 – 90) | Set elevation target — only if elevation mode is enabled |

The `/s-` prefix means "set / command" — same convention used in OI3 keyer and
IC-705 Interface.

### Sending a command from another device

```cpp
// Rotate IP-rotator to 270°
uint16_t az = 270;
net.publish("/s-azimuth", (const uint8_t*)&az, sizeof(az), TRX_NON);
```

`TRX_CON` can be used for `/s-azimuth` if confirmed delivery is required:

```cpp
net.publish("/s-azimuth", (const uint8_t*)&az, sizeof(az), TRX_CON);
```

### Receiving azimuth from another IP-rotator

```cpp
void onAzimuth(const char* from, const uint8_t* data, size_t len) {
    if (len < sizeof(uint16_t)) return;
    uint16_t az;
    memcpy(&az, data, sizeof(az));
    Serial.printf("[%s] azimuth = %u°\n", from, az);
}

net.subscribe("/azimuth", onAzimuth);
```

---

## 6. Configuration — Setup page

Open the **TrxNet** section (collapsible tab) at the bottom of the setup page (`/setup`).

### Rotator ID (NET_ID)

Displayed read-only. Set in the **Station and rotor** section.

- Format: 1–2 characters, e.g. `01`, `A`, `ff`
- Device name on the network: `ROT.{NET_ID}`
- **Empty = TrxNet disabled.** No UDP traffic is generated.
- Changes take effect after save + restart.

### UDP port

Default: **5683**.  
All TrxNet devices on the same network must use the same port.  
Change takes effect after restart.

### Subscribe

Checkbox, default **unchecked** (disabled).  
When checked, the rotator accepts `/s-azimuth` commands from peers.  
Change takes effect after restart.

### Active peers

Live list, refreshed every second via `/trxnet/peers`.  
Shows device name, status indicator, and time since last contact.

| Indicator | Meaning |
|-----------|---------|
| ● green | Peer active (last seen < 95 s ago) |
| ○ grey | Peer timed out (> 95 s, will be removed after the next keepalive cycle) |

---

## 7. EEPROM map

| Address | Size | Variable | Type | Default | Description |
|---------|------|----------|------|---------|-------------|
| 0–1 | 2 B | `NET_ID` | string | `""` | Rotator ID; empty = TrxNet disabled |
| 615–616 | 2 B | `TrxNetPort` | `uint16_t` | 5683 | UDP port |
| 617 | 1 B | `TrxNetSubEnabled` | `bool` | `false` | Subscribe enabled |

`EEPROM_SIZE` = 618.

---

## 8. Backup and restore

`TrxNetPort` and `TrxNetSubEnabled` are included in the JSON config backup
(`/backup/config`):

```json
{
  "format": "ip-rotator-config",
  "version": 1,
  "config": {
    ...
    "trxnet_port": 5683,
    "trxnet_sub": false
  }
}
```

Both fields are optional in the restore payload — if absent, values remain at their
defaults (5683 / false). All other backup fields are still validated normally.

---

## 9. Device ecosystem

TrxNet is used across multiple ham radio devices. All devices on the same subnet
discover each other automatically.

| Device | Device name | Publishes | Subscribes |
|--------|-------------|-----------|------------|
| **IP-rotator** | `ROT.{NET_ID}` | `/azimuth`, `/elevation` | `/s-azimuth`, `/s-elevation` |
| IC-705 Interface | `705.{NET_ID}` | `/hz`, `/mode` | `/s-hz` |
| k3ng CW keyer (OI3) | `OI3.{NET_ID}` | `/hz`, `/mode` | `/s-hz`, `/s-mode`, `/s-cw` |

A typical shack scenario: the IC-705 Interface publishes the current operating frequency
(`/hz`) and mode (`/mode`). The OI3 keyer subscribes to both. IP-rotator publishes
the current antenna bearing — any other device on the network can subscribe to
`/azimuth` to display or log it.

### Cross-device example — rotate to DX station bearing

A custom device (e.g. a DX cluster client on ESP32) subscribes to DX spots,
looks up the bearing, and sends a rotation command:

```cpp
// DX cluster client — send rotation command to all IP-rotators on LAN
uint16_t bearing = 312;   // great-circle bearing to JA
net.publish("/s-azimuth", (const uint8_t*)&bearing, sizeof(bearing), TRX_NON);
```

Every IP-rotator on the network with Subscribe enabled will receive the command.
Use different `NET_ID` values if you have multiple rotators and want to target
a specific one — there is currently no peer-addressed publish (all sends go to
all known peers).

---

## 10. Network requirements

### Mandatory

- All TrxNet devices must be on the **same Layer-2 broadcast domain** (same subnet).
- UDP broadcast (`255.255.255.255`) must reach all devices.
- All devices must use the **same UDP port** (default 5683).

### Common failure modes

| Symptom | Likely cause |
|---------|-------------|
| Peers never appear | AP client isolation enabled on WiFi router |
| Peers appear on bench but not on site | Guest network or VLAN separating devices |
| Peers appear but commands not received | Subscribe checkbox not enabled |
| Peers disappear after ~95 s | `loop()` blocked; keepalive not sent in time |
| Commands received but rotator does not move | Subscribe disabled, or `MaxRotateDegree` exceeded |

### Ethernet vs WiFi

IP-rotator runs on **Olimex ESP32-PoE** with wired Ethernet. Wired Ethernet is not
affected by AP client isolation. If mixing wired and WiFi TrxNet devices, ensure
the switch/AP does not filter broadcasts between the wired and wireless segments.

---

## 11. Tunable limits

Override these `#define` values **before** `#include <TrxNet.h>` to change
static buffer sizes:

| Define | Default | Description |
|--------|---------|-------------|
| `TRXNET_MAX_PEERS` | 6 | Maximum simultaneous peers |
| `TRXNET_MAX_SUBS` | 8 | Maximum subscriptions |
| `TRXNET_MAX_DEVICE_NAME` | 32 | Device name buffer size (incl. null) |
| `TRXNET_MAX_TOPIC_LEN` | 32 | Topic path buffer size (incl. null) |
| `TRXNET_MAX_PAYLOAD` | 64 | Maximum payload bytes per message |
| `TRXNET_MAX_PENDING` | 2 | Shared CON retransmit queue slots |
| `TRXNET_MAX_SEEN` | 16 | Incoming CON dedup ring buffer |
| `TRXNET_ANNOUNCE_MS` | 30000 | Keepalive broadcast interval (ms) |
| `TRXNET_PEER_TIMEOUT_MS` | 95000 | Peer removal timeout (ms) |
| `TRXNET_CON_TIMEOUT_MS` | 2000 | CON retransmit interval (ms) |
| `TRXNET_CON_MAX_RETRIES` | 3 | CON retransmit attempts |

IP-rotator uses default values. All buffers are statically allocated — no heap
fragmentation risk on long-running devices.

---

## 12. Common pitfalls

### Blocking in `loop()` breaks CON retransmit

```cpp
void loop() {
    net.loop();
    delay(500);   // BAD — CON retransmit fires every 2 s; this breaks timing
}
```

IP-rotator uses non-blocking timers throughout `loop()`.

### Blocking in a callback stalls all packet processing

```cpp
void onSetAzimuth(const char* from, const uint8_t* data, size_t len) {
    rotateToAngle(az);   // BAD if this takes >1 ms
}
```

IP-rotator uses the volatile flag pattern:

```cpp
void onSetAzimuth(const char* from, const uint8_t* data, size_t len) {
    // just store the value and set a flag
    trxPendingAz = az;
    trxAzPending = true;
}

void trxNetProcessPending() {    // called from loop()
    if (trxAzPending) {
        trxAzPending = false;
        AzimuthTarget = (int)trxPendingAz;
        UiTargetAzimuth = AzimuthTarget + StartAzimuth;
        if (UiTargetAzimuth > 359) UiTargetAzimuth -= 360;
        RotCalculate();
    }
}
```

### `begin()` called before network is up

`EthEvent` with `ARDUINO_EVENT_ETH_GOT_IP` is the correct place to call
`trxNetBegin()`. Do not call it in `setup()` before the Ethernet link is confirmed.

### Port mismatch between devices

All TrxNet devices must use the same UDP port. If the port is changed via the
setup page, all other devices must be updated to the same port and restarted.

### Azimuth coordinate system

IP-rotator publishes `Azimuth` — the internal rotator value relative to `StartAzimuth`
(the CCW end-stop calibration offset), ranging from 0 to `MaxRotateDegree`.  
This is **not** necessarily a compass bearing. Two rotators with different
`StartAzimuth` settings will interpret the same published value differently.
If real-world bearing coordination between rotators is needed, ensure all rotators
share the same `StartAzimuth` calibration.

### `setPort()` must be called before `begin()`

```cpp
// Correct order:
net.setPort(TrxNetPort);   // load from EEPROM first
net.begin("ROT.01");

// Wrong:
net.begin("ROT.01");
net.setPort(5700);          // has no effect — socket already open
```

---

*See also: [20260519-trxnet-implementation.md](20260519-trxnet-implementation.md) — original design spec with full EEPROM and code fragments.*

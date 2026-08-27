# ChirpStack — a brief

What it is, what it would take to point this board at it, and where a
single-channel gateway stops fitting the model.

## What it is

An open-source **LoRaWAN Network Server**. It sits between gateways and
applications and does the part of LoRaWAN that is not radio: it authenticates
devices, decrypts and deduplicates uplinks, schedules downlinks, manages
sessions and keys, and hands you the payload over MQTT, HTTP or gRPC.

Deduplication is the piece people underestimate. In LoRaWAN a device does not
associate with a gateway — it broadcasts, and *every* gateway in earshot
forwards the same packet. The network server recognises the duplicates, keeps
the best copy, and decides which gateway answers. That is why "which gateway is
my device on" is not a question LoRaWAN answers.

It is the usual self-hosted answer to The Things Network. Relevant here because
**TTN v3 refuses single-channel gateways** — SparkFun's own listing says so —
so a private server is the only route for this board.

## Shape of a deployment (v4)

v4 merged what v3 split into network-server and application-server. You run:

| Component | Role |
|---|---|
| `chirpstack` | the server itself |
| `chirpstack-gateway-bridge` | Semtech UDP ⇄ MQTT translation |
| Mosquitto (or any MQTT broker) | transport between the two |
| PostgreSQL | devices, keys, sessions |
| Redis | device state, deduplication windows |

The upstream `chirpstack-docker` repository is a compose file wiring all five
together, which is the least effort way to see it working. Anything permanent
wants real Postgres backups — losing that database loses every device session.

## Connecting this board is a two-line change

The stock firmware already speaks the right protocol. Verified in the source:
`loraModem.h:500-505` defines `PROTOCOL_VERSION 0x01`, `PKT_PUSH_DATA`,
`PKT_PULL_DATA`, `PKT_PULL_RESP`, `PKT_PULL_ACK` — that is the **Semtech UDP
packet forwarder** protocol, exactly what the gateway bridge listens for on UDP
1700.

So in `ESP-sc-gway.h`:

```c
#define _TTNSERVER "10.0.0.x"     // your ChirpStack host
#define _TTNPORT   1700           // already correct
```

Rebuild and upload — see `docs/programming.md`. No protocol work, no
translation layer. The gateway's EUI is derived from the MAC, so register it in
ChirpStack as `840d8e***0c5a80`-ish (read the exact value from the boot banner).

## Where single-channel stops fitting

This is the part worth understanding before investing a weekend. LoRaWAN
assumes a gateway that hears **8 channels across several spreading factors at
once**. This board hears one channel at one spreading factor. Uplinks work; the
rest degrades.

**Joins (OTAA) need a downlink.** The device transmits a join-request and
expects a join-accept in a receive window. Sending that reliably from a
single-channel gateway is awkward, and a device that cannot join never sends
anything. **Use ABP instead** — session keys provisioned by hand, no join
exchange. That is exactly what SparkFun's own device sketch does, and why.

**The device must transmit on our one channel.** A normal LoRaWAN device hops
across all 8 channels of its sub-band, so roughly 7 of 8 uplinks land on a
frequency we are not listening to. The device has to be pinned — the same
`LMIC_disableSubBand` / `LMIC_enableChannel` trick in `etc/reference/`. This is
not something you can fix at the server.

**Turn ADR off.** Adaptive data rate lets the server move a device to a
different spreading factor. We only hear one, so the first adjustment makes the
device disappear.

**RX2 is on a different frequency.** The downlink window RX2 is 923.3 MHz at
SF12, and the downlink band as a whole is 923.3–927.5 MHz — nowhere near the
903.9 MHz uplink channel. RX1 (same frequency and SF as the uplink) is the only
window a parked single-channel gateway serves naturally.

Net: **uplink-only ABP with ADR off and a pinned channel works.** Bidirectional
LoRaWAN with OTAA does not, without more work than the board is worth.

## Is it the right tool?

Depends on which problem you actually have.

**ChirpStack earns its keep when** you want many devices from different people,
proper key management, payload decoders, and integrations that already exist
(MQTT, HTTP, InfluxDB, Postgres). It is a real product and it does that well.

**It is heavy when** the goal is "a few sensors reporting to something that
records them". LoRaWAN brings join procedures, session keys, MAC commands,
frame counters and ADR — and a single-channel gateway is at odds with half of
it. Five containers and a database is a lot of machinery to carry a handful of
readings.

The alternative, for this board as it stands: raw LoRa with our own framing —
which `src/apps/link` and `src/apps/gateway` already do — and have the gateway
POST to whatever collects the data. `/status.json` is already the right shape.
No join, no keys to manage, no channel plan to satisfy, and nothing that breaks
because the gateway hears one channel.

**A reasonable split:** raw LoRa for your own nodes where you control both ends,
and ChirpStack only if you need to interoperate with LoRaWAN devices you did not
write. Buying a real 8-channel gateway is also worth pricing against the effort
of making a single-channel one behave.

## If you try it

1. `chirpstack-docker` on the LAN, compose up, reach the web UI.
2. Register the gateway by EUI; confirm it appears and its "last seen" updates —
   the gateway sends periodic status even with no traffic, so this proves the
   UDP path before any device exists.
3. Create an application, add a device with **ABP**, ADR **off**.
4. Pin the device to the gateway's channel and spreading factor.
5. Watch the frames arrive in the UI, then wire an integration.

Step 2 is the one that tells you whether the network path works, and it needs no
radio traffic at all. Do not skip to step 5 and debug two things at once.

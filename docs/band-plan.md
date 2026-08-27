# The 902–928 MHz band, and why we sit on 915.0

Computed from the LoRaWAN Regional Parameters for US902-928 and checked in code
— `lib/loramath/loramath.h` has the channel arithmetic and `test/` asserts it.

## "915 MHz" is a band, not a frequency

The single most common confusion here. The US ISM band runs **902–928 MHz**,
and everyone calls it "the 915 band" after roughly its middle. Saying a radio is
"on 915" says almost nothing — it names a 26 MHz span.

## The LoRaWAN US915 plan

| | |
|---|---|
| 125 kHz uplinks | channels 0–63, `902.3 + 0.2 × n` MHz → **902.3 … 914.9** |
| 500 kHz uplinks | channels 64–71, `903.0 + 1.6 × k` MHz → 903.0 … 914.2 |
| 500 kHz downlinks | 8 channels, `923.3 + 0.6 × k` MHz → **923.3 … 927.5** |

Note the downlinks are in a different part of the band entirely. That is why a
single-channel gateway parked on an uplink channel cannot serve the RX2 window —
see `docs/chirpstack.md`.

**Sub-band 2 is what US networks use**: 125 kHz channels 8–15 (903.9, 904.1,
904.3, 904.5, 904.7, 904.9, 905.1, 905.3) plus 500 kHz channel 65 (904.6).

## Where the interesting frequencies land

```
903.900 MHz -> 125 kHz uplink channel 8      the stock 2019 gateway config
915.000 MHz -> NOT a LoRaWAN channel         our raw-LoRa default
906.875 MHz -> NOT a LoRaWAN channel         Meshtastic US LongFast
```

915.0 is not in the plan: `(915.0 − 902.3) / 0.2 = 63.5`, so it falls between
channels 63 and 64. It is a **raw-LoRa convention** — mid-band, memorable, and
the default in many point-to-point libraries and examples.

**A correction worth keeping.** This project's notes described 903.9 MHz as
"US915 uplink channel 0" in three places. It is **channel 8**. Channel 0 is
902.3 MHz. What makes 903.9 significant is that it is the first channel of
sub-band 2, which is why single-channel gateways conventionally sit there.

## Why our default is 915.0

`lib/board/board.h` sets `kDefaultFrequencyHz = 915000000UL`.

We are doing raw point-to-point LoRa, not LoRaWAN. There is no plan to satisfy —
the frequency only has to **match the other radio**, and the other radios here
are already on 915.0. Changing one constant is cheaper than reconfiguring
several devices.

If this board ever speaks LoRaWAN again, change it back to `903900000`. That is
channel 8, and it is the right single-channel choice for a US network.

## Four things must match, not one

A LoRa receiver demodulates a signal only when **all** of these agree. Miss one
and you get silence, with no error at either end — which is this project's
recurring failure:

| | ours |
|---|---|
| Frequency | 915.0 MHz |
| Spreading factor | SF7 |
| Bandwidth | 125 kHz |
| Sync word | `0x12` (private; `0x34` is LoRaWAN, `0x2b` Meshtastic) |

Coding rate (ours 4/5) is carried in the explicit header, so a receiver adapts
to it — it is the one parameter that does not have to be agreed in advance.

`docs/radio-settings-request.md` is the version to send to whoever owns the
other radio.

## A compliance note for anything permanent

Under FCC 15.247, the higher power allowances in 902–928 MHz assume either
frequency hopping or a signal at least 500 kHz wide. LoRa at 125 kHz sitting on
one fixed channel is neither — real LoRaWAN complies by hopping across its
channel plan.

This does not matter for bench testing at low power. It is worth confirming
before a permanent fixed-frequency link goes up at 17 dBm. Widening to 500 kHz
bandwidth or reducing transmit power are both single-constant changes in
`lib/board/board.h`.

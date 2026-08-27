# Measuring SNR, and why it stops working on a bench

Written to be forwarded to whoever runs the transmitting side.

## SNR is measured at the receiver. The transmitter cannot measure it.

There is no round trip in raw LoRa — the transmitter sends and learns nothing.
Every signal-quality number comes from the receiving radio, per packet:

| Register | Meaning |
|---|---|
| `RegPktSnrValue` (0x19) | SNR of the last packet, two's complement, **quarter-dB steps** |
| `RegPktRssiValue` (0x1A) | RSSI of the last packet |
| `RegRssiValue` (0x1B) | current wideband RSSI — the noise floor when idle |

Datasheet §5.5.5, and note the correction that applies below the noise floor:

```
SNR <  0 : Packet Strength(dBm) = -157 + PacketRssi + PacketSnr * 0.25
SNR >= 0 : RSSI(dBm)            = -157 + PacketRssi
```

`src/apps/listen` prints all of this already. **So the transmitting side does
not need to do anything to "measure SNR" — it needs to tell us its transmit
power, and preferably turn it down.**

## The bench problem, in numbers

Free-space path loss at 915 MHz is `20log₁₀(d) + 20log₁₀(f) − 147.55`:

| Distance | Loss |
|---|---|
| 1 m | 31.7 dB |
| 3 m | 41.2 dB |
| 10 m | 51.7 dB |
| 30 m | 61.2 dB |

At 17 dBm with 2 dBi antennas at each end:

```
1 m   ->  -10.7 dBm at the receiver
10 m  ->  -30.7 dBm at the receiver
SF7 / 125 kHz sensitivity  ->  about -123 dBm
```

**That is roughly 112 dB more signal than the receiver needs.** Two radios a
metre apart with full antennas are not a weak-signal experiment; they are an
overload experiment.

## What that does to the measurement

**SNR saturates.** The useful window runs from the demodulator floor up to
somewhere around +10 dB. Past that the estimator compresses and the reading
pins near its top regardless of how much stronger the signal gets. A row of
packets all reporting +9 or +10 dB is not a great link — it is a number that has
stopped carrying information.

Demodulator floors, datasheet Table 13:

| SF | 6 | 7 | 8 | 9 | 10 | 11 | 12 |
|---|---|---|---|---|---|---|---|
| SNR | −5 | **−7.5** | −10 | −12.5 | −15 | −17.5 | **−20** dB |

**Packets can get worse when you move closer.** A saturated front end produces
CRC failures that look like a bad link but are caused by too much signal. If
errors rise as the radios approach, suspect overload before anything else.

## Getting into the measurable range

You need to throw away roughly 80 dB. No single lever does it; stack them.

| Lever | Gain | Notes |
|---|---|---|
| Transmit power 17 → 2 dBm | **15 dB** | Free. `kDefaultTxPowerDbm` in `lib/board/board.h`; the SX1276 accepts 2–17 on PA_BOOST |
| Coaxial attenuator | **20–40 dB** | The proper answer. RP-SMA on this board — check the polarity |
| Distance 1 m → 10 m | **20 dB** | Different rooms is better still |
| Smaller antenna | 2–5 dB | Marginal on its own; see `docs/antenna.md` |

Minimum power + a 40 dB attenuator + 10 m gets you to about −86 dBm, which sits
comfortably in the range where SNR moves when conditions do.

**Do not simply unscrew the antenna.** Transmitting into an open connector
reflects the power back into the PA and can damage it — an RFM95HW is the
+20 dBm build. If you want the antenna off, put a 50 Ω dummy load or an
attenuator on instead.

## Lab mode, on our side

Reducing *our* transmit power does nothing while we are only listening — the
overload comes from their transmitter. But the SX1276 has a receive-side
attenuator, and `RegLna` bits 7-5 give up to **-48 dB** at the front end. That
is a coaxial attenuator you already own.

`src/apps/listen` toggles it at runtime, no reflash:

| key | effect |
|---|---|
| `l` | **lab**: LNA -48 dB, AGC off, TX 2 dBm |
| `n` | **normal**: LNA max, AGC on, TX 17 dBm |
| `?` | current mode, noise floor and counters |

The heartbeat line shows which mode is active, so a capture is never ambiguous.

### Measured, 2026-08-21

| Mode | Reported floor |
|---|---|
| normal | -120 dBm |
| **lab** | **-77 dBm** |
| normal again | -121 dBm |

**43 dB of desensitisation**, against a nominal -48 dB for the G1 to G6 step
plus the LNA boost switching off. Repeatable across three toggles.

**The reported floor goes UP in lab mode, and that is correct.** It is easy to
read -77 dBm as "noisier" and assume something broke. What the number means is
that the receiver is now 43 dB less sensitive: with the LNA turned down, the
mixer and ADC noise dominate instead of the amplified antenna noise, so the
noise floor *referred to the input* rises by very nearly the attenuation. That
is exactly what a coaxial attenuator does to a receiver, and it is the whole
point — a signal has to be 43 dB stronger to be heard.

Two consequences when reading a capture:

- **RSSI and SNR reported in lab mode are not comparable** with figures taken
  in normal mode. Record the mode alongside every measurement; the heartbeat
  does this for you.
- Sensitivity in lab mode is roughly -80 dBm rather than -123 dBm. At one metre
  a 2 dBm transmitter still arrives around -26 dBm, so there is plenty of margin
  for bench work -- but anything across the building will vanish.

**AGC has to be off for this to do anything.** With `AgcAutoOn` set the radio
picks the LNA gain itself and silently ignores `RegLna` — a manual setting looks
applied and changes nothing. `setLnaGain()` clears AGC for you; `n` puts it back.

It is a runtime toggle rather than a build flag on purpose: on this board every
reflash risks a GPIO12 boot-loop that needs a physical power cycle, so changing
a register is much cheaper than rebuilding.

For the transmitting side, `pio run -e tx-lab` is the same beacon at 2 dBm.

**Lab mode will miss weak signals** — that is the point of it. Press `n` before
concluding the band is quiet.

## Removing the antenna does not lower the noise floor

Measured 2026-08-21, same board, same firmware, minutes apart:

| | whip attached | whip removed (pigtail only) |
|---|---|---|
| normal | -121 dBm | **-121 dBm** |
| lab | -78 dBm | **-78 dBm** |

**No change, either mode.** That is the expected result and it is worth
understanding, because "take the antenna off to reduce noise" is a natural
assumption and it is wrong about which quantity moves.

The idle floor here is set by the receiver's **own** noise — mixer and ADC —
not by noise arriving through the antenna. Internal noise does not care what is
attached to the connector, so removing the antenna leaves the floor alone.

What removing it *does* change is **received signal**, by 20-40 dB. That is
genuinely useful when the problem is a transmitter a metre away, but it shows up
in the RSSI and SNR of an actual packet, never in the idle floor. **Do not use
the floor to check whether an antenna change worked.**

Receiving with no antenna is electrically harmless. Transmitting into an open
connector is not — see the warning above.

## The floor moves on its own, by more than you would expect

Same board, same firmware, same antenna, a couple of hours apart:

```
earlier :  -106 / -107 dBm   (repeatedly, over two 150 s samples)
later   :  -121 / -122 dBm
```

**15 dB, environmental.** Nothing in the configuration changed between them.
The band is shared and busy, and what is transmitting nearby varies by the hour.

This is why step 5 below says record the floor every session. An SNR figure
without the floor it was measured against cannot be compared with one taken on
another day — a signal at -100 dBm is 6 dB above one floor and 21 dB above the
other.

## A procedure that yields something

1. Both ends on the same **frequency, spreading factor, bandwidth and sync
   word**. A mismatch in any one gives silence with no error at either end. See
   `docs/radio-settings-request.md`.
2. Transmitter at **minimum power**. Add attenuation until the receiver reports
   SNR in roughly −5 to +5 dB. If it reads +9/+10, you are still saturated.
3. Fix everything, then **change one variable at a time** — distance, or a wall,
   or antenna orientation — and record SNR, RSSI and loss.
4. Watch **packet loss**, not just SNR. `src/apps/link` numbers its beacons so
   the receiver counts gaps; loss is the number that matters at range, and it
   goes non-linear near the floor while SNR still looks survivable.
5. Note the **idle noise floor** each session. Ours reads about −106 dBm indoors
   at 125 kHz. Comparing SNR across days without it is comparing nothing.

## What we need from the transmitting side

- Transmit power in dBm, and whether it can be lowered
- Frequency, spreading factor, bandwidth, sync word — all four
- Whether the payload carries a CRC. Our receiver flags packets whose header
  declares none, because noise decodes into plausible CRC-less packets and we
  will not report those as data
- Something identifiable in the payload — a sequence number is ideal, since it
  turns "we heard something" into a measured loss rate

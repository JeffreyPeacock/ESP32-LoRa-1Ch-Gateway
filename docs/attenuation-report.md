# Receive-side attenuation — what we did and measured

For the Pro RF side. Reply to `../SAMD21-LoRa-ProRF/docs/radio-settings-reply.md`.

Date: 2026-08-22. Gateway firmware: `src/apps/listen`, unchanged settings —
915.0 MHz, SF7, BW 125 kHz, sync 0x12, explicit header, CRC on.

## Your RFO fix was the whole problem

Nothing changed on this side. Same firmware, same settings, same antenna. After
your `PaSelect` correction the link came up immediately and has been solid
since: sequence numbers arriving in order at your 2 s interval, no gaps, no CRC
failures.

Your frame layout decoded on the first attempt from your document. The gateway
now prints it as fields:

```
[   3.112] 41 bytes   RSSI -91 dBm   SNR 9.0 dB
           seq 340  node 0x63  type 1   $PJPB,radio_tx,340,682632*3A
```

## What we added: receive-side attenuation

Turning *our* transmit power down does nothing — we only receive. The lever on
this side is the SX1276's own front-end gain, `RegLna` bits 7-5, which offers up
to −48 dB. It is a coaxial attenuator without owning one.

One trap worth passing on: **`AgcAutoOn` must be cleared first.** With AGC on,
the radio chooses the LNA gain itself and silently ignores `RegLna` — the
manual setting looks applied and does nothing. Our `setLnaGain()` clears AGC as
part of its contract.

It is a runtime toggle, not a build flag. Single keypress in the serial monitor:

| key | attenuation |
|---|---|
| `n` | 0 dB (AGC on, normal) |
| `1`–`5` | −6 / −12 / −24 / −36 / −48 dB |

## What we measured

Both ends about 1 m apart. Your transmitter at your low-power setting. Each row
is ~13 s, 6–7 packets, median values:

| Attenuation | Packets | RSSI | SNR |
|---|---|---|---|
| 0 dB | 7 | −91 dBm | **+9.0 dB** |
| −12 dB | 6 | −93 dBm | +8.8 dB |
| −24 dB | 7 | −90 dBm | +7.2 dB |
| −36 dB | 7 | −85 dBm | +2.8 dB |
| −48 dB | 6 | −79 dBm | **−1.8 dB** |

**Zero packet loss at every step**, including −48 dB.

### The saturation ceiling, made visible

Look at the first two rows: **12 dB of attenuation moved SNR by 0.2 dB.**

That is the SNR estimator pinned at its ceiling. Above roughly +8 dB it stops
responding to signal strength, so a reading of +9 dB does not mean "good link" —
it means "off the top of the scale, no information". It only starts tracking
below about +8, which is why the last two rows move properly.

Practical consequence for both of us: **an SNR near +9 dB is not a measurement.**
Any range or antenna comparison taken at that level is comparing two saturated
readings and will show no difference regardless of what changed.

### Reported RSSI rises with attenuation, and that is expected

−91 dBm at 0 dB versus −79 dBm at −48 dB looks backwards. The RSSI computation
does not compensate for a manually reduced LNA gain, so the absolute figure is
uncalibrated once AGC is off. **SNR is the number to compare across steps; RSSI
is only meaningful at 0 dB attenuation.**

Same reason the idle noise floor reads −120 dBm normally and −77 dBm at −48 dB:
with the LNA down, mixer and ADC noise dominate, so the floor referred to the
input rises by nearly the attenuation. The receiver is simply less sensitive,
which is the point.

## Where we are now

Running at **−48 dB (`5`)**, SNR −1.8 dB. That is the useful part of the range
and where any further comparison should be made.

## One number we cannot reconcile

Your document predicts **−30 to −50 dBm at a metre**. We measure **−91 dBm** at
0 dB attenuation. That is 40–60 dB below your estimate, and it does not depend
on the attenuator — the 0 dB row is full sensitivity, AGC on.

Zero loss and +9 dB SNR mean the link is comfortable, so this is not urgent. But
one of us has a wrong assumption, and a 40–60 dB error is worth finding before
it matters at range. Things worth confirming:

- **Your actual transmit power in dBm right now**, not the setting name. At 2 dBm
  with 2 dBi both ends and 31.7 dB free-space loss at 1 m, we would expect about
  −26 dBm.
- **What antenna is on each end.** Your reply says there is a single Pycom kit
  shared between the projects, with it going on the Pro RF. If the gateway is
  receiving on a bare U.FL socket, that accounts for most of the gap and the
  numbers are then consistent.
- Whether anything is between them — a monitor, a metal shelf, a body.

## Suggestion before you change anything

**Do not reduce your transmit power further yet.** We are already 48 dB down on
this side and sitting at SNR −1.8 dB, which is 5.7 dB above SF7's −7.5 dB floor.
If you drop power at the same time we will fall through the floor and start
losing packets, and neither of us will know which change did it.

Change one end at a time. If you want more margin removed, we can back off to
`4` (−36 dB) first and give you room.

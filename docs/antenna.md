# Antennas

The antenna that came with this board, and what to use on the bench instead.

## What we have: Pycom LoRa & Sigfox Antenna Kit

Named on the SparkFun invoice as "Pycom LoRa and Sigfox Antenna Kit - 915MHz".
SparkFun no longer lists it. DigiKey's page survives as
[7721843](https://www.digikey.com/en/products/detail/pycom-ltd/SIGFOX-LORA-ANTENNA-KIT/7721843),
marked **discontinued**.

| | |
|---|---|
| Manufacturer | Pycom Ltd. |
| Part number | `SIGFOX/LORA ANTENNA KIT` |
| Type | Whip, tilt/swivel — half-wave |
| Band | 868 MHz / 915 MHz LoRa only |
| Antenna connector | **RP-SMA male** |
| Pigtail | RP-SMA female to U.FL / I-PEX MHF1, 1.13 mm coax, **100 mm** |
| Antenna length | ~200 mm |
| Mounting | connector mount |
| Gain | **not published** |

**The gain figure does not exist.** Pycom never published a datasheet for it,
and distributors leave the field blank — [their own forum has a thread asking
for it](https://forum.pycom.io/topic/3396/official-pycom-lora-sigfox-antenna-datasheet).
Comparable 868/915 MHz half-wave whips are 2–3 dBi, but that is an inference
from similar parts, not a specification of this one. Do not put a number in a
link budget and call it measured.

Same for VSWR, impedance (50 Ω is a safe assumption for the connector type),
max power and polarisation (whips are linear/vertical when upright). None are
published.

## Can you just take the big part off?

**Yes — the whip unscrews at the RP-SMA joint.** The kit is two pieces: the
100 mm U.FL-to-RP-SMA pigtail and the whip that screws onto it.

**But do not transmit with the bare pigtail.** An unterminated connector
reflects essentially all the power back into the PA. The radio here is an
RFM95HW, the +20 dBm build, and its PA_BOOST output has nowhere to dump that.
Receiving with no antenna is harmless; transmitting is not. This is why
`src/link` defaults to the receive role and the `tx` env warns at startup.

## Watch the connector polarity

The kit is **RP-SMA**, not SMA. They mate mechanically and will not work: RP-SMA
reverses which half carries the pin. A replacement antenna must be **RP-SMA
male** to screw onto this pigtail. Plenty of 915 MHz LoRa antennas are sold as
plain SMA, and buying one is the usual way to lose an evening.

## Smaller options, in order of how little they cost you

### 1. Keep the pigtail, swap the whip for a stubby — recommended

A 915 MHz **RP-SMA male stubby** is roughly 50–60 mm against the Pycom's 200 mm,
and screws straight onto the pigtail you already have. Typically 1–2 dBi, which
is a little down on a half-wave whip and completely irrelevant across a desk.
Nothing to solder and nothing to undo later.

Search terms that get the right part: "915 MHz RP-SMA male stubby antenna",
"ISM 915 MHz RP-SMA rubber duck 2 dBi". Confirm **RP-SMA**, not SMA.

### 2. A wire quarter-wave on the ANT pad — free

SparkFun's own hookup guide gives the lengths:

| Frequency | Length |
|---|---|
| 915 MHz | **3.07 in / 78 mm** |
| 434 MHz | 6.47 in / 164 mm |

A strip of 22 AWG solid core soldered to the `ANT` pad, sticking straight up.

**Check the jumper first.** The schematic carries a jumper labelled *"Close if
using U.FL connector"*, so the board selects between the U.FL socket and the
`ANT` through-hole pad. Whichever path you want has to be the one that is
connected — verify it on the PCB rather than assuming, because a board that
transmits into a disconnected path has the same problem as one with no antenna.

### 3. Attenuate instead of shrinking

For bench work at a metre or two the real problem is usually too much signal,
not too little. At 17 dBm into a receiver on the same desk you can saturate the
front end, and the symptom — packets that fail CRC when close and succeed when
further away — looks like a bug rather than overload.

Cheaper than any antenna: drop transmit power. `src/board.h` sets
`kDefaultTxPowerDbm = 17`; the driver accepts 2 dBm, and 2 dBm with a stubby is
plenty for a room. Change one constant and rebuild.

## For the actual deployment

Keep the Pycom kit. A tilt-swivel half-wave on 100 mm of coax is a reasonable
outdoor antenna, and its length is the reason it works. The 200 mm that makes it
awkward on a desk is what buys the range at FTG1/SJC/SNA.

If it is ever replaced, the specification to match is a 915 MHz half-wave or
better, RP-SMA male, and — unlike the Pycom — one with a published gain figure
and VSWR curve, so a link budget can be calculated rather than guessed.

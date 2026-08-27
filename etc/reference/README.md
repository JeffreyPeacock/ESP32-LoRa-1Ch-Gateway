# Reference sketches

Third-party code kept for reference, not built by this project. Unlike
`etc/firmware/` and `docs/datasheets/`, this **is** committed — the licence
allows it.

## `ESP-1CH-TTN-Device-ABP.ino`

Turns this board into a LoRaWAN **device** rather than a gateway: it joins by
ABP with fixed session keys and sends "Hello, world!" on a timer and on the
button. SparkFun's intent was that you buy two boards and run one of each.

| | |
|---|---|
| Author | Jim Lindblom, SparkFun Electronics |
| Date | 31 July 2018 |
| Licence | Beerware — "public domain but you buy me a beer" |
| Dependency | `mcci-catena/arduino-lmic` |

Found in a local Arduino sketchbook and copied here on 2026-08-21. It is the
unmodified tutorial download; see below.

### It was never edited

Byte-identical to SparkFun's tutorial download
(`etc/firmware/sparkfun-tutorial-ESP-1CH-TTN-Device-ABP-v01.zip`). Normalising
line endings, it is also identical to the copies in both hardware repos — the
only byte-level difference anywhere is a single stray CRLF. All four share the
normalised hash `29cf0c3926591010`.

**It would not compile as it stands.** The session keys are literal
placeholders:

```c
static const PROGMEM u1_t NWKSKEY[16] = { PASTE_NWKSKEY_KEY_HERE };
static const u1_t PROGMEM APPSKEY[16] = { PASTE_APPSKEY_KEY_HERE };
static const u4_t DEVADDR = 0xPASTE_DEV_ADDR_HERE;
```

Those are not valid C. So this sketch was downloaded and opened, but never
built and never flashed — which fits the board running the *gateway* firmware,
and fits not remembering any of it.

It also means there are **no secrets in this file**, which is why it can be
committed to a public repository.

### Two traps, if it is ever used

**1. The US channel is wrong for our gateway.** Channel selection is
conditional on how the LMIC library was configured, and the US branch reads:

```c
#elif defined(CFG_us915)
  for (int b = 0; b < 8; ++b) LMIC_disableSubBand(b);
  //LMIC_enableChannel(8); // 903.9 MHz
  LMIC_enableChannel(17);
#endif
```

US915 uplink channels are `902.3 + 0.2 × n` MHz, so:

| Channel | Frequency | |
|---|---|---|
| 8 | **903.9 MHz** | what this board's gateway firmware listens on — and it is the line that is commented out |
| 17 | **905.7 MHz** | what the sketch actually enables |

As shipped, the device transmits 1.8 MHz away from where the gateway listens,
and hears nothing back. The one-line fix is already in the file, commented out.

**2. The region comes from the library, not the sketch.** `CFG_us915` versus
`CFG_eu868` is set in `lmic_project_config.h` inside arduino-lmic. Leave it at
the default and the `#if defined(CFG_eu868)` branch compiles instead, putting
the device on **868.1 MHz** — legal in the EU, wrong here. The tutorial says to
configure the library first; this is what it means.

### The pin map in it is right, except RST

```c
const lmic_pinmap lmic_pins = {
  .nss = 16, .rxtx = LMIC_UNUSED_PIN, .rst = 5, .dio = {26, 33, 32},
};
```

NSS 16 and DIO 26/33/32 are correct. **`.rst = 5` is not** — `src/probe`
pulsed GPIO 5 against the live radio and it does not reset it, nor does the 27
that SparkFun's gateway tutorial gives. See `CLAUDE.md`.

Note also what is absent: the SPI pins. LMIC uses the default `SPI` object, so
the bus comes from the Arduino **variant**, `sparkx_esp32_lora`, which sets
`SCK 14 / MISO 12 / MOSI 13`. Build this against a stock `esp32dev` definition
and it compiles, runs, and never reaches the radio.

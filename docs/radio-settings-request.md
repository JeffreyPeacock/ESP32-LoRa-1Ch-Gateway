# Radio settings — please send me these five

Copy-paste this to whoever owns the other radios.

---

I'm trying to receive your LoRa traffic. A LoRa receiver only demodulates a
signal when **every** one of these matches — miss one and I hear nothing at all,
with no error on either side. So I need all five, not just the frequency.

```
Frequency        : ________ MHz     (exact, e.g. 915.0 — not "915 band")
Spreading factor : SF__            (7-12)
Bandwidth        : ___ kHz         (usually 125, 250 or 500)
Coding rate      : 4/_             (5-8)
Sync word        : 0x__            (0x12 private, 0x34 LoRaWAN, 0x2b Meshtastic)
```

Where to read them off, depending on what you're running:

- **RadioLib** — the `radio.begin(freq, bw, sf, cr, syncWord, power, ...)` call
- **arduino-LoRa (sandeepmistry)** — `LoRa.begin()`, then `setSpreadingFactor`,
  `setSignalBandwidth`, `setCodingRate4`, `setSyncWord`. Anything you don't call
  is at its default: 915 MHz / SF7 / 125 kHz / 4/5 / 0x12
- **Meshtastic** — Settings → LoRa → Region and Modem preset. Send me the preset
  name (e.g. "LongFast") and region; that fixes all five
- **LoRaWAN (LMIC, or a commercial node)** — send the region (e.g. US915) and
  the sub-band or channel list. LoRaWAN hops across channels, so there may be no
  single frequency — say so if that's the case
- **Raw SX127x/SX126x register code** — RegFrf, RegModemConfig1/2, RegSyncWord

Two things that trip people up:

**"915 MHz" is a band, not a frequency.** It covers 902–928 MHz. I need the
actual number you transmit on.

**Sync word is the one people forget.** It's a single byte that acts as a
network filter. `0x12` and `0x34` are otherwise-identical signals that cannot
hear each other.

If it's easier, just send the config file or the setup function — I can read it.

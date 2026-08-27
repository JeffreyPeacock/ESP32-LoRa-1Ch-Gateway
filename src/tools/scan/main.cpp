// Scanner -- what can this radio actually hear?
//
// The honest answer to "show me the peers" is that LoRa has none. At the
// physical layer there is no association, no handshake and no client list: a
// receiver demodulates whatever happens to match its settings and drops
// everything else. "Peers" only exist one layer up -- LoRaWAN has DevAddrs,
// Meshtastic has a node database -- and this board is not running either.
//
// Worse, a LoRa receiver is deaf by default. It hears a transmission only when
// frequency, bandwidth, spreading factor AND sync word all match. Our own link
// test uses sync word 0x12, which means it cannot hear LoRaWAN traffic (0x34)
// or Meshtastic (0x2b) even on the right frequency. So "nothing received" from
// a single-configuration receiver says almost nothing about what is on the air.
//
// This sweeps a list of known configurations, dwelling on each, and reports
// anything that demodulates along with the settings that caught it. Finding a
// packet tells you what is out there AND what to configure to talk to it.

#include <Arduino.h>

#include "board.h"
#include "sx1276.h"

namespace {

SX1276 radio;

struct Preset {
    const char* name;
    uint32_t    frequencyHz;
    uint8_t     spreadingFactor;
    uint32_t    bandwidthHz;
    uint8_t     syncWord;
};

// Curated rather than a blind cross-product. Sweeping every combination of
// six spreading factors, three bandwidths and three sync words is 54 dwells --
// several minutes per pass, most of it on combinations nothing uses.
//
// These defaults move between releases of the stacks they belong to, so treat
// them as a starting point. A hit tells you the settings for certain; a miss
// does not prove absence.
constexpr Preset kPresets[] = {
    // What this project's own link test transmits -- raw LoRa on 915.0, which
    // is not a LoRaWAN channel. Keep in step with board.h.
    {"link-test SF7",      915000000UL,  7, 125000UL, 0x12},

    // US915 LoRaWAN uplink channel 8 = 902.3 + 0.2*8. This is the FIRST channel
    // of sub-band 2, which US networks use -- not channel 0, which is 902.3 and
    // which this project's notes wrongly called it for a while. Sync word 0x34
    // is the public LoRaWAN word; a receiver here with 0x12 hears nothing.
    {"LoRaWAN ch8 SF7",    903900000UL,  7, 125000UL, 0x34},
    {"LoRaWAN ch8 SF8",    903900000UL,  8, 125000UL, 0x34},
    {"LoRaWAN ch8 SF9",    903900000UL,  9, 125000UL, 0x34},
    {"LoRaWAN ch8 SF10",   903900000UL, 10, 125000UL, 0x34},

    // US915 LoRaWAN 500 kHz uplink, the SF8 channel that sits above the 125 kHz
    // block.
    {"LoRaWAN ch65 500k",  904600000UL,  8, 500000UL, 0x34},

    // Meshtastic US. LongFast is the out-of-the-box preset, so it is the one
    // most likely to be in the air near anyone else experimenting.
    {"Meshtastic LongFast",906875000UL, 11, 250000UL, 0x2b},
    {"Meshtastic MedFast", 906875000UL,  9, 250000UL, 0x2b},
};

constexpr size_t   kPresetCount = sizeof(kPresets) / sizeof(kPresets[0]);
constexpr uint32_t kDwellMs     = 6000;

uint32_t heardPerPreset[kPresetCount] = {0};
uint32_t sweep = 0;

SX1276::Config configFor(const Preset& preset) {
    return SX1276::Config{
        .frequencyHz     = preset.frequencyHz,
        .spreadingFactor = preset.spreadingFactor,
        .bandwidthHz     = preset.bandwidthHz,
        .codingRate4     = board::kDefaultCodingRate4,
        .txPowerDbm      = board::kDefaultTxPowerDbm,  // unused; never transmits
        .syncWord        = preset.syncWord,
    };
}

// Payloads are usually encrypted (LoRaWAN and Meshtastic both encrypt), so a
// readable string is the exception. Print both: ASCII where it is printable,
// hex otherwise, because the first bytes still identify the protocol.
void printPayload(const SX1276::Packet& packet) {
    Serial.print("           hex  ");
    const uint8_t n = min<uint8_t>(packet.length, 32);
    for (uint8_t i = 0; i < n; ++i) {
        Serial.printf("%02X ", packet.data[i]);
    }
    if (packet.length > n) Serial.print("...");
    Serial.println();

    Serial.print("           text ");
    for (uint8_t i = 0; i < n; ++i) {
        const char c = static_cast<char>(packet.data[i]);
        Serial.print((c >= 32 && c < 127) ? c : '.');
    }
    Serial.println();
}

}  // namespace

void setup() {
    Serial.begin(115200);
    delay(1500);

    Serial.println();
    Serial.println("=== LoRa scanner ===");
    Serial.println("Sweeping known configurations. A LoRa receiver only hears");
    Serial.println("traffic matching frequency, bandwidth, SF and sync word, so");
    Serial.println("silence on one setting proves nothing about the others.");
    Serial.println();

    if (!radio.begin(configFor(kPresets[0]))) {
        Serial.println("RADIO NOT FOUND. Check src/board.h -- this board is");
        Serial.println("SCK 14 / MISO 12 / MOSI 13 / NSS 16, not the VSPI defaults.");
        return;
    }
    Serial.printf("radio: SX1276 found. %u presets, %lu ms each, "
                  "%lu s per sweep.\n\n",
                  static_cast<unsigned>(kPresetCount),
                  static_cast<unsigned long>(kDwellMs),
                  static_cast<unsigned long>(kPresetCount * kDwellMs / 1000));
}

void loop() {
    ++sweep;

    for (size_t i = 0; i < kPresetCount; ++i) {
        const Preset& preset = kPresets[i];
        radio.reconfigure(configFor(preset));
        radio.listen();
        delay(50);   // let the front end settle before reading RSSI

        const int16_t floorDbm = radio.rssiNowDbm();
        uint32_t heardHere = 0;

        const uint32_t until = millis() + kDwellMs;
        while (static_cast<int32_t>(millis() - until) < 0) {
            SX1276::Packet packet;
            if (!radio.poll(&packet)) {
                delay(2);
                continue;
            }
            ++heardHere;
            ++heardPerPreset[i];

            Serial.printf("[%6lu] HEARD on %-21s %.3f MHz SF%u BW%lu sync 0x%02X\n",
                          static_cast<unsigned long>(millis() / 1000),
                          preset.name, preset.frequencyHz / 1e6,
                          preset.spreadingFactor,
                          static_cast<unsigned long>(preset.bandwidthHz / 1000),
                          preset.syncWord);
            Serial.printf("           %u bytes  RSSI %d dBm  SNR %.1f dB\n",
                          packet.length, packet.rssiDbm, packet.snrDb);
            printPayload(packet);
        }

        Serial.printf("  sweep %-3lu %-21s floor %4d dBm  heard %lu  "
                      "(total %lu)  CRC err %lu\n",
                      static_cast<unsigned long>(sweep), preset.name, floorDbm,
                      static_cast<unsigned long>(heardHere),
                      static_cast<unsigned long>(heardPerPreset[i]),
                      static_cast<unsigned long>(radio.crcErrors()));
    }

    // CRC errors are worth watching even when nothing decodes: they mean
    // something with the right sync word and modulation IS transmitting and
    // arriving corrupted. That is a signal, not noise.
    Serial.printf("--- sweep %lu complete, %lu CRC errors so far ---\n\n",
                  static_cast<unsigned long>(sweep),
                  static_cast<unsigned long>(radio.crcErrors()));
}

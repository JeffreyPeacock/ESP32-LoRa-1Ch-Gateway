// Sweep the modulation grid at one frequency, to find a transmitter you know
// is there but cannot hear.
//
// src/tools/scan sweeps named presets -- LoRaWAN, Meshtastic, our link test.
// This is the other question: the frequency is agreed, both ends are running,
// and nothing arrives. That means one of the remaining parameters disagrees,
// and the fastest way to find out which is to try them all.
//
// Grid: spreading factor 7-12 x bandwidth 125/250 kHz x sync word 0x12/0x34.
// Coding rate is deliberately NOT swept: it travels in the explicit header, so
// the receiver adapts to whatever arrives.
//
// TELL THE OTHER END TO TRANSMIT CONTINUOUSLY, or at least every 2-3 seconds,
// while this runs. Each combination is only listened to for a few seconds, and
// a beacon every 30 s will be missed by almost all of them.

#include <Arduino.h>

#include "board.h"
#include "sx1276.h"

namespace {

SX1276 radio;

constexpr uint32_t kFrequencyHz = board::kDefaultFrequencyHz;   // 915.0 MHz
constexpr uint32_t kDwellMs     = 5000;

constexpr uint8_t  kSpreadingFactors[] = {7, 8, 9, 10, 11, 12};
constexpr uint32_t kBandwidths[]       = {125000UL, 250000UL};
constexpr uint8_t  kSyncWords[]        = {0x12, 0x34};

constexpr size_t kCombinations =
    sizeof(kSpreadingFactors) / sizeof(kSpreadingFactors[0]) *
    sizeof(kBandwidths)       / sizeof(kBandwidths[0]) *
    sizeof(kSyncWords)        / sizeof(kSyncWords[0]);

uint32_t sweepNumber = 0;

void report(const char* what, uint8_t sf, uint32_t bw, uint8_t sync,
            const SX1276::Packet* packet) {
    Serial.printf("\n*** %s at SF%u / BW %lu kHz / sync 0x%02X ***\n",
                  what, sf, static_cast<unsigned long>(bw / 1000), sync);
    if (packet != nullptr) {
        Serial.printf("    %u bytes  RSSI %d dBm  SNR %.1f dB%s\n",
                      packet->length, packet->rssiDbm, packet->snrDb,
                      packet->crcPresent ? "" : "  (NO CRC -- unverified)");
        Serial.print("    text ");
        for (uint8_t i = 0; i < packet->length && i < 48; ++i) {
            const char c = static_cast<char>(packet->data[i]);
            Serial.print((c >= 32 && c < 127) ? c : '.');
        }
        Serial.println();
    }
    Serial.println("    ^^ set the receiver to these and it will hear it.\n");
}

}  // namespace

void setup() {
    Serial.begin(115200);
    delay(1500);
    Serial.println("\n=== LoRa modulation sweep ===");
    Serial.printf("frequency fixed at %.3f MHz\n", kFrequencyHz / 1e6);
    Serial.printf("trying %u combinations, %lu ms each -- %lu s per pass\n",
                  static_cast<unsigned>(kCombinations),
                  static_cast<unsigned long>(kDwellMs),
                  static_cast<unsigned long>(kCombinations * kDwellMs / 1000));
    Serial.println("HAVE THE OTHER END TRANSMIT CONTINUOUSLY WHILE THIS RUNS.");
    Serial.println("A beacon every 30 s will be missed by nearly every dwell.\n");

    SX1276::Config config = {
        .frequencyHz     = kFrequencyHz,
        .spreadingFactor = 7,
        .bandwidthHz     = 125000UL,
        .codingRate4     = board::kDefaultCodingRate4,
        .txPowerDbm      = board::kDefaultTxPowerDbm,   // unused; never transmits
        .syncWord        = 0x12,
    };
    if (!radio.begin(config)) {
        Serial.println("RADIO NOT FOUND -- check lib/board/board.h");
        return;
    }
    Serial.println("radio: SX1276 found\n");
}

void loop() {
    ++sweepNumber;

    for (uint8_t sf : kSpreadingFactors) {
        for (uint32_t bw : kBandwidths) {
            for (uint8_t sync : kSyncWords) {
                const SX1276::Config config = {
                    .frequencyHz     = kFrequencyHz,
                    .spreadingFactor = sf,
                    .bandwidthHz     = bw,
                    .codingRate4     = board::kDefaultCodingRate4,
                    .txPowerDbm      = board::kDefaultTxPowerDbm,
                    .syncWord        = sync,
                };
                radio.reconfigure(config);
                radio.listen();
                delay(30);

                const uint32_t crcBefore = radio.crcErrors();
                bool heard = false;

                const uint32_t until = millis() + kDwellMs;
                while (static_cast<int32_t>(millis() - until) < 0) {
                    SX1276::Packet packet;
                    if (radio.poll(&packet)) {
                        heard = true;
                        report("HEARD", sf, bw, sync, &packet);
                    }
                    delay(2);
                }

                // A CRC error is nearly as informative as a packet: the
                // modulation and sync word matched well enough to demodulate,
                // and only the payload was corrupt. That usually means these
                // settings are right and the signal is bad -- or, on a bench,
                // that the front end is being overloaded.
                if (!heard && radio.crcErrors() > crcBefore) {
                    report("CRC ERROR -- right settings, corrupt payload",
                           sf, bw, sync, nullptr);
                }

                if (!heard) {
                    Serial.printf("  pass %-3lu SF%-2u BW%-3lu sync 0x%02X  "
                                  "floor %4d dBm\n",
                                  static_cast<unsigned long>(sweepNumber), sf,
                                  static_cast<unsigned long>(bw / 1000), sync,
                                  radio.rssiNowDbm());
                }
            }
        }
    }

    Serial.printf("--- pass %lu complete, %lu CRC errors total ---\n\n",
                  static_cast<unsigned long>(sweepNumber),
                  static_cast<unsigned long>(radio.crcErrors()));
}

// Link test for the SparkFun LoRa Gateway 1-Channel (ESP32).
//
// One firmware, two roles chosen at build time:
//
//   pio run -e rx -t upload     listen, and report every packet with RSSI/SNR
//   pio run -e tx -t upload     beacon a numbered packet every few seconds
//
// The point is a range measurement between this board and a second radio, which
// is what the wider effort needs -- see the sibling Heltec project. Numbering
// the beacons is what makes the receiver able to report loss rather than just
// "quiet", and loss is the number that matters at range.
//
// RX is the safe default. Transmitting without an antenna can damage the PA on
// an RFM95HW, so the TX role refuses to start unless it is built deliberately.

#include <Arduino.h>

#include "board.h"
#include "sx1276.h"

namespace {

SX1276 radio;

constexpr uint32_t kBeaconIntervalMs = 5000;

// Receiver bookkeeping. Sequence numbers come from the beacon payload, so a
// gap is a lost packet -- the receiver never has to guess.
uint32_t received      = 0;
uint32_t lastSequence  = 0;
bool     haveSequence  = false;
uint32_t missed        = 0;

void printConfig() {
    Serial.printf("  frequency  %.3f MHz\n", board::kDefaultFrequencyHz / 1e6);
    Serial.printf("  modulation SF%u / BW %lu kHz / CR 4/%u\n",
                  board::kDefaultSpreadingFactor,
                  static_cast<unsigned long>(board::kDefaultBandwidthHz / 1000),
                  board::kDefaultCodingRate4);
    Serial.printf("  sync word  0x%02X (private LoRa, not LoRaWAN)\n",
                  board::kDefaultSyncWord);
}

}  // namespace

void setup() {
    Serial.begin(115200);
    delay(1500);

    Serial.println();
#if defined(ROLE_TX)
    Serial.println("=== LoRa link test -- TRANSMITTER ===");
#else
    Serial.println("=== LoRa link test -- RECEIVER ===");
#endif

    const SX1276::Config config = {
        .frequencyHz     = board::kDefaultFrequencyHz,
        .spreadingFactor = board::kDefaultSpreadingFactor,
        .bandwidthHz     = board::kDefaultBandwidthHz,
        .codingRate4     = board::kDefaultCodingRate4,
#if defined(LAB_MODE)
        .txPowerDbm      = board::kLabTxPowerDbm,
#else
        .txPowerDbm      = board::kDefaultTxPowerDbm,
#endif
        .syncWord        = board::kDefaultSyncWord,
    };

    if (!radio.begin(config)) {
        // Almost always a pin map problem. Say so, because the alternative is
        // a board that looks fine and hears nothing -- this project's oldest
        // trap.
        Serial.println("RADIO NOT FOUND.");
        Serial.printf("  RegVersion read 0x%02X, expected 0x12\n",
                      radio.readRegister(0x42));
        Serial.println("  Check src/board.h against the wiring. This board is");
        Serial.println("  SCK 14 / MISO 12 / MOSI 13 / NSS 16, NOT the VSPI");
        Serial.println("  defaults every stock ESP32 target assumes.");
        return;
    }

    Serial.println("radio: SX1276 found");
    printConfig();

#if defined(ROLE_TX)
#if defined(LAB_MODE)
    Serial.printf("  tx power   %u dBm on PA_BOOST  *** LAB MODE ***\n",
                  board::kLabTxPowerDbm);
#else
    Serial.printf("  tx power   %u dBm on PA_BOOST\n", board::kDefaultTxPowerDbm);
#endif
    Serial.println("\n*** ANTENNA MUST BE ATTACHED. Transmitting into an open");
    Serial.println("*** U.FL connector can damage the PA on an RFM95HW.");
    Serial.printf("\nbeaconing every %lu ms\n\n",
                  static_cast<unsigned long>(kBeaconIntervalMs));
#else
    radio.listen();
    // The RSSI register needs a moment after entering receive. Read it too
    // early and it returns 0, which the offset turns into a confident-looking
    // -157 dBm -- a plausible number that is really "no reading yet".
    delay(50);
    Serial.printf("  noise floor %d dBm\n", radio.rssiNowDbm());
    Serial.println("\nlistening\n");
#endif
}

#if defined(ROLE_TX)

void loop() {
    static uint32_t sequence = 0;

    char payload[64];
    const int length = snprintf(payload, sizeof(payload),
                                "SPARKX-1CH seq=%lu up=%lus",
                                static_cast<unsigned long>(++sequence),
                                static_cast<unsigned long>(millis() / 1000));

    const uint32_t startedAt = millis();
    const bool sent = radio.transmit(reinterpret_cast<const uint8_t*>(payload),
                                     static_cast<uint8_t>(length));

    if (sent) {
        Serial.printf("[%6lu] sent  seq=%-4lu %d bytes in %lu ms\n",
                      static_cast<unsigned long>(millis() / 1000),
                      static_cast<unsigned long>(sequence), length,
                      static_cast<unsigned long>(millis() - startedAt));
    } else {
        // TxDone never arrived. Not a lost packet -- a radio that did not
        // transmit at all.
        Serial.printf("[%6lu] FAILED seq=%lu -- no TxDone\n",
                      static_cast<unsigned long>(millis() / 1000),
                      static_cast<unsigned long>(sequence));
    }

    delay(kBeaconIntervalMs);
}

#else  // receiver

void loop() {
    // A silent receiver and a dead receiver look identical, which is useless
    // when the other radio is a mile away and you are deciding whether to walk
    // back. Say something periodically, and make it the noise floor so the
    // line is worth reading.
    static uint32_t nextHeartbeat = 0;
    if (static_cast<int32_t>(millis() - nextHeartbeat) >= 0) {
        nextHeartbeat = millis() + 30000;
        Serial.printf("[%6lu] idle   noise floor %d dBm  received %lu  CRC errors %lu\n",
                      static_cast<unsigned long>(millis() / 1000),
                      radio.rssiNowDbm(),
                      static_cast<unsigned long>(received),
                      static_cast<unsigned long>(radio.crcErrors()));
    }

    SX1276::Packet packet;
    if (!radio.poll(&packet)) {
        return;
    }

    ++received;

    // Beacons carry "seq=N". Parsing it lets the receiver report loss; anything
    // else is still printed, just without loss accounting.
    char text[256];
    const uint8_t n = min<uint8_t>(packet.length, sizeof(text) - 1);
    memcpy(text, packet.data, n);
    text[n] = '\0';

    const char* seqField = strstr(text, "seq=");
    if (seqField != nullptr) {
        const uint32_t sequence = strtoul(seqField + 4, nullptr, 10);
        if (haveSequence && sequence > lastSequence + 1) {
            missed += sequence - lastSequence - 1;
        }
        lastSequence = sequence;
        haveSequence = true;
    }

    Serial.printf("[%6lu] rx %3u bytes  RSSI %4d dBm  SNR %5.1f dB  | %s\n",
                  static_cast<unsigned long>(millis() / 1000),
                  packet.length, packet.rssiDbm, packet.snrDb, text);

    if (haveSequence) {
        const uint32_t expected = received + missed;
        Serial.printf("           received %lu, missed %lu (%.1f%% loss), CRC errors %lu\n",
                      static_cast<unsigned long>(received),
                      static_cast<unsigned long>(missed),
                      expected ? (100.0 * missed / expected) : 0.0,
                      static_cast<unsigned long>(radio.crcErrors()));
    }
}

#endif

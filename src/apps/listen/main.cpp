// Listen, and echo everything to the serial port. Nothing else.
//
// No WiFi, no HTTP, no packet framing, no loss accounting. If you want those,
// src/apps/gateway adds a network and a web page and src/apps/link adds
// sequence numbers and loss statistics. This is the one to reach for when you
// want to see what is arriving and nothing is allowed to get in the way.
//
// Safe with no antenna attached: it never transmits.

#include <Arduino.h>

#include "board.h"
#include "sx1276.h"

namespace {

SX1276 radio;

uint32_t received = 0;

// How far up the reception ladder anything got. Without these, "0 received" is
// ambiguous between "nothing is transmitting on our settings" and "something
// is, and we are failing to decode it" -- which point at completely different
// people's problems.
uint32_t sawPreamble = 0;   // RegModemStat bit 0
uint32_t sawSync     = 0;   // bit 1
uint32_t sawHeader   = 0;   // bit 3

void sampleModemStatus() {
    const uint8_t status = radio.modemStatus();
    if (status & 0x01) ++sawPreamble;
    if (status & 0x02) ++sawSync;
    if (status & 0x08) ++sawHeader;
}

void printLadder() {
    Serial.printf("           detect %lu / sync %lu / header %lu  "
                  "-> %lu decoded, %lu CRC fail, %lu unverified\n",
                  static_cast<unsigned long>(sawPreamble),
                  static_cast<unsigned long>(sawSync),
                  static_cast<unsigned long>(sawHeader),
                  static_cast<unsigned long>(received),
                  static_cast<unsigned long>(radio.crcErrors()),
                  static_cast<unsigned long>(radio.uncheckedPackets()));
    if (sawPreamble == 0) {
        Serial.println("           nothing LoRa-shaped is arriving on these "
                       "settings at all.");
    } else if (received == 0) {
        Serial.println("           preambles ARE arriving but nothing decodes "
                       "-- check the far end's header mode and CRC.");
    }
}

// Lab mode: -48 dB of front-end attenuation with AGC off, for when the other
// radio is on the same bench. Toggled at RUNTIME rather than by a build flag,
// because on this board every reflash risks a GPIO12 boot-loop needing a
// physical power cycle -- see CLAUDE.md. Changing a register beats that.
// 0 = normal (max gain, AGC on); 1..5 = increasing front-end attenuation.
// Stepped rather than on/off because -48 dB proved to be too much: with a
// beacon on the same bench it put SNR at -6.8 dB, which is 0.7 dB above SF7's
// -7.5 dB demodulation floor. Useful measurement wants the middle of the
// range, not the edge of it.
uint8_t attenStep = 0;

constexpr SX1276::LnaGain kSteps[] = {
    SX1276::LnaGain::Max,       // 0 dB
    SX1276::LnaGain::Minus6,
    SX1276::LnaGain::Minus12,
    SX1276::LnaGain::Minus24,
    SX1276::LnaGain::Minus36,
    SX1276::LnaGain::Minus48,
};
constexpr int kStepDb[] = {0, -6, -12, -24, -36, -48};

void applyGain() {
    if (attenStep == 0) {
        radio.setLnaGain(SX1276::LnaGain::Max, /*boost=*/true);
        radio.setAgc(true);      // hand gain control back to the radio
    } else {
        radio.setLnaGain(kSteps[attenStep], /*boost=*/false);
    }
    radio.setTxPower(attenStep ? board::kLabTxPowerDbm : board::kDefaultTxPowerDbm);
    radio.listen();
}

void printMode() {
    if (attenStep == 0) {
        Serial.println("mode       NORMAL -- LNA max, AGC on, TX 17 dBm");
    } else {
        Serial.printf("mode       LAB %d dB -- AGC off, TX 2 dBm\n",
                      kStepDb[attenStep]);
    }
    Serial.println("           n = normal, 1..5 = -6/-12/-24/-36/-48 dB, ? = status");
    Serial.println("           aim for SNR near 0 dB: saturated above about +8,");
    Serial.println("           and SF7 stops decoding below -7.5.");
}

// Heard nothing for this long? Say so, with the noise floor. A silent
// receiver and a dead one look identical otherwise.
constexpr uint32_t kHeartbeatMs = 30000;

// The Pro RF's frame, from its docs/radio-settings-reply.md and confirmed
// byte-for-byte against captured traffic:
//
//   0-3    RadioHead header: to, from, id, flags -- added by RH_RF95::send(),
//          not part of their protocol
//   4      magic 0x4A ('J')
//   5      version 0x01
//   6      type
//   7      nodeId
//   8-11   seq, uint32 little-endian
//   12-    ASCII record, NUL-terminated: "$PJPT,...*HH"
//
// Decoding it turns "we heard something" into a loss rate, because the
// sequence number is right there.
constexpr uint8_t kFrameOverhead = 12;
constexpr uint8_t kMagic   = 0x4A;
constexpr uint8_t kVersion = 0x01;

uint32_t lastSeq = 0;
bool     haveSeq = false;
uint32_t missed  = 0;

bool decodeFrame(const SX1276::Packet& packet) {
    if (packet.length <= kFrameOverhead ||
        packet.data[4] != kMagic || packet.data[5] != kVersion) {
        return false;
    }
    uint32_t seq;
    memcpy(&seq, &packet.data[8], sizeof(seq));   // little-endian both ends

    if (haveSeq && seq > lastSeq + 1) {
        missed += seq - lastSeq - 1;
    }
    lastSeq = seq;
    haveSeq = true;

    // The sender NUL-terminates the record, but a corrupt packet might not.
    char record[256];
    const uint8_t n = min<uint8_t>(packet.length - kFrameOverhead,
                                   sizeof(record) - 1);
    memcpy(record, &packet.data[kFrameOverhead], n);
    record[n] = '\0';

    Serial.printf("           seq %-6lu node 0x%02X type %u   %s\n",
                  static_cast<unsigned long>(seq), packet.data[7],
                  packet.data[6], record);
    const uint32_t expected = received + missed;
    Serial.printf("           %lu received, %lu missed (%.1f%% loss)\n",
                  static_cast<unsigned long>(received),
                  static_cast<unsigned long>(missed),
                  expected ? (100.0 * missed / expected) : 0.0);
    return true;
}

void printPacket(const SX1276::Packet& packet) {
    Serial.printf("\n[%8.3f] %u bytes   RSSI %d dBm   SNR %.1f dB%s\n",
                  millis() / 1000.0, packet.length, packet.rssiDbm, packet.snrDb,
                  packet.crcPresent ? "" : "   *** NO CRC -- UNVERIFIED ***");

    // Below the demodulation floor for this spreading factor, a "packet" is
    // almost certainly noise that decoded into a plausible header. SF7 is
    // about -7.5 dB; every SF buys roughly 2.5 dB more.
    if (!packet.crcPresent && packet.snrDb < -7.5) {
        Serial.println("           (SNR below the SF7 limit and no CRC -- "
                       "this is noise, not a transmission)");
    }

    // A recognised frame prints as fields; anything else falls back to the raw
    // dump, because an unknown transmitter is exactly when you want bytes.
    if (decodeFrame(packet)) {
        return;
    }

    // Both views, always. Text is unreadable when the payload is binary or
    // encrypted, and hex is unreadable when it is a string -- and you rarely
    // know which you have until you look.
    Serial.print("           hex  ");
    for (uint8_t i = 0; i < packet.length; ++i) {
        Serial.printf("%02X ", packet.data[i]);
        if ((i & 0x0F) == 0x0F && i + 1 < packet.length) {
            Serial.print("\n                ");
        }
    }
    Serial.println();

    Serial.print("           text ");
    for (uint8_t i = 0; i < packet.length; ++i) {
        const char c = static_cast<char>(packet.data[i]);
        Serial.print((c >= 32 && c < 127) ? c : '.');
    }
    Serial.println();
}

}  // namespace

void setup() {
    Serial.begin(115200);
    delay(1500);

    Serial.println("\n=== LoRa listener ===");

    const SX1276::Config config = {
        .frequencyHz     = board::kDefaultFrequencyHz,
        .spreadingFactor = board::kDefaultSpreadingFactor,
        .bandwidthHz     = board::kDefaultBandwidthHz,
        .codingRate4     = board::kDefaultCodingRate4,
        .txPowerDbm      = board::kDefaultTxPowerDbm,   // unused; never transmits
        .syncWord        = board::kDefaultSyncWord,
    };

    if (!radio.begin(config)) {
        Serial.println("RADIO NOT FOUND.");
        Serial.printf("  RegVersion read 0x%02X, expected 0x12\n",
                      radio.readRegister(0x42));
        Serial.println("  Check lib/board/board.h. This board is");
        Serial.println("  SCK 14 / MISO 12 / MOSI 13 / NSS 16 -- not the VSPI");
        Serial.println("  defaults every stock ESP32 target assumes.");
        return;
    }

    // Print all four, because all four must match at the far end for anything
    // to arrive. A mismatch in any one gives silence with no error either side.
    Serial.printf("frequency  %.3f MHz\n", board::kDefaultFrequencyHz / 1e6);
    Serial.printf("spreading  SF%u\n", board::kDefaultSpreadingFactor);
    Serial.printf("bandwidth  %lu kHz\n",
                  static_cast<unsigned long>(board::kDefaultBandwidthHz / 1000));
    Serial.printf("sync word  0x%02X\n", board::kDefaultSyncWord);

    printMode();
    applyGain();
    delay(50);   // let the front end settle before the first RSSI read
    // The far end lost a day to transmitting into an unconnected RFO pin with
    // nothing reporting an error. Cheap to confirm we have not done the same.
    Serial.printf("PA path    %s\n",
                  radio.paBoostConfirmed()
                      ? "PA_BOOST confirmed (RegPaConfig bit 7 set)"
                      : "*** RFO -- NOT WIRED ON THIS BOARD ***");
    Serial.printf("noise floor %d dBm\n\nlistening\n",
                  radio.rssiNowDbm());
}

void loop() {
    // Single keys rather than a line-based console: this app is meant to stay
    // out of the way, and one keystroke cannot be half-typed.
    while (Serial.available()) {
        const char key = static_cast<char>(Serial.read());
        switch (key) {
            case 'l': case 'L':      // kept: the old on/off shortcut
            case '1': case '2': case '3': case '4': case '5':
                attenStep = (key == 'l' || key == 'L') ? 5 : (key - '0');
                Serial.println(); printMode(); applyGain();
                delay(50);
                Serial.printf("floor now %d dBm\n", radio.rssiNowDbm());
                break;
            case 'n': case 'N': case '0':
                attenStep = 0;   Serial.println(); printMode(); applyGain();
                delay(50);
                Serial.printf("floor now %d dBm\n", radio.rssiNowDbm());
                break;
            case '?':
                Serial.println(); printMode();
                Serial.printf("noise floor %d dBm, %lu received, %lu CRC errors, "
                              "%lu unverified\n",
                              radio.rssiNowDbm(),
                              static_cast<unsigned long>(received),
                              static_cast<unsigned long>(radio.crcErrors()),
                              static_cast<unsigned long>(radio.uncheckedPackets()));
                printLadder();
                break;
            default: break;   // ignore stray newlines from the terminal
        }
    }

    sampleModemStatus();

    SX1276::Packet packet;
    if (radio.poll(&packet)) {
        ++received;
        printPacket(packet);
        return;
    }

    static uint32_t nextHeartbeat = kHeartbeatMs;
    if (static_cast<int32_t>(millis() - nextHeartbeat) >= 0) {
        nextHeartbeat = millis() + kHeartbeatMs;
        Serial.printf("[%8.3f] idle [%s] -- noise floor %d dBm, %lu received, "
                      "%lu CRC errors, %lu unverified\n",
                      millis() / 1000.0, attenStep ? "LAB" : "normal", radio.rssiNowDbm(),
                      static_cast<unsigned long>(received),
                      static_cast<unsigned long>(radio.crcErrors()),
                      static_cast<unsigned long>(radio.uncheckedPackets()));
        printLadder();
    }
}

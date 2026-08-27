// A small SX1276 LoRa driver, written against this board's confirmed wiring.
//
// Deliberately not RadioLib or arduino-LMIC. Those are better libraries, but
// both want a board definition, and supplying one for hardware that matches no
// stock target is where this project's predecessors went wrong. A few hundred
// lines with the pins passed in explicitly cannot silently pick VSPI.
//
// Scope is a link test: configure, send a packet, receive a packet, report
// RSSI and SNR. No LoRaWAN, no MAC layer, no interrupts -- polling is enough
// at these data rates and leaves nothing hidden.

#pragma once

#include <Arduino.h>
#include <SPI.h>

class SX1276 {
 public:
    // RegLna bits 7-5. Datasheet table at 0x0C: G1 is maximum gain, and each
    // step attenuates the receiver front end. This is the receive-side
    // equivalent of turning transmit power down, and the only lever that helps
    // when the problem is somebody ELSE transmitting too hard at you.
    enum class LnaGain : uint8_t {
        Max      = 1,   //   0 dB
        Minus6   = 2,
        Minus12  = 3,
        Minus24  = 4,
        Minus36  = 5,
        Minus48  = 6,   // -48 dB, the floor
    };
    struct Config {
        uint32_t frequencyHz;
        uint8_t  spreadingFactor;   // 6..12
        uint32_t bandwidthHz;
        uint8_t  codingRate4;       // 5..8, meaning 4/5 .. 4/8
        uint8_t  txPowerDbm;        // 2..17 on PA_BOOST
        uint8_t  syncWord;
    };

    struct Packet {
        uint8_t  data[255];
        uint8_t  length;
        int16_t  rssiDbm;
        float    snrDb;
        // False when the received header said the packet carries no CRC, so
        // nothing verified these bytes. Noise regularly decodes into a
        // plausible header with CRC off, and reporting that as data is how a
        // receiver invents traffic that was never sent.
        bool     crcPresent;
    };

    // Returns false if the radio does not answer with the SX1276 version byte.
    // That is the failure worth catching loudly: a wrong pin map gets you here
    // with 0x00 or 0xFF rather than an error from anything lower down.
    bool begin(const Config& config);

    // Blocking. Returns false on timeout, which in practice means the radio
    // never asserted TxDone -- a stuck SPI bus or a radio left in the wrong
    // mode.
    //
    // TRANSMITTING WITHOUT AN ANTENNA CAN DAMAGE THE PA. The RFM95HW is the
    // +20 dBm build and its PA_BOOST output has nowhere to dump the power.
    bool transmit(const uint8_t* data, uint8_t length, uint32_t timeoutMs = 5000);

    // Change frequency and modulation on an already-initialised radio, without
    // re-checking the version byte. This is what makes a scanner possible: a
    // LoRa receiver only hears traffic whose spreading factor, bandwidth,
    // frequency AND sync word all match, so seeing what is on the air means
    // retuning repeatedly rather than listening once.
    void reconfigure(const Config& config);

    // Puts the radio in continuous receive and returns immediately.
    void listen();

    // Non-blocking. Returns true when a CRC-valid packet was collected.
    // Packets failing CRC are dropped and counted rather than returned: a
    // corrupt payload reported as data is worse than a lost one.
    bool poll(Packet* out);

    uint32_t crcErrors() const { return crcErrors_; }

    // Packets whose header declared no CRC. Counted rather than hidden: a
    // transmitter that genuinely sends without CRC is legitimate, but so is
    // noise, and only you can tell which you are looking at.
    uint32_t uncheckedPackets() const { return unchecked_; }

    // False means PaSelect did not stick and transmissions would go to the
    // unconnected RFO pin. Silent in every other way -- see setTxPower().
    bool paBoostConfirmed() const { return paBoostConfirmed_; }

    // Current wideband RSSI, for a noise-floor reading with no packet present.
    int16_t rssiNowDbm();

    // Set the front-end gain by hand.
    //
    // Automatically disables AGC, because it has to: with AgcAutoOn set the
    // radio chooses the LNA gain itself and silently ignores RegLna, so a
    // manual setting appears to do nothing at all.
    void setLnaGain(LnaGain gain, bool boost);

    // Hand gain control back to the radio.
    void setAgc(bool enabled);

    // 2..17 dBm on PA_BOOST. Public so an application can drop power without
    // rebuilding -- see lab mode.
    void setTxPower(uint8_t dbm);

    // RegModemStat (0x18). The reception ladder, and the only way to tell
    // "nothing is arriving" from "something is arriving and not decoding":
    //   bit 0  signal detected     -- a valid LoRa preamble is present
    //   bit 1  signal synchronized -- preamble ended, modem locked
    //   bit 2  RX on-going
    //   bit 3  header info valid   -- header CRC passed
    //   bit 4  modem clear
    // Bits are transient; sample often.
    uint8_t modemStatus();

    uint8_t readRegister(uint8_t address);
    void    writeRegister(uint8_t address, uint8_t value);

 private:
    void setMode(uint8_t mode);
    void setFrequency(uint32_t hz);
    void setModulation(const Config& config);

    SPIClass spi_{HSPI};
    Config   config_{};
    uint32_t crcErrors_ = 0;
    uint32_t unchecked_ = 0;
    bool     paBoostConfirmed_ = false;
};

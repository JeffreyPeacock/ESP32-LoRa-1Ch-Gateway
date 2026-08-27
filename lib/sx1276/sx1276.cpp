#include "sx1276.h"

#include <math.h>

#include "board.h"

namespace {

// SX1276 register map, LoRa mode. Numbers from the Semtech SX1276/77/78/79
// datasheet rev V7, mirrored in docs/datasheets/.
constexpr uint8_t kRegFifo             = 0x00;
constexpr uint8_t kRegOpMode           = 0x01;
constexpr uint8_t kRegFrfMsb           = 0x06;
constexpr uint8_t kRegPaConfig         = 0x09;
constexpr uint8_t kRegOcp              = 0x0B;
constexpr uint8_t kRegLna              = 0x0C;
constexpr uint8_t kRegFifoAddrPtr      = 0x0D;
constexpr uint8_t kRegFifoTxBaseAddr   = 0x0E;
constexpr uint8_t kRegFifoRxBaseAddr   = 0x0F;
constexpr uint8_t kRegFifoRxCurrentAddr= 0x10;
constexpr uint8_t kRegIrqFlags         = 0x12;
constexpr uint8_t kRegRxNbBytes        = 0x13;
constexpr uint8_t kRegHopChannel       = 0x1C;
constexpr uint8_t kRegModemStat        = 0x18;
constexpr uint8_t kRegPktSnrValue      = 0x19;
constexpr uint8_t kRegPktRssiValue     = 0x1A;
constexpr uint8_t kRegRssiValue        = 0x1B;
constexpr uint8_t kRegModemConfig1     = 0x1D;
constexpr uint8_t kRegModemConfig2     = 0x1E;
constexpr uint8_t kRegPreambleMsb      = 0x20;
constexpr uint8_t kRegPayloadLength    = 0x22;
constexpr uint8_t kRegModemConfig3     = 0x26;
constexpr uint8_t kRegDetectOptimize   = 0x31;
constexpr uint8_t kRegDetectionThresh  = 0x37;
constexpr uint8_t kRegSyncWord         = 0x39;
constexpr uint8_t kRegVersion          = 0x42;
constexpr uint8_t kRegPaDac            = 0x4D;

constexpr uint8_t kModeLongRange = 0x80;
constexpr uint8_t kModeSleep     = 0x00;
constexpr uint8_t kModeStandby   = 0x01;
constexpr uint8_t kModeTx        = 0x03;
constexpr uint8_t kModeRxCont    = 0x05;

constexpr uint8_t kIrqTxDone         = 0x08;
constexpr uint8_t kIrqPayloadCrcErr  = 0x20;
constexpr uint8_t kIrqRxDone         = 0x40;

// RegHopChannel bit 6: did the header of the packet just received declare a
// CRC? In explicit-header mode this is the TRANSMITTER's choice, not ours --
// our RxPayloadCrcOn setting only governs what we send. With no CRC declared,
// PayloadCrcError can never assert, so a corrupt packet looks perfect.
constexpr uint8_t kCrcOnPayload      = 0x40;

constexpr uint8_t kSx1276Version = 0x12;

// The RSSI registers report a value relative to a fixed offset. -157 is the
// high-frequency port figure, which is the one that applies at 915 MHz; the
// low-frequency port would be -164.
constexpr int16_t kRssiOffsetHf = -157;

// RegModemConfig1 encodes bandwidth as an index, not a number.
uint8_t bandwidthIndex(uint32_t hz) {
    if (hz <= 7800)   return 0;
    if (hz <= 10400)  return 1;
    if (hz <= 15600)  return 2;
    if (hz <= 20800)  return 3;
    if (hz <= 31250)  return 4;
    if (hz <= 41700)  return 5;
    if (hz <= 62500)  return 6;
    if (hz <= 125000) return 7;
    if (hz <= 250000) return 8;
    return 9;  // 500 kHz
}

}  // namespace

uint8_t SX1276::readRegister(uint8_t address) {
    spi_.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
    digitalWrite(board::kPinNss, LOW);
    spi_.transfer(address & 0x7F);          // bit 7 clear = read
    uint8_t value = spi_.transfer(0x00);
    digitalWrite(board::kPinNss, HIGH);
    spi_.endTransaction();
    return value;
}

void SX1276::writeRegister(uint8_t address, uint8_t value) {
    spi_.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
    digitalWrite(board::kPinNss, LOW);
    spi_.transfer(address | 0x80);          // bit 7 set = write
    spi_.transfer(value);
    digitalWrite(board::kPinNss, HIGH);
    spi_.endTransaction();
}

void SX1276::setMode(uint8_t mode) {
    writeRegister(kRegOpMode, kModeLongRange | mode);
}

void SX1276::setFrequency(uint32_t hz) {
    // Frf = f * 2^19 / F_XOSC, with a 32 MHz crystal. Done in 64-bit because
    // 915e6 * 2^19 overflows 32 bits by a wide margin.
    uint64_t frf = (static_cast<uint64_t>(hz) << 19) / 32000000ULL;
    writeRegister(kRegFrfMsb + 0, static_cast<uint8_t>(frf >> 16));
    writeRegister(kRegFrfMsb + 1, static_cast<uint8_t>(frf >> 8));
    writeRegister(kRegFrfMsb + 2, static_cast<uint8_t>(frf >> 0));
}

void SX1276::setModulation(const Config& config) {
    const uint8_t bw = bandwidthIndex(config.bandwidthHz);
    const uint8_t cr = config.codingRate4 - 4;      // 5..8 -> 1..4

    // Explicit header: the receiver learns the length from the packet, which
    // is what we want for variable-length text.
    writeRegister(kRegModemConfig1, (bw << 4) | (cr << 1) | 0x00);

    // SF in the high nibble; bit 2 turns on payload CRC so poll() can reject
    // corrupt packets rather than hand them over.
    writeRegister(kRegModemConfig2, (config.spreadingFactor << 4) | 0x04);

    // Low-data-rate optimisation is mandatory when a symbol exceeds 16 ms,
    // which happens at SF11/SF12 on 125 kHz. Getting this wrong costs you
    // packets at exactly the settings chosen for maximum range.
    const uint32_t symbolMs = (1UL << config.spreadingFactor) * 1000UL / config.bandwidthHz;
    const uint8_t  ldro = (symbolMs >= 16) ? 0x08 : 0x00;
    writeRegister(kRegModemConfig3, ldro | 0x04);   // 0x04 = AGC auto on

    // SF6 is a special case: implicit header only, plus different detection
    // constants. Support it because it is the fastest setting, but it has to
    // be set up differently.
    if (config.spreadingFactor == 6) {
        writeRegister(kRegDetectOptimize, 0xC5);
        writeRegister(kRegDetectionThresh, 0x0C);
    } else {
        writeRegister(kRegDetectOptimize, 0xC3);
        writeRegister(kRegDetectionThresh, 0x0A);
    }

    writeRegister(kRegPreambleMsb + 0, 0x00);
    writeRegister(kRegPreambleMsb + 1, 0x08);       // 8 symbols, the default
    writeRegister(kRegSyncWord, config.syncWord);
}

void SX1276::setTxPower(uint8_t dbm) {
    // The RFM95HW wires PA_BOOST only; RFO is not connected, so a request for
    // low power still has to go out through PA_BOOST.
    if (dbm < 2)  dbm = 2;
    if (dbm > 17) dbm = 17;

    writeRegister(kRegPaDac, 0x84);                 // 0x87 would allow +20 dBm
    writeRegister(kRegPaConfig, 0x80 | (dbm - 2));  // 0x80 selects PA_BOOST
    writeRegister(kRegOcp, 0x20 | 0x0B);            // OCP on, ~100 mA

    // Read PaSelect back rather than trusting the write. The RFM95's RFO pin
    // is not connected, so selecting it transmits into nothing -- and nothing
    // reports an error: the radio enters TX, send() succeeds, and the console
    // says "sent". The Pro RF at the other end of this link lost a day to
    // exactly that in 2026-08 via RadioHead's setTxPower(power, useRFO=true).
    paBoostConfirmed_ = (readRegister(kRegPaConfig) & 0x80) != 0;
}

bool SX1276::begin(const Config& config) {
    config_ = config;

    pinMode(board::kPinNss, OUTPUT);
    digitalWrite(board::kPinNss, HIGH);
    spi_.begin(board::kPinSck, board::kPinMiso, board::kPinMosi, board::kPinNss);

    // No reset pin exists on this board (see board.h), so the radio can only be
    // brought to a known state through OpMode. Sleep is required before the
    // LongRangeMode bit will take, because it is only writable in sleep.
    setMode(kModeSleep);
    delay(10);

    if (readRegister(kRegVersion) != kSx1276Version) {
        return false;
    }

    setFrequency(config.frequencyHz);
    setModulation(config);
    setTxPower(config.txPowerDbm);

    writeRegister(kRegLna, 0x23);                   // max gain, boost on
    writeRegister(kRegFifoTxBaseAddr, 0x00);
    writeRegister(kRegFifoRxBaseAddr, 0x00);

    setMode(kModeStandby);
    delay(10);
    return true;
}

void SX1276::setLnaGain(LnaGain gain, bool boost) {
    // AGC overrides RegLna. Clearing AgcAutoOn first is not optional -- leave
    // it set and this function is a no-op with no error.
    setAgc(false);
    const uint8_t value = (static_cast<uint8_t>(gain) << 5) | (boost ? 0x03 : 0x00);
    writeRegister(kRegLna, value);
}

void SX1276::setAgc(bool enabled) {
    const uint8_t current = readRegister(kRegModemConfig3);
    writeRegister(kRegModemConfig3,
                  enabled ? (current | 0x04) : (current & ~0x04));
}

void SX1276::reconfigure(const Config& config) {
    config_ = config;
    setMode(kModeStandby);
    delay(2);
    setFrequency(config.frequencyHz);
    setModulation(config);
    setTxPower(config.txPowerDbm);
}

bool SX1276::transmit(const uint8_t* data, uint8_t length, uint32_t timeoutMs) {
    setMode(kModeStandby);

    writeRegister(kRegFifoAddrPtr, 0x00);
    writeRegister(kRegPayloadLength, length);

    spi_.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
    digitalWrite(board::kPinNss, LOW);
    spi_.transfer(kRegFifo | 0x80);
    for (uint8_t i = 0; i < length; ++i) {
        spi_.transfer(data[i]);
    }
    digitalWrite(board::kPinNss, HIGH);
    spi_.endTransaction();

    writeRegister(kRegIrqFlags, 0xFF);              // write-1-to-clear
    setMode(kModeTx);

    const uint32_t deadline = millis() + timeoutMs;
    while (!(readRegister(kRegIrqFlags) & kIrqTxDone)) {
        if (static_cast<int32_t>(millis() - deadline) >= 0) {
            setMode(kModeStandby);
            return false;
        }
        delay(1);
    }

    writeRegister(kRegIrqFlags, kIrqTxDone);
    setMode(kModeStandby);
    return true;
}

void SX1276::listen() {
    writeRegister(kRegIrqFlags, 0xFF);
    writeRegister(kRegFifoAddrPtr, 0x00);
    setMode(kModeRxCont);
}

bool SX1276::poll(Packet* out) {
    const uint8_t irq = readRegister(kRegIrqFlags);
    if (!(irq & kIrqRxDone)) {
        return false;
    }
    writeRegister(kRegIrqFlags, irq);               // clear what we saw

    if (irq & kIrqPayloadCrcErr) {
        ++crcErrors_;
        return false;
    }

    const bool crcPresent = (readRegister(kRegHopChannel) & kCrcOnPayload) != 0;
    if (!crcPresent) {
        ++unchecked_;
    }

    const uint8_t length = readRegister(kRegRxNbBytes);
    writeRegister(kRegFifoAddrPtr, readRegister(kRegFifoRxCurrentAddr));

    spi_.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
    digitalWrite(board::kPinNss, LOW);
    spi_.transfer(kRegFifo & 0x7F);
    for (uint8_t i = 0; i < length; ++i) {
        out->data[i] = spi_.transfer(0x00);
    }
    digitalWrite(board::kPinNss, HIGH);
    spi_.endTransaction();

    out->length = length;
    out->crcPresent = crcPresent;

    // SNR is a signed value in quarter-dB steps and is needed to correct RSSI:
    // below the noise floor the packet RSSI reading alone understates the
    // signal, which is exactly the regime a range test lives in.
    const int8_t rawSnr = static_cast<int8_t>(readRegister(kRegPktSnrValue));
    out->snrDb = rawSnr / 4.0f;

    // Datasheet 5.5.5:
    //   SNR <  0 : Packet Strength = -157 + PacketRssi + PacketSnr * 0.25
    //   SNR >= 0 : RSSI            = -157 + PacketRssi
    // The correction term is the SNR in dB, so use the float and round --
    // truncating to int here quietly lost up to a dB in exactly the
    // below-the-noise-floor regime the correction exists for.
    const int16_t rawRssi = readRegister(kRegPktRssiValue);
    const float   corrected = (out->snrDb < 0.0f)
        ? kRssiOffsetHf + rawRssi + out->snrDb
        : static_cast<float>(kRssiOffsetHf + rawRssi);
    out->rssiDbm = static_cast<int16_t>(lroundf(corrected));

    return true;
}

uint8_t SX1276::modemStatus() {
    return readRegister(kRegModemStat);
}

int16_t SX1276::rssiNowDbm() {
    return kRssiOffsetHf + readRegister(kRegRssiValue);
}

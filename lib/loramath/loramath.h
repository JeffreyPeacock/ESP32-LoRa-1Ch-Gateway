// LoRa parameter arithmetic, with no Arduino dependency.
//
// Split out from the driver so it can be tested on the host. These are the
// calculations that are wrong silently: a bad Frf tunes you to the wrong
// frequency and everything still "works", and a missing low-data-rate flag
// costs packets only at the settings chosen for maximum range. Neither
// produces an error message, so both deserve tests that run without hardware.

#pragma once

#include <stdint.h>

namespace loramath {

// RegModemConfig1 encodes bandwidth as an index, not a value in hertz.
inline uint8_t bandwidthIndex(uint32_t hz) {
    if (hz <= 7800)   return 0;
    if (hz <= 10400)  return 1;
    if (hz <= 15600)  return 2;
    if (hz <= 20800)  return 3;
    if (hz <= 31250)  return 4;
    if (hz <= 41700)  return 5;
    if (hz <= 62500)  return 6;
    if (hz <= 125000) return 7;
    if (hz <= 250000) return 8;
    return 9;                       // 500 kHz
}

// Frf = f * 2^19 / F_XOSC, with the SX1276's 32 MHz crystal.
//
// Must be done in 64-bit: 915e6 << 19 is about 4.8e14 and overflows a uint32_t
// by six orders of magnitude. In 32-bit arithmetic this silently tunes the
// radio somewhere else entirely.
inline uint32_t frfRegister(uint32_t hz) {
    return static_cast<uint32_t>((static_cast<uint64_t>(hz) << 19) / 32000000ULL);
}

// Symbol duration in microseconds: 2^SF / BW.
inline uint32_t symbolDurationUs(uint8_t spreadingFactor, uint32_t bandwidthHz) {
    return static_cast<uint32_t>((1ULL << spreadingFactor) * 1000000ULL / bandwidthHz);
}

// The datasheet mandates low-data-rate optimisation when a symbol lasts 16 ms
// or more, which happens at SF11 and SF12 on 125 kHz -- exactly the settings
// picked for maximum range. Omit it and the link fails where it matters most.
inline bool needsLowDataRateOptimize(uint8_t spreadingFactor, uint32_t bandwidthHz) {
    return symbolDurationUs(spreadingFactor, bandwidthHz) >= 16000;
}

// US915 uplink channel plan: 64 channels of 125 kHz from 902.3 MHz, 200 kHz
// apart. Worth having as code because SparkFun's own device sketch enables
// channel 17 while its gateway listens on channel 8, and the mismatch is
// invisible unless you do this arithmetic.
inline uint32_t us915UplinkHz(uint8_t channel) {
    return 902300000UL + 200000UL * static_cast<uint32_t>(channel);
}

}  // namespace loramath

// Board definition for the SparkFun LoRa Gateway 1-Channel (ESP32), WRL-15006.
//
// ONE definition of the pin map, because getting it wrong is this board's
// signature failure: every stock ESP32 + SX127x target wires the radio to VSPI
// with NSS on 18, and this board does not. A near-miss target initialises
// cleanly, reports no error, and hears nothing.
//
// These values are confirmed four ways -- the 2018 schematic, SparkFun's
// Arduino variant (sparkx_esp32_lora/pins_arduino.h), the gateway firmware's
// own comment block, and SparkFun's tutorial -- and then by the radio itself:
// src/probe reads RegVersion and gets 0x12 here, 0x00 on VSPI.

#pragma once

#include <Arduino.h>

namespace board {

// SPI. Note these are the HSPI-side pins, NOT the ESP32's VSPI defaults.
// GPIO 18/19/23 are broken out to the headers and are not wired to the radio.
constexpr int kPinSck  = 14;
constexpr int kPinMiso = 12;   // ALSO the flash-voltage strap -- see below
constexpr int kPinMosi = 13;
constexpr int kPinNss  = 16;

// MISO on GPIO12 is not just a SPI pin. GPIO12 is MTDI, the ESP32's VDD_SDIO
// strapping pin, and the XPD_SDIO_FORCE eFuse is not burned -- so the radio's
// MISO state at reset decides whether the flash rail comes up at 3.3 V or
// 1.8 V. High at reset breaks flash access: uploads fail with "Failed to
// communicate with the flash chip" while chip-id still works, because that
// never touches flash. The radio has no reset pin, so only removing power
// clears it. Unplug the USB and plug it back in. See CLAUDE.md.

// Radio interrupts. DIO0 is TxDone/RxDone. On this board the name DIO0 is
// correct -- unlike the sibling Heltec V3 project, where the variant header
// misnames it.
constexpr int kPinDio0 = 26;
constexpr int kPinDio1 = 33;
constexpr int kPinDio2 = 32;

// There is no usable reset pin. SparkFun publishes 5 (LMIC sketch) and 27
// (gateway tutorial, commented "Reset not used"); src/probe pulsed both against
// the live radio and neither resets it. Do not add one back without reading it
// off the PCB first.
constexpr int kPinRst = -1;

// The radio is a Hope RFM95HW per the schematic -- SX1276 silicon, and the
// high-power build, so PA_BOOST is the only output path. RFO is not wired.
constexpr bool kUsesPaBoost = true;

// 915.0 MHz, for raw point-to-point work with the other radios here.
//
// Note this is NOT a LoRaWAN channel -- see docs/band-plan.md. "915 MHz" is the
// name of the 902-928 MHz ISM band, and 915.0 falls between US915 channels 63
// and 64. It is simply a common convention for raw LoRa, and matching the other
// radios matters more than matching a plan we are not using.
//
// If this board ever speaks LoRaWAN again, the right value is 903900000 --
// uplink channel 8, the first channel of sub-band 2, which is what US networks
// use and what the stock 2019 firmware was configured for. It is channel 8, not
// channel 0; this project's notes said channel 0 for a while and were wrong.
constexpr uint32_t kDefaultFrequencyHz = 915000000UL;

// Conservative defaults: SF7/BW125/CR4-5 is the fastest US915 setting and what
// the stock gateway used.
constexpr uint8_t  kDefaultSpreadingFactor = 7;
constexpr uint32_t kDefaultBandwidthHz     = 125000UL;
constexpr uint8_t  kDefaultCodingRate4     = 5;   // 4/5
constexpr uint8_t  kDefaultTxPowerDbm      = 17;  // PA_BOOST, below the 20 dBm
                                                  // mode that needs PaDac
constexpr uint8_t  kDefaultSyncWord        = 0x12; // private LoRa, not LoRaWAN

// "Lab mode" -- for radios on the same bench.
//
// At one metre with full antennas, 17 dBm puts roughly -10 dBm into a receiver
// whose sensitivity is about -123 dBm. That is ~112 dB of excess: the SNR
// estimate saturates and stops carrying information, and the front end can
// throw CRC errors that look like a bad link. See docs/measuring-snr.md.
//
// Two independent levers, because they solve different halves. Transmit power
// only helps when WE transmit; LNA attenuation is what helps when the problem
// is somebody else transmitting too hard at us -- which is the usual case here,
// since this board is normally the listener.
constexpr uint8_t  kLabTxPowerDbm = 2;    // the SX1276's minimum on PA_BOOST
// -48 dB of front-end attenuation, roughly equivalent to a good coaxial
// attenuator without owning one.
constexpr uint8_t  kLabLnaGainStep = 6;   // LnaGain::Minus48

}  // namespace board

// WiFi credentials in NVS, entered over serial.
//
// Deliberately NOT compiled in. The stock 2019 firmware put its SSID and
// password in a `wpa[]` table in a header, which is how this board ended up
// with a MakerSpace's credentials baked into its flash and printed on every
// boot. This repository is public, so that approach is worse than inconvenient.
//
// NVS is its own partition (0x9000 in this board's table), and an application
// upload only writes 0x10000. Credentials stored here therefore survive every
// firmware update -- set them once and reflash freely.

#pragma once

#include <Arduino.h>

namespace netconfig {

// Longest values WPA2 allows, plus a terminator.
constexpr size_t kMaxSsid = 32 + 1;
constexpr size_t kMaxPass = 63 + 1;

struct Credentials {
    char ssid[kMaxSsid];
    char pass[kMaxPass];
    bool valid;         // an SSID is present; a blank password is legitimate
};

// Reads what is stored. `valid` is false when nothing has been saved yet.
Credentials load();

bool save(const char* ssid, const char* pass);
void erase();

// Feed one line of serial input. Returns true if it was a recognised command.
// Kept as a line handler rather than a blocking prompt so an application can
// keep doing its real work while remaining configurable.
//
// Commands: help, show, ssid <name>, pass <secret>, save, erase, connect
bool handleCommand(const String& line);

void printHelp();

// True when handleCommand() saw "connect", so the caller knows to retry.
bool connectRequested();
void clearConnectRequest();

}  // namespace netconfig

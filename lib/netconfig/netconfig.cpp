#include "netconfig.h"

#include <Preferences.h>

namespace netconfig {
namespace {

constexpr char kNamespace[] = "netcfg";
constexpr char kKeySsid[]   = "ssid";
constexpr char kKeyPass[]   = "pass";

// Staged edits. Kept separate from what is stored so a half-typed
// configuration cannot replace a working one -- you must "save" deliberately.
char pendingSsid[kMaxSsid] = {0};
char pendingPass[kMaxPass] = {0};
bool connectFlag = false;

// Never print a password back. The whole reason this exists is that the old
// firmware echoed its credentials to serial on every boot, and this board's
// serial output ends up in session logs.
void printMasked(const char* value) {
    if (value == nullptr || value[0] == '\0') {
        Serial.print("(unset)");
        return;
    }
    Serial.printf("(set, %u chars)", static_cast<unsigned>(strlen(value)));
}

}  // namespace

Credentials load() {
    Credentials out = {};
    Preferences prefs;
    if (!prefs.begin(kNamespace, /*readOnly=*/true)) {
        return out;   // namespace absent: nothing saved yet
    }
    prefs.getString(kKeySsid, out.ssid, sizeof(out.ssid));
    prefs.getString(kKeyPass, out.pass, sizeof(out.pass));
    prefs.end();
    out.valid = out.ssid[0] != '\0';
    return out;
}

bool save(const char* ssid, const char* pass) {
    if (ssid == nullptr || ssid[0] == '\0') {
        return false;
    }
    Preferences prefs;
    if (!prefs.begin(kNamespace, /*readOnly=*/false)) {
        return false;
    }
    prefs.putString(kKeySsid, ssid);
    prefs.putString(kKeyPass, pass != nullptr ? pass : "");
    prefs.end();
    return true;
}

void erase() {
    Preferences prefs;
    if (prefs.begin(kNamespace, /*readOnly=*/false)) {
        prefs.clear();
        prefs.end();
    }
    pendingSsid[0] = '\0';
    pendingPass[0] = '\0';
}

void printHelp() {
    Serial.println();
    Serial.println("  commands:");
    Serial.println("    show            what is stored (password is never printed)");
    Serial.println("    ssid <name>     stage an SSID");
    Serial.println("    pass <secret>   stage a password");
    Serial.println("    save            commit staged values to NVS");
    Serial.println("    connect         (re)connect using what is stored");
    Serial.println("    erase           forget the stored credentials");
    Serial.println("    help            this list");
    Serial.println();
    Serial.println("  e.g.  ssid HOME");
    Serial.println("        pass my-wifi-password");
    Serial.println("        save");
    Serial.println();
    Serial.println("  NVS is a separate partition, so these survive reflashing.");
    Serial.println();
}

bool handleCommand(const String& raw) {
    String line = raw;
    line.trim();
    if (line.isEmpty()) {
        return false;
    }

    // Split on the FIRST space only: passwords legitimately contain spaces,
    // and splitting on all of them would silently truncate one.
    const int space = line.indexOf(' ');
    const String verb = (space < 0) ? line : line.substring(0, space);
    const String rest = (space < 0) ? String() : line.substring(space + 1);

    if (verb.equalsIgnoreCase("help")) {
        printHelp();
        return true;
    }

    if (verb.equalsIgnoreCase("show")) {
        const Credentials stored = load();
        Serial.print("  stored ssid: ");
        Serial.println(stored.valid ? stored.ssid : "(unset)");
        Serial.print("  stored pass: ");
        printMasked(stored.pass);
        Serial.println();
        Serial.print("  staged ssid: ");
        Serial.println(pendingSsid[0] ? pendingSsid : "(none)");
        Serial.print("  staged pass: ");
        printMasked(pendingPass);
        Serial.println();
        return true;
    }

    if (verb.equalsIgnoreCase("ssid")) {
        if (rest.isEmpty()) {
            Serial.println("  usage: ssid <name>");
            return true;
        }
        strncpy(pendingSsid, rest.c_str(), sizeof(pendingSsid) - 1);
        pendingSsid[sizeof(pendingSsid) - 1] = '\0';
        Serial.printf("  staged ssid: %s\n", pendingSsid);
        return true;
    }

    if (verb.equalsIgnoreCase("pass")) {
        strncpy(pendingPass, rest.c_str(), sizeof(pendingPass) - 1);
        pendingPass[sizeof(pendingPass) - 1] = '\0';
        Serial.print("  staged pass: ");
        printMasked(pendingPass);
        Serial.println();
        return true;
    }

    if (verb.equalsIgnoreCase("save")) {
        // Fall back to the stored SSID so "pass ... / save" can fix only the
        // password without retyping the network name.
        const Credentials stored = load();
        const char* ssid = pendingSsid[0] ? pendingSsid
                                          : (stored.valid ? stored.ssid : nullptr);
        const char* pass = pendingPass[0] ? pendingPass : stored.pass;
        if (ssid == nullptr) {
            Serial.println("  nothing to save -- set an ssid first");
            return true;
        }
        Serial.println(save(ssid, pass) ? "  saved to NVS" : "  SAVE FAILED");
        connectFlag = true;
        return true;
    }

    if (verb.equalsIgnoreCase("erase")) {
        erase();
        Serial.println("  erased");
        return true;
    }

    if (verb.equalsIgnoreCase("connect")) {
        connectFlag = true;
        return true;
    }

    Serial.printf("  unknown command: %s (try 'help')\n", verb.c_str());
    return false;
}

bool connectRequested()      { return connectFlag; }
void clearConnectRequest()   { connectFlag = false; }

}  // namespace netconfig

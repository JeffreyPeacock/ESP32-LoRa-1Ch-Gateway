// Listening gateway: WiFi + HTTP + LoRa receive.
//
// Answers the two things you actually want from this board day to day --
// "is it on my network and can I reach it?" and "what is it hearing?" -- and
// is the seed of a real gateway if it ever reports to a network server.
//
// Credentials are NOT in this file. Type them once over serial and they live
// in NVS, which an application upload does not touch. See lib/netconfig.
//
// Received packets go to serial AND to a ring buffer served over HTTP, so you
// can watch it from the desk or from a browser without a cable.

#include <Arduino.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WiFi.h>

#include "board.h"
#include "netconfig.h"
#include "sx1276.h"

namespace {

constexpr char     kHostname[]     = "lora-gw";
constexpr size_t   kHistoryLength  = 32;
constexpr uint32_t kConnectTimeout = 20000;

SX1276    radio;
WebServer server(80);

struct Entry {
    uint32_t atMs;
    uint8_t  length;
    int16_t  rssiDbm;
    float    snrDb;
    char     text[64];
};

Entry    history[kHistoryLength];
size_t   historyCount = 0;      // total ever, so it doubles as a counter
bool     radioReady   = false;
bool     httpStarted  = false;
String   serialLine;

Entry& slotFor(size_t index) { return history[index % kHistoryLength]; }

void record(const SX1276::Packet& packet) {
    Entry& entry = slotFor(historyCount++);
    entry.atMs    = millis();
    entry.length  = packet.length;
    entry.rssiDbm = packet.rssiDbm;
    entry.snrDb   = packet.snrDb;

    // Payloads are often encrypted or binary; keep something printable either
    // way rather than dumping control characters into a web page.
    const uint8_t n = min<uint8_t>(packet.length, sizeof(entry.text) - 1);
    for (uint8_t i = 0; i < n; ++i) {
        const char c = static_cast<char>(packet.data[i]);
        entry.text[i] = (c >= 32 && c < 127) ? c : '.';
    }
    entry.text[n] = '\0';
}

String escapeHtml(const char* s) {
    String out;
    for (const char* p = s; *p; ++p) {
        switch (*p) {
            case '&': out += "&amp;";  break;
            case '<': out += "&lt;";   break;
            case '>': out += "&gt;";   break;
            case '"': out += "&quot;"; break;
            default:  out += *p;
        }
    }
    return out;
}

void handleRoot() {
    String page;
    page.reserve(4096);
    page += F("<!doctype html><meta charset=utf-8>"
              "<meta name=viewport content='width=device-width,initial-scale=1'>"
              "<meta http-equiv=refresh content=10>"
              "<title>LoRa gateway</title>"
              "<style>body{font:14px/1.5 system-ui,sans-serif;margin:2rem;max-width:60rem}"
              "table{border-collapse:collapse;width:100%}"
              "th,td{border-bottom:1px solid #ddd;padding:.3rem .6rem;text-align:left}"
              "th{background:#f4f4f4}code{background:#f4f4f4;padding:0 .2rem}</style>");

    page += F("<h1>SparkFun LoRa Gateway 1-Channel</h1><table>");
    page += "<tr><th>MAC<td><code>" + WiFi.macAddress() + "</code>";
    page += "<tr><th>IP<td><code>" + WiFi.localIP().toString() + "</code>";
    page += "<tr><th>SSID<td>" + escapeHtml(WiFi.SSID().c_str());
    page += "<tr><th>WiFi RSSI<td>" + String(WiFi.RSSI()) + " dBm";
    page += "<tr><th>Uptime<td>" + String(millis() / 1000) + " s";
    page += "<tr><th>Radio<td>";
    page += radioReady ? F("SX1276 ready") : F("NOT FOUND");
    if (radioReady) {
        page += "<tr><th>Listening on<td>" +
                String(board::kDefaultFrequencyHz / 1e6, 3) + " MHz, SF" +
                String(board::kDefaultSpreadingFactor) + ", BW " +
                String(board::kDefaultBandwidthHz / 1000) + " kHz, sync 0x" +
                String(board::kDefaultSyncWord, HEX);
        page += "<tr><th>Noise floor<td>" + String(radio.rssiNowDbm()) + " dBm";
        page += "<tr><th>Packets<td>" + String(historyCount) +
                " received, " + String(radio.crcErrors()) + " CRC errors";
    }
    page += F("</table>");

    page += F("<h2>Recent packets</h2>");
    if (historyCount == 0) {
        page += F("<p>Nothing heard yet. A LoRa receiver only hears traffic "
                  "matching its frequency, bandwidth, spreading factor and sync "
                  "word, so silence here is not evidence the band is quiet.</p>");
    } else {
        page += F("<table><tr><th>age<th>bytes<th>RSSI<th>SNR<th>payload");
        const size_t shown = min(historyCount, kHistoryLength);
        for (size_t i = 0; i < shown; ++i) {
            const Entry& e = slotFor(historyCount - 1 - i);
            page += "<tr><td>" + String((millis() - e.atMs) / 1000) + " s ago";
            page += "<td>" + String(e.length);
            page += "<td>" + String(e.rssiDbm) + " dBm";
            page += "<td>" + String(e.snrDb, 1) + " dB";
            page += "<td><code>" + escapeHtml(e.text) + "</code>";
        }
        page += F("</table>");
    }
    server.send(200, "text/html", page);
}

// A machine-readable view, so this can be polled by something else later
// without scraping HTML.
void handleJson() {
    String out = "{\"mac\":\"" + WiFi.macAddress() +
                 "\",\"ip\":\"" + WiFi.localIP().toString() +
                 "\",\"uptime_s\":" + String(millis() / 1000) +
                 ",\"radio_ready\":" + (radioReady ? "true" : "false") +
                 ",\"received\":" + String(historyCount) +
                 ",\"crc_errors\":" + String(radio.crcErrors()) +
                 ",\"packets\":[";
    const size_t shown = min(historyCount, kHistoryLength);
    for (size_t i = 0; i < shown; ++i) {
        const Entry& e = slotFor(historyCount - 1 - i);
        if (i) out += ',';
        out += "{\"age_s\":" + String((millis() - e.atMs) / 1000) +
               ",\"len\":" + String(e.length) +
               ",\"rssi\":" + String(e.rssiDbm) +
               ",\"snr\":" + String(e.snrDb, 1) + "}";
    }
    out += "]}";
    server.send(200, "application/json", out);
}

// Starting the HTTP server before WiFi is up is fatal, not merely useless.
// WebServer::begin() creates a listening socket, and with no network interface
// lwIP's TCP/IP thread has no mailbox yet:
//
//   assert failed: tcpip_send_msg_wait_sem tcpip.c:455 (Invalid mbox)
//
// which panics and reboots, forever, because the same thing happens on the
// next boot. So this is called only after a successful connection, and only
// once -- the listening socket survives WiFi dropping and returning.
void startHttp() {
    if (httpStarted) {
        return;
    }
    server.on("/", handleRoot);
    server.on("/status.json", handleJson);
    server.begin();
    httpStarted = true;
    Serial.println("http server started");
}

// Read and dispatch any pending serial command. Pulled out of loop() because
// connectWifi() blocks for up to 20 s, and a console that stops answering
// while it retries is a console you cannot fix a typo with -- which is exactly
// when you need it.
void pumpSerial() {
    while (Serial.available()) {
        const char c = static_cast<char>(Serial.read());
        if (c == '\n' || c == '\r') {
            if (serialLine.length()) {
                netconfig::handleCommand(serialLine);
                serialLine = "";
            }
        } else if (serialLine.length() < 128) {
            serialLine += c;
        }
    }
}

bool connectWifi() {
    const netconfig::Credentials creds = netconfig::load();
    if (!creds.valid) {
        Serial.println("no WiFi credentials stored. Set them:");
        netconfig::printHelp();
        return false;
    }

    Serial.printf("connecting to \"%s\" ...\n", creds.ssid);
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(kHostname);
    WiFi.begin(creds.ssid, creds.pass);

    const uint32_t deadline = millis() + kConnectTimeout;
    while (WiFi.status() != WL_CONNECTED &&
           static_cast<int32_t>(millis() - deadline) < 0) {
        delay(250);
        Serial.print('.');
        pumpSerial();
        // A fresh `save` or `connect` supersedes this attempt. Without this,
        // correcting a wrong SSID means waiting out the timeout first.
        if (netconfig::connectRequested()) {
            Serial.println(" superseded");
            return false;
        }
    }
    Serial.println();

    if (WiFi.status() != WL_CONNECTED) {
        // Say which failure it is. "Wrong password" and "no such network" are
        // different problems and the status code distinguishes them.
        Serial.printf("failed, status=%d ", static_cast<int>(WiFi.status()));
        switch (WiFi.status()) {
            case WL_NO_SSID_AVAIL: Serial.println("(no such network in range)"); break;
            case WL_CONNECT_FAILED: Serial.println("(rejected -- wrong password?)"); break;
            default: Serial.println("(timed out)"); break;
        }
        Serial.println("type 'help' for the configuration commands");
        return false;
    }

    Serial.printf("connected. IP http://%s/  (also http://%s.local/)\n",
                  WiFi.localIP().toString().c_str(), kHostname);
    Serial.printf("WiFi RSSI %d dBm\n", WiFi.RSSI());

    if (MDNS.begin(kHostname)) {
        MDNS.addService("http", "tcp", 80);
    }
    return true;
}

}  // namespace

void setup() {
    Serial.begin(115200);
    delay(1500);
    Serial.println("\n=== LoRa listening gateway ===");
    Serial.printf("MAC %s\n", WiFi.macAddress().c_str());

    const SX1276::Config config = {
        .frequencyHz     = board::kDefaultFrequencyHz,
        .spreadingFactor = board::kDefaultSpreadingFactor,
        .bandwidthHz     = board::kDefaultBandwidthHz,
        .codingRate4     = board::kDefaultCodingRate4,
        .txPowerDbm      = board::kDefaultTxPowerDbm,
        .syncWord        = board::kDefaultSyncWord,
    };
    radioReady = radio.begin(config);
    if (radioReady) {
        radio.listen();
        Serial.printf("radio: SX1276 on %.3f MHz SF%u BW%lu sync 0x%02X\n",
                      board::kDefaultFrequencyHz / 1e6,
                      board::kDefaultSpreadingFactor,
                      static_cast<unsigned long>(board::kDefaultBandwidthHz / 1000),
                      board::kDefaultSyncWord);
    } else {
        Serial.println("radio: NOT FOUND -- check lib/board/board.h");
    }

    if (connectWifi()) {
        startHttp();
    }
    Serial.println("type 'help' for WiFi configuration commands\n");
}

void loop() {
    // Only once the network is up. See startHttp().
    if (httpStarted) {
        server.handleClient();
    }

    // Serial stays a command console the whole time, so the board can be
    // reconfigured without reflashing and without a browser.
    pumpSerial();

    if (netconfig::connectRequested()) {
        netconfig::clearConnectRequest();
        WiFi.disconnect();
        if (connectWifi()) {
            startHttp();   // first successful connection of this boot
        }
    }

    // Retry periodically when credentials exist but we are not associated --
    // the access point may simply have been down when we booted. Sixty seconds
    // rather than thirty: each attempt blocks for up to twenty, and a board
    // that spends most of its time connecting is one that misses LoRa packets
    // and answers the console slowly.
    static uint32_t nextRetry = 0;
    if (!WiFi.isConnected() && static_cast<int32_t>(millis() - nextRetry) >= 0) {
        nextRetry = millis() + 60000;
        if (netconfig::load().valid && connectWifi()) {
            startHttp();
        }
    }

    if (radioReady) {
        SX1276::Packet packet;
        if (radio.poll(&packet)) {
            record(packet);
            const Entry& e = slotFor(historyCount - 1);
            Serial.printf("[%6lu] rx %3u bytes  RSSI %4d dBm  SNR %5.1f dB  | %s\n",
                          static_cast<unsigned long>(millis() / 1000),
                          e.length, e.rssiDbm, e.snrDb, e.text);
        }
    }
}

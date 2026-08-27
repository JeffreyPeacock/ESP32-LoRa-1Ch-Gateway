// Radio probe -- prove the SX1276 pin map against the actual hardware.
//
// This project recorded the radio as sitting on the ESP32's default VSPI bus
// until the schematic said otherwise. Four documents now agree it is on
// SCK 14 / MISO 12 / MOSI 13 with NSS 16, but nothing had ever asked the chip.
// This does.
//
// The test is RegVersion (0x42), which an SX1276 answers with 0x12. It is the
// right probe because it is a pure read of a constant: no configuration, no
// mode changes, nothing that depends on the radio being set up correctly. If
// SPI is wired as we believe, it returns 0x12. If not, it returns 0x00 or 0xFF
// -- the bus floating high or low -- which is precisely the silent failure that
// makes a near-miss firmware target so hard to diagnose.
//
// It also settles a contradiction SparkFun never resolved. Their LMIC sketch
// says the radio's reset is GPIO 5; their gateway tutorial says GPIO 27 and
// comments it "Reset not used". Both are published, they disagree, and the
// schematic's RFM_RST net could not be traced to either. So drive each in turn
// and watch whether the chip actually responds to it.

#include <Arduino.h>
#include <SPI.h>

// From the schematic, cross-checked against SparkFun's Arduino variant
// (sparkx_esp32_lora/pins_arduino.h) and the gateway firmware's own comments.
static const int PIN_SCK  = 14;
static const int PIN_MISO = 12;
static const int PIN_MOSI = 13;
static const int PIN_NSS  = 16;

// The two candidate reset pins. Neither is confirmed.
static const int PIN_RST_LMIC     = 5;
static const int PIN_RST_TUTORIAL = 27;

// What this project believed before reading the schematic. Probed here so the
// correction is demonstrated rather than asserted.
static const int VSPI_SCK  = 18;
static const int VSPI_MISO = 19;
static const int VSPI_MOSI = 23;

static const uint8_t REG_VERSION     = 0x42;
static const uint8_t SX1276_VERSION  = 0x12;
static const uint8_t REG_OP_MODE     = 0x01;

static SPIClass spi(HSPI);

// Read one SX127x register. Bit 7 clear selects a read; the address occupies
// the low 7 bits.
static uint8_t readRegister(uint8_t addr) {
    spi.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    digitalWrite(PIN_NSS, LOW);
    spi.transfer(addr & 0x7F);
    uint8_t value = spi.transfer(0x00);
    digitalWrite(PIN_NSS, HIGH);
    spi.endTransaction();
    return value;
}

static void writeRegister(uint8_t addr, uint8_t value) {
    spi.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    digitalWrite(PIN_NSS, LOW);
    spi.transfer(addr | 0x80);
    spi.transfer(value);
    digitalWrite(PIN_NSS, HIGH);
    spi.endTransaction();
}

// 0x00 and 0xFF are what a bus with nothing driving it returns. Treating them
// as "no answer" rather than as data is the whole point of this probe.
static bool isNoAnswer(uint8_t v) { return v == 0x00 || v == 0xFF; }

static void probeBus(const char *label, int sck, int miso, int mosi) {
    spi.end();
    spi.begin(sck, miso, mosi, PIN_NSS);
    pinMode(PIN_NSS, OUTPUT);
    digitalWrite(PIN_NSS, HIGH);
    delay(10);

    uint8_t v = readRegister(REG_VERSION);
    Serial.printf("  %-34s SCK %2d / MISO %2d / MOSI %2d  ->  0x%02X  %s\n",
                  label, sck, miso, mosi, v,
                  v == SX1276_VERSION ? "SX1276 FOUND"
                                      : (isNoAnswer(v) ? "no answer" : "unexpected"));
}

// Does toggling `pin` actually reset the radio? Put a recognisable non-default
// value in a register, pulse the pin low, and see whether it reverts.
//
// RegOpMode resets to 0x09 (LoRa off, STDBY). Writing 0x00 (SLEEP, FSK) is
// safe, reversible, and clearly distinct from the reset value.
static void probeReset(int pin) {
    writeRegister(REG_OP_MODE, 0x00);
    uint8_t before = readRegister(REG_OP_MODE);

    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);     // SX127x reset is active low
    delay(2);
    digitalWrite(pin, HIGH);
    delay(10);

    uint8_t after = readRegister(REG_OP_MODE);
    bool changed = (after != before);
    Serial.printf("  GPIO %-2d  RegOpMode %02X -> %02X   %s\n",
                  pin, before, after,
                  changed ? "RESET LINE -- the radio reacted"
                          : "no effect -- not this pin");
    pinMode(pin, INPUT);        // leave it floating again
}

void setup() {
    Serial.begin(115200);
    delay(1500);                // let the USB bridge settle before printing

    Serial.println();
    Serial.println("=== SparkFun LoRa Gateway 1-Channel (ESP32) -- radio probe ===");
    Serial.printf("chip: %s rev %d, %d MHz\n",
                  ESP.getChipModel(), ESP.getChipRevision(), getCpuFrequencyMhz());

    Serial.println("\n-- SPI bus: which wiring does the radio actually answer on?");
    probeBus("recorded (schematic)", PIN_SCK, PIN_MISO, PIN_MOSI);
    probeBus("previously assumed (VSPI)", VSPI_SCK, VSPI_MISO, VSPI_MOSI);

    // Put the bus back on the wiring that works before probing reset.
    spi.end();
    spi.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_NSS);
    pinMode(PIN_NSS, OUTPUT);
    digitalWrite(PIN_NSS, HIGH);
    delay(10);

    uint8_t version = readRegister(REG_VERSION);
    if (version != SX1276_VERSION) {
        Serial.printf("\nradio did not answer (0x%02X). Skipping the reset probe: "
                      "without a working bus it could not tell you anything.\n", version);
        return;
    }

    Serial.println("\n-- Reset: SparkFun publishes two different pins. Which is real?");
    probeReset(PIN_RST_LMIC);
    probeReset(PIN_RST_TUTORIAL);

    Serial.println("\n-- Registers (sanity check that we are reading real data)");
    Serial.printf("  RegVersion  0x42 = 0x%02X  (SX1276 answers 0x12)\n", version);
    Serial.printf("  RegOpMode   0x01 = 0x%02X\n", readRegister(REG_OP_MODE));
    Serial.printf("  RegFrfMsb   0x06 = 0x%02X\n", readRegister(0x06));
    Serial.printf("  RegFrfMid   0x07 = 0x%02X\n", readRegister(0x07));
    Serial.printf("  RegFrfLsb   0x08 = 0x%02X\n", readRegister(0x08));

    Serial.println("\n=== probe complete ===");
}

void loop() {
    delay(10000);
}

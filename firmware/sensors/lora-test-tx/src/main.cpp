#include <Arduino.h>
#include <RadioLib.h>
#include <Wire.h>
#include <SSD1306Wire.h>

// ------------------------------------------------------------
// Heltec WiFi LoRa 32 V3
// ------------------------------------------------------------

// SX1262
#define LORA_NSS    8
#define LORA_DIO1  14
#define LORA_RST   12
#define LORA_BUSY  13

// OLED
#define OLED_SDA   17
#define OLED_SCL   18
#define OLED_RST   21

// LoRa parameters
constexpr float LORA_FREQ = 915.0;   // MHz
constexpr float LORA_BW   = 125.0;   // kHz
constexpr int   LORA_SF   = 9;
constexpr int   LORA_CR   = 7;
constexpr int   LORA_PWR  = 10;      // dBm

SX1262 radio = new Module(
    LORA_NSS,
    LORA_DIO1,
    LORA_RST,
    LORA_BUSY
);

SSD1306Wire display(
    0x3C,
    OLED_SDA,
    OLED_SCL
);

uint32_t packetNumber = 0;

// ------------------------------------------------------------

void showStatus(const String& line1,
                const String& line2 = "",
                const String& line3 = "")
{
    display.clear();

    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);

    display.drawString(0, 0,  line1);
    display.drawString(0, 16, line2);
    display.drawString(0, 32, line3);

    display.display();
}

// ------------------------------------------------------------

void setup()
{
    Serial.begin(115200);
    delay(1000);

    // OLED reset
    pinMode(OLED_RST, OUTPUT);
    digitalWrite(OLED_RST, LOW);
    delay(20);
    digitalWrite(OLED_RST, HIGH);
    delay(20);

    display.init();
    display.flipScreenVertically();

    showStatus(
        "Den LoRa Test TX",
        "Initializing...",
        "915 MHz"
    );

    Serial.println();
    Serial.println("Den LoRa Test TX");
    Serial.println("----------------");

    // Heltec V3 SX1262 uses a TCXO.
    radio.tcxoVoltage = 1.8;

    Serial.print("Initializing SX1262... ");

    int state = radio.begin(
        LORA_FREQ,
        LORA_BW,
        LORA_SF,
        LORA_CR,
        RADIOLIB_SX126X_SYNC_WORD_PRIVATE,
        LORA_PWR,
        8,
        1.8
    );

    if (state != RADIOLIB_ERR_NONE)
    {
        Serial.print("FAILED, code ");
        Serial.println(state);

        showStatus(
            "RADIO FAILED",
            "Error:",
            String(state)
        );

        while (true)
            delay(1000);
    }

    Serial.println("OK");

    showStatus(
        "Den LoRa Test TX",
        "Radio OK",
        "915.0 MHz"
    );

    delay(1500);
}

// ------------------------------------------------------------

void loop()
{
    String packet =
        "DEN TEST " + String(packetNumber);

    Serial.print("TX: ");
    Serial.print(packet);
    Serial.print(" ... ");

    showStatus(
        "LoRa TX  915 MHz",
        "Packet: " + String(packetNumber),
        "Transmitting..."
    );

    int state = radio.transmit(packet);

    if (state == RADIOLIB_ERR_NONE)
    {
        Serial.println("OK");

        showStatus(
            "LoRa TX  915 MHz",
            "Packet: " + String(packetNumber),
            "TX OK"
        );
    }
    else
    {
        Serial.print("FAILED: ");
        Serial.println(state);

        showStatus(
            "LoRa TX ERROR",
            "Packet: " + String(packetNumber),
            "Error: " + String(state)
        );
    }

    packetNumber++;

    delay(2000);
}

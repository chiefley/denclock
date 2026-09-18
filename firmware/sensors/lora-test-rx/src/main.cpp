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

// Must match transmitter
constexpr float LORA_FREQ = 915.0;   // MHz
constexpr float LORA_BW   = 125.0;   // kHz
constexpr int   LORA_SF   = 9;
constexpr int   LORA_CR   = 7;
constexpr int   LORA_PWR  = 10;      // irrelevant for RX, but keeps config identical

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
        "Den LoRa Test RX",
        "Initializing...",
        "915 MHz"
    );

    Serial.println();
    Serial.println("Den LoRa Test RX");
    Serial.println("----------------");

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
    Serial.println("Waiting for packets...");

    showStatus(
        "LoRa RX  915 MHz",
        "Radio OK",
        "Waiting..."
    );
}

// ------------------------------------------------------------

void loop()
{
    String packet;

    // Blocking receive. Fine for our bench-test receiver.
    int state = radio.receive(packet);

    if (state == RADIOLIB_ERR_NONE)
    {
        float rssi = radio.getRSSI();
        float snr  = radio.getSNR();

        Serial.print("RX: ");
        Serial.print(packet);

        Serial.print("   RSSI: ");
        Serial.print(rssi, 1);
        Serial.print(" dBm");

        Serial.print("   SNR: ");
        Serial.print(snr, 1);
        Serial.println(" dB");

        showStatus(
            packet,
            "RSSI: " + String(rssi, 1) + " dBm",
            "SNR:  " + String(snr, 1) + " dB"
        );
    }
    else if (state == RADIOLIB_ERR_CRC_MISMATCH)
    {
        Serial.println("CRC error");

        showStatus(
            "LoRa RX  915 MHz",
            "CRC error",
            "Waiting..."
        );
    }
    else
    {
        Serial.print("Receive failed: ");
        Serial.println(state);

        showStatus(
            "RX ERROR",
            "Code: " + String(state),
            "Waiting..."
        );
    }
}
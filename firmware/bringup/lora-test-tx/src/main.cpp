#include <Arduino.h>
#include <RadioLib.h>
#include <Wire.h>
#include <SSD1306Wire.h>
#include <Adafruit_SHT4x.h>
#include <math.h>

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

// Header SDA2/SCK2 are assumed to be GPIO41/GPIO42 (SCK2 means I2C SCL).
// Keep Wire (bus 0) on the OLED and use Wire1 for the external sensor.
constexpr int SENSOR_SDA = 41;
constexpr int SENSOR_SCL = 42;
// Built-in battery sensing circuit: no external divider or jumper required.
constexpr int BATTERY_ADC = 1;
constexpr int BATTERY_ENABLE = 37; // Software output: HIGH enables measurement.
constexpr float BATTERY_DIVIDER = 4.9f; // Onboard 390k/100k divider.
Adafruit_SHT4x sht40;
bool sensorReady = false;
uint32_t packetNumber = 0;

bool initializeSensor()
{
    if (!sht40.begin(&Wire1))
        return false;
    sht40.setPrecision(SHT4X_HIGH_PRECISION);
    sht40.setHeater(SHT4X_NO_HEATER);
    return true;
}

// ------------------------------------------------------------

float readBatteryVoltage()
{
    digitalWrite(BATTERY_ENABLE, HIGH);
    delay(10); // Allow the onboard switched divider to settle.
    analogReadMilliVolts(BATTERY_ADC); // Discard the first conversion.
    uint32_t millivolts = 0;
    for (int i = 0; i < 16; ++i)
    {
        millivolts += analogReadMilliVolts(BATTERY_ADC);
        delay(1);
    }
    digitalWrite(BATTERY_ENABLE, LOW);
    return (millivolts / 16.0f) * BATTERY_DIVIDER / 1000.0f;
}

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

    // Vext is active LOW; keep it powered for both the OLED and SHT40.
    pinMode(Vext, OUTPUT);
    digitalWrite(Vext, LOW);
    delay(50);

    pinMode(BATTERY_ENABLE, OUTPUT);
    digitalWrite(BATTERY_ENABLE, LOW);
    analogReadResolution(12);
    analogSetPinAttenuation(BATTERY_ADC, ADC_0db);

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

    if (!Wire1.begin(SENSOR_SDA, SENSOR_SCL, 100000))
    {
        Serial.println("Sensor I2C initialization failed");
        showStatus("SENSOR BUS FAILED", "Check GPIO41/42");
        while (true)
            delay(1000);
    }
    Wire1.setTimeOut(100);
    sensorReady = initializeSensor();
    Serial.println(sensorReady ? "SHT40 OK" : "SHT40 missing; will retry");

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
    if (!sensorReady)
        sensorReady = initializeSensor();

    sensors_event_t humidity{}, temperature{};
    bool valid = sensorReady && sht40.getEvent(&humidity, &temperature);
    valid = valid && isfinite(temperature.temperature)
        && isfinite(humidity.relative_humidity)
        && temperature.temperature >= -40 && temperature.temperature <= 125
        && humidity.relative_humidity >= 0 && humidity.relative_humidity <= 100;
    if (!valid)
    {
        sensorReady = false; // Retry initialization on the next test cycle.
        Serial.println("SHT40 read failed; sending nan for temperature and RH");
    }

    const String tempText = valid ? String(temperature.temperature, 2) : "nan";
    const String rhText = valid ? String(humidity.relative_humidity, 2) : "nan";
    const String batteryText = String(readBatteryVoltage(), 2);
    String packet = "T=" + tempText + " C RH=" + rhText
        + " % BAT=" + batteryText + " V";

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
            "TX T: " + (valid ? tempText : String("N/A")) + " C",
            "RH: " + (valid ? rhText : String("N/A")) + " %",
            "Battery: " + batteryText + " V"
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

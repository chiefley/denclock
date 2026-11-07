# Hardware
- Beelink Mini 8 NUC at static IP 192.168.50.90:5000 
- ROADOM 10.1" Touchscreen Monitor, 1024x600 IPS
- LoRa V3 ESP32 LoRa V3 Development Board, SX1262 ESP32 V3 Dual-core OLED Type C WI-FI Kit OLED Display ESP32 Module CP2012 863-928 MHz for IOT Meshtastic Arduin0
- Adafruit Sensirion SHT40 Temperature & Humidity Sensor - STEMMA QT / Qwiic

# Connections
SHT40 powered from the HelTek LoRa V3 board Vext Pin.
SHT40 I2C connected to SDA2 and SCK2 on the LoRa board.

## Surface Go 3 client

The Debian 13 / XFCE Surface Go 3 is an additional deployment, separate from the
Windows NUC above. Use the [master Surface Go provisioning guide](Surface%20Go%203%20Installation%20and%20Deployment%20Guide.md)
for hardware preparation, installation, observed reference configuration, and
first-device acceptance.

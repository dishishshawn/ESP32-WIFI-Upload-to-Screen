# ESP32 Wi-Fi Upload to Screen

An ESP32 hosts a small web page on your local network. Upload a JPEG from your phone or computer, and it appears on a 1.8" ST7735 TFT display. The image is saved to flash (LittleFS), so it survives a reset.

## How it works

1. The ESP32 joins your Wi-Fi and prints its IP address over serial (115200 baud).
2. Open that IP in a browser and upload a `.jpg`.
3. The upload is written to flash as `/latest.jpg`.
4. The main loop decodes the JPEG with TJpg_Decoder, scaling it down (1/2 or 1/4) to fit the 128×160 screen, and draws it with TFT_eSPI.

## Hardware

- ESP32 DevKit V1
- ST7735 1.8" 128×160 TFT (SPI)

## Setup

Built with [PlatformIO](https://platformio.org/) and the Arduino framework.

1. Create `src/secrets.h` (it is git-ignored):
   ```cpp
   #define WIFI_SSID "your-network"
   #define WIFI_PASSWORD "your-password"
   ```
2. Configure your display pins in TFT_eSPI's `User_Setup.h`.
3. Set `upload_port` in `platformio.ini` to your board's serial port.
4. Build and upload: `pio run -t upload`, then `pio device monitor` to get the IP.

## Libraries

ESPAsyncWebServer, AsyncTCP, TFT_eSPI, TJpg_Decoder, LittleFS

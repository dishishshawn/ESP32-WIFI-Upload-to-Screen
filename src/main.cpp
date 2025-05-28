#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
#include <LittleFS.h>

#include "secrets.h" // WIFI_SSID and WIFI_PASSWORD should be defined in this file

TFT_eSPI tft = TFT_eSPI();
AsyncWebServer server(80); // Default port

#define TFT_WIDTH 128
#define TFT_HEIGHT 160

volatile bool newImageReady = false;

// Could use SPIFFS, but LittleFS is easier in this case
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 Upload</title>
</head>
<body>
  <h1>ESP32 File Upload</h1>
  <form method="POST" action="/upload" enctype="multipart/form-data">
    <input type="file" name="file" accept="image/jpeg">
    <input type="submit" value="Upload">
  </form>
  <a href="/show">Show Uploaded Image</a>
  <p><b>Note:</b> Image is stored in flash and survives reset.</p>
</body>
</html>
)rawliteral";

// Fast JPEG-to-TFT callback
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  tft.pushImage(x, y, w, h, bitmap);
  return true;
}

void setup() {
  Serial.begin(115200);
  while (!Serial) continue;

  tft.init();
  tft.setRotation(0); // Portrait mode
  tft.fillScreen(TFT_BLACK);
  tft.setSwapBytes(true);
  delay(2000);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    tft.setCursor(0, 0);
    tft.print("Wifi not connected, waiting...");
    Serial.print(".");
  }
  tft.fillScreen(TFT_WHITE);
  tft.setCursor(0, 0);
  tft.print("Wifi connected,");
  Serial.println("Wifi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  TJpgDec.setCallback(tft_output);

  // Initialize LittleFS
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS initialization failed!");
    return;
  }

  // Serve HTML from PROGMEM
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });

  // File Upload
  server.on("/upload", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      request->send(200, "text/plain", "Upload successful! <a href='/show'>Show Image</a>");
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      static File file;
      if (index == 0) {
        file = LittleFS.open("/upload.tmp", "w");
        if (!file) {
          Serial.println("Failed to open file for writing!");
          request->send(507, "text/plain", "Insufficient Storage");
          return;
        }
        Serial.printf("Starting upload: %s\n", filename.c_str());
      }
      file.write(data, len);
      if (final) {
        file.close();
        LittleFS.rename("/upload.tmp", "/latest.jpg");
        newImageReady = true;
        Serial.printf("Upload complete: %s\n", filename.c_str());
      }
    }
  );

  // Show image on TFT from flash
  server.on("/show", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (LittleFS.exists("/latest.jpg")) {
      request->send(200, "text/plain", "Image will be displayed on TFT.");
    } else {
      request->send(200, "text/plain", "No image uploaded.");
    }
  });

  server.begin();
}

void loop() {
  if (newImageReady) {
    tft.fillScreen(TFT_BLACK);
    File jpgFile = LittleFS.open("/latest.jpg", "r");
    if (jpgFile) {
      size_t jpgSize = jpgFile.size();
      uint8_t* jpgBuf = (uint8_t*)malloc(jpgSize);
      if (jpgBuf && jpgFile.read(jpgBuf, jpgSize) == jpgSize) {
        // Decode and resize the JPEG
        TJpgDec.setJpgScale(1); // No scaling by default
        uint16_t jpgWidth = 0, jpgHeight = 0;
        TJpgDec.getJpgSize(&jpgWidth, &jpgHeight, jpgBuf, jpgSize);

        // Calculate scaling factor to fit the display
        float xScale = (float)TFT_WIDTH / jpgWidth;
        float yScale = (float)TFT_HEIGHT / jpgHeight;
        float scale = (xScale < yScale) ? xScale : yScale;

        // Set the scale for decoding
        if (scale < 1.0) {
          uint8_t jpgScale = 1;
          if (scale <= 0.25)      jpgScale = 4;
          else if (scale <= 0.5)  jpgScale = 2;
          else                    jpgScale = 1;
          TJpgDec.setJpgScale(jpgScale);
        } else {
          TJpgDec.setJpgScale(1);
        }

        // Decode and draw the resized JPEG
        TJpgDec.drawJpg(0, 0, jpgBuf, jpgSize);
      } else {
        Serial.println("Failed to read JPEG data into buffer!");
      }
      free(jpgBuf);
      jpgFile.close();
    } else {
      Serial.println("Failed to open /latest.jpg for reading!");
    }
    newImageReady = false;
  }
  delay(100);
}
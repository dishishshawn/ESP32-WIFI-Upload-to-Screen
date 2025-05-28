#include <Arduino.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <AsyncTCP.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <JPEGDecoder.h>
#include "WifiServer.h"
#include "secrets.h"

#define HOST "192.168.125.122"
#define PORT 3333
#define MAX_CLIENTS CONFIG_LWIP_MAX_ACTIVE_TCP

size_t permits = MAX_CLIENTS;

// from github me-no-dev/AsyncTCP
void makeRequest() {
  if (!permits)
    return;

  Serial.printf("** permits: %d\n", permits);

  AsyncClient* client = new AsyncClient;

  client->onError([](void* arg, AsyncClient* client, int8_t error) {
    Serial.printf("** error occurred %s \n", client->errorToString(error));
    client->close(true);
    delete client;
  });

  client->onConnect([](void* arg, AsyncClient* client) {
    permits--;
    Serial.printf("** client has been connected: %" PRIu16 "\n", client->localPort());

    client->onDisconnect([](void* arg, AsyncClient* client) {
      Serial.printf("** client has been disconnected: %" PRIu16 "\n", client->localPort());
      client->close(true);
      delete client;

      permits++;
      makeRequest();
    });

    client->onData([](void* arg, AsyncClient* client, void* data, size_t len) {
      Serial.printf("** data received by client: %" PRIu16 ": len=%u\n", client->localPort(), len);
    });

    client->write("GET /README.md HTTP/1.1\r\nHost: " HOST "\r\nUser-Agent: ESP\r\nConnection: close\r\n\r\n");
  });

  if (client->connect(HOST, PORT)) {
  } else {
    Serial.println("** connection failed");
  }
}

TFT_eSPI tft = TFT_eSPI(); // Invoke library, pins defined in User_Setup.h


void setup() {
  Serial.begin(115200);
  while(!Serial) 
    continue;

  tft.init();
  tft.setRotation(1); // Landscape mode
  tft.fillScreen(TFT_BLACK); 

  SPIFFS.begin(true); 

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    tft.setCursor(0, 0);
    tft.print("Wifi not connected, waiting...");
  }
  tft.fillScreen(TFT_WHITE);
  tft.setCursor(0, 0);
  tft.print("Wifi connected,");
  Serial.println(WiFi.localIP());

}

void loop() {


  
}
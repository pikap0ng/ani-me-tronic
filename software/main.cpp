#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include "Audio.h"

// Wi-Fi credentials (referencing lab 4)
char ssid[50];
char pass[50];

// ChatGPT API Key
const char* openai_api_key = "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";

// AWS endpoint for storing data
const char* aws_endpoint = "http://ec2-54-193-17-156.us-west-1.compute.amazonaws.com/data";

// I2S pins, this is for the hardware setup of the speaker connection 
#define I2S_DOUT 25
#define I2S_BCLK 27
#define I2S_LRC 26

Audio audio;

// Initialize NVS and retrieve Wi-Fi credentials
void nvs_access() {
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);

  nvs_handle_t my_handle;
  err = nvs_open("storage", NVS_READWRITE, &my_handle);
  if (err == ESP_OK) {
    size_t ssid_len = sizeof(ssid);
    size_t pass_len = sizeof(pass);

    if (nvs_get_str(my_handle, "ssid", ssid, &ssid_len) == ESP_OK &&
        nvs_get_str(my_handle, "pass", pass, &pass_len) == ESP_OK) {
      Serial.println("WiFi credentials retrieved successfully.");
    } else {
      Serial.println("Failed to retrieve WiFi credentials from NVS.");
    }
    nvs_close(my_handle);
  } else {
    Serial.println("Failed to open NVS storage.");
  }
}

// Connect to Wi-Fi
void connectToWiFi() {
  WiFi.begin(ssid, pass);
  Serial.print("Connecting to WiFi");
  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected successfully.");
    Serial.println("IP address: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nFailed to connect to WiFi.");
    while (true) {
      delay(1000);
    }
  }
}

// Send command to ChatGPT + retrieve response
String sendToChatGPT(String command) {
  HTTPClient http;
  http.begin("https://api.openai.com/v1/completions");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", String("Bearer ") + openai_api_key);

  // Create JSON payload
    // note to self: this is a best practice that makes it easier to refine stuff moving forward 
  String payload = "{\"model\": \"text-davinci-003\", \"prompt\": \"" + command + "\", \"max_tokens\": 150}";

  int httpResponseCode = http.POST(payload);
  if (httpResponseCode == 200) {
    String response = http.getString();
    int start = response.indexOf("\"text\":\"") + 9;
    int end = response.indexOf("\"", start);
    http.end();
    return response.substring(start, end);
  } else {
    Serial.printf("Error sending to ChatGPT: %d\n", httpResponseCode);
  }
  http.end();
  return "Error: Unable to fetch response";
}

// Store interaction in AWS
void storeInAWS(String command, String response) {
  HTTPClient http;
  http.begin(aws_endpoint);
  http.addHeader("Content-Type", "application/json");

  // Construct JSON payload
  String jsonPayload = "{\"command\": \"" + command + "\", \"response\": \"" + response + "\"}";

  int httpResponseCode = http.POST(jsonPayload);
  if (httpResponseCode > 0) {
    Serial.printf("AWS HTTP Response Code: %d\n", httpResponseCode);
    Serial.println("AWS Response: " + http.getString());
  } else {
    Serial.printf("AWS POST request failed: %s\n", http.errorToString(httpResponseCode).c_str());
  }

  http.end();
}

void setup() {
  Serial.begin(9600);
  delay(1000);

  nvs_access();
  connectToWiFi();

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(50); // speaker volume

  Serial.println("Type a command to send to ChatGPT:");
}

void loop() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    Serial.println("Command received: " + command);

    // Grab response from ChatGPT
    String response = sendToChatGPT(command);
    Serial.println("ChatGPT Response: " + response);

    // Play chatGPT response through speaker
    String ttsURL = "http://api.voicerss.org/?key=c7d469d48d164dbcbb6378b225e6d845&hl=en-us&src=" + response;
    audio.connecttohost(ttsURL.c_str());

    // Store interaction in AWS
    storeInAWS(command, response);
  }
}
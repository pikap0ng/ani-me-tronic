/*
 * ESP32 WebSocket Client for Button Press and Servo Control
 * 
 *    - Debounced button presses to avoid multiple triggers.
 *    - Demonstrates WebSocket integration with real-time hardware interaction.
 * 
 */


#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <Servo.h>

// Button and LED Pins
#define RED_BUTTON 33
// #define RED_BUTTON 39
#define BLUE_BUTTON 15
#define RED_LED 27
#define BLUE_LED 2
#define SERVO_BUTTON 32

// Wi-Fi credentials (retrieved from NVS)
// Reference Lab 4 code for all Wi-Fi related code 
char ssid[50];
char pass[50];

// WebSocket server
WebSocketsClient webSocket;
unsigned long lastReconnectAttempt = 0;  // Log the last time it reconnected 
const unsigned long reconnectInterval = 1000; // Retry connection (1/sec)
bool webSocketConnected = false;  // Track WebSocket connection status? 

// Servo control
Servo myservo;

// Initialize NVS and retrieve Wi-Fi credentials (lab 4 too) 
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

// Debugging WebSocket outputs
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch (type) {
        case WStype_CONNECTED: 
            webSocketConnected = true;
            Serial.println("WebSocket connected");
            break;

        case WStype_DISCONNECTED: 
            webSocketConnected = false;
            Serial.println("WebSocket disconnected");
            break;

        case WStype_TEXT: 
            Serial.printf("WebSocket message received: %s\n", payload);
            break;

        case WStype_BIN: // FOR DEBUGGING
            Serial.println("Binary message received (not handled)");
            break;

        case WStype_ERROR: // :( 
            Serial.println("WebSocket error occurred");
            break;

        case WStype_PING: // Hua ping
            Serial.println("WebSocket Ping");
            break;

        case WStype_PONG: 
            Serial.println("WebSocket Pong");
            break;

        default:
            Serial.println("Unhandled WebSocket event");
            break;
    }
}

void connectWebSocket() {
    Serial.println("Attempting to connect to WebSocket...");
    webSocket.begin("54.193.62.45", 5000, "/socket.io/?EIO=4&transport=websocket"); // Public IP address 
    webSocket.onEvent(webSocketEvent);
    // webSocket.setReconnectInterval(5000);

};


// Reconnect WebSocket if disconnected
void reconnectWebSocket() {
    if (!webSocketConnected && WiFi.status() == WL_CONNECTED) {
        unsigned long now = millis();
        if (now - lastReconnectAttempt > reconnectInterval) {
            lastReconnectAttempt = now;
            connectWebSocket();
        }
    }
}

// Send button press event
void sendButtonPress(String button) {
    if (webSocketConnected) {
        String jsonPayload = "{\"button\": \"" + button + "\"}";
        webSocket.sendTXT(jsonPayload);
        Serial.println("Sent WebSocket message: " + jsonPayload);
    } else {
        Serial.println("WebSocket not connected. Cannot send message.");
    }
}

void setup() {
    // Initialize Serial
    Serial.begin(9600);

    // Initialize NVS and retrieve Wi-Fi credentials
    nvs_access();

    // Initialize pins
    pinMode(RED_LED, OUTPUT);
    pinMode(BLUE_LED, OUTPUT);
    pinMode(SERVO_BUTTON, INPUT_PULLUP);
    pinMode(RED_BUTTON, INPUT_PULLUP);
    pinMode(BLUE_BUTTON, INPUT_PULLUP);

    // Initialize Servo
    myservo.attach(12);
    sleep(1);
    Serial.println("Setup complete");

    // Connect to Wi-Fi
    connectToWiFi();

    // Connect to WebSocket server
    connectWebSocket();
}

unsigned long lastPing = 0;
const unsigned long pingInterval = 10000;  // 10 seconds

void loop() {
    webSocket.loop();
    reconnectWebSocket();
    char key;

    if (Serial.available() > 0) { // Check if data is available to read
        key = Serial.read(); // Read one character from the serial buffer
        Serial.print("Key Pressed: ");
        Serial.println(key); // Print the character that was pressed

        // // Example: Do something when specific keys are pressed
        // if (key == 'a') {
        // Serial.println("You pressed 'A'!");
        // } else if (key == 'b') {
        // Serial.println("You pressed 'B'!");
        // } else if (key == '1') {
        // Serial.println("You pressed '1'!");
        // }
    }


    // Every 30 seconds to keep connection alive
    // DONT USE NOW ITS REDUNDDANT
    // if (millis() - lastPing > pingInterval && webSocketConnected) {
    //     webSocket.sendPing();
    //     Serial.println("Sent Ping to WebSocket server");
    //     lastPing = millis();
    // }

    // RED BUTTON: Turns on RED LED and sends "angry" event
    if (digitalRead(RED_BUTTON) == LOW || key == 'a') {
        digitalWrite(RED_LED, HIGH);
        sendButtonPress("angry");  // Send WebSocket message
        delay(300);  // Debounce
    } else {
        digitalWrite(RED_LED, LOW);
    }

    // BLUE BUTTON: Turns on BLUE LED and sends "happy" event
    if (digitalRead(BLUE_BUTTON) == LOW || key == 's') {
        digitalWrite(BLUE_LED, HIGH);
        sendButtonPress("happy");  // Send WebSocket message
        delay(300);  // Debounce
    } else {
        digitalWrite(BLUE_LED, LOW);
    }

    // SERVO CONTROLS
    if (digitalRead(SERVO_BUTTON) == LOW || key == 'd') {
        digitalWrite(25, HIGH);
        myservo.write(179);
    } else {
        digitalWrite(25, LOW);
        myservo.write(90);
    }
}

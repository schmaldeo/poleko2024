#include <Arduino.h>
#include "Sensor.h"
#include "WiFiHelpers.h"
#include "TCPServer.h"
#include "HTTPServer.h"
#include "EspUDPServer.h"
#include <WiFiManager.h>
#include <WiFi.h>
#include <Preferences.h>

// TODO find out why the first sensor reading is empty

constexpr byte BOOT_BUTTON_PIN = 0;

Sensor sensor(2, 16, 17);
TCPServer tcpServer(sensor);
HTTPServer httpServer(sensor);
EspUDPServer udpServer;

bool prevButtonState = HIGH;

void setupSerial();

void setup() {
    setupSerial();
    // this blocks because config portal blocks
    setupWiFi();
    // TODO check out how these three handle connection loss
    tcpServer.setup();
    udpServer.setup();
    httpServer.begin();
}

void loop() {
    auto buttonState = digitalRead(BOOT_BUTTON_PIN);
    if (buttonState == LOW && prevButtonState == HIGH) {
        setupIpSetup();
    }
    prevButtonState = buttonState;
    // TODO test if this works properly connection lost
    if (WiFi.status() != WL_CONNECTED) {
        digitalWrite(LED_PIN, LOW);
        setupIpSetup();
    }
    udpServer.loop();
    tcpServer.loop();
}


/// @brief Sets up UART with PC through USB
void setupSerial() {
    Serial.begin(9600);
}

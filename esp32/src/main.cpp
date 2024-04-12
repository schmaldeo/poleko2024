#include <Arduino.h>
#include "Sensor.h"
#include "WiFiHelpers.h"
#include "TCPServer.h"
#include "EspUDPServer.h"
#include "HTTPServer.h"
#include <WiFiManager.h>
#include <WiFi.h>
#include <Preferences.h>

constexpr byte BOOT_BUTTON_PIN = 0;

Sensor sensor(2, 16, 17);
TCPServer tcpServer(sensor);
HTTPServer httpServer(sensor);
EspUDPServer udpServer;

bool prevButtonState = HIGH;

void setupSerial();
void startServices();
void stopServices();

void setup() {
    setupSerial();
    // this blocks because config portal blocks
    setupWiFi();
    startServices();
}

void loop() {
    auto buttonState = digitalRead(BOOT_BUTTON_PIN);
    if (buttonState == LOW && prevButtonState == HIGH) {
        stopServices();
        setupIpSetup();
        startServices();
    }
    prevButtonState = buttonState;
    if (WiFi.status() != WL_CONNECTED) {
        digitalWrite(LED_PIN, LOW);
        stopServices();
        setupIpSetup();
        startServices();
    }
    udpServer.loop();
    tcpServer.loop();
    httpServer.loop();
}


/// @brief Sets up UART with PC through USB
void setupSerial() {
    Serial.begin(9600);
}

void startServices() {
    tcpServer.setup();
    udpServer.setup();
    httpServer.setup();
}

void stopServices() {
    tcpServer.stop();
    udpServer.stop();
    httpServer.stop();
}
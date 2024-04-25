#include <Arduino.h>
#include "Sensor.h"
#include "WiFiHelpers.h"
#include "TCPServer.h"
#include "EspUDPServer.h"
#include "HTTPServer.h"
#include <WiFiManager.h>
#include <WiFi.h>
#include <Preferences.h>

constexpr byte
BOOT_BUTTON_PIN = 0;

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
    // this call can potentially block the thread, because the configPortal blocks
    setupWiFi();
    startServices();
}

// DO NOT USE ANY FUNCTION THAT DELAYS THE EXECUTION OF CODE INSIDE THIS FUNCTION!!!
void loop() {
    auto buttonState = digitalRead(BOOT_BUTTON_PIN);
    // if the BOOT button was pressed, set up the configuration portal
    if (buttonState == LOW && prevButtonState == HIGH) {
        digitalWrite(LED_PIN, LOW);
        stopServices();
        setupIpSetup();
        startServices();
    }
    prevButtonState = buttonState;
    // if WiFi disconnected, turn the LED off and set up the configuration portal
    if (WiFi.status() != WL_CONNECTED) {
        digitalWrite(LED_PIN, LOW);
        stopServices();
        setupIpSetup();
        startServices();
    }

    // server code
    udpServer.loop();
    tcpServer.loop();
    httpServer.loop();
}


/// @brief Sets up USB UART connection
void setupSerial() {
    Serial.begin(9600);
}

/// @brief Starts servers
void startServices() {
    tcpServer.setup();
    udpServer.setup();
    httpServer.setup();
}

/// @brief Stops servers
void stopServices() {
    tcpServer.stop();
    udpServer.stop();
    httpServer.stop();
}
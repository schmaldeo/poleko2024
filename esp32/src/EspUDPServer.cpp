#include "EspUDPServer.h"

// instance variable is required because the timer handle is static, so there's a lot of shenanigans involving static methods
EspUDPServer* EspUDPServer::instance = nullptr;

EspUDPServer::EspUDPServer() : udp(WiFiUDP()), timer(ESP32Timer(0)) {
    instance = this;
}

/// @brief Sets up a UDP server periodically broadcasting sensor data (IP and MAC addresses) in the network. Must be used in the setup() function in
/// main.cpp. You must also include the EspUDPServer::loop() function in loop() in main.cpp.
/// @param port Port to use (5506 by default)
void EspUDPServer::setup(unsigned short port) {
    if (started) {
        return;
    }
    if (!stopped) {
        udp.begin(WiFi.localIP(), port);
        timer.attachInterrupt(0.2, timerHandle);
        log_e("UDP set up");
    } else {
        udp = WiFiUDP();
        udp.begin(WiFi.localIP(), port);
        timer.reattachInterrupt();
        log_e("UDP set up");
        stopped = false;
    }
    started = true;
}

/// @brief Stops the UDP server.
void EspUDPServer::stop() {
    if (stopped) {
        return;
    }
    udp.stop();
    timer.detachInterrupt();
    stopped = true;
    started = false;
    log_e("UDP stopped");
}

/// @brief Listens for timer flag changes. Must be used in loop() function in main.cpp. You must also include 
/// the EspUDPServer::setup() function in setup() in main.cpp.
void EspUDPServer::loop() {
    if (!instance->stopped) {
        if (timerFlag) {
            timerFlag = false;
            sendPacket();
        }
    }
}

/// @brief Hardware timer handle
bool EspUDPServer::timerHandle(void *_) {
    instance->timerFlag = true;
    return true;
}

/// @brief Broadcasts a packet that includes sensor's IP and MAC addresses
void EspUDPServer::sendPacket() {
    udp.beginPacket(IPAddress(255, 255, 255, 255), 5506);
    JsonDocument doc;
    doc["ip"] = WiFi.localIP();
    doc["mac"] = WiFi.macAddress();
    String interfaceInfo;
    serializeJson(doc, interfaceInfo);
    udp.printf(interfaceInfo.c_str());
    udp.endPacket();
}
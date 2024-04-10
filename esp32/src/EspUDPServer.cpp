#include "EspUDPServer.h"


EspUDPServer* EspUDPServer::instance = nullptr;

EspUDPServer::EspUDPServer() : udp(WiFiUDP()) {
    instance = this;
}

void EspUDPServer::setup(unsigned short port) {
    udp.begin(WiFi.localIP(), port);
    log_e("UDP set up");
    ESP32Timer timer(0);
    timer.attachInterrupt(0.2, timerHandle);
}

void EspUDPServer::loop() {
    if (timerFlag) {
        timerFlag = false;
        sendPacket();
    }
}

bool EspUDPServer::timerHandle(void *_) {
    instance->timerFlag = true;
    return true;
}

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
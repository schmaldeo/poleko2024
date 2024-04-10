#include "EspUDPServer.h"

EspUDPServer::EspUDPServer() : udp(WiFiUDP()) {
    instance = this;
}

EspUDPServer* EspUDPServer::instance = nullptr;
bool flag = false;

void EspUDPServer::setup(unsigned short port) {
    udp.begin(WiFi.localIP(), port);
    log_e("UDP set up");
    ESP32Timer timer(1);
    timer.attachInterrupt(3, timerHandle);
}

void EspUDPServer::loop() {
    if (flag) {
        flag = false;
        sendPacket();
    }
}

bool EspUDPServer::timerHandle(void *_) {
    instance->flag = true;
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
#include "EspUDPServer.h"


EspUDPServer* EspUDPServer::instance = nullptr;

EspUDPServer::EspUDPServer() : udp(WiFiUDP()), timer(ESP32Timer(0)) {
    instance = this;
}

void EspUDPServer::setup(unsigned short port) {
    if (!stoppedOnce) {
        udp.begin(WiFi.localIP(), port);
        timer.attachInterrupt(0.2, timerHandle);
        log_e("UDP set up");
    } else {
        udp = WiFiUDP();
        udp.begin(WiFi.localIP(), port);
        timer.reattachInterrupt();
        log_e("UDP set up");
    }
}

void EspUDPServer::stop() {
    udp.stop();
    timer.detachInterrupt();
    stoppedOnce = true;
    log_e("UDP stopped");
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
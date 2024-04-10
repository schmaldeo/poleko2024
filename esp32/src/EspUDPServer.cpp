#include "EspUDPServer.h"

EspUDPServer::EspUDPServer() : udp(WiFiUDP()) { }

void EspUDPServer::setup(unsigned short port) {
    udp.begin(WiFi.localIP(), port);
    log_e("UDP set up");
}

void EspUDPServer::loop() {
    udp.beginPacket(IPAddress(255, 255, 255, 255), 5506);
    JsonDocument doc;
    doc["ip"] = WiFi.localIP();
    doc["mac"] = WiFi.macAddress();
    String interfaceInfo;
    serializeJson(doc, interfaceInfo);
    udp.printf(interfaceInfo.c_str());
    udp.endPacket();
}

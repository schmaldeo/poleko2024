#include <WiFiUdp.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "Sensor.h"
#include <ESP32TimerInterrupt.hpp>

#pragma once

class EspUDPServer {
    public:
    EspUDPServer();
    void setup(unsigned short port = 5506);
    void loop();
    void sendPacket();
    static bool timerHandle(void *_);
    
    private:
    volatile bool timerFlag;
    WiFiUDP udp;
    // need to do this because no callback signature in timer library accepts parameters
    static EspUDPServer* instance;
};
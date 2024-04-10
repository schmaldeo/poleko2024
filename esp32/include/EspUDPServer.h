#include <WiFiUdp.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "Sensor.h"

#pragma once

class EspUDPServer {
    public:
    EspUDPServer();
    void setup(unsigned short port = 5506);
    void loop();
    
    private:
    WiFiUDP udp;
};
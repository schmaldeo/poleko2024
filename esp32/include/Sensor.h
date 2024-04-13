#include <Arduino.h>
#include <utility>

#pragma once

class Sensor {
public:
    Sensor(int uartNr, int rxPin, int txPin);
    std::pair<float, float> getSensorData();
    String getJsonString();
private:
    HardwareSerial serial;
    String readSensorData();
    std::pair<float, float> processSensorData(String& sensorData);
};

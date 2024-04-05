#include <Arduino.h>
#include <utility>

#ifndef SENSOR_H
#define SENSOR_H

class Sensor {
public:
    Sensor(HardwareSerial& hardwareSerial);
    std::pair<float, float> getSensorData();
private:
    HardwareSerial& serial;
    String readSensorData();
    std::pair<float, float> processSensorData(String& sensorData);
};

#endif
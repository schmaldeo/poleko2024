#include <Arduino.h>
#include <utility>
#include <Sensor.h>

Sensor::Sensor(HardwareSerial& hardwareSerial) : serial(hardwareSerial) { }

/// @brief Gets data from the sensor
/// @return std::pair where the first item is the humidity and the second item is the temperature
std::pair<float, float> Sensor::getSensorData() {
    String rawSensorResponse = readSensorData();
    return processSensorData(rawSensorResponse);
}


/// @brief Reads data from the sensor
/// @return Unprocessed String returned by the sensor
String Sensor::readSensorData() {
    serial.println("{F99RDD}\r\n");

    char receivedChar;
    String receivedMessage = "";

    while (serial.available() > 0) {
        char receivedChar = serial.read();

        if (receivedChar == 0xD) {
            return receivedMessage;
        } else {
            receivedMessage += receivedChar;
        }
    }

    return receivedMessage;
}

/// @brief Processes raw String returned by the sensor
/// @param sensorData String returned by the sensor
/// @return std::pair where the first item is the humidity and the second item is the temperature
std::pair<float, float> Sensor::processSensorData(String& sensorData) {
    // length of the string is constant, doing a check to avoid processing empty strings that sometimes get passed
    if (sensorData.length() != 94) {
        return std::make_pair(0.0f, 0.0f);
    }
    char* data = new char[sensorData.length() + 1];
    strcpy(data, sensorData.c_str());

    char* split = strtok(data, ";");

    split = strtok(NULL, ";");

    float humidity = atof(split);

    for (int i = 0; i < 4; i++) {
        split = strtok(NULL, ";");
    }

    float temperature = atof(split);

    delete[] data;

    return std::make_pair(humidity, temperature);
}

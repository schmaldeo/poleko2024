#include <Arduino.h>
#include <utility>
#include <Sensor.h>
#include <ArduinoJson.h>

Sensor::Sensor(int uartNr, int rxPin, int txPin) : serial(HardwareSerial(uartNr)) {
    serial.begin(19200, SERIAL_8N1, rxPin, txPin);
}

/// @brief Gets data from the sensor
/// @return std::pair where the first item is the humidity and the second item is the temperature
std::pair<float, float> Sensor::getSensorData() {
    String rawSensorResponse = readSensorData();
    return processSensorData(rawSensorResponse);
}

/// @brief Gets data from the sensor and parses it into a JSON string
/// @return JSON string containing current sensor reading.
String Sensor::getJsonString() {
    auto data = getSensorData();
    JsonDocument doc;
    doc["humidity"] = data.first;
    doc["temperature"] = data.second;
    String stringified;
    serializeJson(doc, stringified);
    return stringified;
}

/// @brief Reads data from the sensor
/// @return Unprocessed String returned by the sensor
String Sensor::readSensorData() {
    serial.println("{F99RDD}\r\n");

    char receivedChar;
    String receivedMessage = "";

    // sometimes the sensor returns an empty string, so retries are there just to make sure you actually get something if its connected
    byte retries = 0;
    while (retries < 5) {
        while (serial.available() > 0) {
            char receivedChar = serial.read();

            if (receivedChar == 0xD) {
                return receivedMessage;
            } else {
                receivedMessage += receivedChar;
            }
        }
        retries += 1;
        delay(100);
    }

    return receivedMessage;
}

/// @brief Processes raw String returned by the sensor
/// @param sensorData String returned by the sensor
/// @return std::pair where the first item is the humidity and the second item is the temperature
std::pair<float, float> Sensor::processSensorData(String &sensorData) {
    // length of the string is constant, doing a check to avoid processing empty strings that sometimes get passed
    if (sensorData.length() != 94) {
        return std::make_pair(0.0f, 0.0f);
    }
    char *data = new char[sensorData.length() + 1];
    strcpy(data, sensorData.c_str());

    char *split = strtok(data, ";");

    split = strtok(NULL, ";");

    float humidity = atof(split);

    for (int i = 0; i < 4; i++) {
        split = strtok(NULL, ";");
    }

    float temperature = atof(split);

    delete[] data;

    return std::make_pair(humidity, temperature);
}

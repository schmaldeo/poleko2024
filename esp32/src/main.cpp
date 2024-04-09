#include <Arduino.h>
#include "Sensor.h"
#include <WiFiManager.h>
#include <WiFi.h>
#include <Preferences.h>
#include "WiFiHelpers.h"

// TODO add sensors by mac
// TODO network discovery
// ^ both might be doable with UDP broadcast
// TODO http endpoint, json in http and tcp

constexpr byte BOOT_BUTTON_PIN = 0;

HardwareSerial SensorSerial(2);
Sensor sensor(SensorSerial);
Preferences preferences;

bool prevButtonState = HIGH;


void setupSerial(HardwareSerial& sensorSerial, int rxPin, int txPin);

void setup() {
    setupWiFi();
    setupSerial(SensorSerial, 16, 17);
}

void loop() {
    auto buttonState = digitalRead(BOOT_BUTTON_PIN);
    if (buttonState == LOW && prevButtonState == HIGH) {
        setupIpSetup();
    }
    prevButtonState = buttonState;
    // TODO test if this works properly connection lost
    if (initialWiFiSetupOver == true && WiFi.status() != WL_CONNECTED) {
        digitalWrite(LED_PIN, LOW);
        setupIpSetup();
    }

    if (WiFi.status() == WL_CONNECTION_LOST) {
        // TODO start blinking

    }
    // delay(2000);


    // std::pair<float, float> data = sensor.getSensorData();
    // Serial.print("Humidity: ");
    // Serial.print(data.first);
    // Serial.print("\n");
    // Serial.print("Temperature: ");
    // Serial.print(data.second);
    // Serial.print("\n\n");
}


/// @brief Sets up UART communication
/// @param sensorSerial A reference to the HardwareSerial object related to the sensor
/// @param rxPin RxD pin the sensor is plugged into
/// @param txPin TxD pin the sensor is plugged into
void setupSerial(HardwareSerial& sensorSerial, int rxPin, int txPin) {
    // TODO move to Sensor, Serial0 initialisation should be optional
    Serial.begin(9600);
    sensorSerial.begin(19200, SERIAL_8N1, rxPin, txPin);
}

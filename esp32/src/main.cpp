#include <Arduino.h>
#include "Sensor.h"
#include <WiFiManager.h>

// TODO add sensors by mac
// TODO network discovery
// TODO http endpoint, json in http and tcp

HardwareSerial SensorSerial(2);
Sensor sensor(SensorSerial);

void setupSerial(HardwareSerial& sensorSerial, int rxPin, int txPin);

class IPAddressParameter : public WiFiManagerParameter {
public:
    IPAddressParameter(const char *id, const char *placeholder, IPAddress address)
        : WiFiManagerParameter("") {
        init(id, placeholder, address.toString().c_str(), 16, "", WFM_LABEL_BEFORE);
    }

    bool getValue(IPAddress &ip) {
        return ip.fromString(WiFiManagerParameter::getValue());
    }
};

class DefaultGatewayParameter : public WiFiManagerParameter {
public:
    DefaultGatewayParameter(const char *id, const char *placeholder, IPAddress address)
        : WiFiManagerParameter("") {
        init(id, placeholder, address.toString().c_str(), 16, "", WFM_LABEL_BEFORE);
    }

    bool getValue(IPAddress &ip) {
        return ip.fromString(WiFiManagerParameter::getValue());
    }
};

class SubnetMaskParameter : public WiFiManagerParameter {
public:
    SubnetMaskParameter(const char *id, const char *placeholder, IPAddress address)
        : WiFiManagerParameter("") {
        init(id, placeholder, address.toString().c_str(), 16, "", WFM_LABEL_BEFORE);
    }

    bool getValue(IPAddress &ip) {
        return ip.fromString(WiFiManagerParameter::getValue());
    }
};

struct Settings {
    uint32_t ip;
    uint32_t gateway;
    uint32_t subnetMask;
} settings;

void setup() {
    WiFiManager wm;
    wm.setCountry("PL");
    wm.resetSettings();
    IPAddress ip(settings.ip);
    IPAddress gateway(settings.gateway);
    IPAddress subnetMask(settings.subnetMask);
    IPAddressParameter ipParam("ip", "IP address", ip);
    DefaultGatewayParameter gatewayParam("gateway", "Default gateway", gateway);
    SubnetMaskParameter subnetParam("subnet", "Subnet mask", subnetMask);
    wm.addParameter(&ipParam);
    wm.addParameter(&gatewayParam);
    wm.addParameter(&subnetParam);
    std::vector<const char*> menu = {"wifi","param","info","restart","exit"};
    wm.setMenu(menu);
    wm.autoConnect();
    setupSerial(SensorSerial, 16, 17);
    // TODO open setup with boot button
    // TODO find out what storage this library uses by default and add the IP settings to it
    bool res;

    if(!res) {
        Serial.println("Failed to connect");
        wm.reboot();
    } 
    else {
        Serial.println("connected");
    }
}

void loop() {
    delay(2000);


    std::pair<float, float> data = sensor.getSensorData();
    Serial.print("Humidity: ");
    Serial.print(data.first);
    Serial.print("\n");
    Serial.print("Temperature: ");
    Serial.print(data.second);
    Serial.print("\n\n");
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

#include <Arduino.h>
#include "Sensor.h"
#include <WiFiManager.h>
#include <WiFi.h>
#include <Preferences.h>

// TODO add sensors by mac
// TODO network discovery
// ^ both might be doable with UDP broadcast
// TODO http endpoint, json in http and tcp

constexpr byte BOOT_BUTTON_PIN = 0;
constexpr byte LED_PIN = 2;

HardwareSerial SensorSerial(2);
Sensor sensor(SensorSerial);
Preferences preferences;

bool prevButtonState = HIGH;
bool initialWiFiSetupOver = false;

struct IpSettings {
    IPAddress ip;
    IPAddress subnetMask;
    IPAddress defaultGateway;
};

void setupSerial(HardwareSerial& sensorSerial, int rxPin, int txPin);
IpSettings getSavedIpSettings(Preferences& preferences);
void saveIpSettings(Preferences& preferences, IpSettings& settings);
void setupWiFi();
void setupIpSetup();

class IPAddressParameter : public WiFiManagerParameter {
public:
    IPAddressParameter(const char *id, const char *placeholder, IPAddress address)
        : WiFiManagerParameter("") {
        init(id, placeholder, address.toString().c_str(), 16, "", WFM_LABEL_BEFORE);
    }

    IPAddress getValue() {
        IPAddress ip;
        ip.fromString(WiFiManagerParameter::getValue());
        return ip;
    }

    bool isValid() {
        IPAddress ip;
        return ip.fromString(WiFiManagerParameter::getValue()) && ip != 0;
    }
};

class DefaultGatewayParameter : public WiFiManagerParameter {
public:
    DefaultGatewayParameter(const char *id, const char *placeholder, IPAddress address)
        : WiFiManagerParameter("") {
        init(id, placeholder, address.toString().c_str(), 16, "", WFM_LABEL_BEFORE);
    }

    IPAddress getValue() {
        IPAddress ip;
        ip.fromString(WiFiManagerParameter::getValue());
        return ip;
    }

    bool isValid() {
        IPAddress ip;
        return ip.fromString(WiFiManagerParameter::getValue()) && ip != 0;
    }
};

class SubnetMaskParameter : public WiFiManagerParameter {
public:
    SubnetMaskParameter(const char *id, const char *placeholder, IPAddress address)
        : WiFiManagerParameter("") {
        init(id, placeholder, address.toString().c_str(), 16, "", WFM_LABEL_BEFORE);
    }

    IPAddress getValue() {
        IPAddress ip;
        ip.fromString(WiFiManagerParameter::getValue());
        return ip;
    }

    bool isValid() {
        IPAddress ip;
        return ip.fromString(WiFiManagerParameter::getValue()) && ip != 0;
    }
};

void setup() {
    Preferences preferences;
    auto ipSettings = getSavedIpSettings(preferences);
    WiFi.config(ipSettings.ip, ipSettings.defaultGateway, ipSettings.subnetMask);
    pinMode(LED_PIN, OUTPUT);
    setupSerial(SensorSerial, 16, 17);
    WiFiManager wf;
    // wf.resetSettings();
    wf.setConnectTimeout(15);
    bool connected = wf.autoConnect();
    Serial.println(connected);
    initialWiFiSetupOver = true;
    if (WiFi.status() == WL_CONNECTED) {
        // LED indicating whether wifi is connected
        digitalWrite(LED_PIN, HIGH);
    } else {
        wf.reboot();
    }
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

void setupWiFi() {
}

void setupIpSetup() {
    Preferences preferences;
    auto prefSettings = getSavedIpSettings(preferences);

    // reset static IP in case user wants to switch to another network
    WiFi.config(0u, 0u, 0u);
    WiFiManager wm;
    wm.setCountry("PL");
    IPAddressParameter ipParam("ip", "IP address", prefSettings.ip);
    SubnetMaskParameter maskParam("subnet", "Subnet mask", prefSettings.subnetMask);
    DefaultGatewayParameter gatewayParam("gateway", "Default gateway", prefSettings.defaultGateway);
    wm.addParameter(&ipParam);
    wm.addParameter(&maskParam);
    wm.addParameter(&gatewayParam);
    std::vector<const char*> menu = {"wifi","param","info","restart","exit"};
    wm.setMenu(menu);
    bool connectedOrChangedWiFi = wm.startConfigPortal();

    // if the user used the portal to connect to connect to a different WiFi AP, set DHCP
    if (connectedOrChangedWiFi) {
        auto dhcpSettings = IpSettings{0u, 0u, 0u};
        saveIpSettings(preferences, dhcpSettings);
        return;
    }

    // check if:
    // 1. all are valid
    // 2. at least one is different from the one stored
    // then WiFi.config and preferences.put
    if (ipParam.isValid() && gatewayParam.isValid() && maskParam.isValid()) {
        IPAddress paramIp = ipParam.getValue();
        IPAddress paramGateway = gatewayParam.getValue();
        IPAddress paramMask = maskParam.getValue();
        if ((prefSettings.ip != paramIp) || (prefSettings.defaultGateway != paramGateway) || (prefSettings.subnetMask != paramMask)) {
            auto settings = IpSettings{paramIp, paramGateway, paramMask};
            saveIpSettings(preferences, settings);
            // FIXME doesnt persistently set static
            // wm.setSTAStaticIPConfig(paramIp, paramGateway, paramMask);
            WiFi.config(paramIp, paramGateway, paramMask);
        }
    }
}

IpSettings getSavedIpSettings(Preferences& preferences) {
    preferences.begin("ipSettings");
    auto prefIp = preferences.getString("ip", "");
    IPAddress ip;
    ip.fromString(prefIp);
    auto prefMask = preferences.getString("mask", "");
    IPAddress mask;
    mask.fromString(prefMask);
    auto prefGateway = preferences.getString("gateway", "");
    IPAddress gateway;
    gateway.fromString(prefGateway);
    return IpSettings{ip, mask, gateway};
}

void saveIpSettings(Preferences& preferences, IpSettings& settings) {
    preferences.begin("ipSettings");
    preferences.putString("ip", settings.ip.toString());
    preferences.putString("mask", settings.subnetMask.toString());
    preferences.putString("gateway", settings.defaultGateway.toString());
}

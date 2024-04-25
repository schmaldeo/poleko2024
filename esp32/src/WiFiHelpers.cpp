#include "WiFiHelpers.h"
#include <Arduino.h>
#include <WiFiManager.h>
#include <WiFi.h>
#include <Preferences.h>

bool initialWiFiSetupOver = false;

/// @brief Sets up sensor's WiFi connection. Gets saved IP preferences and tries to connect to a saved access point if there is such.
/// If it cannot connect to the saved AP, opens a configuration portal.
void setupWiFi() {
    auto ipSettings = getSavedIpSettings();
    WiFi.config(ipSettings.ip, ipSettings.defaultGateway, ipSettings.subnetMask);
    pinMode(LED_PIN, OUTPUT);
    WiFiManager wm;
    wm.setCountry("PL");
    wm.setConnectTimeout(15);
    bool connected = wm.autoConnect();
    if (WiFi.status() == WL_CONNECTED) {
        // LED indicating whether wifi is connected
        digitalWrite(LED_PIN, HIGH);
    } else {
        wm.reboot();
    }
}

/// @brief Sets up the network configuration portal on which you can change the current WiFi and network parameters.
void setupIpSetup() {
    auto prefSettings = getSavedIpSettings();

    WiFiManager wm;
    wm.setCountry("PL");
    wm.setConnectTimeout(15);
    IPAddressParameter ipParam("ip", "IP address", prefSettings.ip);
    IPAddressParameter maskParam("subnet", "Subnet mask", prefSettings.subnetMask);
    IPAddressParameter gatewayParam("gateway", "Default gateway", prefSettings.defaultGateway);
    wm.addParameter(&ipParam);
    wm.addParameter(&maskParam);
    wm.addParameter(&gatewayParam);
    std::vector<const char *> menu = {"wifi", "param", "info", "restart", "exit"};
    wm.setMenu(menu);
    bool connectedOrChangedWiFi = wm.startConfigPortal();

    // if the user got disconnected but connected again, put the connection LED back on
    if (connectedOrChangedWiFi) {
        digitalWrite(LED_PIN, HIGH);
    }

    // check if:
    // 1. all are valid
    // 2. at least one is different from the one stored
    // then WiFi.config and preferences.put
    // getValue() assures validity of the address (it's either valid or 0.0.0.0 in which case DHCP activates)
    IPAddress paramIp = ipParam.getValue();
    IPAddress paramGateway = gatewayParam.getValue();
    IPAddress paramMask = maskParam.getValue();
    if ((prefSettings.ip != paramIp) || (prefSettings.defaultGateway != paramGateway) ||
        (prefSettings.subnetMask != paramMask)) {
        auto settings = IpSettings{paramIp, paramMask, paramGateway};
        saveIpSettings(settings);
        WiFi.config(paramIp, paramGateway, paramMask);
        digitalWrite(LED_PIN, HIGH);
    }
}

/// @brief Gets network parameters saved in microcontroller's flash memory.
/// @return IpSettings struct containing network parameters.
IpSettings getSavedIpSettings() {
    Preferences preferences;
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

/// @brief Saves network parameters to microcontroller's flash memory.
/// @param settings Settings to save
void saveIpSettings(IpSettings &settings) {
    Preferences preferences;
    preferences.begin("ipSettings");
    preferences.putString("ip", settings.ip.toString());
    preferences.putString("mask", settings.subnetMask.toString());
    preferences.putString("gateway", settings.defaultGateway.toString());
}

IPAddressParameter::IPAddressParameter(const char *id, const char *placeholder, IPAddress address)
        : WiFiManagerParameter("") {
    init(id, placeholder, address.toString().c_str(), 16, "", WFM_LABEL_BEFORE);
}

/// @brief Gets IP address from the parameter.
/// @return IP entered in the input or IPAddress(0u), which can set DHCP up if all 3 parameters are set to it
IPAddress IPAddressParameter::getValue() {
    IPAddress ip;
    Serial.println(WiFiManagerParameter::getValue());
    if (isValid()) {
        ip.fromString(WiFiManagerParameter::getValue());
    } else {
        ip = IPAddress(0u);
    }
    return ip;
}

/// @brief Checks whether the entered IP address is valid
/// @return Boolean indicating whether the address is valid or not
bool IPAddressParameter::isValid() {
    IPAddress ip;
    return ip.fromString(WiFiManagerParameter::getValue());
}

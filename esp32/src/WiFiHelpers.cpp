#include "WiFiHelpers.h"
#include <Arduino.h>
#include <WiFiManager.h>
#include <WiFi.h>
#include <Preferences.h>

bool initialWiFiSetupOver = false;

void setupWiFi() {
    Preferences preferences;
    auto ipSettings = getSavedIpSettings(preferences);
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

void setupIpSetup() {
    Preferences preferences;
    auto prefSettings = getSavedIpSettings(preferences);

    WiFiManager wm;
    wm.setCountry("PL");
    wm.setConnectTimeout(15);
    IPAddressParameter ipParam("ip", "IP address", prefSettings.ip);
    IPAddressParameter maskParam("subnet", "Subnet mask", prefSettings.subnetMask);
    IPAddressParameter gatewayParam("gateway", "Default gateway", prefSettings.defaultGateway);
    wm.addParameter(&ipParam);
    wm.addParameter(&maskParam);
    wm.addParameter(&gatewayParam);
    std::vector<const char*> menu = {"wifi","param","info","restart","exit"};
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
    if ((prefSettings.ip != paramIp) || (prefSettings.defaultGateway != paramGateway) || (prefSettings.subnetMask != paramMask)) {
        auto settings = IpSettings{paramIp, paramMask, paramGateway};
        saveIpSettings(preferences, settings);
        WiFi.config(paramIp, paramGateway, paramMask);
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

IPAddressParameter::IPAddressParameter(const char *id, const char *placeholder, IPAddress address)
    : WiFiManagerParameter("") {
    init(id, placeholder, address.toString().c_str(), 16, "", WFM_LABEL_BEFORE);
}

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

bool IPAddressParameter::isValid() {
    IPAddress ip;
    return ip.fromString(WiFiManagerParameter::getValue());
}

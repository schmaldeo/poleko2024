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
    wm.resetSettings();
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

    // reset static IP in case user wants to switch to another network
    WiFi.config(0u, 0u, 0u);
    WiFiManager wm;
    wm.setCountry("PL");
    IPAddressParameter ipParam("ip", "IP address", prefSettings.ip);
    IPAddressParameter maskParam("subnet", "Subnet mask", prefSettings.subnetMask);
    IPAddressParameter gatewayParam("gateway", "Default gateway", prefSettings.defaultGateway);
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

IPAddressParameter::IPAddressParameter(const char *id, const char *placeholder, IPAddress address)
    : WiFiManagerParameter("") {
    init(id, placeholder, address.toString().c_str(), 16, "", WFM_LABEL_BEFORE);
}

IPAddress IPAddressParameter::getValue() {
    IPAddress ip;
    ip.fromString(WiFiManagerParameter::getValue());
    return ip;
}

bool IPAddressParameter::isValid() {
    IPAddress ip;
    return ip.fromString(WiFiManagerParameter::getValue()) && ip != 0;
}

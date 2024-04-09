#include <Arduino.h>
#include <WiFiManager.h>
#include <WiFi.h>
#include <Preferences.h>

#pragma once

constexpr byte LED_PIN = 2;

struct IpSettings {
    IPAddress ip;
    IPAddress subnetMask;
    IPAddress defaultGateway;
};

class IPAddressParameter : public WiFiManagerParameter {
public:
    IPAddressParameter(const char *id, const char *placeholder, IPAddress address);

    IPAddress getValue();
    bool isValid();
};

bool initialWiFiSetupOver;

void setupWiFi();
void setupIpSetup();
IpSettings getSavedIpSettings(Preferences& preferences);
void saveIpSettings(Preferences& preferences, IpSettings& settings);
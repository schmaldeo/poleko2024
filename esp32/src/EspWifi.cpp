#include <Arduino.h>
#include <EspWifi.h>
#include <Wifi.h>

EspWifi::EspWifi() { }

void EspWifi::connectOrStartAP(String stationSsid, String stationPassword, String apSsid, String apPassword, unsigned short timeout) {
    if (stationSsid != emptyString) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(stationSsid, stationPassword);

        int count = 0;
        while ((WiFi.status() != WL_CONNECTED || WiFi.status() != WL_CONNECT_FAILED) && count < timeout / 1000) {
            delay(1000);
            count++;
        }
    }

    if (WiFi.status() != WL_CONNECTED) {
        setupAP(apSsid, apPassword);
    }
}

void EspWifi::setupAP(String ssid, String password) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);
    // TODO setup webserver
}
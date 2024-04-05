#include <Arduino.h>

class EspWifi {
    public:
    EspWifi();
    void connectOrStartAP(String stationSsid, String stationPassword, String apSsid, String apPassword = emptyString, unsigned short timeout = 5000);
    void setupAP(String ssid, String password);
};
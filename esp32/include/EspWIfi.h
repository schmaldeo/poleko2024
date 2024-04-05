#include <Arduino.h>

class EspWifi {
    public:
    EspWifi();
    void connectOrStartAP(String stationSsid = emptyString, String stationPassword = emptyString, String apSsid = emptyString, String apPassword = emptyString, unsigned short timeout = 5000);
    void setupAP(String ssid, String password);
};
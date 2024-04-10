#include <WiFi.h>
#include "Sensor.h"

class NewHTTPServer {
    public:
    NewHTTPServer(Sensor& sensor);
    void setup();
    void stop();
    void loop();

    private:
    WiFiServer server;
    Sensor& sensor;
};
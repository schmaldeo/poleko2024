#include <WiFi.h>
#include "Sensor.h"

class HTTPServer {
    public:
    HTTPServer(Sensor& sensor);
    void setup();
    void stop();
    void loop();

    private:
    WiFiServer server;
    Sensor& sensor;
};
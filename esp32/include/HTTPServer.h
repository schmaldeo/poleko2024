#include <WiFi.h>
#include "Sensor.h"

class HTTPServer {
    public:
    HTTPServer(Sensor& sensor);
    void setup();
    void stop();
    void loop();

    private:
    bool started;
    bool stopped;
    WiFiServer server;
    Sensor& sensor;
};
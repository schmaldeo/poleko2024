#include <ESPAsyncWebServer.h>
#include <Sensor.h>

#pragma once

class HTTPServer {
    public:
    HTTPServer(Sensor& sensor, unsigned short port = 80);
    void begin();

    private:
    AsyncWebServer* server;
    Sensor& sensor;
    class RequestHandler : public AsyncWebHandler {
        public:
        RequestHandler(Sensor& sensor) : sensor(sensor) { }
        virtual ~RequestHandler() {}
        bool canHandle(AsyncWebServerRequest *request);
        void handleRequest(AsyncWebServerRequest *request);

        private:
        Sensor& sensor;
    };
};
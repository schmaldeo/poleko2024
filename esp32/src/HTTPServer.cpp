#include "HTTPServer.h"

HTTPServer::HTTPServer(Sensor& sensor, unsigned short port) : 
sensor(sensor)
, server(new AsyncWebServer(port))
, port(port) { }

void HTTPServer::begin() {
    if (stoppedOnce) {
        server = new AsyncWebServer(port);
    }
    server->addHandler(new RequestHandler(sensor)).setFilter(ON_STA_FILTER);
    server->begin();
    log_e("HTTP set up");
}

// FIXME broken really badly
void HTTPServer::stop() {
    server->end();
    delete server;
    stoppedOnce = true;
}

bool HTTPServer::RequestHandler::canHandle(AsyncWebServerRequest *request) {
    return true;
}

void HTTPServer::RequestHandler::handleRequest(AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->print(sensor.getJsonString());
    request->send(response);
}
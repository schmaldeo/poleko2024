#include "HTTPServer.h"

HTTPServer::HTTPServer(Sensor& sensor, unsigned short port) : sensor(sensor), server(new AsyncWebServer(port)) { }

void HTTPServer::begin() {
    server->addHandler(new RequestHandler(sensor)).setFilter(ON_STA_FILTER);
    server->begin();
}

bool HTTPServer::RequestHandler::canHandle(AsyncWebServerRequest *request) {
    return true;
}

void HTTPServer::RequestHandler::handleRequest(AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->print(sensor.getJsonString());
    request->send(response);
}
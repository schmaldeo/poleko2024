#include "HTTPServer.h"
#include <ArduinoJson.h>

HTTPServer::HTTPServer(Sensor &sensor) : server(WiFiServer(80)), sensor(sensor) {}

void HTTPServer::setup() {
    if (started) {
        return;
    }
    server.begin();
    started = true;
    stopped = false;
    log_e("HTTP set up");
}

void HTTPServer::stop() {
    if (stopped) {
        return;
    }
    server.stop();
    stopped = true;
    started = false;
    log_e("HTTP stopped");
}

void HTTPServer::loop() {
    if (stopped) {
        return;
    }
    auto client = server.accept();

    if (client) {
        log_e("Client connected");
        String currentLine = "";
        while (client.connected()) {
            if (client.available()) {
                char c = client.read();
                if (c == '\n') {
                    // if the current line is blank, you got two newline characters in a row.
                    // that's the end of the client HTTP request, so send a response:
                    if (currentLine.length() == 0) {
                        // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
                        // and a content-type so the client knows what's coming, then a blank line:
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:application/json");
                        client.println();

                        // the content of the HTTP response follows the header:
                        auto sensorData = sensor.getSensorData();
                        JsonDocument doc;
                        doc["humidity"] = sensorData.first;
                        doc["temperature"] = sensorData.second;
                        doc["rssi"] = WiFi.RSSI();
                        String serialized;
                        serializeJson(doc, serialized);
                        client.print(serialized);

                        // The HTTP response ends with another blank line:
                        client.println();
                        break;
                    } else {
                        currentLine = "";
                    }
                } else if (c != '\r') {
                    currentLine += c;
                }
            }
        }
        client.stop();
        log_e("Client disconnected");
    }
}
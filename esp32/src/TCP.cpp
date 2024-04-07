#include "TCP.h"
#include <sstream>

// TODO field thats an array for timers, on disconnect or timeout remove the timer, request interval change either through tcp or http

// TODO client adds the interval on connection

TCP::TCP(unsigned short port) : server(new AsyncServer(port)) { }

void TCP::setup(Sensor& sensor) {
    server->onClient(&handleClient, static_cast<void*>(&sensor));
    server->begin();
    Serial.println("TCP set up");
}

void TCP::handleClient(void *arg, AsyncClient *client) {
    Sensor* sensor = static_cast<Sensor*>(arg);
    Serial.print("new client has been connected to server, ip:");
    Serial.println(client->remoteIP().toString().c_str());
    clientConnectedHandler(client, sensor);
    client->onData(&handleData, nullptr);
    client->onError(&handleError, nullptr);
    client->onDisconnect(&handleDisconnect, nullptr);
    client->onTimeout(&handleTimeout, nullptr);
}

// FIXME infinite loop
void TCP::clientConnectedHandler(AsyncClient *client, Sensor *sensor) {
    // while (true) {
        if (client->connected()) {
            std::pair<float, float> data = sensor->getSensorData();
            std::stringstream fmt;
            fmt << data.first << "," << data.second;
            String stringified = fmt.str().c_str();
            client->add(stringified.c_str(), strlen(stringified.c_str()));
            client->send();
        }
        // delay(1500);
    // }
}

void TCP::handleData(void *arg, AsyncClient *client, void *data, size_t len) {
    Serial.println("data");
}

void TCP::handleError(void *arg, AsyncClient *client, int8_t error) {
    Serial.println("error");
}

void TCP::handleDisconnect(void *arg, AsyncClient *client) {
    Serial.println("disconnect");
}

void TCP::handleTimeout(void *arg, AsyncClient *client, uint32_t time) {
    Serial.println("timeout");
}
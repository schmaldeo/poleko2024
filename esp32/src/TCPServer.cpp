#include "TCPServer.h"
#include <sstream>
#include <ESP32TimerInterrupt.h>
#include <ArduinoJson.h>

// TODO client adds the interval on connection

volatile bool timerFlag = false;
TCPServer* TCPServer::instance = nullptr;

TCPServer::TCPServer(Sensor& sensor, unsigned short port) : server(new AsyncServer(port)), sensor(sensor), timer(ESP32Timer(1)) {
    // TODO implement singleton
    instance = this;
}

TCPServer::~TCPServer() {
    delete server;
}

void TCPServer::setup() {
    server->onClient(&handleClient, static_cast<void*>(&sensor));
    server->begin();
    log_e("TCP set up");
}

void TCPServer::handleClient(void *arg, AsyncClient *client) {
    Sensor* sensor = static_cast<Sensor*>(arg);
    log_e("new client has been connected to server, ip: %s", client->remoteIP().toString());

    if (instance->clients.size() == 0) {
        instance->timer.attachInterrupt(0.5, timerHandle);
    }

    instance->clients.push_back(client);

    client->onData(&handleData, nullptr);
    client->onError(&handleError, nullptr);
    client->onDisconnect(&handleDisconnect, nullptr);
    client->onTimeout(&handleTimeout, nullptr);
}

void TCPServer::loop() {
    if (timerFlag) {
        timerFlag = false;
        sendDataToClient();
    }
}


bool TCPServer::sendDataToClient() {
    for (auto client : instance->clients) {
        if (client->connected()) {
            auto str = instance->sensor.getJsonString();
            client->add(str.c_str(), strlen(str.c_str()));
            client->send();
        }
    }
    return true;
}

bool TCPServer::timerHandle(void *_) {
    timerFlag = true;
    return true;
}

void TCPServer::handleData(void *arg, AsyncClient *client, void *data, size_t len) {
    Serial.println("data");
}

void TCPServer::handleError(void *arg, AsyncClient *client, int8_t error) {
    Serial.println("error");
}

void TCPServer::handleDisconnect(void *arg, AsyncClient *client) {
    log_e("Client disconnected, IP: %s", client->remoteIP().toString());
    auto iterator = std::find(instance->clients.begin(), instance->clients.end(), client);
    if (iterator != instance->clients.end()) {
        instance->clients.erase(iterator);
    }

    if (instance->clients.size() == 0) {
        instance->timer.detachInterrupt();
    }
}

void TCPServer::handleTimeout(void *arg, AsyncClient *client, uint32_t time) {
    Serial.println("timeout");
    log_e("Client timed out, IP: %s", client->remoteIP().toString());
    auto iterator = std::find(instance->clients.begin(), instance->clients.end(), client);
    if (iterator != instance->clients.end()) {
        instance->clients.erase(iterator);
    }

    if (instance->clients.size() == 0) {
        instance->timer.detachInterrupt();
    }
}
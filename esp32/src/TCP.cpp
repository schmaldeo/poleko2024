#include "TCP.h"
#include <sstream>
#include <ESP32TimerInterrupt.h>

// TODO field thats an vector for timers, on disconnect or timeout remove the timer, request interval change either through tcp or http

// TODO client adds the interval on connection

AsyncClient *globalClient = nullptr;
Sensor *globalSensor = nullptr;
volatile bool timerFlag = false;

TCP::TCP(unsigned short port) : server(new AsyncServer(port)) { }

TCP::~TCP() {
    delete server;
}

void TCP::setup(Sensor& sensor) {
    server->onClient(&handleClient, static_cast<void*>(&sensor));
    server->begin();
    Serial.println("TCP set up");
}

void TCP::handleClient(void *arg, AsyncClient *client) {
    Sensor* sensor = static_cast<Sensor*>(arg);
    Serial.print("new client has been connected to server, ip:");
    Serial.println(client->remoteIP().toString().c_str());

    globalClient = client;
    globalSensor = sensor;

    ESP32Timer timer(0);
    timer.attachInterrupt(0.5, timerHandle);

    client->onData(&handleData, nullptr);
    client->onError(&handleError, nullptr);
    client->onDisconnect(&handleDisconnect, nullptr);
    client->onTimeout(&handleTimeout, nullptr);
}

void TCP::loop() {
    if (timerFlag) {
        timerFlag = false;
        sendDataToClient();
    }
}


bool TCP::sendDataToClient() {
    if (globalClient->connected()) {
        std::pair<float, float> data = globalSensor->getSensorData();
        std::stringstream fmt;
        fmt << data.first << "," << data.second;
        auto str = fmt.str();
        globalClient->add(str.c_str(), strlen(str.c_str()));
        globalClient->send();
    }
    return true;
}

bool TCP::timerHandle(void *_) {
    timerFlag = true;
    return true;
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
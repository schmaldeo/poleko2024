#include "TCP.h"
#include <sstream>
#include <ESP32TimerInterrupt.h>

// TODO field thats an vector for timers, on disconnect or timeout remove the timer, request interval change either through tcp or http

// TODO client adds the interval on connection

AsyncClient *globalClient = nullptr;
Sensor *globalSensor = nullptr;
volatile bool timerFlag = false;

void TCP::setup(unsigned short port, Sensor& sensor) {
    // TODO free
    auto server = new AsyncServer(port);
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

    // clientConnectedHandler(client, sensor);
    ESP32Timer timer(0);
    timer.attachInterrupt(0.5, handel);

    // auto timer = timerBegin(0, 200, true);
    // timerAttachInterrupt(timer, timerHandle, false);
    // timerStart(timer);
    client->onData(&handleData, nullptr);
    client->onError(&handleError, nullptr);
    client->onDisconnect(&handleDisconnect, nullptr);
    client->onTimeout(&handleTimeout, nullptr);
}

void TCP::loop() {
    if (timerFlag) {
        timerFlag = false;
        timerHandle(nullptr);
    }
}


bool TCP::timerHandle(void* lol) {
    if (globalClient->connected()) {
        std::pair<float, float> data = globalSensor->getSensorData();
        std::stringstream fmt;
        fmt << data.first << "," << data.second;
        String stringified = fmt.str().c_str();
        globalClient->add(stringified.c_str(), strlen(stringified.c_str()));
        globalClient->send();
    }
    return true;
}

bool handel(void* lol) {
    timerFlag = true;
    return true;
}

// void TCP::clientConnectedHandler(AsyncClient *client, Sensor *sensor) {
//     if (client->connected()) {
//         std::pair<float, float> data = sensor->getSensorData();
//         std::stringstream fmt;
//         fmt << data.first << "," << data.second;
//         String stringified = fmt.str().c_str();
//         client->add(stringified.c_str(), strlen(stringified.c_str()));
//         client->send();
//     }
// }

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
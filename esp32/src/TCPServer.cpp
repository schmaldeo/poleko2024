#include "TCPServer.h"
#include <sstream>
#include <ESP32TimerInterrupt.h>
#include <ArduinoJson.h>

// TODO field thats an vector for timers, on disconnect or timeout remove the timer, request interval change either through tcp or http

// TODO client adds the interval on connection

// TODO figure out how to deal with the timer callback in case more connections are handled

AsyncClient *globalClient = nullptr;
Sensor *globalSensor = nullptr;
volatile bool timerFlag = false;

TCPServer::TCPServer(unsigned short port) : server(new AsyncServer(port)) { }

TCPServer::~TCPServer() {
    delete server;
}

void TCPServer::setup(Sensor& sensor) {
    server->onClient(&handleClient, static_cast<void*>(&sensor));
    server->begin();
    Serial.println("TCP set up");
}

void TCPServer::handleClient(void *arg, AsyncClient *client) {
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

void TCPServer::loop() {
    if (timerFlag) {
        timerFlag = false;
        sendDataToClient();
    }
}


bool TCPServer::sendDataToClient() {
    if (globalClient->connected()) {
        std::pair<float, float> data = globalSensor->getSensorData();
        JsonDocument doc;
        doc["humidity"] = data.first;
        doc["temperature"] = data.second;
        String json;
        serializeJson(doc, json);
        globalClient->add(json.c_str(), strlen(json.c_str()));
        globalClient->send();
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
    Serial.println("disconnect");
}

void TCPServer::handleTimeout(void *arg, AsyncClient *client, uint32_t time) {
    Serial.println("timeout");
}
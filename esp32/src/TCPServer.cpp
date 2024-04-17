#include "TCPServer.h"
#include <sstream>
#include <ESP32TimerInterrupt.h>
#include <ArduinoJson.h>
#include <driver/timer.h>
#include <WiFi.h>

volatile bool timerFlag = false;
TCPServer *TCPServer::instance = nullptr;

TCPServer::TCPServer(Sensor &sensor, unsigned short port) :
        server(new AsyncServer(port)), sensor(sensor), timer(ESP32Timer(1)), port(port) {
    // TODO implement singleton
    instance = this;
}

TCPServer::~TCPServer() {
    for (auto client: clients) {
        delete client;
    }
}

void TCPServer::setup() {
    if (started) {
        return;
    }
    if (stopped) {
        server = std::make_unique<AsyncServer>(port);
        stopped = false;
    }
    server->onClient(&handleClient, nullptr);
    server->begin();
    started = true;
    log_e("TCP set up");
}

void TCPServer::stop() {
    if (stopped) {
        return;
    }
    server = nullptr;
    for (auto client: clients) {
        delete client;
    }
    clients.clear();
    if (interruptAttachedOnce) {
        timer.detachInterrupt();
    }
    stopped = true;
    started = false;
    log_e("TCP stopped");
}

void TCPServer::handleClient(void *arg, AsyncClient *client) {
    log_e("new client has been connected to server, ip: %s", client->remoteIP().toString());

    if (instance->clients.size() == 0) {
        if (!instance->interruptAttachedOnce) {
            instance->timer.attachInterrupt(0.5, timerHandle);
            instance->interruptAttachedOnce = true;
        } else {
            instance->timer.reattachInterrupt();
        }
    }

    instance->clients.push_back(client);

    client->onData(&handleData, nullptr);
    client->onError(&handleError, nullptr);
    client->onDisconnect(&handleDisconnect, nullptr);
    client->onTimeout(&handleTimeout, nullptr);
}

void TCPServer::loop() {
    if (!instance->stopped) {
        if (timerFlag) {
            timerFlag = false;
            sendDataToClient();
        }
    }
}


bool TCPServer::sendDataToClient() {
    auto sensorData = instance->sensor.getSensorData();
    JsonDocument doc;
    doc["humidity"] = sensorData.first;
    doc["temperature"] = sensorData.second;
    doc["rssi"] = WiFi.RSSI();
    String serialized;
    serializeJson(doc, serialized);
    for (auto client: instance->clients) {
        if (client->connected()) {
            client->add(serialized.c_str(), strlen(serialized.c_str()));
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
    auto input = static_cast<char *>(data);
    JsonDocument doc;
    deserializeJson(doc, input);
    // no need to validate the json because even if it's invalid it doesnt throw an exception, 
    //the check for existence of a key in the document is the more important part
    float interval = doc["interval"];
    if (interval) {
        // need to use the esp-idf functions rather than the ones from the external library because those 
        // don't work properly when you try to change the interval
        // TODO save interval with Preferences
        timer_set_counter_value(TIMER_GROUP_0, TIMER_1, 0);
        timer_set_alarm_value(TIMER_GROUP_0, TIMER_1, interval * 1000000);
        timer_start(TIMER_GROUP_0, TIMER_1);
        log_e("Set TCP timer interval to %f", interval);
    }
}

void TCPServer::handleError(void *arg, AsyncClient *client, int8_t error) {
    Serial.println("error");
}

void TCPServer::handleDisconnect(void *arg, AsyncClient *client) {
    log_e("Client disconnected");
    auto iterator = std::find(instance->clients.begin(), instance->clients.end(), client);
    if (iterator != instance->clients.end()) {
        instance->clients.erase(iterator);
    }

    if (instance->clients.size() == 0) {
        instance->timer.detachInterrupt();
    }
}

void TCPServer::handleTimeout(void *arg, AsyncClient *client, uint32_t time) {
    log_e("Client timed out, IP: %s", client->remoteIP().toString());
    auto iterator = std::find(instance->clients.begin(), instance->clients.end(), client);
    if (iterator != instance->clients.end()) {
        instance->clients.erase(iterator);
    }

    if (instance->clients.size() == 0) {
        instance->timer.detachInterrupt();
    }
}

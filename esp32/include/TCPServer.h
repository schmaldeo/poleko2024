#include <HardwareSerial.h>
#include <AsyncTCP.h>
#include <Sensor.h>
#include <ESP32TimerInterrupt.hpp>
#include <memory>

#pragma once

class TCPServer {
public:
    TCPServer(Sensor &sensor, unsigned short port = 5505);

    ~TCPServer();

    TCPServer(const TCPServer &) = delete;

    TCPServer &operator=(const TCPServer &) = delete;

    void setup();

    void stop();

    static void loop();

private:
    std::shared_ptr <AsyncServer> server;
    Sensor &sensor;
    std::vector<AsyncClient *> clients;
    ESP32Timer timer;
    bool interruptAttachedOnce;
    bool started = false;
    bool stopped = false;
    unsigned short port;
    static TCPServer *instance;

    static bool timerHandle(void *_);

    static bool sendDataToClient();

    static void handleClient(void *arg, AsyncClient *client);

    static void handleData(void *arg, AsyncClient *client, void *data, size_t len);

    static void handleError(void *arg, AsyncClient *client, int8_t error);

    static void handleDisconnect(void *arg, AsyncClient *client);

    static void handleTimeout(void *arg, AsyncClient *client, uint32_t time);
};
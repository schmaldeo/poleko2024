#include <HardwareSerial.h>
#include <AsyncTCP.h>
#include <Sensor.h>
#include <ESP32TimerInterrupt.hpp>

#pragma once

class TCP {
    public:
    TCP(unsigned short port);
    ~TCP();
    void setup(Sensor& sensor);
    static void loop();
    private:
    AsyncServer* server;
    static bool handel(void* lol);
    static bool timerHandle(void *lol);
    static void handleClient(void *arg, AsyncClient *client);
    static void handleData(void *arg, AsyncClient *client, void *data, size_t len);
    static void handleError(void *arg, AsyncClient *client, int8_t error);
    static void handleDisconnect(void *arg, AsyncClient *client);
    static void handleTimeout(void *arg, AsyncClient *client, uint32_t time);
};
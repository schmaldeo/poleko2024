#include <HardwareSerial.h>
#include <AsyncTCP.h>
#include <Sensor.h>
#include <Ticker.h>

#pragma once

class TCP {
    public:
    TCP(unsigned short port);
    void setup(Sensor& sensor);
    private:
    AsyncServer* server;
    static void clientConnectedHandler(AsyncClient *client, Sensor *sensor);
    static void handleClient(void *arg, AsyncClient *client);
    static void handleData(void *arg, AsyncClient *client, void *data, size_t len);
    static void handleError(void *arg, AsyncClient *client, int8_t error);
    static void handleDisconnect(void *arg, AsyncClient *client);
    static void handleTimeout(void *arg, AsyncClient *client, uint32_t time);
};
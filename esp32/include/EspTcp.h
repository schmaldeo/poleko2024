#include <HardwareSerial.h>
#include <AsyncTCP.h>

#ifndef ESPTCP_H
#define ESPTCP_H

class EspTcp {
    public:
    EspTcp(unsigned short port);
    void setup();
    private:
    AsyncServer* server;
    static void handleClient(void *arg, AsyncClient *client);
    static void handleData(void *arg, AsyncClient *client, void *data, size_t len);
    static void handleError(void *arg, AsyncClient *client, int8_t error);
    static void handleDisconnect(void *arg, AsyncClient *client);
    static void handleTimeout(void *arg, AsyncClient *client, uint32_t time);
};

#endif
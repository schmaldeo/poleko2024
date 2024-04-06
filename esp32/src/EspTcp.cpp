#include "EspTcp.h"

EspTcp::EspTcp(unsigned short port) : server(new AsyncServer(port)) { }

void EspTcp::setup() {
    server->onClient(&handleClient, server);
    server->begin();
    Serial.println("TCP set up");
}

void EspTcp::handleClient(void *arg, AsyncClient *client) {
    Serial.print("\n new client has been connected to server, ip:");
    Serial.println(client->remoteIP().toString().c_str());
    client->onData(&handleData, NULL);
    client->onError(&handleError, NULL);
    client->onDisconnect(&handleDisconnect, NULL);
    client->onTimeout(&handleTimeout, NULL);
}

void EspTcp::handleData(void *arg, AsyncClient *client, void *data, size_t len) {
    Serial.println("data");
}

void EspTcp::handleError(void *arg, AsyncClient *client, int8_t error) {
    Serial.println("error");
}

void EspTcp::handleDisconnect(void *arg, AsyncClient *client) {
    Serial.println("disconnect");
}

void EspTcp::handleTimeout(void *arg, AsyncClient *client, uint32_t time) {
    Serial.println("timeout");
}
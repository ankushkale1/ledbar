#ifndef WEBSOCKET_LOGGER_H
#define WEBSOCKET_LOGGER_H

#include <Arduino.h>
#include <WebSocketsServer.h>

class WebsocketLogger : public Print {
public:
    WebsocketLogger(WebSocketsServer& server);
    void begin();
    void loop();
    virtual size_t write(uint8_t);
    virtual size_t write(const uint8_t *buffer, size_t size);

private:
    WebSocketsServer& _webSocket;
    void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
};

#endif

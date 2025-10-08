#include "WebsocketLogger.h"
#include <ArduinoLog.h>

WebsocketLogger::WebsocketLogger(WebSocketsServer &server)
    : _webSocket(server) {}

void WebsocketLogger::begin()
{
    _webSocket.onEvent([this](uint8_t num, WStype_t type, uint8_t *payload, size_t length)
                       { webSocketEvent(num, type, payload, length); });
}

void WebsocketLogger::loop()
{
    _webSocket.loop();
}

size_t WebsocketLogger::write(uint8_t character)
{
    _webSocket.broadcastTXT(&character, 1);
    return Serial.write(character);
}

size_t WebsocketLogger::write(const uint8_t *buffer, size_t size)
{
    _webSocket.broadcastTXT(buffer, size);
    return Serial.write(buffer, size);
}

void WebsocketLogger::webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
    switch (type)
    {
    case WStype_DISCONNECTED:
        Log.info("[WebSocket] Client #%u disconnected.\n", num);
        break;
    case WStype_CONNECTED:
    {
        IPAddress ip = _webSocket.remoteIP(num);
        Log.info("[WebSocket] Client #%u connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
    }
    break;
    case WStype_TEXT:
        // Not expecting any text from clients, but you could handle it here
        break;
    case WStype_BIN:
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
        break;
    }
}

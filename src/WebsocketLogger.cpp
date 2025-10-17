#include "WebsocketLogger.h"
#include <ArduinoLog.h>

WebsocketLogger::WebsocketLogger(WebSocketsServer &server)
    : _webSocket(server), _bufferIndex(0) {}

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
    if (_bufferIndex < LOG_BUFFER_SIZE - 1)
    {
        _buffer[_bufferIndex++] = character;
        if (character == '\n')
        {
            flush();
        }
    }
    else
    {
        flush();
        _buffer[_bufferIndex++] = character;
    }
    return Serial.write(character);
}

size_t WebsocketLogger::write(const uint8_t *buffer, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        write(buffer[i]);
    }
    return size;
}

void WebsocketLogger::flush()
{
    if (_bufferIndex > 0)
    {
        _buffer[_bufferIndex] = '\0'; // Null-terminate the string
        _webSocket.broadcastTXT((uint8_t *)_buffer, _bufferIndex);
        _bufferIndex = 0;
    }
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
    case WStype_PING:
        // Handle ping by sending a pong (automatically done by WebSockets library)
        break;
    case WStype_PONG:
        // Handle pong response (can be used for connection health monitoring)
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

//
// Created by Mike Mitterer on 28.08.18.
//

#ifndef ESP32_SOCKETHANDLER_H
#define ESP32_SOCKETHANDLER_H

#include <AsyncWebSocket.h>

class SocketHandler {
    enum class MessageType { Text, Binary };

public:

    void onEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type,  void* arg, uint8_t* data, size_t len);

private:
    void feedback(AsyncWebSocketClient *client, MessageType type);

};

#endif //ESP32_SOCKETHANDLER_H

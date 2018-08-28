//
// Created by Mike Mitterer on 28.08.18.
//

#include "SocketHandler.h"

#include <sstream>

#include <ArduinoLog.h>
#include <ArduinoJson.h>

/**
 * Wird von AsyncWebSocket als callback bei "onEvent" aufgerufen.
 *
 * Usage:
 *      ...
 *      // sh - ist im Prinzip der this-Pointer der zuvor definierten Variable
 *      // SocketHandler sh;
 *      //
 *      auto callback = std::bind(&SocketHandler::onEvent,sh,_1,_2,_3,_4,_5,_6);
 *      ws.onEvent(callback);
 *
 * @param server
 * @param client
 * @param type
 * @param arg
 * @param data
 * @param len
 */
void SocketHandler::onEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type,
                            void* arg,
                            uint8_t* data,
                            size_t len) {

    using ::SocketHandler;


    //client connected
    if (type == WS_EVT_CONNECT) {
        StaticJsonBuffer<200> jsonBuffer;
        JsonObject& root = jsonBuffer.createObject();
        String message;

        Log.notice(F("ws[%s][%d] connect" CR), server->url(), client->id());

        root["message"] = "Hello Client " + String(client->id()) + " :)";
        root.printTo(message);

        client->text(message);

        // if ((!client->status()) == WS_CONNECTED) {
        //    client->ping();
        // }
    }
    //client disconnected
    else if (type == WS_EVT_DISCONNECT) {
        Log.notice(F("ws[%s][%d] disconnect" CR), server->url(), client->id());

    } else if (type == WS_EVT_ERROR) {
        //error was received from the other end
        Log.notice(F("ws[%s][%d] error(%d): %s" CR),
        server->url(), client->id(), *((uint16_t *) arg), (char *) data);

    } else if (type == WS_EVT_PONG) {
        //pong message was received (in response to a ping request maybe)
        Log.notice(F("ws[%s][%u] pong[%u]: %s" CR),
        server->url(), client->id(), len, (len) ? (char *) data : "");

    } else if (type == WS_EVT_DATA) {
        //data packet
        auto *info = (AwsFrameInfo *) arg;

        if (info->final && info->index == 0 && info->len == len) {
            //the whole message is in a single frame and we got all of it's data
            Log.notice(F("ws[%s][%d] %s-message[%l]: " CR), server->url(), client->id(),
                    (info->opcode == WS_TEXT) ? "text" : "binary",
                    static_cast<unsigned long long>(info->len));

            if (info->opcode == WS_TEXT) {
                data[len] = 0;
                Log.notice(F("- %s" CR), (char *) data);
            } else {
                for (size_t i = 0; i < info->len; i++) {
                    Log.notice(F("%02x "), data[i]);
                }
                Log.notice(F("" CR));
            }

            feedback(client,info->message_opcode == WS_TEXT ? MessageType::Text : MessageType ::Binary);

        } else {
            //message is comprised of multiple frames or the frame is split into multiple packets
            if (info->index == 0) {
                if (info->num == 0) {
                    Log.notice(F("ws[%s][%d] %s-message start" CR),
                    server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
                }

                Log.notice(F("ws[%s][%d] frame[%d] start[%l]" CR),
                server->url(), client->id(), info->num,
                        static_cast<unsigned long long>(info->len));
            }

            Log.notice(F("ws[%s][%d] frame[%d] %s[%l - %l]: " CR),
            server->url(), client->id(),
                    info->num,
                    (info->message_opcode == WS_TEXT) ? "text" : "binary",
                    static_cast<unsigned long long>(info->index),
                    static_cast<unsigned long long>(info->index + len));

            if (info->message_opcode == WS_TEXT) {
                data[len] = 0;
                Log.notice(F("+ %s" CR), (char *) data);

            } else {
                for (size_t i = 0; i < len; i++) {
                    Log.notice(F("%02x "), data[i]);
                }
                Log.notice(F("-" CR));
            }

            if ((info->index + len) == info->len || info->len == 0) {
                Log.notice(F("ws[%s][%d] frame[%d] end[%l]" CR),
                server->url(), client->id(), info->num,
                        static_cast<unsigned long long>(info->len));

                if (info->final) {
                    Log.notice(F("ws[%s][%d] %s-message end" CR),
                    server->url(), client->id(),
                            (info->message_opcode == WS_TEXT) ? "text" : "binary");

                    feedback(client,info->message_opcode == WS_TEXT ? MessageType::Text : MessageType ::Binary);
                }
            }
        }
    }

}

/**
 * Default feedback an den Client.
 *
 * @param client
 * @param type Unterscheidet zwischen Text und Binär
 */
void SocketHandler::feedback(AsyncWebSocketClient *client, SocketHandler::MessageType type) {
    using ::SocketHandler;

    String message;
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();

    if (type == MessageType::Text) {
        root["message"] = "I got your text message";
        root.printTo(message);
        client->text(message);
    }
    else {
        root["message"] = "I got your binary message";
        root.printTo(message);
        client->binary(message);
    }
}

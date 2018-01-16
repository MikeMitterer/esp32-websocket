/**
 * Hello-World ESP32.
 * Blinkt eine LED und ermöglicht ein OAT update
 *
 * Wenn das OAT funktioniere soll muss im platformio.ini der Upload-Port gesetzt sein!
 *
 * Mehr:
 *      https://diyprojects.io/arduinoota-esp32-wi-fi-ota-wireless-update-arduino-ide/
 *
 * OTA mit platformio:
 *      http://docs.platformio.org/en/latest/platforms/espressif8266.html#over-the-air-ota-update
 *      
 */
#include "stdafx.h"
#include "config.h"
#include "ota.h"

#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>

const gpio_num_t internalLED = GPIO_NUM_2;

// Internal network I (192.168.0.0/24)
const std::string ssid{"MikeMitterer-LS"};
const std::string password{WebSocket_PASSWORD_LS};

static const uint8_t PORT = 80;
static const uint8_t MAX_RETRIES = 80;

AsyncWebServer server(PORT);
AsyncWebSocket ws("/ws"); // access at ws://[esp ip]/ws
AsyncEventSource events("/events"); // event source (Server-Sent events)
WebSocketsServer webSocket = WebSocketsServer(81);

const char* http_username = "admin";
const char* http_password = "admin";

//flag to use from web update to reboot the ESP
bool shouldReboot = false;

void onRequest(AsyncWebServerRequest* request) {
    //Handle Unknown Request
    request->send(404);
}

void onBody(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    //Handle body
}

void onUpload(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final) {
    //Handle upload
}

void onEvent(AsyncWebSocket* server,
             AsyncWebSocketClient* client,
             AwsEventType type,
             void* arg,
             uint8_t* data,
             size_t len) {

    if (type == WS_EVT_CONNECT) {
        //client connected
        //Log.notice(F("ws[%s][%d] connect" CR), server->url(), client->id());
        client->printf("Hello Client %u :)", client->id());
        if (!client->status() == WS_CONNECTED) {
            client->ping();
        }

    } else if (type == WS_EVT_DISCONNECT) {
        //client disconnected
        //Log.notice(F("ws[%s][%d] disconnect" CR), server->url(), client->id());

    } else if (type == WS_EVT_ERROR) {
        //error was received from the other end
        //Log.notice(F("ws[%s][%d] error(%d): %s" CR), server->url(), client->id(), *((uint16_t*) arg), (char*) data);

    } else if (type == WS_EVT_PONG) {
        //pong message was received (in response to a ping request maybe)
        //Log.notice(F("ws[%s][%u] pong[%u]: %s" CR), server->url(), client->id(), len, (len) ? (char*) data : "");

    } else if (type == WS_EVT_DATA) {
        //data packet
        auto* info = (AwsFrameInfo*) arg;
        if (info->final && info->index == 0 && info->len == len) {
            //the whole message is in a single frame and we got all of it's data
//            Log.notice(F("ws[%s][%u] %s-message[%llu]: "), server->url(), client->id(),
//                       (info->opcode == WS_TEXT) ? "text" : "binary", info->len);

            if (info->opcode == WS_TEXT) {
                data[len] = 0;
//                Log.notice(F("%s" CR), (char*) data);
            } else {
                for (size_t i = 0; i < info->len; i++) {
  //                  Log.notice(F("%02x "), data[i]);
                }
    //            Log.notice(F("" CR));
            }
            if (info->opcode == WS_TEXT) {
                client->text("I got your text message");
            }
            else {
                client->binary("I got your binary message");
            }

        } else {
            //message is comprised of multiple frames or the frame is split into multiple packets
            if (info->index == 0) {
                if (info->num == 0) {
//                    Log.notice(F("ws[%s][%u] %s-message start" CR), server->url(), client->id(),
//                               (info->message_opcode == WS_TEXT) ? "text" : "binary");
                }

//                Log.notice(F("ws[%s][%u] frame[%u] start[%llu]" CR), server->url(), client->id(), info->num, info->len);
            }

//            Log.notice(F("ws[%s][%u] frame[%u] %s[%llu - %llu]: "), server->url(), client->id(), info->num,
//                       (info->message_opcode == WS_TEXT) ? "text" : "binary", info->index, info->index + len);

            if (info->message_opcode == WS_TEXT) {
                data[len] = 0;
//-                Log.notice(F("%s" CR), (char*) data);
            } else {
                for (size_t i = 0; i < len; i++) {
//                    Log.notice(F("%02x "), data[i]);
                }
                //+Log.notice(F("" CR));
            }

            if ((info->index + len) == info->len) {
//                Log.notice(F("ws[%s][%u] frame[%u] end[%llu]"
//                                     CR), server->url(), client->id(), info->num, info->len);
                if (info->final) {
//                    Log.notice(F("ws[%s][%u] %s-message end"
//                                         CR), server->url(), client->id(),
//                               (info->message_opcode == WS_TEXT) ? "text" : "binary");

                    if (info->message_opcode == WS_TEXT)
                        client->text("I got your text message");
                    else
                        client->binary("I got your binary message");
                }
            }
        }
    }

}

void setup() {
    Serial.begin(115200);
    Log.begin(LOG_LEVEL_NOTICE, &Serial);

    // initialize digital pin LED_BUILTIN as an output.
    pinMode(internalLED, OUTPUT);

    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.println("Booting!\n");

    int retry = 0;
    WiFi.mode(WIFI_STA);
    while (WiFi.status() != WL_CONNECTED && retry < MAX_RETRIES) {
        digitalWrite(internalLED, HIGH);
        delay(100);
        digitalWrite(internalLED, LOW);
        Serial.print(".");
        retry++;
    }

    Serial.println("");
    if (retry >= MAX_RETRIES) {
        Serial.println("Connection Failed! Rebooting...");
        delay(5000);
        ESP.restart();
    }

    digitalWrite(internalLED, HIGH);
    Serial.println("IP: ");
    Serial.println(WiFi.localIP());

    initOTA();
    ArduinoOTA.begin();

    // attach AsyncWebSocket
    ws.onEvent(onEvent);
    server.addHandler(&ws);

    // respond to GET requests on URL /heap
    server.on("/heap", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(200, "text/plain", String(ESP.getFreeHeap()));
    });

    // attach AsyncEventSource
    server.addHandler(&events);

    // Catch-All Handlers
    // Any request that can not find a Handler that canHandle it
    // ends in the callbacks below.
    server.onNotFound(onRequest);
    server.onFileUpload(onUpload);
    server.onRequestBody(onBody);

    server.begin();
    Serial.println("Ready!");
}

void loop() {
    ArduinoOTA.handle();
}
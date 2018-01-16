/**
 * WebSocket-Server
 * Blinkt eine LED und ermöglicht ein OAT update
 *
 * Wenn das OAT funktioniere soll muss im platformio.ini der Upload-Port gesetzt sein!
 *
 * Die Library ESP82Websocket wurde von 'https://github.com/morrissinger/ESP8266-Websocket'
 * kopiert - siehe Beschreibung weiter unten
 *
 * Mehr:
 *      https://techtutorialsx.com/2017/11/03/esp32-arduino-websocket-server/
 *
 * Variante mit JSON-Parsing
 *      https://techtutorialsx.com/2017/11/05/esp32-arduino-websocket-server-receiving-and-parsing-json-content/
 *      
 * OTA mit platformio:
 *      http://docs.platformio.org/en/latest/platforms/espressif8266.html#over-the-air-ota-update
 *      
 */
#include "stdafx.h"
#include "config.h"
#include "ota.h"

const gpio_num_t internalLED = GPIO_NUM_2;

WiFiServer server(80);
WebSocketServer webSocketServer;

static const uint8_t MAX_RETRIES = 80;

// Internal network I (192.168.0.0/24)
const std::string ssid{"MikeMitterer-LS"};
const std::string password{WebSocket_PASSWORD_LS};

uint32_t clientID{0};

void setup() {
    Serial.begin(115200);

    // initialize digital pin LED_BUILTIN as an output.
    pinMode(internalLED, OUTPUT);

    Serial.println("Starting Webserver...");

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());

    uint8_t retries{0};
    while (WiFi.status() != WL_CONNECTED && retries < MAX_RETRIES) {
        digitalWrite(internalLED,HIGH);
        delay(100);
        digitalWrite(internalLED,LOW);
        Serial.print(".");
        retries++;
    }
    Serial.println("");

    if(retries >= MAX_RETRIES) {
        Serial.println("Connection Failed! Rebooting...");
        delay(5000);
        ESP.restart();
    }

    digitalWrite(internalLED,HIGH);

    initOTA();
    ArduinoOTA.begin();

    Serial.println("Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    server.begin();
    delay(100);
}

void loop() {
    ArduinoOTA.handle();
    WiFiClient client = server.available();

    if (client.connected() && webSocketServer.handshake(client)) {

        String data;

        while (client.connected()) {

            data = webSocketServer.getData();

            if (data.length() > 0) {
                data = data + " ID: " + clientID;
                Serial.println(data);
                
                webSocketServer.sendData(data);
            }

            delay(10); // Delay needed for receiving the data correctly
        }

        Serial.println("The client disconnected");
        clientID++;

        delay(100);
    }

    delay(100);
}



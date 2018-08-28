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
 * Debug:
 * > https://www.esp32.com/viewtopic.php?t=263
 * > http://esp-idf.readthedocs.io/en/latest/get-started/idf-monitor.html#launch-gdb-for-gdbstub
 * 
 *      /Users/mikemitterer/.platformio/packages/toolchain-xtensa32/bin/xtensa-esp32-elf-gdb ./.pioenvs/nodemcu-32s/firmware.elf -b 115200 -ex 'target remote /dev/cu.SLAB_USBtoUART'
 *      /Users/mikemitterer/.platformio/packages/toolchain-xtensa32/bin/xtensa-esp32-elf-gdb ./.pioenvs/nodemcu-32s/firmware.elf
 *      /Users/mikemitterer/.platformio/packages/toolchain-xtensa32/bin/xtensa-esp32-elf-addr2line -pfiaC -e ./.pioenvs/nodemcu-32s/firmware.elf
 */
#include "stdafx.h"
#include "config.h"
#include "ota.h"
#include "socket/SocketHandler.h"

#include <functional>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Stepper.h>
#include <WiFi.h>
#include "SSD1306.h"
#include <ArduinoJson.h>

using namespace std::placeholders;

#ifdef BOARD_LOLIN32PRO
const gpio_num_t internalLED = GPIO_NUM_5;
#else
const gpio_num_t internalLED = GPIO_NUM_2;
#endif

// Internal network I (192.168.0.0/24)
#define USE_LINKSYS_WLAN

#ifdef USE_LINKSYS_WLAN
const std::string ssid{"MikeMitterer-Asus"};            // NOLINT(cert-err58-cpp)
const std::string password{WebSocket_PASSWORD_ASUS};    // NOLINT(cert-err58-cpp)
#else
const std::string ssid{ "MikeMitterer-TPL" };
const std::string password{ WebSocket_PASSWORD_TPL };
#endif

static const uint8_t PORT = 80;
static const uint8_t MAX_RETRIES = 80;

AsyncWebServer server(PORT);                                               // NOLINT(cert-err58-cpp)
AsyncWebSocket ws("/ws"); // access at ws://[esp ip]/ws                    // NOLINT(cert-err58-cpp)
AsyncEventSource events("/events"); // event source (Server-Sent events)   // NOLINT(cert-err58-cpp)

// Pins
const uint8_t STEP_PIN = 2;
const uint8_t DIRECTION_PIN = 15;

enum class Direction {
    Up, Down
} direction{Direction::Up};

enum class States {
    Error, Idle,
    MotorOn, MotorOff
};

/// Per default fährt die Türe hoch und initialisiert sich.
States currentState = States::MotorOff;

const int STEPS_PER_REVOLUTION = 200;
// create an instance of the stepper class, specifying
// the number of steps of the motor and the pins it's
// attached to
Stepper stepper(STEPS_PER_REVOLUTION, DIRECTION_PIN, STEP_PIN);           // NOLINT(cert-err58-cpp)

// OLED
const uint8_t SDA_PIN = 21;
const uint8_t SCL_PIN = 22;
const uint8_t OLED_ADDRESS = 0x3c;

SSD1306 display(OLED_ADDRESS, SDA_PIN, SCL_PIN);                         // NOLINT(cert-err58-cpp)

const char *http_username = "admin";
const char *http_password = "admin";

//flag to use from web update to reboot the ESP
bool shouldReboot = false;


void onRequest(AsyncWebServerRequest *request) {
    //Handle Unknown Request
    request->send(404);
}

void onBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    //Handle body
}

void onUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    //Handle upload
}


SocketHandler sh;

void setup() {
    Serial.begin(115200);
    Log.begin(LOG_LEVEL_NOTICE, &Serial);

    // OLED setup
    display.init();
    display.setFont(ArialMT_Plain_10);
    display.clear();

    // initialize digital pin LED_BUILTIN as an output.
    pinMode(internalLED, OUTPUT);

    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.print("\nBooting ");

    display.drawString(0, 0, "Booting...");
    display.display();

    int retry = 0;
    WiFiClass::mode(WIFI_STA);
    while (WiFiClass::status() != WL_CONNECTED && retry < MAX_RETRIES) {
        digitalWrite(internalLED, HIGH);
        delay(100);
        digitalWrite(internalLED, LOW);
        Serial.print(".");
        retry++;
    }

    Serial.println("");
    if (retry >= MAX_RETRIES) {
        Serial.println("Connection Failed! Rebooting...");

        display.clear();
        display.drawString(0, 0, "Connection Failed!");
        display.drawString(0, 11, "Rebooting...");
        display.display();

        delay(5000);
        ESP.restart();
    }

    digitalWrite(internalLED, HIGH);
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    display.clear();
    delay(10);
    display.drawString(0, 0, String("IP:  ") + WiFi.localIP().toString());
    display.drawString(0, 11, String("MAC: ") + WiFi.macAddress());
    display.display();

    initOTA();
    ArduinoOTA.begin();

    // Basic Stepper settings
    pinMode(STEP_PIN, OUTPUT);
    pinMode(DIRECTION_PIN, OUTPUT);

    digitalWrite(STEP_PIN, LOW);
    digitalWrite(DIRECTION_PIN, direction == Direction::Up ? static_cast<uint8_t>(HIGH)
                                                           : static_cast<uint8_t>(LOW));

    // set the speed of the motor to 30 RPMs
    stepper.setSpeed(3000);

    // attach AsyncWebSocket
    //ws.onEvent(onEvent);

    auto callback = std::bind(&SocketHandler::onEvent,sh,_1,_2,_3,_4,_5,_6);
    ws.onEvent(callback);
    server.addHandler(&ws);

    // attach AsyncEventSource
    server.addHandler(&events);

    // Catch-All Handlers
    // Any request that can not find a Handler that canHandle it
    // ends in the callbacks below.
    server.onNotFound(onRequest);
    server.onFileUpload(onUpload);
    server.onRequestBody(onBody);

    // respond to GET requests on URL /heap
    server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", String(ESP.getFreeHeap()));
    });

    server.on("/motor/on", HTTP_GET, [](AsyncWebServerRequest *request) {
        currentState = States::MotorOn;
        request->send(200, "text/plain", "OK");
    });

    server.on("/motor/off", HTTP_GET, [](AsyncWebServerRequest *request) {
        currentState = States::MotorOff;
        request->send(200, "text/plain", "OK");
    });

    server.begin();
    Serial.println("Ready!");
}

void loop() {
    ArduinoOTA.handle();

    switch (currentState) {

        case States::MotorOn:

            digitalWrite(internalLED, LOW);
            stepper.step(250);

            //stepper.step(direction == Direction::Up ? 6400 : -6400);
            //stepper.step(direction == Direction::Up ? 50 : -50);
            break;

        case States::MotorOff:
            digitalWrite(internalLED, HIGH);
            break;

        case States::Idle:
            delay(10);
            break;
    }

}
#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <HX711.h>
#include <Wire.h>
#include <SPIFFS.h>
#include "config.h"
#include "vessel_manager.h"
#include "display_ui.h"
#include <AsyncWebSocket.h>
#include "wifi_credentials.h"

void setupWebServer();

HX711 scale;
VesselManager vesselManager;
DisplayUI *display;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
Preferences preferences;

volatile bool rotaryInterrupt = false;
volatile int rotaryDirection = 0;
volatile bool buttonPressed = false;

#define BUTTON_DEBOUNCE_DELAY 250
volatile unsigned long lastButtonPressTime = 0;

bool calibrationMode = false;
float knownWeight = 100.0;

void IRAM_ATTR rotaryISR_cw() {
    unsigned long currentTime = millis();
    if(!digitalRead(ROTARY_PIN_RIGHT) && (currentTime - lastButtonPressTime > BUTTON_DEBOUNCE_DELAY)) {
        rotaryDirection = 1;
        rotaryInterrupt = true;
    }
    lastButtonPressTime = currentTime;
}

void IRAM_ATTR rotaryISR_ccw() {
    unsigned long currentTime = millis();
    if(!digitalRead(ROTARY_PIN_LEFT) && (currentTime - lastButtonPressTime > BUTTON_DEBOUNCE_DELAY)) {
        rotaryDirection = -1;
        rotaryInterrupt = true;
    }
    lastButtonPressTime = currentTime;
}

void IRAM_ATTR buttonISR() {
    unsigned long currentTime = millis();
    if(!digitalRead(ROTARY_PIN_BUTTON) && (currentTime - lastButtonPressTime > BUTTON_DEBOUNCE_DELAY)) {
        buttonPressed = true;
    }
    lastButtonPressTime = currentTime;
}

struct WSClient {
    uint32_t id;
    bool updatesEnabled;
    bool inUse;
};

#define MAX_CLIENTS 10
WSClient wsClients[MAX_CLIENTS];
int numClients = 0;

WSClient* findClient(uint32_t id) {
    for (int i = 0; i < numClients; i++) {
        if (wsClients[i].id == id) {
            return &wsClients[i];
        }
    }
    return nullptr;
}

WSClient* addClient(uint32_t id) {
    if (numClients < MAX_CLIENTS) {
        wsClients[numClients].id = id;
        wsClients[numClients].updatesEnabled = false;
        wsClients[numClients].inUse = true;
        return &wsClients[numClients++];
    }
    return nullptr;
}

void removeClient(uint32_t id) {
    for (int i = 0; i < numClients; i++) {
        if (wsClients[i].id == id) {
            wsClients[i].inUse = false;
            wsClients[i].updatesEnabled = false;
            // Compact the array by moving active clients forward
            for (int j = i; j < numClients - 1; j++) {
                if (wsClients[j + 1].inUse) {
                    wsClients[j] = wsClients[j + 1];
                }
            }
            numClients--;
            break;
        }
    }
}

void broadcastStatus(const char* status, bool error = false) {
    StaticJsonDocument<200> doc;
    doc["status"] = status;
    if (error) {
        doc["error"] = true;
    }
    String response;
    serializeJson(doc, response);
    ws.textAll(response);
}

bool calibrateScale(float knownWeight) {
    if (knownWeight <= 0) {
        return false;
    }

    // First tare the scale
    scale.tare(20);  // Use 20 readings for better accuracy
    delay(1000);     // Give time for stability
    
    // Get multiple readings and check for stability
    long readings[5];
    for (int i = 0; i < 5; i++) {
        readings[i] = scale.read_average(10);
        delay(100);
    }
    
    // Check readings stability
    long avgReading = 0;
    long maxDiff = 0;
    for (int i = 0; i < 5; i++) {
        avgReading += readings[i];
        for (int j = i + 1; j < 5; j++) {
            long diff = abs(readings[i] - readings[j]);
            if (diff > maxDiff) maxDiff = diff;
        }
    }
    avgReading /= 5;
    
    // If readings are unstable (more than 1% variation), return false
    if (maxDiff > (avgReading * 0.01)) {
        return false;
    }
    
    // Calculate and set scale factor
    float scaleFactor = (float)avgReading / knownWeight;
    if (scaleFactor <= 0) {
        return false;
    }
    
    // Set and save the new scale factor
    scale.set_scale(scaleFactor);
    preferences.begin("scale", false);
    preferences.putFloat("factor", scaleFactor);
    preferences.end();
    
    Serial.printf("Calibrated scale - Raw: %ld, Weight: %.2f, Factor: %.2f\n", 
                 avgReading, knownWeight, scaleFactor);
    
    return true;
}

void setup() {
    Serial.begin(115200);
    Wire.begin(I2C_SDA, I2C_SCL);

    if(!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");
        return;
    }

    display = new DisplayUI();
    display->init();

#ifdef WIFI_SSID
    WiFi.mode(WIFI_STA);

#ifdef USE_STATIC_IP
    IPAddress local_ip;
    IPAddress gateway;
    IPAddress subnet;
    IPAddress dns1;
    IPAddress dns2;

    local_ip.fromString(STATIC_IP);
    gateway.fromString(STATIC_GATEWAY);
    subnet.fromString(STATIC_SUBNET);
    dns1.fromString(STATIC_DNS1);
    dns2.fromString(STATIC_DNS2);

    if (!WiFi.config(local_ip, gateway, subnet, dns1, dns2)) {
        Serial.println("Failed to configure static IP");
    }
#endif

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        char buf[32];
        sprintf(buf, "Connecting %d/20", attempts + 1);
        display->setWiFiStatus(buf);
        display->showWeight(0.0, nullptr);
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to WiFi");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        display->setWiFiStatus("Connected", WiFi.localIP().toString().c_str());
        display->showWeight(0.0, nullptr);
        delay(2000);
        display->clearWiFiStatus();
    } else {
        Serial.println("\nFailed to connect to WiFi");
        display->setWiFiStatus("Connection Failed");
        display->showWeight(0.0, nullptr);
        delay(3000);
        ESP.restart();
    }
#else
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
    display->setWiFiStatus("AP Mode", WiFi.softAPIP().toString().c_str());
#endif

    scale.begin(HX711_DATA_PIN, HX711_CLOCK_PIN);
    preferences.begin("scale", false);
    float scaleFactor = preferences.getFloat("factor", 1.0f);
    preferences.end();
    scale.set_scale(scaleFactor);
    scale.tare();

    pinMode(ROTARY_PIN_LEFT, INPUT_PULLUP);
    pinMode(ROTARY_PIN_RIGHT, INPUT_PULLUP);
    pinMode(ROTARY_PIN_BUTTON, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(ROTARY_PIN_LEFT), rotaryISR_ccw, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ROTARY_PIN_RIGHT), rotaryISR_cw, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ROTARY_PIN_BUTTON), buttonISR, CHANGE);

    setupWebServer();
    server.begin();
}

void loop() {
    static unsigned long lastUpdate = 0;
    static VesselConfig* currentVessel = nullptr;

    if (rotaryInterrupt) {
        if(rotaryDirection > 0) {
            Serial.print("CW ");
        } else {
            Serial.print("CCW ");
        }
        Serial.println("Rotary");
        display->handleRotary(rotaryDirection);
        rotaryInterrupt = false;
    }

    if (buttonPressed) {
        Serial.println("Button");
        display->handleButton();
        if (display->getMenuState() == MAIN_SCREEN) {
            currentVessel = vesselManager.getVessel(display->getSelectedVessel());
        }
        buttonPressed = false;
    }

    if (millis() - lastUpdate > 200) {
        float weight = scale.get_units();
        Serial.print("Weight: ");
        Serial.println(weight);
        display->showWeight(weight, currentVessel);
        lastUpdate = millis();
    }

    static unsigned long lastWebSocketUpdate = 0;
    if (millis() - lastWebSocketUpdate > 200) {
        if (ws.count() > 0) {
            float weight = scale.get_units();
            VesselConfig* currentVessel = nullptr;
            int selectedIndex = -1;

            if (display->getMenuState() == MAIN_SCREEN) {
                selectedIndex = display->getSelectedVessel();
                currentVessel = vesselManager.getVessel(selectedIndex);
            }

            StaticJsonDocument<200> doc;
            doc["weight"] = weight;
            if (currentVessel) {
                doc["selectedVessel"] = selectedIndex;
                doc["vesselWeight"] = currentVessel->vesselWeight;
                doc["spoolWeight"] = currentVessel->spoolWeight;
                doc["filamentWeight"] = weight - currentVessel->vesselWeight - currentVessel->spoolWeight;
            }

            String json;
            serializeJson(doc, json);

            for(int i = 0; i < numClients; i++) {
                if(wsClients[i].updatesEnabled) {
                    AsyncWebSocketClient * client = ws.client(wsClients[i].id);
                    if(client) client->text(json);
                }
            }
        }
        lastWebSocketUpdate = millis();
    }
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void handleWebSocketMessage(AsyncWebSocketClient *client, void *arg, uint8_t *data, size_t len);
void sendVesselList();

void handleWebSocketMessage(AsyncWebSocketClient *client, void *arg, uint8_t *data, size_t len) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        data[len] = 0;  // Ensure null termination
        Serial.printf("Received WebSocket message: %s\n", (char*)data);
        
        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, data, len);
        if (error) {
            Serial.printf("JSON Parse Error: %s\n", error.c_str());
            return;
        }

        const char* command = doc["command"];
        if (!command) {
            Serial.println("No command in message");
            return;
        }

        if (strcmp(command, "toggleUpdates") == 0) {
            WSClient* wsClient = findClient(client->id());
            if (wsClient) {
                wsClient->updatesEnabled = doc["enabled"];
                broadcastStatus(wsClient->updatesEnabled ? "Updates enabled" : "Updates disabled");
            }
            return;
        }

        if (strcmp(command, "selectVessel") == 0) {
            int index = doc["index"] | -1;
            if (index >= 0 && index < vesselManager.getVesselCount()) {
                display->setSelectedVessel(index);
                StaticJsonDocument<200> response;
                response["status"] = "Vessel selected";
                response["selectedVessel"] = index;
                String jsonResponse;
                serializeJson(response, jsonResponse);
                ws.textAll(jsonResponse);
            } else {
                broadcastStatus("Invalid vessel index", true);
            }
            return;
        }

        if (strcmp(command, "addVessel") == 0) {
            JsonObject vessel = doc["vessel"];
            if (!vessel.isNull()) {
                const char* name = vessel["name"];
                float vesselWeight = vessel["vesselWeight"];
                float spoolWeight = vessel["spoolWeight"];
                
                if (vesselManager.addVessel(name, vesselWeight, spoolWeight)) {
                    broadcastStatus("Vessel added");
                    sendVesselList();  // Update all clients
                } else {
                    broadcastStatus("Failed to add vessel", true);
                }
            }
            return;
        }

        if (strcmp(command, "updateVessel") == 0) {
            int index = doc["index"] | -1;
            JsonObject vessel = doc["vessel"];
            if (!vessel.isNull() && index >= 0) {
                const char* name = vessel["name"];
                float vesselWeight = vessel["vesselWeight"];
                float spoolWeight = vessel["spoolWeight"];
                
                if (vesselManager.updateVessel(index, name, vesselWeight, spoolWeight)) {
                    broadcastStatus("Vessel updated");
                    sendVesselList();  // Update all clients
                } else {
                    broadcastStatus("Failed to update vessel", true);
                }
            }
            return;
        }

        if (strcmp(command, "deleteVessel") == 0) {
            int index = doc["index"] | -1;
            if (index >= 0 && vesselManager.deleteVessel(index)) {
                broadcastStatus("Vessel deleted");
                sendVesselList();  // Update all clients
            } else {
                broadcastStatus("Failed to delete vessel", true);
            }
            return;
        }

        if (strcmp(command, "getVessels") == 0) {
            sendVesselList();
            return;
        }

        if (strcmp(command, "tare") == 0) {
            scale.tare(20);  // Use 20 readings for better accuracy
            broadcastStatus("Scale tared");
            return;
        }

        if (strcmp(command, "calibrate") == 0) {
            float knownWeight = doc["weight"] | 0.0f;
            if (knownWeight > 0) {
                if (calibrateScale(knownWeight)) {
                    broadcastStatus("Scale calibrated successfully");
                } else {
                    broadcastStatus("Calibration failed - unstable readings", true);
                }
            } else {
                broadcastStatus("Invalid calibration weight", true);
            }
            return;
        }

        Serial.println("Unknown command");
        broadcastStatus("Unknown command", true);
    }
}

void sendVesselList() {
    StaticJsonDocument<1024> doc;
    JsonArray vessels = doc.createNestedArray("vessels");
    
    for (int i = 0; i < vesselManager.getVesselCount(); i++) {
        VesselConfig* vessel = vesselManager.getVessel(i);
        if (vessel) {
            JsonObject v = vessels.createNestedObject();
            v["name"] = vessel->name;
            v["vesselWeight"] = vessel->vesselWeight;
            v["spoolWeight"] = vessel->spoolWeight;
        }
    }
    
    String json;
    serializeJson(doc, json);
    ws.textAll(json);
}

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
                     void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            addClient(client->id());
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            removeClient(client->id());
            break;
        case WS_EVT_DATA:
            handleWebSocketMessage(client, arg, data, len);
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

void setupWebServer() {
    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
    ws.onEvent(onWebSocketEvent);
    server.addHandler(&ws);
}

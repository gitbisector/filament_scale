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

// Function prototype for setupWebServer
void setupWebServer();

HX711 scale;
VesselManager vesselManager;
DisplayUI *display;
AsyncWebServer server(80);

volatile bool rotaryInterrupt = false;
volatile int rotaryDirection = 0;
volatile bool buttonPressed = false;

#define BUTTON_DEBOUNCE_DELAY 250  // debounce period in milliseconds
volatile unsigned long lastButtonPressTime = 0;

// Interrupt handlers for rotary encoder
void IRAM_ATTR rotaryISR_cw() {
    unsigned long currentTime = millis();
    if(!digitalRead(ROTARY_PIN_RIGHT) && (currentTime - lastButtonPressTime > BUTTON_DEBOUNCE_DELAY)) {
        rotaryInterrupt = true;
        rotaryDirection = -1;
    }
    lastButtonPressTime = currentTime;
}

void IRAM_ATTR rotaryISR_ccw() {
    unsigned long currentTime = millis();
    if(!digitalRead(ROTARY_PIN_LEFT) && (currentTime - lastButtonPressTime > BUTTON_DEBOUNCE_DELAY)) {
        rotaryInterrupt = true;
        rotaryDirection = 1;
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

void setup() {
    Serial.begin(115200);
    Wire.begin(I2C_SDA, I2C_SCL);
    
    // Initialize SPIFFS
    if(!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");
        return;
    }
    
    // Initialize HX711
    scale.begin(HX711_DATA_PIN, HX711_CLOCK_PIN);
    scale.set_scale(258.14); // Use calibration value here
    scale.tare();
    
    
    // Setup rotary encoder
    pinMode(ROTARY_PIN_LEFT, INPUT_PULLUP);
    pinMode(ROTARY_PIN_RIGHT, INPUT_PULLUP);
    pinMode(ROTARY_PIN_BUTTON, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(ROTARY_PIN_LEFT), rotaryISR_ccw, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ROTARY_PIN_RIGHT), rotaryISR_cw, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ROTARY_PIN_BUTTON), buttonISR, CHANGE);
    
    // Setup WiFi Access Point
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
    
    // Setup web server routes
    setupWebServer();
    // Initialize display
    display  = new DisplayUI();
    display->init();
}

void loop() {
    static unsigned long lastUpdate = 0;
    static VesselConfig* currentVessel = vesselManager.getVessel(0);
    
    // Handle rotary encoder
    if (rotaryInterrupt) {
        if(rotaryDirection > 0)
            Serial.print("CW ");
        else
            Serial.print("CCW ");
        Serial.println("Rotary");

        display->handleRotary(rotaryDirection);
        rotaryInterrupt = false;
    }
    
    if (buttonPressed) {
        Serial.println("Button");
        display->handleButton();
        buttonPressed = false;
    }
    
    // Update weight reading every 100ms
    if (millis() - lastUpdate > 200) {
        float weight = scale.get_units();
        Serial.print("Weight: ");
        Serial.println(weight);        
        display->showWeight(weight, currentVessel);
        lastUpdate = millis();
    }
}

void setupWebServer() {
    // Serve static files
    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
    
    // API endpoints
    server.on("/api/vessels", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        DynamicJsonDocument doc(1024);
        JsonArray array = doc.to<JsonArray>();
        
        for (int i = 0; i < vesselManager.getVesselCount(); i++) {
            VesselConfig* vessel = vesselManager.getVessel(i);
            if (vessel) {
                JsonObject obj = array.createNestedObject();
                obj["id"] = i;
                obj["name"] = vessel->name;
                obj["vesselWeight"] = vessel->vesselWeight;
                obj["spoolWeight"] = vessel->spoolWeight;
                obj["lastWeight"] = vessel->lastWeight;
                obj["lastUpdate"] = vessel->lastUpdate;
            }
        }
        
        serializeJson(doc, *response);
        request->send(response);
    });
    
    // Add new vessel
    AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler("/api/vessels", [](AsyncWebServerRequest *request, JsonVariant &json) {
        JsonObject jsonObj = json.as<JsonObject>();
        
        if (jsonObj.containsKey("name") && 
            jsonObj.containsKey("vesselWeight") && 
            jsonObj.containsKey("spoolWeight")) {
            
            const char* name = jsonObj["name"];
            float vesselWeight = jsonObj["vesselWeight"];
            float spoolWeight = jsonObj["spoolWeight"];
            
            bool success = vesselManager.addVessel(name, vesselWeight, spoolWeight);
            
            if (success) {
                request->send(200, "application/json", "{\"status\":\"success\"}");
            } else {
                request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Failed to add vessel\"}");
            }
        } else {
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing required fields\"}");
        }
    });
    server.addHandler(handler);
    
    // Update vessel
    server.on("/api/vessels/{id}", HTTP_PUT, [](AsyncWebServerRequest *request) {
        if (request->hasParam("id")) {
            int id = request->getParam("id")->value().toInt();
            // Handle vessel update
            request->send(200, "application/json", "{\"status\":\"success\"}");
        } else {
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing vessel ID\"}");
        }
    });
    
    // Delete vessel
    server.on("/api/vessels/{id}", HTTP_DELETE, [](AsyncWebServerRequest *request) {
        if (request->hasParam("id")) {
            int id = request->getParam("id")->value().toInt();
            bool success = vesselManager.deleteVessel(id);
            if (success) {
                request->send(200, "application/json", "{\"status\":\"success\"}");
            } else {
                request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Failed to delete vessel\"}");
            }
        } else {
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing vessel ID\"}");
        }
    });
    
    // Get current weight
    server.on("/api/weight", HTTP_GET, [](AsyncWebServerRequest *request) {
        float weight = scale.get_units();
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        DynamicJsonDocument doc(128);
        doc["weight"] = weight;
        serializeJson(doc, *response);
        request->send(response);
    });
    
    server.begin();
}

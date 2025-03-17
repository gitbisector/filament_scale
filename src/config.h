#pragma once

// Pin Definitions
#define ROTARY_PIN_RIGHT    0  // D0 - Left
#define ROTARY_PIN_BUTTON   1  // D1 - Push
#define ROTARY_PIN_LEFT     2  // D2 - Right

// HX711 pins
#define HX711_DATA_PIN   19
#define HX711_CLOCK_PIN  20

// I2C pins for OLED
#define I2C_SDA         22
#define I2C_SCL         23

// OLED Display settings
#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64
#define OLED_RESET     -1
#define SCREEN_ADDRESS  0x3C

// WiFi settings
#define WIFI_AP_SSID    "FilamentScale"
#define WIFI_AP_PASS    "scalewifi"

// Maximum number of vessel configurations
#define MAX_VESSELS     10

// Structure for vessel configuration
struct VesselConfig {
    char name[32];
    float vesselWeight;
    float spoolWeight;
};

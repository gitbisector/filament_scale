#pragma once
#include <U8g2lib.h>
#include "vessel_manager.h"

extern VesselManager vesselManager;
enum MenuState {
    MAIN_SCREEN,
    VESSEL_SELECT,
    CALIBRATION_VESSEL,
    CALIBRATION_SPOOL
};

#define ttype U8G2_SSD1306_128X64_NONAME_F_HW_I2C

class DisplayUI {
public:
    DisplayUI() 
    : display(ttype(U8G2_R2, /* reset=*/ U8X8_PIN_NONE, I2C_SCL, I2C_SDA)) {
        menuState = MAIN_SCREEN;
        selectedVessel = 0;
        calibrationStep = 0;
        wifiStatus[0] = '\0';
    }
    
    void init() {
        display.begin();
        display.clearBuffer();
        display.setFont(u8g2_font_7x14B_tr);
        display.setContrast(255);
        display.drawStr(0, 14, "Initializing...");
        display.sendBuffer();
    }
    
    void showWeight(float weight, const VesselConfig* vessel) {
        if (menuState != MAIN_SCREEN) return;
        
        display.clearBuffer();
        char buf[32];

        // Show WiFi status if it's set
        if (wifiStatus[0] != '\0') {
            display.drawStr(0, 14, wifiStatus);
            if (strstr(wifiStatus, "Connected") != nullptr) {
                sprintf(buf, "IP: %s", ipAddress);
                display.drawStr(0, 28, buf);
            }
            display.sendBuffer();
            return;
        }
        
        // Show normal weight display
        if (vessel) {
            display.drawStr(0, 14, vessel->name);
            display.drawStr(0, 20, "----------------");
            sprintf(buf, "Total: %.1f", weight);
            display.drawStr(0, 30, buf);
            sprintf(buf, "Filament: %.1f", weight - vessel->vesselWeight - vessel->spoolWeight);
            display.drawStr(0, 44, buf);
        } else {
            display.drawStr(0, 14, "No vessel selected");
            display.drawStr(0, 20, "----------------");
            sprintf(buf, "Weight: %.1f", weight);
            display.drawStr(0, 30, buf);
        }
        display.sendBuffer();
    }
    
    void handleRotary(int direction) {
        switch(menuState) {
            case VESSEL_SELECT:
                selectedVessel += direction;
                if (selectedVessel < 0) selectedVessel = vesselManager.getVesselCount() - 1;
                if (selectedVessel >= vesselManager.getVesselCount()) selectedVessel = 0;
                showVesselSelection();
                break;
                
            case CALIBRATION_VESSEL:
            case CALIBRATION_SPOOL:
                // Handle calibration adjustment
                break;
        }
    }
    
    void handleButton() {
        switch(menuState) {
            case MAIN_SCREEN:
                menuState = VESSEL_SELECT;
                showVesselSelection();
                break;
                
            case VESSEL_SELECT:
                if (vesselManager.getVessel(selectedVessel)) {
                    menuState = MAIN_SCREEN;
                    showWeight(0.0, vesselManager.getVessel(selectedVessel)); // Show selected vessel immediately
                }
                break;
                
            case CALIBRATION_VESSEL:
                if (calibrationStep == 0) {
                    // Save empty vessel weight
                    calibrationStep++;
                } else {
                    menuState = MAIN_SCREEN;
                    calibrationStep = 0;
                }
                break;
                
            case CALIBRATION_SPOOL:
                if (calibrationStep == 0) {
                    // Save empty spool weight
                    calibrationStep++;
                } else {
                    menuState = MAIN_SCREEN;
                    calibrationStep = 0;
                }
                break;
        }
    }
    
    void setWiFiStatus(const char* status, const char* ip = nullptr) {
        strncpy(wifiStatus, status, sizeof(wifiStatus) - 1);
        wifiStatus[sizeof(wifiStatus) - 1] = '\0';
        
        if (ip) {
            strncpy(ipAddress, ip, sizeof(ipAddress) - 1);
            ipAddress[sizeof(ipAddress) - 1] = '\0';
        } else {
            ipAddress[0] = '\0';
        }
    }
    
    void clearWiFiStatus() {
        wifiStatus[0] = '\0';
        ipAddress[0] = '\0';
    }

    // Add getter methods
    MenuState getMenuState() const { return menuState; }
    int getSelectedVessel() const { return selectedVessel; }

private:
    ttype display;
    MenuState menuState;
    int selectedVessel;
    int calibrationStep;
    char wifiStatus[32];
    char ipAddress[16];
    
    // Updated to use drawStr consistently instead of Print methods.
    void showVesselSelection() {
        display.clearBuffer();
        int y = 14; // starting y offset
        
        display.drawStr(0, y, "Select Vessel:");
        y += 6;
        display.drawStr(0, y, "----------------");
        y += 10;
        
        VesselConfig* vessel = vesselManager.getVessel(selectedVessel);
        if (vessel) {
            display.drawStr(0, y, vessel->name);
            y += 14;
            
            char buf[32];
            sprintf(buf, "Vessel: %.1f", vessel->vesselWeight);
            display.drawStr(0, y, buf);
            y += 14;
            
            sprintf(buf, "Spool: %.1f", vessel->spoolWeight);
            display.drawStr(0, y, buf);
        }
        display.sendBuffer();
    }
    
    // Updated calibration screen using drawStr.
    void showCalibration(float weight, const char* type) {
        display.clearBuffer();
        int y = 10;
        char buf[32];
        
        sprintf(buf, "Calibrating %s", type);
        display.drawStr(0, y, buf);
        y += 12;
        
        display.drawStr(0, y, "----------------");
        y += 12;
        
        sprintf(buf, "Weight: %.1f", weight);
        display.drawStr(0, y, buf);
        y += 12;
        
        display.drawStr(0, y, "Press to save");
        display.sendBuffer();
    }
};

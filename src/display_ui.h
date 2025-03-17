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
        ipAddress[0] = '\0';
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
        int y = 14;

        // Show WiFi status if it's set
        if (wifiStatus[0] != '\0') {
            display.setFont(u8g2_font_7x13_tr);  // Smaller font for status
            display.drawStr(0, y, wifiStatus);
            if (ipAddress[0] != '\0') {
                sprintf(buf, "IP: %s", ipAddress);
                y += 13;
                display.drawStr(0, y, buf);
            }
            y += 13;
        }

        // Show normal weight display
        if (vessel) {
            // Show vessel name
            display.setFont(u8g2_font_7x14B_tr);
            display.drawStr(0, y, vessel->name);
            y += 8;

            // Separator line
            for (int i = 0; i < 128; i += 2) {
                display.drawPixel(i, y);
            }
            y += 14;

            // Show total weight in large font
            display.setFont(u8g2_font_logisoso16_tr);
            sprintf(buf, "%.1fg", weight);
            display.drawStr(0, y, buf);
            y += 20;

            // Show filament weight
            display.setFont(u8g2_font_7x14_tr);
            float filamentWeight = weight - vessel->vesselWeight - vessel->spoolWeight;
            sprintf(buf, "Filament: %.1fg", filamentWeight);
            display.drawStr(0, y, buf);
        } else {
            // No vessel selected, show simple weight
            display.setFont(u8g2_font_logisoso16_tr);
            sprintf(buf, "%.1fg", weight);
            int strwidth = display.getStrWidth(buf);
            display.drawStr((128 - strwidth) / 2, 40, buf);  // Center the weight

            display.setFont(u8g2_font_7x14_tr);
            display.drawStr(0, 60, "No vessel selected");
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
                    showCalibration(0.0, "Vessel");
                } else {
                    menuState = MAIN_SCREEN;
                    calibrationStep = 0;
                }
                break;
                
            case CALIBRATION_SPOOL:
                if (calibrationStep == 0) {
                    // Save empty spool weight
                    calibrationStep++;
                    showCalibration(0.0, "Spool");
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
        display.clearBuffer();
        display.setFont(u8g2_font_7x14_tr);
        display.drawStr(0, 14, wifiStatus);
        if (ipAddress[0] != '\0') {
            char buf[32];
            sprintf(buf, "IP: %s", ipAddress);
            display.drawStr(0, 28, buf);
        }
        display.sendBuffer();
    }
    
    void clearWiFiStatus() {
        wifiStatus[0] = '\0';
        ipAddress[0] = '\0';
    }

    MenuState getMenuState() const { return menuState; }
    int getSelectedVessel() const { return selectedVessel; }

    void setSelectedVessel(int index) {
        if (index >= 0 && index < vesselManager.getVesselCount()) {
            selectedVessel = index;
            menuState = MAIN_SCREEN;
            VesselConfig* vessel = vesselManager.getVessel(selectedVessel);
            if (vessel) {
                showWeight(0.0, vessel);
            }
        }
    }

private:
    void showVesselSelection() {
        display.clearBuffer();
        display.setFont(u8g2_font_7x14B_tr);
        int y = 14;
        
        display.drawStr(0, y, "Select Vessel:");
        y += 6;

        // Separator line
        for (int i = 0; i < 128; i += 2) {
            display.drawPixel(i, y);
        }
        y += 10;
        
        VesselConfig* vessel = vesselManager.getVessel(selectedVessel);
        if (vessel) {
            display.drawStr(0, y, vessel->name);
            y += 14;
            
            char buf[32];
            display.setFont(u8g2_font_7x14_tr);
            sprintf(buf, "Vessel: %.1fg", vessel->vesselWeight);
            display.drawStr(0, y, buf);
            y += 14;
            
            sprintf(buf, "Spool: %.1fg", vessel->spoolWeight);
            display.drawStr(0, y, buf);
        }
        display.sendBuffer();
    }
    
    void showCalibration(float weight, const char* type) {
        display.clearBuffer();
        display.setFont(u8g2_font_7x14B_tr);
        int y = 14;
        
        char buf[32];
        sprintf(buf, "Calibrating %s", type);
        display.drawStr(0, y, buf);
        y += 6;
        
        // Separator line
        for (int i = 0; i < 128; i += 2) {
            display.drawPixel(i, y);
        }
        y += 12;
        
        display.setFont(u8g2_font_logisoso16_tr);
        sprintf(buf, "%.1fg", weight);
        int strwidth = display.getStrWidth(buf);
        display.drawStr((128 - strwidth) / 2, y, buf);  // Center the weight
        y += 20;
        
        display.setFont(u8g2_font_7x14_tr);
        display.drawStr(0, y, "Press to save");
        display.sendBuffer();
    }

    ttype display;
    MenuState menuState;
    int selectedVessel;
    int calibrationStep;
    char wifiStatus[32];
    char ipAddress[32];
};

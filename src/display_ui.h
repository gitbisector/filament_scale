#pragma once
#include <U8g2lib.h>
#include "vessel_manager.h"
#include "scale.h"

extern VesselManager* vesselManager;
extern Scale* scale;

enum MenuState {
    MAIN_SCREEN,
    VESSEL_SELECT,
    CALIBRATION_VESSEL,
    CALIBRATION_SPOOL,
    QUICK_ADD_VESSEL
};

#define ttype U8G2_SSD1306_128X64_NONAME_F_HW_I2C

class DisplayUI {
private:
    float quickAddWeight = 0.0f;
    int quickAddStep = 0;
    char tempVesselName[32];
public:
    DisplayUI() 
    : display(ttype(U8G2_R2, /* reset=*/ U8X8_PIN_NONE, I2C_SCL, I2C_SDA)) {
        menuState = MAIN_SCREEN;
        selectedVessel = vesselManager->getSelectedVessel();
        calibrationStep = 0;
        quickAddStep = 0;
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
            y += 2;

            // Separator line
            for (int i = 0; i < 128; i += 2) {
                display.drawPixel(i, y);
            }
            y += 20;

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
        int newSelection;
        switch(menuState) {
            case VESSEL_SELECT:
                // Allow -1 for "Quick Add" option
                newSelection = selectedVessel + direction;
                if (newSelection < -1) newSelection = vesselManager->getVesselCount() - 1;
                if (newSelection >= vesselManager->getVesselCount()) newSelection = -1;
                selectedVessel = newSelection;
                if (selectedVessel >= 0) {
                    vesselManager->setSelectedVessel(selectedVessel);
                }
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
                if (selectedVessel == -1) {
                    // No vessel selected, enter quick add mode
                    menuState = QUICK_ADD_VESSEL;
                    quickAddStep = 0;
                    showQuickAdd(0);
                } else if (vesselManager->getVessel(selectedVessel)) {
                    menuState = MAIN_SCREEN;
                    vesselManager->setSelectedVessel(selectedVessel); // Persist selection
                    showWeight(0.0, vesselManager->getVessel(selectedVessel)); // Show selected vessel immediately
                }
                break;
            case QUICK_ADD_VESSEL:
                handleQuickAddButton();
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
        if (index >= 0 && index < vesselManager->getVesselCount()) {
            selectedVessel = index;
            vesselManager->setSelectedVessel(index);
            menuState = MAIN_SCREEN;
            VesselConfig* vessel = vesselManager->getVessel(selectedVessel);
            if (vessel) {
                showWeight(0.0, vessel);
            }
        }
    }

    void handleQuickAddButton() {
        switch(quickAddStep) {
            case 0: // Empty vessel weight
                // Round to one decimal place
                quickAddWeight = roundf(scale->getWeight() * 10.0f) / 10.0f;
                quickAddStep++;
                showQuickAdd(quickAddWeight);
                break;
            case 1: // With full spool
                float fullWeight = roundf(scale->getWeight() * 10.0f) / 10.0f;
                float vesselWeight = quickAddWeight;
                float spoolWeight = roundf((fullWeight - vesselWeight - 1000.0f) * 10.0f) / 10.0f;
                // Generate a default name
                int vesselNum = vesselManager->getVesselCount() + 1;
                snprintf(tempVesselName, sizeof(tempVesselName), "Vessel %d", vesselNum);
                if (vesselManager->addVessel(tempVesselName, vesselWeight, spoolWeight)) {
                    selectedVessel = vesselManager->getVesselCount() - 1;
                    vesselManager->setSelectedVessel(selectedVessel);
                    menuState = MAIN_SCREEN;
                    showWeight(0.0, vesselManager->getVessel(selectedVessel));
                } else {
                    menuState = MAIN_SCREEN;
                    showWeight(0.0, nullptr);
                }
                break;
        }
    }

    void showQuickAdd(float weight) {
        display.clearBuffer();
        display.setFont(u8g2_font_7x14B_tr);
        char buf[32];
        int y = 14;
        if (quickAddStep == 0) {
            display.drawStr(0, y, "Quick Add Vessel");
            y += 20;
            display.setFont(u8g2_font_7x14_tr);
            display.drawStr(0, y, "Place empty vessel");
            y += 20;
            display.drawStr(0, y, "Press button when");
            y += 15;
            display.drawStr(0, y, "ready");
        } else {
            display.drawStr(0, y, "Add 1KG Spool");
            y += 20;
            display.setFont(u8g2_font_7x14_tr);
            sprintf(buf, "Vessel: %.1fg", weight);
            display.drawStr(0, y, buf);
            y += 20;
            display.drawStr(0, y, "Add full spool &");
            y += 15;
            display.drawStr(0, y, "press button");
        }

        display.sendBuffer();
    }

private:
    void showVesselSelection() {
        display.clearBuffer();
        display.setFont(u8g2_font_7x14B_tr);
        int y = 14;

        display.drawStr(0, y, "Select Vessel:");
        y += 3;

        // Separator line
        for (int i = 0; i < 128; i += 2) {
            display.drawPixel(i, y);
        }
        y += 13;

        if (selectedVessel == -1) {
            display.drawStr(0, y, "[Quick Add Vessel]");
        } else {
            VesselConfig* vessel = vesselManager->getVessel(selectedVessel);
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

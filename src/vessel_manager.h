#pragma once
#include <Preferences.h>
#include "config.h"

class VesselManager {
public:
    VesselManager() {
        vesselCount = 0;
        preferences.begin("vessels", false);
        loadFromPreferences();
    }
    
    bool addVessel(const char* name, float vesselWeight, float spoolWeight) {
        if (vesselCount >= MAX_VESSELS) return false;
        
        strncpy(vessels[vesselCount].name, name, sizeof(vessels[vesselCount].name) - 1);
        vessels[vesselCount].vesselWeight = vesselWeight;
        vessels[vesselCount].spoolWeight = spoolWeight;
        vessels[vesselCount].lastWeight = 0;
        vessels[vesselCount].lastUpdate = 0;
        
        vesselCount++;
        saveToPreferences();
        return true;
    }
    
    bool updateVessel(int index, const char* name, float vesselWeight, float spoolWeight) {
        if (index < 0 || index >= vesselCount) return false;
        
        strncpy(vessels[index].name, name, sizeof(vessels[index].name) - 1);
        vessels[index].vesselWeight = vesselWeight;
        vessels[index].spoolWeight = spoolWeight;
        
        saveToPreferences();
        return true;
    }
    
    bool deleteVessel(int index) {
        if (index < 0 || index >= vesselCount) return false;
        
        // Shift remaining vessels
        for (int i = index; i < vesselCount - 1; i++) {
            vessels[i] = vessels[i + 1];
        }
        vesselCount--;
        
        saveToPreferences();
        return true;
    }
    
    VesselConfig* getVessel(int index) {
        if (index < 0 || index >= vesselCount) return nullptr;
        return &vessels[index];
    }
    
    int getVesselCount() {
        return vesselCount;
    }
    
    void saveWeight(int index, float weight) {
        if (index < 0 || index >= vesselCount) return;
        
        vessels[index].lastWeight = weight;
        vessels[index].lastUpdate = millis();
        saveToPreferences();
    }
    
private:
    VesselConfig vessels[MAX_VESSELS];
    int vesselCount;
    Preferences preferences;
    
    void loadFromPreferences() {
        vesselCount = preferences.getInt("count", 0);
        for (int i = 0; i < vesselCount; i++) {
            char key[8];
            sprintf(key, "v%d", i);
            size_t len = preferences.getBytes(key, &vessels[i], sizeof(VesselConfig));
            if (len != sizeof(VesselConfig)) {
                vesselCount = i;
                break;
            }
        }
    }
    
    void saveToPreferences() {
        preferences.putInt("count", vesselCount);
        for (int i = 0; i < vesselCount; i++) {
            char key[8];
            sprintf(key, "v%d", i);
            preferences.putBytes(key, &vessels[i], sizeof(VesselConfig));
        }
    }
};

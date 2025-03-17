#pragma once
#include <Preferences.h>
#include "config.h"

class VesselManager {
public:
    VesselManager() : vesselCount(0), selectedVesselIndex(0) {
        // First open preferences in read/write mode
        if (!preferences.begin("vessels", false)) {
            Serial.println("Failed to initialize preferences");
            return;
        }

        // Load existing vessels
        loadFromPreferences();
    }

    ~VesselManager() {
        preferences.end();
    }

    bool addVessel(const char* name, float vesselWeight, float spoolWeight) {
        if (vesselCount >= MAX_VESSELS) return false;

        VesselConfig& vessel = vessels[vesselCount];
        strncpy(vessel.name, name, sizeof(vessel.name) - 1);
        vessel.name[sizeof(vessel.name) - 1] = '\0';
        vessel.vesselWeight = vesselWeight;
        vessel.spoolWeight = spoolWeight;
        vesselCount++;
        saveToPreferences();
        return true;
    }

    bool updateVessel(int index, const char* name, float vesselWeight, float spoolWeight) {
        if (index < 0 || index >= vesselCount) return false;

        VesselConfig& vessel = vessels[index];
        strncpy(vessel.name, name, sizeof(vessel.name) - 1);
        vessel.name[sizeof(vessel.name) - 1] = '\0';
        vessel.vesselWeight = vesselWeight;
        vessel.spoolWeight = spoolWeight;

        saveToPreferences();
        return true;
    }

    bool deleteVessel(int index) {
        if (index < 0 || index >= vesselCount) return false;

        // Shift remaining vessels left
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

    int getVesselCount() const {
        return vesselCount;
    }

    void setSelectedVessel(int index) {
        if (index >= 0 && index < vesselCount) {
            selectedVesselIndex = index;
            saveToPreferences();
        }
    }

    int getSelectedVessel() const {
        return selectedVesselIndex;
    }

private:
    void loadFromPreferences() {
        // Load vessel count and selection
        vesselCount = preferences.getInt("count", 0);
        selectedVesselIndex = preferences.getInt("selected", 0);

        Serial.printf("Loading %d vessels from preferences\n", vesselCount);

        // Ensure selected index is valid
        if (selectedVesselIndex >= vesselCount) {
            selectedVesselIndex = 0;
        }

        for (int i = 0; i < vesselCount; i++) {
            String prefix = "vessel" + String(i) + "_";
            preferences.getString((prefix + "name").c_str(), vessels[i].name, sizeof(vessels[i].name));
            vessels[i].vesselWeight = preferences.getFloat((prefix + "weight").c_str(), 0.0f);
            vessels[i].spoolWeight = preferences.getFloat((prefix + "spool").c_str(), 0.0f);
        }
    }

    void saveToPreferences() {
        Serial.printf("Saving %d vessels to preferences\n", vesselCount);

        // Clear all preferences to avoid stale data
        preferences.clear();

        // Save current state
        preferences.putInt("count", vesselCount);
        preferences.putInt("selected", selectedVesselIndex);
        for (int i = 0; i < vesselCount; i++) {
            String prefix = "vessel" + String(i) + "_";
            preferences.putString((prefix + "name").c_str(), vessels[i].name);
            preferences.putFloat((prefix + "weight").c_str(), vessels[i].vesselWeight);
            preferences.putFloat((prefix + "spool").c_str(), vessels[i].spoolWeight);
        }

        // Ensure changes are written to flash
        if (!preferences.isKey("count")) {
            Serial.println("Warning: Failed to save vessel count");
        }
    }

    VesselConfig vessels[MAX_VESSELS];
    int vesselCount;
    int selectedVesselIndex;
    Preferences preferences;
};

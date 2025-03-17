#pragma once
#include <Preferences.h>
#include "config.h"

class VesselManager {
public:
    VesselManager() : vesselCount(0) {
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

private:
    void loadFromPreferences() {
        preferences.begin("vessels", true);
        vesselCount = preferences.getInt("count", 0);
        for (int i = 0; i < vesselCount; i++) {
            String prefix = "vessel" + String(i) + "_";
            preferences.getString((prefix + "name").c_str(), vessels[i].name, sizeof(vessels[i].name));
            vessels[i].vesselWeight = preferences.getFloat((prefix + "weight").c_str(), 0.0f);
            vessels[i].spoolWeight = preferences.getFloat((prefix + "spool").c_str(), 0.0f);
        }
        preferences.end();
    }

    void saveToPreferences() {
        preferences.begin("vessels", false);
        preferences.putInt("count", vesselCount);
        for (int i = 0; i < vesselCount; i++) {
            String prefix = "vessel" + String(i) + "_";
            preferences.putString((prefix + "name").c_str(), vessels[i].name);
            preferences.putFloat((prefix + "weight").c_str(), vessels[i].vesselWeight);
            preferences.putFloat((prefix + "spool").c_str(), vessels[i].spoolWeight);
        }
        preferences.end();
    }

    VesselConfig vessels[MAX_VESSELS];
    int vesselCount;
    Preferences preferences;
};

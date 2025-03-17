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

    ~VesselManager() {
        preferences.end();
    }

    bool addVessel(const char* name, float vesselWeight, float spoolWeight) {
        if (vesselCount >= MAX_VESSELS) return false;

        strncpy(vessels[vesselCount].name, name, sizeof(vessels[vesselCount].name) - 1);
        vessels[vesselCount].name[sizeof(vessels[vesselCount].name) - 1] = '\0';
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
        vessels[index].name[sizeof(vessels[index].name) - 1] = '\0';
        vessels[index].vesselWeight = vesselWeight;
        vessels[index].spoolWeight = spoolWeight;

        saveToPreferences();
        return true;
    }

    bool deleteVessel(int index) {
        if (index < 0 || index >= vesselCount) return false;

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
        preferences.clear();
        vesselCount = preferences.getInt("count", 0);

        if (vesselCount > MAX_VESSELS) {
            vesselCount = 0;
            return;
        }

        for (int i = 0; i < vesselCount; i++) {
            char key[16];
            sprintf(key, "name%d", i);
            String name = preferences.getString(key, "");
            if (name.length() == 0) {
                vesselCount = i;
                break;
            }

            strncpy(vessels[i].name, name.c_str(), sizeof(vessels[i].name) - 1);
            vessels[i].name[sizeof(vessels[i].name) - 1] = '\0';

            sprintf(key, "vw%d", i);
            vessels[i].vesselWeight = preferences.getFloat(key, 0.0);

            sprintf(key, "sw%d", i);
            vessels[i].spoolWeight = preferences.getFloat(key, 0.0);

            vessels[i].lastWeight = 0;
            vessels[i].lastUpdate = 0;
        }
    }

    void saveToPreferences() {
        preferences.clear();
        preferences.putInt("count", vesselCount);

        for (int i = 0; i < vesselCount; i++) {
            char key[16];

            sprintf(key, "name%d", i);
            preferences.putString(key, vessels[i].name);

            sprintf(key, "vw%d", i);
            preferences.putFloat(key, vessels[i].vesselWeight);

            sprintf(key, "sw%d", i);
            preferences.putFloat(key, vessels[i].spoolWeight);
        }
    }
};

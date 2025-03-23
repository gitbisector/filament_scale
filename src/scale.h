#pragma once
#include <HX711.h>
#include "config.h"

class Scale {
public:
    Scale() : calibrationFactor(1.0f), offset(0.0f) {}
    
    void init() {
        scale.begin(HX711_DATA_PIN, HX711_CLOCK_PIN);
        scale.set_scale(calibrationFactor);
        scale.set_offset(offset);
    }
    
    float getWeight() {
        return scale.get_units();
    }
    
    void tare() {
        scale.tare();
        offset = scale.get_offset();
    }
    
    void setCalibrationFactor(float factor) {
        calibrationFactor = factor;
        scale.set_scale(calibrationFactor);
    }
    
    float getCalibrationFactor() const {
        return calibrationFactor;
    }
    
    float getOffset() const {
        return offset;
    }
    
    void setOffset(float newOffset) {
        offset = newOffset;
        scale.set_offset(offset);
    }

private:
    HX711 scale;
    float calibrationFactor;
    float offset;
};

#pragma once
#if DAC == 8562
#include "mqtt.h"
#include "settings.h"
#include "dac8562.h"


struct DAC8562Mqtt : public Device {
    std::shared_ptr<SettingsManager> settings;
    std::unique_ptr<DAC8562> dac;

    DAC8562Mqtt(std::shared_ptr<SettingsManager> settings);
    virtual ~DAC8562Mqtt() = default;

    void command_handler(std::shared_ptr<GFX> gfx, String& dest, JsonDocument &jpl) override;
    void simple_set(float value);
};
#endif

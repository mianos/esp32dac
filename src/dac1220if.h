#pragma once
#if DAC == 1220
#include "mqtt.h"
#include "settings.h"
#include "dac1220.h"


struct DAC1220Mqtt : public Device {
    std::shared_ptr<SettingsManager> settings;
    std::unique_ptr<DAC1220> dac;

    DAC1220Mqtt(std::shared_ptr<SettingsManager> settings);
    virtual ~DAC1220Mqtt() = default;

    void command_handler(std::shared_ptr<GFX> gfx, String& dest, JsonDocument &jpl) override;
    void simple_set(float value);
    void set_value(int32_t value);
    void set_voltage(double voltage);
};
#endif

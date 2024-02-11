#pragma once
#include "mqtt.h"
#include "settings.h"
#include "dac1220.h"


struct DAC1220Mqtt : public Device {
    std::shared_ptr<SettingsManager> settings;
    std::unique_ptr<DAC1220> dac;

    DAC1220Mqtt(std::shared_ptr<SettingsManager> settings);
    virtual ~DAC1220Mqtt() = default;

    void command_handler(String& dest, JsonDocument &jpl) override;
    void simple_set(float value);
};

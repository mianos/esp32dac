#pragma once
#include "mqtt.h"
#include "settings.h"
#include "mcp4921.h"


struct MCP4921Mqtt : public Device {
    std::shared_ptr<SettingsManager> settings;
    std::unique_ptr<MCP4921> dac;

    MCP4921Mqtt(std::shared_ptr<SettingsManager> settings);
    virtual ~MCP4921Mqtt() = default;

    void command_handler(String& dest, JsonDocument &jpl) override;
    void simple_set(float value);
};

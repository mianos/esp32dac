#include "mcp4921if.h"

void MCP4921Mqtt::simple_set(float value) {
  dac->writeA(static_cast<uint16_t>(value * MCP4921::MAX_VALUE / 100.0));
}


MCP4921Mqtt::MCP4921Mqtt(std::shared_ptr<SettingsManager> settings) : settings(settings) {
  Serial.printf("Dacmqtt made\n");
  dac = std::make_unique<MCP4921>();
  dac->begin();
}

void MCP4921Mqtt::command_handler(std::shared_ptr<GFX> gfx, String& dest, JsonDocument &jpl) {
    Serial.printf("local handler dest %s\n", dest.c_str());
    
    if (dest == "set") {
      if (jpl.containsKey("value")) {
        auto value = jpl["value"].as<int>();
        Serial.printf("setting DAC to %d\n", value);
        dac->writeA(value);
      } else {
        Serial.printf("needs {\"value\": N}\n");
      }
    } else if (dest == "reset") {
      dac->reset();
      Serial.printf("DAC reset command received\n");
    } else if (dest == "setGain") {
      bool gain2x = jpl.containsKey("gain2x") ? jpl["gain2x"].as<bool>() : false;
      dac->setGain(gain2x);
      Serial.printf("Set Gain command received. Gain 2x: %s\n", gain2x ? "Yes" : "No");
    } else {
      Serial.printf("Unknown command '%s'\n", dest.c_str());
    }
}

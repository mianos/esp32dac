#include "dac.h"

DAC8562Mqtt::DAC8562Mqtt(std::shared_ptr<SettingsManager> settings) : settings(settings) {
  Serial.printf("Dacmqtt made\n");
  dac = std::make_unique<DAC8562>();
  dac->begin();
}

void DAC8562Mqtt::command_handler(String& dest, JsonDocument &jpl) {
    Serial.printf("local handler dest %s\n", dest.c_str());
    // Implement specific command handling for DAC8562Mqtt
    if (dest == "set") {
      if (jpl.containsKey("channel") && jpl.containsKey("value")) {
        auto channel = jpl["channel"].as<int>();
        auto value = jpl["value"].as<int>();
        Serial.printf("setting %d to %d\n", channel, value);
        if (channel == 0) {
          dac->writeA(value);
        } else if (channel == 1) {
          dac->writeB(value);
        } else {
          Serial.printf("unsupported channel %d\n", channel);
        }
      } else {
        Serial.printf("needs {\"channel\": N, \"value\": N}");
      }
    } else {
        Serial.printf("Unknown command '%s'\n", dest.c_str());
    }
}

#include "dac.h"

DAC8562Mqtt::DAC8562Mqtt(std::shared_ptr<SettingsManager> settings) : settings(settings) {
    Serial.printf("Dacmqtt made\n");
}

void DAC8562Mqtt::command_handler(String& dest, JsonDocument &jpl) {
    Serial.printf("local handler dest %s\n", dest.c_str());
    // Implement specific command handling for DAC8562Mqtt
}

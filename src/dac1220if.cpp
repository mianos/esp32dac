#include "dac1220if.h"

void DAC1220Mqtt::simple_set(float value) {
  dac->writeA(static_cast<uint16_t>(value * DAC1220::MAX_VALUE / 100.0));
}

void DAC1220Mqtt::set_value(int32_t value) {
  dac->set_value(value);
}

void DAC1220Mqtt::set_voltage(double voltage) {
  dac->set_voltage(voltage);
}

DAC1220Mqtt::DAC1220Mqtt(std::shared_ptr<SettingsManager> settings) : settings(settings) {
  Serial.printf("Dacmqtt made\n");
  dac = std::make_unique<DAC1220>();
  dac->begin();
}

void DAC1220Mqtt::command_handler(String& dest, JsonDocument &jpl) {
    Serial.printf("local handler dest %s\n", dest.c_str());
    // Implement specific command handling for DAC1220Mqtt
    if (dest == "set") {
      if (jpl.containsKey("value")) {
          uint32_t value;
        if (jpl.containsKey("hex")) {
          auto ss  = jpl["value"].as<String>();
          value = strtol(ss.c_str(), NULL, 16);
        } else {
          value = jpl["value"].as<int>();
        }
        Serial.printf("setting  %d %x\n", value, value);
        dac->set_value(value);
      } else {
        Serial.printf("needs {\"value\": N}");
      }
    } else if (dest == "calibrate") {
        dac->calibrate();
        Serial.printf("calibrate\n");
    } else if (dest == "reference") {
      if (jpl.containsKey("value")) {
        auto voltage = jpl["value"].as<double>();
        Serial.printf("setting reference voltage to %g\n", voltage);
        dac->set_reference(voltage);
      } else {
        Serial.printf("needs {\"value\": <double>}");
     }
    } else if (dest == "voltage") {
      if (jpl.containsKey("value")) {
        auto voltage = jpl["value"].as<double>();
        Serial.printf("setting voltage to %g\n", voltage);
        dac->set_voltage(voltage);
      } else {
        Serial.printf("needs {\"value\": <double>}");
      }
    } else if (dest == "reset") {
      bool resetAll = jpl.containsKey("resetAll") ? jpl["resetAll"].as<bool>() : false;
      dac->reset(resetAll);
      Serial.printf("DAC reset command received. Reset All: %s\n", resetAll ? "Yes" : "No");
    } else if (dest == "setReference") {
      bool enableGain2 = jpl.containsKey("enableGain2") ? jpl["enableGain2"].as<bool>() : false;
      dac->setReference(enableGain2);
      Serial.printf("Set Reference command received. Enable Gain 2: %s\n", enableGain2 ? "Yes" : "No");
    } else if (dest == "setGain") {
      bool gainB1A1 = jpl.containsKey("gainB1A1") ? jpl["gainB1A1"].as<bool>() : false;
      dac->setGain(gainB1A1);
      Serial.printf("Set Gain command received. Gain B1A1: %s\n", gainB1A1 ? "Yes" : "No");
    } else if (dest == "updateAll") {
      dac->updateAll();
      Serial.printf("Update All command received\n");
    } else {
      Serial.printf("Unknown command '%s'\n", dest.c_str());
    }
}

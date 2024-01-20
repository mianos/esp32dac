#pragma once
#include <memory>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "settings.h"
#include "dac8562.h"

struct MqttManagedDevices {
    WiFiClient espClient;
    PubSubClient client;
    std::shared_ptr<SettingsManager> settings;

    static constexpr size_t MQTT_BUFFER_SIZE = 1024;
    static constexpr int MQTT_RECONNECT_DELAY_MS = 10000;
    static constexpr int INITIAL_MQTT_CONNECT_DELAY_MS = 1000;
    static constexpr int LOCAL_MQTT_VERSION = 2;

    explicit MqttManagedDevices(std::shared_ptr<SettingsManager> settings);
    virtual ~MqttManagedDevices() = default;

    void callback(char* topic_str, byte* payload, unsigned int length);
    void publish_error(const String& error);
    bool reconnect();
    void handle();

    virtual void command_handler(String& dest, JsonDocument &jpl) = 0;
};

struct DAC8562Mqtt : public MqttManagedDevices {
    std::unique_ptr<DAC8562> dac;

    DAC8562Mqtt(std::shared_ptr<SettingsManager> settings, std::unique_ptr<DAC8562> dac);
    virtual ~DAC8562Mqtt() = default;

    void command_handler(String& dest, JsonDocument &jpl) override;
};

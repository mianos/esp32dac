#pragma once
#include <memory>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "settings.h"

struct Device {
  virtual void command_handler(String& dest, JsonDocument &jpl) = 0;
  virtual void simple_set(float value) = 0; // 0.0 to 100.0
};

struct MqttManagedDevices {
    WiFiClient espClient;
    PubSubClient client;
    std::shared_ptr<SettingsManager> settings;
    std::unique_ptr<Device> device;

    static constexpr size_t MQTT_BUFFER_SIZE = 1024;
    static constexpr int MQTT_RECONNECT_DELAY_MS = 10000;
    static constexpr int INITIAL_MQTT_CONNECT_DELAY_MS = 1000;

    static constexpr int LOCAL_VERSION = 2;

    MqttManagedDevices(std::shared_ptr<SettingsManager> settings, std::unique_ptr<Device> device);
    virtual ~MqttManagedDevices() = default;

    void callback(char* topic_str, byte* payload, unsigned int length);
    void publish_error(const String& error);
    bool reconnect();
    void handle();

    int wcount = 0;
    void wave();
};


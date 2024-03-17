#pragma once
#include <memory>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "settings.h"
#include "gfx.h"
#include "gen.h"

struct Device {
  virtual void command_handler(std::shared_ptr<GFX> gfx, String& dest, JsonDocument &jpl) = 0;
  virtual void simple_set(float value) = 0; // 0.0 to 100.0
  virtual void set_value(int32_t value) { Serial.printf("Not done for this device yet\n"); }
  virtual void set_voltage(double voltage) { Serial.printf("voltage Not done for this device yet\n"); }
};

struct MqttManagedDevices {
    WiFiClient espClient;
    PubSubClient client;
    std::shared_ptr<SettingsManager> settings;
    std::shared_ptr<GFX> gfx;
    std::unique_ptr<Device> device;

    static constexpr size_t MQTT_BUFFER_SIZE = 1024;
    static constexpr int MQTT_RECONNECT_DELAY_MS = 10000;
    static constexpr int INITIAL_MQTT_CONNECT_DELAY_MS = 1000;

    static constexpr int LOCAL_VERSION = 2;

    MqttManagedDevices(std::shared_ptr<SettingsManager> settings, std::shared_ptr<GFX> gfx, std::unique_ptr<Device> device);
    virtual ~MqttManagedDevices() = default;

    void callback(char* topic_str, byte* payload, unsigned int length);
    void publish_error(const String& error);
    bool reconnect();
    void handle();

    int wcount = 0;
    void wave();
	void publish_result(struct TestConfig& tc);
};


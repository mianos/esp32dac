#include <ArduinoOTA.h>
#include <ESPDateTime.h>
#include <SPI.h> 
#include <Wire.h>

#include "provision.h"
#include "mqtt.h"

WiFiClient wifiClient;

std::shared_ptr<DAC8562Mqtt> mqtt;
std::shared_ptr<SettingsManager> settings;
std::unique_ptr<DAC8562> dac;

unsigned long lastInvokeTime = 0; // Store the last time you called the function
const unsigned long dayMillis = 24UL * 60 * 60 * 1000; // Milliseconds in a day

void setup() {
  Serial.begin(115200);
  // esp_wifi_set_ps(WIFI_PS_NONE);

  delay(2000);
  wifi_connect();
  settings = std::make_shared<SettingsManager>();
  dac = std::make_unique<DAC8562>();

  DateTime.setTimeZone(settings->tz.c_str());
  DateTime.begin(/* timeout param */);
  lastInvokeTime = millis();
  if (!DateTime.isTimeValid()) {
    Serial.printf("Failed to get time from server\n");
  }

  mqtt = std::make_shared<DAC8562Mqtt>(settings, std::move(dac));
}

// int ii;

void loop() {
  unsigned long currentMillis = millis();
  
  mqtt->handle();

  if (currentMillis - lastInvokeTime >= dayMillis) {
      DateTime.begin(1000);
      lastInvokeTime = currentMillis;
  }
  ArduinoOTA.handle();
  delay(10);
}

#include <ArduinoOTA.h>
#include <ESPDateTime.h>
#include <HTTPClient.h>
#include <SPI.h> 
#include <Wire.h>
#include "esp_wifi.h"

#include "gfx.h"

#include "provision.h"
#include "mqtt.h"
#include "gen.h"

#include "mcp4921if.h"
#include "dac8562if.h"
#include "dac1220if.h"


WiFiClient wifiClient;
bool testWiFiConnection();

std::shared_ptr<MqttManagedDevices> mqtt;
std::shared_ptr<SettingsManager> settings;
std::shared_ptr<GFX> gfx;
// std::unique_ptr<MCP4921Mqtt> dac;
// std::unique_ptr<DAC8562Mqtt> tidac;

//unsigned long lastInvokeTime = 0; // Store the last time you called the function
//const unsigned long dayMillis = 24UL * 60 * 60 * 1000; // Milliseconds in a day

void setup() {
  Serial.begin(115200);
//  esp_wifi_set_ps(WIFI_PS_NONE);

  gfx = std::make_shared<GFX>();
  delay(2000);
  wifi_connect();
  settings = std::make_shared<SettingsManager>();
  
  DateTime.setTimeZone(settings->tz.c_str());
  DateTime.begin(/* timeout param */);
//  lastInvokeTime = millis();
  if (!DateTime.isTimeValid()) {
    Serial.printf("Failed to get time from server\n");
  }
#if 0
  auto dac = std::make_unique<MCP4921Mqtt>(settings);
  auto dac = std::make_unique<DAC8562Mqtt>(settings);
#else
  auto dac = std::make_unique<DAC1220Mqtt>(settings);
#endif
	mqtt = std::make_shared<MqttManagedDevices>(settings, gfx, std::move(dac));
	signal_gen(10000000);
	initializePCNT();
	initialize_hardware_timer();
}


enum State {
    STATE_IDLE,
    STATE_WIFI_STOPPING,
    STATE_TEST_RUNNING,
    STATE_TEST_COMPLETE,
    STATE_WIFI_ENABLED,
    STATE_WIFI_CONNECTED,
    STATE_MQTT_PUBLISHED,
    STATE_MQTT_WORKER_LOOP
};

State currentState = STATE_IDLE;
bool testRunning = false;
unsigned long testStartTime = 0;
const int testDuration = 10; // Duration of the test, in seconds
const int delayBetweenTests = 30; // Delay between tests, in seconds
const int delayAfterMQTT = 10; // Delay after MQTT worker loop, in seconds
const int delayAfterWifiStart = 10; // Delay after WiFi start, in seconds
const int delayAfterWifiStop = 5; // Delay after WiFi stop, in seconds
unsigned long lastInvokeTime = 0; // Declare and initialize lastInvokeTime

void loop() {
    unsigned long currentMillis = millis();
    auto test_state = checkPCNTOverflow();
    esp_err_t result; // Declare result outside the switch statement

    switch (currentState) {
        case STATE_IDLE:
            if (test_state == IDLE && !testRunning) {
                runTest(testDuration);
                Serial.printf("Test running\n");
                testRunning = true;
                testStartTime = currentMillis;
                lastInvokeTime = currentMillis;
                currentState = STATE_TEST_RUNNING;
            }
            break;

        case STATE_TEST_RUNNING:
			if (test_state == IDLE) {
                currentState = STATE_TEST_COMPLETE;
                Serial.printf("Test complete\n");
                testRunning = false;
			}
            break;

        case STATE_TEST_COMPLETE:
            result = esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
            if (result == ESP_OK) {
				Serial.printf("esp wifi started\n");
			}
			currentState = STATE_WIFI_ENABLED;
			testStartTime = currentMillis; // Update the start time for the next delay
            break;

        case STATE_WIFI_ENABLED:
            if (currentMillis - testStartTime >= delayAfterWifiStart * 1000UL) {
                currentState = STATE_WIFI_CONNECTED;
				mqtt->handle();
				Serial.printf("Assuming connected\n");
            }
            break;

        case STATE_WIFI_CONNECTED:
            if (WiFi.status() == WL_CONNECTED) {
				testWiFiConnection();
				Serial.printf("publish?\n");
				 mqtt->publish_result(1.0);
				 currentState = STATE_MQTT_PUBLISHED;
            } else {
				Serial.printf("not connected\n");
				delay(1000);
			}
            break;

        case STATE_MQTT_PUBLISHED:
			mqtt->handle();
            currentState = STATE_MQTT_WORKER_LOOP;
            testStartTime = currentMillis; // Update the start time for the worker loop delay
            break;

        case STATE_MQTT_WORKER_LOOP:
            if (currentMillis - testStartTime >= delayAfterMQTT * 1000UL) {
				esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
                currentState = STATE_WIFI_STOPPING;
                testStartTime = currentMillis; // Update the start time for the WiFi stop delay
            } else {
				mqtt->handle();
            }
            break;

        case STATE_WIFI_STOPPING:
            if (currentMillis - testStartTime >= delayAfterWifiStop * 1000UL) {
                currentState = STATE_IDLE;
            }
            break;
    }

    // Your existing loop actions
    ArduinoOTA.handle();
    delay(10);
}

bool testWiFiConnection() {
  HTTPClient http;
  http.begin("http://mqtt2.mianos.com/health");
  int httpResponseCode = http.GET();

  if (httpResponseCode == HTTP_CODE_OK) {
    String response = http.getString();
    Serial.println("HTTP GET response:");
    Serial.println(response);
    http.end();
    return true;
  } else {
    Serial.print("HTTP GET failed, error code: ");
    Serial.println(httpResponseCode);
    http.end();
    return false;
  }
}

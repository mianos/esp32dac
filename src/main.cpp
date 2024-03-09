#include <ArduinoOTA.h>
#include <ESPDateTime.h>
#include <SPI.h> 
#include <Wire.h>
#include <esp_wifi.h>

#include "gfx.h"

#include "provision.h"
#include "mqtt.h"
#include "gen.h"

#include "mcp4921if.h"
#include "dac8562if.h"
#include "dac1220if.h"


WiFiClient wifiClient;

std::shared_ptr<MqttManagedDevices> mqtt;
std::shared_ptr<SettingsManager> settings;
std::shared_ptr<GFX> gfx;
// std::unique_ptr<MCP4921Mqtt> dac;
// std::unique_ptr<DAC8562Mqtt> tidac;

unsigned long lastInvokeTime = 0; // Store the last time you called the function
const unsigned long dayMillis = 24UL * 60 * 60 * 1000; // Milliseconds in a day

void setup() {
  Serial.begin(115200);
//  esp_wifi_set_ps(WIFI_PS_NONE);

  gfx = std::make_shared<GFX>();
  delay(2000);
  wifi_connect();
  settings = std::make_shared<SettingsManager>();
  
  DateTime.setTimeZone(settings->tz.c_str());
  DateTime.begin(/* timeout param */);
  lastInvokeTime = millis();
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

//int ii;

enum State {
    STATE_IDLE,
    STATE_TEST_RUNNING,
    STATE_TEST_COMPLETE
};

State currentState = STATE_IDLE;
bool testRunning = false;
unsigned long testStartTime = 0;
const int testDuration = 10; // Duration of the test, in seconds
const int delayBetweenTests = 20; // Delay between tests, in seconds

void loop() {
    unsigned long currentMillis = millis();
    auto state = checkPCNTOverflow();

    switch (currentState) {
        case STATE_IDLE:
            if (state == IDLE && !testRunning) {
                runTest(testDuration);
                Serial.printf("Test running\n");
                testRunning = true;
                testStartTime = currentMillis;
                lastInvokeTime = currentMillis; // Ensure this is only updated when we start a new test
                currentState = STATE_TEST_RUNNING;
            }
            break;

        case STATE_TEST_RUNNING:
            if (currentMillis - testStartTime >= testDuration * 1000UL) {
                currentState = STATE_TEST_COMPLETE;
                testRunning = false;
				Serial.printf("Test done, waiting\n");
            }
            break;

        case STATE_TEST_COMPLETE:
            if (currentMillis - testStartTime >= (testDuration + delayBetweenTests) * 1000UL) {
                currentState = STATE_IDLE;
            }
            break;
    }

    // Your existing loop actions
    ArduinoOTA.handle();
    delay(10); // Consider replacing with a non-blocking delay if precise timing is needed
}

#include "mqtt.h"
#include <ESPDateTime.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <StringSplitter.h>

#include "provision.h"
#include "gen.h"

void MqttManagedDevices::publish_error(const String& error) {
    JsonDocument doc;
    doc["time"] = DateTime.toISOString();
    doc["error"] = error;
    String topic = "tele/" + settings->sensorName + "/error";
    String output;
    serializeJson(doc, output);
    client.publish(topic.c_str(), output.c_str());
}


void MqttManagedDevices::callback(char* topic_str, byte* payload, unsigned int length) {
    String topic = String(topic_str);
    auto splitter = StringSplitter(topic, '/', 4);
    auto itemCount = splitter.getItemCount();
    if (itemCount < 3) {
        Serial.printf("Item count less than 3 %d '%s'\n", itemCount, topic_str);
        return;
    }

    for (auto i = 0; i < itemCount; i++) {
        String item = splitter.getItemAtIndex(i);
        Serial.println("Item @ index " + String(i) + ": " + String(item));
    }
    Serial.printf("command '%s'\n", splitter.getItemAtIndex(itemCount - 1).c_str());

    if (splitter.getItemAtIndex(0) == "cmnd") {
        JsonDocument jpl;
        auto err = deserializeJson(jpl, payload, length);
        if (err) {
            Serial.printf("deserializeJson() failed: '%s'\n", err.c_str());
            return;
        }
        String dest = splitter.getItemAtIndex(itemCount - 1);
        if (dest == "reprovision") {
            Serial.printf("clearing provisioning\n");
            reset_provisioning();
        } else if (dest == "restart") {
            Serial.printf("rebooting\n");
            ESP.restart();
        } else if (dest == "settings") {
            auto result = settings->loadFromDocument(jpl);
            // Implement specific settings update logic
		} else if (dest == "runtest") {
			Serial.printf("Run test\n");
			if (uxQueueSpacesAvailable(testQueue) > 0) {
				if (jpl.containsKey("period")) {
					TestConfig testConfig{};
					testConfig.period = jpl["period"].as<int>();

					if (jpl.containsKey("test_id")) {
						testConfig.test_id = jpl["test_id"].as<int>();
					}

					if (jpl.containsKey("repeat")) {
						testConfig.repeat = jpl["repeat"].as<int>();
					}

					Serial.printf("Run test - period: %d, test_id: %d, repeat: %d\n", testConfig.period, testConfig.test_id, testConfig.repeat);
					xQueueSend(testQueue, &testConfig, portMAX_DELAY);
				} else {
					Serial.printf("Period missing in runtest\n");
					// Handle the case when the "period" key is missing
					// Return an error message or take alternative actions
					// For example, you can send an error response back to the client
					publish_error("Period is required for runtest");
				}
			} else {
				// Handle the case when the queue is full
				// Log an error message or take alternative actions
				Serial.println("Failed to add test configuration to the queue. Queue is full.");
			}
		}
    }
}


MqttManagedDevices::MqttManagedDevices(std::shared_ptr<SettingsManager> settings,
                                       std::shared_ptr<GFX> gfx,
                                       std::unique_ptr<Device> device)
    : settings(settings),
      gfx(gfx),
      device(std::move(device)),
      client(espClient) {
    client.setBufferSize(MQTT_BUFFER_SIZE);
    client.setServer(settings->mqttServer.c_str(), settings->mqttPort);
    client.setCallback([this](char* topic_str, byte* payload, unsigned int length) {
        callback(topic_str, payload, length);
    });
}

bool MqttManagedDevices::reconnect() {
    String clientId = WiFi.getHostname();
    if (client.connect(clientId.c_str())) {
        delay(INITIAL_MQTT_CONNECT_DELAY_MS);
        String cmnd_topic = String("cmnd/") + settings->sensorName + "/#";
        client.subscribe(cmnd_topic.c_str());
        Serial.printf("mqtt connected as sensor '%s'\n", settings->sensorName.c_str());

        JsonDocument doc;
        doc["version"] = LOCAL_VERSION;
        doc["time"] = DateTime.toISOString();
        doc["hostname"] = WiFi.getHostname();
        doc["ip"] = WiFi.localIP().toString();
        settings->fillJsonDocument(doc);
        String status_topic = "tele/" + settings->sensorName + "/init";
        String output;
        serializeJson(doc, output);
        client.publish(status_topic.c_str(), output.c_str());
        return true;
    } else {
        Serial.printf("failed to connect to %s port %d state %d\n", settings->mqttServer.c_str(), settings->mqttPort, client.state());
        delay(MQTT_RECONNECT_DELAY_MS);
        return false;
    }
}

void MqttManagedDevices::wave() {
  if (wcount == 100) {
    wcount = 0;
  }
//  device->simple_set(wcount++);
  device->set_value((wcount++)  << 15);
}

void MqttManagedDevices::handle() {
    if (!client.connected()) {
        if (!reconnect()) {
			Serial.printf("Failed reconnect, no loop\n");
            return;
        }
    }
    // generic callback
    client.loop();
}

void MqttManagedDevices::publish_result(struct TestConfig& tc) {
	JsonDocument doc;
	doc["time"] = DateTime.toISOString();
	doc["result"] = get_LastTestCount() / (double)tc.period;
	doc["test_id"] = tc.test_id;
	doc["repeat"] = tc.repeat;
	String status_topic = "tele/" + settings->sensorName + "/result";
	String output;
	serializeJson(doc, output);
	client.publish(status_topic.c_str(), output.c_str());
}

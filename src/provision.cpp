#include <ArduinoOTA.h>
#include <WiFiProv.h>
#include <WiFi.h>

int prov_on_flag = 0;
int prov_end = 0;
int prov_err = 0;

void SysProvEvent(arduino_event_t *sys_event)
{
    switch (sys_event->event_id) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        Serial.printf("\nConnected IP address : %s\n", IPAddress(sys_event->event_info.got_ip.ip_info.ip.addr).toString().c_str());
        break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        Serial.printf("\nWHAT?  Disconnected. Connecting to the AP again...\n");
        break;
    case ARDUINO_EVENT_PROV_START:
        Serial.printf("\nProvisioning started\nGive Credentials of your access point using \" Android app \"\n");
        break;
    case ARDUINO_EVENT_PROV_CRED_RECV:
        Serial.printf("\nReceived Wi-Fi credentials\n");
        Serial.printf("\tSSID : %s\n", (const char *)sys_event->event_info.prov_cred_recv.ssid);
        Serial.printf("\tPassword : %s\n", (const char *)sys_event->event_info.prov_cred_recv.password);
        break;
    case ARDUINO_EVENT_PROV_CRED_FAIL:
        Serial.printf("\nProvisioning failed!\nPlease reset to factory and retry provisioning\n");
        if(sys_event->event_info.prov_fail_reason == WIFI_PROV_STA_AUTH_ERROR)
            Serial.printf("\nWi-Fi AP password incorrect\n");
        else
            Serial.printf("\nWi-Fi AP not found....Add API \" nvs_flash_erase() \" before beginProvision()\n");
        break;
    case ARDUINO_EVENT_PROV_CRED_SUCCESS:
        Serial.printf("\nProvisioning Successful\n");
        break;
    case ARDUINO_EVENT_PROV_END:
        Serial.printf("\nProvisioning Ends\n");
        break;
    case ARDUINO_EVENT_PROV_INIT:
        Serial.printf("Prov: Init\n");
        break;
    case ARDUINO_EVENT_PROV_DEINIT:
        Serial.printf("Prov: Stopped\n");
        break;
    case ARDUINO_EVENT_WIFI_SCAN_DONE:
        Serial.printf("Prov: scan done\n");
        break;
    case SYSTEM_EVENT_STA_START:
//        Serial.printf("Station start\n");
        break;
    case SYSTEM_EVENT_STA_CONNECTED:
        Serial.printf("Station connected\n");
        break;
    default:
//        Serial.printf("Some other event %d\n", sys_event->event_id);
        break;
    }
}

void reset_provisioning() {
    wifi_prov_mgr_reset_provisioning();
    ESP.restart();
}

String getMacAddress() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char macStr[18] = { 0 };
  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(macStr);
}

void wifi_connect(const char *pname) {
  WiFi.mode(WIFI_STA);
  WiFi.onEvent(SysProvEvent);
  WiFi.disconnect();  // Disconnect from the WiFi
  Serial.printf("mac '%s'\n", getMacAddress().c_str());
  String name = String("faba_");
  name += String(random(0xffff), HEX);
  Serial.printf("prov name: %s\n",name.c_str());

  Serial.printf("Begin Provisioning using Soft AP\n");
  const char *pop = "bongcloud";

  WiFiProv.beginProvision(WIFI_PROV_SCHEME_SOFTAP, WIFI_PROV_SCHEME_HANDLER_NONE, WIFI_PROV_SECURITY_1, pop, pname);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.printf("Connecting to WiFi...\n");
    delay(2000);
  }
  Serial.printf("Connected\n");

  ArduinoOTA
  .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
  .onEnd([]() {
      Serial.println("\nEnd");
    })
  .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
  .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
  ArduinoOTA.begin();
}

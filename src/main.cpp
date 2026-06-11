#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPDashboardPlus.h>
#include <Preferences.h>
#include <TaskScheduler.h>
#include <PubSubClient.h>
#include <ezLED.h>
#include "dashboard_html.h"  // Auto-generated
#include "Credentials.h"

#define _DEBUG_  // Comment this line to disable debug prints
#include "Debug.h"

#include "DashboardUI.h"

#define deviceName "ESP32 Dashboard Plus"

Preferences prefs;
AsyncWebServer server(80);
ESPDashboardPlus dashboard(deviceName);
DashboardUI ui;

String savedSSID     = "";
String savedPassword = "";

// MQTT
String mqttBroker = "";
uint16_t mqttPort = 1883;
String mqttUser   = "";
String mqttPass   = "";

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

ezLED statusLed(LED_BUILTIN);

// Scheduler
Scheduler ts;

void connectMqtt();
void reconnectMqtt();
Task tConnectMqtt(TASK_IMMEDIATE, TASK_FOREVER, &connectMqtt, &ts, true);
Task tReconnectMqtt(5 * TASK_SECOND, TASK_FOREVER, &reconnectMqtt, &ts, false);
Task tStatusLed(TASK_IMMEDIATE, TASK_FOREVER, []() { statusLed.loop(); }, &ts, true);
Task tDashboardLoop(TASK_IMMEDIATE, TASK_FOREVER, []() { dashboard.loop(); }, &ts, true);

void connectMqtt() {
  if (WiFi.status() != WL_CONNECTED) { return; }

  if (!mqtt.connected()) {
    _def("MQTT not connected, attempting to reconnect...\n");
    tConnectMqtt.disable();
    tReconnectMqtt.enable();
  } else {
    mqtt.loop();
  }
}

void reconnectMqtt() {
  if (WiFi.status() != WL_CONNECTED) { return; }
  if (mqtt.connect(deviceName, mqttUser.c_str(), mqttPass.c_str())) {
    _def("MQTT connected: %s\n", mqttBroker.c_str());
    // mqtt.publish("outTopic", "hello world");
    // mqtt.subscribe("omg/OMG_ESP32_BLE/BTtoMQTT/A4C138C5BFA8");
    statusLed.blinkNumberOfTimes(300, 300, 3);
    dashboard.updateStatusCard("mqtt_status", StatusIcon::WIFI, CardVariant::SUCCESS, "Connected",
                               mqttBroker);
    tReconnectMqtt.disable();
    tConnectMqtt.enable();
    return;
  } else {
    dashboard.updateStatusCard("mqtt_status", StatusIcon::WIFI, CardVariant::WARNING,
                               "Not Connected",
                               "MQTT connect failed, state: " + String(mqtt.state()));
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  _def("Message arrived [ %s ]", topic);
  //   for (int i = 0; i < length; i++) { _def("%c", (char)payload[i]); }
  // _def("\n");

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload, length);
  if (error) {
    _def("JSON parse failed: %s\n", error.c_str());
    return;
  }

  float tempc = doc["tempc"] | 0.0f;
  float hum   = doc["hum"] | 0.0f;

  _def(" tempc: %.2f | hum: %.2f\n", tempc, hum);
}

void setup() {
  _serialBegin(115200);

  // Load saved credentials
  prefs.begin("config", true);
  savedSSID     = prefs.getString("ssid", "");
  savedPassword = prefs.getString("pass", "");
  mqttBroker    = prefs.getString("mqtt_broker", "");
  mqttUser      = prefs.getString("mqtt_user", "");
  mqttPass      = prefs.getString("mqtt_pass", "");
  mqttPort      = prefs.getUInt("mqtt_port", 1883);
  prefs.end();

  mqtt.setServer(mqttBroker.c_str(), mqttPort);
  mqtt.setCallback(mqttCallback);
  mqtt.setBufferSize(512);  // Option: Adjust as needed based on expected message sizes

  // Try saved credentials or start AP
  if (savedSSID.length() > 0) {
    WiFi.begin(savedSSID.c_str(), savedPassword.c_str());
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      attempts++;
    }
  } else {
    _def("No saved WiFi credentials\n");
  }

  if (WiFi.status() != WL_CONNECTED) {
    _def("Starting AP mode...\n");
    WiFi.softAP(apSsid, apPassword);
  }

  ui.init(dashboard, server, savedSSID, savedPassword, mqttBroker, mqttPort, mqttUser, mqttPass,
          prefs);

  server.begin();
}

void loop() {
  ts.execute();
}
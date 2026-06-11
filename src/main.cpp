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

#define deviceName "ESP32 Dashboard Plus"

Preferences prefs;
AsyncWebServer server(80);
ESPDashboardPlus dashboard(deviceName);

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
  mqtt.setBufferSize(512); // Option: Adjust as needed based on expected message sizes

  // Try saved credentials or start AP
  if (savedSSID.length() > 0) {
    WiFi.begin(savedSSID.c_str(), savedPassword.c_str());
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      attempts++;
    }
  }

  if (WiFi.status() != WL_CONNECTED) {
    if (savedSSID.length() <= 0) {
        _def("No saved WiFi credentials, starting AP mode...\n");
      WiFi.softAP(apSsid, apPassword);
    }
  }

  dashboard.begin(&server, DASHBOARD_HTML_DATA, DASHBOARD_HTML_SIZE);

  // WiFi Status
  StatusCard* status = dashboard.addStatusCard("status", "WiFi Status", StatusIcon::WIFI);
  status->setWeight(1);
  if (WiFi.status() == WL_CONNECTED) {
    status->setStatus(StatusIcon::WIFI, CardVariant::SUCCESS, "Connected",
                      WiFi.localIP().toString());
    _def("Connected to WiFi: %s\n", WiFi.localIP().toString().c_str());
  } else {
    status->setStatus(StatusIcon::WIFI, CardVariant::WARNING, "AP Mode", "192.168.4.1");
  }

  StatusCard* mqttStatus = dashboard.addStatusCard("mqtt_status", "MQTT Status", StatusIcon::WIFI);
  mqttStatus->setWeight(2);

  // SSID input
  InputCard* ssidInput = dashboard.addInputCard("ssid", "WiFi SSID", savedSSID);
  ssidInput->setWeight(3);
  ssidInput->setValue(savedSSID);

  // Password input
  InputCard* passInput = dashboard.addInputCard("password", "WiFi Password", "");
  passInput->setWeight(4);
  passInput->inputType = "password";

  // MQTT Broker input
  InputCard* mqttBrokerInput = dashboard.addInputCard("mqtt_broker", "MQTT Broker", "");
  mqttBrokerInput->setWeight(5);
  mqttBrokerInput->setValue(mqttBroker);

  // MQTT Port input
  InputCard* mqttPortInput = dashboard.addInputCard("mqtt_port", "MQTT Port", "");
  mqttPortInput->setWeight(6);
  mqttPortInput->setValue(String(mqttPort));

  // MQTT User input
  InputCard* mqttUserInput = dashboard.addInputCard("mqtt_user", "MQTT User", "");
  mqttUserInput->setWeight(7);
  // mqttUserInput->setValue(mqttUser);

  // Password input
  InputCard* mqttPassInput = dashboard.addInputCard("mqtt_pass", "MQTT Password", "");
  mqttPassInput->setWeight(8);
  mqttPassInput->inputType = "password";

  // Save button
  ButtonCardImpl* configBtn =
    dashboard.addButtonCard("save", "Network Settings", "Save & Restart", []() {
      prefs.begin("config", false);
      // Get values from cards
      InputCard* ssid       = static_cast<InputCard*>(dashboard.getCard("ssid"));
      InputCard* pass       = static_cast<InputCard*>(dashboard.getCard("password"));
      InputCard* mqttBroker = static_cast<InputCard*>(dashboard.getCard("mqtt_broker"));
      InputCard* mqttUser   = static_cast<InputCard*>(dashboard.getCard("mqtt_user"));
      InputCard* mqttPass   = static_cast<InputCard*>(dashboard.getCard("mqtt_pass"));
      InputCard* mqttPort   = static_cast<InputCard*>(dashboard.getCard("mqtt_port"));
      prefs.putString("ssid", ssid->value);
      prefs.putString("pass", pass->value);
      prefs.putString("mqtt_broker", mqttBroker->value);
      prefs.putString("mqtt_user", mqttUser->value);
      prefs.putString("mqtt_pass", mqttPass->value);
      prefs.putUInt("mqtt_port", mqttPort->value.toInt());
      prefs.end();

      delay(1000);
      ESP.restart();
    });
  configBtn->setWeight(8);

  server.begin();
}

void loop() {
  ts.execute();
}
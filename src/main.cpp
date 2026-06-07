#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPDashboardPlus.h>
#include <Preferences.h>
#include <TaskScheduler.h>
#include <PubSubClient.h>
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

// Dashboard
StatusCard* mqttStatus = nullptr;

// Scheduler
Scheduler ts;

void connectMqtt();
void reconnectMqtt();
Task tConnectMqtt(0, TASK_FOREVER, &connectMqtt, &ts, true);
Task tReconnectMqtt(2 * TASK_SECOND, TASK_FOREVER, &reconnectMqtt, &ts, false);
Task tMqttLoop(TASK_IMMEDIATE, TASK_FOREVER, []() { mqtt.loop(); }, &ts, true);
Task tDashboardLoop(TASK_IMMEDIATE, TASK_FOREVER, []() { dashboard.loop(); }, &ts, true);

void connectMqtt() {
  if (WiFi.status() != WL_CONNECTED) { return; }

  if (!mqtt.connected()) {
    _def("MQTT not connected, attempting to reconnect...\n");
    tConnectMqtt.disable();
    tReconnectMqtt.enable();
  }
} 

constexpr uint8_t MQTT_MAX_ATTEMPTS = 5;

void reconnectMqtt() {
  if (WiFi.status() != WL_CONNECTED) { return; }
  static uint8_t attempts = 0;
  if (mqtt.connect(deviceName, mqttUser.c_str(), mqttPass.c_str())) {
    attempts = 0;
    _def("MQTT connected\n");
    // mqtt.subscribe("inTopic");
    // mqtt.publish("outTopic", "hello world");
    mqttStatus->setStatus(StatusIcon::WIFI, CardVariant::SUCCESS, "Connected", mqttBroker);
    tReconnectMqtt.setIntervalNodelay(2 * TASK_SECOND, TASK_INTERVAL_RECALC);
    tReconnectMqtt.disable();
    tConnectMqtt.enable();
    return;
  } else {
    attempts++;
    mqttStatus->setStatus(StatusIcon::WIFI, CardVariant::WARNING, "Not Connected",
                          "Retry " + String(attempts) + "/" + String(MQTT_MAX_ATTEMPTS));
  }

  if (attempts >= MQTT_MAX_ATTEMPTS) {
    attempts = 0;
    _def("MQTT connect failed, state: %d\n", mqtt.state());
    tReconnectMqtt.setIntervalNodelay(5 * TASK_MINUTE, TASK_INTERVAL_RECALC);
  } else if (attempts <= 1) {
    tReconnectMqtt.setIntervalNodelay(2 * TASK_SECOND, TASK_INTERVAL_RECALC);
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  _def("Message arrived [");
  _def("%s", topic);
  _def("] ");
  for (int i = 0; i < length; i++) { _def("%c", (char)payload[i]); }
  _def("\n");
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

  // Try saved credentials or start AP
  if (savedSSID.length() > 0) {
    WiFi.begin(savedSSID.c_str(), savedPassword.c_str());
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      attempts++;
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    //   mqtt.setBufferSize(1024);
    mqtt.setCallback(mqttCallback);
    mqtt.setServer(mqttBroker.c_str(), mqttPort);
    if (!mqtt.connect(deviceName, mqttUser.c_str(), mqttPass.c_str())) {
      _def("Failed to connect to MQTT broker\n");
    }
  } else {
    WiFi.softAP(apSsid, apPassword);
  }

  // if (WiFi.status() != WL_CONNECTED) { WiFi.softAP(apSsid, apPassword); }

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

  // MQTT Status
  StatusCard* mqttStatus = dashboard.addStatusCard("mqtt_status", "MQTT Status", StatusIcon::WIFI);
  mqttStatus->setWeight(2);
  if (mqtt.connected()) {
    // if (mqtt.state() == MQTT_CONNECTED) {
    mqttStatus->setStatus(StatusIcon::WIFI, CardVariant::SUCCESS, "Connected", mqttBroker);
  } else {
    mqttStatus->setStatus(StatusIcon::WIFI, CardVariant::WARNING, "Not Connected",
                          "Retry to restart");
  }

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

  // MQTT User input
  InputCard* mqttUserInput = dashboard.addInputCard("mqtt_user", "MQTT User", "");
  mqttUserInput->setWeight(6);
  mqttUserInput->setValue(mqttUser);

  // MQTT Port input
  InputCard* mqttPortInput = dashboard.addInputCard("mqtt_port", "MQTT Port", "");
  mqttPortInput->setWeight(7);
  mqttPortInput->setValue(String(mqttPort));

  // Password input
  InputCard* mqttPassInput = dashboard.addInputCard("mqtt_pass", "MQTT Password", "");
  mqttPassInput->setWeight(8);
  mqttPassInput->inputType = "password";

  // Save button
  ButtonCardImpl* configBtn = dashboard.addButtonCard("save", "Settings", "Save & Restart", []() {
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
  // checkMqttConnection(mqtt, mqttBroker.c_str(), mqttPort, deviceName, mqttUser.c_str(),
  //                     mqttPass.c_str());
  // if (!mqtt.connected()) { reconnectMqtt(); }
  // mqtt.loop();
  // dashboard.loop();
  ts.execute();
}
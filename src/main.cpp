#include <TaskScheduler.h>
#include <Preferences.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <ESPDashboardPlus.h>
#include <PubSubClient.h>
#include <ezLED.h>
#include "dashboard_html.h"  // Auto-generated
#include "Credentials.h"
#define _DEBUG_  // Comment this line to disable debug prints
#include "Debug.h"
#include "DashboardUI.h"

constexpr const char* deviceName = "myESP32";

// Preferences for storing WiFi and MQTT credentials
Preferences prefs;

// Dashboard and server instances
AsyncWebServer server(80);
ESPDashboardPlus dashboard(deviceName);
DashboardUI dashboardUi;

// WiFi
String staSsid       = "";
String staPass       = "";
bool isApModeRunning = false;

// MQTT
String mqttBroker = "";
uint16_t mqttPort = 1883;
String mqttUser   = "";
String mqttPass   = "";

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

// Status LED
ezLED statusLed(LED_BUILTIN);

// Scheduler
Scheduler ts;

void checkWifiStatus();
void connectMqtt();
void reconnectMqtt();
Task tCheckWifiStatus(5 * TASK_MINUTE, TASK_FOREVER, &checkWifiStatus, &ts, true);
Task tConnectMqtt(TASK_IMMEDIATE, TASK_FOREVER, &connectMqtt, &ts, true);
Task tReconnectMqtt(5 * TASK_SECOND, TASK_FOREVER, &reconnectMqtt, &ts, false);
Task tStatusLed(TASK_IMMEDIATE, TASK_FOREVER, []() { statusLed.loop(); }, &ts, true);
Task tDashboardLoop(TASK_IMMEDIATE, TASK_FOREVER, []() { dashboard.loop(); }, &ts, true);

// ----- Functions ----- //
void checkWifiStatus() {
  if (WiFi.status() == WL_CONNECTED) {
    isApModeRunning = false;
    dashboard.updateStatusCard("wifiStatusId", StatusIcon::WIFI, CardVariant::SUCCESS,
                               "Connected: " + WiFi.SSID(), WiFi.localIP().toString());
  } else {
    dashboard.updateStatusCard("wifiStatusId", StatusIcon::WIFI, CardVariant::WARNING, "AP Mode",
                               "Status: " + String(WiFi.status()) + " | 192.168.4.1");

    if (!isApModeRunning) {
      isApModeRunning = true;
      _def("WiFi disconnected | Status: %d\n", WiFi.status());
      dashboard.logWarning("WiFi disconnected | Status: " + String(WiFi.status()));
      _def("Starting AP mode...\n");
      dashboard.logInfo("Starting AP mode...");
      WiFi.softAP(apSsid, apPassword);
    }
  }
}

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
    mqtt.subscribe("omg/OMG_ESP32_BLE/BTtoMQTT/A4C138C5BFA8");
    statusLed.blinkNumberOfTimes(300, 300, 3);
    dashboard.updateStatusCard("mqttStatusId", StatusIcon::WIFI, CardVariant::SUCCESS, "Connected",
                               mqttBroker);
    tReconnectMqtt.disable();
    tConnectMqtt.enable();
  } else {
    dashboard.updateStatusCard("mqttStatusId", StatusIcon::WIFI, CardVariant::WARNING,
                               "Disconnected", "State: " + String(mqtt.state()));
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  _def("Message arrived [ %s ]", topic);

  // // Print raw payload
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

// ----- Setup ----- //
void setup() {
  _serialBegin(115200);

  // Load saved credentials
  prefs.begin("config", true);
  staSsid    = prefs.getString("staSsid", "");
  staPass    = prefs.getString("staPass", "");
  mqttBroker = prefs.getString("mqttBroker", "");
  mqttUser   = prefs.getString("mqttUser", "");
  mqttPass   = prefs.getString("mqttPass", "");
  mqttPort   = prefs.getUInt("mqttPort", mqttPort);
  prefs.end();

  // Configure MQTT client
  mqtt.setServer(mqttBroker.c_str(), mqttPort);
  mqtt.setCallback(mqttCallback);
  mqtt.setBufferSize(512);  // Option: Adjust as needed based on expected message sizes

  // Try saved credentials or start AP
  if (staSsid.length() > 0) {
    WiFi.begin(staSsid.c_str(), staPass.c_str());
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      attempts++;
    }
  } else {
    _def("No saved WiFi credentials\n");
  }

  // Use mDNS for host name resolution.
  // You can use "http://<deviceName>.local" to access the device
  if (!MDNS.begin(deviceName)) { _def("Error setting up MDNS responder!\n"); }

  if (WiFi.status() != WL_CONNECTED) {
    _def("Starting AP mode...\n");
    WiFi.softAP(apSsid, apPassword);
    isApModeRunning = true;
  }

  // Initialize dashboard UI
  dashboardUi.init(dashboard, server, staSsid, mqttBroker, mqttPort, prefs);

  server.begin();

  // Add service to MDNS-SD
  // Without this call, the mDNS will be active for only a few minutes.
  MDNS.addService("http", "tcp", 80);
}

// ----- loop ----- //
void loop() {
  ts.execute();
}
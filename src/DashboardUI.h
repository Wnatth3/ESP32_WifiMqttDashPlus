#pragma once

#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include "dashboard_html.h"
#include "ESPDashboardPlus.h"
#include "Debug.h"

/**
 * DashboardUI - encapsulates the UI setup for the ESPDashboardPlus dashboard.
 *
 * Usage:
 *   DashboardUI ui;
 *   ui.init(dashboard, server,
 *           savedSSID, savedPassword,
 *           mqttBroker, mqttPort,
 *           mqttUser, mqttPass,
 *           prefs);
 */
class DashboardUI {
public:
  void init(ESPDashboardPlus& dashboard, AsyncWebServer& server, const String& savedSSID,
            const String& savedPassword, const String& mqttBroker, uint16_t mqttPort,
            const String& mqttUser, const String& mqttPass, Preferences& prefs) {
    // Begin the dashboard with embedded HTML payload
    dashboard.begin(&server, DASHBOARD_HTML_DATA, DASHBOARD_HTML_SIZE);

    // WiFi status card
    StatusCard* status = dashboard.addStatusCard("status", "WiFi Status", StatusIcon::WIFI);
    status->setWeight(1);
    if (WiFi.status() == WL_CONNECTED) {
      status->setStatus(StatusIcon::WIFI, CardVariant::SUCCESS, "Connected",
                        WiFi.localIP().toString());
      _def("Connected to WiFi: %s\n", WiFi.localIP().toString().c_str());
    } else {
      status->setStatus(StatusIcon::WIFI, CardVariant::WARNING, "AP Mode", "192.168.4.1");
    }

    // MQTT status placeholder
    StatusCard* mqttStatus =
      dashboard.addStatusCard("mqtt_status", "MQTT Status", StatusIcon::WIFI);
    mqttStatus->setWeight(2);

    // Input cards
    InputCard* ssidInput = dashboard.addInputCard("ssid", "WiFi SSID", savedSSID);
    ssidInput->setWeight(3);
    ssidInput->setValue(savedSSID);

    InputCard* passInput = dashboard.addInputCard("password", "WiFi Password", "");
    passInput->setWeight(4);
    passInput->inputType = "password";

    InputCard* mqttBrokerInput = dashboard.addInputCard("mqtt_broker", "MQTT Broker", "");
    mqttBrokerInput->setWeight(5);
    mqttBrokerInput->setValue(mqttBroker);

    InputCard* mqttPortInput = dashboard.addInputCard("mqtt_port", "MQTT Port", "");
    mqttPortInput->setWeight(6);
    mqttPortInput->setValue(String(mqttPort));

    InputCard* mqttUserInput = dashboard.addInputCard("mqtt_user", "MQTT User", "");
    mqttUserInput->setWeight(7);
    // mqttUserInput->setValue(mqttUser);

    InputCard* mqttPassInput = dashboard.addInputCard("mqtt_pass", "MQTT Password", "");
    mqttPassInput->setWeight(8);
    mqttPassInput->inputType = "password";

    // Save & Restart button
    ButtonCardImpl* configBtn =
      dashboard.addButtonCard("save", "Network Settings", "Save & Restart", [&dashboard, &prefs]() {
        prefs.begin("config", false);
        // Retrieve values from cards
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

    // Restart button
    ButtonCardImpl* restartBtn =
      dashboard.addButtonCard("restart", "Restart Device", "Restart", []() {
        _def("Restarting device...\n");
        delay(1000);
        ESP.restart();
      });
    restartBtn->setWeight(9);
  }
};

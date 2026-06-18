#pragma once

#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include "dashboard_html.h"
#include "ESPDashboardPlus.h"
#include "Debug.h"

/*
  DashboardUI - encapsulates the UI setup for the ESPDashboardPlus dashboard.
  Usage:
    DashboardUI dashboardUI;
    dashboardUI.init(dashboard, server, staSsid, mqttBroker, mqttPort, mqttSubscribeTopic, prefs);
*/

class DashboardUI {
public:
  void init(ESPDashboardPlus& dashboard, AsyncWebServer& server, const String& staSsid,
            const String& mqttBroker, uint16_t mqttPort, const String& mqttSubscribeTopic,
            Preferences& prefs) {
    // Begin the dashboard with embedded HTML payload
    dashboard.begin(&server, DASHBOARD_HTML_DATA, DASHBOARD_HTML_SIZE);

    // Add a card grpup
    dashboard.addGroup("configs", "Network Configuration");

    // WiFi status card
    StatusCard* wifiStatus =
      dashboard.addStatusCard("wifiStatusId", "WiFi Status", StatusIcon::WIFI);
    wifiStatus->setWeight(1);
    dashboard.addCardToGroup("configs", "wifiStatusId");
    if (WiFi.status() == WL_CONNECTED) {
      wifiStatus->setStatus(StatusIcon::WIFI, CardVariant::SUCCESS, "Connected: " + WiFi.SSID(),
                            WiFi.localIP().toString());
      _def("Connected to WiFi: %s\n", WiFi.localIP().toString().c_str());
    } else {
      wifiStatus->setStatus(StatusIcon::WIFI, CardVariant::WARNING, "AP Mode", "192.168.4.1");
    }

    // MQTT status placeholder
    StatusCard* mqttStatus =
      dashboard.addStatusCard("mqttStatusId", "MQTT Status", StatusIcon::WIFI);
    mqttStatus->setStatus(StatusIcon::WIFI, CardVariant::INFO, "Disconnected",
                          "Not setting up yet");
    mqttStatus->setWeight(2);
    dashboard.addCardToGroup("configs", "mqttStatusId");

    // Input cards
    InputCard* staSsidInput = dashboard.addInputCard("staSsidId", "WiFi SSID", "");
    staSsidInput->setWeight(3);
    staSsidInput->setValue(staSsid);
    dashboard.addCardToGroup("configs", "staSsidId");

    InputCard* staPassInput = dashboard.addInputCard("staPassId", "WiFi Password", "");
    staPassInput->setWeight(4);
    staPassInput->inputType = "password";
    dashboard.addCardToGroup("configs", "staPassId");

    InputCard* mqttBrokerInput = dashboard.addInputCard("mqttBrokerId", "MQTT Broker", "");
    mqttBrokerInput->setWeight(5);
    mqttBrokerInput->setValue(mqttBroker);
    dashboard.addCardToGroup("configs", "mqttBrokerId");

    InputCard* mqttPortInput = dashboard.addInputCard("mqttPortId", "MQTT Port", "");
    mqttPortInput->setWeight(6);
    mqttPortInput->setValue(String(mqttPort));
    dashboard.addCardToGroup("configs", "mqttPortId");

    InputCard* mqttUserInput = dashboard.addInputCard("mqttUserId", "MQTT User", "");
    mqttUserInput->setWeight(7);
    dashboard.addCardToGroup("configs", "mqttUserId");

    InputCard* mqttPassInput = dashboard.addInputCard("mqttPassId", "MQTT Password", "");
    mqttPassInput->setWeight(8);
    mqttPassInput->inputType = "password";
    dashboard.addCardToGroup("configs", "mqttPassId");

    InputCard* mqttTopicInput = dashboard.addInputCard("mqttTopicId", "MQTT Subscribe Topic", "");
    mqttTopicInput->setWeight(9);
    mqttTopicInput->setValue(mqttSubscribeTopic);
    dashboard.addCardToGroup("configs", "mqttTopicId");

    // Save & Restart button
    ButtonCardImpl* saveBtn = dashboard.addButtonCard(
      "saveBtnId", "Network Settings", "Save & Restart", [&dashboard, &prefs]() {
        prefs.begin("config", false);
        // Retrieve values from cards
        InputCard* ssid       = static_cast<InputCard*>(dashboard.getCard("staSsidId"));
        InputCard* pass       = static_cast<InputCard*>(dashboard.getCard("staPassId"));
        InputCard* mqttBroker = static_cast<InputCard*>(dashboard.getCard("mqttBrokerId"));
        InputCard* mqttPort   = static_cast<InputCard*>(dashboard.getCard("mqttPortId"));
        InputCard* mqttUser   = static_cast<InputCard*>(dashboard.getCard("mqttUserId"));
        InputCard* mqttPass   = static_cast<InputCard*>(dashboard.getCard("mqttPassId"));
        InputCard* mqttTopic  = static_cast<InputCard*>(dashboard.getCard("mqttTopicId"));
        prefs.putString("staSsid", ssid->value);
        prefs.putString("staPass", pass->value);
        prefs.putString("mqttBroker", mqttBroker->value);
        prefs.putUInt("mqttPort", mqttPort->value.toInt());
        prefs.putString("mqttUser", mqttUser->value);
        prefs.putString("mqttPass", mqttPass->value);
        prefs.putString("mqttTopic", mqttTopic->value);
        prefs.end();
        delay(1000);
        ESP.restart();
      });
    saveBtn->setVariant(CardVariant::SUCCESS);
    saveBtn->setWeight(10);
    dashboard.addCardToGroup("configs", "saveBtnId");

    // Reset network settings button
    ActionButton* resetBtn = dashboard.addActionButton(
      "resetBtnId", "Reset Network Settings", "Reset & restart", "Reset Network Settings?",
      "All WiFi & MQTT settings will be removed and device will restart.", [&prefs]() {
        _def("Resetting network settings...\n");
        prefs.begin("config", false);
        prefs.clear();
        prefs.end();
        delay(1000);
        ESP.restart();
      });
    resetBtn->setVariant(CardVariant::DANGER);
    resetBtn->setWeight(11);
    dashboard.addCardToGroup("configs", "resetBtnId");

    // Restart button
    ButtonCardImpl* restartBtn =
      dashboard.addButtonCard("restartBtnId", "Restart Device", "Restart", []() {
        _def("Restarting device...\n");
        delay(1000);
        ESP.restart();
      });
    restartBtn->setVariant(CardVariant::WARNING);
    restartBtn->setWeight(12);
    dashboard.addCardToGroup("configs", "restartBtnId");
  }
};

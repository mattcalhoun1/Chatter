#include "OnboardUdpChannel.h"

void OnboardUdpChannel::setupWifi() {
  Serial.println("Checking wifi module status");
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("Communication with Onboard WiFi module failed!");
  }
  else {
    Serial.println("Onboard Wifi Module Status: " + String(WiFi.status()));

    // print your MAC address:
    byte mac[6];
    WiFi.macAddress(mac);
    Serial.print("MAC: ");
    printMacAddress(mac);  
    wifiEnabled = true;
  }
}
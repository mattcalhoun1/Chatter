#include "AirliftUdpChannel.h"

void AirliftUdpChannel::setupWifi () {
  #ifdef AIRLIFT_ONBOARD
  WiFi.setPins(this->ssPin, this->ackPin, this->resetPin, this->gpi0, &SPI);  
  Serial.println("Checking AirLift WiFi module status");
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with AirLift WiFi module failed, status: " + String(WiFi.status()));
  }
  else {
    Serial.println("AirLift Wifi Module Status: " + String(WiFi.status()));
    //String fv = WiFi.firmwareVersion();
    //if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    //  Serial.println("Please upgrade the firmware");
    //}

    // print your MAC address:
    byte mac[6];
    WiFi.macAddress(mac);
    Serial.print("MAC: ");
    printMacAddress(mac);  
    wifiEnabled = true;
  }
  #endif
}
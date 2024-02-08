#include "UdpChannel.h"

UdpChannel::UdpChannel (uint8_t channelNum, const char* udpHostName, const char* wifiString, bool logEnabled, ChatStatusCallback* chatStatusCallback) {
    this->channelNum = channelNum;
    memcpy(hostName, udpHostName, strlen(udpHostName));
    hostName[strlen(udpHostName)] = '\0';

    this->logEnabled = logEnabled;
    this->chatStatusCallback = chatStatusCallback;

    bool completeSsid = false;
    int ssidPosition = 0;
    int pwPosition = 0;
    for (int i = 0; i < strlen(wifiString) && wifiString[i] != '\0'; i++) {
      if (wifiString[i] == ENCRYPTION_CRED_DELIMITER) {
        completeSsid = true;
      }
      else if (completeSsid) {
        if (pwPosition < WIFI_PASSWORD_MAX_LEN) {
          password[pwPosition++] = wifiString[i];
        }
        else {
          validWifiConfig = false;
          logConsole("ERROR: WIFI password too long for buffer!");
        }
      }
      else {
        if (ssidPosition < WIFI_SSID_MAX_LEN) {
          ssid[ssidPosition++] = wifiString[i];
        }
        else {
          validWifiConfig = false;
          logConsole("ERROR: WIFI ssid TOO long for buffer!");
        }
      }
    }
    ssid[ssidPosition] = '\0';
    password[pwPosition] = '\0';
}

bool UdpChannel::init() {
  if (validWifiConfig) {
    setupWifi();
    wifiConnect();
  }
  else {
    logConsole("Bad wifi config!");
    return false;
  }

  return wifiConnected;
}

bool UdpChannel::hasMessage () {
    return checkUdpInput();
}

bool UdpChannel::fireAndForget(uint8_t *message, int length, uint8_t address) {
    // for now, all udp is fire and forget, so just do a normal send
    return send(message, length, address);
}


bool UdpChannel::retrieveMessage () {
    // by the time we realize we have a packet, we've already received it.
    // possibly addd chunking logic here?
    return true;
}

uint8_t UdpChannel::getLastSender () {
    return lastSender;
}

uint8_t UdpChannel::getSelfAddress () {
  return WiFi.localIP()[3];
}

uint8_t UdpChannel::getAddress (const char* otherDeviceId) {
  return multicastServer[3];
}

ChatterMessageType UdpChannel::getMessageType () {
    return MessageTypeText;
}

int UdpChannel::getMessageSize () {
    return lastMessageSize;
}

const char* UdpChannel::getTextMessage () {
    return (char*)buf;
}

const uint8_t* UdpChannel::getRawMessage () {
    return buf;
}


bool UdpChannel::broadcast (String message) {
  if(refreshWifiStatus()) {
    chatStatusCallback->updateChatStatus(channelNum, ChatSending);

    // Initialize packet buffer to zeros
    memset(packetBuffer, 0, UDP_MAX_MESSAGE_LEN);
    const char* msg_cstr = message.c_str();

    for (int i = 0; i < message.length(); i++) {
        packetBuffer[i] = (byte)msg_cstr[i];
    }

    // broadcast # of times
    for (int i = 0; i < UDP_BROADCAST_COUNT; i++) {
      Udp.beginPacket(multicastServer, localPort); 
      Udp.write(packetBuffer, UDP_MAX_MESSAGE_LEN);
      Udp.endPacket();

      if (i + 1 < UDP_BROADCAST_COUNT) {
        delay(UDP_BROADCAST_DELAY);
      }
      //Serial.println("6");

    }
    logConsole("UDP broadcast sent");
    chatStatusCallback->updateChatStatus(channelNum, ChatSent);
    return true;
  }
  else {
    logConsole("Broadcast failed, wifi not connected!");
    return false;
  }
}

bool UdpChannel::broadcast(uint8_t *message, uint8_t length) {
  if(refreshWifiStatus()) {
    chatStatusCallback->updateChatStatus(channelNum, ChatSending);

    // Initialize packet buffer to zeros
    memset(packetBuffer, 0, UDP_MAX_MESSAGE_LEN);

    for (int i = 0; i < length; i++) {
        packetBuffer[i] = (byte)message[i];
    }

    // broadcast # of times
    for (int i = 0; i < UDP_BROADCAST_COUNT; i++) {
      Udp.beginPacket(multicastServer, localPort); 
      Udp.write(packetBuffer, length);
      Udp.endPacket();

      if (i + 1 < UDP_BROADCAST_COUNT) {
        delay(UDP_BROADCAST_DELAY);
      }
    }

    logConsole("UDP broadcast sent...");
    chatStatusCallback->updateChatStatus(channelNum, ChatSent);
    return true;
  }
  else {
    logConsole("Broadcast failed, wifi not connected!");
    return false;
  }
}


bool UdpChannel::send(String message, uint8_t address) {
    //return lora->send(message, address);
    return broadcast(message);
}

bool UdpChannel::send(uint8_t *message, int length, uint8_t address) {
  return broadcast(message, length);
}

bool UdpChannel::isEnabled () {
    return true; //? check a switch?
}

bool UdpChannel::isConnected () {
    return refreshWifiStatus();
}

// check swtiches for these?
bool UdpChannel::isEncrypted () {
    return true;
}

bool UdpChannel::isSigned () {
    return true;
}

bool UdpChannel::checkUdpInput () {
  if (refreshWifiStatus()) {
    bool havePacket = false;
    int receives = 0;
    while (havePacket == false && receives++ < UDP_RECEIVE_COUNT) {
      havePacket = Udp.parsePacket();
      if (!havePacket) {
        delay(UDP_RECEIVE_DELAY);
      }
    }
    
    if (havePacket) {
      chatStatusCallback->updateChatStatus(channelNum, ChatReceiving);

      // We've received a packet, read the data from it
      int bytesRead = Udp.read(packetBuffer, UDP_MAX_MESSAGE_LEN); // read the packet into the buffer
      for (int i = 0; i < bytesRead; i++) {
          buf[i] = (uint8_t)packetBuffer[i];
      }
      
      if ((char)buf[0] == UDP_PREFIX[0]) {
        lastMessageSize = bytesRead;
        logConsole("Received message of " + String(lastMessageSize) + " bytes from " + String(Udp.remoteIP()[3]));
        lastSender =  Udp.remoteIP()[3];
        chatStatusCallback->updateChatStatus(channelNum, ChatReceived);
        return true;
      }
    }
  }
  else {
    logConsole("Check for input failed, wifi not connected!");
  }

  return false;
}

bool UdpChannel::refreshWifiStatus () {
  if (wifiConnected == false || WiFi.status() != WL_CONNECTED) {
    logConsole("Wifi status was: " + String(WiFi.status()));
    wifiConnect();
  }

  if (!wifiConnected) {
    chatStatusCallback->updateChatStatus(channelNum, ChatDisconnected);
  }
  else {
    chatStatusCallback->updateChatStatus(channelNum, ChatConnected);
  }
  return wifiConnected;
}

void UdpChannel::wifiConnect () {
  //listNetworks();
  //commander->getEncryptor()->

  // attempt to connect to Wifi network:
  wifiConnected = false;
  if (wifiEnabled) {
    chatStatusCallback->updateChatStatus(channelNum, ChatConnecting);

    int maxRetries = 5;
    int retryCount = 0;
    int status = WiFi.status();

    while (status != WL_CONNECTED && retryCount++ < maxRetries) {
      logConsole("Attempting to connect to WPA SSID: ");
      logConsole(ssid);

      // Connect to WPA/WPA2 network:
      status = WiFi.begin(ssid, password);

      delay(10000);// allow time for connection
    }
    wifiConnected = status == WL_CONNECTED;

    if (wifiConnected) {
      logConsole("WiFi Connection Successful!");
      logConsole("IP: " + String(WiFi.localIP()[0]) + "." + String(WiFi.localIP()[1]) + "." + String(WiFi.localIP()[2]) + "." + String(WiFi.localIP()[3]));

      // enable udp
        int udpStatus = 0;
      retryCount = 0;
      while (udpStatus == 0 && retryCount++ <= maxRetries) {
        udpStatus = Udp.beginMulticast(multicastServer, localPort);
        if (udpStatus == 0) {
          Udp.stop();
        }
        logConsole("UDP Status: " + String(udpStatus));

        if (udpStatus == 0) {
          delay(1000); // wait half a second
        }
      }
      if (udpStatus == 1) {
        chatStatusCallback->updateChatStatus(channelNum, ChatConnected);
        logConsole("UDP Connected.");
      }
      else {
        chatStatusCallback->updateChatStatus(channelNum, ChatDisconnected);
        wifiConnected = false;
        logConsole("UDP NOT connected.");
      }
    }
    else {
      chatStatusCallback->updateChatStatus(channelNum, ChatFailed);
      chatStatusCallback->updateChatStatus(channelNum, ChatDisconnected);
      logConsole("WiFi Connection failed.");
    }
  }
  else {
    logConsole ("WiFi Connection not attempted.");
  }
}

void UdpChannel::listNetworks() {
  // scan for nearby networks:
  Serial.println("** Scan Networks **");
  int numSsid = WiFi.scanNetworks();
  if (numSsid == -1) {
    Serial.println("Couldn't get a wifi connection");
  }
  else {
    // print the list of networks seen:
    Serial.print("number of available networks:");
    Serial.println(numSsid);

    // print the network number and name for each network found:
    for (int thisNet = 0; thisNet < numSsid; thisNet++) {
      Serial.print(thisNet);
      Serial.print(") ");
      Serial.print(WiFi.SSID(thisNet));
      Serial.print("\tSignal: ");
      Serial.print(WiFi.RSSI(thisNet));
      Serial.print(" dBm");
      //Serial.print("\tEncryption: ");
      //printEncryptionType(WiFi.encryptionType(thisNet));
    }
  }
}

void UdpChannel::printMacAddress(byte mac[]) {
  for (int i = 5; i >= 0; i--) {
    if (mac[i] < 16) {
      Serial.print("0");
    }
    Serial.print(mac[i], HEX);
    if (i > 0) {
      Serial.print(":");
    }
  }
  Serial.println();
}

void UdpChannel::printWifiData() {
  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  IPAddress gatewayIp = WiFi.gatewayIP();
  Serial.println(gatewayIp);

  // print your MAC address:
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
  printMacAddress(mac);
}


void UdpChannel::printCurrentNet() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print the MAC address of the router you're attached to:
  byte bssid[6];
  WiFi.BSSID(bssid);
  Serial.print("BSSID: ");
  printMacAddress(bssid);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.println(rssi);

  // print the encryption type:
  byte encryption = WiFi.encryptionType();
  Serial.print("Encryption Type:");
  Serial.println(encryption, HEX);
  Serial.println();
}

// send an NTP request to the time server at the given address

unsigned long UdpChannel::sendNTPpacket(IPAddress& address) {
    int NTP_PACKET_SIZE = 48;
    //Serial.println("1");
    // set all bytes in the buffer to 0
    memset(packetBuffer, 0, UDP_PACKET_SIZE);
    // Initialize values needed to form NTP request
    // (see URL above for details on the packets)
    //Serial.println("2");
    packetBuffer[0] = 0b11100011;   // LI, Version, Mode
    packetBuffer[1] = 0;     // Stratum, or type of clock
    packetBuffer[2] = 6;     // Polling Interval
    packetBuffer[3] = 0xEC;  // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    packetBuffer[12]  = 49;
    packetBuffer[13]  = 0x4E;
    packetBuffer[14]  = 49;
    packetBuffer[15]  = 52;
    //Serial.println("3");
    // all NTP fields have been given values, now
    // you can send a packet requesting a timestamp:
    Udp.beginPacket(address, 123); //NTP requests are to port 123
    //Serial.println("4");
    Udp.write(packetBuffer, NTP_PACKET_SIZE);
    //Serial.println("5");
    Udp.endPacket();
    //Serial.println("6");
    logConsole("NTP request sent...");
}

void UdpChannel::sendTimeRequest () {

  sendNTPpacket(timeServer); // send an NTP packet to a time server
}

void UdpChannel::logConsole (String msg) {
    if (CHAT_LOG_ENABLED) {
        Serial.println(msg);
    }
}
// manages communication across various channels (lora, wifi/udp, etc)
#include <Arduino.h>
#include "ChatGlobals.h"
#include "ChatterChannel.h"
#include "LoRaChannel.h"
#include "UdpChannel.h"
#include "AirliftUdpChannel.h"
#include "OnboardUdpChannel.h"
//#include "CANChannel.h"
#include "UARTChannel.h"
#include "../encryption/Encryptor.h"
//#include "../encryption/ChaChaEncryptor.h"
#include "../encryption/ArdAteccHsm.h"
#include "../encryption/SparkAteccHsm.h"
#include "../encryption/Hsm.h"
#include "ChatStatusCallback.h"
#include "ChatterPacket.h"
#include "../storage/PacketStore.h"
#include "../storage/TrustStore.h"
#include "../rtc/RTClockBase.h"

#ifndef CHATTER_H
#define CHATTER_H

enum ChatterMode {
  BasicMode = 0,
  BridgeMode = 1
};

enum ChatterDeviceType {
  ChatterDeviceUnknown = 0,
  ChatterDeviceBridgeLora = 1,
  ChatterDeviceBridgeWifi = 2,
  ChatterDeviceBridgeCloud = 3,
  ChatterDeviceCommunicator = 4,
  ChatterDeviceRaw = 5
};

class Chatter : ChatStatusCallback {
  public:
    Chatter(ChatterDeviceType _deviceType, ChatterMode _mode, PacketStore* _packetStore, TrustStore* _trustStore, RTClockBase* _rtc, ChatStatusCallback* _statusCallback) {deviceType = _deviceType, mode = _mode, packetStore = _packetStore; trustStore =_trustStore; rtc = _rtc, statusCallback = _statusCallback; }
    bool init ();
    ChatterDeviceType getDeviceType() { return deviceType; }

    void addLoRaChannel (int csPin, int intPin, int rsPin, bool logEnabled);
    void addAirliftUdpChannel (int ssPin, int ackPin, int resetPin, int gpi0, bool logEnabled);
    void addOnboardUdpChannel (bool logEnabled);
    void addUartChannel (bool logEnabled);
    //void addCANChannel (bool logEnabled);

    // gets current transient numeric address of given device id
    uint8_t getAddress(const char* deviceId);
    uint8_t getSelfAddress ();

    int getNumChannels();
    void setDefaultChannel(int channelNum) {defaultChannel = channelNum;}
    ChatStatus getChatStatus(int channelNum) { return status[channelNum]; }
    ChatterChannel* getChannel (int channelNum);
    const char* getDeviceId () {return deviceId; }

    bool hasMessage ();
    bool retrieveMessage ();
    const char* getLastSender ();
    const char* getLastRecipient ();
    ChatterChannel* getLastChannel ();
    ChatterMessageType getMessageType ();
    int getMessageSize ();
    const char* getTextMessage ();
    const uint8_t* getRawMessage ();

    void ingestPacketMetadata (const char* rawPacket, int messageSize);

    // broadcast text and raw messagse
    bool broadcast (String message);
    bool broadcast(uint8_t *message, uint8_t length);

    bool send(uint8_t *message, int length, const char* recipientDeviceId);
    bool send(uint8_t *message, int length, const char* recipientDeviceId, ChatterMessageFlags* flags);

    bool sendViaIntermediary(uint8_t *message, int length, const char* recipientDeviceId, const char* intermediaryDeviceId);
    bool sendViaIntermediary(uint8_t *message, int length, const char* recipientDeviceId, const char* intermediaryDeviceId, ChatterMessageFlags* flags);

    bool send(uint8_t *message, int length, const char* recipientDeviceId, ChatterChannel* channel);
    bool send(uint8_t *message, int length, const char* recipientDeviceId, ChatterMessageFlags* flags, ChatterChannel* channel);
    bool sendViaIntermediary(uint8_t *message, int length, const char* recipientDeviceId, const char* intermediaryDeviceId, ChatterMessageFlags* flags, ChatterChannel* channel);

    bool isRunning () {return running;}

    void updateChatStatus (uint8_t channelNum, ChatStatus newStatus);

    bool selfAnnounce (bool force);

    long getRandom ();

    // chatter packet level
    const uint8_t* getLastPacketMessageId ();
    const uint8_t* getLastPacketChunkId ();
    ChatterPacketEncryptionFlag getLastPacketEncryptionFlag ();
    ChatterMessageFlags getMessageFlags () { return receiveBuffer.flags; }

    Encryptor* getEncryptor() {return encryptor;}
    TrustStore* getTrustStore() {return trustStore;}
    RTClockBase* getRtc() {return rtc;}

  private:
    ChatterDeviceType deviceType;
    ChatterMode mode;
    bool running = false;

    void ingestPacketMetadata (ChatterChannel* channel);
    void primeSendBuffer (const char* recipientDeviceId, ChatterChannel* channel, bool isSigned, bool isHeader, bool isFooter, char* messageId, char* chunkId);
    int populateSendBufferContent (uint8_t* message, int length, ChatterChannel* channel, bool isMetadata);

    void generateMessageId (char* messageIdBuffer);

    int generateFooter (const char* recipientDeviceId, char* messageId, uint8_t* message, int messageLength);
    int generateHeader (const char* recipientDeviceId, char* messageId, ChatterMessageFlags* flags);

    // these methods all operate on the message sitting in the receive buffer, assuming header+content is all in receiveBuffer.content
    bool isExpired();
    bool validateSignature();
    bool validateSignature(bool checkHash);
    void populateReceiveBufferFlags ();

    int numChannels = 0;
    bool mirror = false; // whether to mirror messages across multiple channels
    bool stripe = false; // whether to stripe messages across multiple channels

    Hsm* hsm;
    ChatterChannel* getDefaultChannel ();
    ChatterChannel* hotChannel = 0; // one with newest message

    ChatterChannel* channels[2];
    PacketStore* packetStore;
    TrustStore* trustStore;
    Encryptor* encryptor;
    RTClockBase* rtc;

    char deviceId[CHATTER_DEVICE_ID_SIZE+1];
    char ssid[WIFI_SSID_MAX_LEN + WIFI_PASSWORD_MAX_LEN + 2]; // +2 for delimiter and terminator
    float loraFrequency;

    void logConsole(String msg);
    ChatStatusCallback* statusCallback;

    ChatStatus status[CHAT_MAX_CHANNELS]; // 0 is lora, 1 is udp
    uint8_t sendBuffer[CHATTER_FULL_BUFFER_LEN];
    uint8_t fullSignBuffer[CHATTER_FULL_MESSAGE_BUFFER_SIZE + CHATTER_HEADER_SIZE]; // holds entire content to be signed, including header
    uint8_t footerBuffer[CHATTER_FOOTER_BUFFER_SIZE]; // shared by receive and send, for slightly differnt purposes
    uint8_t hashBuffer[CHATTER_HASH_SIZE]; // used when generating hashes for footer
    uint8_t headerBuffer[CHATTER_HEADER_BUFFER_SIZE];
    char senderPublicKey[ENC_PUB_KEY_SIZE];
    
    //uint8_t receiveBuffer[UDP_MAX_MESSAGE_LEN];
    //int receiveBufferSize;
    ChatterMessageType receiveBufferMessageType;
    ChatterPacket receiveBuffer;

    unsigned long selfAnnounceFrequency = 10000; // every n millis, announce self across all channels
    unsigned long lastAnnounce = 0;
    int defaultChannel = 0; // wifi or lora ... NOT can

};

#endif
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
#include "../encryption/Hsm.h"
#include "../encryption/PseudoHsm.h"
#include "ChatStatusCallback.h"
#include "ChatterPacket.h"
#include "../storage/PacketStore.h"
#include "../storage/TrustStore.h"
#include "../storage/ClusterStore.h"
#include "../storage/fram/FramTrustStore.h"
#include "../storage/DeviceStore.h"
#include "../storage/LicenseStore.h"
#include "../storage/fram/FramDeviceStore.h"
#include "../storage/fram/FramLicenseStore.h"
#include "../storage/fram/FramClusterStore.h"
#include "../storage/fram/FramPacketStore.h"
#include "../storage/fram/CachingFramDatastore.h"

#include "../rtc/RTClockBase.h"

#if !defined(ARDUINO_UNOR4_WIFI)
#include <ArduinoUniqueID.h>
#endif

#ifndef CHATTER_H
#define CHATTER_H

enum ChatterMode {
  BasicMode = 0,
  BridgeMode = 1
};

enum StorageType {
    StorageSD = 0,
    StorageFramSPI = 1,
    StorageFramI2C = 2
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
    Chatter(ChatterDeviceType _deviceType, ChatterMode _mode, RTClockBase* _rtc, StorageType _packetStorageType, ChatStatusCallback* _statusCallback) {deviceType = _deviceType, mode = _mode, rtc = _rtc, packetStorageType = _packetStorageType, statusCallback = _statusCallback; }
    bool init (const char* devicePassword = nullptr);
    ChatterDeviceType getDeviceType() { return deviceType; }

    bool isDeviceInitialized () { return deviceInitialized; } // indicates if the device fram has been initialized (device has alias/etc)

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
    const char* getClusterId () {return clusterId; }
    const char* getDeviceAlias () {return deviceAlias;}
    const char* getClusterAlias () {return clusterAlias;}

    bool hasMessage ();
    bool retrieveMessage ();
    const char* getLastSender ();
    const char* getLastRecipient ();
    ChatterChannel* getLastChannel ();
    ChatterMessageType getMessageType ();
    int getMessageSize ();
    const char* getTextMessage ();
    const uint8_t* getRawMessage ();

    bool factoryReset ();

    void ingestPacketMetadata (const char* rawPacket, int messageSize);

    // broadcast text and raw messagse
    bool broadcast (String message);
    bool broadcast(uint8_t *message, uint8_t length);
    bool broadcast(uint8_t *message, int length, ChatterMessageFlags* flags);
    bool broadcastUnencrypted(uint8_t *message, int length, ChatterMessageFlags* flags);

    bool send(uint8_t *message, int length, const char* recipientDeviceId);
    bool send(uint8_t *message, int length, const char* recipientDeviceId, ChatterMessageFlags* flags);
    bool sendUnencrypted(uint8_t *message, int length, const char* recipientDeviceId, ChatterMessageFlags* flags);

    bool sendViaIntermediary(uint8_t *message, int length, const char* recipientDeviceId, const char* intermediaryDeviceId);
    bool sendViaIntermediary(uint8_t *message, int length, const char* recipientDeviceId, const char* intermediaryDeviceId, ChatterMessageFlags* flags);

    bool send(uint8_t *message, int length, const char* recipientDeviceId, ChatterChannel* channel);
    bool send(uint8_t *message, int length, const char* recipientDeviceId, ChatterMessageFlags* flags, ChatterChannel* channel);
    bool sendViaIntermediary(uint8_t *message, int length, const char* recipientDeviceId, const char* intermediaryDeviceId, ChatterMessageFlags* flags, ChatterChannel* channel, bool forceUnencrypted = false, bool isBroadcast = false);

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
    PacketStore* getPacketStore() {return packetStore;}
    DeviceStore* getDeviceStore() {return deviceStore;}
    LicenseStore* getLicenseStore() {return licenseStore;}
    ClusterStore* getClusterStore() {return clusterStore;}
    RTClockBase* getRtc() {return rtc;}
    Hsm* getHsm () {return hsm;}

    void logDebugInfo ();
    bool isRootDevice (const char* deviceId);

  private:
    ChatterDeviceType deviceType;
    ChatterMode mode;
    bool running = false;
    bool deviceInitialized = false;

    void ingestPacketMetadata (ChatterChannel* channel);
    void primeSendBuffer (const char* recipientDeviceId, ChatterChannel* channel, bool isSigned, bool isHeader, bool isFooter, char* messageId, char* chunkId,  bool forceUnencrypted);
    int populateSendBufferContent (uint8_t* message, int length, ChatterChannel* channel, bool isMetadata, bool forceUnencrypted);

    bool prepareBuffersForSendDeviceInfo ();

    void generateMessageId (char* messageIdBuffer);

    int generateFooter (const char* recipientDeviceId, char* messageId, uint8_t* message, int messageLength);
    int generateHeader (const char* recipientDeviceId, char* messageId, ChatterMessageFlags* flags);

    // these methods all operate on the message sitting in the receive buffer, assuming header+content is all in receiveBuffer.content
    bool isExpired();
    bool validateSignature();
    bool validateSignature(bool checkHash);

    bool isSenderKnown (const char* senderId);

    bool sendDeviceInfo (const char* targetDeviceId, bool requestBack);
    bool broadcastDeviceInfo (bool requestBack);
    bool receiveDeviceInfo (bool isExchange);

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
    DeviceStore* deviceStore;
    LicenseStore* licenseStore;
    ClusterStore* clusterStore;
    Encryptor* encryptor;
    RTClockBase* rtc;

    char defaultClusterId[STORAGE_GLOBAL_NET_ID_SIZE + STORAGE_LOCAL_NET_ID_SIZE + 1]; 

    char deviceId[CHATTER_DEVICE_ID_SIZE+1];
    char clusterId[STORAGE_GLOBAL_NET_ID_SIZE + STORAGE_LOCAL_NET_ID_SIZE + 1]; 
    char clusterAlias[CHATTER_ALIAS_NAME_SIZE + 1];
    char wifiSsid[CHATTER_WIFI_STRING_MAX_SIZE];
    char wifiCred[CHATTER_WIFI_STRING_MAX_SIZE];
    ClusterChannel clusterPreferredChannel;
    ClusterChannel clusterSecondaryChannel;
    float loraFrequency;

    void logConsole(String msg);
    ChatStatusCallback* statusCallback;

    ChatStatus status[CHAT_MAX_CHANNELS]; // 0 is lora, 1 is udp
    uint8_t sendBuffer[CHATTER_FULL_BUFFER_LEN];
    uint8_t fullSignBuffer[CHATTER_FULL_MESSAGE_BUFFER_SIZE + CHATTER_HEADER_SIZE]; // holds entire content to be signed, including header
    uint8_t footerBuffer[CHATTER_FOOTER_BUFFER_SIZE]; // shared by receive and send, for slightly differnt purposes
    uint8_t hashBuffer[CHATTER_HASH_SIZE]; // used when generating hashes for footer
    uint8_t headerBuffer[CHATTER_HEADER_BUFFER_SIZE];
    uint8_t senderPublicKey[ENC_PUB_KEY_SIZE];
    
    //uint8_t receiveBuffer[UDP_MAX_MESSAGE_LEN];
    //int receiveBufferSize;
    ChatterMessageType receiveBufferMessageType;
    ChatterPacket receiveBuffer;

    unsigned long selfAnnounceFrequency = 10000; // every n millis, announce self across all channels
    unsigned long lastAnnounce = 0;
    int defaultChannel = 0; // wifi or lora ... NOT can
    StorageType packetStorageType;

    FramData* fram; // only set if one of the data storage selections is fram

    char framEncryptionKey[33]; // may be up to 32 chars (plus term)
    uint8_t framEncryptionKeySize;
    bool setupFramEncryptionKey (const char* devicePassword);
    void clearEncryptionKeyBuffer ();
    bool deviceStoreInitialized ();
    bool loadClusterConfig(const char* clusterId);

    // for key exchange
    uint8_t licenseBuffer[ENC_SIGNATURE_SIZE];
    uint8_t pubKeyBuffer[ENC_SIGNATURE_SIZE];
    char licenseSigner[CHATTER_DEVICE_ID_SIZE];
    uint8_t internalMessageBuffer[CHATTER_INTERNAL_MESSAGE_BUFFER_SIZE];
    char clusterAliasBuffer[CHATTER_ALIAS_NAME_SIZE];
    char deviceAliasBuffer[CHATTER_ALIAS_NAME_SIZE];
    char deviceAlias[CHATTER_ALIAS_NAME_SIZE+1];
    char clusterBroadcastId[CHATTER_DEVICE_ID_SIZE + 1];
};

#endif
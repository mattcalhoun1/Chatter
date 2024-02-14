#include <Arduino.h>
#include "../chat/ChatterPacket.h"
#include "../encryption/Encryptor.h"
#include "StorageGlobals.h"
#include "StorageBase.h"

#ifndef PACKETSTORE_H
#define PACKETSTORE_H
// abstract base class for storing packet data
class PacketStore : public StorageBase {
    public:
        virtual bool init () = 0;

        virtual bool setReceived (const char* senderId, const char* messageId, int packetNum) = 0;
        virtual bool wasReceived (const char* senderId, const char* messageId, int packetNum) = 0;

        // messages, list/get/save
        virtual bool savePacket (ChatterPacket* packet) = 0;
        virtual bool savePacket (ChatterPacket* packet, Encryptor* encryptor) = 0;
        virtual bool saveMessageTimestamp (const char* senderId, const char* messageId, const char* sortableTime) = 0;
        virtual bool saveAirMessageTimestamp (const char* senderId, const char* messageId, const char* sortableTime) = 0;
        virtual bool saveBridgeMessageTimestamp (const char* senderId, const char* messageId, const char* sortableTime) = 0;

        virtual bool clearMessage (const char* sender, const char* messageId) = 0;
        virtual bool clearAllMessages () = 0;

        //virtual int generateHash (const char* senderId, const char* messageId, uint8_t* hashBuffer) = 0;
        //virtual int generateHash (const char* senderId, const char* messageId, uint8_t* hashBuffer, Encryptor* encryptor) = 0;

        virtual int getNumPackets (const char* senderId, const char* messageId) = 0;
        virtual int readPacket (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer) = 0;
        virtual int readPacket (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer, int maxLength) = 0;
        virtual int readPacket (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer, Encryptor* encryptor) = 0;
        virtual int readPacket (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer, int maxLength, Encryptor* encryptor) = 0;
        virtual int readMessage (const char* senderId, const char* messageId, uint8_t* buffer, int maxLength, Encryptor* encryptor) = 0;

        virtual bool hashMatches (const char* senderId, const char* messageId) = 0;
        virtual bool hashMatches (const char* senderId, const char* messageId, Encryptor* encryptor) = 0;

        virtual bool moveMessageToAirOut (const char* sender, const char* messageId) = 0;
        virtual bool moveMessageToBridgeOut (const char* sender, const char* messageId) = 0;

        virtual int readPacketFromAirOut (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer, int maxLength, Encryptor* encryptor) = 0;
        virtual int readPacketFromBridgeOut (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer, int maxLength, Encryptor* encryptor) = 0;
        virtual bool clearPacketFromAirOut (const char* senderId, const char* messageId, int packetNum) = 0;
        virtual bool clearPacketFromBridgeOut (const char* senderId, const char* messageId, int packetNum) = 0;

        virtual bool readNextAirOutPacketDetails (ChatterPacketMetaData* packetBuffer) = 0;
        virtual bool readNextBridgeOutPacketDetails (ChatterPacketMetaData* packetBuffer) = 0;

        virtual int pruneAgedMessages (const char* oldestDatetime) = 0;

};

#endif
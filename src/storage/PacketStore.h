#include <Arduino.h>
#include "../chat/ChatterPacket.h"
#include "StorageGlobals.h"
#include "StorageBase.h"
#include <SHA256.h>

#ifndef PACKETSTORE_H
#define PACKETSTORE_H
// abstract base class for storing packet data
class PacketStore : public StorageBase {
    public:
        virtual bool init () = 0;
        virtual bool wasReceived (const char* senderId, const char* messageId, int packetNum) = 0;

        // messages, list/get/save
        virtual bool savePacket (ChatterPacket* packet) = 0;
        //virtual bool saveMessageTimestamp (const char* senderId, const char* messageId, const char* sortableTime) = 0;
        virtual bool saveAirMessageTimestamp (const char* senderId, const char* messageId, const char* sortableTime) = 0;
        virtual bool saveBridgeMessageTimestamp (const char* senderId, const char* messageId, const char* sortableTime) = 0;

        virtual bool clearAllMessages () = 0;

        virtual int getNumPackets (const char* senderId, const char* messageId) = 0;
        virtual int readPacket (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer) = 0;
        virtual int readPacket (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer, int maxLength) = 0;
        virtual int readMessage (const char* senderId, const char* messageId, uint8_t* buffer, int maxLength);

        virtual int generateHash (const char* senderId, const char* messageId, uint8_t* hashBuffer);
        virtual bool hashMatches (const char* senderId, const char* messageId);

        virtual bool moveMessageToAirOut (const char* sender, const char* messageId) = 0;
        virtual bool moveMessageToBridgeOut (const char* sender, const char* messageId) = 0;
        virtual bool moveMessageToQuarantine (const char* sender, const char* messageId) = 0;

        virtual int readPacketFromAirOut (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer, int maxLength) = 0;
        virtual int readPacketFromBridgeOut (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer, int maxLength) = 0;
        virtual bool clearPacketFromAirOut (const char* senderId, const char* messageId, int packetNum) = 0;
        virtual bool clearPacketFromBridgeOut (const char* senderId, const char* messageId, int packetNum) = 0;

        virtual bool readNextAirOutPacketDetails (ChatterPacketMetaData* packetBuffer) = 0;
        virtual bool readNextBridgeOutPacketDetails (ChatterPacketMetaData* packetBuffer) = 0;

        virtual int pruneAgedMessages (const char* oldestDatetime) = 0;
    protected:
        SHA256 hasher;
        uint8_t hashCalcBuffer[STORAGE_HASH_LENGTH]; // buffer for when this class calcs hash    
        uint8_t filePacketBuffer[STORAGE_CONTENT_BUFFER_SIZE]; // use this buffer to read footer

};

#endif
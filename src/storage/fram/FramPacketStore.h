#include <Arduino.h>
#include "../../chat/ChatterPacket.h"
#include "../StorageGlobals.h"
#include "../PacketStore.h"
#include "FramData.h"
#include "FramGlobals.h"
#include "PacketData.h"
#include "../../rtc/RTClockBase.h"

#ifndef FRAMPACKETSTORE_H
#define FRAMPACKETSTORE_H

class FramPacketStore : public PacketStore {
    public:
        FramPacketStore(FramData* _datastore, RTClockBase* _rtc) { datastore = _datastore; rtc = _rtc; }
        bool init ();

        bool wasReceived (const char* senderId, const char* messageId, int packetNum);

        // messages, list/get/save
        bool savePacket (ChatterPacket* packet);
        bool saveAirMessageTimestamp (const char* senderId, const char* messageId, const char* sortableTime);
        bool saveBridgeMessageTimestamp (const char* senderId, const char* messageId, const char* sortableTime);
        bool clearMessage (const char* sender, const char* messageId);
        bool clearAllMessages ();

        int getNumPackets (const char* senderId, const char* messageId);
        int readPacket (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer);
        int readPacket (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer, int maxLength);
        int readPacket (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer, int maxLength, PacketStatus status);

        // bridging-related functions
        bool moveMessageToAirOut (const char* sender, const char* messageId);
        bool moveMessageToBridgeOut (const char* sender, const char* messageId);

        int readPacketFromAirOut (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer, int maxLength);
        int readPacketFromBridgeOut (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer, int maxLength);
        bool clearPacketFromAirOut (const char* senderId, const char* messageId, int packetNum);
        bool clearPacketFromBridgeOut (const char* senderId, const char* messageId, int packetNum);

        bool readNextAirOutPacketDetails (ChatterPacketMetaData* packetBuffer);
        bool readNextBridgeOutPacketDetails (ChatterPacketMetaData* packetBuffer);

        int pruneAgedMessages (const char* oldestDatetime); // delete any bridge-related messages that have expired

        /// end bridging related functions

    protected:
        void populateKeyBuffer (const char* senderId, const char* messageId, int packetNum, PacketStatus status);
        FramData* datastore;
        RTClockBase* rtc;
        uint8_t keyBuffer[FRAM_PACKET_KEYSIZE];
        char tsBuffer[12];
        PacketData packetBuffer;

        bool readOldestPacketDetails (PacketStatus status, ChatterPacketMetaData* packetDetailsBuffer);

        int getNumPackets (const char* senderId, const char* messageId, PacketStatus status);
        bool saveMessageTimetamp(const char* senderId, const char* messageId, const char* sortableTime, PacketStatus status);
        bool saveMessageStatus (const char* senderId, const char* messageId, PacketStatus oldStatus, PacketStatus newStatus);
        bool savePacketStatus (const char* senderId, const char* messageId, int packetNum, PacketStatus oldStatus, PacketStatus newStatus);        
};

#endif
#include <Arduino.h>
#include "../../chat/ChatterPacket.h"
#include "../StorageGlobals.h"
#include "../PacketStore.h"
#include "FramData.h"

#ifndef FRAMPACKETSTORE_H
#define FRAMPACKETSTORE_H

class FramPacketStore : public PacketStore {
    public:
        FramPacketStore(FramData* _datastore) { datastore = _datastore; }
        bool init ();

        bool wasReceived (const char* senderId, const char* messageId, int packetNum);

        // messages, list/get/save
        bool savePacket (ChatterPacket* packet);
        bool saveAirMessageTimestamp (const char* senderId, const char* messageId, const char* sortableTime);
        bool saveBridgeMessageTimestamp (const char* senderId, const char* messageId, const char* sortableTime);
        bool clearMessage (const char* sender, const char* messageId);
        bool clearAllMessages ();

        int generateHash (const char* senderId, const char* messageId, uint8_t* hashBuffer);

        int getNumPackets (const char* senderId, const char* messageId);
        int readPacket (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer);
        int readPacket (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer, int maxLength);
        int readMessage (const char* senderId, const char* messageId, uint8_t* buffer, int maxLength);

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

        bool hashMatches (const char* senderId, const char* messageId);

    protected:
        FramData* datastore;


};

#endif
#include "PacketStore.h"

#ifndef DUMMYPACKETSTORE_H
#define DUMMYPACKETSTORE_H
class DummyPacketStore : public PacketStore {
    public:
        bool init ();

        // messages, list/get/save
        bool savePacket (ChatterPacket* packet);
        bool savePacket (ChatterPacket* packet, Encryptor* encryptor);
        bool saveMessageTimestamp (const char* senderId, const char* messageId, const char* sortableTime) {return true;}
        bool saveAirMessageTimestamp (const char* senderId, const char* messageId, const char* sortableTime) {return true;}
        bool saveBridgeMessageTimestamp (const char* senderId, const char* messageId, const char* sortableTime) {return true;}

        bool clearMessage (const char* sender, const char* messageId);
        bool clearAllMessages ();

        //int generateHash (const char* senderId, const char* messageId, uint8_t* hashBuffer);
        //int generateHash (const char* senderId, const char* messageId, uint8_t* hashBuffer, Encryptor* encryptor);

        int getNumPackets (const char* senderId, const char* messageId);
        int readPacket (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer);
        int readPacket (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer, int maxLength);
        int readPacket (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer, Encryptor* encryptor);
        int readPacket (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer, int maxLength, Encryptor* encryptor);
        int readMessage (const char* senderId, const char* messageId, uint8_t* buffer, int maxLength, Encryptor* encryptor);

        bool hashMatches (const char* senderId, const char* messageId);
        bool hashMatches (const char* senderId, const char* messageId, Encryptor* encryptor);

        bool moveMessageToAirOut (const char* sender, const char* messageId) {return true;}
        bool moveMessageToBridgeOut (const char* sender, const char* messageId) {return true;}

        int readPacketFromAirOut (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer, int maxLength, Encryptor* encryptor) {return 0;}
        int readPacketFromBridgeOut (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer, int maxLength, Encryptor* encryptor) {return 0;}
        bool clearPacketFromAirOut (const char* senderId, const char* messageId, int packetNum) {return true;}
        bool clearPacketFromBridgeOut (const char* senderId, const char* messageId, int packetNum) {return true;}

        bool readNextAirOutPacketDetails (ChatterPacketMetaData* packetBuffer) {return true;}
        bool readNextBridgeOutPacketDetails (ChatterPacketMetaData* packetBuffer) {return true;}

        int pruneAgedMessages () {return 0;}

};
#endif
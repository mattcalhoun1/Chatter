#include "../PacketStore.h"
#include <SD.h>
#include <SHA256.h>
#include "../../encryption/Encryptor.h"

#ifndef SDPACKETSTORE_H
#define SDPACKETSTORE_H

class SDPacketStore : public PacketStore {
    public:
        SDPacketStore(Encryptor* _encryptor) { encryptor = _encryptor; }
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
    private:
        bool setReceived (const char* senderId, const char* messageId, const char* packetId);
        bool saveMessageTimestamp (const char* senderId, const char* messageId, const char* sortableTime);


        bool saveMessageTimestamp (const char* senderId, const char* messageId, const char* sortableTime, const char* folder);
        int readPacket (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer, int maxLen, const char* folder);
        int getNumPackets (const char* senderId, const char* messageId, const char* folder);

        bool readOldestPacketDetails (ChatterPacketMetaData* packetBuffer, const char* folder);

        bool copyDirectory (const char* source, const char* dest);
        bool clearMessage (const char* sender, const char* messageId, const char* folder);
        bool clearFolder(const char* fullFolderName);
        bool clearFile(const char* fullFileName);


        const char* messageDir = "/MSG/";
        const char* bridgeOutDir = "/BOUT/";
        const char* airOutDir = "/AOUT/";
        const char* receivedDir = "/RCVD/";

        SHA256 hasher;
        uint8_t filePacketBuffer[STORAGE_CONTENT_BUFFER_SIZE]; // use this buffer to read footer
        uint8_t hashCalcBuffer[STORAGE_HASH_LENGTH]; // buffer for when this class calcs hash    

        char packetFileName[STORAGE_MAX_FILENAME_LENGTH]; // reusable so we don't keep recreating on the stack
        char packetDirectoryName[STORAGE_MAX_FILENAME_LENGTH];

        char senderId[STORAGE_DEVICE_ID_LENGTH + 1];
        char messageId[STORAGE_MESSAGE_ID_LENGTH + 1];
        char packetId[STORAGE_CHUNK_ID_LENGTH + 1];

        int maxBridgeMessageAge = 4; // how long (minutes) a packet is allowed to stay around for forwarding, before being cleaned
        Encryptor* encryptor;
};
#endif
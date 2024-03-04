#include "FramPacketStore.h"

bool FramPacketStore::init () {
    return true;
}

void FramPacketStore::populateKeyBuffer (const char* senderId, const char* messageId, int packetNum, PacketStatus status) {
    memcpy(keyBuffer, senderId, CHATTER_DEVICE_ID_SIZE);
    memcpy(keyBuffer + CHATTER_DEVICE_ID_SIZE, messageId, CHATTER_MESSAGE_ID_SIZE);
    sprintf(((char*)keyBuffer)+CHATTER_DEVICE_ID_SIZE+CHATTER_MESSAGE_ID_SIZE, "%03d", packetNum);
    keyBuffer[FRAM_PACKET_KEYSIZE-1] = status;
}

bool FramPacketStore::wasReceived (const char* senderId, const char* messageId, int packetNum) {
    populateKeyBuffer(senderId, messageId, packetNum, PacketReceived);
    // should we also check for other status?
    return datastore->recordExists (ZonePacket, keyBuffer);
}

// messages, list/get/save
bool FramPacketStore::savePacket (ChatterPacket* packet) {
    packetBuffer.setSenderId((const char*)packet->sender);
    packetBuffer.setRecipientId((const char*)packet->recipient);
    packetBuffer.setChunkId(packet->chunkId);
    packetBuffer.setMessageId(packet->messageId);
    packetBuffer.setStatus(PacketReceived);
    packetBuffer.setTimestamp(rtc->getSortableTime());

    int bytesToWrite = packet->isMetadata ? packet->rawContentLength : packet->contentLength;

    packetBuffer.setFullPacket(packet->content, bytesToWrite);

    if (bytesToWrite == 0) {
        logConsole("Warning: empty packet being saved.");
    }

    bool success = datastore->writeToNextSlot(&packetBuffer);    

    if (!success) {
        logConsole("Failed to save packet!");
    }

    //datastore->logCache();

    return success;

}

bool FramPacketStore::saveMessageTimetamp(const char* senderId, const char* messageId, const char* sortableTime, PacketStatus status) {
    uint8_t numPackets = getNumPackets(senderId, messageId, status);
    for (int packetNum = 0; packetNum < numPackets; packetNum++) {
        populateKeyBuffer(senderId, messageId, packetNum, status);
        uint8_t slotNum = datastore->getRecordNum(ZonePacket, keyBuffer);
        if (slotNum != FRAM_NULL) {
            if(datastore->readRecord(&packetBuffer, slotNum)) {
                packetBuffer.setTimestamp(sortableTime);
                datastore->writeRecord(&packetBuffer, slotNum);
            }
            else {
                logConsole("Fram packet not read");
            }
        }
    }
    if (numPackets == 0) {
        logConsole("Air out message not found in fram");
    }
    return numPackets > 0;    
}

bool FramPacketStore::saveAirMessageTimestamp (const char* senderId, const char* messageId, const char* sortableTime) {
    return saveMessageTimetamp(senderId, messageId, sortableTime, PacketAirOut);
}

bool FramPacketStore::saveBridgeMessageTimestamp (const char* senderId, const char* messageId, const char* sortableTime) {
    return saveMessageTimetamp(senderId, messageId, sortableTime, PacketBridgeOut);
}

bool FramPacketStore::clearAllMessages () {
    return datastore->clearZone(ZonePacket);

}

int FramPacketStore::getNumPackets (const char* senderId, const char* messageId, PacketStatus status) {
    for (uint8_t p = 0; p < 255; p++) {
        populateKeyBuffer(senderId, messageId, p, status);
        if (!datastore->recordExists(ZonePacket, keyBuffer)) {
            return p; // the current index = number of packets
        }
    }
    return 0;
}

int FramPacketStore::getNumPackets (const char* senderId, const char* messageId) {
    return getNumPackets(senderId, messageId, PacketReceived);
}


int FramPacketStore::readPacket (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer) {
    return readPacket(senderId, messageId, packetNum, buffer, CHATTER_FULL_BUFFER_LEN, PacketReceived);
}

int FramPacketStore::readPacket (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer, int maxLength) {
    return readPacket(senderId, messageId, packetNum, buffer, maxLength, PacketReceived);
}

int FramPacketStore::readPacket (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer, int maxLength, PacketStatus status) {
    populateKeyBuffer(senderId, messageId, packetNum, status);
    uint8_t slotNum = datastore->getRecordNum(ZonePacket, keyBuffer);
    if (slotNum != FRAM_NULL) {
        if (!datastore->readRecord(&packetBuffer, slotNum)) {
            logConsole("Read failed!");
        }
        if (packetBuffer.getPacketLength() > maxLength) {
            logConsole("Error: fram packet contained too many bytes");
            return 0;
        }
        memcpy(buffer, packetBuffer.getFullPacket(), packetBuffer.getPacketLength());
        return packetBuffer.getPacketLength();
    }

    logConsole("Packet not found");
    return 0;
}

bool FramPacketStore::saveMessageStatus (const char* senderId, const char* messageId, PacketStatus oldStatus, PacketStatus newStatus) {
    uint8_t numPackets = getNumPackets(senderId, messageId, oldStatus);
    uint8_t slotNum = 0;
    bool saved = false;
    for (uint8_t p = 0; p < numPackets; p++) {
        populateKeyBuffer(senderId, messageId, p, oldStatus);
        slotNum = datastore->getRecordNum(ZonePacket, keyBuffer);
        datastore->readRecord(&packetBuffer, slotNum);
        packetBuffer.setStatus(newStatus);

        // this isn't exactly perfect, if just the last save succeeds, the method returns success
        saved = datastore->writeRecord(&packetBuffer, slotNum);
    }
    return saved;
}

bool FramPacketStore::savePacketStatus (const char* senderId, const char* messageId, int packetNum, PacketStatus oldStatus, PacketStatus newStatus) {
    bool saved = false;
    populateKeyBuffer(senderId, messageId, packetNum, oldStatus);
    uint8_t slotNum = datastore->getRecordNum(ZonePacket, keyBuffer);
    if(datastore->readRecord(&packetBuffer, slotNum)) {
        packetBuffer.setStatus(newStatus);

        // this isn't exactly perfect, if just the last save succeeds, the method returns success
        saved = datastore->writeRecord(&packetBuffer, slotNum);
    }
    return saved;
}

// bridging-related functions
bool FramPacketStore::moveMessageToAirOut (const char* sender, const char* messageId) {
    return saveMessageStatus(sender, messageId, PacketReceived, PacketAirOut);
}

bool FramPacketStore::moveMessageToBridgeOut (const char* sender, const char* messageId) {
    return saveMessageStatus(sender, messageId, PacketReceived, PacketBridgeOut);
}

bool FramPacketStore::moveMessageToQuarantine (const char* sender, const char* messageId) {
    return saveMessageStatus(sender, messageId, PacketReceived, PacketQuarantined);
}

int FramPacketStore::readPacketFromAirOut (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer, int maxLength) {
    return readPacket(senderId, messageId, packetNum, buffer, maxLength, PacketAirOut);
}

int FramPacketStore::readPacketFromBridgeOut (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer, int maxLength) {
    return readPacket(senderId, messageId, packetNum, buffer, maxLength, PacketBridgeOut);
}

bool FramPacketStore::clearPacketFromAirOut (const char* senderId, const char* messageId, int packetNum) {
    return savePacketStatus (senderId, messageId, packetNum, PacketAirOut, PacketDeleted);
}

bool FramPacketStore::clearPacketFromBridgeOut (const char* senderId, const char* messageId, int packetNum) {
    return savePacketStatus (senderId, messageId, packetNum, PacketBridgeOut, PacketDeleted);
}

bool FramPacketStore::readOldestPacketDetails (PacketStatus status, ChatterPacketMetaData* packetDetailsBuffer) {
    memset(tsBuffer, 255, 12); // initialize to greatest possible timestamp
    bool found = false;

    for (uint8_t slotId = 0; slotId < datastore->getNumUsedSlots(ZonePacket); slotId++) {
        // look at the key and see if the status shows its sitting in Air out
        datastore->readKey(keyBuffer, ZonePacket, slotId);
        if (keyBuffer[FRAM_PACKET_KEYSIZE-1] == status) {
            // read this packet to get the timestamp
            datastore->readRecord(&packetBuffer, slotId);
            if (memcmp(packetBuffer.getTimestamp(), tsBuffer, 12) < 0) {
                // this packet is older, so it should be next
                memcpy(packetDetailsBuffer->chunkId, packetBuffer.getChunkId(), CHATTER_CHUNK_ID_SIZE);
                packetDetailsBuffer->chunkId[CHATTER_CHUNK_ID_SIZE] = 0;

                memcpy(packetDetailsBuffer->messageId, packetBuffer.getMessageId(), CHATTER_MESSAGE_ID_SIZE);
                packetDetailsBuffer->messageId[CHATTER_MESSAGE_ID_SIZE] = 0;

                memcpy(packetDetailsBuffer->recipient, packetBuffer.getRecipientId(), CHATTER_DEVICE_ID_SIZE);
                packetDetailsBuffer->recipient[CHATTER_DEVICE_ID_SIZE] = 0;

                memcpy(packetDetailsBuffer->sender, packetBuffer.getSenderId(), CHATTER_DEVICE_ID_SIZE);
                packetDetailsBuffer->sender[CHATTER_DEVICE_ID_SIZE] = 0;

                found = true;
            }
        }
    }

    return found;
}

bool FramPacketStore::readNextAirOutPacketDetails (ChatterPacketMetaData* packetBuffer) {
    return readOldestPacketDetails(PacketAirOut, packetBuffer);
}

bool FramPacketStore::readNextBridgeOutPacketDetails (ChatterPacketMetaData* packetBuffer) {
    return readOldestPacketDetails(PacketBridgeOut, packetBuffer);
}

int FramPacketStore::pruneAgedMessages (const char* oldestDatetime) {
    int numPruned = 0;

    for (uint8_t slotId = 0; slotId < datastore->getNumUsedSlots(ZonePacket); slotId++) {
        // look at the key and see if the status shows its sitting in Air out
        datastore->readKey(keyBuffer, ZonePacket, slotId);
        // skip if it's already deleted
        if (keyBuffer[FRAM_PACKET_KEYSIZE-1] != PacketDeleted) {
            // read this packet to get the timestamp
            datastore->readRecord(&packetBuffer, slotId);
            if (memcmp(packetBuffer.getTimestamp(), oldestDatetime, 12) < 0) {
                // this packet is older, so it should be pruned
                packetBuffer.setStatus(PacketDeleted);
                datastore->writeRecord(&packetBuffer, slotId);
                numPruned++;
            }
        }
    }
 
    return numPruned;    
}

/// end bridging related functions

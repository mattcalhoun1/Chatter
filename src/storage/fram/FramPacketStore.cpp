#include "FramPacketStore.h"

bool FramPacketStore::init () {
    return true;
}

bool FramPacketStore::wasReceived (const char* senderId, const char* messageId, int packetNum) {
    return true;    
}

// messages, list/get/save
bool FramPacketStore::savePacket (ChatterPacket* packet) {
    return true;    

}

bool FramPacketStore::saveAirMessageTimestamp (const char* senderId, const char* messageId, const char* sortableTime) {
    return true;    

}

bool FramPacketStore::saveBridgeMessageTimestamp (const char* senderId, const char* messageId, const char* sortableTime) {
    return true;    

}

bool FramPacketStore::clearMessage (const char* sender, const char* messageId) {
    return true;    

}

bool FramPacketStore::clearAllMessages () {
    return true;    

}

int FramPacketStore::generateHash (const char* senderId, const char* messageId, uint8_t* hashBuffer) {
    return true;    

}

int FramPacketStore::getNumPackets (const char* senderId, const char* messageId) {
    return true;    

}

int FramPacketStore::readPacket (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer) {
    return true;    

}

int FramPacketStore::readPacket (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer, int maxLength) {
    return true;    

}

int FramPacketStore::readMessage (const char* senderId, const char* messageId, uint8_t* buffer, int maxLength) {
    return true;    

}

// bridging-related functions
bool FramPacketStore::moveMessageToAirOut (const char* sender, const char* messageId) {
    return true;    

}

bool FramPacketStore::moveMessageToBridgeOut (const char* sender, const char* messageId) {
    return true;    

}

int FramPacketStore::readPacketFromAirOut (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer, int maxLength) {
    return true;    

}

int FramPacketStore::readPacketFromBridgeOut (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer, int maxLength) {
    return true;    

}

bool FramPacketStore::clearPacketFromAirOut (const char* senderId, const char* messageId, int packetNum) {
    return true;    

}

bool FramPacketStore::clearPacketFromBridgeOut (const char* senderId, const char* messageId, int packetNum) {
    return true;    

}

bool FramPacketStore::readNextAirOutPacketDetails (ChatterPacketMetaData* packetBuffer) {
    return true;    

}

bool FramPacketStore::readNextBridgeOutPacketDetails (ChatterPacketMetaData* packetBuffer) {
    return true;    

}

int FramPacketStore::pruneAgedMessages (const char* oldestDatetime) {
    return true;    

}

/// end bridging related functions

bool FramPacketStore::hashMatches (const char* senderId, const char* messageId) {
    return true;    

}
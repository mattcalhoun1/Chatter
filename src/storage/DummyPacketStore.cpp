#include "DummyPacketStore.h"

bool DummyPacketStore::init() {
    logConsole("WARNING: Dummy packet store running!");
    return true;
}

bool DummyPacketStore::savePacket (ChatterPacket* packet) {
    return true;
}

bool DummyPacketStore::savePacket (ChatterPacket* packet, Encryptor* encryptor) {
    return true;
}

bool DummyPacketStore::clearMessage (const char* sender, const char* messageId) {
    return true;
}

bool DummyPacketStore::clearAllMessages () {
    return true;
}

int DummyPacketStore::getNumPackets (const char* senderId, const char* messageId) {
    return 2;
}

int DummyPacketStore::readPacket (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer) {
    sprintf((char*)buffer, "Dummy", 5);
    return 1;
}

int DummyPacketStore::readPacket (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer, int maxLength) {
    sprintf((char*)buffer, "Dummy", 5);
    return 1;
}

int DummyPacketStore::readPacket (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer, Encryptor* encryptor) {
    sprintf((char*)buffer, "Dummy", 5);
    return 1;
}

int DummyPacketStore::readPacket (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer, int maxLength, Encryptor* encryptor) {
    sprintf((char*)buffer, "Dummy", 5);
    return 1;
}

int DummyPacketStore::readMessage (const char* senderId, const char* messageId, uint8_t* buffer, int maxLength, Encryptor* encryptor) {
    sprintf((char*)buffer, "Dummy", 5);
    return 1;
}

bool DummyPacketStore::hashMatches (const char* senderId, const char* messageId) {
    return 1;
}

bool DummyPacketStore::hashMatches (const char* senderId, const char* messageId, Encryptor* encryptor) {
    return true;
}

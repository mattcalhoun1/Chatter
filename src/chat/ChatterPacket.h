#include <Arduino.h>
#include "ChatGlobals.h"
#include "ChatterMessageFlags.h"

#ifndef CHATTERPACKET_H
#define CHATTERPACKET_H

enum ChatterPacketEncryptionFlag {
    PacketClear = 0,
    PacketEncrypted = 1,
    PacketSigned = 2
};

struct ChatterPacketMetaData {
    uint8_t sender[CHATTER_DEVICE_ID_SIZE + 1];
    uint8_t recipient[CHATTER_DEVICE_ID_SIZE + 1];
    ChatterPacketEncryptionFlag encryptionFlag;
    uint8_t messageId[CHATTER_MESSAGE_ID_SIZE + 1];
    uint8_t chunkId[CHATTER_CHUNK_ID_SIZE + 1];
    int packetId; // used when moving between bridge buffers
};

struct ChatterPacketSmall : public ChatterPacketMetaData {
    uint8_t content[CHATTER_FULL_BUFFER_LEN];
    int contentLength; 
};

struct ChatterPacket : public ChatterPacketMetaData {
    //uint8_t content[CHATTER_PACKET_SIZE - (CHATTER_DEVICE_ID_SIZE*2 + CHATTER_MESSAGE_ID_SIZE + CHATTER_CHUNK_ID_SIZE + 1) + 1]; // +1 for enc flag and +1 for term

    // the content buffer may be holding a piece of the message, or the entire message
    uint8_t content[CHATTER_FULL_MESSAGE_BUFFER_SIZE];
    
    int contentLength; // how much of the content space is user data

    // to calculate non-header point of the raw message
    int headerLength = CHATTER_PACKET_HEADER_LENGTH;

    bool isMetadata; // whether this particular packet is metadata or part of the user's message. metadata is included in the sig/hash

    const char* rawMessage; // pointer to the message that probably exists in the channel or somewhere outside this struct
    int rawContentLength; // how much of the raw content length is user data

    ChatterMessageFlags flags;
};

#endif
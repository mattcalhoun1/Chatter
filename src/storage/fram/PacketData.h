// for indexing
// unencrypted
// sender size of device id
// packet number 3 digits
// message id 3 digits
// status 1 digit
//
// ecnrypted
// timestamp 12 char
// message 150

#include <Arduino.h>
#include "FramData.h"
#include "Globals.h"

#ifndef PACKETDATA_H
#define PACKETDATA_H

enum PacketStatus {
  PacketReceived = (uint8_t)'R',
  PacketBridgeOut = (uint8_t)'B',
  PacketAirOut = (uint8_t)'A',
  PacketDeleted = (uint8_t)'D',
};

class PacketData : public FramRecord {
  public:
    FramZone getZone () { return ZonePacket; }

    void setSenderId (const char* _senderId);
    const char* getSenderId () {return (const char*)senderId;}

    void setMessageId (const uint8_t* _messageId);
    const uint8_t* getMessageId () {return messageId;}

    void setChunkId (const uint8_t* _chunkId);
    const uint8_t* getChunkId () {return chunkId;}

    void setTimestamp (const char* _packetTimestamp);
    const char* getTimestamp () {return (const char*)packetTimestamp;}

    void setStatus (uint8_t _status) {status = _status;}
    uint8_t getStatus() {return status;}

    void setFullPacket(const uint8_t* _fullPacket, uint8_t _packetLength);
    const uint8_t* getFullPacket () {return fullPacket;}
    uint8_t getPacketLength () { return packetLength; }

    void deserialize (const uint8_t* recordKey, const uint8_t* dataBuffer);
    void serialize (uint8_t* dataBuffer);
    void serializeKey (uint8_t* keyBuffer);

  protected:
    // unencrypted
    char senderId[CHATTER_DEVICE_ID_SIZE];
    uint8_t messageId[CHATTER_MESSAGE_ID_SIZE];
    uint8_t chunkId[CHATTER_CHUNK_ID_SIZE];
    uint8_t status = PacketReceived; // which state (awaiting air out, received, etc) is the packet in

    // encrypted
    uint8_t packetTimestamp[FRAM_TS_SIZE];
    uint8_t packetLength;
    uint8_t fullPacket[CHATTER_FULL_BUFFER_LEN];
};

#endif
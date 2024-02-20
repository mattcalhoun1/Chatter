#include "PacketData.h"

void PacketData::setSenderId (const char* _senderId) {
  memcpy(senderId, _senderId, CHATTER_DEVICE_ID_SIZE);
}

void PacketData::setRecipientId (const char* _recipientId) {
  memcpy(recipientId, _recipientId, CHATTER_DEVICE_ID_SIZE);
}

void PacketData::setTimestamp (const char* _packetTimestamp) {
  memset(packetTimestamp, 0, FRAM_TS_SIZE);
  memcpy(packetTimestamp, (uint8_t*)_packetTimestamp, FRAM_TS_SIZE);
}

void PacketData::setMessageId(const uint8_t* _messageId) {
  memcpy(messageId, _messageId, CHATTER_MESSAGE_ID_SIZE);
}

void PacketData::setChunkId(const uint8_t* _chunkId) {
  memcpy(chunkId, _chunkId, CHATTER_CHUNK_ID_SIZE);
}

void PacketData::setFullPacket(const uint8_t* _fullPacket, uint8_t _packetLength) {
  memset(fullPacket, 0, CHATTER_FULL_BUFFER_LEN);
  memcpy(fullPacket, _fullPacket, _packetLength);
  packetLength = _packetLength;
}


void PacketData::deserialize (const uint8_t* recordKey, const uint8_t* dataBuffer) {

  const uint8_t* bufferPos = recordKey;
  memcpy(senderId, bufferPos, CHATTER_DEVICE_ID_SIZE);
  bufferPos += CHATTER_DEVICE_ID_SIZE;  
  memcpy(messageId, bufferPos, CHATTER_MESSAGE_ID_SIZE);
  bufferPos += CHATTER_MESSAGE_ID_SIZE;  
  memcpy(chunkId, bufferPos, CHATTER_CHUNK_ID_SIZE);
  bufferPos += CHATTER_CHUNK_ID_SIZE;
  status = bufferPos[0];

  // data buffer is in the following order
  bufferPos = dataBuffer;
  memcpy(recipientId, bufferPos, CHATTER_DEVICE_ID_SIZE);
  bufferPos += CHATTER_DEVICE_ID_SIZE;

  memcpy(packetTimestamp, bufferPos, FRAM_TS_SIZE);
  bufferPos += FRAM_TS_SIZE;

  packetLength = bufferPos[0];
  bufferPos += 1;

    memset(fullPacket, 0, CHATTER_FULL_BUFFER_LEN);
  memcpy(fullPacket, bufferPos, packetLength);
}

int PacketData::serialize (uint8_t* dataBuffer) {
  uint8_t* bufferPos = dataBuffer;
  memcpy(bufferPos, recipientId, CHATTER_DEVICE_ID_SIZE);
  bufferPos += CHATTER_DEVICE_ID_SIZE;


  memcpy(bufferPos, packetTimestamp, FRAM_TS_SIZE);
  bufferPos += FRAM_TS_SIZE;

  bufferPos[0] = packetLength;
  bufferPos += 1;

  memcpy(bufferPos, fullPacket, packetLength);

  return CHATTER_DEVICE_ID_SIZE + FRAM_TS_SIZE + 1 + packetLength;
}

void PacketData::serializeKey (uint8_t* keyBuffer) {
  uint8_t* bufferPos = keyBuffer;
  memcpy(bufferPos, senderId, CHATTER_DEVICE_ID_SIZE);
  bufferPos += CHATTER_DEVICE_ID_SIZE;
  memcpy(bufferPos, messageId, CHATTER_MESSAGE_ID_SIZE);
  bufferPos += CHATTER_MESSAGE_ID_SIZE;
  memcpy(bufferPos, chunkId, CHATTER_CHUNK_ID_SIZE);
  bufferPos += CHATTER_CHUNK_ID_SIZE;
  bufferPos[0] = status;
}

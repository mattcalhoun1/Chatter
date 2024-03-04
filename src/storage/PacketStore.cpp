#include "PacketStore.h"

// reads entire message into buffer. does not check signature /etc
int PacketStore::readMessage (const char* senderId, const char* messageId, uint8_t* buffer, int maxLength) {
  int fullLength = 0;
  int bufferPosition = 0;
  memset(buffer, 0, maxLength);

  uint8_t lastPacket = getNumPackets(senderId, messageId) - 1;
  if (lastPacket > 0) {
    for (int packetNum = lastPacket; packetNum > 0; packetNum--) {
      // Clear the message buffer
      int chunkSize = readPacket(senderId, messageId, packetNum, filePacketBuffer);
      if (chunkSize > 0) {
        if (bufferPosition + chunkSize + 1 > maxLength) {
          logConsole("Full message too large for buffer. Truncating!");
          fullLength = bufferPosition; // truncate
          packetNum = 0; // skip to the end
        }
        else {
          memcpy(buffer + bufferPosition, filePacketBuffer, chunkSize);
          bufferPosition += chunkSize;

          // update the full length to the new buffer position+1
          fullLength = bufferPosition;//bufferPosition + 1;
        }
      }
      else {
        logConsole("Failed to read a message packet!");
        return 0;
      }
    }

    return fullLength;
  }

  logConsole("Error: message is missing!");
  return 0;
}

bool PacketStore::hashMatches (const char* senderId, const char* messageId) {
  // Calculate a hash of the given message, place hash into a temp buffer
  memset(hashCalcBuffer, 0, STORAGE_HASH_LENGTH); // make sure getting good comparison
  generateHash(senderId, messageId, hashCalcBuffer);
  memset(filePacketBuffer, 0, STORAGE_CONTENT_BUFFER_SIZE);

  // sig packet is not encrypted
  int sigPacketSize = readPacket(senderId, messageId, STORAGE_SIGNATURE_PACKET_ID, filePacketBuffer, STORAGE_SIG_PACKET_EXPECTED_LENGTH + STORAGE_PACKET_HEADER_LENGTH);

  if (sigPacketSize == STORAGE_SIG_PACKET_EXPECTED_LENGTH + STORAGE_PACKET_HEADER_LENGTH) {
    // the provided hash (or first 32 of it) is expected to contain a matching hash
    uint8_t* packetHashLocation = filePacketBuffer + STORAGE_PACKET_HEADER_LENGTH;
    bool match = true;
    for (int i = 0; i < STORAGE_HASH_LENGTH; i++) {
      if (hashCalcBuffer[i] != packetHashLocation[i]) {
        match = false;
      }
    }
    return match;
  }
  else {
    logConsole("Error: signature packet on disk is not expected size of " + String(STORAGE_SIG_PACKET_EXPECTED_LENGTH + STORAGE_PACKET_HEADER_LENGTH) + ", but instead is " + String(sigPacketSize));
  }

  return false;
}

int PacketStore::generateHash (const char* senderId, const char* messageId, uint8_t* hashBuffer) {
  hasher.clear();
  // the packets are in reverse order, highest packet nubmer being beginning fo the message
  // 1 is the very first part of the message, 0 is the footer/sig
  uint8_t lastPacket = getNumPackets(senderId, messageId) - 1;
  if (lastPacket > 0) {
    for (int packetNum = lastPacket; packetNum > 0; packetNum--) {
      // Read the bytes of the packet into the hasher
      memset(filePacketBuffer, 0, STORAGE_CONTENT_BUFFER_SIZE);
      int chunkSize = readPacket(senderId, messageId, packetNum, filePacketBuffer);

      if (chunkSize > 0) {
        hasher.update(filePacketBuffer, chunkSize);
      }
      else {
        logConsole("Failed to read a message packet!");
        return 0;
      }
    }

    // Now calculate the hash and put it into the buffer
    hasher.finalize(hashBuffer, hasher.hashSize());
    return hasher.hashSize();
  }

  logConsole("Message not found, cant generate hash");
  return 0;
}
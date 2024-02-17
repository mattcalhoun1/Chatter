#include "SDPacketStore.h"

bool SDPacketStore::init() {
    if (!running) {
      if (!SD.exists(messageDir)){
        SD.mkdir(messageDir);
      }
      if (!SD.exists(airOutDir)){
        SD.mkdir(airOutDir);
      }
      if (!SD.exists(bridgeOutDir)){
        SD.mkdir(bridgeOutDir);
      }
      if (!SD.exists(receivedDir)){
        SD.mkdir(receivedDir);
      }

      running = true;
    }
    return running;
}

bool SDPacketStore::setReceived (const char* senderId, const char* messageId, int packetNum) {
  // make sure sender exists
  sprintf(packetDirectoryName, "%s%s", receivedDir, senderId);
  if (!SD.exists(packetDirectoryName)) {
    SD.mkdir(packetDirectoryName);
  }
  

  sprintf(packetDirectoryName, "%s%s/%s%03d", receivedDir, senderId, messageId, packetNum);
  if (!SD.exists(packetDirectoryName)) {
    if(SD.mkdir(packetDirectoryName)) {
      return true;
    }
    else {
      logConsole("Could not create " + String(packetDirectoryName));
      return false;
    }
  }
  return true;
}

bool SDPacketStore::wasReceived (const char* senderId, const char* messageId, int packetNum) {
  sprintf(packetDirectoryName, "%s%s/%s%03d", receivedDir, senderId, messageId, packetNum);
  return SD.exists(packetDirectoryName);
}

int SDPacketStore::getNumPackets (const char* senderId, const char* messageId) {
  return getNumPackets(senderId, messageId, messageDir);
}

int SDPacketStore::getNumPackets (const char* senderId, const char* messageId, const char* folder) {
  uint8_t lastPacket = 1;
  bool messageExists = false;
  sprintf(packetDirectoryName, "%s%s/%s/", folder, senderId, messageId);
  if (SD.exists(packetDirectoryName)) {
    sprintf(packetFileName, "%s%s/%s/%03d", folder, senderId, messageId, lastPacket);
    while (SD.exists(packetFileName)) {
      messageExists = true;

      // bump the packet # and recreate file name
      sprintf(packetFileName, "%s%s/%s/%03d", folder, senderId, messageId, ++lastPacket);
    }
    lastPacket -= 1; // we would have gone just past the highest packet
  }
  else {
    logConsole("Message does not exist: " + String(packetDirectoryName));
  }

  return messageExists ? lastPacket : 0;
}


// all of these read packet methods assume the packet exists on sd card
int SDPacketStore::readPacket (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer) {
  // don't read more than max expected content
  return readPacket(senderId, messageId, packetNum, buffer, STORAGE_CONTENT_BUFFER_SIZE);
}

int SDPacketStore::readPacket (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer, Encryptor* encryptor) {
  return readPacket(senderId, messageId, packetNum, buffer, STORAGE_CONTENT_BUFFER_SIZE, encryptor);
}

int SDPacketStore::readPacket (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer, int maxLen) {
  return readPacket(senderId, messageId, packetNum, buffer, maxLen, nullptr);
}

int SDPacketStore::readPacket (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer, int maxLen, Encryptor* encryptor) {
  return readPacket(senderId, messageId, packetNum, buffer, maxLen, encryptor, messageDir);
}

// assumes the packet exists
int SDPacketStore::readPacket (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer, int maxLen, Encryptor* encryptor, const char* folder) {
    sprintf(packetFileName, "%s%s/%s/%03d", folder, senderId, messageId, packetNum);

    int chunkSize = 0;

    // Read the bytes of the packet into the hasher
    if (SD.exists(packetFileName)) {
      File packetFile = SD.open(packetFileName, FILE_READ);

      // no file should ever contain more than our buffer size or if we know how much to expect, not more than that
      if ((encryptor == nullptr && packetFile.available() <= maxLen) || (encryptor != nullptr && packetFile.available() <= STORAGE_CONTENT_BUFFER_SIZE)) {
        chunkSize = packetFile.available();
        packetFile.read(buffer, chunkSize);

        // Decrypt the buffer
        if (encryptor != nullptr) {
          encryptor->decryptVolatile(buffer, chunkSize);

          // recreate the buffer with decrypted version
          chunkSize = encryptor->getUnencryptedBufferLength();
          memcpy(buffer, encryptor->getUnencryptedBuffer(), chunkSize);
          buffer[chunkSize] = '\0';//term

          //logConsole("Read packet from disk ,size: " + String(chunkSize));
        }
      }
      else {
        logConsole("ERROR! File is larger than buffer! File size: " + String(packetFile.available()));
      }
      packetFile.close();

    }
    else {
      logConsole("Packet requested from storage, but not found: " + String(senderId) + "/" + String(messageId) + " ... packet num: " + String(packetNum));
    }

  return chunkSize;
}

// reads entire message into buffer. does not check signature /etc
int SDPacketStore::readMessage (const char* senderId, const char* messageId, uint8_t* buffer, int maxLength, Encryptor* encryptor) {
  int fullLength = 0;
  int bufferPosition = 0;
  memset(buffer, 0, maxLength);

  uint8_t lastPacket = getNumPackets(senderId, messageId);
  if (lastPacket > 0) {
    for (int packetNum = lastPacket; packetNum > 0; packetNum--) {
      // Clear the message buffer
      int chunkSize = readPacket(senderId, messageId, packetNum, filePacketBuffer, encryptor);
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

bool SDPacketStore::hashMatches (const char* senderId, const char* messageId) {
  return hashMatches(senderId, messageId, nullptr);
}

bool SDPacketStore::hashMatches (const char* senderId, const char* messageId, Encryptor* encryptor) {
  // Calculate a hash of the given message, place hash into a temp buffer
  memset(hashCalcBuffer, 0, STORAGE_HASH_LENGTH); // make sure getting good comparison
  generateHash(senderId, messageId, hashCalcBuffer, encryptor);
  memset(filePacketBuffer, 0, STORAGE_CONTENT_BUFFER_SIZE);

  // sig packet is not encrypted
  int sigPacketSize = readPacket(senderId, messageId, STORAGE_SIGNATURE_PACKET_ID, filePacketBuffer, STORAGE_SIG_PACKET_EXPECTED_LENGTH + STORAGE_PACKET_HEADER_LENGTH , encryptor);

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

int SDPacketStore::generateHash (const char* senderId, const char* messageId, uint8_t* hashBuffer) {
  return generateHash(senderId, messageId, hashBuffer, nullptr);
}

int SDPacketStore::generateHash (const char* senderId, const char* messageId, uint8_t* hashBuffer, Encryptor* encryptor) {
  hasher.clear();
  // the packets are in reverse order, highest packet nubmer being beginning fo the message
  // 1 is the very first part of the message, 0 is the footer/sig
  uint8_t lastPacket = getNumPackets(senderId, messageId);
  if (lastPacket > 0) {
    for (int packetNum = lastPacket; packetNum > 0; packetNum--) {
      // Read the bytes of the packet into the hasher
      memset(filePacketBuffer, 0, STORAGE_CONTENT_BUFFER_SIZE);
      int chunkSize = readPacket(senderId, messageId, packetNum, filePacketBuffer, encryptor);
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

bool SDPacketStore::saveMessageTimestamp (const char* senderId, const char* messageId, const char* sortableTime) {
  return saveMessageTimestamp(senderId, messageId, sortableTime, messageDir);
}

bool SDPacketStore::savePacket(ChatterPacket* packet) {
  return savePacket(packet, nullptr);
}

bool SDPacketStore::savePacket(ChatterPacket* packet, Encryptor* encryptor) {
    if (packet->contentLength <= STORAGE_CONTENT_MAX_UNENCRYPTED_SIZE) {
      // to calculate non-header point of the raw message
      memcpy(senderId, packet->sender, STORAGE_DEVICE_ID_LENGTH + 1);
      memcpy(messageId, packet->messageId, STORAGE_MESSAGE_ID_LENGTH + 1);
      memcpy(packetId, packet->chunkId, STORAGE_CHUNK_ID_LENGTH + 1);


        sprintf(packetDirectoryName, "%s%s", messageDir, senderId);
        if (!SD.exists(packetDirectoryName)) {
            SD.mkdir(packetDirectoryName);
        }

      sprintf(packetFileName, "%s%s/%s/%s", messageDir, senderId, messageId, packetId);

      sprintf(packetDirectoryName, "%s%s/%s/", messageDir, senderId, messageId);

      if (isSafeFilename(packetFileName)) {
        // create the parent if it doesn't exist
        if (!SD.exists(packetDirectoryName)) {
          SD.mkdir(packetDirectoryName);
        }

        // delete the packet if it already exists
        if (SD.exists(packetFileName)) {
          SD.remove(packetFileName);
        }
        File packetFile = SD.open(packetFileName, FILE_WRITE);
        int bytesToWrite = packet->isMetadata ? packet->rawContentLength : packet->contentLength;
        if (encryptor == nullptr) {
          packetFile.write(packet->content, bytesToWrite);
        }
        else {
          encryptor->encryptVolatile((const char*)packet->content, bytesToWrite);
          packetFile.write(encryptor->getEncryptedBuffer(), encryptor->getEncryptedBufferLength());
        }
        packetFile.flush();
        packetFile.close();
        return true;
      }
      else {
        logConsole("File name not safe");
      }
    }
    else {
      logConsole("Packet size too big for SD storage buffer!");
    }
    return false;
}

bool SDPacketStore::clearMessage (const char* sender, const char* messageId) {
  const char* allMessagedirs[] = {messageDir, airOutDir, bridgeOutDir};
  bool success = false;
  for (int dirCount = 0; dirCount < 3; dirCount++) {
    success = clearMessage(sender, messageId, allMessagedirs[dirCount]);
  }

  return success;
}

// clears a folder, just one level deep (like a message folder)
// does not delete the folder
bool SDPacketStore::clearFolder(const char* fullFolderName) {
  bool allClear = true;

  if (isSafeFilename(fullFolderName) && SD.exists(fullFolderName)) {
    //logConsole("Clearing folder: " + String(fullFolderName));
    File message = SD.open(fullFolderName);
    while (true) {
      File packet = message.openNextFile();
      if (! packet) {
        break;
      }
      packet.close();

      const char* packetName = packet.name();
      sprintf(packetFileName, "%s/%s", fullFolderName, packetName);
      if(!SD.remove(packetFileName)) {
        logConsole("failed to remove packet");
        allClear = false;
      }
    }
    message.close();
  }

  return allClear;
}

// Deletes the individual file
bool SDPacketStore::clearFile(const char* fullFileName) {
  if (isSafeFilename(fullFileName) && SD.exists(fullFileName)) {
    //logConsole("Clearing file: " + String(fullFileName));
    return SD.remove(fullFileName);
  }

  return false;
}

bool SDPacketStore::clearMessage (const char* sender, const char* messageId, const char* folder) {
  sprintf(packetDirectoryName, "%s%s/%s", folder, sender, messageId);
  if (clearFolder(packetDirectoryName)) {
    return SD.rmdir(packetDirectoryName);
  }

  return false;
}

bool SDPacketStore::clearAllMessages () {
  logConsole("Clearing all messages...");
  bool allClear = true;

  const char* allMessagedirs[] = {messageDir, airOutDir, bridgeOutDir, receivedDir};
  for (int dirCount = 0; dirCount < 4; dirCount++) {
    const char* currMsgDir = allMessagedirs[dirCount];
    File dir = SD.open(currMsgDir);
    while (true) {
      File sender =  dir.openNextFile();
      if (! sender) {
        // no more files
        break;
      }

      const char* senderName = sender.name();
      while (true) {
        File message = sender.openNextFile();
        if (! message) {
          break;
        }

        const char* messageName = message.name();
        while (true) {
          File packet = message.openNextFile();
          if (! packet) {
            break;
          }
          packet.close();

          const char* packetName = packet.name();
          sprintf(packetFileName, "%s%s/%s/%s", currMsgDir, senderName, messageName, packetName);
          if(!SD.remove(packetFileName)) {
            logConsole("failed to remove packet");
            allClear = false;
          }
        }

        // now the message dir should be empty
        sprintf(packetFileName, "%s%s/%s", currMsgDir, senderName, messageName);
        if (!SD.rmdir(packetFileName)) {
          logConsole("Failed to remove message dir: " + String(packetFileName));
          allClear = false;
        }
      }
      sender.close();

      // now the sender dir should be empty
      sprintf(packetFileName, "%s%s", currMsgDir, senderName);
      if (!SD.rmdir(packetFileName)) {
        logConsole("Failed to remove sender dir: " + String(packetFileName));
        allClear = false;
      }
    }
    dir.close();
  }

  return allClear;
}

bool SDPacketStore::saveMessageTimestamp (const char* senderId, const char* messageId, const char* sortableTime, const char* folder) {
  // add a ts file to the given message so we know the age

  sprintf(packetFileName, "%s%s/%s/ts", folder, senderId, messageId);
  sprintf(packetDirectoryName, "%s%s/%s/", folder, senderId, messageId);

  //logConsole("Want to write ts to: " + String(packetFileName) + " in dir: " + String(packetDirectoryName));

  if (isSafeFilename(packetFileName)) {
    // create the parent if it doesn't exist
    if (!SD.exists(packetDirectoryName)) {
      SD.mkdir(packetDirectoryName);
    }

    // delete the packet if it already exists
    if (SD.exists(packetFileName)) {
      SD.remove(packetFileName);
    }

    File tsFile = SD.open(packetFileName, FILE_WRITE);
    tsFile.write(sortableTime, 12);
    tsFile.flush();
    tsFile.close();
    return true;
  }
  else {
    logConsole("File name not safe");
  }

  return false;
}

int SDPacketStore::pruneAgedMessages (const char* oldestDatetime) {
  int prunedMessageCount = 0;
  memset(packetFileName, 0, STORAGE_MAX_FILENAME_LENGTH);
  char currentTs[15]; // should be 12 digits yymmddhhmmss, plus the 't' , plus terminator
  const char* allMessagedirs[] = {messageDir, airOutDir, bridgeOutDir};
  for (int dirCount = 0; dirCount < 4; dirCount++) {
    const char* folder = allMessagedirs[dirCount];

    File dir = SD.open(folder);
    while (true) {

      File sender =  dir.openNextFile();
      if (! sender) {
        // no more files
        break;
      }

      const char* senderName = sender.name();
      while (true) {
        File message = sender.openNextFile();
        if (! message) {
          break;
        }
        bool deleteThisMessage = false;

        const char* messageName = message.name();
        message.close();

        sprintf(packetFileName, "%s%s/%s/%s", folder, senderName, messageName, "ts");
        if (SD.exists(packetFileName)) {
          // read the timestamp out of it
          File tsFile = SD.open(packetFileName, FILE_READ);
          tsFile.read((uint8_t*)currentTs, 12);
          currentTs[12] = '\0';
          tsFile.close();

          if (strcmp(currentTs, oldestDatetime) < 0) {
            deleteThisMessage = true;
          }
        }
        message.close();

        if (deleteThisMessage) {
          sprintf(packetDirectoryName, "%s%s/%s", folder, senderName, messageName);
          logConsole("Clearing aged messge: " + String(packetDirectoryName));
          clearFolder(packetDirectoryName);
          SD.rmdir(packetDirectoryName);
          prunedMessageCount++;
        }
      }
      sender.close();
    }
    dir.close();
  }
  return prunedMessageCount;
}

bool SDPacketStore::readOldestPacketDetails (ChatterPacketMetaData* packetBuffer, const char* folder) {
  bool anyPacketFound = false;
  memset(packetFileName, 0, STORAGE_MAX_FILENAME_LENGTH);

  char oldestTs[15]; // should be 12 digits yymmddhhmmss, plus the 't' , plus terminator
  char currentTs[15]; // should be 12 digits yymmddhhmmss, plus the 't' , plus terminator

  File dir = SD.open(folder);
  while (true) {

    File sender =  dir.openNextFile();
    if (! sender) {
      // no more files
      break;
    }

    const char* senderName = sender.name();
    while (true) {
      File message = sender.openNextFile();
      if (! message) {
        break;
      }

      const char* messageName = message.name();
      message.close();

      sprintf(packetFileName, "%s%s/%s/%s", folder, senderName, messageName, "ts");
      if (SD.exists(packetFileName)) {
        // read the timestamp out of it
        File tsFile = SD.open(packetFileName, FILE_READ);
        tsFile.read((uint8_t*)currentTs, 12);
        currentTs[12] = '\0';
        tsFile.close();

        if (anyPacketFound == false || strcmp(currentTs, oldestTs) < 0) {
          // make sure at least one packet still exists and its not just the timestamp
          sprintf(packetFileName, "%s%s/%s/%s", folder, senderName, messageName, "000");
          if (SD.exists(packetFileName)) {
            memcpy(oldestTs, currentTs, 12);
            memcpy(packetBuffer->sender, senderName, strlen(senderName));
            packetBuffer->sender[strlen(senderName)] = '\0';
            memcpy(packetBuffer->messageId, messageName, strlen(messageName));
            packetBuffer->messageId[strlen(messageName)] = '\0';
            anyPacketFound = true;
          }
        }
      }
      message.close();
    }
    sender.close();
  }
  dir.close();

  // if a packet was found, the max packet id is the one we want
  if (anyPacketFound) {
    packetBuffer->packetId = getNumPackets((const char*)packetBuffer->sender, (const char*)packetBuffer->messageId, folder);
  }

  return anyPacketFound;
}

bool SDPacketStore::copyDirectory (const char* source, const char* dest) {
  int filesCopied = 0;

  //sprintf(packetDirectoryName, "%s%s/%s", folder, sender, messageId);
  if (isSafeFilename(source) && SD.exists(source) && isSafeFilename(dest)) {
    logConsole("Copying directory: " + String(packetDirectoryName));

    if (SD.exists(dest)) {
      if (!clearFolder(dest)) {
        logConsole("Unable to clear folder for copy!");
      }
    }
    else {
      SD.mkdir(dest);
    }

    File message = SD.open(source);
    while (true) {
      File packet = message.openNextFile();
      if (! packet) {
        break;
      }
      const char* packetName = packet.name();
      sprintf(packetFileName, "%s/%s", dest, packetName);
      File newPacket = SD.open(packetFileName, FILE_WRITE);

      int bytesRead = 0;
      while ((bytesRead = packet.read(filePacketBuffer, STORAGE_CONTENT_BUFFER_SIZE)) > 0) {
        newPacket.write(filePacketBuffer, bytesRead);
      }
      newPacket.flush();
      newPacket.close();
      packet.close();
      filesCopied++;
    }
    message.close();
  }
  else {
    logConsole("Either source does not exist or dest is unsafe name.");
    logConsole("Source: " + String(source));
    logConsole("Dest: " + String(dest));
  }

  return filesCopied;

}

bool SDPacketStore::readNextAirOutPacketDetails (ChatterPacketMetaData* packetBuffer) {
  return readOldestPacketDetails(packetBuffer, airOutDir);
}

bool SDPacketStore::readNextBridgeOutPacketDetails (ChatterPacketMetaData* packetBuffer) {
  return readOldestPacketDetails(packetBuffer, bridgeOutDir);
}

bool SDPacketStore::moveMessageToAirOut (const char* sender, const char* messageId) {
  char src[STORAGE_MAX_FILENAME_LENGTH];
  char dest[STORAGE_MAX_FILENAME_LENGTH];
  sprintf(src, "%s%s/%s", messageDir, sender, messageId);
  sprintf(dest, "%s%s/%s", airOutDir, sender, messageId);
  if(copyDirectory(src, dest)) {
    if (clearMessage (sender, messageId, messageDir)) {
      return true;
    }
    else {
      logConsole("Copied directory to air out, but not able to remove message from message folder");
    }
  }
  else {
    logConsole("unable to copy message to air out");
  }
  return false;
}

bool SDPacketStore::moveMessageToBridgeOut (const char* sender, const char* messageId) {
  char src[STORAGE_MAX_FILENAME_LENGTH];
  char dest[STORAGE_MAX_FILENAME_LENGTH];
  sprintf(src, "%s%s/%s", messageDir, sender, messageId);
  sprintf(dest, "%s%s/%s", bridgeOutDir, sender, messageId);
  if(copyDirectory(src, dest)) {
    if (clearMessage (sender, messageId, messageDir)) {
      return true;
    }
    else {
      logConsole("Copied directory to bridge out, but not able to remove message from message folder");
    }
  }
  else {
    logConsole("unable to copy message to bridge out");
  }
  return false;
}

int SDPacketStore::readPacketFromAirOut (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer, int maxLength, Encryptor* encryptor) {
  return readPacket(senderId, messageId, packetNum, buffer, maxLength, encryptor, airOutDir);
}

int SDPacketStore::readPacketFromBridgeOut (const char* senderId, const char* messageId, int packetNum, uint8_t* buffer, int maxLength, Encryptor* encryptor) {
  return readPacket(senderId, messageId, packetNum, buffer, maxLength, encryptor, bridgeOutDir);
}

bool SDPacketStore::clearPacketFromAirOut (const char* senderId, const char* messageId, int packetNum) {
  sprintf(packetFileName, "%s%s/%s/%03d", airOutDir, senderId, messageId, packetNum);
  clearFile(packetFileName);
}

bool SDPacketStore::clearPacketFromBridgeOut (const char* senderId, const char* messageId, int packetNum) {
  sprintf(packetFileName, "%s%s/%s/%03d", bridgeOutDir, senderId, messageId, packetNum);
  clearFile(packetFileName);
}

bool SDPacketStore::saveAirMessageTimestamp (const char* senderId, const char* messageId, const char* sortableTime) {
  return saveMessageTimestamp(senderId, messageId, sortableTime, airOutDir);
}

bool SDPacketStore::saveBridgeMessageTimestamp (const char* senderId, const char* messageId, const char* sortableTime) {
  return saveMessageTimestamp(senderId, messageId, sortableTime, bridgeOutDir);
}

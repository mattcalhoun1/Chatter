#include "SDSecrets.h"

bool SDSecrets::init() {
    bool success = true;
    if (!running) {
        // make sure structure exists
        if (!SD.exists(secretsDir)){
            success = SD.mkdir(secretsDir);
        }
        Serial.println("SD Secrets Initialized");

        running = true;
    }
    return success;
   
}

bool SDSecrets::readSlot(uint8_t slot, byte* dataBuffer, uint8_t dataLength) {
    populateFullFileName(slot);
    memset(dataBuffer, 0, dataLength);

    File slotFile = SD.open(fullFileName, FILE_READ);

    int bytesRead = 0;
    int bytesCopied = 0;
    while (bytesRead < dataLength && (bytesRead = slotFile.read(secretsBuffer, ENC_SD_FILE_BUFFER_SIZE)) > 0) {
        memcpy(dataBuffer + bytesCopied, secretsBuffer, bytesRead);
        bytesCopied += bytesRead;
    }
    slotFile.close();
}

bool SDSecrets::writeSlot(uint8_t slot, byte* dataBuffer, uint8_t dataLength) {
    populateFullFileName(slot);
    Serial.println(fullFileName);

    if (SD.exists(fullFileName)) {
        SD.remove(fullFileName);
    }

    File slotFile = SD.open(fullFileName, FILE_WRITE);

    int bytesWritten = 0;
    while (bytesWritten < dataLength) {
        slotFile.write(dataBuffer[bytesWritten++]);
    }
    slotFile.flush();
    slotFile.close();
}

void SDSecrets::populateFullFileName(uint8_t slot) {
  memset(fullFileName, 0, ENC_SD_MAX_FILE_SIZE+1);
  sprintf(fullFileName, "%s%04d", secretsDir, slot);
}

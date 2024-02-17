#include <Arduino.h>
#include "Secrets.h"
#include "EncryptionGlobals.h"
#include <SD.h>

#ifndef SDSECRETS_H
#define SDSECRETS_H

class SDSecrets : public Secrets {
    public:
        bool init();
        bool readSlot(uint8_t slot, byte* dataBuffer, uint8_t dataLength);
        bool writeSlot(uint8_t slot, byte* dataBuffer, uint8_t dataLength);
        bool isRunning() {return running;}
    protected:
        const char* secretsDir = "/secrets/";
        bool running = false;
        char fullFileName[ENC_SD_MAX_FILE_SIZE+1];
        void populateFullFileName(uint8_t slot);
        char secretsBuffer[ENC_SD_FILE_BUFFER_SIZE];

};

#endif

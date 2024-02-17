
// DUE does not like the arduino atecc library
#ifndef ARDUINO_SAM_DUE

#include <Arduino.h>
#include "Hsm.h"
#include "EncryptionGlobals.h"
#include "Secrets.h"
#include "SDSecrets.h"
#include "ArdAteccSecrets.h"

#include <ArduinoECCX08.h>
#include <utility/ECCX08SelfSignedCert.h>
#include <utility/ECCX08DefaultTLSConfig.h>

#ifndef ARDATECCHSM_H
#define ARDATECCHSM_H

class ArdAteccHsm : public Hsm {
    public:
        bool init();
        bool lockDevice (uint8_t defaultPkSlot, uint8_t defaultPkStorage);
        bool generateNewKeypair (uint8_t pkSlot, uint8_t pkStorage);

        long getRandomLong();

        bool loadPublicKey(uint8_t slot, byte* publicKeyBuffer);
        bool verifySignature(uint8_t* message, uint8_t* signature, const byte* publicKey);
        bool sign (uint8_t slot, uint8_t* message, uint8_t* signatureBuffer);

        bool readSlot(uint8_t slot, byte* dataBuffer, uint8_t dataLength);
        bool writeSlot(uint8_t slot, byte* dataBuffer, uint8_t dataLength);
    
    protected:
        void logConsole(const char* message);
        void logConsole(String message);
        Secrets* secrets;
        int freeMemory();

};

#endif

#endif
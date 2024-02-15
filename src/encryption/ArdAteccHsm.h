
// DUE does not like the arduino atecc library
#ifndef ARDUINO_SAM_DUE

#include <Arduino.h>
#include "Hsm.h"
#include "EncryptionGlobals.h"

#include <ArduinoECCX08.h>
#include <utility/ECCX08SelfSignedCert.h>
#include <utility/ECCX08DefaultTLSConfig.h>

#ifndef ARDATECCHSM_H
#define ARDATECCHSM_H

class ArdAteccHsm : public Hsm {
    public:
        bool init();
        bool lockDevice (int defaultPkSlot, int defaultPkStorage);
        bool generateNewKeypair (int pkSlot, int pkStorage);

        long getRandomLong();

        bool loadPublicKey(int slot, byte* publicKeyBuffer);
        bool verifySignature(uint8_t* message, uint8_t* signature, const byte* publicKey);
        bool sign (int slot, uint8_t* message, uint8_t* signatureBuffer);

        bool readSlot(int slot, byte* dataBuffer, int dataLength);
        bool writeSlot(int slot, byte* dataBuffer, int dataLength);
    
    protected:
        void logConsole(const char* message);
        void logConsole(String message);
};

#endif

#endif
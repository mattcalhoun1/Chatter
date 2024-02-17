#include "SparkAteccHsm.h"

// only for due
#ifdef ARDUINO_SAM_DUE

bool SparkAteccHsm::init() {
    logConsole("Beginning Sparkfun Atecc HSM");
    bool result = atecc.begin();
    if (result) {
        logConsole("Atecc is running");
    }
    else {
        logConsole("Atecc is NOT running");
    }
    return result;
}

bool SparkAteccHsm::lockDevice (uint8_t defaultPkSlot, uint8_t defaultPkStorage) {
    atecc.readConfigZone(false); // Debug argument false (OFF)
    if (!atecc.configLockStatus) {
        if (!atecc.writeConfigSparkFun()) {
            logConsole("Hsm write config failed!");
            return false;
        }

        if (!atecc.lockConfig()) {
            logConsole("Hsm lock config failed!");
            return false;
        }

        if (!atecc.dataOTPLockStatus || !atecc.slot0LockStatus)
        // since this device was just locked, generate the initial keypair
        if(!atecc.createNewKeyPair()) {
            logConsole("Hsm keypair create failed");
        }

        if (!atecc.lockDataAndOTP()) {
            logConsole("Hsm lock data and otp failed");
        }
        if (!atecc.lockDataSlot0()) {
            logConsole("Hsm lock signing key failed");
        }

        if (atecc.dataOTPLockStatus  && atecc.slot0LockStatus) {
            logConsole("Hsm successfully locked");
            return true;
        }
        else {
            logConsole("Hsm otp or slot 0 not locked");
            return false;
        }
    }
    else {
        logConsole("Device is properly locked");
    }

  return true;
}

bool SparkAteccHsm::generateNewKeypair (uint8_t pkSlot, uint8_t pkStorage) {
  return atecc.createNewKeyPair(pkSlot);
}

long SparkAteccHsm::getRandomLong() {
    return atecc.random(65535);
}

bool SparkAteccHsm::loadPublicKey(uint8_t slot, byte* publicKeyBuffer) {
    if(atecc.generatePublicKey(slot, false)) {
        memcpy(publicKeyBuffer, atecc.publicKey64Bytes, ENC_ECC_DSA_KEY_SIZE);
        return true;
    }
    logConsole("Hsm failed to generate pub key");
    return false;
}

bool SparkAteccHsm::verifySignature(uint8_t* message, uint8_t* signature, const byte* publicKey) {
    return atecc.verifySignature(message, signature, (uint8_t*)publicKey);
}

bool SparkAteccHsm::sign (uint8_t slot, uint8_t* message, uint8_t* signatureBuffer) {
    logConsole("SparkAteccHsm: invoking sign");
    delay(500);
    memset(signatureBuffer, 0, ENC_SIGNATURE_SIZE);

    if (!atecc.loadTempKey(message)) {
        logConsole("load temp key failed");
    }
    else {
        logConsole("load temp key success");
    }

    atecc.createSignature(message);
    logConsole("SparkAteccHsm: sig created");
    delay(500);

    for (int i = 0; i < 64; i++) {
        Serial.print(atecc.signature[i]);Serial.print(" ");
    }
    Serial.println("");

    memcpy(signatureBuffer, atecc.signature, ENC_SIGNATURE_SIZE);


    logConsole("SparkAteccHsm: mem copied");
    delay(500);

    return true;
}

bool SparkAteccHsm::readSlot(uint8_t slot, byte* dataBuffer, uint8_t dataLength) {
    return atecc.read_output(ZONE_DATA, getEepromAddress(slot, 0, 0), dataLength, (uint8_t*)dataBuffer, false);
}

bool SparkAteccHsm::writeSlot(uint8_t slot, byte* dataBuffer, uint8_t dataLength) {
    return atecc.write(ZONE_DATA, getEepromAddress(slot, 0, 0), (uint8_t*) dataBuffer, dataLength);
}

uint16_t SparkAteccHsm::getEepromAddress (uint8_t slot, uint8_t block, uint8_t offset) {
    // on atecc608 , one block is 32 bytes. each data slot has 2 blocks (64) plus a few more byte to make it 72 total
    return EEPROM_DATA_ADDRESS(slot, block, offset);	
}


void SparkAteccHsm::logConsole (const char* message) {
    if (ENC_LOG_ENABLED) {
        Serial.println(message);
    }
}

#endif
#include "ArdAteccHsm.h"

// DUE does not like the arduino atecc library
#ifndef ARDUINO_SAM_DUE

bool ArdAteccHsm::init() {
  return ECCX08.begin();
}

bool ArdAteccHsm::lockDevice (int defaultPkSlot, int defaultPkStorage) {
  if (!ECCX08.locked()) {
    if (!ECCX08.writeConfiguration(ECCX08_DEFAULT_TLS_CONFIG)) {
      logConsole("Writing default config to encryption device failed");
      return false;
    }

    if (!ECCX08.lock()) {
      logConsole("Locking ECCX08 configuration failed!");
      return false;
    }

    logConsole("Encryption device successfully locked");

    // since this device was just locked, generate the initial keypair
    return generateNewKeypair(defaultPkSlot, defaultPkStorage);
  }

  return true;
}

bool ArdAteccHsm::generateNewKeypair (int pkSlot, int pkStorage) {
  if (!ECCX08SelfSignedCert.beginStorage(pkSlot, pkStorage, true)) {
    logConsole("Error starting self signed cert generation!");
    return false;
  }

  // if rtc is available, use that instead
  ECCX08SelfSignedCert.setCommonName(ECCX08.serialNumber());
  ECCX08SelfSignedCert.setIssueYear(ENC_SSC_YEAR);
  ECCX08SelfSignedCert.setIssueMonth(ENC_SSC_MONTH);
  ECCX08SelfSignedCert.setIssueDay(ENC_SSC_DATE);
  ECCX08SelfSignedCert.setIssueHour(ENC_SSC_HOUR);
  ECCX08SelfSignedCert.setExpireYears(ENC_SSC_VALID);

  String cert = ECCX08SelfSignedCert.endStorage();

  if (!cert) {
    logConsole("Error generating self signed cert!");
    return false;
  }

  logConsole("New Cert: ");
  logConsole(cert);

  logConsole("SHA1: ");
  logConsole(ECCX08SelfSignedCert.sha1());

  return true;
}

long ArdAteccHsm::getRandomLong() {
    return ECCX08.random(65535);
}

bool ArdAteccHsm::loadPublicKey(int slot, byte* publicKeyBuffer) {
    return ECCX08.generatePublicKey(slot, publicKeyBuffer) == 1;
}

bool ArdAteccHsm::verifySignature(uint8_t* message, uint8_t* signature, const byte* publicKey) {
    return ECCX08.ecdsaVerify(message, signature, publicKey) == 1;
}

bool ArdAteccHsm::sign (int slot, uint8_t* message, uint8_t* signatureBuffer) {
    return ECCX08.ecSign(slot, message, signatureBuffer) == 1;
}

bool ArdAteccHsm::readSlot(int slot, byte* dataBuffer, int dataLength) {
    return ECCX08.readSlot(slot, dataBuffer, dataLength) == 1;
}

bool ArdAteccHsm::writeSlot(int slot, byte* dataBuffer, int dataLength) {
    return ECCX08.writeSlot(slot, dataBuffer, dataLength) == 1;
}

void ArdAteccHsm::logConsole (const char* message) {
    if (ENC_LOG_ENABLED) {
        Serial.println(message);
    }
}

void ArdAteccHsm::logConsole (String message) {
    logConsole(message.c_str());
}

#endif
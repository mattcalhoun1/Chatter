#include "ArdAteccHsm.h"

// DUE does not like the arduino atecc library
#ifndef ARDUINO_SAM_DUE

bool ArdAteccHsm::init() {
  bool success = ECCX08.begin();

  if (success) {
#if defined (ARDUINO_SAMD_NANO_33_IOT)
    secrets = new SDSecrets();
#elif defined (ARDUINO_UNOR4_WIFI)
    secrets = new ArdAteccSecrets();
#endif
    success = secrets->init();
  }
  else {
    logConsole("SECRETES not intiialized");
  }

  return success;
}

bool ArdAteccHsm::lockDevice (uint8_t defaultPkSlot, uint8_t defaultPkStorage) {
  if (!ECCX08.locked()) {
    logConsole("Device not locked");

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

bool ArdAteccHsm::generateNewKeypair (uint8_t pkSlot, uint8_t pkStorage) {
  if (!ECCX08SelfSignedCert.beginStorage(pkSlot, pkStorage, true)) {
    logConsole("Error starting self signed cert generation in pk slot: " + String(pkSlot) + " storage slot: " + String(pkStorage));
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

bool ArdAteccHsm::loadPublicKey(uint8_t slot, byte* publicKeyBuffer) {
    return ECCX08.generatePublicKey(slot, publicKeyBuffer) == 1;
}

bool ArdAteccHsm::verifySignature(uint8_t* message, uint8_t* signature, const byte* publicKey) {
    return ECCX08.ecdsaVerify(message, signature, publicKey) == 1;
}

bool ArdAteccHsm::sign (uint8_t slot, uint8_t* message, uint8_t* signatureBuffer) {
    return ECCX08.ecSign(slot, message, signatureBuffer) == 1;
}

bool ArdAteccHsm::readSlot(uint8_t slot, byte* dataBuffer, uint8_t dataLength) {
    return secrets->readSlot(slot, dataBuffer, dataLength);
}

bool ArdAteccHsm::writeSlot(uint8_t slot, byte* dataBuffer, uint8_t dataLength) {
    return secrets->writeSlot(slot, dataBuffer, dataLength);
}

void ArdAteccHsm::logConsole (const char* message) {
    if (ENC_LOG_ENABLED) {
        Serial.println(message);
    }
}

void ArdAteccHsm::logConsole (String message) {
    logConsole(message.c_str());
}


#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__

int ArdAteccHsm::freeMemory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}
#endif
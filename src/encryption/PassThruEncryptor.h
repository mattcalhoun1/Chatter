#include "Encryptor.h"

#ifndef PASSTHRUENCRYPTOR_H
#define PASSTHRUENCRYPTOR_H

class PassThruEncryptor : public Encryptor {
  public:
    PassThruEncryptor(TrustStore* _trustStore) : Encryptor(_trustStore) {}
    bool init ();

    void encrypt(const char* plainText, int len, int slot);
    void decrypt(uint8_t* encrypted, int len, int slot);
    void encryptVolatile(const char* plainText, int len);
    void decryptVolatile(uint8_t* encrypted, int len);

  protected:
    void prepareForEncryption ();
    void prepareForVolatileEncryption();
};
#endif
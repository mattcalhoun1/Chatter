#include "Encryptor.h"
#include <ChaCha.h>

#ifndef CHACHAENCRYPTOR_H
#define CHACHAENCRYPTOR_H

class ChaChaEncryptor : public Encryptor {
  public:
    ChaChaEncryptor(TrustStore* _trustStore) : Encryptor(_trustStore) {}
    bool init ();

    void encrypt(const char* plainText, int len, int slot);
    void decrypt(uint8_t* encrypted, int len, int slot);
    void encryptVolatile(const char* plainText, int len);
    void decryptVolatile(uint8_t* encrypted, int len);

  protected:
    void prepareForEncryption ();
    void prepareForVolatileEncryption();

    ChaCha chacha;
    ChaCha volatileChacha;
};

#endif
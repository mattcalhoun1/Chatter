#include <Arduino.h>
#include "../chat/ChatGlobals.h"
#include "EncryptionGlobals.h"
#include <SHA256.h>
#include "../storage/TrustStore.h"
#include "Hsm.h"
#include "EncryptionAlgo.h"
#include "ChaChaAlgo.h"

#ifndef ENCRYPTOR_H
#define ENCRYPTOR_H

class Encryptor {
  public:
    Encryptor(TrustStore* _trustStore, Hsm* _hsm) {trustStore = _trustStore, hsm = _hsm;}
    bool init ();
    //const char* getDeviceId(); // unique alphanumeric id of this device
    long getRandom();
    bool verify();
    bool verify(const byte pubkey[]);
    bool signMessage();

    bool loadPublicKey ();
    bool setPublicKeyBuffer (const char* publicKey);
    byte* getPublicKeyBuffer ();

    void setSignatureBuffer(byte* sigBuffer);
    byte* getSignatureBuffer ();
    void setMessageBuffer (byte* messageBuffer);
    byte* getMessageBuffer ();

    void logBufferHex(const byte input[], int inputLength);

    // encrypt the value, place cleartext in encrypted buffer
    void encrypt(const char* plainText, int len);
    void encryptVolatile(const char* plainText, int len);

    // decrypt the value, place cleartext in unencrypted buffer
    void decrypt(uint8_t* encrypted, int len);
    void decryptVolatile(uint8_t* encrypted, int len);

    // buffers containing the result of encryption or decryption
    uint8_t* getEncryptedBuffer();
    uint8_t* getUnencryptedBuffer();
    int getEncryptedBufferLength();
    int getUnencryptedBufferLength();


    int generateHash(const char* plainText, int inputLength, uint8_t* hashBuffer);

    void hexify (const byte input[], int inputLength);
    const char* getHexBuffer ();

   // uint8_t* getVolatileEncryptionKey () {return volatileEncryptionKey;}

  protected:
    Hsm* hsm;
    TrustStore* trustStore;
    void logConsole(String msg);
    byte signatureBuffer[ENC_SIG_BUFFER_SIZE];
    byte messageBuffer[ENC_MSG_BUFFER_SIZE]; // can only sign exactly 32 bytes
    byte publicKeyBuffer[ENC_PUB_KEY_SIZE]; // holds public key for validating messagees
    byte dataSlotBuffer[ENC_DATA_SLOT_BUFFER_SIZE];

    /* Hex related. generally for dealing with aes key / atecc608 data slots */
    void hexCharacterStringToBytes(byte *byteArray, const char *hexString, int maxBufferSize);
    void hexCharacterStringToBytesMax(byte *byteArray, const char *hexString, int hexLength, int maxBufferSize);
    byte nibble(char c);

    int loadedDataSlot = -1; // which data slot is currently loaded (if any)
    int loadedEncryptionKeySlot = -1; // which encryption key is currently loaded (if any)

    uint8_t unencryptedBuffer[ENC_UNENCRYPTED_BUFFER_SIZE];
    uint8_t encryptedBuffer[ENC_ENCRYPTED_BUFFER_SIZE];
    char hexBuffer[ENC_HEX_BUFFER_SIZE];

    int findEncryptionBufferEnd (uint8_t* buffer, int maxLen);
    int findDecryptionBufferEnd (uint8_t* buffer, int maxLen);

};
#endif
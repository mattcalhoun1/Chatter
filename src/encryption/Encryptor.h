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
    bool setPublicKeyBuffer (uint8_t* publicKey);
    uint8_t* getPublicKeyBuffer ();

    void setSignatureBuffer(uint8_t* sigBuffer);
    uint8_t* getSignatureBuffer ();
    void setMessageBuffer (uint8_t* messageBuffer);
    uint8_t* getMessageBuffer ();

    void logBufferHex(const byte input[], int inputLength);

    // encrypt the value, place cleartext in encrypted buffer
    void encrypt(const char* plainText, int len);
    void encryptVolatile(const char* plainText, int len);
    void encryptForRecipient (const uint8_t* recipientPublicKey, const char* plainText, int len);

    // decrypt the value, place cleartext in unencrypted buffer
    void decrypt(uint8_t* encrypted, int len);
    void decryptVolatile(uint8_t* encrypted, int len);
    void decryptFromSender (const uint8_t* senderPublicKey, uint8_t* encrypted, int len);

    // buffers containing the result of encryption or decryption
    uint8_t* getEncryptedBuffer();
    uint8_t* getUnencryptedBuffer();
    int getEncryptedBufferLength();
    int getUnencryptedBufferLength();

    int baseUnencryptedBufferLength = 0; // if encrypting, this is what the unecrypted length was, prior to encrypting
    int baseEncryptedBufferLength = 0; // if decrypting, this is what the encrypted lenvth was, prior to decrypting


    int generateHash(const char* plainText, int inputLength, uint8_t* hashBuffer);

    static void hexify (char* outputBuffer, const byte input[], int inputLength);
    void hexify (const byte input[], int inputLength);
    const char* getHexBuffer ();
    void clearHexBuffer ();

    void hexCharacterStringToBytes(byte *byteArray, const char *hexString, int maxBufferSize);
    void hexCharacterStringToBytesMax(byte *byteArray, const char *hexString, int hexLength, int maxBufferSize);

  protected:
    Hsm* hsm;
    TrustStore* trustStore;
    void logConsole(String msg);
    uint8_t signatureBuffer[ENC_SIG_BUFFER_SIZE];
    uint8_t messageBuffer[ENC_MSG_BUFFER_SIZE]; // can only sign exactly 32 bytes
    uint8_t publicKeyBuffer[ENC_PUB_KEY_SIZE]; 
    //uint8_t dataSlotBuffer[ENC_DATA_SLOT_BUFFER_SIZE];

    byte nibble(char c);

    //int loadedDataSlot = -1; // which data slot is currently loaded (if any)
    //int loadedEncryptionKeySlot = -1; // which encryption key is currently loaded (if any)

    uint8_t unencryptedBuffer[ENC_UNENCRYPTED_BUFFER_SIZE];
    uint8_t encryptedBuffer[ENC_ENCRYPTED_BUFFER_SIZE];
    char hexBuffer[ENC_HEX_BUFFER_SIZE];

    int findEncryptionBufferEnd (uint8_t* buffer, int maxLen);
    int findDecryptionBufferEnd (uint8_t* buffer, int maxLen);

};
#endif
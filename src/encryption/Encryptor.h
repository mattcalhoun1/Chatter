#include <Arduino.h>
#include "../chat/ChatGlobals.h"
#include "EncryptionGlobals.h"
#include <SHA256.h>
#include "../storage/TrustStore.h"
#include "Hsm.h"
#include "EncryptionAlgo.h"
#include "ChaChaAlgo.h"

/*
   Based on Atecc608
   Data slots:
    Slots 0-7 Contain 36 Bytes
    Slot 8 Contains 416 Bytes
    Slots 9-15 Contain 72 Bytes
 */

#ifndef ENCRYPTOR_H
#define ENCRYPTOR_H

class Encryptor {
  public:
    Encryptor(TrustStore* _trustStore, Hsm* _hsm) {trustStore = _trustStore, hsm = _hsm;}
    bool init ();
    const char* getDeviceId(); // unique alphanumeric id of this device
    long getRandom();
    bool verify();
    bool verify(uint8_t slot);
    bool verify(const byte pubkey[]);
    bool signMessage(uint8_t slot);

    bool loadPublicKey (uint8_t slot);
    bool setPublicKeyBuffer (const char* publicKey);
    byte* getPublicKeyBuffer ();

    void setSignatureBuffer(byte* sigBuffer);
    byte* getSignatureBuffer ();
    void setMessageBuffer (byte* messageBuffer);
    byte* getMessageBuffer ();

    bool loadDataSlot(uint8_t slot);
    bool saveDataSlot(uint8_t slot);

    // text and data slots are the same, just treated differently
    // depending which method you choose (hex vs clear text)
    void setDataSlotBuffer (const char* hexData);
    void setDataSlotBuffer (byte* data);
    byte* getDataSlotBuffer ();

    void setTextSlotBuffer (const char* textData);
    int getTextSlotBuffer (char* target);

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

    //bool generateNewKeypair (int pkSlot, int pkStorage, int year, int month, int day, int hour, int expire);

    void hexify (const byte input[], int inputLength);
    const char* getHexBuffer ();

    uint8_t* getVolatileEncryptionKey () {return volatileEncryptionKey;}

  protected:
    Hsm* hsm;
    EncryptionAlgo* algo;
    TrustStore* trustStore;
    void logConsole(String msg);
    byte signatureBuffer[ENC_SIG_BUFFER_SIZE];
    byte messageBuffer[ENC_MSG_BUFFER_SIZE]; // can only sign exactly 32 bytes
    byte publicKeyBuffer[ENC_PUB_KEY_SIZE]; // holds public key for validating messagees
    byte dataSlotBuffer[ENC_DATA_SLOT_BUFFER_SIZE];
    bool loadEncryptionKey (uint8_t slot);
    void syncKeys ();
    void generateNextVolatileKey();

    /* Hex related. generally for dealing with aes key / atecc608 data slots */
    void hexCharacterStringToBytes(byte *byteArray, const char *hexString, int maxBufferSize);
    void hexCharacterStringToBytesMax(byte *byteArray, const char *hexString, int hexLength, int maxBufferSize);
    byte nibble(char c);

    int loadedDataSlot = -1; // which data slot is currently loaded (if any)
    int loadedEncryptionKeySlot = -1; // which encryption key is currently loaded (if any)

    uint8_t encryptionKeyBuffer[ENC_SYMMETRIC_KEY_BUFFER_SIZE];
    uint8_t volatileEncryptionKey[ENC_SYMMETRIC_KEY_BUFFER_SIZE];

    uint8_t unencryptedBuffer[ENC_UNENCRYPTED_BUFFER_SIZE];
    uint8_t encryptedBuffer[ENC_ENCRYPTED_BUFFER_SIZE];
    char hexBuffer[ENC_HEX_BUFFER_SIZE];
    char deviceId[ENC_DATA_SLOT_SIZE + 1];

    int findEncryptionBufferEnd (uint8_t* buffer, int maxLen);
    int findDecryptionBufferEnd (uint8_t* buffer, int maxLen);

};
#endif
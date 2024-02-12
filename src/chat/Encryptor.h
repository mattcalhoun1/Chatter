#include <Arduino.h>
#include <ArduinoECCX08.h>
#include "ChatGlobals.h"
#include <SHA256.h>
#include "../storage/TrustStore.h"
#include <utility/ECCX08SelfSignedCert.h>
#include <utility/ECCX08DefaultTLSConfig.h>


/*
   Based on Atecc608
   Data slots:
    Slots 0-7 Contain 36 Bytes
    Slot 8 Contains 416 Bytes
    Slots 9-15 Contain 72 Bytes
 */

#ifndef ENCRYPTOR_H
#define ENCRYPTOR_H

#define ENC_SYMMETRIC_KEY_SIZE 16
#define ENC_SYMMETRIC_KEY_BUFFER_SIZE 33
#define ENC_VOLATILE_KEY_SIZE 32
#define ENC_DATA_SLOT_SIZE 32
#define ENC_DATA_SLOT_BUFFER_SIZE 33 // is the driver overrunnign our buffer when we load slot 10?
#define ENC_HEX_BUFFER_SIZE 129
#define ENC_MSG_BUFFER_SIZE 32 // only 32 byte messages are allowed for sig
#define ENC_SIG_BUFFER_SIZE 64
#define ENC_PUB_KEY_SIZE 128

#define ENC_UNENCRYPTED_BUFFER_SIZE 150 // really only need closer to 128, but extra since encryption/decryption can change size
#define ENC_ENCRYPTED_BUFFER_SIZE 150

class Encryptor {
  public:
    Encryptor(TrustStore* _trustStore) {trustStore = _trustStore;}
    virtual bool init ();
    const char* getDeviceId(); // unique alphanumeric id of this device
    long getRandom();
    bool verify();
    bool verify(int slot);
    bool verify(const byte pubkey[]);
    int sign(int slot);

    bool loadPublicKey (int slot);
    bool setPublicKeyBuffer (const char* publicKey);
    byte* getPublicKeyBuffer ();

    void setSignatureBuffer(byte* sigBuffer);
    byte* getSignatureBuffer ();
    void setMessageBuffer (byte* messageBuffer);
    byte* getMessageBuffer ();

    bool loadDataSlot(int slot);
    bool saveDataSlot(int slot);

    // text and data slots are the same, just treated differently
    // depending which method you choose (hex vs clear text)
    void setDataSlotBuffer (const char* hexData);
    void setDataSlotBuffer (byte* data);
    byte* getDataSlotBuffer ();

    void setTextSlotBuffer (const char* textData);
    int getTextSlotBuffer (char* target);

    void logBufferHex(const byte input[], int inputLength);

    virtual void encrypt(const char* plainText, int len, int slot) = 0;
    virtual void decrypt(uint8_t* encrypted, int len, int slot) = 0;
    virtual void encryptVolatile(const char* plainText, int len) = 0;
    virtual void decryptVolatile(uint8_t* encrypted, int len) = 0;
    uint8_t* getEncryptedBuffer();
    uint8_t* getUnencryptedBuffer();
    int getEncryptedBufferLength();
    int getUnencryptedBufferLength();


    int generateHash(const char* plainText, int inputLength, uint8_t* hashBuffer);

    // debugging - move back to private
    void loadEncryptionKey (int slot);

    bool generateNewKeypair (int pkSlot, int pkStorage, int year, int month, int day, int hour, int expire);

    void hexify (const byte input[], int inputLength);
    const char* getHexBuffer ();

  protected:
    bool lockEncryptionDevice ();

    void logConsole(String msg);
    byte signatureBuffer[ENC_SIG_BUFFER_SIZE];
    byte messageBuffer[ENC_MSG_BUFFER_SIZE]; // can only sign exactly 32 bytes
    byte publicKeyBuffer[ENC_PUB_KEY_SIZE]; // holds public key for validating messagees
    byte dataSlotBuffer[ENC_DATA_SLOT_BUFFER_SIZE];
    void syncKeys ();
    void generateNextVolatileKey ();

    /* Hex related. generally for dealing with aes key / atecc608 data slots */
    void hexCharacterStringToBytes(byte *byteArray, const char *hexString);
    void hexCharacterStringToBytes(byte *byteArray, const char *hexString, int hexLength);
    byte nibble(char c);

    int loadedDataSlot = -1; // which data slot is currently loaded (if any)
    int loadedEncryptionKeySlot = -1; // which encryption key is currently loaded (if any)

    uint8_t unencryptedBuffer[ENC_UNENCRYPTED_BUFFER_SIZE];
    uint8_t encryptedBuffer[ENC_ENCRYPTED_BUFFER_SIZE];
    char hexBuffer[ENC_HEX_BUFFER_SIZE];
    uint8_t encryptionKeyBuffer[ENC_SYMMETRIC_KEY_BUFFER_SIZE];
    uint8_t volatileEncryptionKey[ENC_SYMMETRIC_KEY_BUFFER_SIZE];
    char deviceId[ENC_DATA_SLOT_SIZE + 1];

    int findEncryptionBufferEnd (uint8_t* buffer, int maxLen);
    int findDecryptionBufferEnd (uint8_t* buffer, int maxLen);
    virtual void prepareForEncryption () = 0;
    virtual void prepareForVolatileEncryption() = 0;

    TrustStore* trustStore;
};
#endif
#include "FramGlobals.h"
#include "../StorageGlobals.h"
#include <Arduino.h>
#include "FramRecord.h"
#include "ChaCha.h"
#include <SHA256.h>
#include "FramHardware.h"
#include "FramAdafruitSPI.h"
#include "FramAdafruitI2C.h"

#ifndef FRAMDATA_H
#define FRAMDATA_H

#define FRAM_PACKET_SIZE 174
#define FRAM_TS_SIZE 12

#define FRAM_ZONE_METADATA_SIZE 4 // 4 bytes, 0 = num slots used, 1 = newest slot, 2 = reserved, 3 = reserved

#define FRAM_DEVICE_LOC 0x00
#define FRAM_DEVICE_KEYSIZE 16
#define FRAM_DEVICE_DATASIZE_USABLE 32 // how much cleartext to allow in. encryption will bump the size on fram
#define FRAM_DEVICE_DATASIZE 32 + 32 // only storing 16 bytes, but double to allow for growth from encryption
#define FRAM_DEVICE_ENCRYPTED true
#define FRAM_DEVICE_VOLATILE false
#define FRAM_DEVICE_SLOTS 10

#define FRAM_CLUSTER_LOC FRAM_DEVICE_LOC + (FRAM_DEVICE_KEYSIZE + FRAM_DEVICE_DATASIZE) * FRAM_DEVICE_SLOTS + FRAM_ZONE_METADATA_SIZE
#define FRAM_CLUSTER_KEYSIZE STORAGE_GLOBAL_NET_ID_SIZE + STORAGE_LOCAL_NET_ID_SIZE + 1 // ex: USABCA for cluster USABC, active status
#define FRAM_CLUSTER_DATASIZE 150 // leave room for encryption
#define FRAM_CLUSTER_ENCRYPTED true
#define FRAM_CLUSTER_VOLATILE false
#define FRAM_CLUSTER_SLOTS 20

#define FRAM_ENC_BUFFER_SIZE 174

#define FRAM_TRUST_FLAGS 3 // number of trust flags (channel(s), etc)
#define FRAM_TRUST_LOC FRAM_CLUSTER_LOC + (FRAM_CLUSTER_KEYSIZE + FRAM_CLUSTER_DATASIZE) * FRAM_CLUSTER_SLOTS + FRAM_ZONE_METADATA_SIZE
#define FRAM_TRUST_KEYSIZE CHATTER_DEVICE_ID_SIZE + 1 // +1 for status
#define FRAM_TRUST_DATASIZE ENC_PUB_KEY_SIZE+CHATTER_ALIAS_NAME_SIZE + FRAM_TRUST_FLAGS
#define FRAM_TRUST_ENCRYPTED false
#define FRAM_TRUST_VOLATILE false
#define FRAM_TRUST_SLOTS STORAGE_MAX_CLUSTER_DEVICES

// sender size of device id
// packet number 3 digits
// message id 3 digits
// timestamp 12 char
// so far, used 4248 bytes of fram. the number of slots we have left for packets depends on how much
// fram is on the chip. 
// 32k chip = 32768 bytes - 4248 == 28520
// each packet uses 15 bytes for the key (8 + 3 + 3 + 1), and we have 4 bytes at the zone front for metadata
//    and 171 for the encrypted payload
//    for a total of 186 bytes per packet
//    or 153 packets. To be save, we'll keep this at 120 packets, and should have empty area at the end of fram

#define FRAM_PACKET_LOC FRAM_TRUST_LOC + (FRAM_TRUST_KEYSIZE + FRAM_TRUST_DATASIZE) * FRAM_TRUST_SLOTS + FRAM_ZONE_METADATA_SIZE //  
#define FRAM_PACKET_KEYSIZE CHATTER_DEVICE_ID_SIZE + CHATTER_MESSAGE_ID_SIZE + CHATTER_CHUNK_ID_SIZE + 1 // +1 for packet status
#define FRAM_PACKET_DATASIZE CHATTER_FULL_BUFFER_LEN + CHATTER_DEVICE_ID_SIZE + FRAM_TS_SIZE + 1 // 1 for the packet length
#define FRAM_PACKET_ENCRYPTED true
#define FRAM_PACKET_VOLATILE true
#define FRAM_PACKET_SLOTS 110 // total fram usage should be 32336 of 32768

#define FRAM_LICENSE_LOC FRAM_PACKET_LOC + (FRAM_PACKET_KEYSIZE + FRAM_PACKET_DATASIZE) * FRAM_PACKET_SLOTS + FRAM_ZONE_METADATA_SIZE //  
#define FRAM_LICENSE_KEYSIZE STORAGE_GLOBAL_NET_ID_SIZE + STORAGE_LOCAL_NET_ID_SIZE + 1 // +1 for status
#define FRAM_LICENSE_DATASIZE CHATTER_DEVICE_ID_SIZE + 64 // just the signer and the sig, unencrypted
#define FRAM_LICENSE_ENCRYPTED false
#define FRAM_LICENSE_VOLATILE false
#define FRAM_LICENSE_SLOTS FRAM_CLUSTER_SLOTS // same as # of clusters, we need a license slot for each one

// still thinking through message layout
//#define FRAM_MESSAGE_LOC FRAM_LICENSE_LOC + (FRAM_LICENSE_KEYSIZE + FRAM_LICENSE_DATASIZE) * FRAM_LICENSE_SLOTS + FRAM_ZONE_METADATA_SIZE //  
//#define FRAM_MESSAGE_KEYSIZE STORAGE_GLOBAL_NET_ID_SIZE + STORAGE_LOCAL_NET_ID_SIZE + 1 // +1 for status
//#define FRAM_MESSAGE_DATASIZE CHATTER_DEVICE_ID_SIZE + 64 // just the signer and the sig, unencrypted
//#define FRAM_MESSAGE_ENCRYPTED true
//#define FRAM_MESSAGE_VOLATILE false
//#define FRAM_MESSAGE_SLOTS FRAM_CLUSTER_SLOTS // same as # of clusters, we need a license slot for each one

#define FRAM_NULL 255
#define FRAM_MAX_KEYSIZE FRAM_DEVICE_KEYSIZE

//uint32_t max_addr;

enum FramConnectionType {
    FramSPI = 0,
    FramI2C = 1
};

class FramData {
  public:
    FramData (const uint8_t* _key, const uint8_t* _volatileKey, FramConnectionType _connectionType);
    FramData (const char* _passphrase, uint8_t _length, FramConnectionType _connectionType);
    virtual bool init ();

    virtual uint8_t getNumUsedSlots (FramZone zone);
    virtual uint8_t getNextSlot (FramZone zone);
    virtual bool clearZone (FramZone zone);
    virtual bool clearAllZones ();

    bool isRunning () {return running;}

    bool writeToNextSlot (FramRecord* record);

    virtual bool writeRecord (FramRecord* record, uint8_t slot);
    virtual bool readRecord (FramRecord* record, uint8_t slot);

    virtual bool recordExists (FramZone zone, uint8_t* recordKey);
    virtual uint8_t getRecordNum (FramZone zone, uint8_t* recordKey) = 0;
    virtual void readKey(uint8_t* buffer, FramZone zone, uint8_t slot) = 0;

    virtual void logCache() = 0;

  protected:
    void encrypt(uint8_t* clear, int clearLength);
    void decrypt(uint8_t* encrypted, int encryptedLength);

    void encryptVolatile(uint8_t* clear, int clearLength);
    void decryptVolatile(uint8_t* encrypted, int encryptedLength);

    uint16_t getFramDataLocation (FramZone zone, uint8_t slot);
    uint16_t getFramKeyLocation (FramZone zone, uint8_t slot);

    uint8_t encryptedBuffer[FRAM_ENC_BUFFER_SIZE];
    uint8_t unencryptedBuffer[FRAM_ENC_BUFFER_SIZE];

    uint8_t guessEncryptionBufferSize (uint8_t* buffer, uint8_t terminator);

    ChaCha chacha;
    ChaCha chachaVolatile;

    uint8_t iv[ENC_IV_SIZE];

    uint16_t zoneLocations[FRAM_NUM_ZONES] = { FRAM_DEVICE_LOC, FRAM_CLUSTER_LOC, FRAM_TRUST_LOC, FRAM_PACKET_LOC, FRAM_LICENSE_LOC };
    uint8_t zoneKeySizes[FRAM_NUM_ZONES] = { FRAM_DEVICE_KEYSIZE, FRAM_CLUSTER_KEYSIZE, FRAM_TRUST_KEYSIZE, FRAM_PACKET_KEYSIZE, FRAM_LICENSE_KEYSIZE };
    uint8_t zoneSlots[FRAM_NUM_ZONES] = { FRAM_DEVICE_SLOTS, FRAM_CLUSTER_SLOTS, FRAM_TRUST_SLOTS, FRAM_PACKET_SLOTS, FRAM_LICENSE_SLOTS };
    uint8_t zoneDataSizes[FRAM_NUM_ZONES] = { FRAM_DEVICE_DATASIZE, FRAM_CLUSTER_DATASIZE, FRAM_TRUST_DATASIZE, FRAM_PACKET_DATASIZE, FRAM_LICENSE_DATASIZE };
    uint8_t zoneEncrypted[FRAM_NUM_ZONES] = { FRAM_DEVICE_ENCRYPTED, FRAM_CLUSTER_ENCRYPTED, FRAM_TRUST_ENCRYPTED, FRAM_PACKET_ENCRYPTED, FRAM_LICENSE_ENCRYPTED };
    uint8_t zoneVolatile[FRAM_NUM_ZONES] = { FRAM_DEVICE_VOLATILE, FRAM_CLUSTER_VOLATILE, FRAM_TRUST_VOLATILE, FRAM_PACKET_VOLATILE, FRAM_LICENSE_VOLATILE };

    FramZone zoneId[FRAM_NUM_ZONES] = {ZoneDevice, ZoneCluster, ZoneTrust, ZonePacket, ZoneLicense};

    FramConnectionType connectionType;
    FramHardware* fram;
    //Adafruit_EEPROM_I2C fram;
    bool running = false;

    uint8_t keyBuffer[FRAM_MAX_KEYSIZE]; // for temporarily holding keys, various purposes. choosing largest key size

};

#endif
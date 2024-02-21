#include "FramData.h"
#include "PacketData.h"
#include "ClusterConfig.h"
#include "TrustConfig.h"
#include "DeviceConfig.h"

#ifndef CACHINGFRAMDATASTORE_H
#define CACHINGFRAMDATASTORE_H

class CachingFramDatastore : public FramData {
  public:
    CachingFramDatastore (const uint8_t* _key, const uint8_t* _volatileKey, bool _lazyLoadCache = false) : FramData (_key, _volatileKey) {lazyLoadCache = _lazyLoadCache;}
    CachingFramDatastore (const char* _passphrase, uint8_t _length, bool _lazyLoadCache = false) : FramData (_passphrase, _length) {lazyLoadCache = _lazyLoadCache;}
    bool init ();

    uint8_t getNumUsedSlots (FramZone zone);
    uint8_t getNextSlot (FramZone zone);
    bool clearZone (FramZone zone);

    bool writeRecord (FramRecord* record, uint8_t slot);
    bool readRecord (FramRecord* record, uint8_t slot);

    uint8_t getRecordNum (FramZone zone, uint8_t* recordKey);

    void readKey(uint8_t* buffer, FramZone zone, uint8_t slot);

    void logCache ();

  protected:
    bool cacheLoaded = false;
    bool lazyLoadCache = false;
    uint8_t slotsUsedCache[FRAM_NUM_ZONES] = {FRAM_NULL,FRAM_NULL,FRAM_NULL,FRAM_NULL};
    uint8_t latestSlotIdCache[FRAM_NUM_ZONES] = {FRAM_NULL,FRAM_NULL,FRAM_NULL,FRAM_NULL};

    uint8_t zoneMap0[FRAM_DEVICE_SLOTS][FRAM_DEVICE_KEYSIZE];
    uint8_t zoneMap1[FRAM_CLUSTER_SLOTS][FRAM_CLUSTER_KEYSIZE];
    uint8_t zoneMap2[FRAM_TRUST_SLOTS][FRAM_TRUST_KEYSIZE];
    uint8_t zoneMap3[FRAM_PACKET_SLOTS][FRAM_PACKET_KEYSIZE];

    uint8_t* zoneMaps[FRAM_NUM_ZONES];

    void updateMap (FramRecord* record, uint8_t slot);
    void loadCacheIfNecessary ();

};
#endif
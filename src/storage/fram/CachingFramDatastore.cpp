#include "api/Common.h"
#include "CachingFramDatastore.h"

bool CachingFramDatastore::init () {
    bool success = FramData::init();

    zoneMaps[0] = &zoneMap0[0][0];
    zoneMaps[1] = &zoneMap1[0][0];
    zoneMaps[2] = &zoneMap2[0][0];
    zoneMaps[3] = &zoneMap3[0][0];

    if (!lazyLoadCache) {
        loadCacheIfNecessary();
    }

    return success;
}

uint8_t CachingFramDatastore::getNumUsedSlots (FramZone zone) {
    loadCacheIfNecessary();
    if (slotsUsedCache[zone] == FRAM_NULL) {
        slotsUsedCache[zone] = FramData::getNumUsedSlots(zone);
    }

    return slotsUsedCache[zone];
}

uint8_t CachingFramDatastore::getLatestSlot(FramZone zone) {
    if (latestSlotIdCache[zone] == FRAM_NULL) {
        latestSlotIdCache[zone] = fram.read(zoneLocations[zone] + 1);
    }
    return latestSlotIdCache[zone];
}

uint8_t CachingFramDatastore::getNextSlot (FramZone zone) {
    loadCacheIfNecessary();
    getLatestSlot(zone);
    if (latestSlotIdCache[zone] >= zoneSlots[zone] - 1) {
        return 0;
    }
    return latestSlotIdCache[zone] + 1; // slot after newest will be next
}

bool CachingFramDatastore::clearZone (FramZone zone) {
    FramData::clearZone(zone);
    slotsUsedCache[zone] = 0;
    latestSlotIdCache[zone] = zoneSlots[zone] - 1;
}

void CachingFramDatastore::readKey(uint8_t* buffer, FramZone zone, uint8_t slot) {
    loadCacheIfNecessary();
    uint8_t* map = zoneMaps[zone];
    memcpy(buffer, map + (slot * zoneKeySizes[zone]), zoneKeySizes[zone]);
}

uint8_t CachingFramDatastore::getRecordNum (FramZone zone, uint8_t* recordKey) {
    // cache lazyload already happens in next line
    uint8_t usedSlots = getNumUsedSlots(zone);
    uint8_t zid = zoneId[zone];
    uint8_t latest = getLatestSlot(zone);

    // if the slots are empty, this record does not exist
    if (usedSlots == 0) {
        return FRAM_NULL;
    }

    // this search happens in reverse, starting with the newest added
    // item. The assumption is that newer records are likely 'hotter'
    // than older, so we want to increase odds of doing very few iterations.
    // If this assumption isn't true for the use case, might want to override

    // search backwards, starting at the lateset slot
    uint8_t searchPos = latest;
    uint8_t recordsSearched = 0;
    while (recordsSearched < usedSlots) {
        // look at the key in current position
        readKey(keyBuffer, zone, searchPos);

        // memcmp, if they match return this slot
        if (memcmp(keyBuffer, recordKey, zoneKeySizes[zid]) == 0) {
            return searchPos;
        }

        // if we just saw record zero, need to reacharound
        searchPos--;
        if (searchPos < 0) {
            searchPos = zoneSlots[zid] - 1;
        }

        recordsSearched++;
    }

    return FRAM_NULL;
}

void CachingFramDatastore::updateMap (FramRecord* record, uint8_t slot) {
    record->serializeKey(keyBuffer);

    uint8_t* map = zoneMaps[record->getZone()];
    memcpy(map + (slot * zoneKeySizes[record->getZone()]), keyBuffer, zoneKeySizes[record->getZone()]);
}

void CachingFramDatastore::loadCacheIfNecessary () {
    if (!cacheLoaded) {
        for (uint8_t z = 0; z < FRAM_NUM_ZONES; z++) {
            uint8_t usedSlots = FramData::getNumUsedSlots(zoneId[z]);//getNumUsedSlots(zoneId[z]);
            for (uint8_t s = 0; s < usedSlots; s++) {
                if (zoneId[z] == ZoneDevice) {
                    DeviceConfig device;
                    readRecord(&device, s);
                    updateMap(&device, s);
                }
                else if (zoneId[z] == ZoneCluster) {
                    ClusterConfig cluster;
                    readRecord(&cluster, s);
                    updateMap(&cluster, s);
                }
                else if (zoneId[z] == ZoneTrust) {
                    TrustConfig trust;
                    readRecord(&trust, s);
                    updateMap(&trust, s);
                }
                else if (zoneId[z] == ZonePacket) {
                    PacketData packet;
                    readRecord(&packet, s);
                    updateMap(&packet, s);
                }
            }
        }
        cacheLoaded = true;
    }
}

void CachingFramDatastore::logCache () {
  for (uint8_t z = 0; z < FRAM_NUM_ZONES; z++) {
    uint8_t usedSlots = getNumUsedSlots(zoneId[z]);
    
    for (uint8_t s = 0; s < usedSlots; s++) {
      readKey(keyBuffer, zoneId[z], s);
      for (uint8_t i = 0; i < zoneKeySizes[z]; i++) {
        Serial.print((char)keyBuffer[i]);
      }
      Serial.println("");
    }
    
  }
}

bool CachingFramDatastore::writeRecord (FramRecord* record, uint8_t slot) {
  bool success = FramData::writeRecord(record, slot);
  if (success) {
    // update the key cache at that slot
    updateMap(record, slot);

    // bump the slot used/id cache as necessary
    if (slotsUsedCache[record->getZone()] < zoneSlots[record->getZone()]) {
      slotsUsedCache[record->getZone()] = slotsUsedCache[record->getZone()] + 1;
    }
    latestSlotIdCache[record->getZone()] = slot;
  }
  return success;
}

bool CachingFramDatastore::readRecord (FramRecord* record, uint8_t slot) {
  return FramData::readRecord(record, slot);
}

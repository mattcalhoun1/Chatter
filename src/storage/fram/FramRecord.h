#ifndef FRAMRECORD_H
#define FRAMRECORD_H
#include "Globals.h"

class FramRecord {
  public:
    virtual FramZone getZone () = 0;
    virtual void deserialize (const uint8_t* recordKey, const uint8_t* dataBuffer) = 0;
    virtual void serialize (uint8_t* dataBuffer) = 0;
    virtual void serializeKey (uint8_t* keyBuffer) = 0;
};

#endif
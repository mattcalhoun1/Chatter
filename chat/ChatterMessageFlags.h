#include <Arduino.h>

#ifndef CHATTERMSGFLAGS_H
#define CHATTERMSGFLAGS_H

// these are sent as part of the message header, unencrypted, but part of sig
// DO NOT set these to 0 or 255 or they will interfere with header processing.
// 1 to 254 is OK
struct ChatterMessageFlags {
    uint8_t Flag0 = 1;
    uint8_t Flag1 = 1;
    uint8_t Flag2 = 1;
    uint8_t Flag3 = 1;
    uint8_t Flag4 = 1;
    uint8_t Flag5 = 1;
};

#endif
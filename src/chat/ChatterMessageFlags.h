#include <Arduino.h>

#ifndef CHATTERMSGFLAGS_H
#define CHATTERMSGFLAGS_H

#define FLAG_NOT_SET 1

#define FLAG_CTRL_TYPE_ID 2
#define FLAG_CTRL_TYPE_EXCHANGE_ID 3

// these are sent as part of the message header, unencrypted, but part of sig
// DO NOT set these to 0 or 255 or they will interfere with header processing.
// 1 to 254 is OK
struct ChatterMessageFlags {
    uint8_t Flag0 = FLAG_NOT_SET;
    uint8_t Flag1 = FLAG_NOT_SET;
    uint8_t Flag2 = FLAG_NOT_SET;
    uint8_t Flag3 = FLAG_NOT_SET;
    uint8_t Flag4 = FLAG_NOT_SET; 
    uint8_t Flag5 = FLAG_NOT_SET;
};

#endif
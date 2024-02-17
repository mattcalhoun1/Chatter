#include "ArdAteccSecrets.h"

bool ArdAteccSecrets::init () {
    return true;
}

bool ArdAteccSecrets::readSlot(uint8_t slot, byte* dataBuffer, uint8_t dataLength) {
    return ECCX08.readSlot(slot, dataBuffer, dataLength) == 1;
}

bool ArdAteccSecrets::writeSlot(uint8_t slot, byte* dataBuffer, uint8_t dataLength) {
    return ECCX08.writeSlot(slot, dataBuffer, dataLength) == 1;
}
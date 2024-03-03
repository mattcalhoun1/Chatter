#include <Arduino.h>
#include "StorageGlobals.h"
#include "StorageBase.h"

#ifndef LICENSESTORE_H
#define LICENSESTORE_H

enum LicenseStatus {
    LicenseActive = (uint8_t)'A',
    LicenseDeleted = (uint8_t)'D',
    LicenseUnknown = (uint8_t)'U'
};

class LicenseStore : public StorageBase {
    public:
        virtual bool init () = 0;

        virtual bool loadSignerId (const char* clusterId, char* buffer) = 0;
        virtual bool loadLicense (const char* clusterId, uint8_t* buffer) = 0;
        virtual LicenseStatus getStatus (const char* clusterId) = 0;

        virtual bool deleteLicense (const char* clusterId) = 0;
        virtual bool addLicense (const char* clusterId, const char* signer, uint8_t* license) = 0;

};

#endif
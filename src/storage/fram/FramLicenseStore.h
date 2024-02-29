#include <Arduino.h>
#include "../../chat/ChatterPacket.h"
#include "../StorageGlobals.h"
#include "../LicenseStore.h"
#include "FramData.h"
#include "FramGlobals.h"
#include "ClusterLicense.h"

#ifndef FRAMLICENSESTORE_H
#define FRAMLICENSESTORE_H

class FramLicenseStore : public LicenseStore {
    public:
        FramLicenseStore(FramData* _datastore) { datastore = _datastore; }
        bool init ();

        bool loadSignerId (const char* clusterId, char* buffer);
        bool loadLicense (const char* clusterId, uint8_t* buffer);
        LicenseStatus getStatus (const char* clusterId);

        bool deleteLicense (const char* clusterId);
        bool addLicense (const char* clusterId, const char* signer, uint8_t* license);

    protected:
        void populateKeyBuffer (const char* clusterId);
        bool loadLicenseBuffer (const char* clusterId);
        FramData* datastore;
        uint8_t keyBuffer[FRAM_LICENSE_KEYSIZE];
        ClusterLicense licenseBuffer;
        bool licenseLoaded = false; // just indicates whether license buffer has been populated with ANYthing

};

#endif
#include "FramLicenseStore.h"

bool FramLicenseStore::init () {
    return true;
}

bool FramLicenseStore::loadLicenseBuffer (const char* clusterId) {
    // if this is already loaded, dont do anything
    if (licenseLoaded == false || memcmp(clusterId, licenseBuffer.getClusterId(), STORAGE_GLOBAL_NET_ID_SIZE + STORAGE_LOCAL_NET_ID_SIZE) != 0) {
        populateKeyBuffer(clusterId);
        uint8_t slotNum = datastore->getRecordNum(ZoneLicense, (uint8_t*)keyBuffer);
        if (slotNum != FRAM_NULL) {
            return datastore->readRecord(&licenseBuffer, slotNum);
        }
        else {
            logConsole("License not found by key");
            return false;
        }
    }

    // license already loaded into buffer
    return true;
}

void FramLicenseStore::populateKeyBuffer (const char* clusterId) {
    memcpy(keyBuffer, clusterId, FRAM_CLUSTER_KEYSIZE - 1);
    keyBuffer[FRAM_CLUSTER_KEYSIZE - 1] = LicenseActive;
}

bool FramLicenseStore::loadSignerId (const char* clusterId, char* buffer) {
    if (loadLicenseBuffer(clusterId)) {
        memcpy(buffer, licenseBuffer.getSignerId(), STORAGE_DEVICE_ID_LENGTH);
        buffer[STORAGE_DEVICE_ID_LENGTH] = 0;
        return true;
    }
    return false;
}

bool FramLicenseStore::loadLicense (const char* clusterId, uint8_t* buffer) {
    if (loadLicenseBuffer(clusterId)) {
        memcpy(buffer, licenseBuffer.getLicense(), ENC_SIG_BUFFER_SIZE);
        return true;
    }
    return false;
}

LicenseStatus FramLicenseStore::getStatus (const char* clusterId) {
    if (loadLicenseBuffer(clusterId)) {
        return licenseBuffer.getStatus();
    }
    return LicenseUnknown;
}

bool FramLicenseStore::deleteLicense (const char* clusterId) {
    if (loadLicenseBuffer(clusterId)) {
        licenseBuffer.setStatus(LicenseDeleted);

        populateKeyBuffer(clusterId);

        uint8_t slotNum = datastore->getRecordNum(ZoneLicense, (uint8_t*)keyBuffer);
        if (slotNum != FRAM_NULL) {
            bool result = datastore->writeRecord(&licenseBuffer, slotNum);
            if (result) {
                logConsole("Deleted license");
            }
            else {
                logConsole("Failed to delete license");
            }
            return result;
        }
        return false;
    }
    return true; // if not found, its already deleted
}

bool FramLicenseStore::addLicense (const char* clusterId, const char* signer, uint8_t* license) {
    populateKeyBuffer(clusterId);

    if (datastore->recordExists(ZoneLicense, (uint8_t*)keyBuffer)) {
        logConsole("license already exists, deleting existing");
        if (deleteLicense(clusterId)) {
            logConsole("existing license deleted, continuing");
        }
        else {
            logConsole("license delete failed.");
            return false;
        }
    }

    licenseBuffer.setClusterId(clusterId);
    licenseBuffer.setStatus(LicenseActive);
    licenseBuffer.setSignerId(signer);
    licenseBuffer.setLicense(license);

    if (datastore->writeToNextSlot(&licenseBuffer)) {
        logConsole("License added");
        return true;
    }

    logConsole("License add failed");
    return false;

}

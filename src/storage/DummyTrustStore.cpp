#include "DummyTrustStore.h"

bool DummyTrustStore::init () {
    logConsole("WARNING: Dummy packet store running!");
    return true;
}

List<String> DummyTrustStore::getDeviceIds() {
    List<String> dummyList;
    dummyList.add(String("CAL000"));
    dummyList.add(String("CAL001"));
    return dummyList;
}
bool DummyTrustStore::loadPublicKey(const char* deviceId, char* keyBuffer) {
    return true;
}

bool DummyTrustStore::loadAlias(const char* deviceId, char* aliasBuffer) {
    sprintf(aliasBuffer, "DummyDevice", 12);
    return true;
}

bool DummyTrustStore::addTrustedDevice (const char* deviceId, const char* alias, const char* publicKey) {
    return true;
}

bool DummyTrustStore::addTrustedDevice (const char* deviceId, const char* alias, const char* key, bool overwrite) {
    return true;
}

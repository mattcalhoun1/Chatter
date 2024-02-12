#include "ChatterAdmin.h"

bool ChatterAdmin::handleAdminRequest () {
    const char* nextLine = Serial.readStringUntil('\n').c_str();

    AdminRequestType requestType = extractRequestType(nextLine);
    ChatterDeviceType deviceType = extractDeviceType(nextLine);

    Serial.println("Admin request type: " + String(requestType) + " from device type: " + String(deviceType));

    if (requestType == AdminRequestOnboard || requestType == AdminRequestSync) {
        // pub key is required
        if (ingestPublicKey(chatter->getEncryptor()->getPublicKeyBuffer())) {
            Serial.println("Public key ingested");

            // check if this device id is known
            char knownDeviceId[CHATTER_DEVICE_ID_SIZE+1];
            if(chatter->getTrustStore()->findDeviceId ((char*)chatter->getEncryptor()->getPublicKeyBuffer(), knownDeviceId)) {
                Serial.print("We Know: ");
                Serial.println(knownDeviceId);
            } 
            else {
                Serial.print("We dont know that key");
            }

            // if it's onboard, it should not yet exist in our truststore. if that's the case, do a sync instead

            // if it's sync, it should exist in our truststore

            // make sure it exists in our truststore if its a sync request

            // should we check a sig?
        }
        else {
            Serial.println("Bad public key!");
        }
    }

    return true;
}

bool ChatterAdmin::ingestPublicKey (byte* buffer) {
    int bytesRead = 0;
    bool validKey = true; // assume valid until we find a bad byte
    unsigned long startTime = millis();
    unsigned long maxWait = 3000;//3 sec

    // pub key can only be hex with no spaces
    while ((millis() - startTime < maxWait) && bytesRead < ENC_PUB_KEY_SIZE && validKey) {
        if (Serial.available()) {
            // ignore carriage returns, as this may be preceeded by one
            if (Serial.peek() == '\n' || Serial.peek() == CHATTER_ADMIN_DELIMITER || Serial.peek() == '\0' || Serial.peek() == '\r') {
                Serial.read();
            }
            else {
                buffer[bytesRead++] = (byte)Serial.read();

                if ((buffer[bytesRead-1] >= '0' && buffer[bytesRead-1] <= '9') || (buffer[bytesRead-1] >= 'A' && buffer[bytesRead-1] <= 'F')) {
                    // we have a valid key byte
                }
                else {
                    validKey = false;
                }
            }
        }
    }

    if (bytesRead == ENC_PUB_KEY_SIZE && validKey) {
        return true;
    }

    return false;
}

AdminRequestType ChatterAdmin::extractRequestType (const char* request) {
    if (memcmp(request, CHATTER_ADMIN_REQ_SYNC, 4) == 0) {
        return AdminRequestSync;
    }
    else if (memcmp(request, CHATTER_ADMIN_REQ_ONBOARD, 4) == 0) {
        return AdminRequestOnboard;
    }
    else if (memcmp(request, CHATTER_ADMIN_REQ_GENESIS, 4) == 0) {
        return AdminRequestGenesis;
    }
    return AdminRequestNone;
}

ChatterDeviceType ChatterAdmin::extractDeviceType(const char* request) {
    // the message needs to be at least 4 chars + delimiter + 2 (device type string)
    if (strlen(request) >= 7) {
        if (memcmp(request + 5, DEVICE_TYPE_BRIDGE_LORA, 2) == 0) {
            return ChatterDeviceBridgeLora;
        }
        else if (memcmp(request + 5, DEVICE_TYPE_BRIDGE_WIFI, 2) == 0) {
            return ChatterDeviceBridgeWifi;
        }
        else if (memcmp(request + 5, DEVICE_TYPE_BRIDGE_CLOUD, 2) == 0) {
            return ChatterDeviceBridgeCloud;
        }
        else if (memcmp(request + 5, DEVICE_TYPE_COMMUNICATOR, 2) == 0) {
            return ChatterDeviceCommunicator;
        }
        else if (memcmp(request + 5, DEVICE_TYPE_RAW, 2) == 0) {
            return ChatterDeviceRaw;
        }
    }
    return ChatterDeviceUnknown;
}

bool ChatterAdmin::genesis () {
    // global network id = US for now
    // Generate a random local network id, ascii 65-90 inclusive
    char newNetworkId[9];
    newNetworkId[0] = 'U';
    newNetworkId[1] = 'S';

    Encryptor* encryptor = chatter->getEncryptor();
    for (int i = 0; i < 3; i++) {
        int nextDigit = (encryptor->getRandom() % 26) + 65;
        newNetworkId[i+2] = (char)nextDigit;
    }
    

    // device id is network info + 000 for this genesis device
    memset(newNetworkId+5, '0', 3);
    newNetworkId[8] = '\0';

    // generate a new key
    // 0 - 9, A - F
    // ascii 0 == 48
    // ascii A == 65
    char newKey[33];
    newKey[32] = '\0';
    for (int i = 0; i < 32; i++) {
        int nextDigit = (encryptor->getRandom() % 16);
        if (nextDigit < 6) {
            // char A-F
            newKey[i] = (char)(nextDigit + 65);
        }
        else {
            // number 0-9
            newKey[i] = (char)(nextDigit - 6 + 48);
        }
    }

    // save it all in the atecc
    Serial.print("New device ID: ");
    Serial.print(newNetworkId);
    Serial.println("");
    Serial.print("New key: ");
    Serial.print(newKey);
    Serial.println("");


    // later, get this id/pw from user, not hardcoded like this
    const char* wifiStuff = "chatter.wifi|m@ttc@lhoun$";
    encryptor->setTextSlotBuffer(wifiStuff);
    if(encryptor->saveDataSlot(WIFI_SSID_SLOT)) {
        Serial.println("WiFi Config Saved");
    }
    else {
        Serial.println("WiFi Config Failed!");
    }

    // clear out existing truststore


    return true;
}

bool ChatterAdmin::syncDevice () {
    dumpTruststore();
    dumpSymmetricKey();
    dumpWiFi();
    dumpTime();
}

bool ChatterAdmin::dumpTruststore () {
    TrustStore* trustStore = chatter->getTrustStore();
    Encryptor* encryptor = chatter->getEncryptor();

    List<String> others = trustStore->getDeviceIds();
    char otherDeviceAlias[12];
    Serial.println("=== BEGIN TRUSTSTORE ===");
    for (int i = 0; i < others.getSize(); i++) {
        const String& otherDeviceStr = others[i];
        // try loading public key for that
        const char* otherDeviceId = otherDeviceStr.c_str();
        trustStore->loadAlias(otherDeviceId, otherDeviceAlias);
        trustStore->loadPublicKey(otherDeviceId, (char*)encryptor->getPublicKeyBuffer());

        Serial.println(otherDeviceId);
        Serial.println((char*)encryptor->getPublicKeyBuffer());
        Serial.println(otherDeviceAlias);
    }
    Serial.println("=== END TRUSTSTORE ===");
    return true;
}

bool ChatterAdmin::dumpSymmetricKey() {
    Serial.println("=== KEY ===");
    Encryptor* encryptor = chatter->getEncryptor();
    if (encryptor->loadDataSlot(ENCRYPTION_KEY_SLOT)) {
        encryptor->logBufferHex(encryptor->getDataSlotBuffer(), ENC_DATA_SLOT_SIZE);
    }
    Serial.println("");
    Serial.println("=== END KEY ===");

}

bool ChatterAdmin::dumpWiFi () {
    Serial.println("=== WIFI ===");
    Encryptor* encryptor = chatter->getEncryptor();
    encryptor->loadDataSlot(WIFI_SSID_SLOT);
    char ssid[32];
    encryptor->getTextSlotBuffer(ssid);
    Serial.print(ssid);
    Serial.println("");
    Serial.println("=== END WIFI ===");
}

bool ChatterAdmin::dumpTime () {
    Serial.println("=== TIME ===");
    Serial.print(chatter->getRtc()->getSortableTime());
    Serial.println("");
    Serial.println("=== END TIME ===");
}

bool ChatterAdmin::onboardNewDevice () {
    // ask device what it is

    // bridge gets one flow, other gets another

    return true;
}

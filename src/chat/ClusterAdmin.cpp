#include "ClusterAdmin.h"

bool ClusterAdmin::handleAdminRequest () {
    const char* nextLine = Serial.readStringUntil('\n').c_str();

    AdminRequestType requestType = extractRequestType(nextLine);
    ChatterDeviceType deviceType = extractDeviceType(nextLine);

    //Serial.println("Admin request type: " + String(requestType) + " from device type: " + String(deviceType));

    if (requestType == AdminRequestOnboard || requestType == AdminRequestSync) {
        // pub key is required
        if (ingestPublicKey(chatter->getEncryptor()->getPublicKeyBuffer())) {

            // future enhancement: this would be a good point to generate a message to challenge for a signature. 

            // check if this device id is known
            char knownDeviceId[CHATTER_DEVICE_ID_SIZE+1];
            char knownAlias[CHATTER_ALIAS_NAME_SIZE+1];
            bool trustedKey = chatter->getTrustStore()->findDeviceId ((char*)chatter->getEncryptor()->getPublicKeyBuffer(), knownDeviceId);
            if(trustedKey) {
                chatter->getTrustStore()->loadAlias(knownDeviceId, knownAlias);
                /*Serial.print("We Know: ");
                Serial.print(knownDeviceId);
                Serial.print("/");
                Serial.println(knownAlias);*/

                // if it's onboard, it should not yet exist in our truststore. if that's the case, do a sync instead
                // if it's sync, we are good to proceed
                return syncDevice(knownDeviceId, knownAlias);
            } 
            else {
                // we don't know this key, which is to be expected if it's on onboard
                if (requestType == AdminRequestOnboard) {
                    return onboardNewDevice(deviceType, (const char*)chatter->getEncryptor()->getPublicKeyBuffer());
                }
            }
        }
        else {
            Serial.println("Bad public key!");
        }
    }
    else if (requestType == AdminRequestGenesis && deviceType == ChatterDeviceRaw) {
        // this needs to be done by shorting pin A0
        return false;
    }

    return true;
}

AdminRequestType ClusterAdmin::extractRequestType (const char* request) {
    if (memcmp(request, CLUSTER_REQ_SYNC, 4) == 0) {
        return AdminRequestSync;
    }
    else if (memcmp(request, CLUSTER_REQ_ONBOARD, 4) == 0) {
        return AdminRequestOnboard;
    }
    else if (memcmp(request, CLUSTER_REQ_GENESIS, 4) == 0) {
        return AdminRequestGenesis;
    }
    return AdminRequestNone;
}

ChatterDeviceType ClusterAdmin::extractDeviceType(const char* request) {
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

bool ClusterAdmin::genesis () {
    Serial.println("Creating a new cluster. Proceed with caution.");
    char newNetworkId[9];
    while (!getUserInput ("New Cluster ID, 5 upper-case letters (Ex: USCAL):", newNetworkId, 5, 5, false, false) ) {
        delay(10);
    }

    // device id is network info + 000 for this genesis device
    memset(newNetworkId+5, '0', 3);
    newNetworkId[8] = '\0';

    // save it all in the atecc
    Serial.print("New device ID: ");
    Serial.print(newNetworkId);
    Serial.println("");

    // device id setup
    Encryptor* encryptor = chatter->getEncryptor();
    encryptor->setTextSlotBuffer(newNetworkId);
    encryptor->saveDataSlot(DEVICE_ID_SLOT);

    // generate a new key
    // 0 - 9, A - F
    // ascii 0 == 48
    // ascii A == 65
    char newKey[33];
    newKey[32] = '\0';
    // this loop assumes the key slot and iv slot are adjacent
    for (int keySlot = ENCRYPTION_KEY_SLOT; keySlot <= ENCRYPTION_IV_SLOT; keySlot++) {
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

        Serial.print("New key: ");
        Serial.print(newKey);
        Serial.println("");

        // update the symmetric key
        encryptor->setDataSlotBuffer(newKey);
        if (encryptor->saveDataSlot(keySlot)) {
            Serial.println("Symmetric Key Updated");
        }

    }

    char loraFrequency[6];
    memset(loraFrequency, 0, 6);

    char* loraFrequencyEnd = loraFrequency + 5;
    while (strtof(loraFrequency, &loraFrequencyEnd) < 868.0 || strtof(loraFrequency, &loraFrequencyEnd) > 960.0) {
        memset(loraFrequency, 0, 6);
        while (!getUserInput ("LoRa Frequency, exactly 5 digits (ex: 915.0):", loraFrequency, 5, 5, true, true) ) {
            delay(10);
        }
    }
    encryptor->setTextSlotBuffer(loraFrequency);
    if(encryptor->saveDataSlot(LORA_FREQUENCY_SLOT)) {
        Serial.println("Lora Frequency Saved");
    }
    else {
        Serial.println("Lora Frequency Failed!");
    }

    char ssid[WIFI_SSID_MAX_LEN];
    char pw[WIFI_PASSWORD_MAX_LEN];
    memset(ssid, 0, WIFI_SSID_MAX_LEN);
    memset(pw, 0, WIFI_PASSWORD_MAX_LEN);

    while (!getUserInput ("WiFi Network SSID:", ssid, 5, WIFI_SSID_MAX_LEN - 1, true, true) ) {
        delay(10);
    }

    while (!getUserInput ("WiFi Network Password:", pw, 5, WIFI_PASSWORD_MAX_LEN - 1, true, true) ) {
        delay(10);
    }
    
    String newWifiCreds = String(ssid) + String(ENCRYPTION_CRED_DELIMITER) + String(pw);
    encryptor->setTextSlotBuffer(newWifiCreds.c_str());
    if(encryptor->saveDataSlot(WIFI_SSID_SLOT)) {
        Serial.println("WiFi Config Saved: " + newWifiCreds);
    }
    else {
        Serial.println("WiFi Config Failed!");
    }

    char newDate[13]; //yymmddhhmmss
    while (!getUserInput ("Time (yymmddhhmmss, ex: 241231135959):", newDate, 12, 12, false, false) ) {
        delay(10);
    }
    chatter->getRtc()->setNewDateTime(newDate);
    
    Serial.println("New Date: " + String(chatter->getRtc()->getSortableTime()));

    // clear out existing truststore, and add self with new id
    chatter->getTrustStore()->clearTruststore();
    encryptor->loadPublicKey(CHATTER_SIGN_PK_SLOT);
    encryptor->hexify(encryptor->getPublicKeyBuffer(), ENC_PUB_KEY_SIZE);
    chatter->getTrustStore()->addTrustedDevice(newNetworkId, BASE_LORA_ALIAS, (const char*)encryptor->getHexBuffer(), true);

    Serial.println("Genesis Complete! Ready to onboard devices.");

    return true;
}

bool ClusterAdmin::syncDevice (const char* deviceId, const char* alias) {
    dumpDevice(deviceId, alias);
    dumpTruststore();
    dumpSymmetricKey();
    dumpWiFi();
    dumpTime();
    dumpFrequency();
}

bool ClusterAdmin::dumpTruststore () {
    TrustStore* trustStore = chatter->getTrustStore();
    Encryptor* encryptor = chatter->getEncryptor();

    List<String> others = trustStore->getDeviceIds();
    char otherDeviceAlias[CHATTER_ALIAS_NAME_SIZE + 1];
    for (int i = 0; i < others.getSize(); i++) {
        const String& otherDeviceStr = others[i];
        // try loading public key for that
        const char* otherDeviceId = otherDeviceStr.c_str();
        trustStore->loadAlias(otherDeviceId, otherDeviceAlias);
        trustStore->loadPublicKey(otherDeviceId, (char*)encryptor->getPublicKeyBuffer());

        Serial.print(CLUSTER_CFG_TRUST);
        Serial.print(CLUSTER_CFG_DELIMITER);
        Serial.print(otherDeviceId);
        Serial.print((char*)encryptor->getPublicKeyBuffer());
        Serial.print(otherDeviceAlias);
        Serial.println("");
    }
    return true;
}

bool ClusterAdmin::dumpSymmetricKey() {
    Serial.print(CLUSTER_CFG_KEY);
    Serial.print(CLUSTER_CFG_DELIMITER);
    Encryptor* encryptor = chatter->getEncryptor();
    if (encryptor->loadDataSlot(ENCRYPTION_KEY_SLOT)) {
        encryptor->logBufferHex(encryptor->getDataSlotBuffer(), ENC_DATA_SLOT_SIZE);
    }
    Serial.println("");

    Serial.print(CLUSTER_CFG_IV);
    Serial.print(CLUSTER_CFG_DELIMITER);
    if (encryptor->loadDataSlot(ENCRYPTION_IV_SLOT)) {
        encryptor->logBufferHex(encryptor->getDataSlotBuffer(), ENC_DATA_SLOT_SIZE);
    }
    Serial.println("");
}

bool ClusterAdmin::dumpWiFi () {
    Encryptor* encryptor = chatter->getEncryptor();
    encryptor->loadDataSlot(WIFI_SSID_SLOT);
    char ssid[32];
    encryptor->getTextSlotBuffer(ssid);
    Serial.print(CLUSTER_CFG_WIFI);
    Serial.print(CLUSTER_CFG_DELIMITER);
    Serial.print(ssid);
    Serial.println("");
}

bool ClusterAdmin::dumpFrequency () {
    Encryptor* encryptor = chatter->getEncryptor();
    encryptor->loadDataSlot(LORA_FREQUENCY_SLOT);
    char frequency[32];
    encryptor->getTextSlotBuffer(frequency);
    frequency[5] = '\0';
    Serial.print(CLUSTER_CFG_FREQ);
    Serial.print(CLUSTER_CFG_DELIMITER);
    Serial.println(frequency);
}

bool ClusterAdmin::dumpTime () {
    Serial.print(CLUSTER_CFG_TIME);
    Serial.print(CLUSTER_CFG_DELIMITER);
    Serial.print(chatter->getRtc()->getSortableTime());
    Serial.println("");
}

bool ClusterAdmin::dumpDevice (const char* deviceId, const char* alias) {
    Serial.print(CLUSTER_CFG_DEVICE);
    Serial.print(CLUSTER_CFG_DELIMITER);
    Serial.print(deviceId);
    Serial.print(alias);
    Serial.println("");
}


bool ClusterAdmin::onboardNewDevice (ChatterDeviceType deviceType, const char* devicePublicKey) {
    Serial.println("We will onboard this device");

    char newAddress[CHATTER_DEVICE_ID_SIZE + 1];
    memcpy(newAddress, chatter->getDeviceId(), CHATTER_DEVICE_ID_SIZE - 3);
    newAddress[CHATTER_DEVICE_ID_SIZE] = '\0';
    char alias[CHATTER_ALIAS_NAME_SIZE + 1];
    memset(alias, 0, CHATTER_ALIAS_NAME_SIZE + 1);

    switch (deviceType) {
        case ChatterDeviceBridgeWifi:
            memcpy(newAddress + (CHATTER_DEVICE_ID_SIZE - 3), BASE_WIFI_ADDRESS, 3);
            memcpy(alias, BASE_WIFI_ADDRESS, strlen(BASE_WIFI_ADDRESS));
            break;
        case ChatterDeviceBridgeCloud:
            memcpy(newAddress + (CHATTER_DEVICE_ID_SIZE - 3), BASE_CLOUD_ADDRESS, 3);
            memcpy(alias, BASE_CLOUD_ALIAS, strlen(BASE_CLOUD_ALIAS));
            break;
        case ChatterDeviceCommunicator:
            // find the next available communicator address
            if (chatter->getTrustStore()->findNextAvailableDeviceId (newAddress, STARTING_DEVICE_ADDRESS, newAddress+(CHATTER_DEVICE_ID_SIZE - 3))) {
                memcpy(alias, "Com_", 4);
                memcpy(alias+4, newAddress+(CHATTER_DEVICE_ID_SIZE - 3), 3);
            }
            else {
                Serial.println("Cluster is full.");
                return false;
            }
            break;
        default:
            Serial.println("ERROR! Onboarding not supported for this device.");
            return false;
    }

    Serial.println("This will be: " + String(newAddress));
    Serial.println("Alias: " + String(alias));

    if(chatter->getTrustStore()->addTrustedDevice(newAddress, alias, devicePublicKey, true)) {
        syncDevice (newAddress, alias);
    }


}

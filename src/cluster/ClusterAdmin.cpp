#include "ClusterAdmin.h"

bool ClusterAdmin::handleAdminRequest () {

    while (Serial.available() > 0 && (Serial.peek() == '\n' || Serial.peek() == '\r' || Serial.peek() == '\0')) {
        Serial.print("skipping: ");Serial.print((char)Serial.read());
    }

    if (Serial.available() > 0) {
        String sNextLine = Serial.readStringUntil('\n');
        const char* nextLine = sNextLine.c_str(); 

        AdminRequestType requestType = extractRequestType(nextLine);
        ChatterDeviceType deviceType = extractDeviceType(nextLine);

        Serial.println("Admin request type: " + String(requestType) + " from device type: " + String(deviceType) + " Received " + sNextLine);

        if (requestType == AdminRequestOnboard || requestType == AdminRequestSync) {
            // pub key is required
            if (ingestPublicKey(chatter->getEncryptor()->getPublicKeyBuffer())) {

                // future enhancement: this would be a good point to generate a message to challenge for a signature. 

                // check if this device id is known
                char knownDeviceId[CHATTER_DEVICE_ID_SIZE+1];
                char knownAlias[CHATTER_ALIAS_NAME_SIZE+1];
                bool trustedKey = chatter->getTrustStore()->findDeviceId ((const uint8_t*)chatter->getEncryptor()->getPublicKeyBuffer(), chatter->getClusterId(), knownDeviceId);
                if(trustedKey) {
                    chatter->getTrustStore()->loadAlias(knownDeviceId, knownAlias);
                    /*Serial.print("We Know: ");
                    Serial.print(knownDeviceId);
                    Serial.print("/");
                    Serial.println(knownAlias);*/

                    // if it's onboard, it should not yet exist in our truststore. if that's the case, do a sync instead
                    // if it's sync, we are good to proceed
                    return syncDevice(chatter->getClusterId(), knownDeviceId, knownAlias);
                } 
                else {
                    // we don't know this key, which is to be expected if it's on onboard
                    if (requestType == AdminRequestOnboard) {
                        return onboardNewDevice(chatter->getClusterId(), deviceType, (const uint8_t*)chatter->getEncryptor()->getPublicKeyBuffer());
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
    }
    else {
        // it was just control characters
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
    uint8_t clusterIdLength = CHATTER_LOCAL_NET_ID_SIZE + CHATTER_GLOBAL_NET_ID_SIZE;
    while (!getUserInput ("New Cluster ID, 5 upper-case letters (Ex: USCAL):", clusterId, clusterIdLength, clusterIdLength, false, false) ) {
        delay(10);
    }

    // device id is network info + 000 for this genesis device
    memcpy(newDeviceId, clusterId, clusterIdLength);
    memset(newDeviceId+clusterIdLength, '0', 3);
    newDeviceId[CHATTER_DEVICE_ID_SIZE] = '\0';

    Serial.print("New device ID: ");
    Serial.print(newDeviceId);
    Serial.println("");

    memcpy(clusterId, newDeviceId, STORAGE_GLOBAL_NET_ID_SIZE + STORAGE_LOCAL_NET_ID_SIZE);
    clusterId[STORAGE_GLOBAL_NET_ID_SIZE + STORAGE_LOCAL_NET_ID_SIZE] = 0;

    // device id setup
    Encryptor* encryptor = chatter->getEncryptor();

    // generate a new key/iv
    chatter->getHsm()->generateSymmetricKey(symmetricKey, ENC_SYMMETRIC_KEY_SIZE);
    chatter->getHsm()->generateSymmetricKey(iv, ENC_IV_SIZE);

    char loraFrequency[6];
    memset(loraFrequency, 0, 6);

    char* loraFrequencyEnd = loraFrequency + 5;
    while (strtof(loraFrequency, &loraFrequencyEnd) < 868.0 || strtof(loraFrequency, &loraFrequencyEnd) > 960.0) {
        memset(loraFrequency, 0, 6);
        while (!getUserInput ("LoRa Frequency, exactly 5 digits (ex: 915.0):", loraFrequency, 5, 5, true, true) ) {
            delay(10);
        }
    }
    frequency = atof(loraFrequency);

    memset(wifiSsid, 0, CHATTER_WIFI_STRING_MAX_SIZE);
    memset(wifiCred, 0, CHATTER_WIFI_STRING_MAX_SIZE);

    while (!getUserInput ("WiFi Network SSID:", wifiSsid, 5, CHATTER_WIFI_STRING_MAX_SIZE - 1, true, true) ) {
        delay(10);
    }

    while (!getUserInput ("WiFi Network Password:", wifiCred, 5, CHATTER_WIFI_STRING_MAX_SIZE - 1, true, true) ) {
        delay(10);
    }
    
    char newDate[13]; //yymmddhhmmss
    while (!getUserInput ("Time (yymmddhhmmss, ex: 241231135959):", newDate, 12, 12, false, false) ) {
        delay(10);
    }
    chatter->getRtc()->setNewDateTime(newDate);
    
    Serial.println("New Date: " + String(chatter->getRtc()->getSortableTime()));

    //chatter->getTrustStore()->clearTruststore();
    encryptor->loadPublicKey();

    Serial.println("Adding self to truststore");
    chatter->getHsm()->loadPublicKey(pubKey);
    chatter->getTrustStore()->addTrustedDevice(newDeviceId, BASE_LORA_ALIAS, pubKey, true);
    
    Serial.println("Adding cluster to storage");
    chatter->getClusterStore()->addCluster (clusterId, alias, newDeviceId, symmetricKey, iv, frequency, wifiSsid, wifiCred, ClusterChannelLora, ClusterChannelUdp, ClusterAuthFull, ClusterLicenseRoot);

    Serial.println("Making default cluster");
    chatter->getDeviceStore()->setDefaultClusterId(clusterId);

    Serial.println("Genesis Complete! Ready to onboard devices.");

    return true;
}

bool ClusterAdmin::genesisRandom () {
    Serial.println("Creating a new random cluster.");
    if (chatter == nullptr) {
        Serial.println("Chatter is null!");
    }

    //Encryptor* encryptor = chatter->getEncryptor();
    PseudoHsm hsm = PseudoHsm(chatter->getDeviceStore(), chatter->getClusterStore());
    hsm.factoryReset();

    // global network id = US for now
    // Generate a random local network id, ascii 65-90 inclusive
    newDeviceId[0] = 'U';
    newDeviceId[1] = 'S';

    Serial.println("Generating new cluster id");
    for (int i = 0; i < 3; i++) {
        int nextDigit = (hsm.getRandomLong() % 26) + 65;
        newDeviceId[i+2] = (char)nextDigit;
    }
    // device id is network info + 000 for this genesis device
    memset(newDeviceId+5, '0', 3);
    newDeviceId[8] = '\0';

    // save it all in the atecc
    Serial.print("New device ID: ");
    Serial.println(newDeviceId);

    memcpy(clusterId, newDeviceId, STORAGE_GLOBAL_NET_ID_SIZE + STORAGE_LOCAL_NET_ID_SIZE);
    clusterId[STORAGE_GLOBAL_NET_ID_SIZE + STORAGE_LOCAL_NET_ID_SIZE] = 0;

    // generate a new key
    // generate a new key/iv
    Serial.println("Generating symmetric key");
    hsm.generateSymmetricKey(symmetricKey, ENC_SYMMETRIC_KEY_SIZE);
    hsm.generateSymmetricKey(iv, ENC_IV_SIZE);
    Serial.println("Finished symmetric key");

    frequency = 915.0;

    memset(wifiSsid, 0, CHATTER_WIFI_STRING_MAX_SIZE);
    memset(wifiCred, 0, CHATTER_WIFI_STRING_MAX_SIZE);
    memcpy(wifiSsid, "none", 4);
    memcpy(wifiCred, "none", 4);

    char newDate[13]; //yymmddhhmmss
    // default to 1/1/2024 if clock not set. will have to fix later
    if (strcmp(chatter->getRtc()->getSortableTime(), "240101010101") < 0) {
        chatter->getRtc()->setNewDateTime("240101010101");
        Serial.println("New Date: " + String(chatter->getRtc()->getSortableTime()));
    }
    
    Serial.println("Adding self to truststore");

    // dump pub key into key buffer
    hsm.loadPublicKey(pubKey);
    Serial.print("This Device Pub Key: ");
    for (uint8_t i = 0; i < ENC_PUB_KEY_SIZE; i++) {
        Serial.print(pubKey[i]); Serial.print(" ");
    }
    Serial.println("");

    chatter->getTrustStore()->addTrustedDevice(newDeviceId, BASE_LORA_ALIAS, pubKey, true);
    
    Serial.println("Adding cluster to storage");
    chatter->getClusterStore()->addCluster (clusterId, alias, newDeviceId, symmetricKey, iv, frequency, wifiSsid, wifiCred, ClusterChannelLora, ClusterChannelNone, ClusterAuthFull, ClusterLicenseRoot);

    Serial.println("Making default cluster");
    chatter->getDeviceStore()->setDeviceName(newDeviceId);
    chatter->getDeviceStore()->setDefaultClusterId(clusterId);

    Serial.println("Genesis Complete! Ready to onboard devices.");

    return true;
}

bool ClusterAdmin::syncDevice (const char* hostClusterId, const char* deviceId, const char* alias) {
    dumpDevice(deviceId, alias);
    dumpTruststore(hostClusterId);
    dumpSymmetricKey(hostClusterId);
    dumpWiFi(hostClusterId);
    dumpTime();
    dumpFrequency(hostClusterId);
    dumpChannels(hostClusterId);
    dumpAuthType(hostClusterId);
    dumpLicense(deviceId);
}

bool ClusterAdmin::dumpTruststore (const char* hostClusterId) {
    TrustStore* trustStore = chatter->getTrustStore();
    Encryptor* encryptor = chatter->getEncryptor();

    List<String> others = trustStore->getDeviceIds();
    char otherDeviceAlias[CHATTER_ALIAS_NAME_SIZE + 1];
    for (int i = 0; i < others.getSize(); i++) {
        const String& otherDeviceStr = others[i];
        // if its pub key of device on this cluser, dump it
        const char* otherDeviceId = otherDeviceStr.c_str();
        if (memcmp(otherDeviceId, hostClusterId, CHATTER_GLOBAL_NET_ID_SIZE + CHATTER_LOCAL_NET_ID_SIZE) == 0) {
            trustStore->loadAlias(otherDeviceId, otherDeviceAlias);
            trustStore->loadPublicKey(otherDeviceId, pubKey);

            Serial.print(CLUSTER_CFG_TRUST);
            Serial.print(CLUSTER_CFG_DELIMITER);
            Serial.print(otherDeviceId);

            encryptor->hexify(pubKey, ENC_PUB_KEY_SIZE);
            memset(pubKey, 0, ENC_PUB_KEY_SIZE);
            for (uint8_t i = 0; i < ENC_PUB_KEY_SIZE*2; i++) {
                Serial.print(encryptor->getHexBuffer()[i]);
            }
            encryptor->clearHexBuffer(); // dont leave key in hex buffer

            Serial.print((char*)encryptor->getPublicKeyBuffer());
            Serial.print(otherDeviceAlias);
            Serial.println("");
        }
    }
    return true;
}

bool ClusterAdmin::dumpSymmetricKey(const char* hostClusterId) {
    Serial.print(CLUSTER_CFG_KEY);
    Serial.print(CLUSTER_CFG_DELIMITER);
    chatter->getClusterStore()->loadSymmetricKey(hostClusterId, symmetricKey);

    Encryptor* encryptor = chatter->getEncryptor();
    encryptor->hexify(symmetricKey, ENC_SYMMETRIC_KEY_SIZE);
    memset(symmetricKey, 0, ENC_SYMMETRIC_KEY_SIZE*2);
    for (uint8_t i = 0; i < ENC_SYMMETRIC_KEY_SIZE*2; i++) {
        Serial.print(encryptor->getHexBuffer()[i]);
    }
    encryptor->clearHexBuffer(); // dont leave key in hex buffer
    Serial.println("");

    Serial.print(CLUSTER_CFG_IV);
    Serial.print(CLUSTER_CFG_DELIMITER);
    chatter->getClusterStore()->loadIv(hostClusterId, iv);
    encryptor->hexify(iv, ENC_IV_SIZE);
    memset(iv, 0, ENC_IV_SIZE);

    for (uint8_t i = 0; i < ENC_IV_SIZE*2; i++) {
        Serial.print(encryptor->getHexBuffer()[i]);
    }
    encryptor->clearHexBuffer(); // dont leave key in hex buffer
    Serial.println("");
}

bool ClusterAdmin::dumpWiFi (const char* hostClusterId) {
    // CLUSTER_CFG_WIFI_SSID , CLUSTER_CFG_WIFI_CRED
    chatter->getClusterStore()->loadWifiSsid(hostClusterId, wifiSsid);
    chatter->getClusterStore()->loadWifiCred(hostClusterId, wifiCred);

    Serial.print(CLUSTER_CFG_WIFI_SSID);
    Serial.print(CLUSTER_CFG_DELIMITER);
    Serial.println(wifiSsid);
    Serial.print(CLUSTER_CFG_WIFI_CRED);
    Serial.print(CLUSTER_CFG_DELIMITER);
    Serial.println(wifiCred);
}

bool ClusterAdmin::dumpFrequency (const char* hostClusterId) {
    Serial.print(CLUSTER_CFG_FREQ);
    Serial.print(CLUSTER_CFG_DELIMITER);
    Serial.println(chatter->getClusterStore()->getFrequency(hostClusterId));
}

bool ClusterAdmin::dumpChannels (const char* hostClusterId) {
    Serial.print(CLUSTER_CFG_PRIMARY);
    Serial.print(CLUSTER_CFG_DELIMITER);
    Serial.println((char)chatter->getClusterStore()->getPreferredChannel(hostClusterId));

    Serial.print(CLUSTER_CFG_SECONDARY);
    Serial.print(CLUSTER_CFG_DELIMITER);
    Serial.println((char)chatter->getClusterStore()->getSecondaryChannel(hostClusterId));
}

bool ClusterAdmin::dumpAuthType (const char* hostClusterId) {
    Serial.print(CLUSTER_CFG_AUTH);
    Serial.print(CLUSTER_CFG_DELIMITER);
    Serial.println((char)chatter->getClusterStore()->getAuthType(hostClusterId));
}


bool ClusterAdmin::dumpTime () {
    Serial.print(CLUSTER_CFG_TIME);
    Serial.print(CLUSTER_CFG_DELIMITER);
    Serial.println(chatter->getRtc()->getSortableTime());
}

bool ClusterAdmin::dumpDevice (const char* deviceId, const char* alias) {
    Serial.print(CLUSTER_CFG_DEVICE);
    Serial.print(CLUSTER_CFG_DELIMITER);
    Serial.print(deviceId);
    Serial.println(alias);
}

bool ClusterAdmin::generateEncodedLicense (const char* deviceId) {
    // get the pub key
    if(chatter->getTrustStore()->loadPublicKey(deviceId, pubKey)) {
        // sha256 the pub key
        int hashLength = chatter->getEncryptor()->generateHash((const char*)pubKey, ENC_PUB_KEY_SIZE, hashBuffer);
        
        // sign it
        chatter->getEncryptor()->setMessageBuffer(hashBuffer);
        chatter->getEncryptor()->signMessage();
        chatter->getEncryptor()->hexify(chatter->getEncryptor()->getSignatureBuffer(), ENC_SIGNATURE_SIZE);
        memcpy(hexEncodedLicense, chatter->getEncryptor()->getHexBuffer(), ENC_SIGNATURE_SIZE*2);
        hexEncodedLicense[ENC_SIGNATURE_SIZE*2 - 1] = 0;
        return true;
    }

    logConsole("failed to generate license");
    return false;
}

bool ClusterAdmin::dumpLicense (const char* deviceId) {

    if (generateEncodedLicense(deviceId)) {
        Serial.print(CLUSTER_CFG_LICENSE);
        Serial.print(CLUSTER_CFG_DELIMITER);
        Serial.print(chatter->getDeviceId()); // we are the signer
        Serial.println(hexEncodedLicense);
        return true;
    }

    logConsole("license not sent");
    return false;
}


bool ClusterAdmin::onboardNewDevice (const char* hostClusterId, ChatterDeviceType deviceType, const uint8_t* devicePublicKey) {
    Serial.println("We will onboard this device");

    char newAddress[CHATTER_DEVICE_ID_SIZE + 1];
    memcpy(newAddress, chatter->getDeviceId(), CHATTER_DEVICE_ID_SIZE - 3);
    newAddress[CHATTER_DEVICE_ID_SIZE] = '\0';
    char alias[CHATTER_ALIAS_NAME_SIZE + 1];
    memset(alias, 0, CHATTER_ALIAS_NAME_SIZE + 1);

    switch (deviceType) {
        case ChatterDeviceBridgeLora:
            memcpy(newAddress + (CHATTER_DEVICE_ID_SIZE - 3), BASE_LORA_ADDRESS, 3);
            memcpy(alias, BASE_LORA_ALIAS, strlen(BASE_LORA_ALIAS));
            break;
        case ChatterDeviceBridgeWifi:
            memcpy(newAddress + (CHATTER_DEVICE_ID_SIZE - 3), BASE_WIFI_ADDRESS, 3);
            memcpy(alias, BASE_WIFI_ALIAS, strlen(BASE_WIFI_ALIAS));
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
        syncDevice (hostClusterId, newAddress, alias);
    }


}

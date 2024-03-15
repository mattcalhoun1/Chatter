#include "ClusterAssistant.h"

bool ClusterAssistant::attemptOnboard () {
    // wait for serial buffer write capability
    Encryptor* encryptor = chatter->getEncryptor();
    ClusterStore* clusterStore = chatter->getClusterStore();
    DeviceStore* deviceStore = chatter->getDeviceStore();
    Hsm* hsm = chatter->getHsm();
    TrustStore* trustStore = chatter->getTrustStore();
    bool success = false;

    sendOnboardRequest();
    sendPublicKeyAndAlias(hsm, encryptor, chatter->getDeviceAlias());
    Serial.println("end pub key");

    bool receivedId = false;
    bool receivedKey = false;
    bool receivedIv = false;
    bool receivedTrust = false;
    bool receivedWifiSsid = false;
    bool receivedWifiCred = false;
    bool receivedFrequency = false;
    bool receivedTime = false;
    bool receivedPrimaryChannel = false;
    bool receivedSecondaryChannel = false;
    bool receivedAuthType = false;

    // needs to be big enough to hold a public key plus command/config info
    char nextClusterConfig[ENC_PUB_KEY_SIZE*2 + 24];
    memset(nextClusterConfig, 0, ENC_PUB_KEY_SIZE*2 + 24);

    Serial.println("next cluster config memset done");

    while (!receivedId || !receivedKey || !receivedIv || !receivedTrust || !receivedWifiSsid || !receivedAuthType ||
        !receivedWifiCred || !receivedFrequency || !receivedTime || !receivedPrimaryChannel || !receivedSecondaryChannel) {
        // skip newlines and terms
        while (Serial.peek() == '\0' || Serial.peek() == '\r' || Serial.peek() == '\n') {
            Serial.println("cleaering a byte");
            Serial.read();
        }

        // read next line
        Serial.println("reading until...");
        int bytesRead = 0;
        while (Serial.available() > 0 && Serial.peek() != '\n' && bytesRead < ENC_PUB_KEY_SIZE + 24) {
            nextClusterConfig[bytesRead++] = Serial.read();
        }
        //int bytesRead = Serial.readBytesUntil('\n', nextClusterConfig, ENC_PUB_KEY_SIZE + 23);
        //Serial.println("Read " + String(bytesRead));

        for (int i = 0; i < bytesRead; i++) {
            Serial.print((char)nextClusterConfig[i]);
        }
        Serial.println("");

        if (strlen(nextClusterConfig) > 5) {
            ClusterConfigType typeIngested = ingestClusterData(nextClusterConfig, bytesRead, trustStore, encryptor);
            Serial.print("Ingested: "); Serial.println(typeIngested);
            switch (typeIngested) {
                case ClusterDeviceId:
                    receivedId = true;
                    break;
                case ClusterKey:
                    receivedKey = true;
                    break;
                case ClusterIv:
                    receivedIv = true;
                    break;
                case ClusterTrustStore:
                    receivedTrust = true;
                    break;
                case ClusterWifiSsid:
                    receivedWifiSsid = true;
                    break;
                case ClusterWifiCred:
                    receivedWifiCred = true;
                    break;
                case ClusterFrequency:
                    receivedFrequency = true;
                    break;
                case ClusterTime:
                    receivedTime = true;
                    break;
                case ClusterPrimaryChannel:
                    receivedPrimaryChannel = true;
                    break;
                case ClusterSecondaryChannel:
                    receivedSecondaryChannel = true;
                    break;
                case ClusterAuth:
                    receivedAuthType = true;
                    break;
            }
        }
        delay(1000);
        Serial.println("Waiting for input...");
    }

    Serial.println("Cluster config received. Adding to storage.");

            char newDeviceId[CHATTER_DEVICE_ID_SIZE + 1];
        char clusterId[STORAGE_GLOBAL_NET_ID_SIZE + STORAGE_LOCAL_NET_ID_SIZE + 1];

        char trustedDeviceId[CHATTER_DEVICE_ID_SIZE + 1];
        char alias[CHATTER_ALIAS_NAME_SIZE + 1];
        char hexEncodedPubKey[ENC_PUB_KEY_SIZE * 2 + 1];
        char hexEncodedSymmetricKey[ENC_SYMMETRIC_KEY_SIZE * 2 + 1];
        char hexEncodedIv[ENC_IV_SIZE * 2 + 1];

        char loraFrequency[CHATTER_LORA_FREQUENCY_DIGITS + 1];


        char wifiSsid[CHATTER_WIFI_STRING_MAX_SIZE + 1];
        char wifiCred[CHATTER_WIFI_STRING_MAX_SIZE + 1];

    clusterStore->addCluster (clusterId, alias, newDeviceId, symmetricKey, iv, frequency, wifiSsid, wifiCred, primaryChannel, secondaryChannel, authType, ClusterLicenseRoot);
    Serial.println("New cluster added.");

    return true;
}

ClusterConfigType ClusterAssistant::ingestClusterData (const char* dataLine, int bytesRead, TrustStore* trustStore, Encryptor* encryptor) {

    int dataPosition = -1;
    for (int i = 0; i < bytesRead; i++) {
        if (dataLine[i] == CLUSTER_CFG_DELIMITER) {
            dataPosition = i+1;
            i = bytesRead; // stop looping
        }
    }

    // if a delimiter was found
    if (dataPosition != -1) {
        const char* clusterData = dataLine + dataPosition;

        Serial.print("cluster data: ");
        for (int i = dataPosition; i < bytesRead; i++) {
            Serial.print((dataLine[i]));
        }
        Serial.println("");

        if (memcmp(dataLine, CLUSTER_CFG_DEVICE, strlen(CLUSTER_CFG_DEVICE)) == 0) {
            // the first 8 digits are the newly assigned id
            // device id setup
            memcpy(newDeviceId, clusterData, CHATTER_DEVICE_ID_SIZE);
            newDeviceId[CHATTER_DEVICE_ID_SIZE] = '\0';
            
            memcpy(clusterId, clusterData, STORAGE_GLOBAL_NET_ID_SIZE + STORAGE_LOCAL_NET_ID_SIZE);
            clusterId[STORAGE_GLOBAL_NET_ID_SIZE + STORAGE_LOCAL_NET_ID_SIZE + 1] = 0;
            
            return ClusterDeviceId;
        }
        else if (memcmp(dataLine, CLUSTER_CFG_TRUST, strlen(CLUSTER_CFG_TRUST)) == 0) {
            // first 8 digits are the trusted device id
            memcpy(trustedDeviceId, clusterData, CHATTER_DEVICE_ID_SIZE);
            trustedDeviceId[CHATTER_DEVICE_ID_SIZE] = '\0';

            //!!! hex encoded id is probably bigger than enc pub key by 2x
            memcpy(hexEncodedPubKey, clusterData + CHATTER_DEVICE_ID_SIZE, ENC_PUB_KEY_SIZE * 2);
            Serial.print("Encoded Pub Key: ");
            for (int i = 0; i < ENC_PUB_KEY_SIZE * 2; i++) {
                Serial.print(hexEncodedPubKey[i]);
            }
            Serial.println("");

            // if we set the public key buffer of encryptor using this encoded key,
            // we can get the non-encoded key back
            memset(pubKey, 0, ENC_PUB_KEY_SIZE);
            encryptor->hexCharacterStringToBytesMax(pubKey, hexEncodedPubKey, ENC_PUB_KEY_SIZE*2, ENC_PUB_KEY_SIZE);
            
            Serial.print("Raw Pub Key: ");
            for (int i = 0; i < ENC_PUB_KEY_SIZE; i++) {
                Serial.print(pubKey[i]); Serial.print(" ");
            }
            Serial.println("");

            // the rest of the buffer is alias
            memset(alias, 0, CHATTER_ALIAS_NAME_SIZE+1);
            memcpy(alias, clusterData + CHATTER_DEVICE_ID_SIZE + (ENC_PUB_KEY_SIZE*2), bytesRead - (dataPosition + CHATTER_DEVICE_ID_SIZE + (ENC_PUB_KEY_SIZE*2)));

            // this needs to go into the truststore now
            trustStore->addTrustedDevice(trustedDeviceId, alias, pubKey, true);

            return ClusterTrustStore;
        }
        else if (memcmp(dataLine, CLUSTER_CFG_KEY, strlen(CLUSTER_CFG_KEY)) == 0) {
            // not all devices store the key
            hexEncodedSymmetricKey[ENC_SYMMETRIC_KEY_SIZE*2] = '\0';
            if (chatter->getDeviceType() == ChatterDeviceCommunicator) {
                memcpy(hexEncodedSymmetricKey, clusterData, ENC_SYMMETRIC_KEY_SIZE*2);
            }
            else {
                memcpy(hexEncodedSymmetricKey, "E70455AE70455AE70455AE70455AE70", ENC_SYMMETRIC_KEY_SIZE*2); // topsecret
            }

            encryptor->hexCharacterStringToBytesMax(symmetricKey, hexEncodedSymmetricKey, ENC_SYMMETRIC_KEY_SIZE*2, ENC_SYMMETRIC_KEY_SIZE);
            //Serial.print("KEY: "); Serial.println(hexEncodedSymmetricKey);
            
            return ClusterKey;
        }
        else if (memcmp(dataLine, CLUSTER_CFG_IV, strlen(CLUSTER_CFG_IV)) == 0) {
            hexEncodedIv[ENC_IV_SIZE*2] = '\0';
            if (chatter->getDeviceType() == ChatterDeviceCommunicator) {
                memcpy(hexEncodedIv, clusterData, ENC_IV_SIZE*2);
            }
            else {
                memcpy(hexEncodedIv, "E70455AE70455AE7", ENC_IV_SIZE*2); // topsecret
            }

            encryptor->hexCharacterStringToBytesMax(iv, hexEncodedIv, ENC_IV_SIZE*2, ENC_IV_SIZE);

            //Serial.print("IV: "); + Serial.println(hexEncodedIv);
            return ClusterIv;
        }
        else if (memcmp(dataLine, CLUSTER_CFG_WIFI_SSID, strlen(CLUSTER_CFG_WIFI_SSID)) == 0) {
            memset(wifiSsid, 0, CHATTER_WIFI_STRING_MAX_SIZE+1);
            memcpy(wifiSsid, clusterData, bytesRead - dataPosition);

            //Serial.println("SSID: " + String(wifiConfig));
            Serial.print("ssid: ");
            for (int i = 0; i < bytesRead - dataPosition; i++) {
                Serial.print(wifiSsid[i]);
            }
            Serial.println("");

            return ClusterWifiSsid;
        }
        else if (memcmp(dataLine, CLUSTER_CFG_WIFI_CRED, strlen(CLUSTER_CFG_WIFI_CRED)) == 0) {
            memset(wifiCred, 0, CHATTER_WIFI_STRING_MAX_SIZE+1);
            memcpy(wifiCred, clusterData, bytesRead - dataPosition);

            Serial.print("cred: ");
            for (int i = 0; i < bytesRead - dataPosition; i++) {
                Serial.print('*');
            }
            Serial.println("");

            return ClusterWifiCred;
        }
        else if (memcmp(dataLine, CLUSTER_CFG_FREQ, strlen(CLUSTER_CFG_FREQ)) == 0) {
            loraFrequency[CHATTER_LORA_FREQUENCY_DIGITS] = 0;
            memcpy(loraFrequency, clusterData, 5);

            frequency = atof(loraFrequency);

            return ClusterFrequency;
        }
        else if (memcmp(dataLine, CLUSTER_CFG_TIME, strlen(CLUSTER_CFG_TIME)) == 0) {
            char sTime[13];
            sTime[12] = 0;
            memcpy(sTime, clusterData, 12);
            //Serial.println("Time: " + String(sTime));
            chatter->getRtc()->setNewDateTime(sTime);
            return ClusterTime;
        }
        else if (memcmp(dataLine, CLUSTER_CFG_PRIMARY, strlen(CLUSTER_CFG_PRIMARY)) == 0) {
            primaryChannel = (ClusterChannel)((uint8_t)(clusterData[0]));

            return ClusterPrimaryChannel;
        }
        else if (memcmp(dataLine, CLUSTER_CFG_SECONDARY, strlen(CLUSTER_CFG_SECONDARY)) == 0) {
            secondaryChannel = (ClusterChannel)((uint8_t)(clusterData[0]));

            return ClusterSecondaryChannel;
        }
        else if (memcmp(dataLine, CLUSTER_CFG_AUTH, strlen(CLUSTER_CFG_AUTH)) == 0) {
            authType = (ClusterAuthType)((uint8_t)(clusterData[0]));

            return ClusterAuth;
        }
        else if (memcmp(dataLine, CLUSTER_CFG_LICENSE, strlen(CLUSTER_CFG_LICENSE)) == 0) {
            // first 8 digits are the signer
            memcpy(signerId, clusterData, CHATTER_DEVICE_ID_SIZE);
            signerId[CHATTER_DEVICE_ID_SIZE] = '\0';

            //!!! hex encoded license is normal sig * 2
            memcpy(hexEncodedLicense, clusterData + CHATTER_DEVICE_ID_SIZE, ENC_SIGNATURE_SIZE * 2);
            hexEncodedLicense[ENC_SIGNATURE_SIZE * 2] = '\0';

            Serial.print("Encoded license: ");
            for (int i = 0; i < ENC_PUB_KEY_SIZE * 2; i++) {
                Serial.print(hexEncodedLicense[i]);;
            }
            Serial.println("");

            // if we set the public key buffer of encryptor using this encoded key,
            // we can get the non-encoded key back
            memset(license, 0, ENC_SIGNATURE_SIZE);
            encryptor->hexCharacterStringToBytesMax(license, hexEncodedLicense, ENC_SIGNATURE_SIZE*2, ENC_SIGNATURE_SIZE);
            
            Serial.print("Raw license: ");
            for (int i = 0; i < ENC_SIGNATURE_SIZE; i++) {
                Serial.print(license[i]); Serial.print(" ");
            }
            Serial.println("");

            // store the license
            if(chatter->getLicenseStore()->addLicense(clusterId, signerId, license)) {
                logConsole("License saved to license store.");
                return ClusterLicense;
            }
            else {
                logConsole("License failed to save");
                return ClusterNone;
            }

        }
    }

    // this was not a config
    return ClusterNone;
}

bool ClusterAssistant::sendOnboardRequest () {
    // send onboard request
    Serial.print(CLUSTER_REQ_ONBOARD);
    Serial.print(CLUSTER_REQ_DELIMITER);
    switch (chatter->getDeviceType()) {
        case ChatterDeviceBridgeWifi:
            Serial.print(DEVICE_TYPE_BRIDGE_WIFI);
            break;
        case ChatterDeviceCommunicator:
            Serial.print(DEVICE_TYPE_COMMUNICATOR);
            break;
        default:
            Serial.print(DEVICE_TYPE_RAW);
            break;
    }
    Serial.println("");
    Serial.flush();
    return true;
}

bool ClusterAssistant::sendPublicKeyAndAlias (Hsm* hsm, Encryptor* encryptor, const char* deviceAlias) {
    //uint8_t pubkey[ENC_PUB_KEY_SIZE];
    hsm->loadPublicKey(pubKey);
    encryptor->hexify(pubKey, ENC_PUB_KEY_SIZE);
    const char* hexifiedPubKey = encryptor->getHexBuffer();

    for (int hexCount = 0; hexCount < ENC_PUB_KEY_SIZE*2; hexCount++) {
        Serial.print(hexifiedPubKey[hexCount]);
    }
    Serial.print(deviceAlias);
    Serial.println("");
    return true;
}

#include "ClusterAssistant.h"

bool ClusterAssistant::attemptOnboard () {
    // wait for serial buffer write capability
    Encryptor* encryptor = chatter->getEncryptor();

    // clear our own truststore before proceeding
    chatter->getTrustStore()->clearTruststore();
    bool success = false;

    sendOnboardRequest();
    sendPublicKey(encryptor);
    Serial.println("end pub key");

    bool receivedId = false;
    bool receivedKey = false;
    bool receivedIv = false;
    bool receivedTrust = false;
    bool receivedWifi = false;
    bool receivedFrequency = false;
    bool receivedTime = false;

    // needs to be big enough to hold a public key plus command/config info
    char nextClusterConfig[ENC_PUB_KEY_SIZE + 24];
    memset(nextClusterConfig, 0, ENC_PUB_KEY_SIZE + 24);

    Serial.println("next cluster config memset done");

    while (!receivedId || !receivedKey || !receivedIv || !receivedTrust || !receivedWifi || !receivedFrequency || !receivedTime) {
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
            ClusterConfigType typeIngested = ingestClusterData(nextClusterConfig, bytesRead, encryptor);
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
                case ClusterWifi:
                    receivedWifi = true;
                    break;
                case ClusterFrequency:
                    receivedFrequency = true;
                    break;
                case ClusterTime:
                    receivedTime = true;
                    break;
            }
        }
        delay(1000);
        Serial.println("Waiting for input...");
    }

    return true;
}

ClusterConfigType ClusterAssistant::ingestClusterData (const char* dataLine, int bytesRead, Encryptor* encryptor) {

/*
#define CLUSTER_CFG_TRUST "TRUST"
#define CLUSTER_CFG_KEY "KEY"
#define CLUSTER_CFG_IV "IV"
#define CLUSTER_CFG_WIFI "WIFI"
#define CLUSTER_CFG_TIME "TIME"
#define CLUSTER_CFG_FREQ "FREQ"
#define CLUSTER_CFG_DEVICE "DEVICE"
*/

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
            char newDeviceId[CHATTER_DEVICE_ID_SIZE + 1];
            memcpy(newDeviceId, clusterData, CHATTER_DEVICE_ID_SIZE);
            newDeviceId[CHATTER_DEVICE_ID_SIZE] = '\0';
            encryptor->setTextSlotBuffer(newDeviceId);
            encryptor->saveDataSlot(DEVICE_ID_SLOT);
            return ClusterDeviceId;
        }
        else if (memcmp(dataLine, CLUSTER_CFG_TRUST, strlen(CLUSTER_CFG_TRUST)) == 0) {
            // first 8 digits are the trusted device id
            char trustedDeviceId[CHATTER_DEVICE_ID_SIZE + 1];
            memcpy(trustedDeviceId, clusterData, CHATTER_DEVICE_ID_SIZE);
            trustedDeviceId[CHATTER_DEVICE_ID_SIZE] = '\0';

            // next 128 are the public key
            memcpy(encryptor->getPublicKeyBuffer(), clusterData + CHATTER_DEVICE_ID_SIZE, ENC_PUB_KEY_SIZE);

            // the rest of the buffer is alias
            char alias[CHATTER_ALIAS_NAME_SIZE + 1];
            memset(alias, 0, CHATTER_ALIAS_NAME_SIZE+1);
            memcpy(alias, clusterData + CHATTER_DEVICE_ID_SIZE + ENC_PUB_KEY_SIZE, bytesRead - (dataPosition + CHATTER_DEVICE_ID_SIZE + ENC_PUB_KEY_SIZE));

            chatter->getTrustStore()->addTrustedDevice(trustedDeviceId, alias, (char*)encryptor->getPublicKeyBuffer());
            return ClusterTrustStore;
        }
        else if (memcmp(dataLine, CLUSTER_CFG_KEY, strlen(CLUSTER_CFG_KEY)) == 0) {
            char newKey[ENC_SYMMETRIC_KEY_BUFFER_SIZE];
            newKey[ENC_SYMMETRIC_KEY_BUFFER_SIZE-1] = '\0';
            memcpy(newKey, clusterData, ENC_SYMMETRIC_KEY_BUFFER_SIZE-1);

            Serial.println("KEY: " + String(newKey));

            encryptor->setDataSlotBuffer(newKey);
            encryptor->saveDataSlot(ENCRYPTION_KEY_SLOT);
            return ClusterKey;
        }
        else if (memcmp(dataLine, CLUSTER_CFG_IV, strlen(CLUSTER_CFG_IV)) == 0) {
            char newKey[ENC_SYMMETRIC_KEY_BUFFER_SIZE];
            newKey[ENC_SYMMETRIC_KEY_BUFFER_SIZE-1] = '\0';
            memcpy(newKey, clusterData, ENC_SYMMETRIC_KEY_BUFFER_SIZE-1);

            Serial.println("IV: " + String(newKey));

            encryptor->setDataSlotBuffer(newKey);
            encryptor->saveDataSlot(ENCRYPTION_IV_SLOT);
            return ClusterIv;
        }
        else if (memcmp(dataLine, CLUSTER_CFG_WIFI, strlen(CLUSTER_CFG_WIFI)) == 0) {
            char wifiConfig[WIFI_SSID_MAX_LEN + WIFI_PASSWORD_MAX_LEN + 1];
            memset(wifiConfig, 0, WIFI_SSID_MAX_LEN + WIFI_PASSWORD_MAX_LEN);
            memcpy(wifiConfig, clusterData, bytesRead - dataPosition);

            //Serial.println("SSID: " + String(wifiConfig));
            Serial.print("ssid: ");
            for (int i = 0; i < bytesRead - dataPosition; i++) {
                Serial.print(wifiConfig[i]);
            }
            Serial.println("");

            encryptor->setTextSlotBuffer(wifiConfig);
            encryptor->saveDataSlot(WIFI_SSID_SLOT);

            Serial.println("ecryptor saved.");            
            return ClusterWifi;
        }
        else if (memcmp(dataLine, CLUSTER_CFG_FREQ, strlen(CLUSTER_CFG_FREQ)) == 0) {
            char freq[6];
            freq[5] = 0;
            memcpy(freq, clusterData, 5);

            encryptor->setTextSlotBuffer(freq);
            Serial.println("saving from assistant");
            encryptor->saveDataSlot(LORA_FREQUENCY_SLOT);
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
    }

    // this was not a config
    return ClusterNone;
}

void ClusterAssistant::sendOnboardRequest () {
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
}

void ClusterAssistant::sendPublicKey (Encryptor* encryptor) {
    encryptor->loadPublicKey(CHATTER_SIGN_PK_SLOT);
    encryptor->hexify(encryptor->getPublicKeyBuffer(), ENC_PUB_KEY_SIZE);
    const char* hexifiedPubKey = encryptor->getHexBuffer();

    for (int hexCount = 0; hexCount < ENC_PUB_KEY_SIZE; hexCount++) {
        Serial.print(hexifiedPubKey[hexCount]);
    }
    Serial.println("");
}

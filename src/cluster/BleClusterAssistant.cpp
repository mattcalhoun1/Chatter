#include "BleClusterAssistant.h"

bool BleClusterAssistant::init () {
  if (!BLE.begin()) {
    logConsole("BLE init failed");
    return false;
  }
  
  BLE.setLocalName(BLE_CLUSTER_COMMUNICATOR_SERVICE_NAME); 
  BLE.advertise();
}

bool BleClusterAssistant::attemptOnboard () {
    // wait for serial buffer write capability
    Encryptor* encryptor = chatter->getEncryptor();
    ClusterStore* clusterStore = chatter->getClusterStore();
    DeviceStore* deviceStore = chatter->getDeviceStore();
    Hsm* hsm = chatter->getHsm();
    TrustStore* trustStore = chatter->getTrustStore();
    bool success = false;

    sendOnboardRequest();
    sendPublicKey(hsm, encryptor);
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
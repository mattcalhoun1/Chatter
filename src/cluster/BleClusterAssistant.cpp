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
    if (connectToPeripheral() && establishBleBuffer()) {
        // wait for serial buffer write capability
        Encryptor* encryptor = chatter->getEncryptor();
        ClusterStore* clusterStore = chatter->getClusterStore();
        DeviceStore* deviceStore = chatter->getDeviceStore();
        Hsm* hsm = chatter->getHsm();
        TrustStore* trustStore = chatter->getTrustStore();
        bool success = false;

        if (sendOnboardRequest()) {
            logConsole("Onboard request sent and acknowledged. Sending public key.");

            if (sendPublicKeyAndAlias(hsm, encryptor, chatter->getDeviceAlias())) {
                logConsole("Public key accepted. Beginning onboard.");

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
                bool receivedLicense = false;

                unsigned long onboardStart = millis();

                // loop until session times out or we've received all configs, whichever comes first
                while ((millis() - onboardStart < BLE_MAX_ONBOARD_SESSION_TIME) && 
                    (
                        !receivedId || !receivedKey || !receivedIv || !receivedTrust || 
                        !receivedWifiSsid || !receivedAuthType || !receivedWifiCred || 
                        !receivedFrequency || !receivedTime || !receivedPrimaryChannel || 
                        !receivedSecondaryChannel || !receivedLicense)) 
                {
                    if (bleBuffer->receiveRxBufferFromClient(&peripheral)) {
                        ClusterConfigType typeIngested = ingestClusterData((const char*)bleBuffer->getRxBuffer(), bleBuffer->getRxBufferLength(), trustStore, encryptor);
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
                            case ClusterLicense:
                                receivedLicense = true;
                                break;
                        }
                    }
                    delay(1000);
                    logConsole("Waiting for admin device");
                }

                logConsole("Full config received. Adding cluster to storage.");

                if(clusterStore->addCluster (clusterId, alias, newDeviceId, symmetricKey, iv, frequency, wifiSsid, wifiCred, primaryChannel, secondaryChannel, authType, ClusterLicenseRoot)) {
                    logConsole("New cluster added. Setting as default.");
                    return chatter->getDeviceStore()->setDefaultClusterId(clusterId);
                }
                else {
                    logConsole("cluster not saved!");
                }
            }
            else {
                logConsole("Public key not sent.");
            }
        }
        else {
            logConsole("appeared to connect, but onboard request not received.");
        }
    }
    else {
        logConsole("Peripheral not connected or buffer not established");
    }

    logConsole("Onboard was not successful");
    return false;
}

bool BleClusterAssistant::sendOnboardRequest() {
    bleBuffer->clearTxBuffer();
    uint8_t* txBuffer = bleBuffer->getTxBuffer();
    sprintf((char*)txBuffer, "%s", "ONBD:C0");
    bleBuffer->setTxBufferLength(7);

    logConsole("Sending onboard request");
    if (bleBuffer->sendTxBufferToClient()) {
        logConsole("Wrote onboard request"); 

        // wait for acknowledgement to proceed
        return waitForPrompt("PUB", BLE_MAX_SESSION_STEP_WAIT);
    }

    logConsole("Onboard request not processed");
    return false;
}

bool BleClusterAssistant::connectToPeripheral(){
  logConsole("- Discovering peripheral device...");
  unsigned long startTime = millis();
  do
  {
    logConsole("Scanning");
    if (!BLE.scanForUuid(BLE_CLUSTER_INTERFACE_UUID)) {
      BLE.scanForName(BLE_CLUSTER_ADMIN_SERVICE_NAME);
    }
    peripheral = BLE.available();

    if (!peripheral) {
      delay(BLE_PERIPHERAL_SCAN_FREQ);
    }
  } while ((!peripheral) && (millis() - startTime < BLE_PERIPHERAL_CONNECT_TIME));
  
  if (peripheral) {
    logConsole("Admin device found");
    Serial.print("Admin MAC address: ");
    Serial.println(peripheral.address());
    BLE.stopScan();
    return true;
  }

  return false;
}

bool BleClusterAssistant::establishBleBuffer () {
  logConsole("Establishing ble buffer to peripheral");

  if (peripheral.connect()) {
    logConsole("Peripheral connected");
  } else {
    logConsole("Peripheral connection failed");
    return false;
  }

  if (peripheral.discoverAttributes()) {
    logConsole("Peripheral attribute discovery complete");
  } else {
    logConsole("Peripheral attribute discovery failed");
    peripheral.disconnect();
    return false;
  }

  rxObj = peripheral.characteristic(BLE_CLUSTER_RX_UUID);
  txObj = peripheral.characteristic(BLE_CLUSTER_TX_UUID);
  rxReadObj = peripheral.characteristic(BLE_CLUSTER_RX_READ_UUID);
  txReadObj = peripheral.characteristic(BLE_CLUSTER_TX_READ_UUID);
  status = peripheral.characteristic(BLE_CLUSTER_STATUS_UUID);
    
  if (!rxObj || !txObj || !rxReadObj || !txReadObj || !status) {
    logConsole("Admin missing expected characteristics!");
    peripheral.disconnect();
    return false;
  } else if (!rxObj.canWrite() || !txObj.canRead() || !txReadObj.canWrite() || !rxReadObj.canRead()) {
    logConsole("Peripheral does not have all read/write access!");
    peripheral.disconnect();
    return false;
  }

  if (peripheral.connected()) {
    // rx and tx are swiched at this end
    BLECharacteristic* rx = &txObj;
    BLECharacteristic* rxRead = &txReadObj;
    BLECharacteristic* tx = &rxObj;
    BLECharacteristic* txRead = &rxReadObj;
    bleBuffer = new BleBuffer(tx, txRead, rx, rxRead);
    return true;
  }

  logConsole("Peripherael connection lost");
  return false;
}

bool BleClusterAssistant::sendPublicKeyAndAlias (Hsm* hsm, Encryptor* encryptor, const char* deviceAlias) {
    hsm->loadPublicKey(pubKey);
    encryptor->hexify(pubKey, ENC_PUB_KEY_SIZE);
    const char* hexifiedPubKey = encryptor->getHexBuffer();

    bleBuffer->clearTxBuffer();
    uint8_t* txBuffer = bleBuffer->getTxBuffer();
    memcpy(txBuffer, "PUB:", 4);
    memcpy(txBuffer+4, (uint8_t*)encryptor->getHexBuffer(), ENC_PUB_KEY_SIZE*2);
    memcpy(txBuffer+4+(ENC_PUB_KEY_SIZE*2), deviceAlias, strlen(deviceAlias));
    bleBuffer->setTxBufferLength(ENC_PUB_KEY_SIZE*2 + 4 + strlen(deviceAlias));

    return bleBuffer->sendTxBufferToClient();
}
  
bool BleClusterAssistant::waitForPrompt (const char* expectedPrompt, int maxWaitTime) {
  unsigned long startTime = millis();
  bool receivedPrompt = false;

  while (millis() - startTime < BLE_MAX_SESSION_STEP_WAIT && !receivedPrompt) {
    if (bleBuffer->receiveRxBufferFromClient (&peripheral)) {
      Serial.print("Prompt: "); Serial.println((char*)bleBuffer->getRxBuffer());
      receivedPrompt = strcmp((char*)bleBuffer->getRxBuffer(), expectedPrompt) == 0;
    }

    if (!receivedPrompt) {
      delay(100);
    }
  }

  return receivedPrompt;
}

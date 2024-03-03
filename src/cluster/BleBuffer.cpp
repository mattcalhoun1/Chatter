#include "BleBuffer.h"


bool BleBuffer::writeBleBufferWait (BLECharacteristic* bleChar, BLECharacteristic* bleFlagChar, uint8_t* buff, int len) {
    // clear the tx read flag
    bleChar->writeValue(buff, BLE_SMALL_BUFFER_SIZE);
    bleFlagChar->writeValue("0");
    unsigned long startTime = millis();
    bool ack = false;
    char flag[] = {'\0'};

    while (!ack && millis() - startTime < BLE_MAX_SESSION_STEP_WAIT) {
        BLE.poll();
        bleFlagChar->readValue(flag, 1);
        ack = flag[0] == '1';
        if (!ack) {
            delay(100);
        }
    }

    return ack;
}

bool BleBuffer::sendBufferToClient (BLECharacteristic* bleChar, BLECharacteristic* bleFlagChar, uint8_t* buff, uint8_t len) {
    uint8_t bytesSent = 0;
    unsigned long startTime = millis();
    memset(bleBuffer, BLE_HEADER_CHAR, BLE_SMALL_BUFFER_SIZE);
    bleBufferLength = BLE_SMALL_BUFFER_SIZE;
    if (!writeBleBufferWait(bleChar, bleFlagChar, bleBuffer, bleBufferLength)) {
        logConsole("Failed to transmit header");
        return false;
    }
    while (bytesSent < len && millis() - startTime < BLE_MAX_SESSION_STEP_WAIT) {
        // copy next portion of txbuffer into blebuffer
        memset(bleBuffer, 0, BLE_SMALL_BUFFER_SIZE);
        if (len - bytesSent < BLE_SMALL_BUFFER_SIZE) {
            bleBufferLength = len - bytesSent;
        }
        else {
            bleBufferLength = BLE_SMALL_BUFFER_SIZE;
        }
        memcpy(bleBuffer, buff + bytesSent, bleBufferLength);
        if (!writeBleBufferWait(bleChar, bleFlagChar, bleBuffer, bleBufferLength)) {
            logConsole("Write ble buffer failed or was not acknowledged");
            return true;
        }
        else {
            bytesSent += bleBufferLength;
        }
    }

    memset(bleBuffer, BLE_FOOTER_CHAR, BLE_SMALL_BUFFER_SIZE);
    bleBufferLength = BLE_SMALL_BUFFER_SIZE;
    if (!writeBleBufferWait(bleChar, bleFlagChar, bleBuffer, bleBufferLength)) {
        logConsole("Failed to transmit footer");
        return false;
    }

    return bytesSent >= len;
}

bool BleBuffer::sendTxBufferToClient () {
    return sendBufferToClient(tx, txRead, txBuffer, txBufferLength);
}

bool BleBuffer::bleBufferContainsHeader () {
    for (uint8_t i = 0; i < BLE_SMALL_BUFFER_SIZE; i++) {
        if ((char)bleBuffer[i] != BLE_HEADER_CHAR) {
            return false;
        }
    }

    return true;
}

bool BleBuffer::bleBufferContainsFooter () {
    for (uint8_t i = 0; i < BLE_SMALL_BUFFER_SIZE; i++) {
        if ((char)bleBuffer[i] != BLE_FOOTER_CHAR) {
            return false;
        }
    }

    return true;
}

uint8_t BleBuffer::receiveBufferFromClient (BLECharacteristic* bleChar, BLECharacteristic* bleFlagChar, uint8_t* buff, uint8_t maxLength, BLEDevice* device) {
    bool receivedHeader = false;
    bool receivedFooter = false;
    uint8_t bytesRead = 0;

    unsigned long startTime = millis();
    memset(buff, 0, maxLength);
    char flag[1];

    while ((!receivedHeader || !receivedFooter) && (millis() - startTime < BLE_MAX_SESSION_STEP_WAIT) && (bytesRead <= maxLength)) {
        if(device) {
            BLE.poll();
            bleFlagChar->readValue(flag, 1);

            if (flag[0] == '0') {
                memset(bleBuffer, 0, BLE_SMALL_BUFFER_SIZE);
                bleChar->readValue(bleBuffer, BLE_SMALL_BUFFER_SIZE);

                if (!receivedHeader && bleBufferContainsHeader()) {
                    receivedHeader = true;
                }
                else if (!receivedFooter && bleBufferContainsFooter()) {
                    receivedFooter = true;
                }
                else {
                    // add all chars up to the null/term
                    uint8_t end = findChar('\0', bleBuffer, BLE_SMALL_BUFFER_SIZE);
                    if (end == 255 && bytesRead + BLE_SMALL_BUFFER_SIZE <= maxLength) {
                        // this full buffer is part of the transmission
                        memcpy(buff + bytesRead, bleBuffer, BLE_SMALL_BUFFER_SIZE);
                        bytesRead += BLE_SMALL_BUFFER_SIZE;
                    }
                    else if (bytesRead + (end+1) <= maxLength) {
                        memcpy(buff + bytesRead, bleBuffer, end + 1);
                        bytesRead += (end+1);
                    }
                    else {
                        logConsole("Message too large for buffer!");
                        return 0;
                    }
                }

                // acknowledge receipt to the device
                bleFlagChar->writeValue("1");
                BLE.poll();
            }

        }
        else {
            logConsole("device disconnected while waiting");
            return false;
        }
    }

    return receivedHeader && receivedFooter ? bytesRead : 0;
}


bool BleBuffer::receiveRxBufferFromClient (BLEDevice* device) {
    rxBufferLength = receiveBufferFromClient(rx, rxRead, rxBuffer, BLE_MAX_BUFFER_SIZE, device);
    return rxBufferLength > 0;
}

bool BleBuffer::send (const char* content) {
    clearTxBuffer();
    sprintf((char*)txBuffer, "%s", content);
    txBufferLength = strlen(content);
    return sendTxBufferToClient();    
}

void BleBuffer::clearTxBuffer () {
    memset(txBuffer, 0, BLE_MAX_BUFFER_SIZE+1);
    txBufferLength = 0;
}

void BleBuffer::clearRxBuffer () {
    memset(rxBuffer, 0, BLE_MAX_BUFFER_SIZE+1);
    rxBufferLength = 0;
}

void BleBuffer::logConsole(const char* msg) {
    Serial.println(msg);
}

uint8_t BleBuffer::findChar (char toFind, const  uint8_t* buffer, uint8_t bufferLen) {
    for (uint8_t i = 0; i < bufferLen; i++) {
        Serial.print((char)buffer[i]);
        if (buffer[i] == (uint8_t)toFind) {
            return i;
        }
    }
    Serial.println("");

    return 255; // not found
}
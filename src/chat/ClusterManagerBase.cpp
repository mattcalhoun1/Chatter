#include "ClusterManagerBase.h"

bool ClusterManagerBase::ingestPublicKey (byte* buffer) {
    int bytesRead = 0;
    bool validKey = true; // assume valid until we find a bad byte
    unsigned long startTime = millis();
    unsigned long maxWait = 3000;//3 sec

    // pub key can only be hex with no spaces
    while ((millis() - startTime < maxWait) && bytesRead < ENC_PUB_KEY_SIZE && validKey) {
        if (Serial.available()) {
            // ignore carriage returns, as this may be preceeded by one
            if (Serial.peek() == '\n' || Serial.peek() == CLUSTER_REQ_DELIMITER || Serial.peek() == '\0' || Serial.peek() == '\r') {
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

bool ClusterManagerBase::ingestSymmetricKey (byte* buffer) {
    int bytesRead = 0;
    bool validKey = true; // assume valid until we find a bad byte
    unsigned long startTime = millis();
    unsigned long maxWait = 3000;//3 sec

    // key can only be hex with no spaces
    while ((millis() - startTime < maxWait) && bytesRead < (ENC_SYMMETRIC_KEY_BUFFER_SIZE-1) && validKey) {
        if (Serial.available()) {
            // ignore carriage returns, as this may be preceeded by one
            if (Serial.peek() == '\n' || Serial.peek() == CLUSTER_REQ_DELIMITER || Serial.peek() == '\0' || Serial.peek() == '\r') {
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

    if (bytesRead == (ENC_SYMMETRIC_KEY_BUFFER_SIZE-1) && validKey) {
        return true;
    }

    return false;
}

bool ClusterManagerBase::getUserInput (const char* prompt, char* inputBuffer, int minLength, int maxLength, bool symbolsAllowed, bool lowerCaseAllowed) {
    Serial.print(prompt);

    while (Serial.available() < minLength + 1) {
        delay(10);
    }
    char input[maxLength+1];
    memset(input, 0, maxLength + 1);
    Serial.readBytesUntil('\n', input, maxLength);

    //const char* input = Serial.readStringUntil('\n').c_str();
    int inCount = 0;
    int goodCount = 0;

    Serial.println(input);

    while (goodCount < maxLength && inCount < strlen(input)) {
        if (input[inCount] == '\r' || input[inCount] == '\n' || input[inCount] == '\0') {
            //skip this
            inCount++;
        }
        else {
            // if no symbols allowed, make sure this is not a symbols
            if (input[inCount] == '@' || input[inCount] == '.' || input[inCount] == '$' || input[inCount] == '!') {
                if (symbolsAllowed) {
                    // this is ok
                    inputBuffer[goodCount++] = input[inCount++];
                }
                else {
                    Serial.println("Invalid character: " + String(input[inCount]));
                    return false;
                }
            }
            else if ((input[inCount] >= '0' && input[inCount] <= '9') || (input[inCount] >= 'A' && input[inCount] <= 'Z')) {
                // this is definitely ok
                inputBuffer[goodCount++] = input[inCount++];
            }
            else if (input[inCount] >= 'a' && input[inCount] <= 'z') {
                if (lowerCaseAllowed) {
                    inputBuffer[goodCount++] = input[inCount++];
                }
                else {
                    Serial.println("Invalid lower input: " + String(input[inCount]));
                    return false;
                }
            }
            else {
                Serial.println("Invalid input: " + String(input[inCount]));
                return false;
            }
        }
    }
    return goodCount >= minLength;
}

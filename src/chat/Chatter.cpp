#include "Chatter.h"
#include <List.hpp>

bool Chatter::init (const char* devicePassword) {
    setupFramEncryptionKey(devicePassword);

    running = false;
    if (mode == BridgeMode || mode == BasicMode) {
        logConsole("Creating caching fram datastore");

        // choose storage type.
        // if this is a new device, we will need to build the fram data from scratch, so lazy load for now
        switch (packetStorageType) {
            case StorageFramSPI:
                fram = new CachingFramDatastore(framEncryptionKey, framEncryptionKeySize, FramSPI, true);
                break;
            case StorageFramI2C:
                fram = new CachingFramDatastore(framEncryptionKey, framEncryptionKeySize, FramI2C, true);
                break;
            default:
                logConsole("Unsupported storage type");
                return false;
        }

        if (fram->init()) {
            logConsole("Fram initialized");
            deviceStore = new FramDeviceStore(fram);
            clusterStore = new FramClusterStore(fram);

            trustStore = new FramTrustStore(fram);
            if (!trustStore->init()) {
                logConsole("TrustStore did not initialize!");
                return false;
            }

            licenseStore = new FramLicenseStore(fram);
            if (!licenseStore->init()) {
                logConsole("LicenseStore did not initialize!");
                return false;
            }

            if(deviceStoreInitialized ()) {
                logConsole("Connecting to Default Cluster: ");
                logConsole(defaultClusterId);

                if (loadClusterConfig(defaultClusterId)) {
                    hsm = new PseudoHsm(deviceStore, clusterStore);
                    if (!hsm->init(clusterId)){
                        logConsole("Unable to initialize hsm");
                    }

                    encryptor = new Encryptor(trustStore, hsm);

                    if(encryptor->init()) {
                        encryptor->loadPublicKey();
                        logConsole("Encryption module connected");
                        packetStore = new FramPacketStore(fram, rtc);
                        if (!packetStore->init()) {
                            logConsole("PacketStore did not initialize!");
                            return false;
                        }

                        // clear all messages
                        packetStore->clearAllMessages();

                        // use the sender public key buffer for this, since there's no sending going on yet
                        loraFrequency = clusterStore->getFrequency(clusterId);
                        if (loraFrequency < 868.0 || loraFrequency > 960.0) {
                            logConsole("Lora frequency out of range, using default");
                            loraFrequency = LORA_DEFAULT_FREQUENCY;
                        }
                        logConsole("Lora frequency: " + String(loraFrequency));

                        // initialize statuses to no device
                        for (int d = 0; d < CHAT_MAX_CHANNELS; d++) {
                            status[d] = ChatNoDevice;
                        }
                        
                        logConsole("Device ID: " + String(deviceId));

                        logConsole("Known Devices: ");
                        List<String> others = trustStore->getDeviceIds();
                        char otherDeviceAlias[CHATTER_ALIAS_NAME_SIZE + 1];
                        for (int i = 0; i < others.getSize(); i++) {
                            const String& otherDeviceStr = others[i];
                            // try loading public key for that
                            const char* otherDeviceId = otherDeviceStr.c_str();
                            trustStore->loadAlias(otherDeviceId, otherDeviceAlias);
                            trustStore->loadPublicKey(otherDeviceId, (uint8_t*)encryptor->getPublicKeyBuffer());
                            logConsole("Device: " + String(otherDeviceId) + ", Alias: " + String(otherDeviceAlias));
                        }

                        running = true;
                    }
                    else {
                        logConsole("Chatter did not initialize. Encryption chip error?");
                    }
                } 
                else {
                    logConsole("Cluster store not initialized or missing default cluster");
                }
            }
            else {
                // need onboarding/etc
                logConsole("Device not yet initialized.");
            }
        }
        else {
            logConsole("Fram not initialized");
        }
    }
    else {
        logConsole("Invalid Chatter mode!");
    }

    return running;
}

bool Chatter::selfAnnounce (bool force) {
    if (force || millis() - selfAnnounceFrequency > lastAnnounce) {
        logConsole("Self Announcing");
        return broadcastDeviceInfo(false);
    }
}

long Chatter::getRandom() {
    return encryptor->getRandom();
}

void Chatter::generateMessageId (char* messageIdBuffer) {
    uint8_t r;
    for (int i = 0; i < CHATTER_MESSAGE_ID_SIZE; i++) {
        r = encryptor->getRandom() % 16;
        if (r < 10) {
            messageIdBuffer[i] = (char)(r+48);
        }
        else {
            messageIdBuffer[i] = (char)(r + 55);
        }
    }
    messageIdBuffer[CHATTER_MESSAGE_ID_SIZE] = '\0';
}


bool Chatter::isExpired() {
    //logConsole("Checking expiry of Message " + String((char*)receiveBuffer.messageId) + " from " + String((char*)receiveBuffer.sender));

    // header format is:
    // HHH[nbf][naf]... where nbf and naf are in format: YYMMDDhhmmss
    // example: HHH231231235959240101075959 meaning the message is good from 12/31/2023 23:59:59 to 01/01/2024 07:59:59

    uint8_t tsLength = 12; // lenght of the sortable timestamp, same format as explained above.
    const char* nbf = (char*)receiveBuffer.content + 3 + (mode == BridgeMode ? receiveBuffer.headerLength : 0);
    const char* na = (char*)receiveBuffer.content + 3 + tsLength + (mode == BridgeMode ? receiveBuffer.headerLength : 0);
    const char* now = rtc->getSortableTime();

    /*Serial.print("nbf: ");
    for (int i = 0; i < tsLength; i++) {
        Serial.print(nbf[i]);
    }
    Serial.println("");

    Serial.print("na: ");
    for (int i = 0; i < tsLength; i++) {
        Serial.print(na[i]);
    }
    Serial.println("");

    Serial.print("now: ");
    for (int i = 0; i < tsLength; i++) {
        Serial.print(now[i]);
    }
    Serial.println("");*/
    

    // since the time format is sortable string, we can simply compare the ascii values
    return memcmp(now, nbf, tsLength) < 0 || memcmp(now, na, tsLength) > 0;
}

bool Chatter::isSenderKnown (const char* senderId) {
    return trustStore->loadPublicKey((char*)receiveBuffer.sender, senderPublicKey);
}

bool Chatter::validateSignature() {
    return validateSignature(true);
}

bool Chatter::validateSignature(bool checkHash) {
    //logConsole("Validating Signature of Message " + String((char*)receiveBuffer.messageId) + " from " + String((char*)receiveBuffer.sender));

    // bridge only checks sig, not hash. dont want to decrypt
    if(checkHash == false || packetStore->hashMatches((char*)receiveBuffer.sender, (char*)receiveBuffer.messageId)) {
        // check signature now
        if (trustStore->loadPublicKey((char*)receiveBuffer.sender, senderPublicKey)) {
            int packetSize = packetStore->readPacket((char*)receiveBuffer.sender, (char*)receiveBuffer.messageId, CHATTER_SIGNATURE_PACKET_ID, footerBuffer, receiveBuffer.headerLength + CHATTER_HASH_SIZE + CHATTER_SIGNATURE_LENGTH);

            if (packetSize == receiveBuffer.headerLength + CHATTER_HASH_SIZE + CHATTER_SIGNATURE_LENGTH) {
                uint8_t* providedSignature = footerBuffer + receiveBuffer.headerLength + CHATTER_HASH_SIZE; // skip the hash to get to the sig
                encryptor->setPublicKeyBuffer(senderPublicKey);
                encryptor->setSignatureBuffer(providedSignature);
                encryptor->setMessageBuffer(footerBuffer + receiveBuffer.headerLength);
                if(encryptor->verify()) {
                    logConsole("Signature is good");
                    return true;
                }
                else {
                    logConsole("Bad signature");
                }
            }
            else {
                logConsole("Unable to read signature packet from storage. Size on storage was: " + String(packetSize) + ", expected: " + String(receiveBuffer.headerLength + CHATTER_HASH_SIZE + CHATTER_SIGNATURE_LENGTH));
            }
        }
        else {
            logConsole("Unable to load public key for " + String((char*)receiveBuffer.sender));
        }
    }
    else {
        logConsole("Bad hash!");
    }

    return false;
}

void Chatter::updateChatStatus (uint8_t channelNum, ChatStatus newStatus) {
    // cache the status
    status[channelNum] = newStatus;

    // bubble event out to listener
    statusCallback->updateChatStatus(channelNum, newStatus);
}

void Chatter::addLoRaChannel (int csPin, int intPin, int rsPin, bool logEnabled) {
    uint8_t loraAddr = getSelfAddress();
    LoRaChannel* loraChannel = new LoRaChannel(numChannels, loraFrequency, loraAddr, csPin, intPin, rsPin, logEnabled, this);
    channels[numChannels++] = loraChannel;

    if(loraChannel->init())
    {
        logConsole("Added LoRa Channel on Address: " + String(loraAddr));
    }
    else {
        logConsole("LoRa init failed!");
    }

}

void Chatter::addUartChannel (bool logEnabled) {
    UARTChannel* uartChannel = new UARTChannel(numChannels, logEnabled, this);
    channels[numChannels++] = uartChannel;

    if(uartChannel->init()) {
        logConsole("Added UART Channel");
    }
    else {
        logConsole("UART init failed!");
    }
}

/*void Chatter::addCANChannel (bool logEnabled) {
    CANChannel* canChannel = new CANChannel();
    channels[numChannels++] = canChannel;

    if  (canChannel->init()) {
        logConsole("Added CAN channel");
    }
    else {
        logConsole("CAN init failed!");
    }
}*/

void Chatter::addAirliftUdpChannel (int ssPin, int ackPin, int resetPin, int gpi0, bool logEnabled) {
    UdpChannel* udpChannel = new AirliftUdpChannel(numChannels, getDeviceId(), wifiSsid, wifiCred, ssPin, ackPin, resetPin, gpi0, logEnabled, this);
    channels[numChannels++] = udpChannel;
    udpChannel->init();

    logConsole("Airlift UDP Channel with hostname: " + String(getDeviceId()));
}

void Chatter::addOnboardUdpChannel(bool logEnabled) {
    UdpChannel* udpChannel = new OnboardUdpChannel(numChannels, getDeviceId(), wifiSsid, wifiCred, logEnabled, this);
    channels[numChannels++] = udpChannel;
    udpChannel->init();

    logConsole("Onboard UDP Channel with hostname: " + String(getDeviceId()));
}

uint8_t Chatter::getAddress(const char* deviceId) {
    return 0;
}

uint8_t Chatter::getSelfAddress () {
    // self address is numeric last 3 digits of device id
    char sAddr[4];
    sAddr[0] = deviceId[5];
    sAddr[1] = deviceId[6];
    sAddr[2] = deviceId[7];
    sAddr[3] = '\0';

    return (uint8_t)atoi(sAddr);
}

int Chatter::getNumChannels () {
    return numChannels;
}

ChatterChannel* Chatter::getChannel (int channelNum) {
    return channels[channelNum];
}

ChatterChannel* Chatter::getDefaultChannel () {
    return getChannel(defaultChannel);
}

bool Chatter::hasMessage () {
    // Look across channels until we find a message
    for (int channelCount = 0; channelCount < numChannels; channelCount++) {
        if (channels[channelCount]->isConnected()) {
            updateChatStatus(channelCount, ChatConnected);
            if (channels[channelCount]->hasMessage()) {
                hotChannel = channels[channelCount];
                return true;
            }
        }
        else {
            updateChatStatus(channelCount, ChatDisconnected);
        }
    }
    return false;
}

void Chatter::populateReceiveBufferFlags () {
    // populates the flags in the receive buffer, using the content sitting in the receive buffer (should contain full header)
    // skip over HHH, nbf/na,  and device id
    //int flagPosition = 3 + 12*2 + CHATTER_DEVICE_ID_SIZE;//commented out 3/4 + receiveBuffer.headerLength;
    
    // new logic, 3/15 - maybe bridge has to skip header lenght
    int flagPosition = 3 + 12*2 + CHATTER_DEVICE_ID_SIZE;//commented out 3/4 + receiveBuffer.headerLength;
    if (mode == BridgeMode) {
        flagPosition += receiveBuffer.headerLength;
    }

    receiveBuffer.flags.Flag0 = receiveBuffer.content[flagPosition++];
    receiveBuffer.flags.Flag1 = receiveBuffer.content[flagPosition++];
    receiveBuffer.flags.Flag2 = receiveBuffer.content[flagPosition++];
    receiveBuffer.flags.Flag3 = receiveBuffer.content[flagPosition++];
    receiveBuffer.flags.Flag4 = receiveBuffer.content[flagPosition++];
    receiveBuffer.flags.Flag5 = receiveBuffer.content[flagPosition++];
}

void Chatter::ingestPacketMetadata (ChatterChannel* channel) {
    //logConsole("INgesting from: " + String(channel->getTextMessage()));
    ingestPacketMetadata(channel->getTextMessage(), channel->getMessageSize());
}

void Chatter::ingestPacketMetadata (const char* rawPacket, int messageSize) {
    receiveBuffer.rawMessage = rawPacket;
    //memcpy(receiveBuffer.rawMessage, rawPacket, messageSize);
    //receiveBuffer.rawMessage = channel->getTextMessage();
    int packetPosition = 0;
    for (int i = 0; i < CHATTER_DEVICE_ID_SIZE; i++) {
        receiveBuffer.recipient[i] = receiveBuffer.rawMessage[packetPosition++];
    }
    receiveBuffer.recipient[CHATTER_DEVICE_ID_SIZE] = '\0';

    for (int i = 0; i < CHATTER_DEVICE_ID_SIZE; i++) {
        receiveBuffer.sender[i] = receiveBuffer.rawMessage[packetPosition++];
    }
    receiveBuffer.sender[CHATTER_DEVICE_ID_SIZE] = '\0';

    receiveBuffer.encryptionFlag = (ChatterPacketEncryptionFlag)receiveBuffer.rawMessage[packetPosition++];

    for (int i = 0; i < CHATTER_MESSAGE_ID_SIZE; i++) {
        receiveBuffer.messageId[i] = receiveBuffer.rawMessage[packetPosition++];
    }
    receiveBuffer.messageId[CHATTER_MESSAGE_ID_SIZE] = '\0';

    receiveBuffer.packetId = 0;
    for (int i = 0; i < CHATTER_CHUNK_ID_SIZE; i++) {
        receiveBuffer.chunkId[i] = receiveBuffer.rawMessage[packetPosition++];
        receiveBuffer.packetId += (receiveBuffer.chunkId[i] - 48) * pow(10,((CHATTER_CHUNK_ID_SIZE-1) - i));
    }
    receiveBuffer.chunkId[CHATTER_CHUNK_ID_SIZE] = '\0';

    receiveBuffer.rawContentLength = messageSize;//channel->getMessageSize();
    receiveBuffer.contentLength = messageSize - receiveBuffer.headerLength;//channel->getMessageSize() - receiveBuffer.headerLength;

    if (mode == BridgeMode) {
        // copy the raw content into content for now
        memcpy(receiveBuffer.content, receiveBuffer.rawMessage, messageSize);
    }

    // if it's unencrypted and the first 3 letters of the raw message are 'HHH', this is a header record
    receiveBuffer.isMetadata = false;
    if (receiveBuffer.encryptionFlag == PacketClear) {
        if (receiveBuffer.contentLength == CHATTER_HEADER_SIZE) {
            if (memcmp(receiveBuffer.rawMessage + receiveBuffer.headerLength, "HHH", 3) == 0) {
                receiveBuffer.isMetadata = true;
            }
        }
    }
}

bool Chatter::retrieveMessage () {
    bool retrieved = hotChannel->retrieveMessage();
    if(retrieved) {
        ingestPacketMetadata(hotChannel);
        if (packetStore-> wasReceived ((const char*)receiveBuffer.sender, (const char*)receiveBuffer.messageId, receiveBuffer.packetId)) {
            //logConsole("received duplicate, ignoring...");
            return false;
        }
        else {
            Serial.print("Message for: ");Serial.println((const char*)receiveBuffer.recipient);

            // in bridge mode, we look at all packets.
            // otherwise, just direct and broadcast
            if (mode == BridgeMode || strcmp((const char*)receiveBuffer.recipient, deviceId) == 0 || strcmp((const char*) receiveBuffer.recipient, clusterBroadcastId) == 0) {
                //bool otherRecipient = strcmp((const char*)receiveBuffer.recipient, deviceId) != 0;

                /*logConsole("To: " + String((char*)receiveBuffer.recipient));
                logConsole("From: " + String((char*)receiveBuffer.sender));
                logConsole("Message ID: " + String((char*)receiveBuffer.messageId));
                logConsole("Chunk ID: " + String((char*)receiveBuffer.chunkId));
                logConsole("Length: " + String(receiveBuffer.rawContentLength));
                logConsole("Is Sig: " + String(receiveBuffer.encryptionFlag == PacketSigned));*/


                if (receiveBuffer.encryptionFlag == PacketEncrypted || receiveBuffer.encryptionFlag == PacketSigned) {

                    // in bridge mode or with a signature, we do not decrypt
                    if (mode == BridgeMode || receiveBuffer.encryptionFlag == PacketSigned) {
                        // just copy directly over, leave as is
                        for (int i = 0; i < receiveBuffer.rawContentLength; i++) {
                            this->receiveBuffer.content[i] = (uint8_t)receiveBuffer.rawMessage[i];
                        }
                        receiveBuffer.contentLength = receiveBuffer.rawContentLength;
                        receiveBuffer.content[receiveBuffer.rawContentLength] = '\0'; // null terminate
                    }
                    else { 
                        int encryptedLength = receiveBuffer.contentLength; // exclude header
                        uint8_t* encryptedMessage = (uint8_t*)(receiveBuffer.rawMessage + receiveBuffer.headerLength);

                        encryptor->decrypt(encryptedMessage, encryptedLength);
                        uint8_t* unencryptedBuffer = encryptor->getUnencryptedBuffer();
                        int unencryptedLen = encryptor->getUnencryptedBufferLength();

                        for (int i = 0; i < unencryptedLen; i++) {
                            this->receiveBuffer.content[i] = unencryptedBuffer[i];
                        }
                        receiveBuffer.contentLength = unencryptedLen;
                        receiveBuffer.content[unencryptedLen] = '\0'; // null terminate
                    }

                    // save the chatter packet
                    packetStore->savePacket(&receiveBuffer);
                    receiveBufferMessageType = receiveBuffer.encryptionFlag == PacketSigned ? MessageTypeSignature : MessageTypeText;


                    // if it was a signature, go ahead and copy the entire message into
                    // the receive buffer, so it will be there after sig is verified (or not)
                    //if (receiveBuffer.encryptionFlag == PacketSigned) {

                    // check if we have an entire message
                    if (isCompleteSignedMessage ((const char*)receiveBuffer.sender, (const char*)receiveBuffer.messageId)) {
                        // read the complete message into the buffer.
                        // The header will be there, which contains metadata to check expiry
                        receiveBuffer.contentLength = packetStore->readMessage((const char*)receiveBuffer.sender, (const char*)receiveBuffer.messageId, receiveBuffer.content, CHATTER_FULL_MESSAGE_BUFFER_SIZE);                    
                        
                        populateReceiveBufferFlags();
                        bool checkHash = mode != BridgeMode; // in bridge mode (no decryption) we cant check the hash, just the sig
                        if (isExpired() == false) {
                            // is this a device identity message
                            if (receiveBuffer.flags.Flag4 == FLAG_CTRL_TYPE_ID || receiveBuffer.flags.Flag4 == FLAG_CTRL_TYPE_EXCHANGE_ID) {
                                if (receiveDeviceInfo(receiveBuffer.flags.Flag4 == FLAG_CTRL_TYPE_EXCHANGE_ID)) {
                                    logConsole("Newly trusted device. Checking for quarantined messages...");

                                    // this message does not bubble up to the consumer at all, but 
                                    // if we have messages from this sender sitting in quarantine, they
                                    // can bubble up now
                                    return false;
                                }
                                else {
                                    logConsole("Received bad device info. Not trusting");
                                    return false;
                                }

                            } // do we know the sender
                            else if (isSenderKnown((const char*)receiveBuffer.sender)) {
                                if (validateSignature(checkHash)) {
                                    // remove the header from the content buffer so the consumer doesn't see it
                                    int actualContentLength = 0;
                                    for (int i = CHATTER_HEADER_SIZE; i < receiveBuffer.contentLength; i++) {
                                        receiveBuffer.content[i-CHATTER_HEADER_SIZE] = receiveBuffer.content[i];
                                        actualContentLength++;
                                    }

                                    receiveBuffer.content[actualContentLength] = '\0';
                                    receiveBuffer.contentLength = actualContentLength;
                                    receiveBufferMessageType = MessageTypeComplete;
                                }
                                else {
                                    logConsole("Invalid signature!");
                                    return false;
                                }
                            }
                            else {
                                logConsole("Sender is not trusted. Requesting key/license.");

                                // quarantine the message
                                packetStore->moveMessageToQuarantine((const char*)receiveBuffer.sender, (const char*)receiveBuffer.messageId);

                                // send request for device info
                                sendDeviceInfo((const char*)receiveBuffer.sender, true);

                                // the quarantined message should get processed after we have received trusted device info
                                return false;
                            }
                        }
                        else {
                            logConsole("Message expired!");
                            return false;
                        }
                    }

                    return retrieved;

                    // if it wasn't encrypted, fall through to normal unencrypted processing
                }
                else if (this->receiveBuffer.isMetadata) {
                    // this is an unencrypted header. save it so it can be part of the sig check
                    // bridge needs to keep the header portion, so it knows where the message goes.
                    // any other recipient needs to strip the header portion, since it only cares about the header content
                    if (mode != BridgeMode) {
                        memcpy(receiveBuffer.content, receiveBuffer.rawMessage + receiveBuffer.headerLength, receiveBuffer.contentLength);

                        // in case this device is the recipient, the header is not considered metadata for saving purposes.
                        // the way this is saved should probably be refactored a little
                        receiveBuffer.isMetadata = false;
                    }
                    receiveBufferMessageType = MessageTypeHeader;

                    // save the packet
                    return packetStore->savePacket(&receiveBuffer);
                }

                // this is an unencrypted payload packet
                // just save as is, it could be part of a pub key exchange
                logConsole("Received unencrypted payload packet, saving");
                memcpy(receiveBuffer.content, receiveBuffer.rawMessage + receiveBuffer.headerLength, receiveBuffer.contentLength);
                return packetStore->savePacket(&receiveBuffer);

                // if we get here, it means no encryption is expected
                /*const uint8_t* msg = hotChannel->getMessageType() == MessageTypeText ? ((uint8_t*)hotChannel->getTextMessage()) : hotChannel->getRawMessage();
                for (int i = 0; i < hotChannel->getMessageSize(); i++) {
                    this->receiveBuffer.content[i] = msg[i];
                }
                receiveBuffer.contentLength = hotChannel->getMessageSize();
                receiveBufferMessageType = hotChannel->getMessageType();*/
            }
            else {
                logConsole("This device (" + String(deviceId) + ") is not the receipient (" + String((char*)receiveBuffer.recipient) + "). Ignoring.");
            }
        }
    }
    else {
        logConsole("Message not retrieved from channel!");
    }

    return retrieved;
}

bool Chatter::isCompleteSignedMessage (const char* senderDeviceId, const char* messageId) {
    uint8_t numPackets = packetStore->getNumPackets(senderDeviceId, messageId);

    if (numPackets >= 3) {
        // check for footer
        if (packetStore->packetExists(senderDeviceId, messageId, 0)) {
            for (uint8_t p = 1; p < numPackets; p++) {
                if (!packetStore->packetExists(senderDeviceId, messageId, p)) {
                    logConsole("Gap found");
                    return false;
                }
            }

            // no gaps found
            // read header into send buffer, which should be unused at this point
            int headerPacketLength = packetStore->readPacket (senderDeviceId, messageId, numPackets - 1, sendBuffer, CHATTER_FULL_BUFFER_LEN); 
            int expectedSize = (mode == BridgeMode) ? (receiveBuffer.headerLength + CHATTER_HEADER_SIZE) : CHATTER_HEADER_SIZE;
            uint8_t* expectedLocation = (mode == BridgeMode) ? sendBuffer + receiveBuffer.headerLength : sendBuffer;

            // packet length is fixed, but depends on whether in bridge mode or basic mode
            if (headerPacketLength == expectedSize) {
                if (memcmp(expectedLocation, "HHH", 3) == 0) {
                    return true;
                }
                else {
                    logConsole("Header was right size but missing header text indicator (bridge mode)");
                }
            }
            else {
                logConsole("Header packet wrong size: " + String(headerPacketLength) + " vs expected " + String(expectedSize));
            }
        }
        else {
            logConsole("No footer found");
        }

    }
    else {
        logConsole("not enough packets");
    }

    return false;
}

ChatterChannel* Chatter::getLastChannel () {
    return hotChannel;
}

const char* Chatter::getLastSender () {
    return (const char*)receiveBuffer.sender;
}

const char* Chatter::getLastRecipient () {
    return (const char*)receiveBuffer.recipient;
}

bool Chatter::wasLastMessageBroadcast () {
    return memcmp(receiveBuffer.recipient, clusterBroadcastId, CHATTER_DEVICE_ID_SIZE) == 0;
}

ChatterMessageType Chatter::getMessageType () {
    return receiveBufferMessageType;
}

int Chatter::getMessageSize () {
    /*if (hotChannel->isEncrypted()) {
        return clearMessageSize;
    }*/
    return receiveBuffer.contentLength;
}

const char* Chatter::getTextMessage () {
    return (char*)receiveBuffer.content;
}

const uint8_t* Chatter::getRawMessage () {
    return receiveBuffer.content;
}


bool Chatter::broadcast (String message) {
    return broadcast((uint8_t*)message.c_str(), message.length());
}

bool Chatter::broadcast(uint8_t *message, uint8_t length) {
    return sendViaIntermediary(message, length, clusterBroadcastId, clusterBroadcastId, nullptr, getDefaultChannel(), false, true);
}

bool Chatter::broadcast(uint8_t *message, int length, ChatterMessageFlags* flags) {
    return sendViaIntermediary(message, length, clusterBroadcastId, clusterBroadcastId, flags, getDefaultChannel(), false, true);
}

bool Chatter::broadcastUnencrypted(uint8_t *message, int length, ChatterMessageFlags* flags) {
    return sendViaIntermediary(message, length, clusterBroadcastId, clusterBroadcastId, flags, getDefaultChannel(), true, true);
}


// gets a header ready for message sending
void Chatter::primeSendBuffer (const char* recipientDeviceId, ChatterChannel* channel, bool isSigned, bool isHeader, bool isFooter, char* messageId, char* chunkId, bool forceUnencrypted) {
    memset(this->sendBuffer, 0, CHATTER_FULL_BUFFER_LEN);

    int bufferPos = 0;
    // recipient
    for (int i = 0; i < CHATTER_DEVICE_ID_SIZE; i++) {
        sendBuffer[bufferPos++] = (uint8_t)recipientDeviceId[i];
    }

    for (int i = 0; i < CHATTER_DEVICE_ID_SIZE; i++) {
        sendBuffer[bufferPos++] = (uint8_t)deviceId[i];
    }

    // packet security flag
    if (forceUnencrypted) {
        sendBuffer[bufferPos++] = PacketClear;
    }    
    else if (isFooter) {
        sendBuffer[bufferPos++] = PacketSigned;
    }
    else if (isHeader) {
        sendBuffer[bufferPos++] = PacketClear;
    }
    else if (isSigned) {
        sendBuffer[bufferPos++] = PacketSigned;
    }
    else if (channel->isEncrypted()) {
        sendBuffer[bufferPos++] = PacketEncrypted;
    }
    else {
        sendBuffer[bufferPos++] = PacketClear;
    }

    // message/chunk
    for (int i = 0; i < CHATTER_MESSAGE_ID_SIZE; i++) {
        sendBuffer[bufferPos++] = messageId[i];
    }

    for (int i = 0; i < CHATTER_CHUNK_ID_SIZE; i++) {
        sendBuffer[bufferPos++] = chunkId[i];
    }
}

int Chatter::populateSendBufferContent (uint8_t* message, int length, ChatterChannel* channel, bool isMetadata, bool forceUnencrypted) {
    uint8_t* rawContentArea = sendBuffer + CHATTER_PACKET_HEADER_LENGTH;
    int fullMessageLength = length + CHATTER_PACKET_HEADER_LENGTH; // if encrypted, this length will change

    if (channel->isEncrypted() && isMetadata == false && forceUnencrypted == false) {
        // encrypt the message
        encryptor->encrypt((const char*)message, length);
        int encryptedSize = encryptor->getEncryptedBufferLength();

        for (int i = 0; i < encryptedSize; i++) {
            rawContentArea[i] = (uint8_t)encryptor->getEncryptedBuffer()[i];
        }

        fullMessageLength = encryptedSize + CHATTER_PACKET_HEADER_LENGTH;
    }
    else {
        // copy the cleartext Message
        for (int i = 0; i < length; i++) {
            rawContentArea[i] = (uint8_t)message[i];
        }
    }

    return fullMessageLength;
}

// assumes header buffer has been populated already
int Chatter::generateFooter (const char* recipientDeviceId, char* messageId, uint8_t* message, int messageLength) {
    // footer = footer hash (32 bytes) + sig (64 bytes)

    memset(fullSignBuffer, 0, CHATTER_HEADER_SIZE + CHATTER_FULL_MESSAGE_BUFFER_SIZE);
    memcpy(fullSignBuffer, headerBuffer, CHATTER_HEADER_SIZE);
    memcpy(fullSignBuffer+(CHATTER_HEADER_SIZE), message, messageLength);

    /*Serial.println("Sig base " + String(CHATTER_HEADER_SIZE + messageLength) + " bytes: ");
    for (int i = 0; i < CHATTER_HEADER_SIZE + messageLength; i++) {
        Serial.print((char)fullSignBuffer[i]);
    }
    Serial.println("");*/

    // hash of message, 32 bytes
    int hashSize = encryptor->generateHash((char*)fullSignBuffer, (CHATTER_HEADER_SIZE) + messageLength, hashBuffer);

    int footerIndex = 0;
    // add hash
    for (int i = 0; i < hashSize; i++) {
        footerBuffer[footerIndex++] = hashBuffer[i];
    }

    // Add sig
    encryptor->setMessageBuffer(hashBuffer);
    encryptor->signMessage();
    byte* sig = encryptor->getSignatureBuffer();
    for (int i = 0; i < CHATTER_SIGNATURE_LENGTH; i++) {
        footerBuffer[footerIndex++] = sig[i];
    }

    // terminate the buffer repeatedly until we fill the footer buffer
    while (footerIndex < CHATTER_FOOTER_BUFFER_SIZE) {
        footerBuffer[footerIndex++] = '\0';
    }
    /*Serial.print("Full Footer (" + String(footerIndex) + "): ");
    for (int i = 0; i < CHATTER_FOOTER_SIZE; i++) {
        Serial.print((char)footerBuffer[i]);
    }
    Serial.println("");*/
    return CHATTER_FOOTER_SIZE;
}

int Chatter::generateHeader (const char* recipientDeviceId, char* messageId, ChatterMessageFlags* flags) {
    // header = [nbf][naf][recipient][reserved*6][rand*16]
    uint8_t headerPosition = 0;

    memset(headerBuffer,0,CHATTER_HEADER_BUFFER_SIZE);
    memset(headerBuffer,'H',3);
    headerPosition += 3;

    // nbf
    memcpy(headerBuffer + headerPosition, rtc->getSortableTimePlusSeconds(CHATTER_EXPIRY_NBF_SECONDS), CHATTER_EXPIRY_DATE_LENGTH);
    headerPosition += CHATTER_EXPIRY_DATE_LENGTH;

    // na
    memcpy(headerBuffer + headerPosition, rtc->getSortableTimePlusSeconds(CHATTER_EXPIRY_NAF_SECONDS), CHATTER_EXPIRY_DATE_LENGTH);
    headerPosition += CHATTER_EXPIRY_DATE_LENGTH;

    // recipient
    memcpy(headerBuffer + headerPosition, recipientDeviceId, CHATTER_DEVICE_ID_SIZE);
    headerPosition += CHATTER_DEVICE_ID_SIZE;

    // 6 flags, or 0 in all of them, if not set
    if (flags == nullptr) {
        // flags should never be set to 0 or they interfere with things further down line.
        memset(headerBuffer + headerPosition, 1, 6);
        headerPosition += 6;
    }
    else {
        headerBuffer[headerPosition++] = flags->Flag0;
        headerBuffer[headerPosition++] = flags->Flag1;
        headerBuffer[headerPosition++] = flags->Flag2;
        headerBuffer[headerPosition++] = flags->Flag3;
        headerBuffer[headerPosition++] = flags->Flag4;
        headerBuffer[headerPosition++] = flags->Flag5;
    }

    // 14 rand
    for (int i = 0; i < 14; i++) {
        headerBuffer[headerPosition] = (uint8_t)encryptor->getRandom();
        headerPosition++;
    }

    // terminate the buffer repeatedly until we fill the header buffer
    while (headerPosition < CHATTER_HEADER_BUFFER_SIZE) {
        headerBuffer[headerPosition++] = '\0';
    }

    //logConsole("Full Header (" + String(CHATTER_HEADER_SIZE) + "): ");
    /*for (int i = 0; i < CHATTER_HEADER_SIZE; i++) {
        Serial.print((char)headerBuffer[i]);
    }
    Serial.println("");*/
    return CHATTER_HEADER_SIZE;
}

bool Chatter::send(uint8_t *message, int length, const char* recipientDeviceId) {
    return sendViaIntermediary(message, length, recipientDeviceId, recipientDeviceId, nullptr, getDefaultChannel());
}

bool Chatter::send(uint8_t *message, int length, const char* recipientDeviceId, ChatterChannel* channel) {
    return sendViaIntermediary(message, length, recipientDeviceId, recipientDeviceId, nullptr, channel);
}

bool Chatter::send(uint8_t *message, int length, const char* recipientDeviceId, ChatterMessageFlags* flags) {
    return sendViaIntermediary(message, length, recipientDeviceId, recipientDeviceId, flags, getDefaultChannel());
}

bool Chatter::sendUnencrypted(uint8_t *message, int length, const char* recipientDeviceId, ChatterMessageFlags* flags) {
    return sendViaIntermediary(message, length, recipientDeviceId, recipientDeviceId, flags, getDefaultChannel(), true);
}

bool Chatter::sendViaIntermediary(uint8_t *message, int length, const char* recipientDeviceId, const char* intermediaryDeviceId) {
    return sendViaIntermediary(message, length, recipientDeviceId, intermediaryDeviceId, nullptr, getDefaultChannel());
}

bool Chatter::sendViaIntermediary(uint8_t *message, int length, const char* recipientDeviceId, const char* intermediaryDeviceId, ChatterMessageFlags* flags) {
    return sendViaIntermediary(message, length, recipientDeviceId, intermediaryDeviceId, flags, getDefaultChannel());
}

bool Chatter::send(uint8_t *message, int length, const char* recipientDeviceId, ChatterMessageFlags* flags, ChatterChannel* channel) {
    return sendViaIntermediary(message, length, recipientDeviceId, recipientDeviceId, flags, channel);
}

bool Chatter::sendViaIntermediary(uint8_t *message, int length, const char* recipientDeviceId, const char* intermediaryDeviceId, ChatterMessageFlags* flags, ChatterChannel* channel, bool forceUnencrypted, bool isBroadcast) {
    if (forceUnencrypted) {
        logConsole("Warning: Unencrypted send!");
    }

    uint8_t address = channel->getAddress(intermediaryDeviceId);
    if (strcmp(recipientDeviceId, intermediaryDeviceId) != 0) {
        logConsole("Sending through intermediary: " + String(intermediaryDeviceId) + " @ " + String(address));
    }
    else {
        logConsole("Not sending through intermediary.");
    }

    char messageId[CHATTER_MESSAGE_ID_SIZE + 1];
    generateMessageId(messageId);

    Serial.print("MessageID: "); Serial.println(messageId);

    // this does not take into account that encryption can change size of message
    int contentChunkSize = CHATTER_PACKET_SIZE - CHATTER_PACKET_HEADER_LENGTH;
    int chunks = ceil(length / ((float)CHATTER_PACKET_SIZE - (float)CHATTER_PACKET_HEADER_LENGTH));
    int lastChunkSize = length % contentChunkSize;

    chunks += 1; // header

    logConsole("Message Length: " + String(length) + ", Content chunk size: " + String(contentChunkSize) + ", Chunks: " + String(chunks) + ", Last chunk: " + String(lastChunkSize));
    
    // send the header first, unencrypted
    char chunkId[CHATTER_CHUNK_ID_SIZE];
    chunkId[CHATTER_CHUNK_ID_SIZE - 1] = '\0';
    sprintf(chunkId, "%03d", chunks);
    primeSendBuffer (recipientDeviceId, channel, false, true, false, messageId, chunkId, forceUnencrypted);
    int thisHeaderLength = generateHeader(recipientDeviceId, messageId, flags);
    int fullMetadataLength = populateSendBufferContent (headerBuffer, thisHeaderLength, channel, true, forceUnencrypted);

    if (isBroadcast) {
        if (!channel->broadcast(sendBuffer, fullMetadataLength)) {
            logConsole("Header broadcast failed");
            return false;
        }
    }
    else {
        if (!channel->send(sendBuffer, fullMetadataLength, address)) {
            logConsole("Header send failed");
            return false;
        }
    }

    // now send message chunks
    for (int chunkNum = 1; chunkNum < chunks; chunkNum++) {
        // notice 03 is hardcoded, if chunk id size changes from 3, need to change this
        sprintf(chunkId, "%03d", chunks - chunkNum);        

        // prime with a header
        primeSendBuffer (recipientDeviceId, channel, false, false, false, messageId, chunkId, forceUnencrypted);

        // do any encoding/encryption necessary on the content and get the final length
        // because chunk 1 was the header, the message position is offset by message + (chunkNum-1)*contentChunkSize
        int thisChunkLength = (chunkNum + 1 < chunks) ? contentChunkSize : lastChunkSize;
        uint8_t* messagePosition = message + ((chunkNum-1) * contentChunkSize);

        //logConsole("next chunk starts at position: "  + String(chunkNum * contentChunkSize) + " of " + String(length));
        int finalFullLength = populateSendBufferContent (messagePosition, thisChunkLength, channel, false, forceUnencrypted);

        // send it
        if (isBroadcast) {
            if (!channel->broadcast(sendBuffer, finalFullLength)) {
                logConsole("Failed to broadcast chunk " + String(chunkId) + " length " + String(finalFullLength));
                return false;
            }
        }
        else {
            if (!channel->send(sendBuffer, finalFullLength, address)) {
                logConsole("Failed to send chunk " + String(chunkId) + " length " + String(finalFullLength));
                return false;
            }
        }
    }

    // if signing, chunk0 will be the signature
    if (channel->isSigned()) {
        //encryptor->setMessageBuffer
        int thisFooterLength = generateFooter(recipientDeviceId, messageId, message, length);

        // prime with a header
        primeSendBuffer (recipientDeviceId, channel, true, false, true, messageId, "000", forceUnencrypted);

        int finalFullLength = populateSendBufferContent (footerBuffer, thisFooterLength, channel, true, forceUnencrypted); // don't encrypt footer, so sig can be validated
        if (isBroadcast) {
            if (!channel->broadcast(sendBuffer, finalFullLength)) {
                return false;
            }
            logConsole(String("Successfully broadcast signed message: ") + String(messageId));
        }
        else {
            if (!channel->send(sendBuffer, finalFullLength, address)) {
                return false;
            }
            logConsole(String("Successfully sent signed message: ") + String(messageId));
        }
    }
    else {
        logConsole("Message sent/broadcast with no signature.");
    }

    return true;
}

bool Chatter::isRootDevice (const char* deviceId) {
    // returnt true if the given device is 000 of the current cluster
    return memcmp(deviceId, clusterId, CHATTER_GLOBAL_NET_ID_SIZE + CHATTER_LOCAL_NET_ID_SIZE) == 0 && memcmp(deviceId + CHATTER_GLOBAL_NET_ID_SIZE + CHATTER_LOCAL_NET_ID_SIZE, "000", 3) == 0;
}

bool Chatter::receiveDeviceInfo (bool isExchange) {
    // device info should be sitting in our message buffer. parse it and check it
    // license should be signed appropriately

    // trim out the header
    int actualContentLength = 0;
    for (int i = CHATTER_HEADER_SIZE; i < receiveBuffer.contentLength; i++) {
        receiveBuffer.content[i-CHATTER_HEADER_SIZE] = receiveBuffer.content[i];
        actualContentLength++;
    }
    if (actualContentLength >= ENC_PUB_KEY_SIZE + CHATTER_DEVICE_ID_SIZE + ENC_SIGNATURE_SIZE + CHATTER_ALIAS_NAME_SIZE) {
        int fullFixedSize = ENC_PUB_KEY_SIZE + CHATTER_DEVICE_ID_SIZE + (ENC_SIGNATURE_SIZE*2) + CHATTER_ALIAS_NAME_SIZE;
        // pull out the pieces
        memcpy(pubKeyBuffer, receiveBuffer.content, ENC_PUB_KEY_SIZE);
        memcpy(deviceAliasBuffer, receiveBuffer.content + ENC_PUB_KEY_SIZE, CHATTER_ALIAS_NAME_SIZE);

        memcpy(licenseSigner, receiveBuffer.content + ENC_PUB_KEY_SIZE + CHATTER_ALIAS_NAME_SIZE, CHATTER_DEVICE_ID_SIZE);

        // dehex the sig into the license buffer
        encryptor->hexCharacterStringToBytesMax(licenseBuffer, (const char*)receiveBuffer.content + ENC_PUB_KEY_SIZE + CHATTER_DEVICE_ID_SIZE, ENC_SIGNATURE_SIZE * 2, ENC_SIGNATURE_SIZE);

        memset(clusterAliasBuffer, 0, CHATTER_ALIAS_NAME_SIZE);
        memcpy(clusterAliasBuffer, receiveBuffer.content + fullFixedSize, actualContentLength - fullFixedSize);

        // if the network requires root sig, the signer must be root
        if (!isRootDevice(licenseSigner)) {
            if(clusterStore->getLicenseType(clusterId) == ClusterLicenseRoot) {
                logConsole("Devices must be licensed by root on this network");
                return false;
            }
            else if (!trustStore->loadPublicKey(licenseSigner, encryptor->getPublicKeyBuffer())) {
                // we must know this signer, or we can't proceed
                logConsole("Non-Root license are OK ont the network, but signer is not known to this device");
                return false;
            }
        }

        // Load the signer's key
        if(trustStore->loadPublicKey(licenseSigner, encryptor->getPublicKeyBuffer())) {

            // hash the provided pub key and alias
            memset(fullSignBuffer, 0, ENC_PUB_KEY_SIZE + CHATTER_ALIAS_NAME_SIZE);
            memcpy(fullSignBuffer, pubKeyBuffer, ENC_PUB_KEY_SIZE);
            memcpy(fullSignBuffer, deviceAliasBuffer, CHATTER_ALIAS_NAME_SIZE);
            int hashLength = encryptor->generateHash((const char*)fullSignBuffer, ENC_PUB_KEY_SIZE + CHATTER_ALIAS_NAME_SIZE, encryptor->getMessageBuffer());

            // check the sig
            encryptor->setSignatureBuffer(licenseBuffer);
            if (encryptor->verify()) {
                logConsole("Received valid license for device!");

                // add it to storage
                if(trustStore->addTrustedDevice ((const char*)receiveBuffer.sender, deviceAliasBuffer, pubKeyBuffer, true)) {
                    logConsole("Device now trusted.");
                    logConsole("Alias: ");
                    logConsole(deviceAliasBuffer);
                }

                // send our own, if this is an exchange
                if (isExchange) {
                    sendDeviceInfo((const char*)receiveBuffer.sender, false);
                }

                return true;
            }
        }
    }
    else {
        logConsole("Device info message size: " + String(actualContentLength) + "is not the correct size ");
    }

    return false;
}

bool Chatter::broadcastDeviceInfo (bool requestBack) {
    // Broadcast own key/license, with flag set to receive theirs
    logConsole("broadcasting device info");

    ChatterMessageFlags flg;
    flg.Flag4 = requestBack ? FLAG_CTRL_TYPE_EXCHANGE_ID : FLAG_CTRL_TYPE_ID;

    if (prepareBuffersForSendDeviceInfo()) {
        // dont encrypt, because it could be bridge that receives, which doesnt store encryption key
        return broadcastUnencrypted(internalMessageBuffer, ENC_PUB_KEY_SIZE + CHATTER_DEVICE_ID_SIZE + (ENC_SIGNATURE_SIZE*2) + strlen(clusterAlias), &flg);
    }

    logConsole("Failed to prepare for send device info");
    return false;
}

bool Chatter::sendDeviceInfo (const char* targetDeviceId, bool requestBack) {
    // Send our own key/license, with flag set to receive theirs
    logConsole("Sending device info");

    ChatterMessageFlags flg;
    flg.Flag4 = requestBack ? FLAG_CTRL_TYPE_EXCHANGE_ID : FLAG_CTRL_TYPE_ID;

    if (prepareBuffersForSendDeviceInfo()) {
        // dont encrypt, because it could be bridge that receives, which doesnt store encryption key
        return sendUnencrypted(internalMessageBuffer, ENC_PUB_KEY_SIZE + CHATTER_DEVICE_ID_SIZE + (ENC_SIGNATURE_SIZE*2) + strlen(clusterAlias), targetDeviceId, &flg);
    }

    logConsole("Failed to prepare for send device info");
    return false;
}

bool Chatter::prepareBuffersForSendDeviceInfo () {
    // load pub key into buffer
    hsm->loadPublicKey(pubKeyBuffer);

    if (isRootDevice(deviceId)) {
        // use the full sign buffer to hash things
        memset(fullSignBuffer, 0, ENC_PUB_KEY_SIZE + CHATTER_ALIAS_NAME_SIZE);
        memcpy(fullSignBuffer, pubKeyBuffer, ENC_PUB_KEY_SIZE);
        memcpy(fullSignBuffer + ENC_PUB_KEY_SIZE, deviceAlias, strlen(deviceAlias));

        // sha256 this device's own pub key
        int hashLength = encryptor->generateHash((const char*)fullSignBuffer, ENC_PUB_KEY_SIZE + CHATTER_ALIAS_NAME_SIZE, encryptor->getMessageBuffer());
        
        // sign it
        if (encryptor->signMessage()) {
            memcpy(licenseBuffer, encryptor->getSignatureBuffer(), ENC_SIGNATURE_SIZE);
            memcpy(licenseSigner, deviceId, CHATTER_DEVICE_ID_SIZE);
            logConsole("Providing self-signed license.");
        }
        else {
            logConsole("Sign of own key failed.");
            return false;
        }
    }
    else {
        // load our license from storage
        licenseStore->loadLicense(clusterId, licenseBuffer);
        licenseStore->loadSignerId(clusterId, licenseSigner);
        logConsole("Sending root-signed license");
    }

    memset(internalMessageBuffer, 0, CHATTER_INTERNAL_MESSAGE_BUFFER_SIZE);
    memcpy(internalMessageBuffer, pubKeyBuffer, ENC_PUB_KEY_SIZE);
    memcpy(internalMessageBuffer, deviceAlias, strlen(deviceAlias));
    memcpy(internalMessageBuffer + ENC_PUB_KEY_SIZE + CHATTER_ALIAS_NAME_SIZE, licenseSigner, CHATTER_DEVICE_ID_SIZE);

    // hex the sig (it likes to have null byte sequences, which messes things up on the other end sometimes)
    encryptor->hexify(licenseBuffer, ENC_SIGNATURE_SIZE);

    memcpy(internalMessageBuffer + ENC_PUB_KEY_SIZE + CHATTER_DEVICE_ID_SIZE, encryptor->getHexBuffer(), ENC_SIGNATURE_SIZE*2);
    memcpy(internalMessageBuffer + ENC_PUB_KEY_SIZE + CHATTER_DEVICE_ID_SIZE + (ENC_SIGNATURE_SIZE*2), clusterAlias, strlen(clusterAlias));

    return true;
}

// chatter packet level
const uint8_t* Chatter::getLastPacketMessageId () {
    return receiveBuffer.messageId;
}

const uint8_t* Chatter::getLastPacketChunkId () {
    return receiveBuffer.chunkId;
}

ChatterPacketEncryptionFlag Chatter::getLastPacketEncryptionFlag () {
    return receiveBuffer.encryptionFlag;
}


void Chatter::logConsole(String msg) {
  if (CHAT_LOG_ENABLED) {
    Serial.println(msg);
  }
}

void Chatter::clearEncryptionKeyBuffer() {
    memset(framEncryptionKey, 0, 33);
    framEncryptionKeySize = 0;
}

bool Chatter::setupFramEncryptionKey (const char* devicePassword) {
    clearEncryptionKeyBuffer();

    // setup encryption key, default to unique device id if no password is set
    if (devicePassword != nullptr && strlen(devicePassword) > 0) {
        framEncryptionKeySize = strlen(devicePassword);
        memcpy(framEncryptionKey, devicePassword, framEncryptionKeySize);
        return true;
    }

    // no encryption key provided, default to device id until user hopefully sets a password later
    uint8_t rawId[16];

#if defined(ARDUINO_UNOR4_WIFI)
    memcpy(rawId, "replacewithrfid0", 16);
#elif !defined(ARDUINO_UNOR4_WIFI)
    framEncryptionKeySize = min(16, UniqueIDsize);
    memcpy(rawId, UniqueID, framEncryptionKeySize);
#endif
    // hexify the unique id
    uint8_t appended = 0;
    while (framEncryptionKeySize < 16) {
        // copy earlier part of device id into raw
        rawId[framEncryptionKeySize++] = rawId[appended++];
    }
    Encryptor::hexify(framEncryptionKey, rawId, 16);
    framEncryptionKeySize = 32;

    return true;
}

void Chatter::logDebugInfo () {
    fram->logCache();
}

bool Chatter::loadClusterConfig (const char* newClusterId) {
    if (clusterStore->init()) {
        logConsole("Joining cluster: ");
        logConsole(newClusterId);
        if (clusterStore->loadDeviceId(newClusterId, deviceId)) {
            memcpy(clusterId, newClusterId, CHATTER_GLOBAL_NET_ID_SIZE + CHATTER_LOCAL_NET_ID_SIZE);
            clusterId[CHATTER_GLOBAL_NET_ID_SIZE + CHATTER_LOCAL_NET_ID_SIZE] = 0;
            memset(clusterAlias, 0, CHATTER_ALIAS_NAME_SIZE + 1);
            deviceId[CHATTER_DEVICE_ID_SIZE] = 0;

            memset(clusterBroadcastId, 0, CHATTER_DEVICE_ID_SIZE+1);
            sprintf(clusterBroadcastId, "%s%s", clusterId, CHATTER_BROADCAST_ID);
            clusterStore->loadAlias(clusterId, clusterAlias);
            clusterStore->loadWifiCred(clusterId, wifiCred);
            clusterStore->loadWifiSsid(clusterId, wifiSsid);
            clusterPreferredChannel = clusterStore->getPreferredChannel(clusterId);
            clusterSecondaryChannel = clusterStore->getSecondaryChannel(clusterId);
            loraFrequency = clusterStore->getFrequency(clusterId);
            logConsole("Cluster configured: ");
            logConsole(clusterId);
            return true;
        }
        else {
            logConsole("Cluster not found");
        }
    }
    else {
        logConsole("Cluster store not initialized");
    }
    return false;
}

bool Chatter::deviceStoreInitialized () {
    if (deviceStore->init()) {
        memset(deviceAlias, 0, CHATTER_ALIAS_NAME_SIZE+1);
        // check if we already have a device id
        if (deviceStore->loadDeviceName (deviceAlias)) {
            deviceInitialized = true;
            logConsole("Device Alias: ");
            logConsole(deviceAlias);

            memset(defaultClusterId, 0, STORAGE_GLOBAL_NET_ID_SIZE + STORAGE_LOCAL_NET_ID_SIZE + 1);
            return deviceStore->getDefaultClusterId(defaultClusterId);
        }
        else {
            logConsole("New device");
        }
    }
    else {
        logConsole("device store init failed.");
    }
    return false;
}

bool Chatter::factoryReset () {
    // clearing all fram completes the reset
    return fram->clearAllZones();
}
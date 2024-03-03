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

            if(deviceStoreInitialized ()) {
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
        lastAnnounce = millis();
        uint8_t loraAddress = this->getChannel(0)->getSelfAddress();
        uint8_t udpAddress = this->getChannel(1)->getSelfAddress();
        String announceMsg = String(this->getDeviceId()) + "_" + String(loraAddress) + "_" + String(udpAddress);
        logConsole("Self Announcing: " + announceMsg);

        for (int channelCount = 0; channelCount < numChannels; channelCount++) {
            if (channels[channelCount]->isEnabled() && channels[channelCount]->isConnected()) {
                channels[channelCount]->broadcast(announceMsg);
            }
        }
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
    int flagPosition = 3 + 12*2 + CHATTER_DEVICE_ID_SIZE + receiveBuffer.headerLength;

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
            // in bridge mode, we look at all packets
            if (mode == BridgeMode || strcmp((const char*)receiveBuffer.recipient, deviceId) == 0) {
                bool otherRecipient = strcmp((const char*)receiveBuffer.recipient, deviceId) != 0;
                if (receiveBuffer.encryptionFlag == PacketEncrypted || receiveBuffer.encryptionFlag == PacketSigned) {

                    /*logConsole("To: " + String((char*)receiveBuffer.recipient));
                    logConsole("From: " + String((char*)receiveBuffer.sender));
                    logConsole("Message ID: " + String((char*)receiveBuffer.messageId));
                    logConsole("Chunk ID: " + String((char*)receiveBuffer.chunkId));
                    logConsole("Length: " + String(receiveBuffer.rawContentLength));
                    logConsole("Is Sig: " + String(receiveBuffer.encryptionFlag == PacketSigned));*/

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
                    if (receiveBuffer.encryptionFlag == PacketSigned) {
                        
                        // read the complete message into the buffer.
                        // The header will be there, which contains metadata to check expiry
                        receiveBuffer.contentLength = packetStore->readMessage((const char*)receiveBuffer.sender, (const char*)receiveBuffer.messageId, receiveBuffer.content, CHATTER_FULL_MESSAGE_BUFFER_SIZE);                    
                        
                        populateReceiveBufferFlags();
                        bool checkHash = mode != BridgeMode; // in bridge mode (no decryption) we cant check the hash, just the sig
                        if (isExpired() == false) {
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

                logConsole("Received raw looking packet (not saving) of length: " + String(receiveBuffer.contentLength));
                // if we get here, it means no encryption is expected
                const uint8_t* msg = hotChannel->getMessageType() == MessageTypeText ? ((uint8_t*)hotChannel->getTextMessage()) : hotChannel->getRawMessage();
                for (int i = 0; i < hotChannel->getMessageSize(); i++) {
                    this->receiveBuffer.content[i] = msg[i];
                }
                receiveBuffer.contentLength = hotChannel->getMessageSize();
                receiveBufferMessageType = hotChannel->getMessageType();
            }
            else {
                logConsole("This device (" + String(deviceId) + ") is not the receipient (" + String((char*)receiveBuffer.recipient) + "). Ignoring.");
            }
        }
    }
    else {
        logConsole("Message not retrieved!");
    }

    return retrieved;
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
    ChatterChannel* channel = getDefaultChannel();

    // broadcast to own network
    char broadcastAddress[CHATTER_DEVICE_ID_SIZE + 1];
    memcpy(broadcastAddress, this->getDeviceId(), CHATTER_DEVICE_ID_SIZE - 3);
    memcpy(broadcastAddress + (CHATTER_DEVICE_ID_SIZE - 3), CHATTER_BROADCAST_ID, 3);
    broadcastAddress[CHATTER_DEVICE_ID_SIZE] = '\0';

    // prime with a header
    primeSendBuffer (CHATTER_BROADCAST_ID, channel, false, false, false, "ABC", "001");

    // do any encoding/encryption necessary on the content and get the final length
    int finalFullLength = populateSendBufferContent (message, length, channel, false);

    return channel->broadcast(sendBuffer, finalFullLength);
}


// gets a header ready for message sending
void Chatter::primeSendBuffer (const char* recipientDeviceId, ChatterChannel* channel, bool isSigned, bool isHeader, bool isFooter, char* messageId, char* chunkId) {
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
    if (isFooter) {
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

int Chatter::populateSendBufferContent (uint8_t* message, int length, ChatterChannel* channel, bool isMetadata) {
    uint8_t* rawContentArea = sendBuffer + CHATTER_PACKET_HEADER_LENGTH;
    int fullMessageLength = length + CHATTER_PACKET_HEADER_LENGTH; // if encrypted, this length will change

    if (channel->isEncrypted() && isMetadata == false) {
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

    Serial.println("Sig base: ");
    for (int i = 0; i < CHATTER_HEADER_SIZE + messageLength; i++) {
        Serial.print((char)fullSignBuffer[i]);
    }
    Serial.println("");

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

bool Chatter::sendViaIntermediary(uint8_t *message, int length, const char* recipientDeviceId, const char* intermediaryDeviceId) {
    return sendViaIntermediary(message, length, recipientDeviceId, intermediaryDeviceId, nullptr, getDefaultChannel());
}

bool Chatter::sendViaIntermediary(uint8_t *message, int length, const char* recipientDeviceId, const char* intermediaryDeviceId, ChatterMessageFlags* flags) {
    return sendViaIntermediary(message, length, recipientDeviceId, intermediaryDeviceId, flags, getDefaultChannel());
}

bool Chatter::send(uint8_t *message, int length, const char* recipientDeviceId, ChatterMessageFlags* flags, ChatterChannel* channel) {
    return sendViaIntermediary(message, length, recipientDeviceId, recipientDeviceId, flags, channel);
}

bool Chatter::sendViaIntermediary(uint8_t *message, int length, const char* recipientDeviceId, const char* intermediaryDeviceId, ChatterMessageFlags* flags, ChatterChannel* channel) {
    uint8_t address = channel->getAddress(intermediaryDeviceId);
    if (strcmp(recipientDeviceId, intermediaryDeviceId) != 0) {
        logConsole("Sending through intermediary: " + String(intermediaryDeviceId) + " @ " + String(address));
    }
    else {
        logConsole("Not sending through intermediary.");
    }

    char messageId[CHATTER_MESSAGE_ID_SIZE + 1];
    generateMessageId(messageId);

    // this does not take into account that encryption can change size of message
    int contentChunkSize = CHATTER_PACKET_SIZE - CHATTER_PACKET_HEADER_LENGTH;
    int chunks = ceil(length / ((float)CHATTER_PACKET_SIZE - (float)CHATTER_PACKET_HEADER_LENGTH));
    int lastChunkSize = length % contentChunkSize;

    chunks += 1; // header

    //logConsole("Message Length: " + String(length) + ", Content chunk size: " + String(contentChunkSize) + ", Chunks: " + String(chunks) + ", Last chunk: " + String(lastChunkSize));
    
    // send the header first, unencrypted
    char chunkId[CHATTER_CHUNK_ID_SIZE];
    chunkId[CHATTER_CHUNK_ID_SIZE - 1] = '\0';
    sprintf(chunkId, "%03d", chunks);
    primeSendBuffer (recipientDeviceId, channel, false, true, false, messageId, chunkId);
    int thisHeaderLength = generateHeader(recipientDeviceId, messageId, flags);
    int fullMetadataLength = populateSendBufferContent (headerBuffer, thisHeaderLength, channel, true);
    if (!channel->send(sendBuffer, fullMetadataLength, address)) {
        logConsole("Header send failed");
        return false;
    }

    // now send message chunks
    for (int chunkNum = 1; chunkNum < chunks; chunkNum++) {
        // notice 03 is hardcoded, if chunk id size changes from 3, need to change this
        sprintf(chunkId, "%03d", chunks - chunkNum);        

        // prime with a header
        primeSendBuffer (recipientDeviceId, channel, false, false, false, messageId, chunkId);

        // do any encoding/encryption necessary on the content and get the final length
        // because chunk 1 was the header, the message position is offset by message + (chunkNum-1)*contentChunkSize
        int thisChunkLength = (chunkNum + 1 < chunks) ? contentChunkSize : lastChunkSize;
        uint8_t* messagePosition = message + ((chunkNum-1) * contentChunkSize);

        //logConsole("next chunk starts at position: "  + String(chunkNum * contentChunkSize) + " of " + String(length));
        int finalFullLength = populateSendBufferContent (messagePosition, thisChunkLength, channel, false);

        // send it
        if (!channel->send(sendBuffer, finalFullLength, address)) {
            logConsole("Failed to send chunk " + String(chunkId) + " length " + String(finalFullLength));
            return false;
        }
    }

    // if signing, chunk0 will be the signature
    if (channel->isSigned()) {
        //encryptor->setMessageBuffer
        int thisFooterLength = generateFooter(recipientDeviceId, messageId, message, length);

        // prime with a header
        primeSendBuffer (recipientDeviceId, channel, true, false, true, messageId, "000");

        int finalFullLength = populateSendBufferContent (footerBuffer, thisFooterLength, channel, true); // don't encrypt footer, so sig can be validated
        if (!channel->send(sendBuffer, finalFullLength, address)) {
            return false;
        }
        logConsole(String("Successfully sent message: ") + String(messageId));
    }

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
            deviceId[CHATTER_DEVICE_ID_SIZE] = 0;
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
        // check if we already have a device id
        if (deviceStore->loadDeviceName (deviceId)) {
            logConsole("Device: ");
            logConsole(deviceId);

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
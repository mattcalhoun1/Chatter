#include "LoRaChannel.h"

LoRaChannel::LoRaChannel (uint8_t channelNum, float frequency, uint8_t selfAddress, int csPin, int intPin, int rsPin, bool logEnabled, ChatStatusCallback* chatStatusCallback) {
    this->channelNum = channelNum;
    lora = new LoRaTrans(frequency, selfAddress, csPin, intPin, rsPin, logEnabled);
    this->chatStatusCallback = chatStatusCallback;
}

bool LoRaChannel::init() {
    chatStatusCallback->updateChatStatus(channelNum, ChatConnecting);
    bool result = lora->init();
    chatStatusCallback->updateChatStatus(channelNum, result ? ChatConnected : ChatDisconnected);

    return result;
}

bool LoRaChannel::hasMessage () {
    return lora->hasMessage();
}

bool LoRaChannel::retrieveMessage () {
    chatStatusCallback->updateChatStatus(channelNum, ChatReceiving);
    bool result = lora->retrieveMessage();
    chatStatusCallback->updateChatStatus(channelNum, result ? ChatReceived : ChatFailed);

    return result;
}

uint8_t LoRaChannel::getSelfAddress () {
  return lora->getSelfAddress();
}

uint8_t LoRaChannel::getAddress (const char* otherDeviceId) {
    // address is numeric last 3 digits of device id
    char sAddr[4];
    sAddr[0] = otherDeviceId[5];
    sAddr[1] = otherDeviceId[6];
    sAddr[2] = otherDeviceId[7];
    sAddr[3] = '\0';

    return (uint8_t)atoi(sAddr);

  return LORA_ADDR_BROADCAST;
}

uint8_t LoRaChannel::getLastSender () {
    return lora->getLastSender();
}

ChatterMessageType LoRaChannel::getMessageType () {
    return MessageTypeText;
}

int LoRaChannel::getMessageSize () {
    return lora->getLastMessageSize();
}

const char* LoRaChannel::getTextMessage () {
    return (char*)lora->getMessageBuffer();
}

const uint8_t* LoRaChannel::getRawMessage () {
    return lora->getMessageBuffer();
}


bool LoRaChannel::broadcast (String message) {
    chatStatusCallback->updateChatStatus(channelNum, ChatSending);
    bool result = lora->broadcast(message);
    chatStatusCallback->updateChatStatus(channelNum, result ? ChatSent : ChatFailed);
    return result;
}

bool LoRaChannel::broadcast(uint8_t *message, uint8_t length) {
    chatStatusCallback->updateChatStatus(channelNum, ChatSending);
    bool result = lora->broadcast(message, length);
    chatStatusCallback->updateChatStatus(channelNum, result ? ChatSent : ChatFailed);
    return result;
}


bool LoRaChannel::send(String message, uint8_t address) {
    chatStatusCallback->updateChatStatus(channelNum, ChatSending);
    bool result = lora->send(message, address);
    chatStatusCallback->updateChatStatus(channelNum, result ? ChatSent : ChatFailed);
    return result;
}

bool LoRaChannel::send(uint8_t *message, int length, uint8_t address) {
    chatStatusCallback->updateChatStatus(channelNum, ChatSending);
    bool result = lora->send(message, length, address);
    chatStatusCallback->updateChatStatus(channelNum, result ? ChatSent : ChatFailed);
    return result;
}

bool LoRaChannel::fireAndForget(uint8_t *message, int length, uint8_t address) {
    chatStatusCallback->updateChatStatus(channelNum, ChatSending);
    bool result = lora->fireAndForget(message, length, address);
    chatStatusCallback->updateChatStatus(channelNum, result ? ChatSent : ChatFailed);
    return result;
}

bool LoRaChannel::isEnabled () {
    return true; //? check a switch?
}

bool LoRaChannel::isConnected () {
    return lora->isRunning();
}

// check swtiches for these?
bool LoRaChannel::isEncrypted () {
    return true;
}

bool LoRaChannel::isSigned () {
    return true;
}

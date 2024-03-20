#include <Arduino.h>

#ifndef CHATTERCHANNEL_H
#define CHATTERCHANNEL_H

enum ChatterMessageType {
  MessageTypeText = 1,
  MessageTypeRaw = 2,
  MessageTypeSignature = 3,
  MessageTypeHeader = 4,
  MessageTypeComplete = 5,
  MessageTypeConfirm = 6
};

// to be implemented by any channel
class ChatterChannel {
  public:
    virtual bool init () = 0;

    // interacting with the channel
    virtual bool hasMessage () = 0;
    virtual bool retrieveMessage () = 0;
    virtual uint8_t getLastSender () = 0;
    virtual uint8_t getSelfAddress () = 0; // self address
    virtual ChatterMessageType getMessageType () = 0;
    virtual int getMessageSize () = 0;
    virtual const char* getTextMessage () = 0; // pointer to text message buffer
    virtual const uint8_t* getRawMessage () = 0; // pointer to raw message buffer
    virtual uint8_t getAddress (const char* otherDeviceId) = 0;

    // broadcast text and raw messagse
    virtual bool broadcast (String message) = 0;
    virtual bool broadcast(uint8_t *message, uint8_t length) = 0;

    virtual bool supportsAck () = 0;

    // send text and raw messages to device, notice limitation of 0-255 address for now!
    virtual bool send (String message, uint8_t address) = 0;
    virtual bool send(uint8_t *message, int length, uint8_t address) = 0;

    virtual bool fireAndForget(uint8_t *message, int length, uint8_t address) = 0;

    // how the channel is configured
    virtual const char* getName() = 0;
    virtual bool isEnabled () = 0;
    virtual bool isConnected () = 0;
    virtual bool isEncrypted () = 0; // whether encryption is required
    virtual bool isSigned () = 0; // whether sig is required/validated

    virtual bool putToSleep () = 0;
};

#endif
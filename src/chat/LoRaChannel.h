#include "ChatterChannel.h"
#include "LoRaTrans.h"
#include "ChatStatusCallback.h"

#ifndef LORACHATTER_H
#define LORACHATTER_H
class LoRaChannel : public ChatterChannel {
    public:
        LoRaChannel (uint8_t channelNum, float frequency, uint8_t selfAddress, int csPin, int intPin, int rsPin, bool logEnabled, ChatStatusCallback* chatStatusCallback);

        LoRaTrans* getLoRaTrans () { return lora; }

        bool init();

        // interacting with the channel
        bool hasMessage ();
        bool retrieveMessage ();
        uint8_t getLastSender ();
        uint8_t getSelfAddress();
        ChatterMessageType getMessageType ();
        int getMessageSize ();
        const char* getTextMessage (); // pointer to text message buffer
        const uint8_t* getRawMessage (); // pointer to raw message buffer

        uint8_t getAddress (const char* otherDeviceId);


        // broadcast text and raw message
        bool broadcast (String message);
        bool broadcast(uint8_t *message, uint8_t length);

        bool supportsAck () {return true;}

        // send text and raw messages to device, notice limitation of 0-255 address for now!
        bool send (String message, uint8_t address);
        bool send(uint8_t *message, int length, uint8_t address);

        bool fireAndForget(uint8_t *message, int length, uint8_t address);

        // how the channel is configured
        const char* getName() {return "LoRa";}
        bool isEnabled ();
        bool isConnected ();
        bool isEncrypted (); // whether encryption is required
        bool isSigned (); // whether sig is required/validated

        bool putToSleep ();

    private:
        uint8_t channelNum;
        LoRaTrans* lora;
        ChatStatusCallback* chatStatusCallback;

};
#endif
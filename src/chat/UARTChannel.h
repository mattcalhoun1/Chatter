#include <Arduino.h>
#include "ChatterChannel.h"
#include "ChatGlobals.h"
#include "ChatStatusCallback.h"

#ifndef UARTCHANNEL_H
#define UARTCHANNEL_H

#define UART_CHANNEL_SPEED 115200

// channel for UART/serial connections
class UARTChannel : public ChatterChannel {
  public:
    UARTChannel(int _channelNum, bool _logEnabled, ChatStatusCallback* _chatStatusCallback) {channelNum = _channelNum; logEnabled = _logEnabled; chatStatusCallback = _chatStatusCallback;}
    bool init ();

    // interacting with the channel
    bool hasMessage ();
    bool retrieveMessage ();
    uint8_t getLastSender ();
    uint8_t getSelfAddress (); // self address
    ChatterMessageType getMessageType ();
    int getMessageSize ();
    const char* getTextMessage (); // pointer to text message buffer
    const uint8_t* getRawMessage (); // pointer to raw message buffer
    uint8_t getAddress (const char* otherDeviceId);

    // broadcast text and raw messagse
    bool broadcast (String message);
    bool broadcast(uint8_t *message, uint8_t length);

    // send text and raw messages to device, notice limitation of 0-255 address for now!
    bool send (String message, uint8_t address);
    bool send(uint8_t *message, int length, uint8_t address);

    bool fireAndForget(uint8_t *message, int length, uint8_t address);

    bool supportsAck () {return true;}

    // how the channel is configured
    const char* getName();
    bool isEnabled ();
    bool isConnected ();
    bool isEncrypted (); // whether encryption is required
    bool isSigned (); // whether sig is required/validated

    bool putToSleep () { return false; } // not yet implemented

  private:
    uint8_t messageBuffer[CHATTER_FULL_BUFFER_LEN];
    int lastMessageLength = 0;
    void logConsole(const char* msg);
    void logConsole(String msg);
    bool logEnabled;
    unsigned long uartTimeout = 3000; // 3 sec timeout
    int channelNum;
    ChatStatusCallback* chatStatusCallback;
};

#endif
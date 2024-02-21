#include "UdpChannel.h"

// just to let compile on non-airlift
#ifndef WL_NO_MODULE
#define WL_NO_MODULE WL_NO_SHIELD
#endif

#ifndef AIRLIFTUDPCHANNEL_H
#define AIRLIFTUDPCHANNEL_H

class AirliftUdpChannel : public UdpChannel {
    public:
        AirliftUdpChannel (uint8_t channelNum, const char* udpHostName, const char* ssid, const char* cred, int _ssPin, int _ackPin, int _resetPin, int _gpi0, bool logEnabled, ChatStatusCallback* chatStatusCallback) : UdpChannel(channelNum, udpHostName, ssid, cred, logEnabled, chatStatusCallback) {ssPin = _ssPin; ackPin = _ackPin; resetPin = _resetPin; gpi0 = _gpi0; }
    protected:
        void setupWifi ();
};

#endif
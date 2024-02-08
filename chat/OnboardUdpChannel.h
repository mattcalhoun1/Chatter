#include "UdpChannel.h"

#ifndef ONBOARDUDPCHANNEL_H
#define ONBOARDUDPCHANNEL_H

// just to let compile on non-onboard
#ifndef WL_NO_SHIELD
#define WL_NO_SHIELD WL_NO_MODULE
#endif

class OnboardUdpChannel : public UdpChannel {
    public:
        OnboardUdpChannel (uint8_t channelNum, const char* udpHostName, const char* wifiString, bool logEnabled, ChatStatusCallback* chatStatusCallback) : UdpChannel(channelNum, udpHostName, wifiString, logEnabled, chatStatusCallback) {}

    protected:
        void setupWifi ();
};

#endif
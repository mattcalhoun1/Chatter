#ifndef CHATSTATUSCALLBACK_H
#define CHATSTATUSCALLBACK_H

enum ChatStatus {
    ChatDisconnected = 0,
    ChatConnecting = 1,
    ChatConnected = 2,
    ChatReceiving = 3,
    ChatReceived = 4,
    ChatSending = 5,
    ChatSent = 6,
    ChatFailed = 7,
    ChatNoDevice = 8
};

class ChatStatusCallback {
    public:
        virtual void updateChatStatus (uint8_t channelNum, ChatStatus newStatus) = 0;
};
#endif
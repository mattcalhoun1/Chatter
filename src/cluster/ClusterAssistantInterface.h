#include "ClusterAssistant.h"
#include "ClusterGlobals.h"

#ifndef BLECLUSTERASSISTANTINTERFACE_H
#define BLECLUSTERASSISTANTINTERFACE_H

class ClusterAssistantInterface : public ClusterAssistant {
    public:
        ClusterAssistantInterface (Chatter* _chatter) : ClusterAssistant(_chatter) {}
        virtual bool init () = 0;
        virtual bool attemptOnboard () = 0;
};

#endif
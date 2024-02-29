#include "ClusterAdmin.h"
#include "../chat/Chatter.h"

#ifndef CLUSTERADMININTERFACE_H
#define CLUSTERADMININTERFACE_H

class ClusterAdminInterface : public ClusterAdmin {
    public:
        ClusterAdminInterface (Chatter* _chatter) : ClusterAdmin(_chatter) {}
        virtual bool init () = 0;
        //virtual bool handleClientInput (const char* input, int inputLength) = 0;
        virtual bool isConnected () = 0;
};

#endif
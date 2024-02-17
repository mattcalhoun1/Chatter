#ifndef CLUSTERINTERFACE_H
#define CLUSTERINTERFACE_H

class ClusterInterface {
    public:
        virtual bool init () = 0;
        virtual bool handleClientInput (const char* input, int inputLength) = 0;
        virtual bool sendToClient (const char* clusterData, int dataLength) = 0;
        virtual bool isConnected () = 0;
};

#endif
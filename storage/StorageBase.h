#include "StorageGlobals.h"
#include "Arduino.h"

#ifndef STORAGEBASE_H
#define STORAGEBASE_H

class StorageBase {
    public:
        bool isSafeFilename (const char* filename);
        bool isRunning () {return running;}
    protected:
        void logConsole(const char* msg);
        void logConsole(String msg);
        bool running = false;
};

#endif
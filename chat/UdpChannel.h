#include "ChatterChannel.h"
#include "ChatGlobals.h"

#ifdef AIRLIFT_ONBOARD
#include <WiFiNINA.h>
#include <WiFiUdp.h>
#endif


#ifdef WIFI_ONBOARD
#include "WiFiS3.h"
#endif
#include <SPI.h>
#include "ChatStatusCallback.h"

#ifndef UDPCHANNEL_H
#define UDPCHANNEL_H

#define UDP_PACKET_SIZE 48
#define UDP_MAX_MESSAGE_LEN 150
#define UDP_BROADCAST_COUNT 3 // broadcast same message 3 times
#define UDP_BROADCAST_DELAY 250 // ms delay between broadcasts
#define UDP_RECEIVE_COUNT 5 // broadcast same message 5 times
#define UDP_RECEIVE_DELAY 50 // ms delay between broadcasts

class UdpChannel : public ChatterChannel {
    public:
        UdpChannel (uint8_t channelNum, const char* udpHostName, const char* wifiString, bool logEnabled, ChatStatusCallback* chatStatusCallback);

        bool init();

        // interacting with the channel
        bool hasMessage ();
        bool retrieveMessage ();
        uint8_t getSelfAddress();
        uint8_t getLastSender ();
        ChatterMessageType getMessageType ();
        int getMessageSize ();
        const char* getTextMessage (); // pointer to text message buffer
        const uint8_t* getRawMessage (); // pointer to raw message buffer

        uint8_t getAddress (const char* otherDeviceId);

        // broadcast text and raw message
        bool broadcast (String message);
        bool broadcast(uint8_t *message, uint8_t length);

        bool supportsAck () {return false;}

        // send text and raw messages to device, notice limitation of 0-255 address for now!
        bool send (String message, uint8_t address);
        bool send(uint8_t *message, int length, uint8_t address);

        bool fireAndForget(uint8_t *message, int length, uint8_t address);

        // how the channel is configured
        const char* getName() {return "WiFi";}
        bool isEnabled ();
        bool isConnected ();
        bool isEncrypted (); // whether encryption is required
        bool isSigned (); // whether sig is required/validated

    protected:
        unsigned long sendNTPpacket(IPAddress& address);
        void printCurrentNet();
        void printWifiData();
        void printMacAddress(byte mac[]);
        //void printEncryptionType(int thisType);
        void listNetworks();
        void wifiConnect();
        bool refreshWifiStatus();
        bool checkUdpInput();
        virtual void setupWifi() = 0;
        void sendTimeRequest ();

        // low ssid/pw limit
        char ssid[16];
        char password[16];

        char hostName[16];
        bool validWifiConfig = true;
        bool logEnabled = false;

        unsigned int localPort = 2390;      // local port to listen for UDP packets
        IPAddress timeServer = IPAddress(129, 6, 15, 28); // time.nist.gov NTP server
        //IPAddress multicastServer = IPAddress(192,168,0, 255);
        
        // todo pull this from config
        IPAddress multicastServer = IPAddress(192,168,254,255);
        
        //IPAddress timeServer(10, 0, 0, 122); // my pc
        //const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
        byte packetBuffer[ UDP_MAX_MESSAGE_LEN ]; //buffer to hold incoming and outgoing packets

        uint8_t buf[UDP_MAX_MESSAGE_LEN];
        int lastSender;
        int lastMessageSize = 0;
        //uint8_t chunkInBuffer[UDP_MAX_CHUNK_IN_BUFFER_SIZE];
        //int chunkInBufferSize = 0; // how many received bytes are in the buffer
        //unsigned long textBufferTime = 0;
        //unsigned long chunkBufferTime = 0;
        
        uint8_t channelNum;
        int ssPin;
        int ackPin;
        int resetPin;
        int gpi0;
        ChatStatusCallback* chatStatusCallback;

        // A UDP instance to let us send and receive packets over UDP
        WiFiUDP Udp;
        bool wifiEnabled = false;
        bool wifiConnected = false;
        int timeCheckInterval = 5; // every 5 loops

        void logConsole (String msg);

};
#endif
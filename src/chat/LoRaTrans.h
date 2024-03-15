#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <RHReliableDatagram.h>
#include <RH_RF95.h>

#ifndef pin_size_t
#define pin_size_t __uint8_t
#endif

#ifndef LORATRANS_H
#define LORATRANS_H


//#define LORA_RFM9X_MAX_MESSAGE_LEN 64
#define LORA_MAX_MESSAGE_LEN 150 // really only expect about 128, but when encrypting, message chunks may be slightly larger

#define LORA_ACK_TIMEOUT 1000

#define LORA_ADDR_BROADCAST 255
#define LORA_POWER 23
#define LORA_SLEEP_AFTER_TX true
#define LORA_SLEEP_AFTER_RX true

#define LORA_BROADCAST_COUNT 4 // broadcast same message 4 times
#define LORA_BROADCAST_DELAY 500 // ms delay between broadcasts

class LoRaTrans {
  public:
    LoRaTrans(float frequency, uint8_t selfAddress, int csPin, int intPin, int rsPin, bool logEnabled);
    bool hasMessage ();
    long retrieveMessage();
    int getLastSender ();
    uint8_t getSelfAddress() {return selfAddress;}
    int getLastMessageSize () {return lastMessageSize;}
    uint8_t* getMessageBuffer ();
    bool broadcast (String message);
    bool broadcast(uint8_t *message, uint8_t length);
    bool send (String message, int address);
    bool send(uint8_t *message, int length, uint8_t address);
    bool fireAndForget (String message, int address);
    bool fireAndForget(uint8_t *message, int length, uint8_t address);
    int getLastRssi ();

    bool init ();
    bool isRunning ();

  private:
    void reset ();
    
    float frequency;
    uint8_t selfAddress;
    pin_size_t csPin;
    pin_size_t intPin;
    pin_size_t rsPin;
    bool logEnabled = false;

    RH_RF95* rfm9x;
    RHReliableDatagram* rfm9x_manager;
    uint8_t buf[LORA_MAX_MESSAGE_LEN];
    bool running = false;
    int lastSender;
    int lastMessageSize = 0;
    void logConsole(String msg);
    void logConsole(String msg, bool newline);
    void logConsole(int msg, bool newline, int base);
    unsigned long messageBufferTime = 0;
};
#endif
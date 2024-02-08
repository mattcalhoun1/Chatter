#include "UARTChannel.h"

bool UARTChannel::init() {
  logConsole("Initializing UART Channel");
    chatStatusCallback->updateChatStatus(channelNum, ChatConnecting);


  Serial1.begin(UART_CHANNEL_SPEED);
  unsigned long startTime = millis();

  while (Serial1 == false && millis() - startTime < uartTimeout) {
    delay(200);
  }

  if (Serial1) {
    chatStatusCallback->updateChatStatus(channelNum, ChatConnected);
  }
  else {
    chatStatusCallback->updateChatStatus(channelNum, ChatDisconnected);
  }

  return Serial1;
}

bool UARTChannel::hasMessage () {
  lastMessageLength = 0;
    bool termed = false; // whether we receved termination chars (00000)

  if (Serial1.available() > 0) {
    chatStatusCallback->updateChatStatus(channelNum, ChatReceiving);
    unsigned long startTime = millis();
    // first byte should be # of bytes to expect
    //uint8_t nextMessageSize = (uint8_t)Serial1.read();
    while (Serial1.available() > 0 && !termed && millis() - startTime < uartTimeout) {
        messageBuffer[lastMessageLength++] = (uint8_t)Serial1.read();

        // check if termed
        if (lastMessageLength >5 && messageBuffer[lastMessageLength-1] == 255 && messageBuffer[lastMessageLength-2] == 255 && messageBuffer[lastMessageLength-3] == 255 && messageBuffer[lastMessageLength-4] == 255 && messageBuffer[lastMessageLength-5] == 255) {
            termed = true;
            lastMessageLength -= 5;
            Serial1.write((uint8_t)1); // write an ack
            Serial1.flush();
        }

        if (lastMessageLength > CHATTER_FULL_BUFFER_LEN) {
            chatStatusCallback->updateChatStatus(channelNum, ChatFailed);
            logConsole("UART out of sync, invalid message size sent. clearing buffer. messages may be lost.");
            // clear the buffer and throw away some data
            while (Serial1.available()) {
                Serial1.read();
            }
        }
    }
    if (termed) {
        chatStatusCallback->updateChatStatus(channelNum, ChatReceived);
    }
    else {
        chatStatusCallback->updateChatStatus(channelNum, ChatFailed);
        logConsole("No UART term received.");
    }
  }

  if (Serial1) {
    chatStatusCallback->updateChatStatus(channelNum, ChatConnected);
  }

  return lastMessageLength > 0 && termed;
}

bool UARTChannel::retrieveMessage () {
  // does nothing since the retrieval happens on the message check
  return lastMessageLength > 0;
}

uint8_t UARTChannel::getLastSender () {
  return 0;
}

uint8_t UARTChannel::getSelfAddress () {
  return 0;
}

ChatterMessageType UARTChannel::getMessageType () {
  return MessageTypeText;
}

int UARTChannel::getMessageSize () {
  return lastMessageLength;
}
const char* UARTChannel::getTextMessage () {
  return (const char*) messageBuffer;
}

const uint8_t* UARTChannel::getRawMessage () {
  return messageBuffer;
}


uint8_t UARTChannel::getAddress (const char* otherDeviceId) {
  return 0;
}

bool UARTChannel::broadcast (String message) {
  return send((uint8_t*)message.c_str(), message.length(), 0);
}

bool UARTChannel::broadcast(uint8_t *message, uint8_t length) {
  return send(message, length, 0);
}

bool UARTChannel::send (String message, uint8_t address) {
  return send((uint8_t*)message.c_str(), message.length(), address);
}

bool UARTChannel::send(uint8_t *message, int length, uint8_t address) {
    bool success = false;
    chatStatusCallback->updateChatStatus(channelNum, ChatSending);
    logConsole("Writing to UART " + String(length) + " bytes");
    if (length <= CHATTER_FULL_BUFFER_LEN) {
        for (int i = 0; i < length; i++) {
            Serial1.write((uint8_t)message[i]);
        }
        success = true;
        //success = Serial1.write(message, length) == length;
        Serial1.flush();

        // write a termination of 5 max chars
        for (int i = 0; i < 5; i++) {
            Serial1.write((uint8_t)255);
        }
        Serial1.flush();

        // wait for confirmation from other side
        unsigned long startTime = millis();
        bool ackReceived = false;
        while (millis() - startTime < uartTimeout && !ackReceived) {
            if (Serial1.available() == 1 && (uint8_t)Serial1.peek() == 1) {
                Serial1.read();
                ackReceived = true;
            }
            else {
                delay(50);
            }
        }
        if (ackReceived) {
            logConsole("Message complete to UART " + String(length) + " bytes");
            success = true;
            chatStatusCallback->updateChatStatus(channelNum, ChatSent);
        }
        else {
            logConsole("Acknowledgement not received");
            chatStatusCallback->updateChatStatus(channelNum, ChatFailed);
        }
    }
    else {
        logConsole("Message size (" + String(length) + ") too large for UART buffer!");
        chatStatusCallback->updateChatStatus(channelNum, ChatFailed);
    }

    return success;
}

bool UARTChannel::fireAndForget(uint8_t *message, int length, uint8_t address) {
    // fire and forget not supported in uart, because it would corrupt the serial buffer
    return send(message, length, address);
}

const char* UARTChannel::getName() {
  return "UART";
}

bool UARTChannel::isEnabled () {
  return true;
}

bool UARTChannel::isConnected () {
  return Serial1;
}

bool UARTChannel::isEncrypted () {
  return false;
}

bool UARTChannel::isSigned () {
  return false;
}

void UARTChannel::logConsole (const char* msg) {
    if (logEnabled) {
        Serial.println(msg);
    }
}

void UARTChannel::logConsole (String msg) {
    logConsole(msg.c_str());
}
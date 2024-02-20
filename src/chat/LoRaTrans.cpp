#include "LoRaTrans.h"
#include "RHSoftwareSPI.h"

LoRaTrans::LoRaTrans(float frequency, uint8_t selfAddress, int csPin, int intPin, int rsPin, bool logEnabled) {
  this->frequency = frequency;
  this->selfAddress = selfAddress;
  this->csPin = csPin;
  this->intPin = intPin;
  this->rsPin = rsPin;
  this->logEnabled = logEnabled;

  rfm9x = new RH_RF95(csPin, intPin);
  //RHSoftwareSPI rhs;
  //rhs.setPins(uint8_t miso = 12, uint8_t mosi = 11, uint8_t sck = 13);
  //rfm9x = new RH_RF95(csPin, intPin, rhs);
}

bool LoRaTrans::init () {
  delay(100);
  rfm9x_manager = new RHReliableDatagram(*rfm9x, this->selfAddress);
  pinMode(this->rsPin, OUTPUT);
  digitalWrite(this->rsPin, HIGH);

  reset();

  int retryCount = 0;
  int maxRetries = 3;
  running = false;
  while (retryCount++ < maxRetries && !running) {
    running = rfm9x_manager->init();
    if (!running) {
      delay(500);
    }
    /*else {
      rfm9x_manager->setTimeout(1000);
    }*/
  }
  if (!running) {
    logConsole("RFM9X radio init failed");
    running = false;
  }
  else {
    logConsole("RFM9X radio init OK!");

    // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM (for low power module)
    // No encryption
    if (!rfm9x->setFrequency(this->frequency)) {
      logConsole("setFrequency failed");
      running = false;
    }
    else {
      // If you are using a high power RF69 eg RFM69HW, you *must* set a Tx power with the
      // ishighpowermodule flag set like this:
      //rfm9x->setTxPower(14, true);
      rfm9x->setTxPower(LORA_POWER, false);

      logConsole("RFM9X radio @", false);
      logConsole(String(this->frequency), false);
      logConsole(" MHz");
      running = true;
    }
  }
  return running;
}

bool LoRaTrans::isRunning () {
  return running;
}

bool LoRaTrans::hasMessage() {
  if (running) {
    unsigned long startTime = millis();
    bool available = false;
    while ((!available) && millis() - startTime < 1000) {
      available = rfm9x_manager->available();
      if (!available) {
        delay(20);
      }
    }

    if (available == false && LORA_SLEEP_AFTER_RX) {
      rfm9x->sleep();
    }

    return available;
  }
  return false;
}

uint8_t* LoRaTrans::getMessageBuffer() {
  return buf;
}

bool LoRaTrans::broadcast(String message) {
  return broadcast((uint8_t*)message.c_str(), message.length());
}

bool LoRaTrans::broadcast(uint8_t* message, uint8_t length) {
  logConsole("LoRa Broadcasting ", false);
  logConsole(String(length), false);
  logConsole(" to ", false);
  logConsole(String(LORA_ADDR_BROADCAST));
  bool sent = rfm9x_manager->sendtoWait(message, length, LORA_ADDR_BROADCAST);

  if (LORA_SLEEP_AFTER_TX) {
    rfm9x->sleep();
  }

  return sent;
}

bool LoRaTrans::send(String message, int address) {
  return send((uint8_t*)message.c_str(), message.length(), address);
}

int LoRaTrans::getLastRssi() {
  return rfm9x->lastRssi();
}

int LoRaTrans::getLastSender() {
  return lastSender;
}

bool LoRaTrans::fireAndForget(String message, int address) {
  return fireAndForget((uint8_t*)message.c_str(), message.length(), address);
}

bool LoRaTrans::fireAndForget(uint8_t* message, int length, uint8_t address) {

  logConsole("LoRa Firing ", false);
  logConsole(String(length), false);
  logConsole(" to ", false);
  logConsole(String(address));

  int sends = 0;
  bool sent = false;

  while (sends++ < LORA_BROADCAST_COUNT) {
    sent = rfm9x_manager->sendto(message, length, address);
    delay(LORA_BROADCAST_DELAY);
  }

  if (LORA_SLEEP_AFTER_TX) {
    rfm9x->sleep();
  }

  return sent;
}

bool LoRaTrans::send(uint8_t* message, int length, uint8_t address) {

  logConsole("LoRa Sending ", false);
  logConsole(String(length), false);
  logConsole(" to ", false);
  logConsole(String(address));

  int retries = 0;
  int maxRetries = 2;
  bool sent = false;

  while (sent == false && retries <= maxRetries) {
    if (rfm9x_manager->sendtoWait(message, length, address)) {
        sent = true;
    }
    else {
      retries += 1;
      if (retries <= maxRetries) {
        logConsole("LoRa send failed. Retrying.");
      }
      else {
        logConsole("LoRa Send Failed. No more retries.");
      }
    }
  }

  if (LORA_SLEEP_AFTER_TX) {
    rfm9x->sleep();
  }

  return sent;
}

long LoRaTrans::retrieveMessage() {
  uint8_t len = sizeof(buf);
  uint8_t from;
  if (rfm9x_manager->recvfromAckTimeout(buf, &len, LORA_ACK_TIMEOUT, &from)) {
      buf[len] = 0; // terminate
      logConsole("LoRa: Received and acknowledged message size: " + String(len) + " from: " + String(from));

      /*for (int i = 0; i < len; i++) {
        Serial.print((char)buf[i]);
      }
      Serial.println("");*/

      this->lastSender = from;
      messageBufferTime = millis();
      lastMessageSize = len;

      if (LORA_SLEEP_AFTER_RX) {
        rfm9x->sleep();
      }
      return len;
    } else {
      logConsole("Failed to receive or acknowledge message, possibly size: " + String(len) + " from: " + String(from));
    }
  lastMessageSize = 0;
  if (LORA_SLEEP_AFTER_RX) {
    rfm9x->sleep();
  }
  return 0;
}

void LoRaTrans::reset() {
  // manual reset
  logConsole("Resetting LoRa...");
  digitalWrite(this->rsPin, LOW);
  delay(10);
  digitalWrite(this->rsPin, HIGH);
  delay(10);
  logConsole("LoRa successfully reset.");
}

void LoRaTrans::logConsole(String msg, bool newline) {
  if (logEnabled) {
    if (newline) {
      Serial.println(msg);
    } else {
      Serial.print(msg);
    }
  }
}

void LoRaTrans::logConsole(int msg, bool newline, int base) {
  if (logEnabled) {
    if (newline) {
      Serial.println(msg, base);
    } else {
      Serial.print(msg, base);
    }
  }
}

void LoRaTrans::logConsole(String msg) {
  logConsole(msg, true);
}
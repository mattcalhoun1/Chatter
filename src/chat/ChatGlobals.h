#ifndef CHATGLOBALS_H
#define CHATGLOBALS_H

// these two wifi options are mutually exclusive
//#define WIFI_ONBOARD true
#define AIRLIFT_ONBOARD true

#define CHAT_LOG_ENABLED true
#define CHAT_MAX_CHANNELS 2

#define ENCRYPTION_CRED_DELIMITER '|'
#define ENCRYPTION_KEY_SLOT 10

#define WIFI_SSID_MAX_LEN 16
#define WIFI_PASSWORD_MAX_LEN 16
#define WIFI_HOSTNAME_MAX_LEN 16

#define CHATTER_FULL_BUFFER_LEN 150 // full buffer for holding messages to go out. since encryption can increase size of message, this should be > packet size

#define CHATTER_PACKET_SIZE 128
#define CHATTER_PACKET_HEADER_LENGTH 19 // (CHATTER_DEVICE_ID_SIZE*2 + CHATTER_MESSAGE_ID_SIZE + CHATTER_CHUNK_ID_SIZE + 1)
#define CHATTER_DEVICE_ID_SIZE 6
#define CHATTER_MESSAGE_ID_SIZE 3
#define CHATTER_CHUNK_ID_SIZE 3
#define CHATTER_SIGNATURE_PACKET_ID 0 // always packet 0 contains sig
#define CHATTER_FULL_MESSAGE_BUFFER_SIZE 1024 // max size of message pieced back together

#define CHATTER_SIGN_PK_SLOT 0
#define CHATTER_STORAGE_PK_SLOT 8

#define CHATTER_SSC_YEAR 2024
#define CHATTER_SSC_MONTH 2
#define CHATTER_SSC_DATE 11
#define CHATTER_SSC_VALID 30
#define CHATTER_SSC_HOUR 1


#define CHATTER_SIGNATURE_LENGTH 64
#define CHATTER_HASH_SIZE 32

// these are for full message headers/footers, which chunk messages between
#define CHATTER_FOOTER_SIZE 96 // 32 hash + 64 sig
#define CHATTER_FOOTER_BUFFER_SIZE 97 // plus term
#define CHATTER_HEADER_SIZE 55 // HHH[nbf][na][recipient][reserved*6][rand*16] = 3 + 24 + 6 + 6 + 16, nbf date format: 241231235959
#define CHATTER_HEADER_BUFFER_SIZE 56 // HHH[nbf][na][recipient][reserved*6][rand*16] = 24 + 6 + 6 + 16 + term, nbf date format: 241231235959

#define CHATTER_EXPIRY_NBF_SECONDS -60 // 1 min clock diff before
#define CHATTER_EXPIRY_NAF_SECONDS 600 // 10 min clock diff after
#define CHATTER_EXPIRY_DATE_LENGTH 12

#define CHATTER_BROADCAST_ID "CAL255"

#define UDP_PREFIX "C"
#define WIFI_SSID_SLOT 14
#define DEVICE_ID_SLOT 9 // unique alphanumeric ID of the device. Optionally add extra config since we have 72 bytes here

#endif
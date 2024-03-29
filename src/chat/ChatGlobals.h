#ifndef CHATGLOBALS_H
#define CHATGLOBALS_H

// these two wifi options are mutually exclusive
//#define WIFI_ONBOARD true
#define AIRLIFT_ONBOARD true

#define BRIDGE_LORA_ADDRESS "001" //
#define BRIDGE_WIFI_ADDRESS "002"
#define BRIDGE_CLOUD_ADDRESS "003"

#define BRIDGE_LORA_ALIAS "Bridge_LoRa"
#define BRIDGE_WIFI_ALIAS "Bridge_Wifi"
#define BRIDGE_CLOUD_ALIAS "Bridge_Cloud"

#define STARTING_DEVICE_ADDRESS 10 // first com device will get this address

#define LORA_DEFAULT_FREQUENCY 915.0
#define LORA_MIN_FREQUENCY 824.0
#define LORA_MAX_FREQUENCY 960.0

#define CHATTER_ALIAS_NAME_SIZE 12
#define CHATTER_MIN_ALIAS_LENGTH 3

#define DEVICE_TYPE_BRIDGE_LORA "BL"
#define DEVICE_TYPE_BRIDGE_WIFI "BW"
#define DEVICE_TYPE_BRIDGE_CLOUD "BC"
#define DEVICE_TYPE_COMMUNICATOR "C0" // gen 0 communicator
#define DEVICE_TYPE_RAW "XX" // direct serial entry (like from ide)

#define CHAT_LOG_ENABLED true
#define CHAT_MAX_CHANNELS 2

#define ENCRYPTION_CRED_DELIMITER '|'
#define ENCRYPTION_KEY_SLOT 10
#define ENCRYPTION_IV_SLOT 11

#define CHATTER_WIFI_STRING_MAX_SIZE 33
//#define WIFI_SSID_MAX_LEN 16
//#define WIFI_PASSWORD_MAX_LEN 16
#define WIFI_HOSTNAME_MAX_LEN 16

#define CHATTER_FULL_BUFFER_LEN 150 // full buffer for holding messages to go out. since encryption can increase size of message, this should be > packet size

#define CHATTER_PACKET_SIZE 128
#define CHATTER_PACKET_HEADER_LENGTH 23 // (CHATTER_DEVICE_ID_SIZE*2 + CHATTER_MESSAGE_ID_SIZE + CHATTER_CHUNK_ID_SIZE + 1)
#define CHATTER_DEVICE_ID_SIZE 8 // global network (2 bytes) + local network (3 bytes) + local address (3 numeric --- 0:254)
#define CHATTER_GLOBAL_NET_ID_SIZE 2
#define CHATTER_LOCAL_NET_ID_SIZE 3

#define CHATTER_MESSAGE_ID_SIZE 3
#define CHATTER_CHUNK_ID_SIZE 3
#define CHATTER_SIGNATURE_PACKET_ID 0 // always packet 0 contains sig
#define CHATTER_SIGNATURE_CHUNK_ID "000"
#define CHATTER_FULL_MESSAGE_BUFFER_SIZE 1024 // max size of message pieced back together
#define CHATTER_INTERNAL_MESSAGE_BUFFER_SIZE 256 // max size of messages generated by chatter layer itself
#define CHATTER_MAX_PACKETS 10 // 

#define CHATTER_SIGN_PK_SLOT 0
#define CHATTER_STORAGE_PK_SLOT 8

#define CHATTER_SIGNATURE_LENGTH 64
#define CHATTER_HASH_SIZE 32

// these are for full message headers/footers, which chunk messages between
#define CHATTER_FOOTER_SIZE 96 // 32 hash + 64 sig
#define CHATTER_FOOTER_BUFFER_SIZE 97 // plus term
#define CHATTER_HEADER_SIZE 55 // HHH[nbf][na][recipient][flags*6][rand*14] = 3 + 24 + 8 + 6 + 14, nbf date format: 241231235959
#define CHATTER_HEADER_BUFFER_SIZE 56 // HHH[nbf][na][recipient][flags*6][rand*14] = 24 + 8 + 6 + 14 + term, nbf date format: 241231235959

#define CHATTER_EXPIRY_NBF_SECONDS -300 // 5 min clock diff before (for drift)
#define CHATTER_EXPIRY_NAF_SECONDS 600 // 10 min clock diff after
#define CHATTER_EXPIRY_NAF_SECONDS_LONG 5400 // 1.5 hour
#define CHATTER_EXPIRY_DATE_LENGTH 12

#define CHATTER_BROADCAST_ID "255"

#define WIFI_SSID_SLOT 14
#define LORA_FREQUENCY_SLOT 13
#define DEVICE_ID_SLOT 9 // unique alphanumeric ID of the device. Optionally add extra config since we have 72 bytes here

#endif
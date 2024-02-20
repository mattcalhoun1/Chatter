#ifndef GLOBALS_H
#define GLOBALS_H

#define CHATTER_WIFI_STRING_MAX_SIZE 33
#define CHATTER_LORA_FREQUENCY_DIGITS 5

#define CHATTER_FULL_BUFFER_LEN 150 // full buffer for holding messages to go out. since encryption can increase size of message, this should be > packet size

#define CHATTER_PACKET_SIZE 128
#define CHATTER_PACKET_HEADER_LENGTH 23 // (CHATTER_DEVICE_ID_SIZE*2 + CHATTER_MESSAGE_ID_SIZE + CHATTER_CHUNK_ID_SIZE + 1)
#define CHATTER_DEVICE_ID_SIZE 8 // global network (2 bytes) + local network (3 bytes) + local address (3 numeric --- 0:254)
#define CHATTER_GLOBAL_NET_ID_SIZE 2
#define CHATTER_LOCAL_NET_ID_SIZE 3

#define CHATTER_MESSAGE_ID_SIZE 3
#define CHATTER_CHUNK_ID_SIZE 3
#define CHATTER_SIGNATURE_PACKET_ID 0 // always packet 0 contains sig

#define CHATTER_ALIAS_NAME_SIZE 12

#define ENC_SYMMETRIC_KEY_SIZE 16
#define ENC_IV_SIZE 8

#define ENC_SYMMETRIC_KEY_BUFFER_SIZE 65 // was 33
#define ENC_VOLATILE_KEY_SIZE 32
#define ENC_DATA_SLOT_SIZE 32 // was 32
#define ENC_DATA_SLOT_BUFFER_SIZE 33 // was 33 // is the driver overrunnign our buffer when we load slot 10?
#define ENC_HEX_BUFFER_SIZE 129 // was 129
#define ENC_MSG_BUFFER_SIZE 32 // only 32 byte messages are allowed for sig
#define ENC_SIG_BUFFER_SIZE 64
#define ENC_PUB_KEY_SIZE 40
#define ENC_PRIV_KEY_SIZE 21

#define FRAM_NUM_ZONES 4
#define FRAM_ZONE_DEVICE 0
#define FRAM_ZONE_CLUSTER 1
#define FRAM_ZONE_TRUST 2
#define FRAM_ZONE_PACKET 3

enum FramZone {
    ZoneDevice = FRAM_ZONE_DEVICE,
    ZoneCluster = FRAM_ZONE_CLUSTER,
    ZoneTrust = FRAM_ZONE_TRUST,
    ZonePacket = FRAM_ZONE_PACKET
};
#endif
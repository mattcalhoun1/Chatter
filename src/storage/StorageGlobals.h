#ifndef STORAGEGLOBALS_H
#define STORAGEGLOBALS_H

#define STORAGE_LOG_ENABLED true
#define STORAGE_DEVICE_ID_LENGTH 8
#define STORAGE_MAX_ALIAS_LENGTH 12
#define STORAGE_GLOBAL_NET_ID_SIZE 2
#define STORAGE_LOCAL_NET_ID_SIZE 3

// we could go as high as 254, but are limited
// by truststore size of the master device.
// since the device IDs + status are held in memory
// we dont want to use that many for now
#define STORAGE_MAX_CLUSTER_DEVICES 90

// slots reserved for cluster root, bridges, etc
#define FRAM_RESERVED_TRUST_SLOTS 4

#define STORAGE_PUBLIC_KEY_LENGTH 40 // 
#define STORAGE_PRIVATE_KEY_LENGTH 21 // 

#define STORAGE_HASH_BUFFER_SIZE 150
#define STORAGE_HASH_LENGTH 32 //sha256
#define STORAGE_PACKET_HEADER_LENGTH 23

#define STORAGE_MAX_FILENAME_LENGTH 64
#define STORAGE_MAX_TRUSTSTORE_FILENAME_LENGTH 16

#define STORAGE_CONTENT_MAX_UNENCRYPTED_SIZE 130 // max allowed unencrypted
#define STORAGE_CONTENT_BUFFER_SIZE 150 // tmp buffer for holding message content. higher in case of encryption

#define STORAGE_SIGNATURE_PACKET_ID 0 // always packet 0 contains sig

#define STORAGE_MESSAGE_ID_LENGTH 3
#define STORAGE_CHUNK_ID_LENGTH 3

#define STORAGE_SIG_PACKET_EXPECTED_LENGTH 96 // expect 32 (hash) + 64 (sig). different would indicate bad sig


#endif
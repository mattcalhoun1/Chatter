#ifndef ENCRYPTIONGLOBALS_H
#define ENCRYPTION_GLOBALS_H

#define ENC_LOG_ENABLED true

#define ENC_SSC_YEAR 2024
#define ENC_SSC_MONTH 2
#define ENC_SSC_DATE 11
#define ENC_SSC_VALID 30
#define ENC_SSC_HOUR 1
#define ENC_IV_SIZE 8

#define ENC_ECC_DSA_KEY_SIZE 64
#define ENC_SIGNATURE_SIZE 64

#define ENC_SYMMETRIC_KEY_SIZE 16
#define ENC_SYMMETRIC_KEY_BUFFER_SIZE 33
#define ENC_VOLATILE_KEY_SIZE 32
#define ENC_DATA_SLOT_SIZE 32
#define ENC_DATA_SLOT_BUFFER_SIZE 33 // is the driver overrunnign our buffer when we load slot 10?
#define ENC_HEX_BUFFER_SIZE 129
#define ENC_MSG_BUFFER_SIZE 32 // only 32 byte messages are allowed for sig
#define ENC_SIG_BUFFER_SIZE 64
#define ENC_PUB_KEY_SIZE 128

#define ENC_UNENCRYPTED_BUFFER_SIZE 150 // really only need closer to 128, but extra since encryption/decryption can change size
#define ENC_ENCRYPTED_BUFFER_SIZE 150


#endif
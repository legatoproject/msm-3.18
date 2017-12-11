/* kernel/drivers/misc/tzbsp_crypto_api.h
 *
 * Copyright (C) 2012-2017 Sierra Wireless, Inc
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * This is a Sierra created file and it is adapted and should keep sync to TZ
 * code:
 *
 *  trustzone_images/core/securemsm/trustzone/qsee/include/\
 *  crypto/tzbsp_crypto_api.h
 *
 */

#ifndef TZBSP_CRYPTO_API_H
#define TZBSP_CRYPTO_API_H

typedef uint64_t    uint64;
typedef uint32_t    uint32;
typedef uint16_t    uint16;
typedef uint8_t     uint8;
typedef int64_t     int64;
typedef int32_t     int32;
typedef int16_t     int16;
typedef int8_t      int8;

#define TZ_STORAGE_SVC_MAX_KEYLEN   (CRYPTO_ASYM_KEY_SIZE_MAX)

#define TZ_SVC_CRYPTO               (10)

#define TZ_SYSCALL_CREATE_CMD_ID(s, f) \
    ((uint32)(((s & 0x3ff) << 10) | (f & 0x3ff)))

#define TZ_CRYPTO_SERVICE_SYM_ID   TZ_SYSCALL_CREATE_CMD_ID(TZ_SVC_CRYPTO, 0x02)
#define TZ_CRYPTO_SERVICE_RSA_ID   TZ_SYSCALL_CREATE_CMD_ID(TZ_SVC_CRYPTO, 0x03)

/*----------------------------------------------------------------------------
 * Include Files
 * -------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * -------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 * Type Declarations
 * -------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 * Function Declarations and Documentation
 * -------------------------------------------------------------------------*/
#define TZ_CRYPTO_SVC_CLOCK_ENABLE      (1)
#define TZ_CRYPTO_SVC_CLOCK_DISABLE     (0)
#define TZ_CRYPTO_SVC_NONCE_LEN         (8)
#define TZ_CRYPTO_SVC_MAC_LEN           (16)
#define TZ_CRYPTO_SVC_MAX_PLAIN_DLEN    (0x800)

/* "" CRYPOT_API MAGIC NUMBER FOR SYM_KEY */
#define CRYPTO_ASYM_MAGIC_NUM (0x4B4D4B42)

/* Endianness converter Macro */
#define crypto_asym_htonl(x)    \
  (((((uint32)(x) & 0x000000FFU) << 24)     | \
  (((uint32)(x)   & 0x0000FF00U) <<  8)     | \
  (((uint32)(x)   & 0x00FF0000U) >>  8)     | \
  (((uint32)(x)   & 0xFF000000U) >> 24)))

#define CRYPTO_ASYM_AES128_KEY_SIZE     (16)
#define CRYPTO_ASYM_AES256_KEY_SIZE     (32)
#define CRYPTO_ASYM_HMAC_KEY_SIZE       (32)

/* 4096 bits */
#define CRYPTO_ASYM_KEY_SIZE_MAX        (512+16)

/* 5 bytes max public exponent size supported */
#define CRYPTO_ASYM_PUB_EXP_SIZE_MAX    (5)

/* AES128 CBC IV */
#define CRYPTO_ASYM_IV_LENGTH           (16)
/* SHA2 will be used for HMAC  */
#define CRYPTO_ASYM_HMAC_LENGTH         (32)

typedef uint32 CRYPTO_ASYM_BLONG;
typedef uint32 KM_BLONG;

///< Maximum key size in bits
#define KM_MAX_KEY_SIZE                 (4128)

///< Bytes per digit
#define KM_BLONG_SIZE                   (sizeof(KM_BLONG))

/*
 -------------------------------------------------------------------------------
    Data structure for storing the encrypted key blob
 -------------------------------------------------------------------------------
 */
typedef struct
{
  void *    key_material;
  uint32    key_material_len;
} tz_storage_service_key_blob_t;

/*
 -------------------------------------------------------------------------------
    Commands supported for the symmetric storage service.
 -------------------------------------------------------------------------------
 */
typedef enum
{
    TZ_STOR_SVC_GENERATE_KEY                = 0x00000001,
    TZ_STOR_SVC_SEAL_DATA                   = 0x00000002,
    TZ_STOR_SVC_UNSEAL_DATA                 = 0x00000003,
    TZ_STOR_SVC_IMPORT_KEY                  = 0x00000004,
    TZ_STOR_SVC_LAST_CMD_ENTRY              = 0x7FFFFFFF,
} tz_storage_service_cmd_t;

/*
 -------------------------------------------------------------------------------
    Generate Key Data Structures
 -------------------------------------------------------------------------------
 */
typedef struct tz_storage_service_gen_key_cmd
{
    tz_storage_service_cmd_t        cmd_id;
    tz_storage_service_key_blob_t   key_blob;
} tz_storage_service_gen_key_cmd_t;

typedef struct tz_storage_service_gen_key_resp
{
    tz_storage_service_cmd_t    cmd_id;
    int32                       status;
    uint32                      key_blob_size;
} tz_storage_service_gen_key_resp_t;

/*
 -------------------------------------------------------------------------------
    Seal Data Structures
 -------------------------------------------------------------------------------
 */
typedef struct tz_storage_service_seal_data_cmd
{
    tz_storage_service_cmd_t        cmd_id;
    tz_storage_service_key_blob_t   key_blob;
    uint8*                          plain_data;
    uint32                          plain_data_len;
    uint8*                          output_buffer;
    uint32                          output_len;
} tz_storage_service_seal_data_cmd_t;

typedef struct tz_storage_service_seal_data_resp {
  tz_storage_service_cmd_t  cmd_id;
  int32                     status;
  uint32                    sealed_data_len;
}tz_storage_service_seal_data_resp_t;


/**
 * Unseal Data Structures
 */
typedef struct tz_storage_service_unseal_data_cmd
{
    tz_storage_service_cmd_t        cmd_id;
    tz_storage_service_key_blob_t   key_blob;
    uint8*                          sealed_data;
    uint32                          sealed_dlen;
    uint8*                          output_buffer;
    uint32                          output_len;
} tz_storage_service_unseal_data_cmd_t;

typedef struct tz_storage_service_unseal_data_resp
{
    tz_storage_service_cmd_t    cmd_id;
    int32                       status;
    uint32                      unsealed_data_len;
} tz_storage_service_unseal_data_resp_t;

/*
 -------------------------------------------------------------------------------
    Symmetric Import Data Structures
 -------------------------------------------------------------------------------
 */
typedef struct tz_storage_service_import_key_cmd
{
    tz_storage_service_cmd_t        cmd_id;
    tz_storage_service_key_blob_t   key_blob;
    uint8*                          aes_key_p;
    uint32                          aes_key_len;
} tz_storage_service_import_key_cmd_t;

typedef struct tz_storage_service_import_key_resp
{
    tz_storage_service_cmd_t    cmd_id;
    int32                       status;
    uint32                      key_blob_size;
} tz_storage_service_import_key_resp_t;

/*
 -------------------------------------------------------------------------------
    Commands supported for the asymmetric signing and authentication service.
 -------------------------------------------------------------------------------
 */
typedef enum crypto_asym_rsa_cmd
{
    CRYPTO_STORAGE_GENERATE_KEY             = 0x00000001,
    CRYPTO_STORAGE_EXPORT_PUBKEY            = 0x00000002,
    CRYPTO_STORAGE_SIGN_DATA                = 0x00000003,
    CRYPTO_STORAGE_VERIFY_DATA              = 0x00000004,
    CRYPTO_STORAGE_IMPORT_KEY               = 0x00000005,
    CRYPTO_STORAGE_LAST_CMD_ENTRY           = 0x7FFFFFFF,
} crypto_asym_rsa_cmd_t;

/* This naming scheme is deprecated. Kept for backwards compatiblity. */
typedef enum crypto_asym_rsa_cmd crypto_asym_cmd_t;

/*
 -------------------------------------------------------------------------------
    Padding types supported
 -------------------------------------------------------------------------------
 */
typedef enum tz_asym_rsa_digest_pad_algo
{
    TZ_RSA_DIGEST_PADDING_NONE              = 0x00000000,
    TZ_RSA_PKCS115_SHA2_256                 = 0x00000001,
    TZ_RSA_PSS_SHA2_256                     = 0x00000002,
    TZ_RSA_MAX_VAL                          = 0x7FFFFFFF,
} tz_asym_rsa_digest_pad_algo_t;

/* This naming scheme is deprecated. Kept for backwards compatiblity. */
typedef enum tz_asym_rsa_digest_pad_algo tz_rsa_digest_pad_algo_t;
/*
 -------------------------------------------------------------------------------
    RSA Key Type definition
 -------------------------------------------------------------------------------
 */
typedef struct crypto_asym_rsa_key
{
    /* Validate the integrity of keyblob content */
    uint32                      magic_num;
    /* API version number */
    uint32                      version;
    tz_rsa_digest_pad_algo_t    digest_padding;
    /* Modulus (N) array [MSB...LSB] */
    uint8                       modulus[CRYPTO_ASYM_KEY_SIZE_MAX];
    /* Modulus array length */
    uint32                      modulus_size;
    /* public exponent (e) array [MSB...LSB] */
    uint8                       public_exponent[CRYPTO_ASYM_KEY_SIZE_MAX];
    /* public exponent array length */
    uint32                      public_exponent_size;
    /* Initial vector */
    uint8                       iv[CRYPTO_ASYM_IV_LENGTH];
    /* Encrypted Private Exponent(d) array [MSB...LSB] */
    uint8                       encrypted_private_exponent[CRYPTO_ASYM_KEY_SIZE_MAX];
    /* Encrypted Private Exponent array length */
    uint32                      encrypted_private_exponent_size;
    /* HMAC array */
    uint8                       hmac[CRYPTO_ASYM_HMAC_LENGTH];
} crypto_asym_rsa_key_t;

/* This naming scheme is deprecated. Kept for backwards compatiblity. */
typedef struct crypto_asym_rsa_key crypto_rsa_key_type;

/*
 -------------------------------------------------------------------------------
    Data structure for storing the encrypted key blob
 -------------------------------------------------------------------------------
 */
typedef struct crypto_asym_rsa_key_blob
{
    crypto_rsa_key_type*    key_material;
    uint32                  key_material_len;
} crypto_asym_rsa_key_blob_t;

/* This naming scheme is deprecated. Kept for backwards compatiblity. */
typedef struct crypto_asym_rsa_key_blob crypto_rsa_key_blob_type;

/*
 -------------------------------------------------------------------------------
    Parameters needed to generate an RSA key.
 -------------------------------------------------------------------------------
 */
typedef struct crypto_asym_rsa_keygen_params
{
    uint32                      modulus_size;
    uint64                      public_exponent;
    tz_rsa_digest_pad_algo_t    digest_pad_type;
} crypto_asym_rsa_keygen_params_t;

/*
 -------------------------------------------------------------------------------
    RSA Key Format.
 -------------------------------------------------------------------------------
 */
typedef enum crypto_asym_rsa_key_format
{
    CRYPTO_STORAGE_FORMAT_RAW_BYTES = 1,
} crypto_asym_rsa_key_format_t;

/* This naming scheme is deprecated. Kept for backwards compatiblity. */
typedef enum crypto_asym_rsa_key_format crypto_storage_rsa_key_format_t;

/*
 -------------------------------------------------------------------------------
    Assymetric key generation structures.
 -------------------------------------------------------------------------------
 */
typedef struct crypto_asym_rsa_gen_keypair_cmd
{
    crypto_asym_cmd_t               cmd_id;
    crypto_rsa_key_blob_type        key_blob;
    crypto_asym_rsa_keygen_params_t rsa_params;
} crypto_asym_rsa_gen_keypair_cmd_t;

/* This naming scheme is deprecated. Kept for backwards compatiblity. */
typedef struct crypto_asym_rsa_gen_keypair_cmd crypto_asym_gen_keypair_cmd_t;

typedef struct crypto_asym_rsa_gen_keypair_resp
{
    crypto_asym_cmd_t   cmd_id;
    int32               status;
    unsigned int        key_blob_size;
} crypto_asym_rsa_gen_keypair_resp_t;

/* This naming scheme is deprecated. Kept for backwards compatiblity. */
typedef struct crypto_asym_rsa_gen_keypair_resp crypto_asym_gen_keypair_resp_t;

/*
 -------------------------------------------------------------------------------
    Assymetric key import structures.
--------------------------------------------------------------------------------
 */
typedef struct crypto_asym_rsa_import_key_cmd
{
    crypto_asym_cmd_t           cmd_id;
    uint8                       modulus[CRYPTO_ASYM_KEY_SIZE_MAX];
    uint32                      modulus_size;
    uint8                       public_exponent[CRYPTO_ASYM_PUB_EXP_SIZE_MAX];
    uint32                      public_exponent_size;
    uint8                       private_exponent[CRYPTO_ASYM_KEY_SIZE_MAX];
    uint32                      private_exponent_size;
    tz_rsa_digest_pad_algo_t    digest_pad_type;
    /* Output */
    crypto_rsa_key_blob_type    key_blob;
} crypto_asym_rsa_import_key_cmd_t;

/* This naming scheme is deprecated. Kept for backwards compatiblity. */
typedef struct crypto_asym_rsa_import_key_cmd crypto_storage_rsa_import_key_cmd_t;

typedef struct crypto_asym_rsa_import_key_resp
{
    crypto_asym_rsa_cmd_t   cmd_id;
    int32                   status;
} crypto_asym_rsa_import_key_resp_t;

/* This naming scheme is deprecated. Kept for backwards compatiblity. */
typedef struct crypto_asym_rsa_import_key_resp crypto_storage_rsa_import_key_resp_t;

/*
 -------------------------------------------------------------------------------
    Assymetric key sign structures.
 -------------------------------------------------------------------------------
 */
typedef struct crypto_asym_rsa_sign_data_cmd
{
    crypto_asym_cmd_t           cmd_id;
    crypto_rsa_key_blob_type    key_blob;
    uint8*                      data;
    size_t                      data_len;
    uint8*                      signeddata;
    uint32                      signeddata_len;
} crypto_asym_rsa_sign_data_cmd_t;

/* This naming scheme is deprecated. Kept for backwards compatiblity. */
typedef struct crypto_asym_rsa_sign_data_cmd crypto_asym_sign_data_cmd_t;

typedef struct crypto_asym_rsa_sign_data_resp
{
    crypto_asym_cmd_t   cmd_id;
    size_t              sig_len;
    int32               status;
} crypto_asym_rsa_sign_data_resp_t;

/* This naming scheme is deprecated. Kept for backwards compatiblity. */
typedef struct crypto_asym_rsa_sign_data_resp crypto_asym_sign_data_resp_t;

/*
 -------------------------------------------------------------------------------
    Assymetric key verify structures.
 -------------------------------------------------------------------------------
 */
typedef struct crypto_asym_rsa_verify_data_cmd
{
    crypto_asym_cmd_t           cmd_id;
    crypto_rsa_key_blob_type    key_blob;
    uint8*                      signed_data;
    size_t                      signed_dlen;
    uint8*                      signature;
    size_t                      slen;
} crypto_asym_rsa_verify_data_cmd_t;

/* This naming scheme is deprecated. Kept for backwards compatiblity. */
typedef struct crypto_asym_rsa_verify_data_cmd crypto_asym_verify_data_cmd_t;

typedef struct crypto_asym_rsa_verify_data_resp
{
    crypto_asym_cmd_t   cmd_id;
    int32               status;
} crypto_asym_rsa_verify_data_resp_t;

/* This naming scheme is deprecated. Kept for backwards compatiblity. */
typedef struct crypto_asym_rsa_verify_data_resp crypto_asym_verify_data_resp_t;

/*
 -------------------------------------------------------------------------------
    Assymetric key export structures.
 -------------------------------------------------------------------------------
 */
typedef struct crypto_asym_rsa_export_key_cmd
{
    crypto_asym_cmd_t               cmd_id;
    crypto_rsa_key_blob_type        key_blob;
    crypto_storage_rsa_key_format_t export_format;
    uint8*                          modulus;
    uint32                          modulus_size;
    uint8*                          public_exponent;
    uint32                          public_exponent_size;
} crypto_asym_rsa_export_key_cmd_t;

/* This naming scheme is deprecated. Kept for backwards compatiblity. */
typedef struct crypto_asym_rsa_export_key_cmd crypto_storage_rsa_export_key_cmd_t;

typedef struct crypto_asym_rsa_export_key_resp
{
    crypto_asym_cmd_t   cmd_id;
    int32               status;
    uint32              modulus_size;
    uint32              public_exponent_size;
    uint32              exported_key_len;
} crypto_asym_rsa_export_key_resp_t;

/* This naming scheme is deprecated. Kept for backwards compatiblity. */
typedef struct crypto_asym_rsa_export_key_resp crypto_storage_rsa_export_key_resp_t;

/*
 -------------------------------------------------------------------------------
    Error codes specific for this feature. Returned as the status value of the
    response structure.
 -------------------------------------------------------------------------------
 */
typedef enum
{
    /* Operation successful */
    E_CE_STOR_SUCCESS                       = 0,

    /* Operation failed due to unknown err */
    E_CE_STOR_FAILURE                       = 1,
    /* Allocation from a memory pool failed */
    E_CE_STOR_NO_MEMORY                     = 2,
    /* Allocation from a memory pool failed */
    E_CE_STOR_NULL_PARAM                    = 3,
    E_CE_STOR_MEM_ALLOC_FALURE              = 4,
    /* Arg is not recognized */
    E_CE_STOR_INVALID_ARG                   = 10,
    /* Arg value is out of range */
    E_CE_STOR_OUT_OF_RANGE                  = 11,
    /* Ptr arg is bad address */
    E_CE_STOR_BAD_ADDRESS                   = 12,
    /* Expected data, received none */
    E_CE_STOR_NO_DATA                       = 13,
    /* Data failed sanity check, e.g. CRC */
    E_CE_STOR_BAD_DATA                      = 14,
    /* Data is correct, but contents invalid */
    E_CE_STOR_DATA_INVALID                  = 15,
    /* Data is not yet or not any more valid */
    E_CE_STOR_DATA_EXPIRED                  = 16,
    /* Data is too large to process */
    E_CE_STOR_DATA_TOO_LARGE                = 17,
    E_CE_STOR_INVALID_ARG_LEN               = 18,
    E_CE_STOR_SECURE_RANGE                  = 19,
    E_CE_STOR_DERIVED_KEY_FAIL              = 20,
    E_CE_STOR_PRNG_FAIL                     = 21,
    E_CE_STOR_RSA_INIT_KEY_FAIL             = 22,
    E_CE_STOR_AES_CCM_SET_PARAM_FAIL        = 23,
    E_CE_STOR_AES_CCM_ENC_FAIL              = 24,
    E_CE_STOR_AES_CCM_DEC_FAIL              = 25,
    E_CE_STOR_RSA_BIG_INT_INIT_FAIL         = 26,
    E_CE_STOR_RSA_SIG_FAIL                  = 27,
    E_CE_STOR_RSA_VERIFY_SIG_FAIL           = 28,
    E_CE_STOR_RSA_KEY_HASH_FAIL             = 29,
    E_CE_STOR_PAD_FAIL                      = 30,
    E_CE_STOR_HMAC_FAIL                     = 31,
    E_CE_STOR_DEC_PRIV_EXP_FAIL             = 32,
    E_CE_STOR_RESERVED                      = 0x7FFFFFFF,
} E_CE_STOR_ErrnoType;

#endif /* TZBSP_CRYPTO_API_H */


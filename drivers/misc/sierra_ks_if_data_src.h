/*
********************************************************************************
* Name:  sierra_ks_if_data_src.h
*
* Sierra Wireless keystore shared kernel file. This file contains information
* required by both kernel and the keystore data provider, and must be shared.
* Most likely, keystore data provider would be SBL.
*
* Since we do not have shared repositories, please make sure that if anything
* changes in this file, that data source provider header file gets updated as
* well.
*
*===============================================================================
* Copyright (C) 2019 Sierra Wireless Inc.
********************************************************************************
 */
#ifndef _SIERRA_KS_IF_DATA_SRC_H
#define _SIERRA_KS_IF_DATA_SRC_H

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif

/* Key types which could be stored in the key store. Note that Linux already
 * have key_t type for unrelated thing.
 */
typedef enum
{
    /* OEM key hash */
    OEM0,

    /* IMA ".system" key */
    IMA0,

    /* DM Verity key for rootfs */
    RFS0,

    /* DM Verify for Legato */
    LGT0
} keytype_t;

/* The actual shared data structure, single record. */
typedef struct
{
    /* version of this structure */
    uint32_t    version;

    /* csum of all elements except csum itself */
    uint32_t    crc32;

    /* Offset to the beginning of next record from the beginning of this record,
     * in bytes. Set to zero if next record does not exist.
     */
    uint32_t    next_rec;

    /* key type. This is actually keytype_t, but we need to make sure that this
       is 4 bytes wide. */
    uint32_t   key_type;

    /* Length of this key, in bytes. */
    uint32_t    key_length;

    /* Key data. We can use variable array or max size array here. */
    uint8_t     key[0];

} __attribute__((__packed__)) key_store_rec_t;

#endif

/* arch/arm/mach-msm/sierra_smem.h
 *
 * Copyright (C) 2012 Sierra Wireless, Inc
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
 */

#ifndef SIERRA_SMEM_H
#define SIERRA_SMEM_H

#ifdef __KERNEL__
#include <linux/types.h>
#endif

/* NOTE: this file is also used by LK so please keep this file generic */

/* Sierra shared memory
 *
 * A section of memory at the top of DDR is reserved for Sierra
 * boot-app messages, crash information, and other data.  This section
 * is non-initialized in order to preserve data across reboots.  The
 * size and base are (unfortunately) defined in multiple places.
 *
 * WARNING:  These definitions must be kept in sync
 *
 * boot_images/build/ms/9x45.target.builds
 * modem_proc/config/9645/cust_config.xml
 * modem_proc/sierra/bs/api/bsaddress.h
 * apps_proc/kernel/arch/arm/mach-msm/include/mach/sierra_smem.h
 * apps_proc/kernel/arch/arm/boot/dts/qcom/mdm9640.dtsi
 */

#ifndef DDR_MEM_BASE
#define DDR_MEM_BASE 0x80000000
#endif

#define SIERRA_MEM_SIZE           0x10000000 /* 256 MB */
#define SIERRA_SMEM_SIZE          0x00100000 /*   1 MB */
#define SIERRA_SMEM_BASE_PHY      ((DDR_MEM_BASE + SIERRA_MEM_SIZE) - (SIERRA_SMEM_SIZE))

/* Local constants and enumerated types */
/* below ER related defines and structures need to keep in sync with:
 * modem_proc/sierra/er/src/eridefs.h
 */
#define ERROR_START_MARKER  0x45524552 /* "ER" in ASCII */
#define ERROR_END_MARKER    0x45524552 /* "ER" in ASCII */
#define ERROR_USER          0x0101
#define ERROR_EXCEPTION     0x0202
#define ERROR_FATAL_ERROR   0x0404
#define ERROR_LOCK_MARKER   0x0303
#define ERROR_START_GLOBALTIME_MARKER   0x47744774
#define ERROR_END_GLOBALTIME_MARKER     0x47744774

#define MAX_SERIAL_LEN      20  /* must be larger than (NV_UE_IMEI_SIZE-1)*2 */
#define MAX_VER_LEN         24
#define DATE_TIME_LEN       16

#define ERROR_STRING_LEN    64
#define MAX_STACK_DATA      32
#define MAX_TASK_NAME       12
#define MAX_ARM_REGISTERS   15
#define MAX_EXT_REGISTERS   17
#define QDSP6_REG_SP        (29 - MAX_ARM_REGISTERS)  /* R29 = SP */
#define QDSP6_REG_FP        (30 - MAX_ARM_REGISTERS)  /* R30 = FP */
#define QDSP6_REG_LR        (31 - MAX_ARM_REGISTERS)  /* R31 = LR */

#define MAX_FORMAT_PARAM    4

#define DUMP_SET_FLAG       0x0001

#define BC_VALID_BOOT_MSG_MARKER           0xBABECAFEU   /* indicates message from Boot to App */
#define BC_MSG_MARKER_M                    0xFFFF0000U
#define BCBOOTAPPFLAG_DLOAD_MODE_M         0x00000008

#define ERDUMP_SAVE_CMD_START              0xFF00
#define ERDUMP_SAVE_CMD_ERRSTR             0xFF01
#define ERDUMP_SAVE_CMD_ERRDATA            0xFF02
#define ERDUMP_SAVE_CMD_FMTSTR             0xFF03
#define ERDUMP_SAVE_CMD_FMTDATA            0xFF04
#define ERDUMP_SAVE_CMD_REGISTERS          0xFF05
#define ERDUMP_SAVE_CMD_FRAME              0xFF06
#define ERDUMP_SAVE_CMD_END                0xFF0F
#define ERDUMP_PROC_TYPE_APPS              0x41505053 /* "APPS" in ascii hex */

/* Shared Memory Sub-region offsets.
 * Must be sync with bsmem.c(in MPSS). 
 */
#define BS_SMEM_CRC_SIZE                   0x0004   /* 4 bytes CRC value for each shared memory area */
/* Note: total size must be less than 1MB(0x100000)
 * (0x1000+0x4) + (0x400+0x4) + (0x1000+0x4) + (0x300+0x4) +
 * (0x2000+0x4) + (0x400+0x4) + (0x400+0x4) + (0x400+0x4) +
 * (0x814+0x4) + (0x10+0x4) + (0x20+0x4) + (0x20+0x4) +
 * (0x20+0x4) + (0x10+0x4) + (0x1000+0x4) + (0x4C+0x4) +
 * (0x5010+0x4) + (0x1000+0x4) + (0x8040+0x4) + (0x10d+0x4) + (0x40+0x4)
 * = 0x14DD1
 */
#define BS_SMEM_CWE_SIZE                   0x1000   /* 512 * 8 slots              */
#define BS_SMEM_MSG_SIZE                   0x0400   /* 1 kB, fixed for expansion  */
#define BS_SMEM_ERR_SIZE                   0x1000   /* (0x07F8 + 0x07F8 + 0x0010) */
#define BS_SMEM_ERR_DUMP_SIZE              0x7F8
#define BS_SMEM_USBD_SIZE                  0x0300
#define BS_SMEM_CACHE_SIZE                 0x2000
#define BS_SMEM_EFSLOG_SIZE                0x0400   /* 1 KB */
#define BS_SMEM_FWUP_SIZE                  0x0400   /* 1 KB */
#define BS_SMEM_IM_SIZE                    0x0400   /* 1 KB */
#define BS_SMEM_MIBIB_SIZE                 0x0814   /* 2KB + 20 bytes */
#define BS_SMEM_MODE_SIZE                  0x0010   /* 16 bytes for mode switching */
#define BS_SMEM_DSSD_SIZE                  0x0020   /* 32 bytes for dual system boot up */
#define BS_SMEM_SECB_SIZE                  0x0020   /* 32 bytes for dual system boot up */
#define BS_SMEM_ABANDON_SIZE               0x0020   /* Abandon 32 bytes for co-work msg */
#define BS_SMEM_PR_SW_SIZE                 0x0010   /* 16 bytes for interlock between program refresh and normal SW update */
#define BS_SMEM_LKC_SIZE                   0x1000   /* 4KB bytes for linux kernel crash msg */
#define BS_SMEM_CR_SKU_SIZE                0x004C   /* 76 bytes for Cross SKU update */
#define BS_SMEM_APP_DUMP_SIZE              0x5010   /* 2 KB*10 + 16  for app dump info */
#define BS_SMEM_ELOG_SIZE                  0x1000   /* 4KB for EE log */
#define BS_SMEM_EFS_RW_LOG_SIZE            0x8040   /* for EFS reading/writing log */
#define BS_SMEM_FTUR_CNFG_SIZE             0x0110   /* 272 bytes for reliability feature configuration*/
#define BS_SMEM_COWORK_SIZE                0x0040   /* 64 bytes for co-work msg */
#define BS_SMEM_ERESTORE_SIZE              0x0010   /* 16 byte for EFS restore info */
#define BS_SMEM_NFATAL_LOG_SIZE            0x2010   /* For non fatal error log */

#define BSMEM_CWE_OFFSET                   (0)
#define BSMEM_MSG_OFFSET                   (BSMEM_CWE_OFFSET  + BS_SMEM_CWE_SIZE + BS_SMEM_CRC_SIZE )
#define BSMEM_ERR_OFFSET                   (BSMEM_MSG_OFFSET  + BS_SMEM_MSG_SIZE + BS_SMEM_CRC_SIZE )
#define BSMEM_USBD_OFFSET                  (BSMEM_ERR_OFFSET  + BS_SMEM_ERR_SIZE + BS_SMEM_CRC_SIZE )
#define BSMEM_CACHE_OFFSET                 (BSMEM_USBD_OFFSET + BS_SMEM_USBD_SIZE + BS_SMEM_CRC_SIZE )
#define BSMEM_EFSLOG_OFFSET                (BSMEM_CACHE_OFFSET+ BS_SMEM_CACHE_SIZE + BS_SMEM_CRC_SIZE )
#define BSMEM_FWUP_OFFSET                  (BSMEM_EFSLOG_OFFSET + BS_SMEM_EFSLOG_SIZE + BS_SMEM_CRC_SIZE )
#define BSMEM_IM_OFFSET                    (BSMEM_FWUP_OFFSET + BS_SMEM_FWUP_SIZE + BS_SMEM_CRC_SIZE )
#define BSMEM_MIBIB_OFFSET                 (BSMEM_IM_OFFSET + BS_SMEM_IM_SIZE + BS_SMEM_CRC_SIZE )
#define BSMEM_MODE_OFFSET                  (BSMEM_MIBIB_OFFSET + BS_SMEM_MIBIB_SIZE + BS_SMEM_CRC_SIZE )
#define BSMEM_DSSD_OFFSET                  (BSMEM_MODE_OFFSET + BS_SMEM_MODE_SIZE + BS_SMEM_CRC_SIZE )
#define BSMEM_SECB_OFFSET                  (BSMEM_DSSD_OFFSET + BS_SMEM_DSSD_SIZE + BS_SMEM_CRC_SIZE )
#define BSMEM_ABANDON_OFFSET               (BSMEM_SECB_OFFSET + BS_SMEM_SECB_SIZE + BS_SMEM_CRC_SIZE )
#define BSMEM_PR_SW_OFFSET                 (BSMEM_ABANDON_OFFSET + BS_SMEM_ABANDON_SIZE + BS_SMEM_CRC_SIZE )
#define BSMEM_LKC_OFFSET                   (BSMEM_PR_SW_OFFSET + BS_SMEM_PR_SW_SIZE + BS_SMEM_CRC_SIZE )
#define BSMEM_CR_SKU_OFFSET                (BSMEM_LKC_OFFSET + BS_SMEM_LKC_SIZE + BS_SMEM_CRC_SIZE )
#define BSMEM_APP_DUMP_OFFSET              (BSMEM_CR_SKU_OFFSET + BS_SMEM_CR_SKU_SIZE + BS_SMEM_CRC_SIZE )
#define BSMEM_ELOG_OFFSET                  (BSMEM_APP_DUMP_OFFSET + BS_SMEM_APP_DUMP_SIZE + BS_SMEM_CRC_SIZE)
#define BSMEM_EFS_RW_LOG_OFFSET            (BSMEM_ELOG_OFFSET + BS_SMEM_ELOG_SIZE + BS_SMEM_CRC_SIZE)
#define BSMEM_FTUR_CNFG_OFFSET             (BSMEM_EFS_RW_LOG_OFFSET + BS_SMEM_EFS_RW_LOG_SIZE + BS_SMEM_CRC_SIZE)
#define BSMEM_COWORK_OFFSET                (BSMEM_FTUR_CNFG_OFFSET + BS_SMEM_FTUR_CNFG_SIZE + BS_SMEM_CRC_SIZE )
#define BSMEM_EFS_RESTORE_OFFSET           (BSMEM_COWORK_OFFSET + BS_SMEM_COWORK_SIZE + BS_SMEM_CRC_SIZE)
#define BSMEM_NFATAL_LOG_OFFSET            (BSMEM_EFS_RESTORE_OFFSET + BS_SMEM_ERESTORE_SIZE + BS_SMEM_CRC_SIZE)

/* the buffer len to hold the linux  kmsg when kernel crash
 * if CONFIG_LOG_BUF_SHIFT is not define,is 128KB
 */
#define LKC_KMSG_LEN (1 << CONFIG_LOG_BUF_SHIFT)

/* get more info from the start of kernel panic */
#define LKC_STR_EXTR_LEN (256)

/* kernel log time string length */
#define LKC_STR_TIME_LEN (15)

/* specify the kernel crash kmsg string*/
#define LKC_PANIC_STR "Internal error:"

/* 32-bit random magic numbers - written to indicate that message
 * structure in the shared memory region was initialized
 */
#define BC_SMEM_MSG_MAGIC_BEG      0x92B15380U
#define BC_SMEM_MSG_MAGIC_END      0x31DDF742U

/* Reliability feature configuration data region*/
#define RELIABILITY_CNFG_FTUR_BEG           0x434e4647   /* "CNFG" in ASCII*/
#define RELIABILITY_CNFG_FTUR_END           0x434e4647   /* "CNFG" in ASCII*/
#define CNFG_FTUR_SIZE                      255          /* reliability configuration array length */
#define INVALID_RELIABILITY_CNFG_DATA       0xFF         /* Invalid reliability configuration data */
#define SPI_RECOVERY_MODE                   2            /* SPI recovery mode */
#define USB_RECOVERY_MODE                   1            /* USB recovery mode */
#define DEFAULT_RECOVERY_MODE               1            /* USB recovery mode by default*/
#define ENSABLE_PROGRAM_SCRUGB              2            /* Enable program scrub */
#define DEFAULT_PROGRAM_SCRUGB              1            /* Disable program scrub by default*/
#define SUPPORT_GPIO_TRIG_RECOVERY          2            /* Support SPI GPIO trigger recovery mode */
#define DISABLE_GPIO_TRIG_RECOVERY          1            /* Disable SPI GPIO trigger recovery mode by default*/
#define DEFAULT_MIRROR_SET                  2            /* Both systems are mirror by default*/
#define ENABLE_SIERRA_UART_LOG              1            /* Enable sierra uart log*/
#define DISABLE_SIERRA_UART_LOG             2            /* Disable sierra uart log*/

#define BS_SMEM_SECBOOT_MAGIC_BEG      0x5342494DU
#define BS_SMEM_SECBOOT_MAGIC_END      0x5342494DU


/* Version of the shared memory structure which is used by this 
 * module.  Module is compatible with earlier versions 
 */
#define BC_SMEM_MSG_VERSION                  2
#define BC_SMEM_MSG_CRC32_VERSION_MIN        2

#define BC_MSG_LAUNCH_CODE_INVALID ((uint32_t)(-1))
#define BC_MSG_RECOVER_CNT_INVALID ((uint32_t)(-1))
#define BC_MSG_HWCONFIG_INVALID    ((uint32_t)(-1))
#define BC_MSG_USB_DESC_INVALID    ((void *)(-1))
#define BC_COMP_CHECK              0xFFFFFFFF

/*
  Reset type defination start
  Please note that the values between BS_BCMSG_RTYPE_MIN and BS_BCMSG_RTYPE_MAX must be successive,
  and sync up the new definations to bsudefs.h/sierra_smem.h/atbc.c together.
*/
#define BS_BCMSG_RTYPE_INVALID                 ((uint32_t)(-1))
#define BS_BCMSG_RTYPE_MIN                     ((uint32_t)(1))
#define BS_BCMSG_RTYPE_POWER_CYCLE             BS_BCMSG_RTYPE_MIN        /* Normal power up, power cycle */
#define BS_BCMSG_RTYPE_MP_SOFTWARE             ((uint32_t)(2))           /* Software reset in MPSS*/
#define BS_BCMSG_RTYPE_LINUX_SOFTWARE          ((uint32_t)(3))           /* Software reset in Linux */
#define BS_BCMSG_RTYPE_HARDWARE                ((uint32_t)(4))           /* Hardware reset */
#define BS_BCMSG_RTYPE_MP_CRASH                ((uint32_t)(5))           /* MPSS crash */
#define BS_BCMSG_RTYPE_LINUX_CRASH             ((uint32_t)(6))           /* Linux crash */
#define BS_BCMSG_RTYPE_SW_UPDATE_IN_SBL        ((uint32_t)(7))           /* SW update in SBL */
#define BS_BCMSG_RTYPE_SW_UPDATE_IN_LK         ((uint32_t)(8))           /* SW update in LK */
#define BS_BCMSG_RTYPE_SW_UPDATE_IN_LINUX      ((uint32_t)(9))           /* SW update in Linux */
#define BS_BCMSG_RTYPE_UNKNOWN                 ((uint32_t)(10))          /* Unknown reset */
#define BS_BCMSG_RTYPE_MAX                     BS_BCMSG_RTYPE_UNKNOWN

#define BS_BCMSG_RTYPE_IS_SET                  ((uint32_t)(0x00534554))  /* SET */
#define BS_BCMSG_RTYPE_IS_CLEAR                ((uint32_t)(0x00434C52))  /* CLR */
/*
  Reset type defination end
*/

#define BC_MSG_SIZE_MAX                    340 /* 1/3 of 1kB, on 4-byte boundaries */
#define BC_SMEM_MSG_SZ                     (sizeof(struct bc_smem_message_s))

/* CRC check on the structure minus crc32 field */
#define BC_MSG_CRC_SZ             (BC_SMEM_MSG_SZ - sizeof(uint32_t))

/* 32-bit random magic numbers - written to indicate that message
 * structure in the shared memory region was initialized
 */
#define BS_SMEM_MODE_MAGIC_BEG      0x6D6F6465U
#define BS_SMEM_MODE_MAGIC_END      0x6D6F6465U

/* bs_smem_mode_switch CRC32 field*/
#define BS_SMEM_MODE_SZ            (sizeof(struct bs_smem_mode_switch))
#define BS_MODE_CRC_SIZE           (BS_SMEM_MODE_SZ - sizeof(uint32_t))

/* bscoworkmsg CRC32 field*/
#define BS_SMEM_COWORK_SZ            (sizeof(struct bscoworkmsg))
#define BS_COWORK_CRC_SIZE           (BS_SMEM_COWORK_SZ - sizeof(uint32_t))

/* 32-bit random magic numbers - written to indicate that message
 * structure in the shared memory region was initialized
 */
#define BS_SMEM_COWORK_MAGIC_BEG         0xCD3AE0B5U  /*cooperation mode message start & end marker*/
#define BS_SMEM_COWORK_MAGIC_END         0xCD3AE0B5U  /*cooperation mode message start & end marker*/

/* Padding inside bc_smem_message_s
 *
 * This padding is used to save blank space inside the message
 * structure for future expansion - i.e. newer revisions of the
 * message structure.  Backward compatibility is maintained by fixing
 * the locations of the currently defined fields.
 */
#define BCMSG_MAILBOX_PAD                  ((\
                                            BC_MSG_SIZE_MAX \
                                            - (3 * sizeof(uint32_t)) \
                                            - (2 * sizeof(struct bsmsg_mailbox_s))\
                                            ) / 2 )
/* MSG mailbox offset: MSG region */
#define BSMEM_MSG_APPL_MAILBOX_OFFSET      (BSMEM_MSG_OFFSET + (BC_MSG_SIZE_MAX * BCMSG_MBOX_APPL))
#define BSMEM_MSG_BOOT_MAILBOX_OFFSET      (BSMEM_MSG_OFFSET + (BC_MSG_SIZE_MAX * BCMSG_MBOX_BOOT))
#define BSMEM_MSG_MODM_MAILBOX_OFFSET      (BSMEM_MSG_OFFSET + (BC_MSG_SIZE_MAX * BCMSG_MBOX_MODM))

#define BC_MSG_B2A_ADB_EN                  0x0000000000000004ULL
#define BC_MSG_B2A_DLOAD_MODE              0x0000000000000008ULL
#define BC_MSG_A2B_BOOT_HOLD               0x0000000000000001ULL

/* values from imswidefs.h and must keep in sync */
#define IMSW_SMEM_MAGIC_BEG                0x92B15380U
#define IMSW_SMEM_MAGIC_END                0x31DDF742U
#define IMSW_SMEM_MAGIC_RECOVERY           0x52425679U

/* MIBIB region constant */
#define MIBIB_SMEM_MAGIC_BEG                0x4D494242U  /* "MIBB" in ASCII */
#define MIBIB_SMEM_MAGIC_END                0x4D494245U  /* "MIBE" in ASCII */

/* MIBIB image state machine, for MIBIB smart update feature */
#define MIBIB_TO_UPDATE_IN_SBL              0xBBDAEFA0U  /* LK post to SBL, to smart update MIBIB */
#define MIBIB_TO_UPDATE_IN_SBL_PHASE1       0xBBDAEF0FU  /* SBL updated MIBIB and SBL, SBL should go on update TZ, RPM, LK */
#define MIBIB_UPDATED_IN_SBL                0xBBDAEFAFU  /* SBL post to LK, smart update MIBIB done. LK should update whole spkg in this state */
#define MIBIB_UPDATE_CLEAR                  0x00000000U  /* LK clear state machine when spkg update done */

/* DSSD region constant. It is necessary to sync in dsudefs.h(in mpss), 
 * sierra_smem.h(in kernel) and swidssd.h(in APSS).
 */
#define DS_MAGIC_NUMBER                     0x6475616C  /* "dual" */
#define DS_SSID_SUB_SYSTEM_1                1           /* sub system 1 */
#define DS_SSID_SUB_SYSTEM_2                2           /* sub system 2 */
#define DS_SSID_NOT_SET                     0xFF        /* Used during parameter delivery if SSIDs not set */
#define DS_SYSTEM_1                         0x73797331  /* "sys1" */ 
#define DS_SYSTEM_2                         0x74656D32  /* "tem2" */
#define DS_OUT_OF_SYNC                      0x4F6F5300  /* "OoS" */
#define DS_IS_SYNC                          0x73796E63  /* "sync" */
#define DS_EFS_CORRUPTION                   0x45465343  /* "EFSC" */
#define DS_BOOT_UP_CHANGED                  0x6368616E  /* "chan" */
#define DS_FLAG_NOT_SET                     0xFFFFFFFF  /* Used during parameter delivery if 'swap_reason',  'sw_update_state', 
                                                                                                    * 'out_of_sync', 'efs_corruption_in_sw_update' and 'edb_in_sw_update' not set
                                                                                                    */
/* Bitmasks for images
 * 1. It is used for 'bad image flag' 
 * 2. It is 64 bits length in order to consider future products may use different images
 */
#define DS_IMAGE_CLEAR_FLAG                 0x0 /* Used during parameter delivery if 'bad image flag' is cleared */
#define DS_IMAGE_SBL                        (1 << 0)   /* SBL */
#define DS_IMAGE_MIBIB                      (1 << 1)   /* MIBIB */
#define DS_RESERVED_IMAGE_MASK_1            (1 << 2)   /* Reserved */
#define DS_IMAGE_SEDB                       (1 << 3)   /* SEDB partition */
#define DS_RESERVED_IMAGE_MASK_2            (1 << 4)   /* Reserved */
#define DS_IMAGE_TZ_1                       (1 << 5)   /* TZ of system 1 */
#define DS_IMAGE_TZ_2                       (1 << 6)   /* TZ of system 2 */
#define DS_IMAGE_RPM_1                      (1 << 7)   /* RPM of system 2 */
#define DS_IMAGE_RPM_2                      (1 << 8)   /* RPM of system 1 */
#define DS_IMAGE_MODEM_1                    (1 << 9)   /* MODEM of system 1 */
#define DS_IMAGE_MODEM_2                    (1 << 10)  /* MODEM of system 2 */
#define DS_IMAGE_ABOOT_1                    (1 << 11)  /* LK of system 1 */
#define DS_IMAGE_ABOOT_2                    (1 << 12)  /* LK of system 2 */
#define DS_IMAGE_BOOT_1                     (1 << 13)  /* Kernel of system 1 */
#define DS_IMAGE_BOOT_2                     (1 << 14)  /* Kernel of system 2 */
#define DS_IMAGE_SYSTEM_1                   (1 << 15)  /* Root file system of system 1 */
#define DS_IMAGE_SYSTEM_2                   (1 << 16)  /* Root file system of system 2 */
#define DS_IMAGE_USERDATA_1                 (1 << 17)  /* Legato FRM of system 1 */
#define DS_IMAGE_USERDATA_2                 (1 << 18)  /* Legato FRM of system 2 */
#define DS_IMAGE_CUSTOMERAPP_1              (1 << 19)  /* 'customer0' partition which stores customer application of system 1 */
#define DS_IMAGE_CUSTOMERAPP_2              (1 << 20)  /* 'customer1' partition which stores customer application of system 2 */
#define DS_IMAGE_CUSTOMERAPP_3              (1 << 21)  /* 'customer2' partition which stores customer application of system 2 */
#define DS_IMAGE_FLAG_NOT_SET               0xFFFFFFFFFFFFFFFF /* Used during parameter delivery if 'bad image flag' not set */


/* Interlock between program refresh and normal SW update. 
 * It is necessary to sync in prudefs.h(in mpss) and sierra_smem.h(in kernel) 
 */
#define PR_SW_UDATE_MAGIC_NUMBER 0x70727377  /* "prsw" */
#define PR_SW_UPDATE_CLEAR_FLAG  0x0         /* Set it if program refresh or normal SW update finished */
#define PR_IS_IN_PROGRESS        0x70720000  /* "pr" */
#define SW_UPDATE_IN_PROGRESS    0x73777570  /* "swup" */

/* Cross SKU region constant */
#define CROSS_SKU_SMEM_MAGIC_BEG            0x43524F42U  /* "CROB" in ASCII */
#define CROSS_SKU_SMEM_MAGIC_END            0x43524F45U  /* "CROE" in ASCII */

#define NV_SWI_PRODUCT_SKU_SIZE  32
#define CNFG_FTUR_SIZE  255


/* Bitmask for bsfunctions of BS_SMEM_REGION_COWORK */
#define BSFUNCTIONS_HSIC 0x00000001
#define BSFUNCTIONS_SERVICEPINDISABLE (0x00000001 << 1)
#define BSFUNCTIONS_POWERFAULT (0x00000001 << 2)
#define BSFUNCTIONS_FORCERESET (0x00000001 << 3)
#define BSFUNCTIONS_WARMRESET  (0x00000001 << 4)

/************
 *
 * Name:     ds_swap_reason_e
 *
 * Purpose:  To enumerate all DS swap reasons
 *
 * Notes:    None
 *
 ************/
enum ds_swap_reason_e
{
  DS_SWAP_REASON_MIN = 0,
  DS_SWAP_REASON_NONE = DS_SWAP_REASON_MIN,            /* No swap since power up */
  DS_SWAP_REASON_BAD_IMAGE,                            /* Bad image detected */
  DS_SWAP_REASON_SW_UPDATE,                            /* Normal SW update */
  DS_SWAP_REASON_AT_COMMAND,                           /* AT command trigger */
  DS_SWAP_REASON_APPS,                                 /* Legato API trigger */
  DS_SWAP_REASON_6_ABNORMAL_RESET,                     /* 6 abnormal reset trigger */
  DS_SWAP_REASON_MAX = DS_SWAP_REASON_6_ABNORMAL_RESET,/* End */
};

/************
 *
 * Name:     bcmsg_mailbox_e
 *
 * Purpose:  Enumerated list of image types, useful to control behavior
 *           at run-time
 *
 * Notes:
 *
 ************/
enum bcmsg_mailbox_e
{
  BCMSG_MBOX_MIN  = 0,
  BCMSG_MBOX_BOOT = BCMSG_MBOX_MIN,
  BCMSG_MBOX_MODM,
  BCMSG_MBOX_APPL,
  BCMSG_MBOX_MAX  = BCMSG_MBOX_APPL,
  BCMSG_MBOX_NUM,
};

enum blfeaturecnfg
{
    BC_FTURCNFG_MIN,
    BC_FTURCNFG_DUMP_MODE = BC_FTURCNFG_MIN,
    BC_FTURCNFG_RECOVERY_MODE,
    BC_FTURCNFG_SIERRA_UART_LOG_FLAG,
    BC_FTURCNFG_MIRROR_SET,
    BC_FTURCNFG_PROGRAM_SCRUB,
    BC_FTURCNFG_POWER_FAULT_MODE,
    BC_FTURCNFG_POWER_FAULT_GPIO,
    BC_FTURCNFG_SUP_GPIO_TRIG_RCVY,
    BC_FTURCNFG_MAX = BC_FTURCNFG_SUP_GPIO_TRIG_RCVY,
};

/************
 *
 * Name:     bs_prod_family_e
 *
 * Purpose:  To enumerate product family types
 *
 * Members:  See below
 *
 * Notes:    need to sync with modem /sierra/bs/api/bsudef.h
 *
 ************/
enum bs_prod_family_e
{
  BS_PROD_FAMILY_QCTMTP,           /* Qualcomm MTP 9x30                       */
  BS_PROD_FAMILY_NONE,             /* product family type NONE                */
  BS_PROD_FAMILY_EM,               /* product family Embedded Module          */
  BS_PROD_FAMILY_MC,               /* product family MiniCard                 */
  BS_PROD_FAMILY_AR,               /* product family Automotive               */
  BS_PROD_FAMILY_WP,               /* product family WP                       */
  BS_PROD_FAMILY_UNKNOWN,          /* product family Unknown                  */
  BS_PROD_FAMILY_INVALID = 0xFF    /* product family Invalid                  */
};

/************
 *
 * Name:     bshwtype
 *
 * Purpose:  To enumerate hardware types
 *
 * Members:  See below
 *
 * Notes:    need to sync with modem /sierra/bs/api/bsudef.h
 *
 ************/
enum bshwtype
{
  BSQCTMTP,                     /* Qualcomm MTP 9x30                          */
  BSHWNONE,                     /* HW type NONE (Fuse has not been blown yet) */
  BSAR7582,                     /* 0x02 - Automotive 7582                     */
  BSAR7584,                     /* 0x03 - Automotive 7584                     */
  BSAR7586,                     /* 0x04 - Automotive 7586                     */
  BSAR7586_NB7_NB28,            /* 0x05 - Automotive 7586 for Ficosa          */
  BSAR7588,                     /* 0x06 - Automotive 7588                     */
  BSAR8582,                     /* 0x07 - Automotive 8582                     */
  BSAR7582_NC_NB7_NB13,         /* 0x08 - Automotive 7582 without codec       */
  BSAR7584_NC,                  /* 0x09 - Automotive 7584 without codec       */
  BSAR7586_NC_NB7,              /* 0x0A - Automotive 7586 without codec       */
  BSAR7586_NC_NB7_NB28,         /* 0x0B - Automotive 7586 without codec       */
  BSAR7588_NC,                  /* 0x0C - Automotive 7588 without codec       */
  BSAR8582_NC,                  /* 0x0D - Automotive 8582 without codec       */
  BSWP7601,                     /* 0x0E - WP7601                              */
  BSWP7603,                     /* 0x0F - WP7603                              */
  BSAR7584_NB28A,               /* 0x10 - Automotive 7584                     */
  BSAR7582_NB13,                /* 0x11 - Automotive 7582 no band 13          */
  BSAR7584_NC_NB28A,            /* 0x12 - Automotive 7584 without codec       */
  BSAR7582_NC_NB7,              /* 0x13 - Automotive 7582 without codec&B7    */

  /* Note: Add these marcos is just for AV module compiling.
     Need to re-write according to MDM9x40 hwtype later */
#ifdef STUB_APPS
  BSAC770S,                 /* 0x02 - AirCard 770S (HANSEL) */
  BSMC7355,                 /* 0x03 - MiniCard 7355 */
  BSAR7550,                 /* 0x04 - Automotive 7550 */
  BSAR7552,                 /* 0x05 - Automotive 7552 */
  BSAR7554,                 /* 0x06 - Automotive 7554 */
  BSEM7355,                 /* 0x07 - Embedded Module 7355 */
  BSAC340U,                 /* 0x08 - AirCard 340U (GRADY) */
  BSWP7100,                 /* 0x09 - WP7100 (for Verizon) */
  BSWP7102,                 /* 0x0A - WP7102 (for AT&T) */
  BSWP7104,                 /* 0x0B - WP7104 (for EU&APAC) */
  BSMC7305,                 /* 0x0C - MiniCard 7305 */
  BSEM7305,                 /* 0x0D - Embedded Module 7305 */
  BSAC342U,                 /* 0x0E - AirCard 342U (OSSO) */
  BSAC341U,                 /* 0x0F - AirCard 341U */
  BSEM7655,                 /* 0x10 - EM7655 (TFF version of EM7355) */
  BSMC8805,                 /* 0x11 - MiniCard 8805 */
  BSEM8805,                 /* 0x12 - Embeddded Module 8805 */
  BSAC771S,                 /* 0x13 - AirCard 771S */
  BSYW7X55,                 /* 0x14 - LGA module - experimental */
  BSWP7100_INSIM,           /* 0x15 - WP7100 (with In-SIM) */
  BSWP7102_INSIM,           /* 0x16 - WP7102 (with In-SIM) */
  BSWP7104_INSIM,           /* 0x17 - WP7104 (with In-SIM) */
  BSMC7350,                 /* 0x18 - MiniCard 7350 (for Verizon & Sprint) */
  BSMC7350L,                /* 0x19 - MiniCard 7350-L (for Verizon)*/
  BSMC7802,                 /* 0x1A - MiniCard 7802 (for AT&T) */
  BSMC7304,                 /* 0x1B - MiniCard 7304 (for EU) */
  BSWP7100_LARGER_MEMORY,   /* 0x1C - WP7100 (with Larger Memory Design) */
  BSWP7102_LARGER_MEMORY,   /* 0x1D - WP7102 (with Larger Memory Design) */
  BSWP7104_LARGER_MEMORY,   /* 0x1E - WP7104 (with Larger Memory Design) */
  BSEM7330,                 /* 0x1F - Embedded Module 7330 */
  BSMC7330,                 /* 0x20 - MiniCard 7330 */
  BSAC343U,                 /* 0x21 - AirCard 343U */
  BSMC7371,                 /* 0x22 - MiniCard 7371 */
  BSAC778S,                 /* 0x23 - AirCard 778S */
  BSAR7550_LARGER_MEMORY,   /* 0x24 - Automotive 7550  (with Larger Memory Design) */
  BSAR7552_LARGER_MEMORY,   /* 0x25 - Automotive 7552  (with Larger Memory Design) */
  BSAR7554_LARGER_MEMORY,   /* 0x26 - Automotive 7554  (with Larger Memory Design) */
  BSWP7100_NEW,             /* 0x27 - WP7100 with large memory based on AR PCB */
  BSWP7102_NEW,             /* 0x28 - WP7102 with large memory based on AR PCB */
  BSWP7104_NEW,             /* 0x29 - WP7104 with large memory based on AR PCB */
  BSMC7354,                 /* 0x2A - MiniCard 7354 */
  BSAR7558_LARGER_MEMORY,   /* 0x2B - Automotive 7558  (with Larger Memory Design) */
  BSAR7556,                 /* 0x2C - Automotive 7556 */
  BSWP75XX,                 /* 0x2D - WP75xx - WP75 family, RF board unknown */
  BSWP85XX,                 /* 0x2E - WP85xx - WP85 family, RF board unknown */
  BSWP8548,                 /* 0x2F - WP8548 */
  BSWP8548G,                /* 0x30 - WP8548-G */
  BSAR8652,                 /* 0x31 - Automotive 8652 */
  BSAR7556_LARGER_MEMORY,   /* 0x32 - Automotive 7556 (with Larger Memory Design) */
  BSAR7554RD,               /* 0x33 - Automotive 7554 RD */
  BSAR7552RD,               /* 0x34 - Automotive 7552 RD */
  BSWP7500,                 /* 0x35 - WP7500 */
  BSWP7500G,                /* 0x36 - WP7500-G*/
  BSWP7501,                 /* 0x37 - WP7501 */
  BSWP7501G,                /* 0x38 - WP7501-G */
  BSWP7502,                 /* 0x39 - WP7502 */
  BSWP7502G,                /* 0x3A - WP7502-G */
  BSWP7504,                 /* 0x3B - WP7504 */
  BSWP7504G,                /* 0x3C - WP7504-G */
#endif
  BSHWUNKNOWN,                  /* Unknown HW                                 */
  BSHWINVALID = 0xFF            /* Invalid HW                                 */
};

/* Structures */
/*************
 *
 * Name:     bs_resin_timer
 *
 * Purpose:  Contains resin timer
 *
 * Members:  see below
 *
 * Notes:
 *
 **************/
struct __attribute__((packed)) bs_resin_timer
{
  uint8_t s1_timer;
  uint8_t s2_timer;
};

/*************
 *
 * Name:     sER_DATA - ER Data structure
 *
 * Purpose:  Contains ER data dumps
 *
 * Members:  see below
 *
 * Notes:   make sure uint32 fields is 4-byte aligned
 *
 **************/
struct __attribute__((packed)) sER_DATA
{
  uint32_t start_marker;                   /* Magic number marking the satart */
  uint32_t program_counter;                /* PC at the time of crash */
  uint32_t cpsr;                           /* Program Status Register */
  uint32_t registers[MAX_ARM_REGISTERS];   /* registers */
  uint32_t ext_registers[MAX_EXT_REGISTERS]; /* extra register set for QDSP6 (R15-R31) */
  uint32_t stack_data[MAX_STACK_DATA];     /* Stack dump at the time of crash */
  uint32_t error_source;                   /* user or exception vector */
  uint32_t flags;                          /* bit-mapped flag */
  uint32_t error_id;                       /* unique error ID */
  uint32_t proc_type;                      /* which processor caused the crash */
  uint32_t time_stamp;                     /* up time in seconds since power up */
  uint32_t line;                           /* line number of the crash */
  char     file_name[ERROR_STRING_LEN];    /* file name of the crash */
  char     error_string[ERROR_STRING_LEN]; /* Null-terminated crash message string */
  uint32_t param[MAX_FORMAT_PARAM];        /* params for error_string */
  char     aux_string[ERROR_STRING_LEN];   /* 2nd error string - used to store QDSP FW crash info */
  char     task_name[MAX_TASK_NAME];       /* task name or ID that caused the crash */
  char     app_ver[MAX_VER_LEN];           /* APPL release at the time of crash */
  char     boot_ver[MAX_VER_LEN];          /* BOOT release at the time of crash */
  char     swoc_ver[MAX_VER_LEN];          /* SWoC release at the time of crash */
  char     serial_num[MAX_SERIAL_LEN];     /* modem IMEI */
  char     date_time[DATE_TIME_LEN];       /* date/time at the time of crash */
  uint32_t reserved[MAX_STACK_DATA];       /* reserved for future use */
  uint32_t end_marker;                     /* end marker */
};

/************
 *
 * Name:     bsmsg_mailbox_s
 *
 * Purpose:  Message structure writen by any image but ready only by the
 *           recipient image.  Used for passing information across reboot
 *           cycles
 *
 * Notes:    Structure is packed and uses fixed-width types to ensure
 *           compatibility between images and processors
 *
 *           New versions of this structure must retain all fields of
 *           version 1 in order to maintain backward compatibility
 *
 *           Must reside in uninitialized shared memory
 *
 ************/
struct __attribute__((packed)) bsmsg_mailbox_s
{
  uint64_t flags;                        /* message flags          */
  uint32_t loopback;                     /* loopback flags         */
  uint32_t recover_cnt;                  /* Smart Recovery counter */
  uint32_t launchcode;                   /* launch code            */
  uint32_t hwconfig;                     /* hardware configuration */
  void   * usbdescp;                     /* USB descriptor pointer */
  uint64_t clr_flags;                    /* flags to clear, only used in outbox */
  uint32_t reset_type;                   /* the type to indicate the module reset reason. */
  uint32_t brstsetflg;                   /* the flag to indicate the module reset reason is set. */
  /*** End of message version 1 (44 bytes) ***/

  /*** New fields may be added here.  Do not modify previous fields ***/
};

/************
 *
 * Name:     bc_smem_message_s
 *
 * Purpose:  Message structure used by an individual image
 *
 * Notes:    Structure is packed and uses fixed-width types to ensure
 *           compatibility between images and processors
 *
 *           The offsets of each field must remain constant.  Padding
 *           is automatically decreased when mailboxes are updated
 *           with more fields
 *
 *           Must reside in uninitialized shared memory
 *
 ************/
struct __attribute__((packed)) bc_smem_message_s
{
  uint32_t               magic_beg;    /* Beginning marker */
  uint32_t               version;      /* Message version  */

  struct bsmsg_mailbox_s in;
  uint8_t pad0[BCMSG_MAILBOX_PAD];

  struct bsmsg_mailbox_s out;
  uint8_t pad1[BCMSG_MAILBOX_PAD - sizeof(uint32_t)];
                                       /* -4 for crc32 for backward
                                        * compatibility
                                        */
  uint32_t               magic_end;    /* End Marker        */
  uint32_t               crc32;        /* CRC32 of above fields
                                        * added in V2
                                        */
};

/************
 *
 * Name:     imsw_smem_im_s
 *
 * Purpose:  gobi SMEM structure
 *
 * Notes:    Must be fit in BS_SMEM_REGION_IM
 *           Must be compatible with the same structure define in imswidefs.h
 *
 ************/
struct __attribute__((packed)) imsw_smem_im_s
{
  uint32_t magic_beg;                                /* Beginning marker  */
  uint32_t version;                                  /* structure version */
  uint32_t magic_recovery;                           /* recovery command from HLOS */
  uint8_t  pad[BS_SMEM_IM_SIZE - (5 * sizeof(uint32_t))];  /* padding zone      */
  uint32_t magic_end;                                /* Beginning marker  */
  uint32_t crc32;                                    /* crc32             */
};

/************
 *
 * Name:     mibib_smem_s
 *
 * Purpose:  MIBIB region structure
 *
 * Notes:    Structure is packed and uses fixed-width types to ensure
 *           compatibility between images and processors
 *
 *           Must reside in uninitialized shared memory
 *
 ************/
struct __attribute__((packed)) mibib_smem_s
{
  uint32_t magic_beg;             /* Beginning marker */
  uint32_t update_flag;           /* MIBIB update flag */
  uint32_t download_addr;         /* MIBIB image store address*/
  uint32_t magic_end;             /* End Marker */
  uint32_t crc32;                 /* CRC32 of above fields */
};

/************
 *
 * Name:     bs_smem_mode_switch
 *
 * Purpose:  gobi SMEM structure
 *
 * Notes:    Must be fit in BS_SMEM_MODE_SWITCH
 *
 *
 ************/
struct __attribute__((packed)) bs_smem_mode_switch
{
  uint32_t magic_beg;                                /* Beginning marker  */
  uint32_t  mode;                                    /* factory or normal mode */
  uint32_t magic_end;                                /* Beginning marker  */
  uint32_t crc32;                                    /* crc32             */
};

/************
 *
 * Name:     bs_smem_secboot_info
 *
 * Purpose:  secboot SMEM structure containing secure boot enable or not info.
 *
 * Members:  see below
 *
 * Notes:    None
 *
 *
 ************/
struct __attribute__((packed)) bs_smem_secboot_info
{
  uint32_t magic_beg;                                /* Beginning marker  */
  uint32_t auth_enable;                              /* secboot auth enable flag */
  uint32_t reseved[4];                               /* reseved for future usage */  
  uint32_t magic_end;                                /* Beginning marker  */
  uint32_t crc32;                                    /* crc32             */
};


/************
 *
 * Name:     ds_smem_message_s
 *
 * Purpose:  Message structure used in DS
 *
 * Notes:  It is only used during boot up.
 * 1. In SBL, read DSSD partition and update DSSD SMEM every time if 'is_changed' is NOT equal with DS_BOOT_UP_CHANGED.
 * 2. In SBL, sync DSSD SMEM to DSSD partition if 'is_changed' is equal with DS_BOOT_UP_CHANGED.
 * 3. In SBL, get 'boot_system' flag to determine boot up which TZ, RPM, LK images.
 * 4. In SBL, write DSSD SMEM if any one image of TZ, RPM, LK is invalid.
 * 5. In LK, read DSSD SMEM to get 'boot_system' flag to determine boot up which Kernel.
 * 6. In LK, write DSSD SMEM if kernel image is invalid.
 * 7. In Kernel, read DSSD SMEM to get 'boot_system' flag to determine boot up which root FS.
 * 8. In Kernel, write DSSD SMEM if root FS image is invalid.
 * 9. In root FS, read DSSD SMEM to get 'boot_system' flag to determine boot up which Legato related images.
 * 10. In root FS, write DSSD SMEM if any one of Legato related image is invalid.
 *
 ************/
struct __attribute__((packed)) ds_smem_message_s
{
  uint32_t  magic_beg;             /* Magic begin flag */
  uint8_t   ssid_modem_idx;        /* SSID modem index flag */
  uint8_t   ssid_lk_idx;           /* SSID LK index flag */
  uint8_t   ssid_linux_idx;        /* SSID Linux index flag */
  uint8_t   reserved_8bits;        /* Reserved for 8 bytes align */
  uint32_t  swap_reason;           /* Dual system swap reasons */
  uint32_t  is_changed;            /* Mark if it is changed or not during boot up */
  uint64_t  bad_image;             /* Record bad images */
  uint32_t  magic_end;             /* Magic ending flag */
  uint32_t  crc32;                 /* CRC32 of above fields */
};


/*************
 *
 * Name:     bscoworkmsg - Coopertive work message structure
 *
 * Purpose:  To provide a structure to share the resoure assigned state .
 *
 * Members:  See inline comments below
 *
 * Note:     1. Both markers must contain BC_VALID_BOOT_MSG_MARKER for the
 *              contents to be considered valid.
 *              Otherwise, the structure's contents are undefined.
 *           2. The total size of this structure is small and must reside in
 *              RAM that is never initialized by boot loader at startup.
 *
 *************/
struct __attribute__((packed)) bscoworkmsg
{
  uint32_t magic_beg;        /* Magic ending flag */
  uint32_t bsgpioflag[2];    /* external gpio owner flag. */
  uint8_t  bsuartfun[2];     /* UART1 and UART2 function */
  uint8_t  bsriowner;        /* RI owner */
  uint8_t  bssleepind;       /* Sleep inidcation function */
  uint8_t  bsresettype;      /* reset type */
  uint8_t  bsbootquiet;      /* indicate whether bootquiet is enabled or not */
  uint8_t  bsresintimer[2];  /* S1, S2 timer for HW reset */
  uint32_t bsfunctions;      /* bitmask for functions configured by modem side(ex. HISC) */
  uint8_t  bsresinfotype;    /* reset info type */
  uint8_t  bsresinfosource;  /* reset info source */
  uint8_t  bserrorfatalcount;/* sbl error fatal count */
  uint8_t  bsbreserved[1];   /* The unused memory */
  uint32_t bsreserved[7];    /* The unused memory */
  uint32_t magic_end;        /* Magic ending flag */
  uint32_t crc32;            /* CRC32 of above fields */
};

/************
 *
 * Name:     pr_sw_smem_message_s
 *s
 * Purpose:  Interlock between program refresh and normal SW update
 *
 * Notes:  
 * 1. It is used to interlock between program refresh and normal SW update.
 * 2. Delay program refresh if normal SW update is in progress.
 * 3. Delay normal SW update if program refresh is in progress.
 * 4. It should be sync with normal SW update application
 *
 ************/
struct __attribute__((packed)) pr_sw_smem_message_s
{
  uint32_t  magic_beg;                 /* Magic begin flag */
  uint32_t  pr_or_sw_update;           /* pr or sw update flag */
  uint32_t  magic_end;                 /* Magic ending flag */
  uint32_t  crc32;                     /* CRC32 of above fields */
};

/************
 *
 * Name:     cross_sku_smem_s
 *
 * Purpose:  Cross SKU structure
 *
 * Notes:    Structure store Parent SKU and Product SKU. 
 *           
 *           It should be initialized in SBL, then LK will refer it for Cross SKU update.
 *
 ************/
struct __attribute__((packed)) cross_sku_smem_s
{
  uint32_t magic_beg;                            /* Beginning marker */
  char     ParentSKU[NV_SWI_PRODUCT_SKU_SIZE];   /* Parent SKU */
  char     ProductSKU[NV_SWI_PRODUCT_SKU_SIZE];  /* Product SKU */
  uint32_t magic_end;                            /* End Marker */
  uint32_t crc32;                                /* CRC32 of above fields */
};

/************
 *
 * Name:     cnfg_feature_smem_s
 *
 * Purpose:  reliability configuration feature structure
 *
 * Notes:    Structure store reliability feature configuration data.
 * 
 *           It should be initialized in SBL, then SBL and modem will refer it 
 *           for configuring corresponding reliability feature.
 *
 ************/
struct __attribute__((packed)) cnfg_feature_smem_s
{
  uint32_t magic_beg;                        /* Beginning marker */
  uint8_t  dump_mode;                        /* RAM memory dump mode. We already have a NV
                                                item to configure dump mode, so we should mange it separately */  
  uint8_t  version;                          /* Configuration data version */
  uint8_t  cnfg_parameter[CNFG_FTUR_SIZE];   /* Reliability feature configuration data */ 
  uint8_t  reserved[3];                      /* reserved */
  uint32_t magic_end;                        /* End Marker */
  uint32_t crc32;                            /* CRC32 of above fields */
};

/* FWUpdate status structure feature */
#define BC_FWUP_STR_SIZE            256
#define BC_SMEM_FWUPSTATUS_SZ       (sizeof(struct bc_update_status_s))
/* CRC check on the structure minus crc32 field */
#define BC_FWUPSTATUS_CRC_SZ        (BC_SMEM_FWUPSTATUS_SZ - sizeof(uint32_t))
/************
 *
 * Name:     bc_update_status_s
 *
 * Purpose:  firmware update status structure.  
 *
 * Notes:    Structure is packed and uses fixed-width types to ensure
 *           compatibility between images and processors
 *
 ************/
struct __attribute__((packed)) bc_update_status_s
{
  uint32_t               magic_beg;    /* Beginning marker */
  uint32_t               result;       /* result code      */
  char                   logstr[BC_FWUP_STR_SIZE]; /* log string */
  uint32_t               resultcomp;   /* redundant complement result */
  uint32_t               magic_end;    /* End Marker        */
  uint32_t               crc32;        /* CRC32 of above fields */
};

/* Information for EFS restore
*
* 1. The ds_efs_restore_type is the type of the EFS restore.
*   1.1, EFS sanity restore:
*     1.1.1, No need to restore for mirror systerm, should restore for non-mirror systerm.
*     1.1.2, The 4 efs sanity-restore types just be used to distinguish different efs restore requests from LK/KERNEL/SBL, etc.
*     1.1.3, DS_RESTORE_EFS_SANITY request efs restore for 6 times abnormal reset or other reasons lead to system swap. Need to swap system and restore efs.
*     1.1.4, DS_RESTORE_EFS_SANITY_FROM_LK request DS_RESTORE_EFS_SANITY for bad image detected in lk. Need to swap system and restore efs.
*     1.1.5, DS_RESTORE_EFS_SANITY_FROM_KERNEL request DS_RESTORE_EFS_SANITY for bad image detected in kernel. Need to swap system and restore efs.
*     1.1.6, DS_RESTORE_EFS_SANITY_FROM_SBL request DS_RESTORE_EFS_SANITY for bad image detected in sbl. Need to swap system and restore efs.
*     1.1.7, DS_RESTORE_EFS_3TIMES_ABNORMAL_REBOOT request for 3 times abnormal reboot.
*     1.1.8, DS_RESTORE_EFS_CHECK_VALID_BACKUPS_TEST type is request for check backups in SBL.
*   1.2, DS_RESTORE_EFS_ANYWAY:
*     1.2.1, Should restore efs no matter mirror/non-mirror systerm.
*
* 2. The ds_smem_erestore_info is used to store EFS restore info.
*/
enum ds_efs_restore_type
{
  DS_RESTORE_EFS_TYPE_MIN,
  DS_RESTORE_EFS_SANITY = 1,            /* restore efs sanity, request for 6 times abnormal reset */
  DS_RESTORE_EFS_ANYWAY,                /* restore efs anyway */
  DS_RESTORE_EFS_SANITY_FROM_LK,        /* restore efs sanity, request from lk */
  DS_RESTORE_EFS_SANITY_FROM_KERNEL,    /* restore efs sanity, request from kernel */
  DS_RESTORE_EFS_SANITY_FROM_SBL,       /* restore efs sanity, request from sbl */
  DS_RESTORE_EFS_3TIMES_ABNORMAL_REBOOT,/* restore efs request for 3 times abnormal reboot */
  DS_RESTORE_EFS_CHECK_VALID_BACKUPS_TEST,/* restore efs request to check valid efs backups of current system */
  DS_RESTORE_EFS_TYPE_MAX,

};

enum bl_erestore_info_type
{
  BL_RESTORE_INFO_MIN = 1,
  BL_RESTORE_INFO_ECOUNT_BUF= BL_RESTORE_INFO_MIN,
  BL_RESTORE_INFO_BEROPTION_BUF,
  BL_RESTORE_INFO_RESTORE_DONE,
  BL_RESTORE_INFO_RESTORE_TYPE,
  BL_RESTORE_INFO_MAX = BL_RESTORE_INFO_RESTORE_TYPE,
};

/* Stags for efs restore.
 * Description of the stage of EFS restore processing.
 * BL_RESTORE_INFO_STAGE_TRIGGERED  : if efs restore request from LK/KERNEL/SBL or abnormally reboot, EFS need to be restore.
 *                                    System will enter this state.
 *                                    If the system enter this stage for the second time, means that the module abnormally
 *                                    reboot again before enter modem. In this case need to try to restore EFS again.
 * BL_RESTORE_INFO_STAGE_EFSERASED  : if system enter module and crash to reboot brefore efs restore, system will erase
 *                                    efs. The privious state must be BL_RESTORE_INFO_STAGE_MDMOK
 * BL_RESTORE_INFO_STAGE_MDMOK      : this state will be set when modem bootup in the efs restore processing. The previous
 *                                    state maybe BL_RESTORE_INFO_STAGE_TRIGGERED or BL_RESTORE_INFO_STAGE_EFSERASED. In this
 *                                    state, modem will try to restore efs, if successed, the next state will be BL_RESTORE_INFO_STAGE_RESTORED_FAILED or BL_RESTORE_INFO_STAGE_RESTORED_OK.
 *                                    Or system will check the system to determin  wether need to erase efs. If erase the EFS, enter
 *                                    BL_RESTORE_INFO_STAGE_EFSERASED stage.
 * BL_RESTORE_INFO_STAGE_RESTORED_FAILED: system will enter this state when efs restore failed.
 * BL_RESTORE_INFO_STAGE_RESTORED_OK: system will enter this state when efs restore OK.
 * --------------------------------------------------------------------------------------------------------------------------------
 *
 * EFS request for 6 times abnormal reboot   |===============||
 * Non-mirror system and having valid backups|               ||
 *                                                           ||
 * EFS request frome LK/SBL/KERNEL           |==============>||
 * EFS request for 3 times abnormal reboot   |               ||
 * Non-mirror system and having valid backups|               ||
 *                                                           ||
 *                                          _ _ _ _ _ _      ||
 *                                         | _ _ _ _ _ |     ||
 * Enter BL_RESTORE_INFO_STAGE_TRIGGERED   ||         ||     \/
 * for 2 times, means modem can not boot   ||        BL_RESTORE_INFO_STAGE_TRIGGERED
 * up normally. Try again                  ||          /\    ||
 *                                         ||_ _ _ _ _ ||    ||
 *                                         |_ _ _ _ _ _ |    ||
 *                                                           ||
 *                                          _ _ _ _ _ _      ||
 *                                         | _ _ _ _ _ |     ||
 *                                         ||         ||     \/
 *                                         ||        BL_RESTORE_INFO_STAGE_MDMOK =================> BL_RESTORE_INFO_STAGE_EFSERASED
 *                                         ||         /\   ||      ||         /\                                      ||
 *                                         ||_ _ _ _ _||   ||      ||         ||                                      ||
 *                                         |_ _ _ _ _ _|   ||      ||         ||_ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ ||
 *                                                         ||      ||         |_ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ __|
 *                                                         \/      \/
 *                       BL_RESTORE_INFO_STAGE_RESTORED_FAILED   BL_RESTORE_INFO_STAGE_RESTORED_OK
 *                                                         ||      ||
 *                                                         ||      ||
 *                                                         \/      \/
 *                                                       DS_FLAG_NOT_SET
 *
 *----------------------------------------------------------------------------------------------------------------------------------
 */
enum bl_erestore_info_stage
{
  BL_RESTORE_INFO_STAGE_MIN = 1,
  BL_RESTORE_INFO_STAGE_TRIGGERED = BL_RESTORE_INFO_MIN,
  BL_RESTORE_INFO_STAGE_EFSERASED,
  BL_RESTORE_INFO_STAGE_MDMOK,
  BL_RESTORE_INFO_STAGE_RESTORED_FAILED,
  BL_RESTORE_INFO_STAGE_RESTORED_OK,
  BL_RESTORE_INFO_STAGE_MAX = BL_RESTORE_INFO_STAGE_RESTORED_OK,
};

struct __attribute__((packed)) ds_smem_erestore_info
{
  uint32_t  magic_beg;             /* Magic begin flag */
  uint8_t   erestore_t;            /* EFS restore type */
  uint8_t   errorcount;            /* backup errorcount */
  uint8_t   restored_flag;         /* efs-restore last booting */
  uint8_t   beroption;             /* backup beroption */
  uint32_t  magic_end;             /* Magic ending flag */
  uint32_t  crc32;                 /* CRC32 of above fields */
};

#define DS_MAGIC_EFSB                       0x45465342  /* "EFSB" */
#define DS_MAGIC_EFSE                       0x45465345  /* "EFSE" */
#define BL_RESTORE_INFO_INVALID_VALUE 0xFF
#define DS_ERESTORE_CRC_SZ (sizeof(struct ds_smem_erestore_info) - sizeof(uint32_t))

void sierra_smem_errdump_save_start(void);
void sierra_smem_errdump_save_timestamp(uint32_t time_stamp);
void sierra_smem_errdump_save_errstr(char *errstrp);
void sierra_smem_errdump_save_auxstr(char *errstrp);
void sierra_smem_errdump_save_frame(void *taskp, void *framep);
uint32_t sierra_smem_get_hwconfig(void);
void sierra_smem_errdump_save_regs(void *registers, void *taskp);
int  sierra_smem_get_download_mode(void);
int sierra_smem_boothold_mode_set(void);
int sierra_smem_im_recovery_mode_set(void);
unsigned char * sierra_smem_base_addr_get(void);
int sierra_smem_ds_get_ssid(uint8_t *modem_idx, uint8_t *lk_idx, uint8_t *linux_idx);
int sierra_smem_ds_write_bad_image_and_swap(uint64_t bad_image_mask);
int sierra_support_ar_dualsystem(void);
void ubi_check_bad_image_and_swap(char *ubi_name);
int sierra_smem_handle_bad_partition_name(char * ubi_volume_name);

#endif /* SIERRA_SMEM_H */

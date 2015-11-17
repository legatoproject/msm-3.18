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

/* Shared Memory Sub-region offsets */
#define BS_SMEM_CRC_SIZE               0x0004   /* 4 bytes CRC value for each shared memory area */
#define BS_SMEM_CWE_SIZE                   0x1000   /* 512 * 8 slots              */
#define BS_SMEM_MSG_SIZE                   0x0400   /* 1 kB, fixed for expansion  */
#define BS_SMEM_ERR_SIZE                   0x1000   /* (0x07F8 + 0x07F8 + 0x0010) */
#define BS_SMEM_ERR_DUMP_SIZE              0x7F8
#define BS_SMEM_USBD_SIZE                  0x0300
#define BS_SMEM_CACHE_SIZE                 0x2000
#define BS_SMEM_EFSLOG_SIZE                0x0400   /* 1 KB */
#define BS_SMEM_FWUP_SIZE                  0x0400   /* 1 KB */
#define BS_SMEM_IM_SIZE                    0x0400   /* 1 KB */

#define BSMEM_CWE_OFFSET                   (0)
#define BSMEM_MSG_OFFSET                   (BSMEM_CWE_OFFSET  + BS_SMEM_CWE_SIZE + BS_SMEM_CRC_SIZE )
#define BSMEM_ERR_OFFSET                   (BSMEM_MSG_OFFSET  + BS_SMEM_MSG_SIZE + BS_SMEM_CRC_SIZE )
#define BSMEM_USBD_OFFSET                  (BSMEM_ERR_OFFSET  + BS_SMEM_ERR_SIZE + BS_SMEM_CRC_SIZE )
#define BSMEM_CACHE_OFFSET                 (BSMEM_USBD_OFFSET + BS_SMEM_USBD_SIZE + BS_SMEM_CRC_SIZE )
#define BSMEM_EFSLOG_OFFSET                (BSMEM_CACHE_OFFSET+ BS_SMEM_CACHE_SIZE + BS_SMEM_CRC_SIZE )
#define BSMEM_FWUP_OFFSET                  (BSMEM_EFSLOG_OFFSET + BS_SMEM_EFSLOG_SIZE + BS_SMEM_CRC_SIZE )
#define BSMEM_IM_OFFSET                    (BSMEM_FWUP_OFFSET + BS_SMEM_FWUP_SIZE + BS_SMEM_CRC_SIZE )

#define BC_MSG_SIZE_MAX                    340 /* 1/3 of 1kB, on 4-byte boundaries */
#define BC_SMEM_MSG_SZ                     (sizeof(struct bc_smem_message_s))

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

#define BC_SMEM_MSG_MAGIC_BEG              0x92B15380U
#define BC_SMEM_MSG_MAGIC_END              0x31DDF742U
#define BC_SMEM_MSG_VERSION                1
#define BC_MSG_LAUNCH_CODE_INVALID         ((uint32_t)(-1))
#define BC_MSG_RECOVER_CNT_INVALID         ((uint32_t)(-1))
#define BC_MSG_HWCONFIG_INVALID            ((uint32_t)(-1))
#define BC_MSG_USB_DESC_INVALID            ((void *)(-1))

#define BC_MSG_B2A_ADB_EN                  0x0000000000000004ULL
#define BC_MSG_B2A_DLOAD_MODE              0x0000000000000008ULL
#define BC_MSG_A2B_BOOT_HOLD               0x0000000000000001ULL

/* values from imswidefs.h and must keep in sync */
#define IMSW_SMEM_MAGIC_BEG                0x92B15380U
#define IMSW_SMEM_MAGIC_END                0x31DDF742U
#define IMSW_SMEM_MAGIC_RECOVERY           0x52425679U
#define IMSW_SMEM_RECOVERY_CRC             0x3F535010U

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

/* Structures */

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
  /*** End of message version 1 (36 bytes) ***/

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
  uint8_t pad1[BCMSG_MAILBOX_PAD];

  uint32_t               magic_end;    /* End Marker        */
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

void sierra_smem_errdump_save_start(void);
void sierra_smem_errdump_save_timestamp(uint32_t time_stamp);
void sierra_smem_errdump_save_errstr(char *errstrp);
void sierra_smem_errdump_save_auxstr(char *errstrp);
void sierra_smem_errdump_save_frame(void *taskp, void *framep);
int  sierra_smem_get_download_mode(void);
int sierra_smem_boothold_mode_set(void);
int sierra_smem_im_recovery_mode_set(void);
unsigned char * sierra_smem_base_addr_get(void);

#endif /* SIERRA_SMEM_H */

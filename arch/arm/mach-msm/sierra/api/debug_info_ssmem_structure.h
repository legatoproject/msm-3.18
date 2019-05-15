/*************
 *
 * Filename:  debug_info_ssmem_structure.h
 *
 * Purpose:   DEBUG INFO SSMEM structure defines
 *
 * Notes:     This file will be used for all the images: boot/mpss/lk/kernel
 *            so please keep it clean
 *
 * Copyright: (c) 2019 Sierra Wireless, Inc.
 *            All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 **************/
#ifndef DEBUG_INFO_SSMEM_STRUCTURE_H
#define DEBUG_INFO_SSMEM_STRUCTURE_H

/* Include files */
#if defined(SWI_IMAGE_LK) || defined(__KERNEL__)
#include "aaglobal_linux.h"
#else
#include "aaglobal.h"
#endif

/* Constants and enumerated types */
#define DEBUG_INFO_SSMEM_VER    SSMEM_RG_USER_VER_1P0

/* Structure */
/************
 *
 * Name:     debug_info_s
 *
 * Purpose:  debug info shared memory region layout
 *
 * Notes:    This region will be accessed by a new AT command and a new QMS message
 *
 ************/
PACKED struct PACKED_POST debug_info_s
{
  uint8_t   secure_boot;        /* +0000: Disabled   (0), or Enabled (1) */
  uint8_t   memory_dump;        /* +0001: Disallowed (0), or Allowed (1) */
  uint8_t   jtag_access;        /* +0002: Disallowed (0), or Allowed (1) */
};

#endif /* DEBUG_INFO_SSMEM_STRUCTURE_H */

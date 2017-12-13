/*
 * include/linux/spi/sierra_spi.h 
 *
 * Copyright (C) 2013 Sierra Wireless, Inc
 *	Author: Alex Tan <atan@sierrawireless.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
  */

#ifndef SWI_SPIDEV_H
#define SWI_SPIDEV_H

#include <linux/types.h>

#if defined(CONFIG_SPI_QSD) || defined(CONFIG_SPI_QSD_MODULE)

#define QSD_REG(x) (x)
#define QUP_REG(x)

#define SPI_FIFO_WORD_CNT             0x0048

#else

#define QSD_REG(x)
#define QUP_REG(x) (x)

#define QUP_CONFIG                    0x0000 /* N & NO_INPUT/NO_OUPUT bits */
#define QUP_ERROR_FLAGS_EN            0x030C
#define QUP_ERR_MASK                  0x3
#define SPI_OUTPUT_FIFO_WORD_CNT      0x010C
#define SPI_INPUT_FIFO_WORD_CNT       0x0214
#define QUP_MX_WRITE_COUNT            0x0150
#define QUP_MX_WRITE_CNT_CURRENT      0x0154

#define QUP_CONFIG_SPI_MODE           0x0100
#endif

#define GSBI_CTRL_REG                 0x0
#define GSBI_SPI_CONFIG               0x30
#define QUP_HARDWARE_VER              0x0030
#define QUP_OPERATIONAL_MASK          0x0028
#define QUP_ERROR_FLAGS               0x0308

#define SPI_CONFIG                    QSD_REG(0x0000) QUP_REG(0x0300)
#define SPI_IO_CONTROL                QSD_REG(0x0004) QUP_REG(0x0304)
#define SPI_IO_MODES                  QSD_REG(0x0008) QUP_REG(0x0008)
#define SPI_SW_RESET                  QSD_REG(0x000C) QUP_REG(0x000C)
#define SPI_TIME_OUT_CURRENT          QSD_REG(0x0014) QUP_REG(0x0014)
#define SPI_MX_OUTPUT_COUNT           QSD_REG(0x0018) QUP_REG(0x0100)
#define SPI_MX_OUTPUT_CNT_CURRENT     QSD_REG(0x001C) QUP_REG(0x0104)
#define SPI_MX_INPUT_COUNT            QSD_REG(0x0020) QUP_REG(0x0200)
#define SPI_MX_INPUT_CNT_CURRENT      QSD_REG(0x0024) QUP_REG(0x0204)
#define SPI_MX_READ_COUNT             QSD_REG(0x0028) QUP_REG(0x0208)
#define SPI_MX_READ_CNT_CURRENT       QSD_REG(0x002C) QUP_REG(0x020C)
#define SPI_OPERATIONAL               QSD_REG(0x0030) QUP_REG(0x0018)
#define SPI_ERROR_FLAGS               QSD_REG(0x0034) QUP_REG(0x001C)
#define SPI_ERROR_FLAGS_EN            QSD_REG(0x0038) QUP_REG(0x0020)
#define SPI_DEASSERT_WAIT             QSD_REG(0x003C) QUP_REG(0x0310)
#define SPI_OUTPUT_DEBUG              QSD_REG(0x0040) QUP_REG(0x0108)
#define SPI_INPUT_DEBUG               QSD_REG(0x0044) QUP_REG(0x0210)
#define SPI_TEST_CTRL                 QSD_REG(0x004C) QUP_REG(0x0024)
#define SPI_OUTPUT_FIFO               QSD_REG(0x0100) QUP_REG(0x0110)
#define SPI_INPUT_FIFO                QSD_REG(0x0200) QUP_REG(0x0218)
#define SPI_STATE                     QSD_REG(SPI_OPERATIONAL) QUP_REG(0x0004)

/* SPI_CONFIG fields */
#define SPI_CFG_INPUT_FIRST           0x00000200
#define SPI_NO_INPUT                  0x00000080
#define SPI_NO_OUTPUT                 0x00000040
#define SPI_CFG_LOOPBACK              0x00000100
#define SPI_CFG_N                     0x0000001F

/* SPI_IO_CONTROL fields */
#define SPI_IO_C_FORCE_CS             0x00000800
#define SPI_IO_C_CLK_IDLE_HIGH        0x00000400
#define SPI_IO_C_CLK_ALWAYS_ON        0x00000200
#define SPI_IO_C_MX_CS_MODE           0x00000100
#define SPI_IO_C_CS_N_POLARITY        0x000000F0
#define SPI_IO_C_CS_N_POLARITY_0      0x00000010
#define SPI_IO_C_CS_SELECT            0x0000000C
#define SPI_IO_C_TRISTATE_CS          0x00000002
#define SPI_IO_C_NO_TRI_STATE         0x00000001


/* User space versions of kernel symbols for SPI clocking modes,
 * matching <linux/spi/spi.h>
 */

#define SPI_CPHA		0x01
#define SPI_CPOL		0x02

#define SPI_MODE_0		(0|0)
#define SPI_MODE_1		(0|SPI_CPHA)
#define SPI_MODE_2		(SPI_CPOL|0)
#define SPI_MODE_3		(SPI_CPOL|SPI_CPHA)

#define SPI_CS_HIGH		0x04
#define SPI_LSB_FIRST		0x08
#define SPI_3WIRE		0x10
#define SPI_LOOP		0x20
#define SPI_NO_CS		0x40
#define SPI_READY		0x80

#define SPI_CS_FORCE 0x100
#define SPI_CLK_ALWAYS_ON 0x200
/*---------------------------------------------------------------------------*/

/* IOCTL commands */

#define SPI_IOC_MAGIC			'l'

/**
 * struct spi_ioc_transfer - describes a single SPI transfer
 * @tx_buf: Holds pointer to userspace buffer with transmit data, or null.
 *	If no data is provided, zeroes are shifted out.
 * @rx_buf: Holds pointer to userspace buffer for receive data, or null.
 * @len: Length of tx and rx buffers, in bytes.
 * @speed_hz: Temporary override of the device's bitrate.
 * @bits_per_word: Temporary override of the device's wordsize.
 * @delay_usecs: If nonzero, how long to delay after the last bit transfer
 *	before optionally deselecting the device before the next transfer.
 * @cs_change: True to deselect device before starting the next transfer.
 *
 * This structure is mapped directly to the kernel spi_transfer structure;
 * the fields have the same meanings, except of course that the pointers
 * are in a different address space (and may be of different sizes in some
 * cases, such as 32-bit i386 userspace over a 64-bit x86_64 kernel).
 * Zero-initialize the structure, including currently unused fields, to
 * accommodate potential future updates.
 *
 * SPI_IOC_MESSAGE gives userspace the equivalent of kernel spi_sync().
 * Pass it an array of related transfers, they'll execute together.
 * Each transfer may be half duplex (either direction) or full duplex.
 *
 *	struct spi_ioc_transfer mesg[4];
 *	...
 *	status = ioctl(fd, SPI_IOC_MESSAGE(4), mesg);
 *
 * So for example one transfer might send a nine bit command (right aligned
 * in a 16-bit word), the next could read a block of 8-bit data before
 * terminating that command by temporarily deselecting the chip; the next
 * could send a different nine bit command (re-selecting the chip), and the
 * last transfer might write some register values.
 */
struct spi_ioc_transfer {
	__u64		tx_buf;
	__u64		rx_buf;

	__u32		len;
	__u32		speed_hz;

	__u16		delay_usecs;
	__u8		bits_per_word;
	__u8		cs_change;
	__u32		pad;

	/* If the contents of 'struct spi_ioc_transfer' ever change
	 * incompatibly, then the ioctl number (currently 0) must change;
	 * ioctls with constant size fields get a bit more in the way of
	 * error checking than ones (like this) where that field varies.
	 *
	 * NOTE: struct layout is the same in 64bit and 32bit userspace.
	 */
};

/* not all platforms use <asm-generic/ioctl.h> or _IOC_TYPECHECK() ... */
#define SPI_MSGSIZE(N) \
	((((N)*(sizeof (struct spi_ioc_transfer))) < (1 << _IOC_SIZEBITS)) \
		? ((N)*(sizeof (struct spi_ioc_transfer))) : 0)
#define SPI_IOC_MESSAGE(N) _IOW(SPI_IOC_MAGIC, 0, char[SPI_MSGSIZE(N)])


/* Read / Write of SPI mode (SPI_MODE_0..SPI_MODE_3) */
#define SPI_IOC_RD_MODE			_IOR(SPI_IOC_MAGIC, 1, __u8)
#define SPI_IOC_WR_MODE			_IOW(SPI_IOC_MAGIC, 1, __u8)

/* Read / Write SPI bit justification */
#define SPI_IOC_RD_LSB_FIRST		_IOR(SPI_IOC_MAGIC, 2, __u8)
#define SPI_IOC_WR_LSB_FIRST		_IOW(SPI_IOC_MAGIC, 2, __u8)

/* Read / Write SPI device word length (1..N) */
#define SPI_IOC_RD_BITS_PER_WORD	_IOR(SPI_IOC_MAGIC, 3, __u8)
#define SPI_IOC_WR_BITS_PER_WORD	_IOW(SPI_IOC_MAGIC, 3, __u8)

/* Read / Write SPI device default max speed hz */
#define SPI_IOC_RD_MAX_SPEED_HZ		_IOR(SPI_IOC_MAGIC, 4, __u32)
#define SPI_IOC_WR_MAX_SPEED_HZ		_IOW(SPI_IOC_MAGIC, 4, __u32)

/* Read / Write SPI device deassert time */
#define SPI_IOC_RD_DEASSERT_TIME	_IOR(SPI_IOC_MAGIC, 5, __u8)
#define SPI_IOC_WR_DEASSERT_TIME	_IOW(SPI_IOC_MAGIC, 5, __u8)

extern void swi_reg_bit_set(struct spi_device *spi, int reg_addr, int bit_offset,int set_flag);
extern int swi_read_deassert_time(struct spi_device *spi);
extern void swi_write_deassert_time(struct spi_device *spi,__u8 value);

#endif /* SPIDEV_H */

/* arch/arm/mach-msm/sierra_spi.c
 *
 * Copyright (C) 2013 Sierra Wireless, Inc
 * Author: Alex Tan <atan@sierrawireless.com>
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

/* #define DEBUG */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/compat.h>
#include <linux/of.h>
#include <linux/of_device.h>

#include <linux/spi/spi.h>
#include <linux/spi/sierra_spi.h>
#include <linux/miscdevice.h>

#include <asm/uaccess.h>



#define __devinit 
/* Bit masks for spi_device.mode management.  Note that incorrect
 * settings for some settings can cause *lots* of trouble for other
 * devices on a shared bus:
 *
 *  - CS_HIGH ... this device will be active when it shouldn't be
 *  - 3WIRE ... when active, it won't behave as it should
 *  - NO_CS ... there will be no explicit message boundaries; this
 *  is completely incompatible with the shared bus model
 *  - READY ... transfers may proceed when they shouldn't.
 *
 * REVISIT should changing those flags be privileged?
 */
#define SPI_MODE_MASK    (SPI_CPHA | SPI_CPOL | SPI_CS_HIGH \
                | SPI_LSB_FIRST | SPI_3WIRE | SPI_LOOP \
                | SPI_NO_CS | SPI_READY | SPI_CS_FORCE | SPI_CLK_ALWAYS_ON )

struct swi_spidev_data {
    spinlock_t    spi_lock;
    struct spi_device  *spi;

    /* buffer is NULL unless this device is open (users > 0) */
    struct mutex    buf_lock;
    unsigned    users;
    u8      *buffer;
    u8      *bufferrx;
    u32     swi_cfg_mask;
};

static LIST_HEAD(device_list);
static DEFINE_MUTEX(device_list_lock);

static unsigned bufsiz = 512;

static struct spi_device *spi;
//struct swi_spidev_data  *swi_spidev;
//struct msm_spi    *dd;
/*add STATIC by Alin, for limit the scope*/
static struct swi_spidev_data  *swi_spidev;
static struct msm_spi    *dd;


/*this table is same with the freq table in clock-9615.c*/
static unsigned int gsbi_qup_clk_table[] = {960000,4000000,4800000,9600000,
                                            15058800,24000000,25600000,
                                            48000000,51200000};

static void swi_spidev_setup(int mode)
{
  if(!spi)
    return;
  swi_reg_bit_set(spi,SPI_IO_CONTROL,SPI_IO_C_FORCE_CS,!!(mode & SPI_CS_FORCE));
  swi_reg_bit_set(spi,SPI_IO_CONTROL,SPI_IO_C_CLK_ALWAYS_ON,!!(mode & SPI_CLK_ALWAYS_ON));
}

/*swi_spidev_check_spi_clk - we check the max_speed param and choose a proper freq in the table
@spi: the spi_device struct we create for spi interface
*/
static void swi_spidev_check_spi_clk(struct spi_device *spi)
{
  int i;
  int size = sizeof(gsbi_qup_clk_table)/sizeof(gsbi_qup_clk_table[0]);

  /* Make sure we assert if by any chance table of valid SPI speeds is empty. */
  BUG_ON(size==0);

  if (spi->max_speed_hz <= gsbi_qup_clk_table[0])
  {
    spi->max_speed_hz = gsbi_qup_clk_table[0];
    return;
  }

  for (i = 0; (i < size) && (spi->max_speed_hz > gsbi_qup_clk_table[i]); i++);

  /* Check for out of bounds conditions as well. */
  if (i == size)
  {
    /* Use last valid entry in case proposed speed passed the end of table. */
    spi->max_speed_hz = gsbi_qup_clk_table[size-1];
  }
  else
  {
    if (spi->max_speed_hz > (gsbi_qup_clk_table[i-1] + (gsbi_qup_clk_table[i] - gsbi_qup_clk_table[i-1])/2))
    {
      /* We are closer to the next value in table. */
      spi->max_speed_hz = gsbi_qup_clk_table[i];
    }
    else
    {
      spi->max_speed_hz = gsbi_qup_clk_table[i-1];
    }
  }
}

/*-------------------------------------------------------------------------*/

/*
 * We can't use the standard synchronous wrappers for file I/O; we
 * need to protect against async removal of the underlying spi_device.
 */
static void swi_spidev_complete(void *arg)
{
  complete(arg);
}

static ssize_t
swi_spidev_sync(struct swi_spidev_data *swi_spidev, struct spi_message *message)
{
  DECLARE_COMPLETION_ONSTACK(done);
  int status;

  message->complete = swi_spidev_complete;
  message->context = &done;

  spin_lock_irq(&swi_spidev->spi_lock);
  if (swi_spidev->spi == NULL)
    status = -ESHUTDOWN;
  else
    status = spi_async(swi_spidev->spi, message);
  spin_unlock_irq(&swi_spidev->spi_lock);

  if (status == 0) {
    wait_for_completion(&done);
    status = message->status;
    if (status == 0)
      status = message->actual_length;
  }
  return status;
}

static inline ssize_t
swi_spidev_sync_write(struct swi_spidev_data *swi_spidev, size_t len)
{
  struct spi_transfer  t = {
      .tx_buf  = swi_spidev->buffer,
      .len     = len,
    };
  struct spi_message  m;

  spi_message_init(&m);
  spi_message_add_tail(&t, &m);
  return swi_spidev_sync(swi_spidev, &m);
}

static inline ssize_t
swi_spidev_sync_read(struct swi_spidev_data *swi_spidev, size_t len)
{
  struct spi_transfer  t = {
      .rx_buf  = swi_spidev->bufferrx,
      .len     = len,
    };
  struct spi_message  m;

  spi_message_init(&m);
  spi_message_add_tail(&t, &m);
  return swi_spidev_sync(swi_spidev, &m);
}

/*-------------------------------------------------------------------------*/

/* Read-only message with current device setup */
static ssize_t
swi_spidev_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
  struct swi_spidev_data  *swi_spidev;
  ssize_t      status = 0;

  /* chipselect only toggles at start or end of operation */
  if (count > bufsiz)
    return -EMSGSIZE;

  swi_spidev = filp->private_data;

  mutex_lock(&swi_spidev->buf_lock);
  status = swi_spidev_sync_read(swi_spidev, count);
  if (status > 0) {
    unsigned long  missing;

    missing = copy_to_user(buf, swi_spidev->bufferrx, status);
    if (missing == status)
      status = -EFAULT;
    else
      status = status - missing;
  }
  mutex_unlock(&swi_spidev->buf_lock);

  return status;
}

/* Write-only message with current device setup */
static ssize_t
swi_spidev_write(struct file *filp, const char __user *buf,
    size_t count, loff_t *f_pos)
{
  struct swi_spidev_data  *swi_spidev;
  ssize_t      status = 0;
  unsigned long    missing;

  /* chipselect only toggles at start or end of operation */
  if (count > bufsiz)
    return -EMSGSIZE;

  swi_spidev = filp->private_data;

  mutex_lock(&swi_spidev->buf_lock);
  missing = copy_from_user(swi_spidev->buffer, buf, count);
  if (missing == 0) {
    status = swi_spidev_sync_write(swi_spidev, count);
  } else
    status = -EFAULT;
  mutex_unlock(&swi_spidev->buf_lock);

  return status;
}

static int swi_spidev_message(struct swi_spidev_data *swi_spidev,
    struct spi_ioc_transfer *u_xfers, unsigned n_xfers)
{
  struct spi_message  msg;
  struct spi_transfer  *k_xfers;
  struct spi_transfer  *k_tmp;
  struct spi_ioc_transfer *u_tmp;
  unsigned    n, total;
  u8      *buf, *bufrx;
  int      status = -EFAULT;

  spi_message_init(&msg);
  k_xfers = kcalloc(n_xfers, sizeof(*k_tmp), GFP_KERNEL);
  if (k_xfers == NULL)
    return -ENOMEM;

  /* Construct spi_message, copying any tx data to bounce buffer.
   * We walk the array of user-provided transfers, using each one
   * to initialize a kernel version of the same transfer.
   */
  buf = swi_spidev->buffer;
  bufrx = swi_spidev->bufferrx;
  total = 0;
  for (n = n_xfers, k_tmp = k_xfers, u_tmp = u_xfers;
      n;
      n--, k_tmp++, u_tmp++) {
    k_tmp->len = u_tmp->len;

    total += k_tmp->len;
    if (total > bufsiz) {
      status = -EMSGSIZE;
      goto done;
    }

    if (u_tmp->rx_buf) {
      k_tmp->rx_buf = bufrx;
      if (!access_ok(VERIFY_WRITE, (u8 __user *)
            (uintptr_t) u_tmp->rx_buf,
            u_tmp->len))
        goto done;
    }
    if (u_tmp->tx_buf) {
      k_tmp->tx_buf = buf;
      if (copy_from_user(buf, (const u8 __user *)
            (uintptr_t) u_tmp->tx_buf,
          u_tmp->len))
        goto done;
    }
    buf += k_tmp->len;
    bufrx += k_tmp->len;

    k_tmp->cs_change = !!u_tmp->cs_change;
    k_tmp->bits_per_word = u_tmp->bits_per_word;
    k_tmp->delay_usecs = u_tmp->delay_usecs;
    k_tmp->speed_hz = u_tmp->speed_hz;
#ifdef VERBOSE
    dev_dbg(&swi_spidev->spi->dev,
      "  xfer len %zd %s%s%s%dbits %u usec %uHz\n",
      u_tmp->len,
      u_tmp->rx_buf ? "rx " : "",
      u_tmp->tx_buf ? "tx " : "",
      u_tmp->cs_change ? "cs " : "",
      u_tmp->bits_per_word ? : swi_spidev->spi->bits_per_word,
      u_tmp->delay_usecs,
      u_tmp->speed_hz ? : swi_spidev->spi->max_speed_hz);
#endif
    spi_message_add_tail(k_tmp, &msg);
  }

  status = swi_spidev_sync(swi_spidev, &msg);
  if (status < 0)
    goto done;

  /* copy any rx data out of bounce buffer */
  buf = swi_spidev->bufferrx;
  for (n = n_xfers, u_tmp = u_xfers; n; n--, u_tmp++) {
    if (u_tmp->rx_buf) {
      if (__copy_to_user((u8 __user *)
          (uintptr_t) u_tmp->rx_buf, buf,
          u_tmp->len)) {
        status = -EFAULT;
        goto done;
      }
    }
    buf += u_tmp->len;
  }
  status = total;

done:
  kfree(k_xfers);
  return status;
}

static long
swi_spidev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
  int      err = 0;
  int      retval = 0;
  int      deassert_time = 0;
  struct swi_spidev_data  *swi_spidev;
  struct spi_device  *spi;
  u32      tmp;
  unsigned    n_ioc;
  struct spi_ioc_transfer  *ioc;

  /* Check type and command number */
  if (_IOC_TYPE(cmd) != SPI_IOC_MAGIC)
    return -ENOTTY;

  /* Check access direction once here; don't repeat below.
   * IOC_DIR is from the user perspective, while access_ok is
   * from the kernel perspective; so they look reversed.
   */
  if (_IOC_DIR(cmd) & _IOC_READ)
    err = !access_ok(VERIFY_WRITE,
        (void __user *)arg, _IOC_SIZE(cmd));
  if (err == 0 && _IOC_DIR(cmd) & _IOC_WRITE)
    err = !access_ok(VERIFY_READ,
        (void __user *)arg, _IOC_SIZE(cmd));
  if (err)
    return -EFAULT;

  /* guard against device removal before, or while,
   * we issue this ioctl.
   */
  swi_spidev = filp->private_data;
  spin_lock_irq(&swi_spidev->spi_lock);
  spi = spi_dev_get(swi_spidev->spi);
  spin_unlock_irq(&swi_spidev->spi_lock);

  if (spi == NULL)
    return -ESHUTDOWN;

  /* use the buffer lock here for triple duty:
   *  - prevent I/O (from us) so calling spi_setup() is safe;
   *  - prevent concurrent SPI_IOC_WR_* from morphing
   *    data fields while SPI_IOC_RD_* reads them;
   *  - SPI_IOC_MESSAGE needs the buffer locked "normally".
   */
  mutex_lock(&swi_spidev->buf_lock);

  switch (cmd) {
  /* read requests */
  case SPI_IOC_RD_MODE:
    retval = __put_user(swi_spidev->swi_cfg_mask & SPI_MODE_MASK,
          (__u32 __user *)arg);
    break;
  case SPI_IOC_RD_LSB_FIRST:
    retval = __put_user((swi_spidev->swi_cfg_mask & SPI_LSB_FIRST) ?  1 : 0,
          (__u8 __user *)arg);
    break;
  case SPI_IOC_RD_BITS_PER_WORD:
    retval = __put_user(spi->bits_per_word, (__u8 __user *)arg);
    break;
  case SPI_IOC_RD_MAX_SPEED_HZ:
    retval = __put_user(spi->max_speed_hz, (__u32 __user *)arg);
    break;
  case SPI_IOC_RD_DEASSERT_TIME:
    deassert_time = swi_read_deassert_time(spi);
    retval = __put_user(deassert_time, (__u32 __user *)arg);
    break;
  /* write requests */
  case SPI_IOC_WR_MODE:
    retval = __get_user(tmp, (u32 __user *)arg);
    if (retval == 0) {
      u32  save = swi_spidev->swi_cfg_mask;

      if (tmp & ~SPI_MODE_MASK) {
        retval = -EINVAL;
        break;
      }

      tmp |= swi_spidev->swi_cfg_mask & ~SPI_MODE_MASK;
      spi->mode = (u8)tmp;
      swi_spidev->swi_cfg_mask = tmp;
      
      retval = spi_setup(spi);
      if (retval < 0)
        swi_spidev->swi_cfg_mask = save;
      else
      {
        swi_spidev_setup(swi_spidev->swi_cfg_mask);
        dev_dbg(&spi->dev, "spi mode %04x\n", tmp);
      }
      
    }
    break;
  case SPI_IOC_WR_LSB_FIRST:
    retval = __get_user(tmp, (__u8 __user *)arg);
    if (retval == 0) {
      u8  save = spi->mode;

      if (tmp)
        spi->mode |= SPI_LSB_FIRST;
      else
        spi->mode &= ~SPI_LSB_FIRST;
      retval = spi_setup(spi);
      if (retval < 0)
        spi->mode = save;
      else
        dev_dbg(&spi->dev, "%csb first\n",
            tmp ? 'l' : 'm');
    }
    break;
  case SPI_IOC_WR_BITS_PER_WORD:
    retval = __get_user(tmp, (__u8 __user *)arg);
    if (retval == 0) {
      u8  save = spi->bits_per_word;

      spi->bits_per_word = tmp;
      retval = spi_setup(spi);
      if (retval < 0)
        spi->bits_per_word = save;
      else
        dev_dbg(&spi->dev, "%d bits per word\n", tmp);
    }
    break;
  case SPI_IOC_WR_MAX_SPEED_HZ:
    retval = __get_user(tmp, (__u32 __user *)arg);
    if (retval == 0) {
      u32  save = spi->max_speed_hz;

      spi->max_speed_hz = tmp;
      swi_spidev_check_spi_clk(spi);
      retval = spi_setup(spi);
      if (retval < 0)
        spi->max_speed_hz = save;
      else
        dev_dbg(&spi->dev, "%d Hz (max)\n", tmp);
    }
    break;
  case SPI_IOC_WR_DEASSERT_TIME:
    retval = __get_user(tmp, (__u8 __user *)arg);
    swi_write_deassert_time(spi,(u8)tmp);
    break;
  default:
    /* segmented and/or full-duplex I/O request */
    if (_IOC_NR(cmd) != _IOC_NR(SPI_IOC_MESSAGE(0))
        || _IOC_DIR(cmd) != _IOC_WRITE) {
      retval = -ENOTTY;
      break;
    }

    tmp = _IOC_SIZE(cmd);
    if ((tmp % sizeof(struct spi_ioc_transfer)) != 0) {
      retval = -EINVAL;
      break;
    }
    n_ioc = tmp / sizeof(struct spi_ioc_transfer);
    if (n_ioc == 0)
      break;

    /* copy into scratch area */
    ioc = kmalloc(tmp, GFP_KERNEL);
    if (!ioc) {
      retval = -ENOMEM;
      break;
    }
    if (__copy_from_user(ioc, (void __user *)arg, tmp)) {
      kfree(ioc);
      retval = -EFAULT;
      break;
    }

    /* translate to spi_message, execute */
    retval = swi_spidev_message(swi_spidev, ioc, n_ioc);
    kfree(ioc);
    break;
  }

  mutex_unlock(&swi_spidev->buf_lock);
  spi_dev_put(spi);
  return retval;
}

#ifdef CONFIG_COMPAT
static long
swi_spidev_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
  return swi_spidev_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));
}
#else
#define swi_spidev_compat_ioctl NULL
#endif /* CONFIG_COMPAT */

static int swi_spidev_open(struct inode *inode, struct file *filp)
{
  
  int  status = 0;
  
  if (!swi_spidev->buffer) {
    swi_spidev->buffer = kmalloc(bufsiz, GFP_ATOMIC);
    if (!swi_spidev->buffer) {
      dev_dbg(&swi_spidev->spi->dev, "open/ENOMEM\n");
      status = -ENOMEM;
    }
  }
  if (!swi_spidev->bufferrx) {
    swi_spidev->bufferrx = kmalloc(bufsiz, GFP_ATOMIC);
    if (!swi_spidev->bufferrx) {
      dev_dbg(&swi_spidev->spi->dev, "open/ENOMEM\n");
      kfree(swi_spidev->buffer);
      swi_spidev->buffer = NULL;
      status = -ENOMEM;
    }
  }
  swi_spidev->users++;
  filp->private_data = swi_spidev;
  nonseekable_open(inode, filp);

  return status;
}

static int swi_spidev_release(struct inode *inode, struct file *filp)
{
  struct swi_spidev_data  *swi_spidev;
  int      status = 0;

  swi_spidev = filp->private_data;
  filp->private_data = NULL;

  /* last close? */
  swi_spidev->users--;
  if (!swi_spidev->users) {
    int    dofree;

    kfree(swi_spidev->buffer);
    swi_spidev->buffer = NULL;
    kfree(swi_spidev->bufferrx);
    swi_spidev->bufferrx = NULL;

    /* ... after we unbound from the underlying device? */
    spin_lock_irq(&swi_spidev->spi_lock);
    dofree = (swi_spidev->spi == NULL);
    spin_unlock_irq(&swi_spidev->spi_lock);

    if (dofree)
      kfree(swi_spidev);
  }

  return status;
}

static const struct file_operations swi_spidev_fops = {
  .owner =  THIS_MODULE,
  /* REVISIT switch to aio primitives, so that userspace
   * gets more complete API coverage.  It'll simplify things
   * too, except for the locking.
   */
  .write =  swi_spidev_write,
  .read =    swi_spidev_read,
  .unlocked_ioctl = swi_spidev_ioctl,
  .compat_ioctl = swi_spidev_compat_ioctl,
  .open =    swi_spidev_open,
  .release =  swi_spidev_release,
  .llseek =  no_llseek,
};

static struct miscdevice sierra_spi_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "sierra_spi1",
    .fops = &swi_spidev_fops,
};

static int __devinit swi_spidev_probe(struct spi_device *spi)

{
	
  pr_err("sierra SPI1------>swi_spidev_probe\n");

  /* Allocate driver data */
  swi_spidev = kzalloc(sizeof(*swi_spidev), GFP_KERNEL);
  if (!swi_spidev)
    return -ENOMEM;

  /* Initialize the driver data */
  swi_spidev->spi = spi;
  swi_spidev->swi_cfg_mask = SPI_MODE_3;
  
  spi_set_drvdata(spi, swi_spidev);
  
  spin_lock_init(&swi_spidev->spi_lock);
  mutex_init(&swi_spidev->buf_lock);
  
  return 0;
  
}

static int swi_spidev_remove(struct spi_device *spi)

{
   return 0;
}


static struct of_device_id swi_spi_dt_match[] = {
	{
		.compatible = "sierra,spidev1",
	},
	{},
};

MODULE_DEVICE_TABLE(of, swi_spi_dt_match);


static struct spi_driver swi_spidev_spi_driver = {
  .driver = {
    .name =    "sierra_spi1",
    .owner =  THIS_MODULE,
	.of_match_table = of_match_ptr(swi_spi_dt_match),
  },
  .probe =  swi_spidev_probe,  
  .remove =	swi_spidev_remove,

  /* NOTE:  suspend/resume methods are not necessary here.
   * We don't do anything except pass the requests to/from
   * the underlying controller.  The refrigerator handles
   * most issues; the controller driver handles the rest.
   */
};

/*-------------------------------------------------------------------------*/

static int __init swi_spidev_init(void)
{
  int status;
  struct spi_master *master;
  struct spi_board_info chip = {
          .modalias  = "sierra_spi1",
          .mode    = SPI_MODE_3,
          .bus_num  = 2, 
          .chip_select  = 0, //change by Alin SPI2.0
          .max_speed_hz  = 9600000,
  };
  
  pr_err("sierra SPI1-------->swi_spidev_init\n");


  status = spi_register_driver(&swi_spidev_spi_driver);
  if (status < 0)
    return status;


  master = spi_busnum_to_master(2);
  if (!master) {
    status = -ENODEV;
    goto error;
  }
  
  dd = spi_master_get_devdata(master);
  
  if(dd == NULL)
    pr_err("dd == NULL\n");
  
  /* We create a virtual device that will sit on the bus */
  spi = spi_new_device(master, &chip);
  if (!spi) {
    status = -EBUSY;
    goto error;
  }
  
  return misc_register(&sierra_spi_misc);
  
error:
  spi_unregister_driver(&swi_spidev_spi_driver);
  return status;
}
late_initcall(swi_spidev_init);

static void __exit swi_spidev_exit(void)
{
  if (spi) {
    spi_unregister_device(spi);
    spi = NULL;
  }
  spi_unregister_driver(&swi_spidev_spi_driver);
  misc_deregister(&sierra_spi_misc);
}

module_exit(swi_spidev_exit);

MODULE_AUTHOR("Alex Tan, <atan@sierrawireless.com>");
MODULE_DESCRIPTION("User mode SPI device interface");
MODULE_LICENSE("GPL v2");


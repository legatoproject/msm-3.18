/* arch/arm/mach-msm/sierra_tzdev.c
 *
 * OEM Secure File System(Secure storage) encrypt/decrypt files functions
 *
 * Copyright (c) 2016 Sierra Wireless, Inc
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
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <soc/qcom/scm.h>
#include <asm/cacheflush.h>
#include <linux/fs.h>
#include <linux/io.h>

#include <linux/types.h>
#include <linux/compiler.h>
#include <linux/dma-mapping.h>
#include <linux/dmapool.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/crypto.h>
#include <linux/qcrypto.h>
#include <linux/delay.h>


// Include file that contains the command structure definitions.
#include "tzbsp_crypto_api.h"

struct tzdev_op_req {
  uint8_t*        enckey;
  uint32_t        encklen;
  uint8_t*        plain_data;
  uint32_t        plain_dlen;
  uint8_t*        encrypted_buffer;
  uint32_t        encrypted_len;
};

// SCM command structure for encapsulating the request and response addresses.
struct scm_cmd_buf_s
{
  uint32_t req_addr;
  uint32_t req_size;
  uint32_t resp_addr;
  uint32_t resp_size;
};

#define TZDEV_IOCTL_MAGIC      0x9B

#define TZDEV_IOCTL_KEYGEN_REQ          _IOWR(TZDEV_IOCTL_MAGIC, 0x16, struct tzdev_op_req)
#define TZDEV_IOCTL_SEAL_REQ            _IOWR(TZDEV_IOCTL_MAGIC, 0x17, struct tzdev_op_req)
#define TZDEV_IOCTL_UNSEAL_REQ          _IOWR(TZDEV_IOCTL_MAGIC, 0x18, struct tzdev_op_req)

static int sierra_tzdev_open_times = 0; /* record device open times, for shared resources using */


static int tzdev_storage_service_generate_key(uint8_t *key_material, uint32_t* key_size )
{
  // Define request and response structures.
  tz_storage_service_gen_key_cmd_t *gen_key_reqp = NULL;
  tz_storage_service_gen_key_resp_t *gen_key_respp = NULL;
  int rc = 0;
  int key_material_len = *key_size;
  struct scm_cmd_buf_s scm_cmd_buf;

  gen_key_reqp = kmalloc(sizeof(tz_storage_service_gen_key_cmd_t), GFP_KERNEL);
  gen_key_respp = kmalloc(sizeof(tz_storage_service_gen_key_resp_t), GFP_KERNEL);
  if ((!gen_key_reqp) || (!gen_key_respp))
  {
    printk(KERN_CRIT "%s()_line%d: cannot allocate req/resp\n", __func__,__LINE__);
    rc = -1;
    goto end;
  }

  memset(&scm_cmd_buf, 0, sizeof(scm_cmd_buf));
  memset(key_material, 0, key_material_len);
  memset(gen_key_reqp, 0, sizeof(tz_storage_service_gen_key_cmd_t));
  memset(gen_key_respp, 0, sizeof(tz_storage_service_gen_key_resp_t));


  // Populate scm command structure.
  scm_cmd_buf.req_addr = SCM_BUFFER_PHYS(gen_key_reqp);
  scm_cmd_buf.req_size = SCM_BUFFER_SIZE(tz_storage_service_gen_key_cmd_t);
  scm_cmd_buf.resp_addr = SCM_BUFFER_PHYS(gen_key_respp);
  scm_cmd_buf.resp_size = SCM_BUFFER_SIZE(tz_storage_service_gen_key_resp_t);

  // Populate generate key request structure.
  gen_key_reqp->cmd_id = TZ_STOR_SVC_GENERATE_KEY;
  gen_key_reqp->key_blob.key_material = (void *)SCM_BUFFER_PHYS(key_material);
  gen_key_reqp->key_blob.key_material_len = key_material_len;

  // Flush memory
  dmac_flush_range(key_material, ((void *)key_material + (sizeof(uint8_t) * key_material_len)));
  dmac_flush_range(gen_key_reqp, ((void *)gen_key_reqp + (sizeof(tz_storage_service_gen_key_cmd_t))));
  dmac_flush_range(gen_key_respp, ((void *)gen_key_respp + (sizeof(tz_storage_service_gen_key_resp_t))));

  // Call into TZ
  rc = scm_call(TZ_SVC_CRYPTO, TZ_CRYPTO_SERVICE_SYM_ID,
          (void *)&scm_cmd_buf,
          SCM_BUFFER_SIZE(scm_cmd_buf),
          NULL,
          0);

  // Check return value
  if (rc)
  {
    pr_err("%s()_no%d: scm_call fail  with return val %d\n",__func__,__LINE__, rc);
    goto end;
  }

  // Invalidate cache
  dmac_inv_range(gen_key_respp, ((void *)gen_key_respp + (sizeof(tz_storage_service_gen_key_resp_t))));
  dmac_inv_range(key_material, ((void *)key_material + (sizeof(uint8_t) * key_material_len)));

  // Check return value
  if (gen_key_respp->status != 0)
  {
    pr_err("%s()_line%d: resp.status=%d\n",__func__,__LINE__, gen_key_respp->status);
    rc = -1;
    goto end;
  }

  // Sanity check of command id.
  if (TZ_STOR_SVC_GENERATE_KEY != gen_key_respp->cmd_id)
  {
    pr_err("%s()_line%d: resp.cmd_id %d not matched\n",__func__,__LINE__, gen_key_respp->cmd_id);
    rc = -1;
    goto end;
  }

  *key_size = gen_key_respp->key_blob_size;
  
end:
  // Free data buffers
  if(gen_key_reqp)
  {
    kfree(gen_key_reqp);
  }

  if(gen_key_respp)
  {
    kfree(gen_key_respp);
  }

  return rc;
}

static int tzdev_seal_data_using_aesccm(uint8_t *plain_data, uint32_t plain_data_len, 
  uint8_t *sealed_buffer, uint32_t *sealed_data_len,uint8_t *key_material, uint32_t key_material_len)
{
  // Define request and response structures.
  tz_storage_service_seal_data_cmd_t *seal_data_reqp = NULL;
  tz_storage_service_seal_data_resp_t *seal_data_respp = NULL;
  uint32_t sealed_len = *sealed_data_len;
  int rc = 0;
  struct scm_cmd_buf_s scm_cmd_buf;

  seal_data_reqp = kmalloc(sizeof(tz_storage_service_seal_data_cmd_t), GFP_KERNEL);
  seal_data_respp = kmalloc(sizeof(tz_storage_service_seal_data_resp_t), GFP_KERNEL);
  if ((!seal_data_reqp) || (!seal_data_respp))
  {
    printk(KERN_CRIT "%s()_line%d: cannot allocate req/resp\n", __func__,__LINE__);
    rc = -1;
    goto end;
  }

  memset(&scm_cmd_buf, 0, sizeof(scm_cmd_buf));
  memset(seal_data_reqp, 0, sizeof(tz_storage_service_seal_data_cmd_t));
  memset(seal_data_respp, 0, sizeof(tz_storage_service_seal_data_resp_t));

  // Populate scm command structure.
  scm_cmd_buf.req_addr = SCM_BUFFER_PHYS(seal_data_reqp);
  scm_cmd_buf.req_size = SCM_BUFFER_SIZE(tz_storage_service_seal_data_cmd_t);
  scm_cmd_buf.resp_addr = SCM_BUFFER_PHYS(seal_data_respp);
  scm_cmd_buf.resp_size = SCM_BUFFER_SIZE(tz_storage_service_seal_data_resp_t);

  memset(sealed_buffer, 0, sealed_len);

  // Populate seal data request structure.
  seal_data_reqp->cmd_id = TZ_STOR_SVC_SEAL_DATA;
  // Key material is the output from the generate key command.
  seal_data_reqp->key_blob.key_material = (void *)SCM_BUFFER_PHYS(key_material);
  seal_data_reqp->key_blob.key_material_len = key_material_len;
  // Plain data
  seal_data_reqp->plain_data = (void *)SCM_BUFFER_PHYS(plain_data);
  seal_data_reqp->plain_dlen = plain_data_len;
  // Output buffer
  seal_data_reqp->output_buffer = (volatile void *)SCM_BUFFER_PHYS(sealed_buffer);
  seal_data_reqp->output_len = sealed_len;

  // Flush memory
  dmac_flush_range(plain_data, ((void *)plain_data + (sizeof(uint8_t) * plain_data_len)));
  dmac_flush_range(sealed_buffer, ((void *)sealed_buffer + (sizeof(uint8_t) * sealed_len)));
  dmac_flush_range(key_material, ((void *)key_material + (sizeof(uint8_t) * key_material_len)));
  dmac_flush_range(seal_data_reqp, ((void *)seal_data_reqp + (sizeof(tz_storage_service_seal_data_cmd_t))));
  dmac_flush_range(seal_data_respp, ((void *)seal_data_respp + (sizeof(tz_storage_service_seal_data_resp_t))));

  // Call into TZ
  rc = scm_call(TZ_SVC_CRYPTO, TZ_CRYPTO_SERVICE_SYM_ID,
      (void *)&scm_cmd_buf,
      SCM_BUFFER_SIZE(scm_cmd_buf),
      NULL,
      0);

  // Check return value
  if (rc)
  {
    pr_err("%s()_%d: scm_call failed, rc=%d \n", __func__,__LINE__,rc);
    goto end;
  }

  dmac_inv_range(seal_data_respp, ((void *)seal_data_respp + (sizeof(tz_storage_service_seal_data_resp_t))));
  dmac_inv_range(sealed_buffer, ((void *)sealed_buffer + (sizeof(uint8_t) * sealed_len)));

  // Check return value
  if (seal_data_respp->status != 0)
  {
    pr_err("%s(): TZ_STOR_SVC_SEAL_DATA status: %d\n", __func__,
            seal_data_respp->status);
    rc = -1;
    goto end;
  }

  // Sanity check of command id.
  if (TZ_STOR_SVC_SEAL_DATA != seal_data_respp->cmd_id)
  {
    pr_err("%s(): TZ_STOR_SVC_SEAL_DATA invalid cmd_id: %d\n", __func__,
            seal_data_respp->cmd_id);
    rc = -1;
    goto end;
  }

  *sealed_data_len = seal_data_respp->sealed_data_len;

end:
  // Free req buffer
  if(seal_data_reqp)
  {
    kfree(seal_data_reqp);
  }
  if(seal_data_respp)
  {
    kfree(seal_data_respp);
  }

  return rc;
}

static int tzdev_unseal_data_using_aesccm(uint8_t *sealed_buffer, uint32_t sealed_buffer_len, 
  uint8_t *output_buffer_unseal, uint32_t *unseal_len,uint8_t *key_material, uint32_t key_material_len )
{
  // Define request and response structures.
  tz_storage_service_unseal_data_cmd_t *unseal_datareqp;
  tz_storage_service_unseal_data_resp_t *unseal_datarespp;
  uint32_t output_len_unseal = *unseal_len;
  int rc = 0;
  struct scm_cmd_buf_s scm_cmd_buf;

  unseal_datareqp = kmalloc(sizeof(tz_storage_service_unseal_data_cmd_t), GFP_KERNEL);
  unseal_datarespp = kmalloc(sizeof(tz_storage_service_unseal_data_resp_t), GFP_KERNEL);
  if ((!unseal_datareqp) || (!unseal_datarespp))
  {
    printk(KERN_CRIT "%s()_line%d: cannot allocate req/resp\n", __func__,__LINE__);
    rc = -1;
    goto end;
  }

  memset(&scm_cmd_buf, 0, sizeof(scm_cmd_buf));
  memset(unseal_datareqp, 0, sizeof(tz_storage_service_unseal_data_cmd_t));
  memset(unseal_datarespp, 0, sizeof(tz_storage_service_unseal_data_resp_t));

  // Populate scm command structure.
  scm_cmd_buf.req_addr = SCM_BUFFER_PHYS(unseal_datareqp);
  scm_cmd_buf.req_size = SCM_BUFFER_SIZE(tz_storage_service_unseal_data_cmd_t);
  scm_cmd_buf.resp_addr = SCM_BUFFER_PHYS(unseal_datarespp);
  scm_cmd_buf.resp_size = SCM_BUFFER_SIZE(tz_storage_service_unseal_data_resp_t);

  memset(output_buffer_unseal, 0, output_len_unseal);

  // Populate seal data request structure.
  unseal_datareqp->cmd_id = TZ_STOR_SVC_UNSEAL_DATA;
  // Key material is the output from the generate key command.
  unseal_datareqp->key_blob.key_material =  (void *)SCM_BUFFER_PHYS(key_material);
  unseal_datareqp->key_blob.key_material_len = key_material_len;
  // Encrypted data
  unseal_datareqp->sealed_data = (void *)SCM_BUFFER_PHYS(sealed_buffer);
  unseal_datareqp->sealed_dlen = sealed_buffer_len;

  // Plain data
  unseal_datareqp->output_buffer = (void *)SCM_BUFFER_PHYS(output_buffer_unseal);
  unseal_datareqp->output_len = output_len_unseal;
  
  // Flush memory
  dmac_flush_range(sealed_buffer, ((void *)sealed_buffer + (sizeof(uint8_t) * sealed_buffer_len)));  
  dmac_flush_range(output_buffer_unseal, ((void *)output_buffer_unseal + (sizeof(uint8_t) * output_len_unseal)));
  dmac_flush_range(key_material, ((void *)key_material + (sizeof(uint8_t) * key_material_len)));
  dmac_flush_range(unseal_datareqp, ((void *)unseal_datareqp + (sizeof(tz_storage_service_unseal_data_cmd_t))));
  dmac_flush_range(unseal_datarespp, ((void *)unseal_datarespp + (sizeof(tz_storage_service_unseal_data_resp_t))));

  // Call into TZ
  rc = scm_call(TZ_SVC_CRYPTO, TZ_CRYPTO_SERVICE_SYM_ID,
      (void *)&scm_cmd_buf,
      SCM_BUFFER_SIZE(scm_cmd_buf),
      NULL,
      0);

  // Check return value
  if (rc)
  {
    pr_err("%s(): TZ_STOR_SVC_UNSEAL_DATA ret: %d\n", __func__, rc);
    goto end;
  }

  // Invalidate cache
  dmac_inv_range(unseal_datarespp, ((void *)unseal_datarespp + (sizeof(tz_storage_service_unseal_data_cmd_t))));
  dmac_inv_range(output_buffer_unseal, ((void *)output_buffer_unseal + (sizeof(uint8_t) * output_len_unseal)));

  // Check response status
  if (unseal_datarespp->status != 0)
  {
    pr_err("%s(): TZ_STOR_SVC_UNSEAL_DATA status: %d\n", __func__,
            unseal_datarespp->status);
    rc = -1;
    goto end;
  }

  // Sanity check of cmd_id
  if (TZ_STOR_SVC_UNSEAL_DATA != unseal_datarespp->cmd_id)
  {
    pr_err("%s(): TZ_STOR_SVC_UNSEAL_DATA invalid cmd_id: %d\n", __func__,
            unseal_datarespp->cmd_id);
    rc = -1;
    goto end;
  }

  *unseal_len = unseal_datarespp->unsealed_data_len;

end:
  // free req buffer
  if(unseal_datareqp)
  {
    kfree(unseal_datareqp);
  }
  if(unseal_datarespp)
  {
    kfree(unseal_datarespp);
  }

  return rc;
}

static long sierra_tzdev_ioctl(struct file *file, unsigned cmd, unsigned long arg)
{
  int rc = 0;
  struct tzdev_op_req __user * scm_buf_user_ptr = NULL;

  uint32_t key_material_len;
  uint8_t *key_material = NULL;
  uint32_t plain_data_len;
  uint32_t sealed_data_len;
  uint8_t *plain_datap = NULL;
  uint8_t *sealed_datap = NULL;
  uint32_t output_len;

  scm_buf_user_ptr = (struct tzdev_op_req*)arg;

  if (!scm_buf_user_ptr)
  {
    return -EINVAL;
  }

  if (copy_from_user((void *)&plain_data_len, &(scm_buf_user_ptr->plain_dlen), sizeof(uint32_t)) ||
      copy_from_user((void *)&sealed_data_len, &(scm_buf_user_ptr->encrypted_len), sizeof(uint32_t)) ||
      copy_from_user((void *)&key_material_len, &(scm_buf_user_ptr->encklen), sizeof(uint32_t)))
  {
     printk(KERN_ERR "%s_line%d: copy_from_user failed\n", __func__,__LINE__);
     rc = -EFAULT;
     goto end;
  }

  key_material = (uint8_t *) kmalloc(key_material_len, GFP_KERNEL);
  if (!key_material)
  {
     printk(KERN_CRIT "%s()_line%d: cannot allocate key_material\n", __func__,__LINE__);
     rc = -ENOMEM;
     goto end;
  }

  if (cmd != TZDEV_IOCTL_KEYGEN_REQ)
  {
    /* allocate data buffer */
    plain_datap = kmalloc(plain_data_len, GFP_KERNEL);
    sealed_datap = kmalloc(sealed_data_len, GFP_KERNEL);
    if ((!plain_datap) || (!sealed_datap))
    {
       printk(KERN_CRIT "%s()_line%d: cannot allocate plain/sealed data\n", __func__,__LINE__);
       rc = -ENOMEM;
       goto end;
    }
  }

  switch (cmd) 
  {
    /*Generate keyblob*/
    case TZDEV_IOCTL_KEYGEN_REQ:
      {
        output_len = key_material_len;
        rc = tzdev_storage_service_generate_key(key_material, &output_len);
        pr_info("%s()_line%d:TZDEV_IOCTL_KEYGEN_REQ, get key_size:%d, rc=%d\n",
                 __func__, __LINE__, output_len, rc);

        if (rc)
        {
          break;
        }

        if ((output_len > key_material_len) ||
            copy_to_user(scm_buf_user_ptr->enckey, (void *)key_material, output_len) ||
            copy_to_user(&(scm_buf_user_ptr->encklen), &output_len, sizeof(uint32_t))) 
        {
                printk(KERN_ERR "%s_%d: copy_to_user failed\n", __func__,__LINE__);
                rc = -EFAULT;
                break;
        }
      }
      break;

    /* encrypt data */
    case TZDEV_IOCTL_SEAL_REQ:
      {
        if (copy_from_user((void *)plain_datap, scm_buf_user_ptr->plain_data, plain_data_len) ||
            copy_from_user((void *)key_material, scm_buf_user_ptr->enckey, key_material_len))
        {
           printk(KERN_ERR "%s_line%d: copy_from_user failed\n", __func__,__LINE__);
           rc = -EFAULT;
           break;
        }

        output_len = sealed_data_len;
        rc = tzdev_seal_data_using_aesccm(plain_datap, plain_data_len,
                  sealed_datap, &output_len, key_material, key_material_len);
        pr_info("%s()_line%d: TZDEV_IOCTL_SEAL_REQ: plain_data_len:%d, seal_data_len:%d, rc=%d\n",
                __func__, __LINE__, plain_data_len, output_len, rc);

        if (rc)
        {
          break;
        }

        if ((output_len > sealed_data_len) ||
            copy_to_user(&(scm_buf_user_ptr->encrypted_len), &output_len, sizeof(uint32_t)) ||
            copy_to_user(scm_buf_user_ptr->encrypted_buffer, (void *)sealed_datap, output_len))
        {
                printk(KERN_ERR "%s_line%d: copy_to_user failed\n", __func__,__LINE__);
                rc = -EFAULT;
                break;
        }
      }
      break;

    /* decrypt data */
    case TZDEV_IOCTL_UNSEAL_REQ:
      {
        if (copy_from_user((void *)sealed_datap, scm_buf_user_ptr->encrypted_buffer, sealed_data_len) ||
            copy_from_user((void *)key_material, scm_buf_user_ptr->enckey, key_material_len))
        {
          printk(KERN_ERR "%s_line%d: copy_from_user failed\n", __func__,__LINE__);
          rc = -EFAULT;
          break;
        }

        output_len = plain_data_len;
        rc = tzdev_unseal_data_using_aesccm(sealed_datap, sealed_data_len, 
                   plain_datap, &output_len, key_material, key_material_len);
        pr_info("%s()_line%d: TZDEV_IOCTL_UNSEAL_REQ: sealed data len:%d, plain_data_len:%d, rc=%d\n",
                 __func__, __LINE__, sealed_data_len, output_len, rc);

        if (rc)
        {
          break;
        }

        if ((output_len > plain_data_len) ||
            copy_to_user(&(scm_buf_user_ptr->plain_dlen), &output_len, sizeof(uint32_t)) ||
            copy_to_user(scm_buf_user_ptr->plain_data, (void *)plain_datap, output_len))
        {
          printk(KERN_ERR "%s_line%d: copy_to_user failed\n", __func__,__LINE__);
          rc = -EFAULT;
          break;
        }
      }
      break;

      default:
        rc = -EINVAL;
        break;
  }

end:
  // Free buffers
  if (key_material)
  {
    kfree(key_material);
  }
  if (plain_datap)
  {
    kfree(plain_datap);
  }
  if (sealed_datap)
  {
    kfree(sealed_datap);
  }

  return rc;
}


static int sierra_tzdev_open(struct inode *inode, struct file *file)
{
  sierra_tzdev_open_times++;
  pr_info("%s()_%d: sierra_tzdev_open_times=%d \n", __func__,__LINE__,sierra_tzdev_open_times);
  return 0;
}

static int sierra_tzdev_release(struct inode *inode, struct file *file)
{
  sierra_tzdev_open_times--;
  pr_info("%s()_%d: tzdev_driver_open_times=%d \n", __func__,__LINE__,sierra_tzdev_open_times);
  if(sierra_tzdev_open_times < 0)
  {
    return ENODEV;
  }
  else
  {
    return 0;
  }
}

static struct file_operations sierra_tzdev_fops = {
  .owner = THIS_MODULE,
  .unlocked_ioctl = sierra_tzdev_ioctl,
  .open = sierra_tzdev_open,
  .release = sierra_tzdev_release,
};

static struct miscdevice sierra_tzdev_misc = {
  .minor = MISC_DYNAMIC_MINOR,
  .name = "tzdev",
  .fops = &sierra_tzdev_fops,
};

static int __init sierra_tzdev_init(void)
{
  return misc_register(&sierra_tzdev_misc);
}

static void __exit sierra_tzdev_exit(void)
{
  misc_deregister(&sierra_tzdev_misc); 
}

module_init(sierra_tzdev_init);
module_exit(sierra_tzdev_exit);
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Secure storage driver");


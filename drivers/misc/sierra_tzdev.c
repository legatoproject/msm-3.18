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

// Flags used to indicate which parts of tzdev_op_req structure are to be
// allocated and/or copied between kernel and user space.
#define TZDEV_COPY_ENCKEY           1
#define TZDEV_COPY_PLAIN_DATA       2
#define TZDEV_COPY_ENCRYPTED_BUFFER 4

// Context structure for data passing in ioctl routine
struct tzdev_ioctl_ctx {
  struct tzdev_op_req usr;      // bit copy of user request structure
  struct tzdev_op_req krn;      // kernel side structure with own buffers.
  struct tzdev_op_req __user *orig_usr; // original user space pointer
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

#define TZDEV_IOCTL_RSA_KEYPAIR_REQ     _IOWR(TZDEV_IOCTL_MAGIC, 0x20, struct tzdev_op_req)
#define TZDEV_IOCTL_RSA_SIGN_REQ        _IOWR(TZDEV_IOCTL_MAGIC, 0x21, struct tzdev_op_req)
#define TZDEV_IOCTL_RSA_VERIFY_REQ      _IOWR(TZDEV_IOCTL_MAGIC, 0x22, struct tzdev_op_req)
#define TZDEV_IOCTL_RSA_EXPORTKEY_REQ   _IOWR(TZDEV_IOCTL_MAGIC, 0x23, struct tzdev_op_req)
#define TZDEV_IOCTL_RSA_IMPORTKEY_REQ   _IOWR(TZDEV_IOCTL_MAGIC, 0x24, struct tzdev_op_req)


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
    rc = -ENOMEM;
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
  seal_data_reqp->plain_data_len = plain_data_len;
  // Output buffer
  seal_data_reqp->output_buffer = (uint8 *)SCM_BUFFER_PHYS(sealed_buffer);
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

static int tzdev_generate_RSA_keypair(crypto_rsa_key_blob_type* key_blob_ptr, char* rsa_params_ptr)
{
  // Define request and response structures.
  struct crypto_asym_rsa_gen_keypair_cmd *rsa_gen_key_test_req = NULL;
  struct crypto_asym_rsa_gen_keypair_resp *rsa_gen_key_test_resp = NULL;
  uint32_t rsa_key_blob_len = sizeof(crypto_rsa_key_type);
  int rc = 0;
  // SCM command structure for encapsulating the request and response addresses.
  struct scm_cmd_buf_s scm_cmd_buf;
  crypto_asym_rsa_keygen_params_t* params_p;

  rsa_gen_key_test_req = kmalloc(sizeof(struct crypto_asym_rsa_gen_keypair_cmd),GFP_KERNEL);
  rsa_gen_key_test_resp = kmalloc(sizeof(struct crypto_asym_rsa_gen_keypair_resp),GFP_KERNEL);
  if ((!rsa_gen_key_test_req) || (!rsa_gen_key_test_resp))
  {
    printk(KERN_CRIT "%s()_line%d: cannot allocate req/resp\n", __func__,__LINE__);
    rc = -1;
    goto end;
  }
  params_p = (crypto_asym_rsa_keygen_params_t*)rsa_params_ptr;

  // Populate scm command structure.
  scm_cmd_buf.req_addr = SCM_BUFFER_PHYS(rsa_gen_key_test_req);
  scm_cmd_buf.req_size = SCM_BUFFER_SIZE(struct crypto_asym_rsa_gen_keypair_cmd);
  scm_cmd_buf.resp_addr = SCM_BUFFER_PHYS(rsa_gen_key_test_resp);
  scm_cmd_buf.resp_size = SCM_BUFFER_SIZE(struct crypto_asym_rsa_gen_keypair_resp);
  memset(rsa_gen_key_test_req, 0x0, sizeof(struct crypto_asym_rsa_gen_keypair_cmd));
  memset(rsa_gen_key_test_resp, 0x0, sizeof(struct crypto_asym_rsa_gen_keypair_resp));

  // Populate rsa key generate data request structure.
  rsa_gen_key_test_req->cmd_id = CRYPTO_STORAGE_GENERATE_KEY;
  // Key material will be the output from the generate key command.
  rsa_gen_key_test_req->key_blob.key_material = (void *) SCM_BUFFER_PHYS(key_blob_ptr->key_material);
  rsa_gen_key_test_req->key_blob.key_material_len = key_blob_ptr->key_material_len;
  // Key params are the input to the generate key command.
  rsa_gen_key_test_req->rsa_params.modulus_size = params_p->modulus_size;
  rsa_gen_key_test_req->rsa_params.public_exponent = params_p->public_exponent;
  rsa_gen_key_test_req->rsa_params.digest_pad_type = params_p->digest_pad_type;
  // Flush memory
  dmac_flush_range(key_blob_ptr->key_material, ((void *)key_blob_ptr->key_material + (sizeof(uint8_t) * rsa_key_blob_len)));
  dmac_flush_range(rsa_gen_key_test_req, ((void *)rsa_gen_key_test_req + (sizeof(struct crypto_asym_rsa_gen_keypair_cmd))));
  dmac_flush_range(rsa_gen_key_test_resp, ((void *)rsa_gen_key_test_resp + (sizeof(struct crypto_asym_rsa_gen_keypair_resp))));

  // Call into TZ
  rc = scm_call(TZ_SVC_CRYPTO, TZ_CRYPTO_SERVICE_RSA_ID,
      (void *)&scm_cmd_buf,
      SCM_BUFFER_SIZE(scm_cmd_buf),
      NULL,
      0);

  // Check return value
  if (rc) {
    pr_err("%s(): CRYPTO_STORAGE_GENERATE_KEY ret: %d\n", __func__, rc);
    goto end;
  }

  // Invalidate cache
  dmac_inv_range(rsa_gen_key_test_resp, ((void *)rsa_gen_key_test_resp + (sizeof(struct crypto_asym_rsa_gen_keypair_resp))));
  dmac_inv_range(key_blob_ptr->key_material, ((void *)key_blob_ptr + (sizeof(uint8_t) * rsa_key_blob_len)));

  // Check response status
  if (rsa_gen_key_test_resp->status != 0) {
    rc = -1;
    pr_info("%s(): scm_call return error status for generate RSA Key: resp->status = %d\n", __func__, rsa_gen_key_test_resp->status);
    goto end;
  }

  // Sanity check of cmd_id
  if (CRYPTO_STORAGE_GENERATE_KEY != rsa_gen_key_test_resp->cmd_id) {
    rc = -1;
    pr_info("%s(): req cmd_id: %d, resp cmd_id: %d. ERROR.\n", __func__, rsa_gen_key_test_req->cmd_id, rsa_gen_key_test_resp->cmd_id);
    goto end;
  }

  /* save key size */
  key_blob_ptr->key_material_len =rsa_gen_key_test_resp->key_blob_size;

end:
  if(!rsa_gen_key_test_req)
  {
    kfree(rsa_gen_key_test_req);
    rsa_gen_key_test_req=NULL;
  }
  if(!rsa_gen_key_test_resp)
  {
    kfree(rsa_gen_key_test_resp);
    rsa_gen_key_test_resp=NULL;
  }

  return rc;
}

static int tzdev_RSA_sign_data(crypto_rsa_key_blob_type* key_blob_ptr,
              uint8_t* data,uint32_t dlen,uint8_t* signeddata, uint32_t* signeddata_len)
{
  // Define request and response structures.
  struct crypto_asym_rsa_sign_data_cmd *rsa_sign_data_test_req = NULL;
  struct crypto_asym_rsa_sign_data_resp *rsa_sign_data_test_resp = NULL;
  int rc = 0;
  struct scm_cmd_buf_s scm_cmd_buf;
  uint32_t signed_len = *signeddata_len;

  rsa_sign_data_test_req = kmalloc(sizeof(struct crypto_asym_rsa_sign_data_cmd),GFP_KERNEL);
  rsa_sign_data_test_resp = kmalloc(sizeof(struct crypto_asym_rsa_sign_data_resp),GFP_KERNEL);
  if(NULL == rsa_sign_data_test_req || NULL == rsa_sign_data_test_resp)
  {
    pr_err("%s(): RSA sign data: Memory allocation for rsa_sign_data_test_req failed\n", __func__);
    goto end;
  }

  // Populate scm command structure.
  scm_cmd_buf.req_addr = SCM_BUFFER_PHYS(rsa_sign_data_test_req);
  scm_cmd_buf.req_size = SCM_BUFFER_SIZE(struct crypto_asym_rsa_sign_data_cmd);
  scm_cmd_buf.resp_addr = SCM_BUFFER_PHYS(rsa_sign_data_test_resp);
  scm_cmd_buf.resp_size = SCM_BUFFER_SIZE(struct crypto_asym_rsa_sign_data_resp);
  memset(rsa_sign_data_test_req, 0x0, sizeof(struct crypto_asym_rsa_sign_data_cmd));
  memset(rsa_sign_data_test_resp, 0x0, sizeof(struct crypto_asym_rsa_sign_data_resp));

  // Populate sign data request structure.
  rsa_sign_data_test_req->cmd_id = CRYPTO_STORAGE_SIGN_DATA;
  // Key material is the output from the generate key command.
  rsa_sign_data_test_req->key_blob.key_material = (void *) SCM_BUFFER_PHYS(key_blob_ptr->key_material);
  rsa_sign_data_test_req->key_blob.key_material_len = key_blob_ptr->key_material_len;
  // Plain data
  rsa_sign_data_test_req->data = (uint8_t *) SCM_BUFFER_PHYS(data);
  rsa_sign_data_test_req->data_len = dlen;
  // Signed data
  rsa_sign_data_test_req->signeddata = (uint8_t *) SCM_BUFFER_PHYS(signeddata);
  rsa_sign_data_test_req->signeddata_len = signed_len;

  // Flush memory
  dmac_flush_range(key_blob_ptr->key_material, ((void *)key_blob_ptr->key_material + (sizeof(uint8_t) * (key_blob_ptr->key_material_len))));
  dmac_flush_range(data, ((void *)data + (sizeof(uint8_t) * dlen)));
  dmac_flush_range(signeddata, ((void *)signeddata + (sizeof(uint8_t) * signed_len)));
  dmac_flush_range(rsa_sign_data_test_req, (void *)rsa_sign_data_test_req + (sizeof(struct crypto_asym_rsa_sign_data_cmd)));
  dmac_flush_range(rsa_sign_data_test_resp, (void *)rsa_sign_data_test_resp + (sizeof(struct crypto_asym_rsa_sign_data_resp)));

  // Call into TZ
  rc = scm_call(TZ_SVC_CRYPTO, TZ_CRYPTO_SERVICE_RSA_ID,
    (void *)&scm_cmd_buf,
    SCM_BUFFER_SIZE(scm_cmd_buf),
    NULL,
    0);

  // Check return value
  if (rc) {
    pr_err("%s(): CRYPTO_STORAGE_SIGN_DATA ret: %d\n", __func__, rc);
    goto end;
  }

  // Invalidate cache
  dmac_inv_range(rsa_sign_data_test_resp, (void *)rsa_sign_data_test_resp + (sizeof(struct crypto_asym_rsa_sign_data_resp)));
  dmac_inv_range(signeddata, ((void *)signeddata + (sizeof(uint8_t) * signed_len)));
  dmac_inv_range(data, ((void *)data + (sizeof(uint8_t) * dlen)));

  // Check response status
  if (rsa_sign_data_test_resp->status != 0) {
    pr_info("%s(): scm_call did not return correct status for sign data using RSA key resp->status = %d\n", __func__, rsa_sign_data_test_resp->status);
    rc = -1;
    goto end;
  }

  // Sanity check of cmd_id
  if (CRYPTO_STORAGE_SIGN_DATA != rsa_sign_data_test_resp->cmd_id)
  {
    pr_info("%s(): req cmd_id: %d, resp cmd_id: %d. ERROR.\n", __func__, rsa_sign_data_test_resp->cmd_id, rsa_sign_data_test_req->cmd_id);
    rc = -1;
    goto end;
  }

  *signeddata_len = rsa_sign_data_test_resp->sig_len;
   pr_info("%s()_line%d:, sig_len=%d\n",__func__, __LINE__,*signeddata_len);


end:
  if(rsa_sign_data_test_req)
  {
    kfree(rsa_sign_data_test_req);
    rsa_sign_data_test_req=NULL;
  }
  if(rsa_sign_data_test_resp)
  {
    kfree(rsa_sign_data_test_resp);
    rsa_sign_data_test_resp=NULL;
  }

  return rc;
}


static int tzdev_RSA_verify_data(crypto_rsa_key_blob_type* key_blob_ptr,
              uint8_t* data,uint32_t dlen,uint8_t* signeddata, uint32_t signeddata_len)
{
  // Define request and response structures.
  struct crypto_asym_rsa_verify_data_cmd *rsa_verify_data_test_req = NULL;
  struct crypto_asym_rsa_verify_data_resp *rsa_verify_data_test_resp = NULL;
  int rc = 0;
  struct scm_cmd_buf_s scm_cmd_buf;

  rsa_verify_data_test_req = kmalloc(sizeof(struct crypto_asym_rsa_verify_data_cmd),GFP_KERNEL);
  rsa_verify_data_test_resp = kmalloc(sizeof(struct crypto_asym_rsa_verify_data_resp),GFP_KERNEL);
  if(!rsa_verify_data_test_req || !rsa_verify_data_test_resp)
  {
    pr_err("%s(): RSA verify data: Memory allocation for rsa_sign_data_test_req failed\n", __func__);
    goto end;
  }

  // Populate scm command structure.
  scm_cmd_buf.req_addr = SCM_BUFFER_PHYS(rsa_verify_data_test_req);
  scm_cmd_buf.req_size = SCM_BUFFER_SIZE(struct crypto_asym_rsa_verify_data_cmd);
  scm_cmd_buf.resp_addr = SCM_BUFFER_PHYS(rsa_verify_data_test_resp);
  scm_cmd_buf.resp_size = SCM_BUFFER_SIZE(struct crypto_asym_rsa_verify_data_resp);
  memset(rsa_verify_data_test_req, 0x0, sizeof(struct crypto_asym_rsa_verify_data_cmd));
  memset(rsa_verify_data_test_resp, 0x0, sizeof(struct crypto_asym_rsa_verify_data_resp));

  // Populate sign data request structure.
  rsa_verify_data_test_req->cmd_id = CRYPTO_STORAGE_VERIFY_DATA;
  // Key material is the output from the generate key command.
  rsa_verify_data_test_req->key_blob.key_material = (void *)SCM_BUFFER_PHYS(key_blob_ptr->key_material);
  rsa_verify_data_test_req->key_blob.key_material_len = key_blob_ptr->key_material_len;

  // Signed data is the output from sign data command.
  rsa_verify_data_test_req->signed_data = (uint8_t *) SCM_BUFFER_PHYS(data);
  rsa_verify_data_test_req->signed_dlen = dlen;

  // Signature is the output from sign data command
  rsa_verify_data_test_req->signature = (uint8_t *) SCM_BUFFER_PHYS(signeddata);
  rsa_verify_data_test_req->slen = signeddata_len;

  // Flush memory
  dmac_flush_range(key_blob_ptr->key_material, ((void *)key_blob_ptr->key_material + (sizeof(uint8_t) * (key_blob_ptr->key_material_len))));
  dmac_flush_range(data, ((void *)data + (sizeof(uint8_t) * dlen)));
  dmac_flush_range(signeddata, ((void *)signeddata + (sizeof(uint8_t) * signeddata_len)));
  dmac_flush_range(rsa_verify_data_test_req, (void *)rsa_verify_data_test_req + (sizeof(struct crypto_asym_rsa_verify_data_cmd)));
  dmac_flush_range(rsa_verify_data_test_resp, (void *)rsa_verify_data_test_resp + (sizeof(struct crypto_asym_rsa_verify_data_resp)));

  // Call into TZ
  rc = scm_call(TZ_SVC_CRYPTO, TZ_CRYPTO_SERVICE_RSA_ID,
    (void *)&scm_cmd_buf,
    SCM_BUFFER_SIZE(scm_cmd_buf),
    NULL,
    0);

  // Check return value
  if (rc) {
    pr_err("%s(): CRYPTO_STORAGE_VERIFY_DATA ret: %d\n", __func__, rc);
    goto end;
  }

  // Invalidate cache
  dmac_inv_range(rsa_verify_data_test_resp, (void *)rsa_verify_data_test_resp + (sizeof(struct crypto_asym_rsa_verify_data_resp)));

  // Check response status
  if (rsa_verify_data_test_resp->status != 0)
  {
    pr_info("%s(): scm_call did not return correct status for verify data: resp->status = %d\n", __func__, rsa_verify_data_test_resp->status);
    rc = -1;
    goto end;
  }

  // Sanity check of cmd_id
  if (CRYPTO_STORAGE_VERIFY_DATA!= rsa_verify_data_test_resp->cmd_id)
  {
    pr_info("%s(): req cmd_id: %d, resp cmd_id: %d. ERROR.\n", __func__, CRYPTO_STORAGE_VERIFY_DATA, rsa_verify_data_test_req->cmd_id);
    rc = -1;
    goto end;
  }

end:
  if(rsa_verify_data_test_req)
  {
    kfree(rsa_verify_data_test_req);
    rsa_verify_data_test_req=NULL;
  }
  if(rsa_verify_data_test_resp)
  {
    kfree(rsa_verify_data_test_resp);
    rsa_verify_data_test_resp=NULL;
  }
  return rc;

}

static int tzdev_RSA_export_keys(crypto_rsa_key_blob_type* key_blob_ptr,
  uint8_t *modulus,uint32_t *modulus_size,uint8_t *public_exponent,uint32_t *public_exponent_size)
{
  // Define request and response structures.
  struct crypto_asym_rsa_export_key_cmd *rsa_export_key_test_req = NULL;
  struct crypto_asym_rsa_export_key_resp *rsa_export_key_test_resp = NULL;
  int rc = -1;
  struct scm_cmd_buf_s scm_cmd_buf;

  rsa_export_key_test_req = kmalloc(sizeof(struct crypto_asym_rsa_export_key_cmd),GFP_KERNEL);
  rsa_export_key_test_resp = kmalloc(sizeof(struct crypto_asym_rsa_export_key_resp),GFP_KERNEL);
  if(!rsa_export_key_test_req|| !rsa_export_key_test_resp)
  {
    pr_err("%s(): RSA export keys: Memory allocation for rsa_export_key_test_req/resp failed\n", __func__);
    return -1;
  }

  // Populate scm command structure.
  scm_cmd_buf.req_addr = SCM_BUFFER_PHYS(rsa_export_key_test_req);
  scm_cmd_buf.req_size = SCM_BUFFER_SIZE(struct crypto_asym_rsa_export_key_cmd);
  scm_cmd_buf.resp_addr = SCM_BUFFER_PHYS(rsa_export_key_test_resp);
  scm_cmd_buf.resp_size = SCM_BUFFER_SIZE(struct crypto_asym_rsa_export_key_resp);
  memset(rsa_export_key_test_req, 0x0, sizeof(struct crypto_asym_rsa_export_key_cmd));
  memset(rsa_export_key_test_resp, 0x0, sizeof(struct crypto_asym_rsa_export_key_resp));

  // Populate rsa key export data request structure.
  rsa_export_key_test_req->cmd_id = CRYPTO_STORAGE_EXPORT_PUBKEY;
  // Key material is the output from the generate key command.
  rsa_export_key_test_req->key_blob.key_material = (void *)SCM_BUFFER_PHYS(key_blob_ptr->key_material);//key_blob_ptr->key_material;
  rsa_export_key_test_req->key_blob.key_material_len = key_blob_ptr->key_material_len;
  // Export format is the input parameter for export command
  rsa_export_key_test_req->export_format = CRYPTO_STORAGE_FORMAT_RAW_BYTES; //fixed
  // Modulus
  rsa_export_key_test_req->modulus = (uint8_t *)SCM_BUFFER_PHYS(modulus);
  rsa_export_key_test_req->modulus_size = *modulus_size;
  // Output buffer
  rsa_export_key_test_req->public_exponent = (uint8_t *)SCM_BUFFER_PHYS(public_exponent);
  rsa_export_key_test_req->public_exponent_size = *public_exponent_size;

  // Flush memory
  dmac_flush_range(key_blob_ptr->key_material, ((void *)key_blob_ptr->key_material + (sizeof(uint8_t) * (key_blob_ptr->key_material_len))));
  dmac_flush_range(public_exponent, ((void *)public_exponent + (sizeof(uint8_t) * (*public_exponent_size))));
  dmac_flush_range(modulus, ((void *)modulus + (sizeof(uint8_t) * (*modulus_size))));
  dmac_flush_range(rsa_export_key_test_req, (void *)rsa_export_key_test_req + (sizeof(struct crypto_asym_rsa_export_key_cmd)));
  dmac_flush_range(rsa_export_key_test_resp, (void *)rsa_export_key_test_resp + (sizeof(struct crypto_asym_rsa_export_key_resp)));

  // Call into TZ
  rc = scm_call(TZ_SVC_CRYPTO, TZ_CRYPTO_SERVICE_RSA_ID,
    (void *)&scm_cmd_buf,
    SCM_BUFFER_SIZE(scm_cmd_buf),
    NULL,
    0);

  // Check return value
  if (rc)
  {
    pr_info("%s(): scm_call CRYPTO_STORAGE_EXPORT_PUBKEY failed, rc = %d\n", __func__, rc);
    goto end;
  }

  // Invalidate cache
  dmac_inv_range(rsa_export_key_test_resp, (void *)rsa_export_key_test_resp + (sizeof(struct crypto_asym_rsa_export_key_resp)));
  dmac_inv_range(public_exponent, (void *)public_exponent + (sizeof(uint8_t) * (*public_exponent_size)));
  dmac_inv_range(modulus, (void *)modulus + (sizeof(uint8_t) * (*modulus_size)));
  // Check response status
  if (rsa_export_key_test_resp->status != 0)
  {
    pr_info("%s(): scm_call did not return correct status for export public RSA Key: resp->status = %d\n", __func__, rsa_export_key_test_resp->status);
    rc = -1;
    goto end;
  }

  // Sanity check of cmd_id
  if (CRYPTO_STORAGE_EXPORT_PUBKEY != rsa_export_key_test_resp->cmd_id)
  {
    pr_info("%s(): req cmd_id: %d, resp cmd_id: %d. ERROR.\n", __func__, CRYPTO_STORAGE_EXPORT_PUBKEY, rsa_export_key_test_resp->cmd_id);
    rc = -1;
    goto end;
  }

  *modulus_size = rsa_export_key_test_resp->modulus_size;
  *public_exponent_size = rsa_export_key_test_resp->public_exponent_size;

end:
  // Free output buffer
  if(rsa_export_key_test_req)
  {
    kfree(rsa_export_key_test_req);
    rsa_export_key_test_req=NULL;
  }
  if(rsa_export_key_test_resp)
  {
    kfree(rsa_export_key_test_resp);
    rsa_export_key_test_resp=NULL;
  }

  return rc;
}

static int tzdev_RSA_import_keys(uint8_t *modulus,uint32_t modulus_size,
  uint8_t *pubExp,uint32_t pubExpSize,uint8_t  *privExp, uint32_t privExpSize,
  tz_rsa_digest_pad_algo_t padding, crypto_rsa_key_blob_type* key_blob_ptr)
{
  // Define request and response structures.
  crypto_storage_rsa_import_key_cmd_t *rsa_import_key_cmd = NULL;
  crypto_storage_rsa_import_key_resp_t *rsa_import_key_resp = NULL;
  //rsa_scm_cmd_buf_t rsa_scm_cmd;
  struct scm_cmd_buf_s scm_cmd_buf;
  int rc = 0;

  // Allocate memory for command
  rsa_import_key_cmd = kmalloc(sizeof(crypto_storage_rsa_import_key_cmd_t), GFP_KERNEL);
  // Allocate memory for response
  rsa_import_key_resp = kmalloc(sizeof(crypto_storage_rsa_import_key_resp_t), GFP_KERNEL);
  if (NULL == rsa_import_key_cmd || NULL == rsa_import_key_resp)
  {
    pr_err("%s()RSA import keys: Memory allocation for rsa import keys req/resp commands failed\n", __func__);
    return -1;
  }

  // Populate the SCM command structure.
  scm_cmd_buf.req_addr = SCM_BUFFER_PHYS(rsa_import_key_cmd);
  scm_cmd_buf.req_size = SCM_BUFFER_SIZE(crypto_storage_rsa_import_key_cmd_t);
  scm_cmd_buf.resp_addr = SCM_BUFFER_PHYS(rsa_import_key_resp);
  scm_cmd_buf.resp_size = SCM_BUFFER_SIZE(crypto_storage_rsa_import_key_resp_t);
  // Clear cmd and resp structure.
  memset(rsa_import_key_cmd, 0x0, sizeof(crypto_storage_rsa_import_key_cmd_t));
  memset(rsa_import_key_resp, 0x0, sizeof(crypto_storage_rsa_import_key_resp_t));

  // Populate input data
  rsa_import_key_cmd->cmd_id = CRYPTO_STORAGE_IMPORT_KEY;
  rsa_import_key_cmd->digest_pad_type = padding;
  memcpy(rsa_import_key_cmd->modulus, modulus, modulus_size);
  rsa_import_key_cmd->modulus_size = modulus_size;
  memcpy(rsa_import_key_cmd->public_exponent, pubExp, pubExpSize);
  rsa_import_key_cmd->public_exponent_size = pubExpSize;
  memcpy(rsa_import_key_cmd->private_exponent, privExp, privExpSize);
  rsa_import_key_cmd->private_exponent_size = privExpSize;

  rsa_import_key_cmd->key_blob.key_material = (crypto_rsa_key_type*)SCM_BUFFER_PHYS(key_blob_ptr->key_material);
  rsa_import_key_cmd->key_blob.key_material_len = key_blob_ptr->key_material_len;

  // Flush buffers from cache to memory.
  dmac_flush_range(rsa_import_key_cmd, ((void *)rsa_import_key_cmd) + sizeof(crypto_storage_rsa_import_key_cmd_t));
  dmac_flush_range(rsa_import_key_resp, ((void *)rsa_import_key_resp) + sizeof(crypto_storage_rsa_import_key_resp_t));
  dmac_flush_range(key_blob_ptr->key_material, (void *)(key_blob_ptr->key_material) + sizeof(crypto_rsa_key_type));

  // Send command to TrustZone.
  rc = scm_call(TZ_SVC_CRYPTO, TZ_CRYPTO_SERVICE_RSA_ID,
      (void *)&scm_cmd_buf,
      SCM_BUFFER_SIZE(scm_cmd_buf),
      NULL,
      0);

  if (rc)
  {
    pr_info("%s(): scm_call CRYPTO_STORAGE_IMPORT_KEY failed, rc = %d\n", __func__, rc);
    goto end;
  }

  dmac_inv_range(rsa_import_key_resp, ((void *)rsa_import_key_resp) + sizeof(crypto_storage_rsa_import_key_resp_t));
  dmac_inv_range(key_blob_ptr->key_material, (void *)(key_blob_ptr->key_material) + sizeof(crypto_rsa_key_type));

  if (rsa_import_key_resp->status != 0)
  {
    pr_info("%s(): scm_call did not return correct status for import RSA Key , resp->status = %d\n", __func__, rsa_import_key_resp->status);
    rc = -1;
    goto end;
  }

  if (CRYPTO_STORAGE_IMPORT_KEY != rsa_import_key_resp->cmd_id)
  {
    pr_info("%s(): req cmd_id: %d, resp cmd_id: %d. ERROR.\n", __func__, rsa_import_key_cmd->cmd_id, rsa_import_key_resp->cmd_id);
    rc = -1;
    goto end;
  }
end:
  if (rsa_import_key_cmd != NULL)
  {
    kfree(rsa_import_key_cmd);
    rsa_import_key_cmd = NULL;
  }

  // Free memory for response
  if (rsa_import_key_resp != NULL)
  {
    kfree(rsa_import_key_resp);
    rsa_import_key_resp = NULL;
  }
  return rc;

}

static void sierra_tzdev_free_req_buffers(struct tzdev_ioctl_ctx *tic)
{
  struct tzdev_op_req *krn = &tic->krn;

  if (krn->encklen)
    kfree(krn->enckey);
  if (krn->plain_data)
    kfree(krn->plain_data);
  if (krn->encrypted_buffer)
    kfree(krn->encrypted_buffer);
}

//
// This is called at the beginning of the ioctl operation
// to copy the request structure from user to kernel space.
// After this we have all the pointers and lengths; we still
// need to do copy_from_user to get the data itself.
//
// Here we also store the original user space request pointer
// into the tic structure. Later we need that when we copy
// in the reverse direction as the last step of the ioctl,
// in sierra_tzdev_copy_to_user.
//
// The two levels of copying from user space are split into the
// two functions sierrqa_tzdev_ioctl_prepare and sierra_tzdev_copy_from_user
// because the second function is invoked more than once to
// copy the selected buffers or to just allocate memory.
//
// tic:         context holding copy of user request and
//              the kernel facsimile of that structure
// req:         pointer to user request
//
// Returns a standard Linux error: -EFAULT or 0.
//
static int sierra_tzdev_ioctl_prepare(struct tzdev_ioctl_ctx *tic,
                                      struct tzdev_op_req __user *req)
{
  tic->orig_usr = req;
  return copy_from_user(&tic->usr, req, sizeof tic->usr) ? -EFAULT : 0;
}

//
// Copy selected parts of the tzdev_op_req data from user
// space to kernel space. This is used by sierra_tzdev_ioctl
// for receiving in or in/out parameters, and for reserving space
// for out parameters.
//
// Comments in the sierra_tzdev_ioctl, where it handles the
// TZDEV_IOCTL_SEAL_REQ command, give some hints about the usage
// of these functions.
//
// tic:         context holding copy of user request and
//              the kernel facsimile of that structure
// flags:       bitmask indicating which fields to operate on.
// alloc_only:  boolean indicating that a buffers are to be
//              allocated only, without doing any copying
//              from user space. This is useful for preparing
//              space for data to be returned to user space later
//              (i.e. "out parameters" of the ioctl).
//
// For every field group specified by flags, the function reads the length of
// the buffer from the user space src parameter, and copies that length to the
// same field in the dst parameter. Then in the dst structure, it allocates a
// buffer that large, and puts it in the pointer associated with the length.
// Unless alloc_only is true, then a copy_from_user operation is performed
// to fill the allocated buffer from its user space counterpart.
//
// Returns a standard Linux error: 0 success, -EFAULT, etc.
//
static int sierra_tzdev_copy_from_user(struct tzdev_ioctl_ctx *tic,
                                       unsigned flags,
                                       bool alloc_only)
{
  int rc = -EFAULT;
  struct tzdev_op_req *src = &tic->usr;
  struct tzdev_op_req *dst = &tic->krn;

  if (flags & TZDEV_COPY_ENCKEY) {
    dst->encklen = src->encklen;
    if ((dst->enckey = kmalloc(dst->encklen, GFP_KERNEL)) == NULL) {
      printk(KERN_CRIT "%s()_line%d: cannot allocate key_material\n",
             __func__, __LINE__);
      rc = -ENOMEM;
      goto out;
    }
    if (!alloc_only &&
        copy_from_user(dst->enckey,
                       src->enckey,
                       dst->encklen) != 0)
      goto out;
  }

  if (flags & TZDEV_COPY_PLAIN_DATA) {
    dst->plain_dlen = src->plain_dlen;
    if ((dst->plain_data = kmalloc(dst->plain_dlen, GFP_KERNEL)) == NULL) {
      printk(KERN_CRIT "%s()_line%d: cannot allocate plain data\n",
             __func__, __LINE__);
      rc = -ENOMEM;
      goto out;
    }
    if (!alloc_only && copy_from_user(dst->plain_data,
                                      src->plain_data,
                                      dst->plain_dlen) != 0)
      goto out;
  }

  if (flags & TZDEV_COPY_ENCRYPTED_BUFFER) {
    dst->encrypted_len = src->encrypted_len;
    if ((dst->encrypted_buffer = kmalloc(dst->encrypted_len, GFP_KERNEL)) == NULL) {
      printk(KERN_CRIT "%s()_line%d: cannot allocate sealed data\n",
             __func__, __LINE__);
      rc = -ENOMEM;
      goto out;
    }
    if (!alloc_only && copy_from_user(dst->encrypted_buffer,
                                      src->encrypted_buffer,
                                      dst->encrypted_len) != 0)
      goto out;
  }

  rc = 0;

out:
  if (rc != 0 && rc != -ENOMEM)
    printk(KERN_ERR "%s: copy_from_user failed\n", __func__);

  return rc;
}

//
// Copy selected parts of the tzdev_op_req data from kernel
// space to user space. This is used by sierra_tzdev_ioctl
// for passing out parmeters back to user space.
//
// tic:         context holding copy of user request and
//              the kernel facsimile of that structure
// flags:       bitmask indicating which fields to operate on.
//
// For every field group specified by flags, the function reads the length of
// the buffer from the kernel space src parameter, and copies that length to the
// same field in the user-space dst structure. It also copies the associated
// buffer from kernel space to user space, using the respective pointer
// fields in the two structures.
//
// Returns a standard Linux error: 0 success, -EFAULT, etc.
//
static int sierra_tzdev_copy_to_user(struct tzdev_ioctl_ctx *tic,
                                     unsigned flags)
{
  int rc = -EFAULT;
  struct tzdev_op_req *src = &tic->krn;
  struct tzdev_op_req *dst = &tic->usr;

  if (flags & TZDEV_COPY_ENCKEY) {
    dst->encklen = src->encklen;
    if ((copy_to_user(dst->enckey, src->enckey, src->encklen)) != 0)
      goto out;
  }

  if (flags & TZDEV_COPY_PLAIN_DATA) {
    dst->plain_dlen = src->plain_dlen;
    if ((copy_to_user(dst->plain_data, src->plain_data, src->plain_dlen)) != 0)
      goto out;
  }

  if (flags & TZDEV_COPY_ENCRYPTED_BUFFER) {
    dst->encrypted_len = src->encrypted_len;
    if ((copy_to_user(dst->encrypted_buffer, src->encrypted_buffer,
                      src->encrypted_len)) != 0)
      goto out;
  }

  if ((copy_to_user(tic->orig_usr, &tic->usr, sizeof *tic->orig_usr)) != 0)
    goto out;

  rc = 0;

out:
  if (rc != 0)
    printk(KERN_ERR "%s: copy_to_user/put_user failed\n", __func__);

  return rc;
}

static long sierra_tzdev_ioctl(struct file *file, unsigned cmd, unsigned long arg)
{
  struct tzdev_ioctl_ctx tic = { 0 };
  crypto_rsa_key_blob_type key_blob;
  struct tzdev_op_req *krn = &tic.krn;
  int rc  = sierra_tzdev_ioctl_prepare(&tic,
                                       (struct tzdev_op_req __user *) arg);

  if (rc != 0)
    goto out;

  switch (cmd)
  {
    /*Generate keyblob*/
    case TZDEV_IOCTL_KEYGEN_REQ:
      {
        uint32_t output_len;

        if ((rc = sierra_tzdev_copy_from_user(&tic,
                                              TZDEV_COPY_ENCKEY,
                                              true)) != 0)
          goto out;

        output_len = krn->encklen;

        rc = tzdev_storage_service_generate_key(krn->enckey, &output_len);

        pr_info("%s()_line%d:TZDEV_IOCTL_KEYGEN_REQ, get key_size:%d, rc=%d\n",
                 __func__, __LINE__, output_len, rc);

        if (rc != 0)
          goto out;

        if (output_len > krn->encklen) {
          rc = -EFAULT;
          goto out;
        }

        krn->encklen = output_len;

        rc = sierra_tzdev_copy_to_user(&tic, TZDEV_COPY_ENCKEY);
      }
      break;

    /* encrypt data */
    case TZDEV_IOCTL_SEAL_REQ:
      {
        uint32_t output_len;

        //
        // Copy the in-parameters from user space to kernel space.
        // For this operation, we need the encryption key, and the plain data,
        // so we specify these two in the bitmask.
        //
        if ((rc = sierra_tzdev_copy_from_user(&tic,
                                              TZDEV_COPY_ENCKEY | TZDEV_COPY_PLAIN_DATA,
                                              false)) != 0) // alloc_only is false: we really copy
          goto out;

        //
        // We must reserve, in the kernel-side request structure, a buffer to
        // receive the encrypted block. To do this we use the same function, this
        // time using the flags parameter to select the encrypted buffer,
        // and set alloc_only to true, so the payload is not copied from user
        // space, only the length field is copied. The kernel-side buffer is reserved
        // only, without being initialized from user space.
        //
        if ((rc = sierra_tzdev_copy_from_user(&tic,
                                              TZDEV_COPY_ENCRYPTED_BUFFER,
                                              true)) != 0) // alloc_only true: reserve space only
          goto out;

        //
        // The kernel buffer now has the encrypted_len set by the second
        // sierra_tzdev_copy_from_user above. We use this temporary variable,
        // rather than passing &krn->encrypted_len directly to the crypto
        // routine. We can then perform a sanity check on this.
        //
        output_len = krn->encrypted_len;

        rc = tzdev_seal_data_using_aesccm(krn->plain_data, krn->plain_dlen,
                                          krn->encrypted_buffer, &output_len,
                                          krn->enckey, krn->encklen);

        pr_info("%s()_line%d: TZDEV_IOCTL_SEAL_REQ: plain_data_len:%d, seal_data_len:%d, rc=%d\n",
                __func__, __LINE__, krn->plain_dlen, output_len, rc);

        if (rc != 0)
          goto out;

        if (output_len > krn->encrypted_len) {
          rc = -EFAULT;
          goto out;
        }

        //
        // Store back the actual length reported by the crypto function.
        //
        krn->encrypted_len = output_len;

        //
        // Now propagate the encrypted buffer up to user space.
        //
        rc = sierra_tzdev_copy_to_user(&tic, TZDEV_COPY_ENCRYPTED_BUFFER);
      }
      break;

    /* decrypt data */
    case TZDEV_IOCTL_UNSEAL_REQ:
      {
        uint32_t output_len;

        if ((rc = sierra_tzdev_copy_from_user(&tic,
                                              TZDEV_COPY_ENCKEY | TZDEV_COPY_ENCRYPTED_BUFFER,
                                              false)) != 0)
          goto out;

        if ((rc = sierra_tzdev_copy_from_user(&tic,
                                              TZDEV_COPY_PLAIN_DATA,
                                              true)) != 0)
          goto out;

        output_len = krn->plain_dlen;

        rc = tzdev_unseal_data_using_aesccm(krn->encrypted_buffer, krn->encrypted_len,
                                            krn->plain_data, &output_len,
                                            krn->enckey, krn->encklen);
        pr_info("%s()_line%d: TZDEV_IOCTL_UNSEAL_REQ: sealed data len:%d, plain_data_len:%d, rc=%d\n",
                 __func__, __LINE__, krn->encrypted_len, output_len, rc);

        if (rc != 0)
          goto out;

        if (output_len > krn->plain_dlen) {
          rc = -EFAULT;
          goto out;
        }

        krn->plain_dlen = output_len;

        rc = sierra_tzdev_copy_to_user(&tic, TZDEV_COPY_PLAIN_DATA);
      }
      break;

      /*Generate RSA keyblob*/
    case TZDEV_IOCTL_RSA_KEYPAIR_REQ:
      {
        if ((rc = sierra_tzdev_copy_from_user(&tic,
                                              TZDEV_COPY_PLAIN_DATA,
                                              false)) != 0)
          goto out;
        if ((rc = sierra_tzdev_copy_from_user(&tic,
                                              TZDEV_COPY_ENCKEY,
                                              true)) != 0)
          goto out;

        key_blob.key_material = (crypto_rsa_key_type *) krn->enckey;
        key_blob.key_material_len = krn->encklen;

        /* OOPS: what if plain_datap isn't long enough? */
        rc = tzdev_generate_RSA_keypair(&key_blob, krn->plain_data);

        pr_info("%s()_line%d:TZDEV_IOCTL_RSA_KEYPAIR_REQ, get key_size:%d, rc=%d\n",
                __func__, __LINE__, key_blob.key_material_len, rc);

        if (rc != 0)
          goto out;

        if (key_blob.key_material_len > krn->encklen) {
          rc = -EFAULT;
          goto out;
        }

        krn->encklen = key_blob.key_material_len;

        rc = sierra_tzdev_copy_to_user(&tic, TZDEV_COPY_ENCKEY);
      }
      break;

      /* sign data */
    case TZDEV_IOCTL_RSA_SIGN_REQ:
      {
        uint32_t output_len;

        if ((rc = sierra_tzdev_copy_from_user(&tic,
                                              TZDEV_COPY_ENCKEY | TZDEV_COPY_PLAIN_DATA,
                                              false)) != 0)
          goto out;
        if ((rc = sierra_tzdev_copy_from_user(&tic,
                                              TZDEV_COPY_ENCRYPTED_BUFFER,
                                              true)) != 0)
          goto out;

        key_blob.key_material_len = krn->encklen;
        key_blob.key_material = (crypto_rsa_key_type* ) krn->enckey;

        output_len = krn->encrypted_len;

        rc = tzdev_RSA_sign_data(&key_blob, krn->plain_data, krn->plain_dlen,
                                 krn->encrypted_buffer, &output_len);
        pr_info("%s()_line%d: TZDEV_IOCTL_RSA_SIGN_REQ: plain_data_len:%d, seal_data_len:%d, rc=%d\n",
                __func__, __LINE__, krn->plain_dlen, output_len, rc);

        if (rc != 0)
          goto out;

        if (output_len > krn->encrypted_len) {
          rc = -EFAULT;
          goto out;
        }

        krn->encrypted_len = output_len;

        rc = sierra_tzdev_copy_to_user(&tic, TZDEV_COPY_ENCRYPTED_BUFFER);
      }
      break;

      /* verify data */
    case TZDEV_IOCTL_RSA_VERIFY_REQ:
      {
        if ((rc = sierra_tzdev_copy_from_user(&tic,
                                              TZDEV_COPY_ENCKEY | TZDEV_COPY_PLAIN_DATA | TZDEV_COPY_ENCRYPTED_BUFFER,
                                              false)) != 0)
          goto out;

        key_blob.key_material_len = krn->encklen;
        key_blob.key_material = (crypto_rsa_key_type* ) krn->enckey;

        rc = tzdev_RSA_verify_data(&key_blob, krn->plain_data, krn->plain_dlen,
                                   krn->encrypted_buffer, krn->encrypted_len);
      }
      break;

      /*export public key from keyblob*/
    case TZDEV_IOCTL_RSA_EXPORTKEY_REQ:
      {
        if ((rc = sierra_tzdev_copy_from_user(&tic,
                                              TZDEV_COPY_ENCKEY,
                                              false)) != 0)
          goto out;
        if ((rc = sierra_tzdev_copy_from_user(&tic,
                                              TZDEV_COPY_PLAIN_DATA | TZDEV_COPY_ENCRYPTED_BUFFER,
                                              true)) != 0)
          goto out;

        key_blob.key_material_len = krn->encklen;
        key_blob.key_material = (crypto_rsa_key_type* ) krn->enckey;

        rc = tzdev_RSA_export_keys(&key_blob, krn->plain_data, &krn->plain_dlen,
                                   krn->encrypted_buffer, &krn->encrypted_len);
        pr_info("%s()_line%d:TZDEV_IOCTL_RSA_EXPORTKEY_REQ, get modulus size:%d, pub exp size:%d, rc=%d\n",
                __func__, __LINE__, krn->plain_dlen, krn->encrypted_len, rc);

        if (rc != 0)
          goto out;

        rc = sierra_tzdev_copy_to_user(&tic, TZDEV_COPY_PLAIN_DATA | TZDEV_COPY_ENCRYPTED_BUFFER);
      }
      break;

      /*import key to geneate keyblob*/
    case TZDEV_IOCTL_RSA_IMPORTKEY_REQ:
      {
        uint32_t output_len;

        if ((rc = sierra_tzdev_copy_from_user(&tic,
                                              TZDEV_COPY_ENCKEY,
                                              false)) != 0)
          goto out;

        key_blob.key_material_len = krn->encklen;
        key_blob.key_material = (crypto_rsa_key_type* ) krn->enckey;

        output_len = krn->encklen;

        rc = tzdev_RSA_import_keys(key_blob.key_material->modulus, key_blob.key_material->modulus_size,
                                   key_blob.key_material->public_exponent, key_blob.key_material->public_exponent_size,
                                   key_blob.key_material->encrypted_private_exponent, key_blob.key_material->encrypted_private_exponent_size,
                                   key_blob.key_material->digest_padding, &key_blob);
        pr_info("%s()_line%d:TZDEV_IOCTL_RSA_IMPORTKEY_REQ, get key_size:%d, rc=%d\n",
                __func__, __LINE__, key_blob.key_material_len, rc);

        if (rc != 0)
          goto out;

        if (output_len > krn->encklen) {
          rc = -EFAULT;
          goto out;
        }

        rc = sierra_tzdev_copy_to_user(&tic, TZDEV_COPY_ENCKEY);
      }
      break;

    default:
      rc = -EINVAL;
      break;
  }

out:
  sierra_tzdev_free_req_buffers(&tic);
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


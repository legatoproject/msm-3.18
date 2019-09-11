/*
********************************************************************************
* Name:  sierra_ks_if.c
*
* Sierra Wireless keystore interface module
*
*===============================================================================
* Copyright (C) 2019 Sierra Wireless Inc.
********************************************************************************
*/

/* Define how pr_* prints would look like. */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

/* Some useful info about this driver. */
#define DRV_VERSION     "1.0"
#define DRV_DESCRIPTION "Sierra Wireless keystore interface device driver"
#define DRV_COPYRIGHT   "(C) 2019 Sierra Wireless Inc."
#define DRV_AUTHOR      "Dragan Marinkovic <dmarinkovi@sierrawireless.com>"

/* Do this to avoid full path in __FILE__ macro. If we wanted to, we could
   define THIS_FILENAME as __FILE__ and it would be a standard. */
#define THIS_FILENAME SIERRA_KS_IF_DRV_NAME".c"

/* Include files. */
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/printk.h>
#include <linux/miscdevice.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/crc32.h>
#include <linux/stddef.h>
#include "sierra_ks_if.h"
#include <../sierra/api/ssmemudefs.h>

/* Device minor will be automatically selected by the system. */
#define SIERRA_KS_IF_MINOR (MISC_DYNAMIC_MINOR)

/* Keystore read retry timeout (in ms) */
#define SIERRA_KS_IF_KEYSTORE_READ_TOUT_MS  (5000)

/* Keystore read init delay (in ms) */
#define SIERRA_KS_IF_KEYSTORE_INITIAL_READ_DELAY_MS  (300)

/* Structures */

/* Keystore read state. */
typedef enum
{
    SIERRA_KS_IF_KEYSTORE_DATA_STATE_UNKNOWN = 0,
    SIERRA_KS_IF_KEYSTORE_DATA_STATE_NO_DATA,
    SIERRA_KS_IF_KEYSTORE_DATA_STATE_AVAILABLE,
    SIERRA_KS_IF_KEYSTORE_DATA_STATE_MAX
} SIERRA_KS_IF_KEYSTORE_DATA_STATE_E;

/* Driver debug flag, and useful debug macro. */
static unsigned char sierra_ks_if_debug_flg = 0;
#define SIERRA_KS_IF_DEBUG(do_print, fmt, arg...) \
if(do_print==TRUE) \
    do { \
        if(sierra_ks_if_debug_flg) \
            pr_notice("(%s:%s:%d): " fmt,THIS_FILENAME,__FUNCTION__,__LINE__,##arg); \
    } while(0)


/* Proc entry data for this driver. */
typedef struct {
    struct proc_dir_entry* root_dir;
    struct proc_dir_entry* debug_file;
    struct proc_dir_entry* stats;
    char *root_dir_name;
    char *stats_file_name;
    char *debug_file_name;
    unsigned char debug_file_read_done;
    unsigned char stats_file_read_done;
} sierra_ks_if_proc_entry_info;
sierra_ks_if_proc_entry_info sierra_ks_if_proc_entry =
{
    NULL,
    NULL,
    NULL,
    "driver/"SIERRA_KS_IF_DRV_NAME,
    "stats",
    "debug",
    FALSE,
    FALSE,
};

/* Keystore related data. All key related data will be stored here. The reason
   for strict typing here is because we need to enforce strict compatibility
   between processors.
 */
typedef struct {
    struct list_head list;

    /* Key type. Must match one in keytype_t list. */
    keytype_t key_type;

    /* The length of this key, in bytes. */
    uint32_t  key_length;

    /* Pointer to the key storage. */
    uint8_t   *key;
} sierra_ks_if_data_info;

/* Local Keystore data linked list. */
static struct list_head sierra_ks_if_head_list;
static DEFINE_RWLOCK(sierra_ks_if_list_lock);

/* Lock for userland data read. */
static DEFINE_MUTEX(data_read_lock);

/* keystore read timer. */
static struct timer_list sierra_ks_if_read_timer;

/*
 * Local method specs.
 */

/* Driver entry/exit. */
static int sierra_ks_if_init(void);
static void sierra_ks_if_cleanup(void);

/* Driver userland device interface */
static ssize_t sierra_ks_if_dev_read(struct file *file, char *buf, size_t count,
                                    loff_t *ppos);
static ssize_t sierra_ks_if_dev_write(struct file *file, const char *data,
                                     size_t len, loff_t * ppos);
static long sierra_ks_if_dev_ioctl(struct file *filp, unsigned int cmd,
                                   unsigned long arg);
static int sierra_ks_if_dev_mmap(struct file *filp, struct vm_area_struct *vma);
static int sierra_ks_if_dev_open(struct inode *inode, struct file *file);
static int sierra_ks_if_dev_release(struct inode *inode, struct file *file);

/* Driver proc interface. */
static int sierra_ks_if_create_proc_entry(void);
static void sierra_ks_if_remove_proc_entry(void);

/* for debug entry... */
static int sierra_ks_if_debug_proc_open(struct inode *inode, struct file *file);
static ssize_t sierra_ks_if_debug_proc_read(struct file *file, char *data,
                                            size_t len, loff_t *ppos);
static ssize_t sierra_ks_if_debug_proc_write(struct file *, const char __user *,
                                              size_t, loff_t *);
static int sierra_ks_if_debug_proc_release(struct inode *inode,
                                           struct file *file);

/* for stats entry ... */
static int sierra_ks_if_stats_proc_open(struct inode *inode, struct file *file);
static ssize_t sierra_ks_if_stats_proc_read(struct file *file, char *data,
                                            size_t len, loff_t *ppos);
static int sierra_ks_if_stats_proc_release(struct inode *inode,
                                           struct file *file);

/* Driver debug interface. */
static void __inline__ sierra_ks_if_debug_enable(void);
static void __inline__ sierra_ks_if_debug_disable(void);
static unsigned char __inline__ sierra_ks_if_debug_get(void);

/* Keystore related functions. */
static int sierra_ks_if_scrape_keystore_keys(void);
static int sierra_ks_if_get_key(sierra_ks_if_drv_msg *drv_msg);
static void sierra_ks_if_keydata_init(void);
static int sierra_ks_if_list_add(key_store_rec_t *kstore_rec);
static void sierra_ks_if_list_del_all(void);
void sierra_ks_if_print_all_keys(void);

static void sierra_ks_if_get_keystore_keys_callback(unsigned long state);
static int sierra_ks_if_set_keystore_read_timer(void);
static void sierra_ks_if_clear_keystore_read_timer(void);

/* sierra_ks_if device file operations. */
static struct file_operations sierra_ks_if_dev_fops = {
    .owner              = THIS_MODULE,
    .read               = sierra_ks_if_dev_read,
    .write              = sierra_ks_if_dev_write,
    .unlocked_ioctl     = sierra_ks_if_dev_ioctl,
    .compat_ioctl       = sierra_ks_if_dev_ioctl,
    .mmap               = sierra_ks_if_dev_mmap,
    .open               = sierra_ks_if_dev_open,
    .release            = sierra_ks_if_dev_release,
};

/* debug entry /proc file operations */
static struct file_operations sierra_ks_if_debug_proc_fops = {
    .owner      = THIS_MODULE,
    .open       = sierra_ks_if_debug_proc_open,
    .read       = sierra_ks_if_debug_proc_read,
    .write      = sierra_ks_if_debug_proc_write,
    .release    = sierra_ks_if_debug_proc_release,
};

/* stats entry /proc file operations */
static struct file_operations sierra_ks_if_stats_proc_fops = {
    .owner      = THIS_MODULE,
    .open       = sierra_ks_if_stats_proc_open,
    .read       = sierra_ks_if_stats_proc_read,
    .release    = sierra_ks_if_stats_proc_release,
};

/*******************************************************************************
 * Setup keystore read timer.
 * The reason we need this timer is because we would allow keystore service
 * to show up at the later time, if required.
 ******************************************************************************/
int sierra_ks_if_set_keystore_read_timer(void)
{
    int ret = 0;

    /* timer.function, timer.data */
    setup_timer(&sierra_ks_if_read_timer,
                sierra_ks_if_get_keystore_keys_callback,
                0);

    /* Set current state. */
    sierra_ks_if_read_timer.data =
        (unsigned long)SIERRA_KS_IF_KEYSTORE_DATA_STATE_NO_DATA;

    /* Arm pin state timer. */
    ret = mod_timer(&sierra_ks_if_read_timer,
                    jiffies + msecs_to_jiffies(SIERRA_KS_IF_KEYSTORE_INITIAL_READ_DELAY_MS));
    if (ret) {
        ret = -EPERM;
    }

    return ret;
}

/*******************************************************************************
 * Called once timer expires.
 ******************************************************************************/
void sierra_ks_if_get_keystore_keys_callback(unsigned long state)
{
    unsigned char delete_timer = 0;

    switch(state) {

        case SIERRA_KS_IF_KEYSTORE_DATA_STATE_NO_DATA:
        {
            /* Try to get the data. */
            pr_warning("Trying to get keystore data...\n");
            if( sierra_ks_if_scrape_keystore_keys() == 0 ) {
                pr_warning("Keystore data available.\n");
                sierra_ks_if_read_timer.data =
                    SIERRA_KS_IF_KEYSTORE_DATA_STATE_AVAILABLE;

                /* Do not arm timer, delete it. */
                delete_timer++;
            }
            else {
                pr_warning("Keystore data not yet available.\n");
            }
        }
        break;

        case SIERRA_KS_IF_KEYSTORE_DATA_STATE_AVAILABLE:
        {
            pr_warning("Keystore data already available.\n");

            /* Do not arm timer, delete it. */
            delete_timer++;
        }
        break;

        default:
        {
            /* Do not arm the timer, delete it. */
            delete_timer++;
        }
        break;
    }

    if(delete_timer > 0) {
        pr_warning("Deleting keystore read timer.");
        sierra_ks_if_clear_keystore_read_timer();
    }
    else {
        int ret = 0;

        /* Arm timer. */
        ret = mod_timer(&sierra_ks_if_read_timer,
                        jiffies + msecs_to_jiffies(SIERRA_KS_IF_KEYSTORE_READ_TOUT_MS));
        if(ret) {
            /* Nothing we can do about it. */
            pr_err("Keystore read timer could not be armed, state=%ld\n", state);
            sierra_ks_if_clear_keystore_read_timer();
        }
    }
}

/*******************************************************************************
 * Remove keystore read timer.
 ******************************************************************************/
void sierra_ks_if_clear_keystore_read_timer(void)
{
    del_timer(&sierra_ks_if_read_timer);
}

uint32_t sierra_ks_if_key_id_to_enum(
    char id[4]
)
{
  if (!strncmp(id, "IMA0", 4))
  {
    return IMA0;
  }
  else if (!strncmp(id, "RFS0", 4))
  {
    return RFS0;
  }
  else if (!strncmp(id, "LGT0", 4))
  {
    return LGT0;
  }
  else
  {
    return OEM0;
  }
}

/*******************************************************************************
* Get the the keys from the keystore. This is where we read them and store them
* locally.
* This method could be called from interrupt context (timer function callback
* is one of them). If this is not needed, GFP_KERNEL flags should be used in
* kmalloc (it has less chance of failing).
*******************************************************************************/
int sierra_ks_if_scrape_keystore_keys(void)
{
#define KMALLOC_FLAGS   (GFP_ATOMIC)

    static key_store_rec_t *keystore_rec;
    sec_ssmem_key_hdr_s *flash_key_hdr_p;
    int ret = 0;

    int offset = SEC_OEM_KEY_LENGTH;
    uint8_t* keysp;
    int size;
    //SMAC here is your decision point of whether to have shared memory or flash reads.
    // the items are coming in raw, so it doesn't matter from where on a data point.
    // comes in raw and then is populated into the ks driver. ks driver serves it up.
    //
    //#ifdef KEYSTORE_FLASH_READ
    //keysp = flash_keys_get
    //#elseif KEYSTORE_SHARED_MEM
    keysp = ssmem_keys_get(&size);
    //#endif
    pr_notice("keysp %p size %d", keysp, size);

    SIERRA_KS_IF_DEBUG(TRUE, "Scraping keystore keys...\n");

    while(offset < size)
    {
        flash_key_hdr_p = (sec_ssmem_key_hdr_s*) (keysp + offset);
        pr_notice("key length %d at offset %d\n", flash_key_hdr_p->length, offset);

        keystore_rec = (key_store_rec_t *)kmalloc(sizeof(key_store_rec_t) +
                flash_key_hdr_p->length + 1,
                KMALLOC_FLAGS);

        keystore_rec->version = flash_key_hdr_p->version;
        keystore_rec->key_type = sierra_ks_if_key_id_to_enum(flash_key_hdr_p->id);
        memcpy(keystore_rec->key, keysp + offset + sizeof(sec_ssmem_key_hdr_s),
                flash_key_hdr_p->length);

        keystore_rec->key_length = flash_key_hdr_p->length;

        keystore_rec->crc32 = crc32_le(~0, (void *)&keystore_rec->key[0],
                flash_key_hdr_p->length);

        /* Add this to linked list. */
        sierra_ks_if_list_add(keystore_rec);
        kfree(keystore_rec);

        offset += sizeof(sec_ssmem_key_hdr_s) + flash_key_hdr_p->length;
    }

    ssmem_keys_release();

    /* Print all keys. */
    sierra_ks_if_print_all_keys();

    return ret;
}

/*******************************************************************************
* Get the the keys from the keystore. This is where we read them and store them
* localy. For now, we will fake it.
* This method could be called from interrupt context (timer function callback
* is one of them). If this is not needed, GFP_KERNEL flags should be used in
* kmalloc (it has less chance of failing).
*******************************************************************************/
int sierra_ks_if_simulate_keystore_keys(void)
{
#define KMALLOC_FLAGS   (GFP_ATOMIC)

    static key_store_rec_t *keystore_rec;
    int ret = 0;

    SIERRA_KS_IF_DEBUG(TRUE, "Scraping keystore keys...\n");

    /* Simulate OEM key. */
    keystore_rec = (key_store_rec_t *)kmalloc(sizeof(key_store_rec_t) +
                                              sizeof("OEM Key") + 1,
                                              KMALLOC_FLAGS);
    keystore_rec->version = 1;
    keystore_rec->next_rec = 4096;
    keystore_rec->key_type = OEM0;
    strcpy(keystore_rec->key, "OEM Key");
    keystore_rec->key_length = strlen(keystore_rec->key) + 1;

    keystore_rec->crc32 = crc32_le(~0, (void *)(&keystore_rec->version),
                                   sizeof(keystore_rec->version));
    keystore_rec->crc32 = crc32_le(keystore_rec->crc32,
                                   (void *)(&keystore_rec->next_rec),
                                   sizeof(keystore_rec->next_rec));
    keystore_rec->crc32 = crc32_le(keystore_rec->crc32,
                                   (void *)(&keystore_rec->key_type),
                                   sizeof(keystore_rec->key_type));
    keystore_rec->crc32 = crc32_le(keystore_rec->crc32,
                                   (void *)(&keystore_rec->key_length),
                                   sizeof(keystore_rec->key_length));
    keystore_rec->crc32 = crc32_le(keystore_rec->crc32,
                                   (void *)(&keystore_rec->key),
                                   keystore_rec->key_length);

    /* Add this to linked list. */
    sierra_ks_if_list_add(keystore_rec);
    kfree(keystore_rec);

    /* Simulate IMA0 key. */
    keystore_rec = (key_store_rec_t *)kmalloc(sizeof(key_store_rec_t) +
                                              sizeof("IMA Key") + 1,
                                              KMALLOC_FLAGS);
    keystore_rec->version = 1;
    keystore_rec->next_rec = 4096;
    keystore_rec->key_type = IMA0;
    strcpy(keystore_rec->key, "IMA Key");
    keystore_rec->key_length = strlen(keystore_rec->key) + 1;

    keystore_rec->crc32 = crc32_le(~0, (void *)(&keystore_rec->version),
                                   sizeof(keystore_rec->version));
    keystore_rec->crc32 = crc32_le(keystore_rec->crc32,
                                   (void *)(&keystore_rec->next_rec),
                                   sizeof(keystore_rec->next_rec));
    keystore_rec->crc32 = crc32_le(keystore_rec->crc32,
                                   (void *)(&keystore_rec->key_type),
                                   sizeof(keystore_rec->key_type));
    keystore_rec->crc32 = crc32_le(keystore_rec->crc32,
                                   (void *)(&keystore_rec->key_length),
                                   sizeof(keystore_rec->key_length));
    keystore_rec->crc32 = crc32_le(keystore_rec->crc32,
                                   (void *)(&keystore_rec->key),
                                   keystore_rec->key_length);

    /* Add this to linked list. */
    sierra_ks_if_list_add(keystore_rec);
    kfree(keystore_rec);

    /* Simulate RFS0 key. */
    keystore_rec = (key_store_rec_t *)kmalloc(sizeof(key_store_rec_t) +
                                              sizeof("RFS Key") + 1,
                                              KMALLOC_FLAGS);
    keystore_rec->version = 1;
    keystore_rec->next_rec = 4096;
    keystore_rec->key_type = RFS0;
    strcpy(keystore_rec->key, "RFS Key");
    keystore_rec->key_length = strlen(keystore_rec->key) + 1;

    keystore_rec->crc32 = crc32_le(~0, (void *)(&keystore_rec->version),
                                   sizeof(keystore_rec->version));
    keystore_rec->crc32 = crc32_le(keystore_rec->crc32,
                                   (void *)(&keystore_rec->next_rec),
                                   sizeof(keystore_rec->next_rec));
    keystore_rec->crc32 = crc32_le(keystore_rec->crc32,
                                   (void *)(&keystore_rec->key_type),
                                   sizeof(keystore_rec->key_type));
    keystore_rec->crc32 = crc32_le(keystore_rec->crc32,
                                   (void *)(&keystore_rec->key_length),
                                   sizeof(keystore_rec->key_length));
    keystore_rec->crc32 = crc32_le(keystore_rec->crc32,
                                   (void *)(&keystore_rec->key),
                                   keystore_rec->key_length);

    /* Add this to linked list. */
    sierra_ks_if_list_add(keystore_rec);
    kfree(keystore_rec);

    /* Simulate LGT0 key. */
    keystore_rec = (key_store_rec_t *)kmalloc(sizeof(key_store_rec_t) +
                                              sizeof("LGT Key") + 1,
                                              KMALLOC_FLAGS);
    keystore_rec->version = 1;
    keystore_rec->next_rec = 4096;
    keystore_rec->key_type = LGT0;
    strcpy(keystore_rec->key, "LGT Key");
    keystore_rec->key_length = strlen(keystore_rec->key) + 1;

    keystore_rec->crc32 = crc32_le(~0, (void *)(&keystore_rec->version),
                                   sizeof(keystore_rec->version));
    keystore_rec->crc32 = crc32_le(keystore_rec->crc32,
                                   (void *)(&keystore_rec->next_rec),
                                  sizeof(keystore_rec->next_rec));
    keystore_rec->crc32 = crc32_le(keystore_rec->crc32,
                                   (void *)(&keystore_rec->key_type),
                                   sizeof(keystore_rec->key_type));
    keystore_rec->crc32 = crc32_le(keystore_rec->crc32,
                                   (void *)(&keystore_rec->key_length),
                                   sizeof(keystore_rec->key_length));
    keystore_rec->crc32 = crc32_le(keystore_rec->crc32,
                                   (void *)(&keystore_rec->key),
                                   keystore_rec->key_length);

    /* Add this to linked list. */
    sierra_ks_if_list_add(keystore_rec);
    kfree(keystore_rec);

    /* Print all keys. */
    sierra_ks_if_print_all_keys();

    return ret;
}

/*******************************************************************************
 * Informational print of all link list members.
 ******************************************************************************/
void sierra_ks_if_print_all_keys(void)
{
    struct list_head *lh_p = NULL;
    sierra_ks_if_data_info *di_p = NULL;

    read_lock(&sierra_ks_if_list_lock);
    list_for_each(lh_p, &sierra_ks_if_head_list)
    {
        di_p = list_entry(lh_p, sierra_ks_if_data_info, list);
        pr_warn("Key type = %d, Key length = %d, key = %s\n",
                di_p->key_type, di_p->key_length, di_p->key);
    }
    read_unlock(&sierra_ks_if_list_lock);
}

/*******************************************************************************
 * Will return key requested by userland. The key will be stored in the passed
 * structure. We are here because msg_type = SIERRA_KS_IF_DRV_MSG_TYPE_KEY_REQ.
 *
 * If successful, this function will have message buffer populated and ready
 * to be copied to userland. If error occurred, only msg_type will be set, any
 * other field may or may not be.
 *
 * On success, this function returns 0, negative number otherwise.
 ******************************************************************************/
int sierra_ks_if_get_key(sierra_ks_if_drv_msg *drv_msg)
{
    struct list_head *lh_p = NULL;
    sierra_ks_if_data_info *di_p = NULL;
    keytype_t key_type;
    int ret = 0;
    const int ret_err = -1;

    if(drv_msg->msg_type != SIERRA_KS_IF_DRV_MSG_TYPE_KEY_REQ ) {
        pr_err("Cannot handle non-key related request.\n");
        return ret_err;
    }

    /* Set the message type to response. */
    drv_msg->msg_type = SIERRA_KS_IF_DRV_MSG_TYPE_KEY_RSP;

    /* Message subtype may not be available, and if not it will not be changed. */
    switch(drv_msg->msg_subtype)
    {
        case SIERRA_KS_IF_DRV_MSG_TYPE_DM_VERITY_KEY_RFS_REQ:
        {
            SIERRA_KS_IF_DEBUG(TRUE,"Rootfs DM Verity key request.\n");
            key_type = RFS0;
            drv_msg->msg_subtype = SIERRA_KS_IF_DRV_MSG_TYPE_DM_VERITY_KEY_RFS_RSP;
        }
        break;

        case SIERRA_KS_IF_DRV_MSG_TYPE_DM_VERITY_KEY_LGT_REQ:
        {
            SIERRA_KS_IF_DEBUG(TRUE,"Legato DM Verity key request.\n");
            key_type = LGT0;
            drv_msg->msg_subtype = SIERRA_KS_IF_DRV_MSG_TYPE_DM_VERITY_KEY_LGT_RSP;
        }
        break;

        default:
        {
            pr_err("Unknown key request %d\n", drv_msg->msg_subtype);
            ret = ret_err;
        }
        break;
    }

    /* Grab the key from linked list. */
    if(ret != ret_err) {

        /* We need this to break out of the linked list search. */
        int stop = false;

        read_lock(&sierra_ks_if_list_lock);

        list_for_each(lh_p, &sierra_ks_if_head_list)
        {
            if(stop == false) {
                di_p = list_entry(lh_p, sierra_ks_if_data_info, list);
                if(di_p->key_type == key_type) {
                    pr_notice("%d key found in the list.\n", di_p->key_type);
                    if(di_p->key_length > sizeof(drv_msg->payload) ) {
                        pr_err("Cannot store key %d into userland buffer.\n",
                               key_type);
                        ret = ret_err;
                        stop = true;
                    }
                    else {
                        SIERRA_KS_IF_DEBUG(TRUE, "Setting up key response message.\n");
                        drv_msg->seq++;
                        drv_msg->pl_msg_sz = di_p->key_length;
                        memcpy(drv_msg->payload, di_p->key, di_p->key_length);
                        stop = true;
                    }
                }
            }
            else {
                SIERRA_KS_IF_DEBUG(TRUE, "Breaking out of linked list loop.\n");
                break;
            }
        }

        read_unlock(&sierra_ks_if_list_lock);
    }

    return ret;
}

/*******************************************************************************
 * Init key data store.
 ******************************************************************************/
void sierra_ks_if_keydata_init(void)
{

    /* Initialize keystore liked list. */
    INIT_LIST_HEAD(&sierra_ks_if_head_list);
}

/*******************************************************************************
 * Add list member. It will return 0 if everything is OK, negative number
 * otherwise (most likely -1).
 * This methoud could be called from interrupt context. If not called from
 * interrupt context, use GFP_KERNEL, because it has less chance of failing.
 ******************************************************************************/
int sierra_ks_if_list_add(key_store_rec_t *kstore_rec)
{
#define KMALLOC_FLAGS   (GFP_ATOMIC)

    int ret = 0;
    int ret_err = -1;
    sierra_ks_if_data_info *p = NULL;

    if(kstore_rec == NULL) {
        ret = ret_err;
    }
    else {
        p = (sierra_ks_if_data_info *)kmalloc(sizeof(sierra_ks_if_data_info),
                                              KMALLOC_FLAGS);
        if (p == NULL) {
            pr_err("Cannot allocate space for key ID 0x%x structure.\n",
                   kstore_rec->key_type);
            ret = ret_err;
        }
        else {
            /* Allocate space for a key. */
            p->key = (uint8_t *)kmalloc(kstore_rec->key_length, GFP_ATOMIC);
            if(p->key == NULL) {
                pr_err("Cannot allocate space for key ID 0x%x\n",
                       kstore_rec->key_type);
                ret = ret_err;
            }
            else {
                p->key_type = kstore_rec->key_type;
                p->key_length = kstore_rec->key_length;
                /* Now, copy key... */
                memcpy(p->key, kstore_rec->key, p->key_length);
                /* ... and add key structure to local store. */
                write_lock(&sierra_ks_if_list_lock);
                list_add_tail(&(p->list), &sierra_ks_if_head_list);
                write_unlock(&sierra_ks_if_list_lock);
            }
        }
    }

    return ret;
}

/*******************************************************************************
 * Delete all list members and free their related allocated memory.
 ******************************************************************************/
void sierra_ks_if_list_del_all(void)
{
    struct list_head *lh_p, *lh_q = NULL;
    sierra_ks_if_data_info *di_p = NULL;

    if(list_empty(&sierra_ks_if_head_list)) {
        /* Nothing to do here. */
        return;
    }

    write_lock(&sierra_ks_if_list_lock);

    /* Go through the list, delete all members and free all memory. */
    list_for_each_safe(lh_p, lh_q, &sierra_ks_if_head_list)
    {
        /* Grab the member. */
        di_p = list_entry(lh_p, sierra_ks_if_data_info, list);

        SIERRA_KS_IF_DEBUG(TRUE, "Deleting entry containing key %d\n",
                           di_p->key_type);

        /* delete it from the list. */
        list_del(lh_p);

        /* Free all associated memory. */
        kfree((void *)(di_p->key));
        kfree((void *)di_p);
    }

    write_unlock(&sierra_ks_if_list_lock);
}

/*******************************************************************************
* Userland interface, read method.
*
* The message passing is actually bidirectional. Once userland wants to read
* something from the kernel, it will supply the buffer and the data length.
* Buffer must contain the message sent from userland, and length is total message
* length kernel must read. Once validated, kernel will try to fulfill userland
* request, by setting the appropriate fields and copying the data into the
* supplied buffer. Buffer must be large enough to receive the data from kernel.
*
* We need to use mutex here, because we have one static buffer, and we may
* potentially have multiple callers.
*******************************************************************************/
ssize_t sierra_ks_if_dev_read(struct file *file, char *data, size_t len,
                              loff_t *ppos)
{
    ssize_t ret = 0;
    static sierra_ks_if_drv_msg drv_msg;

    SIERRA_KS_IF_DEBUG(TRUE, "len = %d\n", len);

    /* Check if keystore data is available. If not, nothing we could do
     * about it.
     */
    if(list_empty(&sierra_ks_if_head_list)) {
        /* Nothing to do here. */
        pr_warning("Keystore data not available.\n");
        return -EAGAIN;
    }

    /* This may be rare, but it could happen... */
    if(len == 0) {
        pr_err("Userland data is zero length.\n");
        return -EFAULT;
    }

    /* Check if our data buffer is big enough. */
    if(len > sizeof(drv_msg)) {
        pr_err("Cannot fit user data: user data len = %d, buffer size = %d\n",
               len, sizeof(drv_msg));
        return -EFAULT;
    }

    /* It is worth checking if userland data could be accessed. This way we
     *  could handle things gracefully.
     */
    if (!access_ok(VERIFY_READ, data, len)) {
        pr_err("Cannot read userland data.\n");
        return -EFAULT;
    }


    /* We need to lock this, because operation will be done on the common,
     * static buffer. It is easier to lock the whole operation even though
     * there are places where lock may not be required. Otherwise, there will
     * be lots of locking and unlocking. In addition, there should not be
     * long operations here, and it should not last more than one kernel
     * tick.
     *
     * DM, FIXME: We have one core running Linux, so spinlock does not make
     * sense, but we may need to do it on some other product if this is needed.
     */
    if(mutex_lock_interruptible(&data_read_lock) != 0) {
        pr_warn("Driver operation interrupted.\n");
        return -EINTR;
    }

    /* If some data could not be copied, we will just return, and tell user
     * that something has gone wrong.
     */
    if(copy_from_user((void *)(&drv_msg), data, len)) {
        pr_err("Could not copy all data bytes from userland.\n");
        ret = -EFAULT;
    }

    /* Go in only if there are no previous faults. */
    if(ret == 0) {

        switch(drv_msg.msg_type) {

            case SIERRA_KS_IF_DRV_MSG_TYPE_TEST1_REQ:
            {
                char msg[] = "Message from driver to userland.";

                SIERRA_KS_IF_DEBUG(TRUE,
                                   "SIERRA_KS_IF_DRV_MSG_TYPE_TEST1_REQ: Message from userland is [%s]\n",
                                   drv_msg.payload);

                drv_msg.msg_type = SIERRA_KS_IF_DRV_MSG_TYPE_TEST1_RSP;
                strcpy(drv_msg.payload, msg);
                drv_msg.pl_msg_sz = sizeof(msg) + 1;
                ret = sizeof(drv_msg.msg_type) + sizeof(drv_msg.msg_subtype) +
                     sizeof(drv_msg.seq) + sizeof(drv_msg.pl_msg_sz) + drv_msg.pl_msg_sz;
                if (access_ok(VERIFY_WRITE, data, ret)) {
                    if(copy_to_user(data, (void *)&drv_msg, ret)) {
                        pr_err("SIERRA_KS_IF_DRV_MSG_TYPE_TEST1_REQ: Could not copy all data to userland.\n");
                        ret = -EFAULT;
                    }
                }
                else {
                    pr_err("SIERRA_KS_IF_DRV_MSG_TYPE_TEST1_RSP: Cannot write userland data.\n");
                    ret = -EFAULT;
                }
            }
            break;

            case SIERRA_KS_IF_DRV_MSG_TYPE_KEY_REQ:
            {
                /* drv_msg structure must stay intact if there is an error in
                   order to handle that error. */
                if(sierra_ks_if_get_key(&drv_msg)) {
                    pr_err("SIERRA_KS_IF_DRV_MSG_TYPE_KEY_REQ: Requested key %d is not available.\n",
                           drv_msg.msg_subtype);
                    ret = -ENOENT;
                }
                else {
                    ret = sizeof(drv_msg.msg_type) + sizeof(drv_msg.msg_subtype) +
                     sizeof(drv_msg.seq) + sizeof(drv_msg.pl_msg_sz) + drv_msg.pl_msg_sz;
                    if (access_ok(VERIFY_WRITE, data, ret)) {
                        if(copy_to_user(data, (void *)&drv_msg, ret)) {
                            pr_err("SIERRA_KS_IF_DRV_MSG_TYPE_KEY_REQ: Could not copy all data to userland.\n");
                            ret = -EFAULT;
                        }
                    }
                    else {
                        pr_err("SIERRA_KS_IF_DRV_MSG_TYPE_KEY_REQ: Cannot write userland data.\n");
                        ret = -EFAULT;
                    }
                }
            }
            break;

            default:
            {
                pr_err("Invalid message.\n");
                ret = -EPERM;
            }
            break;
        }
    }

    /* Give back others right to read. */
    mutex_unlock(&data_read_lock);

    return ret;
}

/*******************************************************************************
* Userland interface, write method.
*******************************************************************************/
ssize_t sierra_ks_if_dev_write(struct file *file, const char *data,
                               size_t len, loff_t * ppos)
{
    pr_err("Device write access is not allowed.\n");
    return -EPERM;
}

/*******************************************************************************
* Userland interface, ioctl method. This should never be used.
*******************************************************************************/
long sierra_ks_if_dev_ioctl(struct file *file, unsigned int cmd,
                                  unsigned long arg)
{
    pr_err("ioctl interface is not implemented, and should not be used.\n");
    return -EPERM;
}

/*******************************************************************************
* Userland interface, mmap method.
*******************************************************************************/
int sierra_ks_if_dev_mmap(struct file *filp, struct vm_area_struct *vma)
{
    pr_err("mmap access is not allowed.\n");
    return -EPERM;
}

/*******************************************************************************
* Userland interface, open method.
*******************************************************************************/
int sierra_ks_if_dev_open(struct inode *inode, struct file *file)
{
    return 0;
}

/*******************************************************************************
* Userland interface, release method.
*******************************************************************************/
int sierra_ks_if_dev_release(struct inode *inode, struct file *file)
{
    return 0;
}

/*******************************************************************************
* /proc 'debug' entries.
*******************************************************************************/

/*******************************************************************************
* debug /proc entry open.
*******************************************************************************/
int sierra_ks_if_debug_proc_open(struct inode *inode, struct file *file)
{
    sierra_ks_if_proc_entry.debug_file_read_done = FALSE;
    return 0;
}

/*******************************************************************************
* debug /proc entry read. Currently, code handles singe entry only, which is
* more than sufficient at this point of time.
*******************************************************************************/
ssize_t sierra_ks_if_debug_proc_read(struct file *file, char *data, size_t len,
                                     loff_t *ppos)
{
    unsigned long ret = 0;
    /* Storage includes NULL character. */
    char buf[2];

    SIERRA_KS_IF_DEBUG(TRUE, "File is '%s', len = 0x%x\n",
                       file->f_path.dentry->d_iname, len);

    if(TRUE == sierra_ks_if_proc_entry.debug_file_read_done) {
        SIERRA_KS_IF_DEBUG(TRUE, "Called again, but reading already done.\n");
        ret = 0;
    }
    else {
        /* sprintf does not include termination NULL. */
        ret = sprintf(buf, "%d", sierra_ks_if_debug_get());
        len = copy_to_user(data, buf, ret);
        SIERRA_KS_IF_DEBUG(TRUE, "ret=0x%lx, left to copy=0x%x\n", ret, len);
        ret -= len; /* ret is always >= len */

        /* DM, FIXME: Tell the system that we are done, otherwise we will end up
           in endless loop. The proper way of doing this is to check if len != 0,
           and if not, to allow copying of the rest of the characters to
           userland in next iteration. However, we are copying just one character
           and chances are that we will never have the problem. Hence, no
           checking, and setting blindly that we are done. */
        sierra_ks_if_proc_entry.debug_file_read_done = TRUE;
    }

    SIERRA_KS_IF_DEBUG(TRUE, "ret=0x%lx\n", ret);
    return ret;
}

/*******************************************************************************
* debug /proc entry write.
*******************************************************************************/
static ssize_t sierra_ks_if_debug_proc_write(struct file *file,
                                             const char __user *buffer,
                                             size_t size, loff_t *off)
{
    ssize_t ret = 0;
    /* Storage includes NULL character. */
    char buf[3];

    SIERRA_KS_IF_DEBUG(TRUE, "File is '%s'\n", file->f_path.dentry->d_iname);

    /* We should be getting buffer in the following format:
           X\n
     */
    if(size > (sizeof(buf)-1)) {
        pr_err("Data cannot fit: data size=%d, buffer size=%d\n",
               size, (sizeof(buf)-1));
        ret = (-EINVAL);
    }

    if(ret == 0) {
        if(copy_from_user(buf, buffer, size) != 0) {
            pr_err("Could not copy all user data\n");
            ret = (-EINVAL);
        }
        else {
            ret=size;
            buf[size] = '\0';
            if(buf[0] == '1') {
                sierra_ks_if_debug_enable();
            }
            else if(buf[0] == '0') {
                sierra_ks_if_debug_disable();
            }
            else {
                pr_err("Invalid debug option [%d].\n", buf[0]);
                ret = (-EINVAL);
            }
        }
    }

    SIERRA_KS_IF_DEBUG(TRUE, "ret=%d\n", ret);
    return ret;
}

/*******************************************************************************
* debug /proc entry close.
*******************************************************************************/
int sierra_ks_if_debug_proc_release(struct inode *inode, struct file *file)
{
    return 0;
}

/*******************************************************************************
* /proc 'stats' entries.
*******************************************************************************/

/*******************************************************************************
* stats /proc entry open.
*******************************************************************************/
int sierra_ks_if_stats_proc_open(struct inode *inode, struct file *file)
{
    sierra_ks_if_proc_entry.stats_file_read_done = FALSE;
    return 0;
}

/*******************************************************************************
* stats /proc entry read.
*******************************************************************************/
ssize_t sierra_ks_if_stats_proc_read(struct file *file, char *data, size_t len,
                                    loff_t *ppos)
{
    sierra_ks_if_proc_entry.stats_file_read_done = TRUE;
    return 0;
}

/*******************************************************************************
* stats /proc entry close.
*******************************************************************************/
int sierra_ks_if_stats_proc_release(struct inode *inode, struct file *file)
{
    return 0;
}

/*******************************************************************************
* Create proc entry for this driver.
*******************************************************************************/
int sierra_ks_if_create_proc_entry(void)
{
    char *dir_name = sierra_ks_if_proc_entry.root_dir_name;
    char *debug_file_name = sierra_ks_if_proc_entry.debug_file_name;
    char *stats_name = sierra_ks_if_proc_entry.stats_file_name;
    unsigned char ret = FALSE;

    SIERRA_KS_IF_DEBUG(TRUE, "Creating '%s' driver proc entries.\n", SIERRA_KS_IF_DRV_NAME);

    /* Create /proc root directory for this driver. */
    SIERRA_KS_IF_DEBUG(TRUE, "Creating '%s' driver '%s' proc directory.\n",
                       SIERRA_KS_IF_DRV_NAME, dir_name);
    sierra_ks_if_proc_entry.root_dir = proc_mkdir(dir_name, NULL);
    if(sierra_ks_if_proc_entry.root_dir == NULL) {
        pr_err("Can't create driver proc entry '%s'.\n", dir_name);
        ret = TRUE;
    }

    if(ret == FALSE) {
        /* Create debug entry in driver root directory. */
        SIERRA_KS_IF_DEBUG(TRUE, "Creating '%s' driver '%s' proc entry.\n",
                           SIERRA_KS_IF_DRV_NAME, debug_file_name);
        sierra_ks_if_proc_entry.debug_file = proc_create(debug_file_name,0644,
                                                         sierra_ks_if_proc_entry.root_dir,
                                                         &sierra_ks_if_debug_proc_fops);
        if(sierra_ks_if_proc_entry.debug_file == NULL) {
            pr_err("Can't create driver proc entry '%s'.\n", debug_file_name);
            ret = TRUE;
        }
    }

    if(ret == FALSE) {
        /* Create stats entry in driver root directory. */
        SIERRA_KS_IF_DEBUG(TRUE, "Creating '%s' driver '%s' proc entry.\n",
                           SIERRA_KS_IF_DRV_NAME,stats_name);
        sierra_ks_if_proc_entry.stats=proc_create(stats_name,0444,
                                                  sierra_ks_if_proc_entry.root_dir,
                                                  &sierra_ks_if_stats_proc_fops);
        if(sierra_ks_if_proc_entry.stats == NULL) {
            pr_err("Can't create driver proc entry '%s'.\n", stats_name);
            ret = TRUE;
        }
    }

    return ret;
}

/*******************************************************************************
* Remove proc entry for this driver.
*******************************************************************************/
void sierra_ks_if_remove_proc_entry(void)
{

    SIERRA_KS_IF_DEBUG(TRUE, "Removing '%s' driver proc entries.\n", SIERRA_KS_IF_DRV_NAME);

    if(sierra_ks_if_proc_entry.debug_file != NULL) {
        SIERRA_KS_IF_DEBUG(TRUE, "Removing '%s' proc entry.\n",
                           sierra_ks_if_proc_entry.debug_file_name);
        remove_proc_entry(sierra_ks_if_proc_entry.debug_file_name,
                          sierra_ks_if_proc_entry.root_dir);
    }

    if(sierra_ks_if_proc_entry.stats != NULL) {
        SIERRA_KS_IF_DEBUG(TRUE, "Removing '%s' proc entry.\n",
                           sierra_ks_if_proc_entry.stats_file_name);
        remove_proc_entry(sierra_ks_if_proc_entry.stats_file_name,
                          sierra_ks_if_proc_entry.root_dir);
    }

    if(sierra_ks_if_proc_entry.root_dir != NULL) {
        SIERRA_KS_IF_DEBUG(TRUE, "Removing '%s' proc entry.\n",
                           sierra_ks_if_proc_entry.root_dir_name);
        remove_proc_entry(sierra_ks_if_proc_entry.root_dir_name, NULL);
    }

    SIERRA_KS_IF_DEBUG(TRUE, "Removal of '%s' driver proc entries done.\n", SIERRA_KS_IF_DRV_NAME);
}

/*******************************************************************************
* Enable, disable and get SIERRA_KS_IF_DEBUG.
*******************************************************************************/
void sierra_ks_if_debug_enable(void) { sierra_ks_if_debug_flg=1; }
void sierra_ks_if_debug_disable(void) { sierra_ks_if_debug_flg=0; }
unsigned char sierra_ks_if_debug_get(void) { return sierra_ks_if_debug_flg; }

/* Userland device interface. */
static struct miscdevice sierra_ks_if_miscdev = {
    .minor = SIERRA_KS_IF_MINOR,
    .name = SIERRA_KS_IF_DEV_NAME,
    .nodename = SIERRA_KS_IF_DEV_NAME,
    .fops = &sierra_ks_if_dev_fops,
};

/*******************************************************************************
* Driver entry
*******************************************************************************/
static int __init sierra_ks_if_init(void)
{
    int ret = 0;

    pr_notice("Starting %s, version %s ...\n", DRV_DESCRIPTION, DRV_VERSION);

    ret = misc_register(&sierra_ks_if_miscdev);
    if (ret) {
        pr_err("Can't register misc device %d\n", SIERRA_KS_IF_MINOR);
    }

    ret = sierra_ks_if_create_proc_entry();
    if(ret) {
        pr_err("Can't create proc entries.\n");
    }

    /* Initialize keystore store. */
    sierra_ks_if_keydata_init();

    ret = sierra_ks_if_set_keystore_read_timer();
    if(ret) {
        pr_err("Cannot initialize keystore read timer.\n");
    }

    pr_notice("%s module init done.\n", SIERRA_KS_IF_DRV_NAME);

    return ret;
}

/*******************************************************************************
* Loaded module exit point.
*******************************************************************************/
static void sierra_ks_if_cleanup(void)
{
    pr_notice("Starting %s cleanup...\n", SIERRA_KS_IF_DRV_NAME);

    sierra_ks_if_remove_proc_entry();

    misc_deregister(&sierra_ks_if_miscdev);

    sierra_ks_if_clear_keystore_read_timer();

    sierra_ks_if_list_del_all();

    pr_notice("Goodbye!\n");
}


module_init(sierra_ks_if_init);
module_exit(sierra_ks_if_cleanup);
MODULE_DESCRIPTION(DRV_DESCRIPTION);
MODULE_AUTHOR(DRV_AUTHOR);
MODULE_LICENSE("GPL");
MODULE_ALIAS_MISCDEV(KEYSTOR_IF_MINOR);
MODULE_ALIAS("devname:"SIERRA_KS_IF_DEV_NAME);

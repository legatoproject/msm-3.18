/************
 *
 * $Id$
 *
 * Filename:  sierra_ududefs - user definitions USB gadget driver
 *
 * Copyright: � 2012 Sierra Wireless, Inc.
 *            All rights reserved
 *
 ************/
#ifndef SIERRA_UDUDEFS_H
#define SIERRA_UDUDEFS_H

#define UD_PID_68A2 0x68A2
#define UD_PID_68B1 0x68B1

/* Vendor specific setup request (bRequest) */
#define UD_SWI_SETUP_REQ_SET_DEVICE_POWER_STATE   0x00
#define UD_SWI_SETUP_REQ_SET_MODE_NON_MUX         0x01
#define UD_SWI_SETUP_REQ_SET_MODE_MUX             0x02
#define UD_SWI_SETUP_REQ_GET_MODE_MUX             0x03
#define UD_SWI_SETUP_REQ_GET_NDIS_SUPPORT         0x04
#define UD_SWI_SETUP_REQ_GET_NDIS_PREF            0x05
#define UD_SWI_SETUP_REQ_GET_ATTRIBUTE            0x06
#define UD_SWI_SETUP_REQ_SET_MODE_NMEA            0x07
#define UD_SWI_SETUP_REQ_GET_MODE_NMEA            0x08
#define UD_SWI_SETUP_REQ_SET_HOST_POWER_STATE     0x09
#define UD_SWI_SETUP_REQ_GET_DEV_SWOC_INFO        0x0A
#define UD_SWI_SETUP_REQ_SET_DEV_SWOC_MODE        0x0B
#define UD_SWI_SETUP_REQ_GET_CONFIG_ITEM          0x0C
#define UD_SWI_SETUP_REQ_SET_CONFIG_ITEM          0x0D
#define UD_SWI_SETUP_REQ_SET_DEVICE_RESET         0x0E
#define UD_SWI_SETUP_REQ_GET_OS_FEATURE_REQUEST   0x20
#define UD_SWI_SETUP_REQ_NULL                     0xFF

/* Sierra USB Interface Information */
struct ud_usb_interface {
	unsigned number;
	char * name;
};

/*
  Define Interface assignments for each possible interface
  Interfaces may be exposed or not based on entry in "Start_USB" shell script
  Supports interface types that require more than one interface #
  Ordering is important for those interface types requiring more than one #
  Interface #'s can be listed multiple times, however, They will be assigned in
  first-come-first-served fashion.
  NOTE: To use interface #'s larger than 15 increase define MAX_CONFIG_INTERFACES
*/
static const struct ud_usb_interface ud_interface_68A2[] = {
	/* Interface #      Name   */
	{	0,		"diag"				},
	{	1,		"adb"				},
	{	1,		"Function FS Gadget"		},
	{	2,		"nmea"				},
	{	3,		"modem"				},
	{	4,		"at" 				},
	{	5,		"raw_data"			},
	{	6,		"osa"				},
	{	8,		"rmnet0"			},
	{	9,		"mass_storage"			},
	{	10,		"rmnet1"			},
	{	11,		"rmnet2"			},
	{	12,		"usb_mbim"			},
	{	13,		"usb_mbim"			},
	{	14,		"rndis"				},
	{	15,		"rndis"				},
	{	16,		"g_audio"			},
	{	17,		"g_audio"			},
	{	18,		"g_audio"			},
	{	19,		"cdc_ethernet"			},
	{	20,		"cdc_ethernet"			},
	{	22,		"cdc_network"			},	/* NCM */
	{	23,		"cdc_network"			},	/* NCM */
	{	24,		"cdc_eem"			},	/* EEM */

};

static const struct ud_usb_interface ud_interface_68B1[] = {
	/* Interface #      Name   */
	{	0,		"usb_mbim"		},
	{	1,		"usb_mbim"		},
	{	2,		"diag"			},
	{	3,		"modem"			},
	{	4,		"nmea"			},
	{	5,		"mass_storage"		},
	{	6,		"adb"			},
	{	8,		"rmnet0"		},
	{	10,		"rmnet1"		},
	{	11,		"rmnet2"		},
	{	14,		"rndis"			},
	{	15,		"rndis"			},
};

#define UD_MAX_INTERFACE_68A2 ARRAY_SIZE(ud_interface_68A2)
#define UD_MAX_INTERFACE_68B1 ARRAY_SIZE(ud_interface_68B1)

#define UD_INVALID_INTERFACE 255

static bool interface_reserved[MAX_CONFIG_INTERFACES];

static inline unsigned ud_get_interface_number( const char *interface_name, struct usb_configuration *config )
{
	unsigned interface_number = UD_INVALID_INTERFACE;
	unsigned i;
	unsigned max = UD_MAX_INTERFACE_68A2;
	const struct ud_usb_interface * ud_interface = &ud_interface_68A2[0];

	if(config->cdev->desc.idProduct == UD_PID_68B1){
		ud_interface = &ud_interface_68B1[0];
		max = UD_MAX_INTERFACE_68B1;
	}

	for (i = 0 ; i < max ; i++ ){
		interface_reserved[ud_interface->number] = true;
		if ( (strncmp(interface_name, ud_interface->name, strlen(ud_interface->name)) == 0) &&
			(config->interface[ud_interface->number] == NULL) ){
			/* Strings match */
			interface_number = ud_interface->number;
			break;
		}
		ud_interface++;
	}

	if( interface_number == UD_INVALID_INTERFACE ){
		/* Find next available */
		for(i=0 ; i<MAX_CONFIG_INTERFACES ; i++){
			if( (interface_reserved[i] == false) &&
				(config->interface[i] == NULL) ){
				interface_number = i;
				break;
			}
		}
		pr_info("No Match for Function Name: %s, Int #%d\n", interface_name, interface_number);
	}
	else
	{
		pr_info("Match for Function Name: %s, Int #%d\n", interface_name, interface_number);
	}

	return (interface_number);
}

/*Data types for TRU-Install*/
typedef enum{
	CDROM_TO_ECM,
	ECM_TO_CDROM,
	HID_TO_CDROM,
	HID_TO_ECM,
	ECM_TO_HID,
}ud_usb_dev_switch;

typedef struct {
	unsigned char MsgType;
} ud_msg_notify;

#endif

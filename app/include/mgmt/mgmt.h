#ifndef __USER_DEVICE_H__
#define __USER_DEVICE_H__

#include "esp_common.h"
/* NOTICE---this is for 1024KB spi flash.
 * you can change to other sector if you use other size spi flash. */
#define ESP_PARAM_START_SEC		0x7C

#define ESP_PARAM_SAVE_0    0
#define ESP_PARAM_SAVE_1    1
#define ESP_PARAM_FLAG      2
#define ESP_PARAM_FACTORY   3

struct hnt_factory_param {
    uint32           device_type; 
    char            *device_id; 
    char            *password; 
    char            *xmpp_server; 
    char            *xmpp_jid; 
    char            *xmpp_password;    
    uint8           wlan_mac[6];
};

struct hnt_mgmt_sec_flag_param {
    uint8 flag; 
    uint8 pad[3];
};

void 
hnt_mgmt_system_factory_reset(void);

#define DEVICE_RESET_MAX_COUNT 10
#define DEVICE_RECONNECT_MAX_COUNT 5

#endif

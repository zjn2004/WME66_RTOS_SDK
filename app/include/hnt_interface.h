/*
 * Copyright (c) 2014 Seaing, Ltd.
 * All Rights Reserved. 
 * Seaing Confidential and Proprietary.
 */


#ifndef __HNT_INTERFACE_H__
#define __HNT_INTERFACE_H__
#include "esp_common.h"
#include "user_config.h"
typedef sint8 err_t;


#define DEVICEINFO_STRING_LEN         64
struct hnt_mgmt_factory_param {
    uint8            device_type[DEVICEINFO_STRING_LEN]; 
    uint8            device_id[DEVICEINFO_STRING_LEN]; 
    uint8            password[DEVICEINFO_STRING_LEN]; 
    uint8            xmpp_server[DEVICEINFO_STRING_LEN]; 
    uint8            xmpp_jid[DEVICEINFO_STRING_LEN]; 
    uint8            xmpp_password[DEVICEINFO_STRING_LEN];
    uint8            wlan_mac[6];
    uint8            pad[2];
    uint8            pto_info[4];
};

#define HNT_WELCOME_INFO_NUM  5
typedef struct welcome_info_s {
    uint8_t welcome_flag;
    uint8_t pad;
    char    client_mac_addr[6];
    uint32_t client_ip_addr;
}welcome_info_t;

#define HNT_TIMER_FIX_SUPPORT       0
#define HNT_TIMER_LOOP_SUPPORT      0
#define HNT_TIMER_WEEK_SUPPORT      1
#define MAX_TIMER_COUNT             10
#define DELAY_GROUP_INDEX  0
#define NIGHTMODE_GROUP_INDEX  9

#ifndef PROFILE_WALLSWITCH
#define PROFILE_WALLSWITCH 0
#endif
#ifndef PROFILE_NIGHTLIGHT
#define PROFILE_NIGHTLIGHT 0
#endif
#ifndef PROFILE_FLOORHEAT
#define PROFILE_FLOORHEAT 0
#endif

typedef struct hnt_timer_group_param_s {
    int enable;
#if HNT_TIMER_FIX_SUPPORT
    int on_fix_wait_time_second;
#endif
#if HNT_TIMER_LOOP_SUPPORT
    int on_loop_wait_time_second;
#endif
#if HNT_TIMER_WEEK_SUPPORT
    int on_week_wait_time_second;
#endif
#if HNT_TIMER_FIX_SUPPORT
    int off_fix_wait_time_second;    
#endif
#if HNT_TIMER_LOOP_SUPPORT
    int off_loop_wait_time_second;
#endif
#if HNT_TIMER_WEEK_SUPPORT
    int off_week_wait_time_second;
#endif
    int weekday;
}hnt_timer_group_param_t;

struct hnt_mgmt_saved_param {
    uint8 wifi_enable;    
    uint8 switch_status;
    uint8 pad[2];
    welcome_info_t welcome_info[HNT_WELCOME_INFO_NUM]; 
    hnt_timer_group_param_t group[MAX_TIMER_COUNT];
    int saveTimestamp;
#if PROFILE_WALLSWITCH
    int saveTimestamp2;
    int saveTimestamp3;
    hnt_timer_group_param_t group2[MAX_TIMER_COUNT];
    hnt_timer_group_param_t group3[MAX_TIMER_COUNT];
#endif
#if PROFILE_NIGHTLIGHT
    int duty;
#endif
#if PROFILE_FLOORHEAT
    uint8 cityid[20];
    uint8 weekday;
#endif
};

void 
hnt_mgmt_load_param(struct hnt_mgmt_saved_param *param);
void 
hnt_mgmt_save_param(struct hnt_mgmt_saved_param *param);

int 
hnt_mgmt_factory_init(struct hnt_mgmt_factory_param *factory_param);

int 
hnt_mgmt_get_factory_info(struct hnt_mgmt_factory_param *factory_param);

void
hnt_mgmt_system_factory_reset(void);

void 
hnt_mgmt_anti_copy_enable(uint8_t enable);

void 
hnt_onbd_start(void );

#define    HNT_XMPP_PATH_FLAG_NORMAL    (0)
#define    HNT_XMPP_PATH_FLAG_HISTORY   (1<<0)
#define    HNT_XMPP_PATH_FLAG_ALARM     (1<<1)
#define    HNT_XMPP_PATH_FLAG_UPDATE    (1<<2)
#define    HNT_XMPP_PATH_FLAG_REALDATA    (1<<3)

typedef struct {
    char *Description;
    int LimitMax;
    int LimitMin;
}paramAlarmDesc_t;

typedef struct deviceParameter_s {
    uint32 flag;
    char * paramName;
    int (*getParameterFunc)(char *, char *, int);    
    int (*setParameterFunc)(char *, char *);
    paramAlarmDesc_t *alarmDesc;
}deviceParameter_t;

void 
hnt_xmpp_param_array_regist(deviceParameter_t *custom_xmpp_param_array, uint32 array_size);

typedef struct customInfo_s {
    char *deviceInfoSv;
    char *deviceInfoModelName;
    char *deviceInfoManufacturer;
    uint32 inform_interval;
    uint32 sampling_interval;
}customInfo_t;
void 
hnt_xmpp_custom_info_regist(customInfo_t *customInfo);

void 
hnt_xmpp_notif_update_data(char *path_fullname, char *para_value);

void 
hnt_xmpp_notif_config_changed(char *path_fullname, char *para_value);

void 
hnt_xmpp_notif_device_info(void);

void 
at_cmd_event(uint8_t *buffer);


typedef void (*hnt_welcome_action_welcome_func)(void);
typedef void (*hnt_welcome_action_leave_func)(void);
void 
welcome_action_regist(void *welcome_action_cb, void*leave_action_cb);

void 
welcome_init(void);

void 
welcome_path_handle(char *path_string);

void 
welcome_path_get(char *path_string, int size); 

#define TIMER_MODE_ON          1
#define TIMER_MODE_OFF          2
#define TIMER_MODE_ACTION_NIGHTMODE_ON    3
#define TIMER_MODE_ACTION_NIGHTMODE_OFF    4
#define TIMER_MODE_ACTION_TIMER_CONFIG_CHANGED    5
#define TIMER_MODE_SET_TIME          6
#define TIMER_MODE_TIMER2_OFFSET          0x100
#define TIMER_MODE_TIMER3_OFFSET          0x200

typedef void (*hnt_platform_timer_action_func)(int, int, int);
void 
hnt_platform_timer_action_regist(void *timer_action_cb);

void 
hnt_platform_timer_get(char *buffer);

void 
hnt_platform_timer_start(char *pbuffer);

void 
hnt_platform_timer_init(void);

int 
hnt_platform_night_mode_set(char *buff);

int 
hnt_platform_night_mode_get(char *buff);

void
user_custom_init(void);

#define random os_random
#define snprintf(A, B, C...) sprintf(A, ##C)    

#define INT_TO_BYTE(value, a,b,c,d) a = ((value & 0xff000000) >> 24);\
                                    b = ((value & 0x00ff0000) >> 16);\
                                    c = ((value & 0x0000ff00) >> 8);\
                                    d = ((value & 0x000000ff));

#define SHORT_TO_BYTE(value, a,b)   a = ((value & 0x0000ff00) >> 8);\
                                    b = ((value & 0x000000ff));

#define BYTE_TO_INT(value, a,b,c,d) value = ((uint32_t)((a) & 0xff) << 24) | \
                 ((uint32_t)((b) & 0xff) << 16) | \
                 ((uint32_t)((c) & 0xff) << 8)  | \
                  (uint32_t)((d) & 0xff)

#define BYTE_TO_SHORT(value, a,b) value = ((uint16_t)((a) & 0xff) << 8) | \
                     ((uint16_t)((b) & 0xff))
#define CHECK_FLAG_BIT(value, bit) (value & (1<<bit))
#define CLEAR_FLAG_BIT(value, bit) (value &= (~(1<<bit)))
#define SET_FLAG_BIT(value, bit) (value |= (1<<bit))

#define SECURITY_DEVICE_SUPPORT 0
#if 1
#define ONBD_DEBUG 1
#define log_debug(F, B...) { \
    if(1)   \
    printf("[%5d][%s]"F,__LINE__, __func__, ##B); \
    }
#define log_info(F, B...) { \
    if(1)   \
    printf("[%5d][%s]"F,__LINE__, __func__, ##B); \
    }
#define log_notice(F, B...) { \
    if(1)   \
    printf("[%5d][%s]"F,__LINE__, __func__, ##B); \
    }

#define log_warning(F, B...) { \
    if(1)   \
    printf("[%5d][%s]"F,__LINE__, __func__, ##B); \
    }
#define log_error(F, B...) { \
    if(1)   \
    printf("[%5d][%s]"F,__LINE__, __func__, ##B); \
    }
#else
#define log_debug(F, B...)
#define log_info(F, B...)
#define log_notice(F, B...)
#define log_warning(F, B...)
#define log_error(F, B...)
#endif
//#define ENABLE_HTTPS 1
//#define CLIENT_SSL_ENABLE 1
//#define HAVE_TLS 1
//#define DLMPD_SUPPORT 1

#endif

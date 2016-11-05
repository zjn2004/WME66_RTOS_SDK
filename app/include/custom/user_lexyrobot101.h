#ifndef _USER_LEXYROBOT101_H
#define _USER_LEXYROBOT101_H

#define MAX_UART_PKT_LEN   64
#define MAX_UART_FWUP_PKT_LEN   128



#define uint8_t uint8
#define uint32_t uint32

enum {
/* GET */
    TYPE_GET_WORKING_STATUS = 0x00,
    TYPE_GET_SLAVE_DEV_TYPE = 0x01,     /*slave dev upgrade*/
    TYPE_GET_SLAVE_DEV_SN = 0x02,     /*slave dev upgrade*/
    TYPE_GET_SLAVE_DEV_SW_VER = 0x03,     /*slave dev upgrade*/
    TYPE_GET_CLEAN_TIMES = 0x04,     
    TYPE_GET_CLEAN_RESERVATION = 0x05,     
    
/* SET */
    TYPE_SET_POWER = 0x10,
    TYPE_SET_DIRECTION = 0x11,
    TYPE_SET_WORKING_MODE = 0x12,  
    TYPE_SET_ONE_TOUCH_CALLING = 0x13,
    TYPE_SET_CLEAN_TIMES = 0x14,
    TYPE_SET_WIFI_LED = 0x15,  
    TYPE_SET_SLAVE_DEV_REBOOT = 0x16,  
    TYPE_SET_CLEAN_RESERVATION = 0x17,     
    TYPE_SET_SLAVE_DEV_REBOOT_TO_BL = 0x18,  
    TYPE_SET_SLAVE_DEV_UPGRADE_START = 0x1A,     /*slave dev upgrade*/
    TYPE_SET_SLAVE_DEV_UPGRADE_DATA = 0x1B,     /*slave dev upgrade*/
    TYPE_SET_SLAVE_DEV_UPGRADE_FINISH = 0x1C,     /*slave dev upgrade*/
    
/* EVENT */
    TYPE_EVENT_ALARM = 0x20,
    TYPE_EVENT_WIFI_RESET = 0x21,
    TYPE_EVENT_WORKING_STATUS = 0x22,
    TYPE_EVENT_UPGRADE_READY = 0x23,    
    TYPE_EVENT_BATTERY_CHANGE = 0x24,    
    TYPE_EVENT_GET_TIME = 0x25,    
};

/* get lexy cleaning robot information */
typedef struct{
    uart_msg_hdr_t hdr;
    uint8_t checksum;
    uint8_t dataEnd;
} __attribute__((packed)) msg_get_common_t;


typedef struct{
    uart_msg_hdr_t hdr;
    uint8_t workingStatus;
    uint8_t battery;
    uint8_t alarm;    
    uint8_t checksum;    
    uint8_t dataEnd;    
} __attribute__((packed)) msg_get_robot_status_rsp_t;

typedef struct{
    uart_msg_hdr_t hdr;
    uint8_t cleantimes;
    uint8_t checksum;
    uint8_t dataEnd;
} __attribute__((packed)) msg_get_cleantimes_rsp_t;

typedef struct{
    uart_msg_hdr_t hdr;
    uint8_t type[16];
    uint8_t checksum;
    uint8_t dataEnd;
} __attribute__((packed)) msg_get_slave_dev_type_rsp_t;

typedef struct{
    uart_msg_hdr_t hdr;
    uint8_t swVersion[32];
    uint8_t checksum;
    uint8_t dataEnd;
} __attribute__((packed)) msg_get_slave_dev_swVer_rsp_t;


typedef struct{
    uart_msg_hdr_t hdr;
    uint8_t enable;
    uint8_t hour;
    uint8_t min;   
    uint8_t checksum;
    uint8_t dataEnd;
} __attribute__((packed)) msg_get_robot_reserv_rsp_t;

/* set power  */
typedef struct {
    uart_msg_hdr_t hdr;
    uint8_t power;
    uint8_t checksum;    
    uint8_t dataEnd;       
}msg_set_power_t;


/* set direction  */
typedef struct {
    uart_msg_hdr_t hdr;
    uint8_t direction;
    uint8_t checksum;
    uint8_t dataEnd;
}msg_set_direction_t;


/* set working mode  */
typedef struct {
    uart_msg_hdr_t hdr;
    uint8_t workingMode;
    uint8_t checksum;
    uint8_t dataEnd;
}msg_set_working_mode_t;


/* set one-touch calling mode  */
typedef struct {
    uart_msg_hdr_t hdr;
    uint8_t on;
    uint8_t checksum;
    uint8_t dataEnd;
}msg_set_one_touch_calling_t;


/* set ordered cleaning mode  */
typedef struct {
    uart_msg_hdr_t hdr;
    uint8_t cleaningTimes;
    uint8_t checksum;
    uint8_t dataEnd;
}msg_set_cleaning_times_t;

typedef struct {
    uart_msg_hdr_t hdr;
    uint8_t enable;
    uint8_t hour;
    uint8_t min;   
    uint8_t checksum;
    uint8_t dataEnd;
}msg_set_robot_reserv_t;


/* set wifi LED  */
typedef struct {
    uart_msg_hdr_t hdr;
    uint8_t wifiLED;
    uint8_t checksum;
    uint8_t dataEnd;
}msg_set_wifi_led_t;

/* set slave device upgrade start  */
typedef struct {
    uart_msg_hdr_t hdr;
    uint8_t checksum;
    uint8_t dataEnd;
}msg_set_slavedev_upgrade_start_t;

/* set slave device upgrade process  */
typedef struct {
    uart_msg_hdr_t hdr;
    uint8_t data[MAX_UART_FWUP_PKT_LEN];
    uint8_t checksum;
    uint8_t dataEnd;
}msg_set_slavedev_upgrade_data_t;

/* set slave device upgrade finish  */
typedef struct {
    uart_msg_hdr_t hdr;
    uint8_t filechecksum[4];    
    uint8_t checksum;
    uint8_t dataEnd;
}msg_set_slavedev_upgrade_finish_t;


typedef struct {
    uart_msg_hdr_t hdr;
    uint8_t checksum;
    uint8_t dataEnd;
}msg_set_slavedev_factory_reset_t;

typedef struct {
    uart_msg_hdr_t hdr;
    uint8_t checksum;
    uint8_t dataEnd;
}msg_set_slavedev_reboot_t;


typedef struct {
    uart_msg_hdr_t hdr;
    uint8_t ret;
    uint8_t checksum;
    uint8_t dataEnd;
}msg_set_common_rsp_t;

/* event */
typedef struct {
    uart_msg_hdr_t hdr;
    uint8_t data[0];
}msg_event_t;

/* get time rsp  */
typedef struct {
    uart_msg_hdr_t hdr;
    uint8_t currentTime[4];
    uint8_t checksum;
    uint8_t dataEnd;
}msg_get_time_rsp_t;

typedef struct{
    uint8_t workingStatus;
    uint8_t battery;
    uint8_t alarm;
    uint32_t lastGetTime;        
}uart_get_lexy_param_t;

#define INT_TO_BYTE(value, a,b,c,d) a = ((value & 0xff000000) >> 24);\
                                        b = ((value & 0x00ff0000) >> 16);\
                                        c = ((value & 0x0000ff00) >> 8);\
                                        d = ((value & 0x000000ff));


#define LEXY_ROBOT_101_POWER                       "OnOff_Power"
#define LEXY_ROBOT_101_DIRECTION                   "Setting_Direction"


#define LEXY_ROBOT_101_WORKING_MODE                   "WorkMode"
#define LEXY_ROBOT_101_BATTERY                   "Status_Battery"
#define LEXY_ROBOT_101_ERR                   "ErrorCode"



#define GPIO_VALUE_0 0
#define GPIO_VALUE_1 1

/* LED */
#define LEXYROBOT101_WIFI_LED_GPIO      12


/* CUSTOM INFO */
/* device info */
#define DEV_NAME "R101xi-B22"
#define DEV_CATEGORY "LIVING"
#define DEV_TYPE "CLEARROBOT"
#define DEV_BRAND "LEXY"
#define DEV_MANUFACTURE "Seaing"

#define DEV_MODEL "SEAING_LIVING_CLEARROBOT_R101XI_B22"
#define ALINK_KEY "NFuWmUAIb2B2ubUxsJR4"
#define ALINK_SECRET "Ruflbc5aDEObmayG9qoTtLmo2LBTCnsKAESRGH7w"
#define ALINK_TPSK "mE5au3ZxMXlJf9F+WESCeXoF4hG/NzfSBTelLoXOCDc="

/*sandbox key/secret*/
#define ALINK_KEY_SANDBOX "2tcHGVrhB96xMrUzN5oc"
#define ALINK_SECRET_SANDBOX "M44yDCSQQvZ7v98Fuismj7t1Cf0zRTPlAZm8k0Zb"
#define ALINK_TPSK_SANDBOX "NFNbhL0NRsUHQ/krrd72vfyL3grBtprI5iaHqmSSz+w="


/*设备硬件信息:系统上电后读取的硬件/固件信息,此处为演示需要,直接定义为宏.产品对接时,需要调用自身接口获取*/
#define DEV_SN "1234567890"
#define DEV_VERSION "1.0.0"

#endif  /* _USER_LEXYAIRPURIFIER_H */

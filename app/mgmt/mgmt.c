/******************************************************************************
 * Copyright 2013-2014 Seaing Systems (Wuxi)
 *
 * FileName: user_esp_platform.c
 *
 * Description: The client mode configration.
 *              Check your hardware connection with the host while use this mode.
 *
 * Modification history:
 *     2014/5/09, v1.0 create this file.
*******************************************************************************/
//#include "ets_sys.h"
//#include "os_type.h"
//#include "mem.h"
//#include "osapi.h"
#include "hnt_interface.h"
#include "esp_common.h"
#include "espconn.h"
#include "mgmt/mgmt.h"
#include "mgmt/welcome.h"
#include "iksemel/iksemel.h"

struct hnt_factory_param g_hnt_factory_param;
#if 1//def HNT_PTO_SUPPORT
#define PTO_INFO_MAGIC      0xAA
LOCAL uint8_t pto_enable = 0;
#endif

int ICACHE_FLASH_ATTR
json_get_value(char* json_str, char* key, char* value, int len)
{
    char *start, *end, *ptr;
    if((char *)os_strstr(json_str, key) == NULL) {
        log_debug("test\n");
        return -1;
    }

    ptr = (char *)os_strstr(json_str, key);
    ptr += os_strlen(key);

/* fix the bug when pwd is null ex."pass":"",*/
    while(*ptr == ' ') ptr++;    
    if(*ptr == '\"') ptr++;
    while(*ptr == ' ') ptr++;    
    if(*ptr == ':') ptr++;
    while(*ptr == ' ') ptr++;    
    if(*ptr == '\"') ptr++;
            
    start = ptr;    
    while(*ptr) {
        if(*ptr=='\"') {
            break;
        }
        else{
            ptr++;
        }
    }
    end = ptr;

	if ((end-start)  > len )
    	os_strncpy(value, start, len);
    else
    	os_strncpy(value, start, end-start);

    return 0;
}

/******************************************************************************
 * FunctionName : hnt_mgmt_load_factory_param
 * Description  : load parameter from flash, toggle use two sector by flag value.
 * Parameters   : param--the parame point which write the flash
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
hnt_mgmt_load_factory_param(struct hnt_mgmt_factory_param *param)
{
    spi_flash_read((ESP_PARAM_START_SEC + ESP_PARAM_FACTORY) * SPI_FLASH_SEC_SIZE,
                   (uint32 *)param, sizeof(struct hnt_mgmt_factory_param));
}

void ICACHE_FLASH_ATTR
hnt_mgmt_save_factory_param(struct hnt_mgmt_factory_param *param)
{
    spi_flash_erase_sector(ESP_PARAM_START_SEC + ESP_PARAM_FACTORY);
    spi_flash_write((ESP_PARAM_START_SEC + ESP_PARAM_FACTORY) * SPI_FLASH_SEC_SIZE,
                    (uint32 *)param, sizeof(struct hnt_mgmt_factory_param));
}

void ICACHE_FLASH_ATTR
hnt_mgmt_erase_factory_param(void)
{
    spi_flash_erase_sector(ESP_PARAM_START_SEC + ESP_PARAM_FACTORY);
}


/******************************************************************************
 * FunctionName : hnt_mgmt_load_param
 * Description  : load parameter from flash, toggle use two sector by flag value.
 * Parameters   : param--the parame point which write the flash
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
hnt_mgmt_load_param(struct hnt_mgmt_saved_param *param)
{
    struct hnt_mgmt_sec_flag_param flag;

    spi_flash_read((ESP_PARAM_START_SEC + ESP_PARAM_FLAG) * SPI_FLASH_SEC_SIZE,
                   (uint32 *)&flag, sizeof(struct hnt_mgmt_sec_flag_param));

    if (flag.flag == 0) {
        spi_flash_read((ESP_PARAM_START_SEC + ESP_PARAM_SAVE_0) * SPI_FLASH_SEC_SIZE,
                       (uint32 *)param, sizeof(struct hnt_mgmt_saved_param));
    } else {
        spi_flash_read((ESP_PARAM_START_SEC + ESP_PARAM_SAVE_1) * SPI_FLASH_SEC_SIZE,
                       (uint32 *)param, sizeof(struct hnt_mgmt_saved_param));
    }
}

/******************************************************************************
 * FunctionName : hnt_mgmt_save_param
 * Description  : toggle save param to two sector by flag value,
 *              : protect write and erase data while power off.
 * Parameters   : param -- the parame point which write the flash
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
hnt_mgmt_save_param(struct hnt_mgmt_saved_param *param)
{
    struct hnt_mgmt_sec_flag_param flag;

    spi_flash_read((ESP_PARAM_START_SEC + ESP_PARAM_FLAG) * SPI_FLASH_SEC_SIZE,
                   (uint32 *)&flag, sizeof(struct hnt_mgmt_sec_flag_param));

    ETS_GPIO_INTR_DISABLE();    
    if (flag.flag == 0) {
        spi_flash_erase_sector(ESP_PARAM_START_SEC + ESP_PARAM_SAVE_1);
        spi_flash_write((ESP_PARAM_START_SEC + ESP_PARAM_SAVE_1) * SPI_FLASH_SEC_SIZE,
                        (uint32 *)param, sizeof(struct hnt_mgmt_saved_param));
        flag.flag = 1;
        spi_flash_erase_sector(ESP_PARAM_START_SEC + ESP_PARAM_FLAG);
        spi_flash_write((ESP_PARAM_START_SEC + ESP_PARAM_FLAG) * SPI_FLASH_SEC_SIZE,
                        (uint32 *)&flag, sizeof(struct hnt_mgmt_sec_flag_param));
    } else {
        spi_flash_erase_sector(ESP_PARAM_START_SEC + ESP_PARAM_SAVE_0);
        spi_flash_write((ESP_PARAM_START_SEC + ESP_PARAM_SAVE_0) * SPI_FLASH_SEC_SIZE,
                        (uint32 *)param, sizeof(struct hnt_mgmt_saved_param));
        flag.flag = 0;
        spi_flash_erase_sector(ESP_PARAM_START_SEC + ESP_PARAM_FLAG);
        spi_flash_write((ESP_PARAM_START_SEC + ESP_PARAM_FLAG) * SPI_FLASH_SEC_SIZE,
                        (uint32 *)&flag, sizeof(struct hnt_mgmt_sec_flag_param));
    }
    ETS_GPIO_INTR_ENABLE();
}

/******************************************************************************
 * FunctionName : hnt_mgmt_save_param
 * Description  : toggle save param to two sector by flag value,
 *              : protect write and erase data while power off.
 * Parameters   : param -- the parame point which write the flash
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
hnt_mgmt_factory_reset(void)
{
    spi_flash_erase_sector(ESP_PARAM_START_SEC + ESP_PARAM_SAVE_0);
    spi_flash_erase_sector(ESP_PARAM_START_SEC + ESP_PARAM_SAVE_1);
    spi_flash_erase_sector(ESP_PARAM_START_SEC + ESP_PARAM_FLAG);    
}

void ICACHE_FLASH_ATTR
hnt_mgmt_system_factory_reset(void)
{
    system_restore();
    hnt_mgmt_factory_reset();    
    //os_delay_us(10000);
    system_restart();
}

int ICACHE_FLASH_ATTR
hnt_mgmt_get_factory_info(struct hnt_mgmt_factory_param *factory_param)
{
    if(factory_param == NULL)
        return -1;

    hnt_mgmt_load_factory_param(factory_param);
        
    return 0;
}

void ICACHE_FLASH_ATTR
hnt_mgmt_anti_copy_enable(uint8_t enable)
{
    pto_enable = enable ? 1: 0;
}

/******************************************************************************
 * FunctionName : hnt_mgmt_factory_init
 * Description  : device parame init based on Seaing platform
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
int ICACHE_FLASH_ATTR
hnt_mgmt_factory_init(struct hnt_mgmt_factory_param *factory_param)
{
    struct hnt_mgmt_factory_param tmp_factory_param;   
#if 0//def HNT_PTO_SUPPORT
    uint8 pto_info[4] = {0};
    int i;
#endif
    
    memset(&tmp_factory_param, 0, sizeof(tmp_factory_param));
    if(factory_param == NULL)
        hnt_mgmt_load_factory_param(&tmp_factory_param);
    else
        memcpy(&tmp_factory_param, factory_param, sizeof(struct hnt_mgmt_factory_param));

    log_debug("test:device_type = %s\n", tmp_factory_param.device_type);
    log_debug("test:device_id = %s\n", tmp_factory_param.device_id);
    log_debug("test:password = %s\n", tmp_factory_param.password);
    log_debug("test:xmpp_server = %s\n", tmp_factory_param.xmpp_server);
    log_debug("test:xmpp_jid = %s\n", tmp_factory_param.xmpp_jid);
    log_debug("test:xmpp_password = %s\n", tmp_factory_param.xmpp_password);
    log_debug("test:mac = %02x:%02x:%02x:%02x:%02x:%02x\n", tmp_factory_param.wlan_mac[0], tmp_factory_param.wlan_mac[1],
    	tmp_factory_param.wlan_mac[2], tmp_factory_param.wlan_mac[3], tmp_factory_param.wlan_mac[4], tmp_factory_param.wlan_mac[5]);
#if 0//def HNT_PTO_SUPPORT
    log_debug("test:pto_info = %02x:%02x:%02x:%02x\n", 
                    tmp_factory_param.pto_info[0], 
                    tmp_factory_param.pto_info[1], 
                    tmp_factory_param.pto_info[2], 
                    tmp_factory_param.pto_info[3]);
#endif

    if (tmp_factory_param.device_type[0] != 0xFF)
        g_hnt_factory_param.device_type = atoi(tmp_factory_param.device_type);
    else
        return -1;

            
    if (tmp_factory_param.device_id[0] != 0xFF)
    {
        tmp_factory_param.device_id[DEVICEINFO_STRING_LEN - 1] = '\0';
        g_hnt_factory_param.device_id = iks_strdup(tmp_factory_param.device_id);
    }
    else
        return -1;
            
    if (tmp_factory_param.password[0] != 0xFF)
    {
        tmp_factory_param.password[DEVICEINFO_STRING_LEN - 1] = '\0';
        g_hnt_factory_param.password = iks_strdup(tmp_factory_param.password);
    }
    else
        return -1;

    if (tmp_factory_param.xmpp_server[0] != 0xFF)
    {
        tmp_factory_param.xmpp_server[DEVICEINFO_STRING_LEN - 1] = '\0';
        g_hnt_factory_param.xmpp_server = iks_strdup(tmp_factory_param.xmpp_server);
    }
    else
        return -1;

    if (tmp_factory_param.xmpp_jid[0] != 0xFF)
    {
        tmp_factory_param.xmpp_jid[DEVICEINFO_STRING_LEN - 1] = '\0';
        g_hnt_factory_param.xmpp_jid = iks_strdup(tmp_factory_param.xmpp_jid);
    }
    else
        return -1;

    if (tmp_factory_param.xmpp_password[0] != 0xFF)
    {
        tmp_factory_param.xmpp_password[DEVICEINFO_STRING_LEN - 1] = '\0';
        g_hnt_factory_param.xmpp_password = iks_strdup(tmp_factory_param.xmpp_password);
    }
    else
        return -1;
    
    if (tmp_factory_param.wlan_mac[0] != 0xFF)
    {
        memcpy(g_hnt_factory_param.wlan_mac, tmp_factory_param.wlan_mac, 6);
        wifi_set_macaddr(STATION_IF, g_hnt_factory_param.wlan_mac);        
    }
    else
        return -1;

    return 0;    
}   


bool ICACHE_FLASH_ATTR
hnt_mgmt_platform_reset_mode(void)
{
    if (wifi_get_opmode() == STATION_MODE) {
        wifi_set_opmode(STATIONAP_MODE);
    }

    return false;
}


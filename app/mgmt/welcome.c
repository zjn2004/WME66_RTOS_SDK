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
//#include "user_interface.h"
//#include "hnt_interface.h"
//#include "espconn.h"
#if 0
#include "hnt_interface.h"
#include "esp_common.h"
#include "mgmt/mgmt.h"
#include "mgmt/welcome.h"
//#include "version.h"
#include "iksemel/iksemel.h"
#include "ping.h"
#include "ip_addr.h"

welcome_info_t *welcome_info_table = NULL;
struct espconn *welcome_udpSendEspconn = NULL;
LOCAL os_timer_t welcome_timer;
hnt_welcome_action_welcome_func welcome_action_welcome_cb_func = NULL;
hnt_welcome_action_leave_func welcome_action_leave_cb_func = NULL;
LOCAL uint8 welcome_current_index = 0;

void welcome_ping_test(uint32_t ip);
void welcome_start(void);
void welcome_init(void);
void welcome_write_table_to_flash(void);

LOCAL void ICACHE_FLASH_ATTR
welcome_excute_leave_action_cb(int index)
{
    os_timer_disarm(&welcome_timer);
    welcome_ping_test(welcome_info_table[index].client_ip_addr);    
    os_timer_setfn(&welcome_timer, (os_timer_func_t *)welcome_excute_leave_action_cb, index);
    os_timer_arm(&welcome_timer, WELCOME_PING_TIMEOUT*1000, 0);
}

LOCAL void ICACHE_FLASH_ATTR
welcome_excute_welcome_action_cb(int index)
{
    log_debug("test\n");
    SET_FLAG_BIT(welcome_info_table[index].pad, WELCOME_ACTION_WELCOME_ENABLE_BIT);
    welcome_ping_test(welcome_info_table[index].client_ip_addr);    
    os_timer_disarm(&welcome_timer);
    os_timer_setfn(&welcome_timer, (os_timer_func_t *)welcome_excute_leave_action_cb, index);
    os_timer_arm(&welcome_timer, WELCOME_PING_TIMEOUT*1000, 0);
}

LOCAL void ICACHE_FLASH_ATTR
welcome_excute_welcome_action(int index)
{
    log_debug("test index = %d\n", index);
    
    if(welcome_action_welcome_cb_func && 
        CHECK_FLAG_BIT(welcome_info_table[index].welcome_flag, WELCOME_ACTION_WELCOME_ENABLE_BIT) &&
        CHECK_FLAG_BIT(welcome_info_table[index].pad, WELCOME_ACTION_WELCOME_ENABLE_BIT)
        )
    {
        log_debug("test\n");
        welcome_action_welcome_cb_func();
        CLEAR_FLAG_BIT(welcome_info_table[index].pad, WELCOME_ACTION_WELCOME_ENABLE_BIT);
        os_timer_disarm(&welcome_timer);
        os_timer_setfn(&welcome_timer, (os_timer_func_t *)welcome_excute_welcome_action_cb, index);
        os_timer_arm(&welcome_timer, WELCOME_REQUEST_TIMEOUT*1000, 0);
    }
}

LOCAL void ICACHE_FLASH_ATTR
welcome_excute_leave_action(int index)
{
    log_debug("test\n");
    if(welcome_action_leave_cb_func && 
        CHECK_FLAG_BIT(welcome_info_table[index].welcome_flag, WELCOME_ACTION_LEAVE_ENABLE_BIT))
        welcome_action_leave_cb_func();
}

LOCAL void ICACHE_FLASH_ATTR
welcome_recv(void *arg, char *pusrdata, unsigned short length)
{
    struct espconn *pespconn = arg;
    log_debug("RECV:length=%d\n", length);
    log_debug("udp->remote_port:%d\n", pespconn->proto.udp->remote_port);
    log_debug("udp->remote_ip:%d.%d.%d.%d\n", pespconn->proto.udp->remote_ip[0],
                                            pespconn->proto.udp->remote_ip[1],
                                            pespconn->proto.udp->remote_ip[2],
                                            pespconn->proto.udp->remote_ip[3]);
    if (pusrdata == NULL) {
        return;
    }

    uint8_t msg_type = pusrdata[DHCP_OPTION_MESSAGE_TYPE_OFFSET+2];
    uint8_t msg_len;
    char *client_mac;
    char *client_ip;
    int index = 0;
    struct ip_addr temp_ip;
    
    if(wifi_get_opmode() != STATION_MODE)
        return;
        
    log_debug("RECV:msg type = %d\n", msg_type);
    if(msg_type != 0x03)
    {
        return;
    }

    client_mac = &pusrdata[DHCP_CLIENT_MAC_ADDRESS_OFFSET];
    log_debug("MAC:" MACSTR"\n", client_mac[0],
                                 client_mac[1], 
                                 client_mac[2], 
                                 client_mac[3], 
                                 client_mac[4], 
                                 client_mac[5]) ;  
    if((index = welcome_search_mac_from_table(client_mac)) == -1)
        return;

    log_debug("test\n");

    client_ip = &pusrdata[DHCP_CLIENT_IP_ADDRESS_OFFSET];
    if((client_ip[0] != 0) || (client_ip[1] != 0) || (client_ip[2] != 0) || (client_ip[3] != 0))
    {
        log_debug("IP:%d.%d.%d.%d\n", client_ip[0], client_ip[1], client_ip[2], client_ip[3]);
        IP4_ADDR(&temp_ip, client_ip[0], client_ip[1], client_ip[2], client_ip[3]);
        goto handler;
    }
    log_debug("test\n");

    int offset = DHCP_OPTION_OFFSET;
    uint8_t option_type = 0;
    while(1)
    {
        option_type = pusrdata[offset];
        if(option_type == 0xFF)
        {
            log_debug("test\n");
            break;
        }
            
        if(option_type == 50)
        {
            client_ip = &pusrdata[offset+2];
            log_debug("IP:%d.%d.%d.%d\n", client_ip[0], client_ip[1], client_ip[2], client_ip[3]);
            IP4_ADDR(&temp_ip, client_ip[0], client_ip[1], client_ip[2], client_ip[3]);
            break;
        }        
        msg_len = pusrdata[offset+1];
        offset = offset + 2 + msg_len;
    }

handler:
    log_debug("test\n");    
    struct ip_info ipconfig;
    wifi_get_ip_info(STATION_IF, &ipconfig);

    if(ip_addr_netcmp(&temp_ip, &ipconfig.ip, &ipconfig.netmask))
    {
        log_debug("test\n");        
        welcome_info_table[index].client_ip_addr = temp_ip.addr;
        welcome_current_index = index;
        welcome_excute_welcome_action(index);
        welcome_write_table_to_flash();
    }
}

void ICACHE_FLASH_ATTR
welcome_ping_recv_cb(void* arg, void *pdata)
{
    struct ping_option *ping_opt = (struct ping_option *)arg;
    struct ping_resp *pingresp = (struct ping_resp *)pdata;

    log_debug("test\n");
    log_debug("ping resp:total_count:%d, resp_time:%d, timeout_count:%d, ping_err:%d\n",
        pingresp->total_count, pingresp->resp_time, pingresp->timeout_count, pingresp->ping_err);
}

void ICACHE_FLASH_ATTR
welcome_ping_sent_cb(void* arg, void *pdata)
{
    struct ping_option *ping_opt = (struct ping_option *)arg;
    struct ping_resp *pingresp = (struct ping_resp *)pdata;
    log_debug("test\n");
#if 1 
    log_debug("ping resp:total_count:%d, resp_time:%d, timeout_count:%d, ping_err:%d\n",
        pingresp->total_count, pingresp->resp_time, pingresp->timeout_count, pingresp->ping_err);
    if(pingresp->timeout_count == pingresp->total_count)
    {
        log_debug("test\n");
        os_timer_disarm(&welcome_timer);
        welcome_excute_leave_action(welcome_current_index); 
    }    
#endif
    if(ping_opt != NULL)
        free(ping_opt);
}

void ICACHE_FLASH_ATTR
welcome_ping_test(uint32_t ip)
{
    struct ping_option *ping_opt = (struct ping_option *)zalloc(sizeof(struct ping_option));
    
    log_debug("test\n");
    ping_opt->count = WELCOME_MAX_PING_COUNT;    

    ping_opt->ip = ip;
    log_debug("test"IPSTR "\n", IP2STR(&ping_opt->ip));
//    ping_opt->recv_function = welcome_ping_recv_cb;
//    log_debug("test\n");
    ping_opt->sent_function = welcome_ping_sent_cb;
    log_debug("test\n");

    ping_start(ping_opt);
}

void ICACHE_FLASH_ATTR
welcome_packet_handle(void)
{
    sint8 ret;
    log_debug("system_get_free_heap_size = %d\n", system_get_free_heap_size());

    welcome_udpSendEspconn = (struct espconn*)zalloc(sizeof(struct espconn));
    welcome_udpSendEspconn->type = ESPCONN_UDP;
    welcome_udpSendEspconn->proto.udp = (esp_udp *)zalloc(sizeof(esp_udp));
    welcome_udpSendEspconn->proto.udp->local_port = 67;  

    espconn_regist_recvcb(welcome_udpSendEspconn, welcome_recv);
    ret = espconn_create(welcome_udpSendEspconn);    
    log_debug("ret = %d\n", ret);
}

int ICACHE_FLASH_ATTR
welcome_search_mac_from_table(char *client_mac)
{
    int i;
    for(i = 0; i < HNT_WELCOME_INFO_NUM; i++)
    {
        log_debug("test\n");
        if(welcome_info_table[i].welcome_flag == WELCOME_DIS_LEAVE_DIS)
            continue;
        
        if(os_memcmp(welcome_info_table[i].client_mac_addr, client_mac, 6) == 0)
        {
            log_debug("test\n");
            return i;
        }
    }
    log_debug("test\n");

    return -1;
}

void ICACHE_FLASH_ATTR
welcome_write_table_to_flash(void)
{
    struct hnt_mgmt_saved_param param;

    memset(&param, 0, sizeof(param));
    hnt_mgmt_load_param(&param);
     
    memcpy(&param.welcome_info, welcome_info_table, (HNT_WELCOME_INFO_NUM * sizeof(welcome_info_t)));
    hnt_mgmt_save_param(&param);
}
void ICACHE_FLASH_ATTR
welcome_add_one_client(uint8_t mode, char *client_mac)
{
    int i;
    int find_loc = -1;
    
    for(i = 0; i < HNT_WELCOME_INFO_NUM; i++)
    {
        if((welcome_info_table[i].welcome_flag == WELCOME_DIS_LEAVE_DIS) && (find_loc == -1))
        {
            find_loc = i;
            log_debug("test\n");
            continue;
        }
        
        if(os_memcmp(welcome_info_table[i].client_mac_addr, client_mac, 6) == 0)
        {
            log_debug("test\n");
            welcome_info_table[i].welcome_flag = mode;
            welcome_info_table[i].pad =0xFF;
            goto write_to_flash;
        }
    }
    log_debug("test\n");

    if(i == HNT_WELCOME_INFO_NUM && find_loc != -1)
    {
        log_debug("test\n");
        welcome_info_table[find_loc].welcome_flag = mode;
        welcome_info_table[find_loc].pad =0xFF;
        memcpy(welcome_info_table[find_loc].client_mac_addr, client_mac, 6);    
    }
    else
    {
        return;
    }

write_to_flash:
    log_debug("test\n");
    welcome_write_table_to_flash();
}

void ICACHE_FLASH_ATTR
welcome_del_one_client(uint8_t mode, char *client_mac)
{
    int i;
    for(i = 0; i < HNT_WELCOME_INFO_NUM; i++)
    {
        if(os_memcmp(welcome_info_table[i].client_mac_addr, client_mac, 6) == 0)
        {
            log_debug("test\n");
            memset(&welcome_info_table[i], 0, sizeof(welcome_info_t));
            welcome_write_table_to_flash();
        }
    }
}

void ICACHE_FLASH_ATTR
welcome_path_handle(char *path_string) 
{
    char temp_str[32] = {0};
    int mode = 0;
    int i;
    char client_mac[6];
    char *pPara = NULL;
    
    if(path_string == NULL)
        return;

    json_get_value(path_string, "mode", temp_str, sizeof(temp_str) - 1);
    mode = atoi(temp_str);
    log_debug("test mode = %s\n", temp_str);

    json_get_value(path_string, "mac", temp_str, sizeof(temp_str) - 1);
    log_debug("test mac = %s\n", temp_str);

    pPara = temp_str;
    for(i=0;i<6;i++)
    {
      client_mac[i] = strtol(pPara,&pPara,16);
      pPara += 1;
    }

    if(welcome_info_table == NULL)
        return;

    log_debug("test\n");
    if(mode == WELCOME_DIS_LEAVE_DIS)
    {            
        welcome_del_one_client(mode, client_mac);
    }
    else 
    {
        welcome_add_one_client(mode, client_mac);  
    }
}

void ICACHE_FLASH_ATTR
welcome_path_get(char *path_string, int size) 
{
    struct hnt_mgmt_saved_param param;
    int i;
    int num = 0;
    int len = 0;
    char zero_mac[6] = {0};
    char ff_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    memset(&param, 0, sizeof(param));
    hnt_mgmt_load_param(&param);

    len += snprintf(path_string+len, sizeof(path_string) - len, "[");
    for(i = 0; i < HNT_WELCOME_INFO_NUM; i++)
    {
        if((param.welcome_info[i].welcome_flag == WELCOME_DIS_LEAVE_DIS) ||
            (param.welcome_info[i].welcome_flag == WELCOME_ENA_LEAVE_DIS) ||
            (param.welcome_info[i].welcome_flag == WELCOME_DIS_LEAVE_ENA) || 
            (param.welcome_info[i].welcome_flag == WELCOME_ENA_LEAVE_ENA))
        {
            log_debug("test i = %d\n", i);
            log_debug("param.welcome_info[%d].welcome_flag = %d\n", i, param.welcome_info[i].welcome_flag);
            log_debug("param.welcome_info[%d].pad = %d\n", i, param.welcome_info[i].pad);
            log_debug("param.welcome_info[%d].mac = " MACSTR"\n", i, MAC2STR(param.welcome_info[i].client_mac_addr));
            
            if((os_memcmp(param.welcome_info[i].client_mac_addr, zero_mac, 6) != 0) &&
                (os_memcmp(param.welcome_info[i].client_mac_addr, ff_mac, 6) != 0))
            {
                len += snprintf(path_string+len, sizeof(path_string) - len, "{\"mode\":\"%d\",\"mac\":\""MACSTR"\"},",
                    param.welcome_info[i].welcome_flag, MAC2STR(param.welcome_info[i].client_mac_addr));

                num ++;
            }
        }
    }

    if(num > 0)
    {
        len--;
    }
    
    len += snprintf(path_string+len, sizeof(path_string) - len, "]");
    log_debug("path_string:%s\n", path_string);
}


void ICACHE_FLASH_ATTR
welcome_start(void)
{ 
    welcome_packet_handle();
}

void ICACHE_FLASH_ATTR
welcome_stop(void)
{
    if(welcome_udpSendEspconn != NULL)
    {
        espconn_delete(welcome_udpSendEspconn);
        if(welcome_udpSendEspconn->proto.udp != NULL)
            free(welcome_udpSendEspconn->proto.udp);

        free(welcome_udpSendEspconn);
        welcome_udpSendEspconn = NULL;
    }
}

void ICACHE_FLASH_ATTR
welcome_restart(void)
{ 
    welcome_stop();
    welcome_start();
}

void ICACHE_FLASH_ATTR
welcome_init(void)
{
    int i;
    int num = 0;
    struct hnt_mgmt_saved_param param;

    if((welcome_action_welcome_cb_func == NULL) && 
       (welcome_action_leave_cb_func == NULL))
       return;

    memset(&param, 0, sizeof(param));
    hnt_mgmt_load_param(&param);

    for(i = 0; i < HNT_WELCOME_INFO_NUM; i++)
    {
        if((param.welcome_info[i].welcome_flag == WELCOME_ENA_LEAVE_DIS) ||
            (param.welcome_info[i].welcome_flag == WELCOME_DIS_LEAVE_ENA) || 
            (param.welcome_info[i].welcome_flag == WELCOME_ENA_LEAVE_ENA))
        {
            log_debug("test i = %d\n", i);
            log_debug("param.welcome_info[%d].welcome_flag = %d\n", i, param.welcome_info[i].welcome_flag);
            log_debug("param.welcome_info[%d].pad = %d\n", i, param.welcome_info[i].pad);
            log_debug("param.welcome_info[%d].mac = " MACSTR"\n", i, MAC2STR(param.welcome_info[i].client_mac_addr));
            
            num++;
        }
    }
    log_debug("test\n");

    if(welcome_info_table != NULL)
        free(welcome_info_table);
        
    welcome_info_table = (welcome_info_t *)zalloc(HNT_WELCOME_INFO_NUM * sizeof(welcome_info_t));
    if(num != 0)
    {    
        log_debug("test\n");
        memcpy(welcome_info_table, param.welcome_info, (HNT_WELCOME_INFO_NUM * sizeof(welcome_info_t)));
    }

    for(i = 0; i < HNT_WELCOME_INFO_NUM; i++)
    {
        welcome_info_table[i].pad = 0xFF;
    }
    
    welcome_restart();
}


void ICACHE_FLASH_ATTR
welcome_action_regist(void *welcome_action_cb, void*leave_action_cb)
{
    welcome_action_welcome_cb_func = (hnt_welcome_action_welcome_func)welcome_action_cb;
    welcome_action_leave_cb_func = (hnt_welcome_action_leave_func)leave_action_cb;
}
#endif

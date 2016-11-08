/*
 * Copyright (c) 2014 Seaing, Ltd.
 * All Rights Reserved. 
 * Seaing Confidential and Proprietary.
 * 
 * FILENAME: onbd.c
 * 
 * DESCRIPTION: onboarding task
 * 
 * Date Created: 2015年6月23日
 * 
 * Author: JianHua Zhou (zhoujianhua@seaing.net)
 */
#if 1
#include "esp_common.h"

#include "mgmt/mgmt.h"
#include "iksemel/iksemel.h"
#include "onbd/onbd.h"
#include "lwip/sockets.h"
#include "hnt_interface.h"

#define CONFIG_AIRKISS_SUPPORT 1

#ifdef CONFIG_AIRKISS_SUPPORT
#include "onbd/airkiss.h"
#endif

extern struct hnt_mgmt_saved_param g_hnt_param;
extern struct hnt_factory_param g_hnt_factory_param;

static uint8 onbd_udp_notify_times = ONBD_UDP_TIMES;
static char *modelDescription = NULL; //"{\"jid\":\"13666666669@net\", \"devicetype\":\"2\"}";
int onbd_mgmt_udp_abort = 0;

#define byte_to_int(value, a,b,c,d) value = ((uint32_t)((a) & 0xff) << 24) | \
                 ((uint32_t)((b) & 0xff) << 16) | \
                 ((uint32_t)((c) & 0xff) << 8)  | \
                  (uint32_t)((d) & 0xff)

#define byte_to_short(value, a,b) value = ((uint16_t)((a) & 0xff) << 8) | \
                     ((uint16_t)((b) & 0xff))

static int step = STEP_START;
static uint8_t src_addr1[MAC_LEN], src_addr2[MAC_LEN];
static uint16_t data_len1, data_len2;
static char *encrypt_str= NULL;
//#define ONBD_DEBUG 1
static char *encrypt_num_str= NULL;
static char *decrypt_str= NULL;
static uint32_t data_len;
static uint8_t *scan_chan_list = NULL;
static uint8_t need_scan_channel_num = 0;
static uint8_t curr_chan_index;
static uint8_t encrypt_data_num;
static uint8_t preamble1_count;
static uint8_t preamble2_count;
static uint8_t switch_channel_times = 0;
static uint8_t channel_wait_times = 0;

static uint8_t info_ip[4] = {0};
static unsigned short info_port = 0;
static u8 router_ap_bssid[6];

struct router_info {
    SLIST_ENTRY(router_info)     next;

    u8 bssid[6];
    char ssid[32+1];
    u8 channel;
    u8 authmode;
};
struct router_info *cache_info = NULL;
struct router_info *route_ap_info = NULL;
char *onbd_ssid = NULL;
char *onbd_pwd = NULL;

char *device_ap_ssid = NULL;
LOCAL os_timer_t onbd_notify_timer;

void onbd_mgmt_apconfig_start(void);
void hnt_onbd_fail_handler(void);
void onbd_mgmt_smartconfig_finish(void);
void onbd_mgmt_smartconfig_stop(void);
void hnt_onbd_stop(void);

SLIST_HEAD(router_info_head, router_info) router_list;

static u8 gHntSrcMac[ETH_ALEN] = {0,0,0,0,0,0};
static u8 ICACHE_FLASH_ATTR
hnt_compare_ether_addr(const u8 *addr1, const u8 *addr2)
{
	return !((addr1[0] == addr2[0]) && (addr1[1] == addr2[1]) && (addr1[2] == addr2[2]) &&   \
		(addr1[3] == addr2[3]) && (addr1[4] == addr2[4]) && (addr1[5] == addr2[5]));
}

static u8 ICACHE_FLASH_ATTR
hnt_wifi_compare_mac_addr(u8 *macaddr){
	u8 tmpmacaddr[ETH_ALEN] = {0, 0,0,0,0,0};	

	if (macaddr == NULL){
		return 0;
	}

	if (hnt_compare_ether_addr(gHntSrcMac, tmpmacaddr) == 0){
		memcpy(gHntSrcMac, macaddr, ETH_ALEN);
		return 0;
	}

	if (hnt_compare_ether_addr(gHntSrcMac, macaddr) == 0){
		return 1;
	}
	return 0;
}


void 
connect_to_ap(const char* ssid, const char *pwd)
{
    struct station_config sta_conf;
    struct router_info *info = NULL;
    if(str_isprint(ssid) == 0)
    {
        SLIST_FOREACH(info, &router_list, next) {
            if (memcmp(info->bssid, router_ap_bssid, 6) == 0) {
                if(strlen(info->ssid) == 0)
                    strcpy(info->ssid, ssid);
#ifdef ONBD_DEBUG
                printf("find it %s\n", info->ssid);
#endif
                break;
            }
        }  
    }
    
    memset(&sta_conf, 0, sizeof(sta_conf));
    strncpy(sta_conf.password, pwd, 64);
    printf("connect to ap ");
    if(info != NULL)
    {
        printf("%s\n", info->ssid);

        strncpy(sta_conf.ssid, info->ssid, 32);
        sta_conf.bssid_set = 1;
        memcpy(sta_conf.bssid, info->bssid, 6);
    }  
    else
    {
        printf("%s\n", ssid);
        strncpy(sta_conf.ssid, ssid, 32);
    }
    
    wifi_station_set_config(&sta_conf);
    wifi_station_disconnect();        
    wifi_station_connect();        
}

int ICACHE_FLASH_ATTR
promisc_get_fixed_channel(void *current_bssid, u8 *scanned_ssid)
{
    int channel = 0;
	struct router_info *info = NULL;
    SLIST_FOREACH(info, &router_list, next) {
        if (memcmp(info->bssid, current_bssid, 6) == 0) {
            channel = info->channel;
            if(strlen(info->ssid) != 0)
                strcpy(scanned_ssid, info->ssid);
            break;
        }  
    }
    
    return channel;
}

#ifdef CONFIG_AIRKISS_SUPPORT
airkiss_context_t *akcontex = NULL;
airkiss_config_t *akconf = NULL;

static void ICACHE_FLASH_ATTR
airkiss_finish(void)
{
    int err;
    airkiss_result_t result;
    err = airkiss_get_result(akcontex, &result); 
    if (err == 0)
    {
        printf("get_result() ok!");
        printf("ssid = \"%s\", pwd = \"%s\", ssid_length = %d," 
        "pwd_length = %d, random = 0x%02x\r\n",
        result.ssid, result.pwd, result.ssid_length, 
        result.pwd_length, result.random);
        info_ip[0] = 0;
        info_ip[1] = 0;
        info_ip[2] = 0;
        info_ip[3] = result.random;
        info_port = 10000;
        connect_to_ap(result.ssid, result.pwd);
        onbd_mgmt_smartconfig_finish();
    }
    else
    {
        printf("broadcast_get_result() failed !\r\n");
    }
}

static void ICACHE_FLASH_ATTR
hnt_airkiss_fill_config(airkiss_config_t *pconfig)
{
    pconfig->memset = (airkiss_memset_fn)&memset;
    pconfig->memcpy = (airkiss_memcpy_fn)&memcpy;
    pconfig->memcmp = (airkiss_memcmp_fn)&memcmp;
    pconfig->printf = (airkiss_printf_fn)&printf;

    return;
}

void ICACHE_FLASH_ATTR
hnt_airkiss_start(void)
{
    int ret = -1;
#ifdef ONBD_DEBUG
    printf("start airkiss config...\r\n"
           "airkiss version: %s\r\n", airkiss_version());
#endif
    /* 1.init akcontex */
    if (NULL == akcontex)
    {
        akcontex = malloc(sizeof(airkiss_context_t));
        if (NULL == akcontex)
        {
            //printf("failed to malloc airkiss context.\r\n");
            return;
        }
    }

    /* 2. init akconf */
    if (NULL == akconf)
    {
        akconf = malloc(sizeof(airkiss_config_t));
        if (NULL == akconf)
        {
            //printf("failed to malloc airkiss config.\r\n");
            free(akconf);
            akconf = NULL;
            return;
        }
    }

    memset(akcontex, 0, sizeof(airkiss_context_t));
    memset(akconf,  0, sizeof(airkiss_config_t));
    hnt_airkiss_fill_config(akconf);

    /* 3.初始化airkiss */
    ret = airkiss_init(akcontex, akconf);
    if (0 != ret)
    {
        //printf("failed to init airkiss.\r\n");
        free(akcontex);
        akcontex = NULL;
        free(akconf);
        akconf  = NULL;
        return;
    }
    
    return;
}

void ICACHE_FLASH_ATTR
hnt_airkiss_stop(void)
{
    //printf("stop airkiss oneshot config...\r\n");
    if (NULL != akconf)
    {
        free(akconf);
        akconf = NULL;
    }

    if (NULL != akcontex)
    {
        free(akcontex);
        akcontex = NULL;
    }

    return;
}

void ICACHE_FLASH_ATTR
hnt_airkiss_recv(u8 *data, u16 data_len)
{
    int ret;

    /* 7.把所有接收到的报文传给airkiss处理 */
    ret = airkiss_recv(akcontex, data, data_len);
    if (ret == AIRKISS_STATUS_CHANNEL_LOCKED)/* 8.已经锁定在了当前的信道，此时无需再轮询切换信道监听了 */
    {
        printf("stoped switch channel.\r\n");
        os_timer_disarm(&onbd_notify_timer);
        step = STEP_AIRKISS;   
        
        u8 *bssid;
        int fixed_channel;
        char scanned_ssid[50] = {0};
        bssid = ieee80211_get_BSSID((struct ieee80211_hdr *)data);
        printf("bssid:%02x:%02x:%02x:%02x:%02x:%02x.\r\n", bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
        memcpy(router_ap_bssid, bssid, 6);

		fixed_channel = promisc_get_fixed_channel(bssid, scanned_ssid);
		if (fixed_channel != 0) {
			log_debug("Broadcast force fixed to channel[%d]\r\n",fixed_channel);
			log_debug("Broadcast ssid scanned[%s]\r\n",scanned_ssid);
			wifi_set_channel(fixed_channel);
		}
        printf("lock:channel:%d!\r\n", wifi_get_channel());
    }
    else if (ret == AIRKISS_STATUS_COMPLETE)/* 9.已经接收到了所有的配置数据 */
    {
        printf("finish:channel:%d!\r\n", wifi_get_channel());

        step = STEP_DONE;
        wifi_promiscuous_enable(0);//??????,????
        wifi_set_promiscuous_rx_cb(NULL);
        airkiss_finish();
    }

    return;
}
#endif

int search_preamble1(uint8_t* da, uint8_t* sa)
{
    
    if(da[3] == PREAMBLE1_MAC2 && da[4] == PREAMBLE1_MAC1){
        memcpy(src_addr1, sa, MAC_LEN);
        data_len1 = da[5];
#ifdef ONBD_DEBUG
        printf("[%08d]", system_get_time());
#endif
        printf("hit preamble1\n");
        preamble1_count = 0;
        return STEP_PREAMBLE2;
    }
    if(++preamble1_count > PREAMBLE_SEARCH_LIMIT) {
        /* Can't find preamble1 in PREAMBLE_SEARCH_LIMIT packets,
               * switch to next Channel.
               */
#ifdef ONBD_DEBUG
        printf("can't find preamble1 in %d packets\n", PREAMBLE_SEARCH_LIMIT);
#endif
        preamble1_count = 0;
        return STEP_NEXT_CHAN;
    }
    
    return STEP_PREAMBLE1;
}

int search_preamble2(uint8_t* da, uint8_t* sa)
{   
    if(da[3] == PREAMBLE2_MAC2 && da[4] == PREAMBLE2_MAC1){

        memcpy(src_addr2, sa, MAC_LEN);
#ifdef ONBD_DEBUG
        printf("[%08d]", system_get_time());
#endif
        printf("hit preamble2\n");
        data_len2 = da[5];
        preamble2_count = 0;
        return STEP_CHECK;
    }
    
    if(++preamble2_count > PREAMBLE_SEARCH_LIMIT) {
        /* Can't find preamble2 in PREAMBLE_SEARCH_LIMIT packets,
               * switch to next Channel.
               */
#ifdef ONBD_DEBUG
        printf("can't find preamble2 in %d packets\n", PREAMBLE_SEARCH_LIMIT);
#endif
        preamble2_count = 0;
        return STEP_NEXT_CHAN;
    }
    
    return STEP_PREAMBLE2;
}

int sanity_check()
{
    if( memcmp(src_addr1, src_addr2, MAC_LEN) == 0
        && data_len1 == data_len2 && data_len1 < MAX_DATA_LEN) {
        data_len = data_len1;
        encrypt_data_num = 0;
#ifdef ONBD_DEBUG
        printf("[%08d]", system_get_time());
#endif
        printf("sanity check pass\n");
        return STEP_RECV_DATA;
    }
    
    return STEP_PREAMBLE1;
}

int resemble(uint8_t* da, uint8_t* sa)
{
    int index = da[4];
    
    if(da[3] != DATA_MAC2) {
        return STEP_RECV_DATA;
    }
    if(index >= data_len) {
        return STEP_RECV_DATA;
    }
    
    if(encrypt_num_str[index] == 0)
    {            
        encrypt_str[index] = da[5];
        encrypt_num_str[index] = encrypt_num_str[index] + 1;
        if(++encrypt_data_num != data_len)
            return STEP_RECV_DATA;            
    }
    else
    {
        encrypt_num_str[index] = encrypt_num_str[index] + 1;
        return STEP_RECV_DATA;            
    }
#ifdef ONBD_DEBUG
    /* DONE */
    printf("[%08d]", system_get_time());
#endif    
    printf("resemble done\n");
#ifdef ONBD_DEBUG
    int i = 0;
    for(i = 0; i < data_len; i++) {
        printf("encrypt_num_str[%d] = %d\n", i, encrypt_num_str[i]);
    }
#endif    

    return STEP_DECRYPT;
}

char* 
des_decrypt(uint8_t* des_data, int dlen)
{
    return (char*)des_data;
}


int 
decrypt()
{
    int i;
#ifdef ONBD_DEBUG
    printf("recv data=");
    for(i = 0; i < data_len; i++)
        printf("%02x ",  (unsigned char)encrypt_str[i]);
    printf("\n");
#endif

    for(i = 0; i < data_len; i++)
    {
        decrypt_str[i] = encrypt_str[i]^MAGIC_NUMBER;
    }

#ifdef ONBD_DEBUG
    printf("result=");
    for(i = 0; i < data_len; i++)
        printf("%02x ",  (unsigned char)decrypt_str[i]);
    printf("\n");
#endif    

    return STEP_SET_NETWORK;
}

int 
switch_to_next_chann(void *arg)
{
    uint8_t i;
    uint8_t channel;

    if(++switch_channel_times > CHANNEL_SWITCH_TOTAL_TIME/CHANNEL_SWITCH_TIME)
    {
        switch_channel_times = 0;
        wifi_promiscuous_enable(0);
        wifi_set_promiscuous_rx_cb(NULL);
        onbd_mgmt_smartconfig_stop();
        os_timer_disarm(&onbd_notify_timer);
        os_timer_setfn(&onbd_notify_timer, (os_timer_func_t *)onbd_mgmt_apconfig_start, NULL);
        os_timer_arm(&onbd_notify_timer, 10, 0);
        return STEP_START;
    }

    if(step >= STEP_CHECK)
        return step;
        
    if(need_scan_channel_num == 0)
        need_scan_channel_num = 1;
        
    curr_chan_index = (curr_chan_index+1)%need_scan_channel_num;
    channel = scan_chan_list[curr_chan_index];    

    //printf("Switch to channel %d\n", channel);
    wifi_set_channel(channel);
#ifdef ONBD_DEBUG
    for( i = 0; i < data_len; i++) {
        printf("encrypt_num_str[%d] = %d\n", i, encrypt_num_str[i]);
    }
#endif  
    
    return STEP_START;
}


int onbd_done()
{       
#ifdef ONBD_DEBUG
    printf("[%08d]", system_get_time());
#endif
    if(encrypt_str != NULL)
    {
        free(encrypt_str);
        encrypt_str = NULL;
    }
    if(decrypt_str != NULL)
    {
        free(decrypt_str);
        decrypt_str = NULL;
    }
    if(scan_chan_list != NULL)
    {
        free(scan_chan_list);
        scan_chan_list = NULL;
    }
    if(encrypt_num_str != NULL)
    {
        free(encrypt_num_str);
        encrypt_num_str = NULL;
    }

	struct router_info *info = NULL;
    while (!SLIST_EMPTY(&router_list)) {       /* List Deletion. */
        info = SLIST_FIRST(&router_list);
        SLIST_REMOVE_HEAD(&router_list, next);
        free(info);
    }

#ifdef CONFIG_AIRKISS_SUPPORT
    hnt_airkiss_stop();
#endif

    if(device_ap_ssid != NULL)
    {
        free(device_ap_ssid);
        device_ap_ssid = NULL;
    }

    return STEP_EXIT;
}

int str_isprint(const char *ssid)
{
    const char *tmp = ssid;

    while(*tmp != '\0')
    {
        if(*tmp < 0x20 || *tmp > 0x7e)
            return 0;

        tmp++;
    }

    return 1;
}

int
set_network()
{
    unsigned int info_crc32 = 0;
    unsigned char info_flag = 0;
    int info_hide_ssid = 0;
    int info_ssid_len = 0;
    char info_ssid[128] = {0};
    char info_pass[128] = {0};
    int info_offset = 0;
    char *info_p = decrypt_str;
    int i = 0;

    byte_to_int(info_crc32, decrypt_str[0], decrypt_str[1], decrypt_str[2], decrypt_str[3]);
    //byte_to_int(info_ip, decrypt_str[4], decrypt_str[5], decrypt_str[6], decrypt_str[7]);
    info_ip[0] = decrypt_str[4];
    info_ip[1] = decrypt_str[5];
    info_ip[2] = decrypt_str[6];
    info_ip[3] = decrypt_str[7];
    byte_to_short(info_port, decrypt_str[8], decrypt_str[9]);
    
    info_flag = decrypt_str[10];
#ifdef ONBD_DEBUG
    printf("info_crc32=%x, info_ip=%02x%02x%02x%02x, info_port=%x, info_flag = %x\n", 
    info_crc32, info_ip[0], info_ip[1], info_ip[2], info_ip[3], info_port, info_flag);
#endif
    char buf[512] = {0};
    int jid_len = strlen(g_hnt_factory_param.xmpp_jid);
    os_memcpy(buf, info_p + 4, data_len - 4);
#if SECURITY_DEVICE_SUPPORT    
    os_memcpy(buf+data_len-4, g_hnt_factory_param.xmpp_jid, jid_len);
#else
#if 0
    jid_len = 6;
    sprintf(buf+data_len-4, "%02x%02x%02x", g_hnt_factory_param.wlan_mac[3],
                                            g_hnt_factory_param.wlan_mac[4],
                                            g_hnt_factory_param.wlan_mac[5]);
#else
    jid_len = 0;
#endif
#endif
#ifdef ONBD_DEBUG
    printf("buf=");
    for(i = 0; i < data_len-4+jid_len; i++)
        printf("%02x ", (unsigned char)buf[i]);
    printf("\n");
#endif  
    if(info_crc32 != iks_crc32(0, buf, data_len-4+jid_len)) {
#ifdef ONBD_DEBUG
        printf(" crc check failed, restart\n");
#endif        
        wifi_promiscuous_enable(1);
        /* crc check failed, restart? */
        return STEP_START;
    }

    info_offset = 11;
    if(info_flag & 0x80) //hide ssid
    {
        info_hide_ssid = 1;
        info_ssid_len = info_flag & 0x7f;
        strncpy(info_ssid, info_p+info_offset, info_ssid_len);
        info_offset += info_ssid_len;
    }

    strncpy(info_pass, info_p+info_offset, data_len - info_offset);    
#ifdef ONBD_DEBUG
    printf("info_hide_ssid = %d\n", info_hide_ssid);
    printf("info_ssid = %s\n", info_ssid);
    printf("info_pass = %s\n", info_pass);
#endif    

    connect_to_ap(info_ssid, info_pass);
    os_timer_disarm(&onbd_notify_timer);

    return onbd_done();
}


int ICACHE_FLASH_ATTR
hnt_lock_channel(uint8_t* da)
{
    if(da[3] == PREAMBLE1_MAC2 && da[4] == PREAMBLE1_MAC1){
        printf("hit preamble1\n");
        data_len1 = da[5];
    }
    
    if(da[3] == PREAMBLE2_MAC2 && da[4] == PREAMBLE2_MAC1){
        printf("hit preamble2\n");
        data_len2 = da[5];
    }

    if((data_len1 != 0) && (data_len1 == data_len2) && data_len1 < MAX_DATA_LEN) {
       data_len = data_len1;
       printf("sanity check pass\n");
       return STEP_RECV_DATA;
    }

    return STEP_START;
}

int ICACHE_FLASH_ATTR
hnt_process_packet(struct ieee80211_hdr *hdr)
{
	u8 *sa = NULL;
	u8 *da = NULL;
	u8 *bssid = NULL;

	da = ieee80211_get_DA(hdr);
	if ((0x01 != da[0])||(0x00 != da[1])||(0x5e != da[2])){/*multicast ip Addr range:239.0~239.xx ||(0x76 <= DstMacAddr[3])*/
		return -1;
	}

	if ((DATA_MAC2 != da[3]) && (PREAMBLE1_MAC2 != da[3])){/*Sync Frame Must be 1:00:5e:00:64:xx*/
		return -1;
	}

	//printf("Multicast ADDR:%x:%x:%x:%x:%x:%x\n", da[0], da[1], da[2], da[3], da[4], da[5]);

	sa = ieee80211_get_SA(hdr);
	//printf("step=%d,src ADDR:%x:%x:%x:%x:%x:%x\n", step, sa[0], sa[1], sa[2], sa[3], sa[4], sa[5]);

	if (hnt_wifi_compare_mac_addr(sa)){
    switch(step) {
          case STEP_START:
              step = hnt_lock_channel(da);
              if(step == STEP_RECV_DATA)
              {
                      int fixed_channel;
                      char scanned_ssid[50] = {0};
                      os_memset(encrypt_str, 0, MAX_DATA_LEN);
                      os_memset(encrypt_num_str, 0, MAX_DATA_LEN);
                      
                      encrypt_data_num = 0;
                      
                      os_timer_disarm(&onbd_notify_timer);
                      printf("stoped switch channel:%d\r\n", wifi_get_channel());
                      bssid = ieee80211_get_BSSID((struct ieee80211_hdr *)hdr);
                      os_memcpy(router_ap_bssid, bssid, ETH_ALEN);

                      fixed_channel = promisc_get_fixed_channel(bssid, scanned_ssid);
                      if (fixed_channel != 0) {
                          log_debug("hnt force fixed to channel[%d]\r\n",fixed_channel);
                          log_debug("hnt ssid scanned[%s]\r\n",scanned_ssid);
                          wifi_set_channel(fixed_channel);
                      }
                      
#ifdef ONBD_DEBUG
                      printf("router_ap_bssid:%02x:%02x:%02x:%02x:%02x:%02x\n", (unsigned char)router_ap_bssid[0],
                                          (unsigned char)router_ap_bssid[1],
                                          (unsigned char)router_ap_bssid[2],
                                          (unsigned char)router_ap_bssid[3],
                                          (unsigned char)router_ap_bssid[4],
                                          (unsigned char)router_ap_bssid[5]);  
#endif
              }
              break;
          case STEP_RECV_DATA:
              step = resemble(da, sa);
              break;   
          case STEP_DECRYPT:
              step = decrypt();
              break;
          case STEP_SET_NETWORK:
              wifi_promiscuous_enable(0);
              step = set_network();
              break;
          case STEP_DONE:
              step = onbd_done();
          default:
              break;
        }
    }
}


void ICACHE_FLASH_ATTR
processPacket(uint8* buf, uint16 len)
{
    uint8_t *da, *sa, *bssid;
    
    da = buf + 16;
    sa = buf + 10;
    bssid = buf + 4;

    switch(step) {
      case STEP_START:
      case STEP_PREAMBLE1:
          os_memset(encrypt_str, 0, MAX_DATA_LEN);
          os_memset(encrypt_num_str, 0, MAX_DATA_LEN);

          data_len = 0;
          step = search_preamble1(da, sa);
          break;
      case STEP_PREAMBLE2:
          step = search_preamble2(da, sa);
          os_memcpy(router_ap_bssid, bssid, 6);
#ifdef ONBD_DEBUG
          printf("router_ap_bssid:%02x:%02x:%02x:%02x:%02x:%02x\n", (unsigned char)router_ap_bssid[0],
                (unsigned char)router_ap_bssid[1],
                (unsigned char)router_ap_bssid[2],
                (unsigned char)router_ap_bssid[3],
                (unsigned char)router_ap_bssid[4],
                (unsigned char)router_ap_bssid[5]);  
#endif
          break;
      case STEP_CHECK:
          step = sanity_check();
          break;
      case STEP_RECV_DATA:
          step = resemble(da, sa);
          break;   
      case STEP_DECRYPT:
          step = decrypt();
          break;
      case STEP_NEXT_CHAN:
          step = switch_to_next_chann(NULL);
          break; 
      case STEP_SET_NETWORK:
          wifi_promiscuous_enable(0);
          step = set_network();
          break;
      case STEP_DONE:
          step = onbd_done();
      default:
          break;
    }        
}



void ICACHE_FLASH_ATTR
on_recv_wifi_promiscuous_pkt(uint8* buf, uint16 len)
{
	struct router_info *info = NULL;  
	struct sniffer_buf * sniffer = (struct sniffer_buf *)buf;
    uint16_t buf_len = 0;
    uint16_t cnt = 0;
    int pkt_len = 0;

    if(len == 12)
        return;
    
    buf += 12;    
    struct probe_request_80211 *probe_buf = (struct probe_request_80211*)buf;	
    if (FRAME_TYPE_MANAGEMENT == probe_buf->framectrl.Type) 
    {    	/* Management frame */    	
        if (FRAME_SUBTYPE_PROBE_REQUEST == probe_buf->framectrl.Subtype) 
        {              		/* Probe Request */    		
            ptagged_parameter tag = (ptagged_parameter)(buf + sizeof(probe_request));    		
            if (tag->tag_length != 0)    		
            {	    		
                uint8_t ssid_buff[32];	    		
                memset(ssid_buff, 0, 32);	    		
                memcpy(ssid_buff, (uint8_t *)tag + 2, tag->tag_length);
                if(strncmp(ssid_buff, device_ap_ssid, tag->tag_length) == 0)
                {
                    //printf("ssid_buff:%s\n", ssid_buff);
                    wifi_promiscuous_enable(0);
                    wifi_set_promiscuous_rx_cb(NULL);
                    onbd_mgmt_smartconfig_stop();
                    os_timer_disarm(&onbd_notify_timer);
                    os_timer_setfn(&onbd_notify_timer, (os_timer_func_t *)onbd_mgmt_apconfig_start, NULL);
                    os_timer_arm(&onbd_notify_timer, 10, 0);
                    return ;
                }
            }
        }
    }
#ifdef CONFIG_AIRKISS_SUPPORT
    if(step >= STEP_DONE)
        return;
    
    if(sniffer->rx_ctrl.sig_mode != 0)
    {
        pkt_len = sniffer->rx_ctrl.HT_length;
    }
    else
    {
        pkt_len = sniffer->rx_ctrl.legacy_length;
    }
    
    hnt_airkiss_recv(buf, pkt_len);
    
    if(step >= STEP_AIRKISS)
        return;
#endif

    hnt_process_packet((struct ieee80211_hdr *)buf);
}


void onboarding_start(void)
{
    struct station_config sta_conf; 
    wifi_station_get_config(&sta_conf);
    
    printf("AP SSID:%s, PWD:%s\n", sta_conf.ssid, sta_conf.password);
    if(strlen(sta_conf.ssid) == 0 && strlen(sta_conf.password) == 0) 
    {
        wifi_set_channel(scan_chan_list[0]);
        curr_chan_index = 0;
        os_timer_setfn(&onbd_notify_timer, (os_timer_func_t*)switch_to_next_chann, NULL);
        os_timer_arm(&onbd_notify_timer, CHANNEL_SWITCH_TIME, 1);
        wifi_promiscuous_enable(1);
        wifi_set_promiscuous_rx_cb(on_recv_wifi_promiscuous_pkt);
    }
}

/**********************************************************************************
udp send/recv handler
***********************************************************************************/
void ICACHE_FLASH_ATTR
onbd_get_device_info(void)
{
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
    char *buffer ;
    char *str = NULL;
    unsigned int len = 0;

    if(modelDescription == NULL)
    {
        buffer = (char *)malloc(MODEL_DESC_LEN);
        modelDescription = (char *)malloc(MODEL_DESC_LEN);
        
        memset(modelDescription, 0, MODEL_DESC_LEN); 
        /*"{\"jid\":\"13666666669@net\", \"devicetype\":\"2\"}"*/
        
        len = snprintf(buffer, MODEL_DESC_LEN, "{\"jid\":\"%s", g_hnt_factory_param.xmpp_jid);
        len += snprintf(buffer + len, MODEL_DESC_LEN - len, "\", \"devicetype\":\"%d\"", g_hnt_factory_param.device_type);
#ifndef SECURITY_DEVICE_SUPPORT
        len += snprintf(buffer + len, MODEL_DESC_LEN - len, ", \"devid\":\"%s\"", g_hnt_factory_param.device_id);
        len += snprintf(buffer + len, MODEL_DESC_LEN - len, ", \"devpass\":\"%s\"", g_hnt_factory_param.password);
#endif
        len += snprintf(buffer + len, MODEL_DESC_LEN - len, ", \"mac\":\""MACSTR"\"", MAC2STR(g_hnt_factory_param.wlan_mac));
        len += snprintf(buffer + len, MODEL_DESC_LEN - len, "}");
        log_debug("buffer = %s\n", buffer);
        str = iks_base64_encode(buffer, len);
        snprintf(modelDescription, MODEL_DESC_LEN, "%s", str);
        iks_free(str);    
        log_debug("modelDescription = %s\n", modelDescription);
        free(buffer);
    }
}

static void 
onbd_udp_stop(void)
{
    if(modelDescription != NULL)
    {
        free(modelDescription);
        modelDescription = NULL;
    }
}

int ICACHE_FLASH_ATTR
onbd_udp_send(int sock, struct sockaddr_in *from)
{
    char *bufr = NULL;
    int ret = 0;
    int len = 0;
    
    onbd_get_device_info();
    
    len = strlen(modelDescription);
    bufr = (char *)malloc(MODEL_DESC_LEN);
    bufr[0] = 0x01;
    bufr[1] = len;
    snprintf(bufr+2, MODEL_DESC_LEN - 2, "%s", modelDescription);

    ret = sendto(sock, bufr, len+2, 0, (struct sockaddr *)from, sizeof(struct sockaddr));
    log_debug("ret = %d\n", ret);
    free(bufr);

    return ret;
}

void ICACHE_FLASH_ATTR
wifi_scan_done(void *arg, STATUS status)
{
    uint8 i, j;

    if(encrypt_str == NULL)
        encrypt_str = (char *)os_malloc(MAX_DATA_LEN);

    if(encrypt_num_str == NULL)
        encrypt_num_str = (char *)os_malloc(MAX_DATA_LEN);

    if(decrypt_str == NULL)
        decrypt_str = (char *)os_malloc(MAX_DATA_LEN);

    if(scan_chan_list == NULL)
        scan_chan_list = (uint8_t *)os_malloc(MAX_CHAN_NUM);
    
    os_memset(encrypt_str, 0, MAX_DATA_LEN);
    os_memset(encrypt_num_str, 0, MAX_DATA_LEN);
    os_memset(decrypt_str, 0, MAX_DATA_LEN);
    os_memset(scan_chan_list, 0, MAX_CHAN_NUM);
    
    os_memset(gHntSrcMac, 0, ETH_ALEN);
    need_scan_channel_num = 0;
    curr_chan_index = 0;
    
	SLIST_INIT(&router_list);
	if (status == OK) {
		struct bss_info *bss = (struct bss_info *)arg;

		while (bss != NULL) {
            printf(MACSTR" ssid:%s, channel:%d, rssi:%d\n", MAC2STR(bss->bssid), bss->ssid, bss->channel, bss->rssi);
			if (bss->channel != 0 && bss->rssi > -70) {
                 for (j = 0; (j <= need_scan_channel_num) && (j < MAX_CHAN_NUM); j++)
                 {                        
                     if (scan_chan_list[j] == 0)
                     {
                         scan_chan_list[j] = bss->channel;
                         need_scan_channel_num++;
                         break;
                     }
                         
                     if (scan_chan_list[j] == bss->channel)
                     {
                         break;
                     }
                }
                
				struct router_info *info = NULL;
				info = (struct router_info *)os_zalloc(sizeof(struct router_info));
				info->authmode = bss->authmode;
				info->channel = bss->channel;
				os_memcpy(info->bssid, bss->bssid, 6);
				os_memcpy(info->ssid, bss->ssid, 32);
                
				SLIST_INSERT_HEAD(&router_list, info, next);
			}
			bss = STAILQ_NEXT(bss, next);
		}

        for(i = 0; i < MAX_CHAN_NUM; i++) {
            if(scan_chan_list[i] != 0) {
                os_printf("chan[%d]:%d\n", i, scan_chan_list[i]);
            }
        }

        onboarding_start();
	} else {
		os_printf("err, scan status %d\n", status);
	}
}


void ICACHE_FLASH_ATTR
system_init_done(void)
{
    log_debug("system_get_free_heap_size = %d\n", system_get_free_heap_size());

    struct station_config sta_conf;
    wifi_station_get_config(&sta_conf);
    if(strlen(sta_conf.ssid) == 0 && strlen(sta_conf.password) == 0) 
    {
        struct scan_config config;
        memset(&config, 0, sizeof(config));
        config.show_hidden = 1;
        wifi_station_scan(&config, wifi_scan_done);
    }
}


/******************************
direct config onbd code here 
********************************/
LOCAL void ICACHE_FLASH_ATTR
aponbd_set_router_bssid(char *router_bssid_str)
{
    int i;
    
    for(i=0;i<6;i++)
    {
      router_ap_bssid[i] = strtol(router_bssid_str, &router_bssid_str,16);
      router_bssid_str += 1;
    }
}

LOCAL void ICACHE_FLASH_ATTR
aponbd_connect_router_ap_cb(void)
{
#define BUF_LEN_64  64
    char *info_ssid = malloc(BUF_LEN_64); 
    char *info_pwd = malloc(BUF_LEN_64);
    char *info_bssid = malloc(BUF_LEN_64); 

    memset(info_ssid, 0, BUF_LEN_64);
    memset(info_pwd, 0, BUF_LEN_64);
    memset(info_bssid, 0, BUF_LEN_64);
    json_get_value(decrypt_str, "\"ssid", info_ssid, BUF_LEN_64 - 1);
    json_get_value(decrypt_str, "pwd", info_pwd, BUF_LEN_64 - 1);
    json_get_value(decrypt_str, "\"bssid", info_bssid, BUF_LEN_64 - 1);
    aponbd_set_router_bssid(info_bssid);

    connect_to_ap(info_ssid, info_pwd);
    onbd_udp_notify_times = APONBD_UDP_TIMES;

    free(info_ssid);
    free(info_pwd);
    free(info_bssid);
}

void ICACHE_FLASH_ATTR
aponbd_wifi_scan_done(void *arg, STATUS status)
{
    uint8 i, j;
    
	SLIST_INIT(&router_list);
	if (status == OK) {
		struct bss_info *bss = (struct bss_info *)arg;

		while (bss != NULL) {
            //printf(MACSTR" ssid:%s, channel:%d, rssi:%d\n", MAC2STR(bss->bssid), bss->ssid, bss->channel, bss->rssi);
			if (bss->channel != 0) {              
				struct router_info *info = NULL;
				info = (struct router_info *)os_zalloc(sizeof(struct router_info));
				info->authmode = bss->authmode;
				info->channel = bss->channel;
				os_memcpy(info->bssid, bss->bssid, 6);
				os_memcpy(info->ssid, bss->ssid, 32);
                
				SLIST_INSERT_HEAD(&router_list, info, next);
			}
			bss = STAILQ_NEXT(bss, next);
		}
		aponbd_connect_router_ap_cb();
	} else {
		os_printf("err, scan status %d\n", status);
	}
}


LOCAL void ICACHE_FLASH_ATTR
aponbd_set_status(uint8_t status)
{
    step = status;
}

LOCAL int ICACHE_FLASH_ATTR
aponbd_get_info_ipport(void)
{
    char info_ipport[32] = {0};
    char *preInfoport = NULL;
    char *curInfoport = NULL;
    char tempStr[32] = {0};

    json_get_value(decrypt_str, "ipport", info_ipport, sizeof(info_ipport) - 1);
    log_debug("info_ipport = %s\n", info_ipport);

    preInfoport = info_ipport;
    memset(tempStr, 0, sizeof(tempStr));
    strcpy(tempStr, preInfoport);
    curInfoport = strchr(preInfoport, '.');
    if(curInfoport != NULL)
    {
        tempStr[curInfoport - preInfoport] = '\0';
        info_ip[0] = atoi(tempStr);
    }
    else
    {
        return -1;
    }

    preInfoport = curInfoport+1;
    memset(tempStr, 0, sizeof(tempStr));
    strcpy(tempStr, preInfoport);
    curInfoport = strchr(preInfoport, '.');
    if(curInfoport != NULL)
    {
        tempStr[curInfoport - preInfoport] = '\0';
        info_ip[1] = atoi(tempStr);
    }
    else
    {
        return -1;
    }

    preInfoport = curInfoport+1;
    memset(tempStr, 0, sizeof(tempStr));
    strcpy(tempStr, preInfoport);
    curInfoport = strchr(preInfoport, '.');
    if(curInfoport != NULL)
    {
        tempStr[curInfoport - preInfoport] = '\0';
        info_ip[2] = atoi(tempStr);
    }
    else
    {
        return -1;
    }
    
    preInfoport = curInfoport+1;
    memset(tempStr, 0, sizeof(tempStr));
    strcpy(tempStr, preInfoport);
    curInfoport = strchr(preInfoport, ':');
    if(curInfoport != NULL)
    {
        tempStr[curInfoport - preInfoport] = '\0';
        info_ip[3] = atoi(tempStr);
    }
    else
    {
        return -1;
    }

    preInfoport = curInfoport+1;
    strcpy(tempStr, preInfoport);
    info_port = atoi(tempStr); 

    return 0;
}


static void
aponbd_finish(void)
{
    log_debug("test\n");
    if(decrypt_str != NULL)
    {
        free(decrypt_str);
        decrypt_str = NULL;
    }
    
    if(device_ap_ssid != NULL)
        free(device_ap_ssid);
}

#if 0
static int aponbd_sta_check_cb(void)
{
#define STA_MAC_BUF_LEN  64
    u8 *sta_buf;
    u32 sta_num;
    int ret = 0;

    if(step < STEP_DONE)
    {
        sta_buf = malloc(STA_MAC_BUF_LEN);
        memset(sta_buf, 0, STA_MAC_BUF_LEN);
        tls_wifi_get_authed_sta_info(&sta_num, sta_buf, STA_MAC_BUF_LEN);
        if(sta_num > 0)
        {
            log_debug("test\n");
            tls_wifi_set_listen_mode(0);  
            tls_wifi_data_recv_cb_register(NULL);
            tls_wifi_scan_result_cb_register(NULL);
            onbd_mgmt_smartconfig_stop();
            
            tls_wifi_disconnect();
            aponbd_ap_init();
            aponbd_set_status(STEP_APONBD_START);
            tls_os_queue_send(net_task_q, (void *)MSG_MGMT_UDP, 0);

            ret = 1;
        }        
        free(sta_buf);
    }

    return ret;
}
#endif
static void 
aponbd_sta_mode(void *arg)
{
    log_debug("test\n");

    aponbd_get_info_ipport();

    aponbd_connect_router_ap_cb();
}


static void 
onbd_mgmt_udp_send_search_ack(int sudp, struct sockaddr_in *from)
{ 
    struct sockaddr_in sin;
    uint8_t broadcast_ip[4] = {0xff, 0xff, 0xff, 0xff};

    onbd_udp_send(sudp, from);

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = *((u32_t*)broadcast_ip);
    sin.sin_port= from->sin_port;   

    onbd_udp_send(sudp, &sin);
}
static int
onbd_mgmt_udp_sent_config_info_ack(int sudp, struct sockaddr_in *from)
{
    char bufr[3];
    int ret;
    int i;
    
    bufr[0] = 0x02;
    bufr[1] = 0x01;
    bufr[2] = 0x00;

    for(i = 0; i < APONBD_SENT_ACK1_TIMES; i++)
    {
        ret = sendto(sudp, bufr, bufr[1]+2, 0, (struct sockaddr *)from, sizeof(struct sockaddr));
		vTaskDelay(20 / portTICK_RATE_MS);	 // 100 ms
    }

    return ret;
}

void onbd_mgmt_udp_handler_stop(void)
{
    onbd_mgmt_udp_abort = 1;
}

static void onbd_mgmt_udp_recv_cb(int sudp, char *pusrdata, int length, struct sockaddr_in *from)
{
    log_debug("RECV:length=%d\n", length);
    log_debug("client %s:%d\n", 
            inet_ntoa(from->sin_addr), ntohs(from->sin_port));
    int ret;
    
    if (pusrdata == NULL) {
        return;
    }
    log_debug("RECV:BUF=%s\n", pusrdata);

    uint8_t msg_type = pusrdata[0];
    uint8_t msg_len = pusrdata[1];
    uint8_t msg;
    char *decode_str;
    log_debug("RECV:msg_type=%d:%d\n", pusrdata[0], pusrdata[1]);

    log_debug("msg_type = %d\n", msg_type);
    
    if(ONBD_MGMT_UDP_MSG_TYPE_CONFIG_INFO == msg_type) /* configure wifi ssid/pwd */  
    {
        log_debug("test\n");
        if(wifi_get_opmode() != SOFTAP_MODE)
            return;
            
        ret = onbd_mgmt_udp_sent_config_info_ack(sudp, from);

        log_debug("ret = %d\n", ret);
        if((ret > 0) && (decrypt_str == NULL))
        {
            char *encode_str = (char *)malloc(msg_len + 1);
            MEMCPY(encode_str, &pusrdata[2], msg_len);    
            decrypt_str = iks_base64_decode (encode_str);
            log_debug("recv str:%s\n", decrypt_str);
            free(encode_str);
            aponbd_sta_mode(NULL);
            onbd_mgmt_udp_handler_stop();
        }
    }
    else if(ONBD_MGMT_UDP_MSG_TYPE_SEARCH == msg_type) /* search device */  
    {/* search device */     
        log_debug("test\n");
        if(wifi_get_opmode() != STATION_MODE)
            return;
            
        log_debug("test\n");
        onbd_mgmt_udp_send_search_ack(sudp, from);       
    }
    else if(ONBD_MGMT_UDP_MSG_TYPE_ACK == msg_type) /* onbd udp ack */ 
    {/* onbd udp ack */       
        log_debug("test\n");
        if(wifi_get_opmode() != STATION_MODE)
            return;

        log_debug("test\n");
        
        onbd_mgmt_udp_handler_stop();
        hnt_onbd_stop();
    }
}

static int aponbd_ap_init(void)
{
    struct hnt_mgmt_factory_param factory_info;
    hnt_mgmt_get_factory_info(&factory_info);    
    wifi_set_opmode(STATIONAP_MODE);

    wifi_softap_dhcps_stop(); 

    struct ip_info ap_info;
    IP4_ADDR(&ap_info.ip, 192, 168, 169, 1);
    IP4_ADDR(&ap_info.netmask, 255, 255, 255, 0);
    IP4_ADDR(&ap_info.gw, 192, 168, 169, 1);

    if ( true != wifi_set_ip_info(SOFTAP_IF, &ap_info)) {
            os_printf("set default ip wrong\n");
    }

    wifi_softap_dhcps_start();

    struct softap_config config;
    wifi_softap_get_config(&config);
    memset(config.ssid, 0, sizeof(config.ssid));
    sprintf(config.ssid, "Seaing_%d_%02x%02x%02x", g_hnt_factory_param.device_type,
    factory_info.wlan_mac[3], factory_info.wlan_mac[4],factory_info.wlan_mac[5]);

    config.ssid_len = strlen(config.ssid);
    config.authmode = AUTH_OPEN;
    device_ap_ssid = (char *)malloc(config.ssid_len);
    memcpy(device_ap_ssid, config.ssid, config.ssid_len);

    wifi_softap_set_config(&config);
//    wifi_set_event_handler_cb(wifi_handle_event_cb);
}


void ICACHE_FLASH_ATTR
onbd_mgmt_config_stop(void)
{
    os_timer_disarm(&onbd_notify_timer);
}

void 
onbd_mgmt_smartconfig_finish(void)
{
    onbd_mgmt_config_stop();
    onbd_done();
}

void 
onbd_mgmt_smartconfig_stop(void)
{
    log_debug("test\n");
    if(encrypt_str != NULL)
        free(encrypt_str);
    if(decrypt_str != NULL)
        free(decrypt_str);
    if(scan_chan_list != NULL)
        free(scan_chan_list);
    if(encrypt_num_str != NULL)
        free(encrypt_num_str);

    encrypt_str = NULL;
    decrypt_str = NULL;
    scan_chan_list = NULL;
    encrypt_num_str = NULL;

    #if 0
	struct router_info *info = NULL;
    while (!SLIST_EMPTY(&router_list)) {       /* List Deletion. */
        info = SLIST_FIRST(&router_list);
        SLIST_REMOVE_HEAD(&router_list, next);
        free(info);
    }
    #endif
}


static int sudp = -1;

void onbd_mgmt_active_udp_send(int sock)
{
    struct sockaddr_in sin;
    uint8_t broadcast_ip[4] = {0xff, 0xff, 0xff, 0xff};
    
    if(onbd_udp_notify_times-- > 0)
    {        
        if((info_ip[0] == 0) && (info_ip[1] == 0) && (info_ip[2] == 0))
        {
            return;
        }
        memset(&sin, 0, sizeof(struct sockaddr));
        sin.sin_family=AF_INET;
        sin.sin_addr.s_addr=*((u32_t*)info_ip);
        sin.sin_port=htons(info_port);   
        printf("send to %d.%d.%d.%d:%d\n", info_ip[0], 
            info_ip[1], info_ip[2], info_ip[3], info_port);
        onbd_udp_send(sock, &sin);
        
        
        sin.sin_family=AF_INET;
        sin.sin_addr.s_addr=*((u32_t*)broadcast_ip);
        sin.sin_port=htons(info_port);   
        
        onbd_udp_send(sock, &sin);
    }
    else
    {        
        hnt_mgmt_system_factory_reset();    
    }    
}
#if 0
void onbd_mgmt_udp_set_ip(void)
{
    if((info_ip[0] == 0) && (info_ip[1] == 0) && (info_ip[2] == 0))
    {    
        struct tls_ethif * ethif = tls_netif_get_ethif();
        printf("ip=%d.%d.%d.%d\n",ip4_addr1(&ethif->ip_addr.addr),ip4_addr2(&ethif->ip_addr.addr),
            ip4_addr3(&ethif->ip_addr.addr),ip4_addr4(&ethif->ip_addr.addr));
        info_ip[0] = ip4_addr1(&ethif->ip_addr.addr);
        info_ip[1] = ip4_addr2(&ethif->ip_addr.addr);
        info_ip[2] = ip4_addr3(&ethif->ip_addr.addr);
    }
}
#endif
void onbd_mgmt_udp_handler_start(void)
{
#define ONDB_RECV_PKT_SIZE  256
    int n, fromlen;
    struct sockaddr_in from;
    fd_set read_set;
    struct timeval tv;
    int ret;
    char *recvpacket = NULL;
    log_debug("test\n");

    if(sudp < 0)
    {
        return;
    }


    onbd_mgmt_udp_abort = 0;
    recvpacket = (char *)malloc(ONDB_RECV_PKT_SIZE);      
    
    for(;;)
    {
        if(onbd_mgmt_udp_abort)
        {
            log_debug("test\n");
            break;
        }   

        if(sudp < 0)
        {
            break;
        }
        
        FD_ZERO(&read_set);
        FD_SET(sudp, &read_set);
        tv.tv_sec  = 1;
        tv.tv_usec = 0;
        
        ret = select(sudp + 1, &read_set, NULL, NULL, &tv);
        if (ret > 0)
        {
            if (FD_ISSET(sudp, &read_set))
            {
                fromlen=sizeof(from);
                memset(recvpacket, 0, ONDB_RECV_PKT_SIZE);
                n = recvfrom(sudp, recvpacket, ONDB_RECV_PKT_SIZE, 0, (struct sockaddr *)&from, (socklen_t *)&fromlen);
                if(n < 0)
                {
                    //printf("%d: recvfrom error\r\n", received_cnt + 1);
                    log_debug("test\n");
                }

                log_debug("buffer:%s\n", recvpacket);
                onbd_mgmt_udp_recv_cb(sudp, recvpacket, n, &from);

                FD_CLR(sudp, &read_set);
            }
            else
            {
                log_debug("test\n");
            }
        }
        
        onbd_mgmt_active_udp_send(sudp);
    }
    log_debug("test\n");

    free(recvpacket);
}

#if 0
void onbd_mgmt_sta_check(void)
{
    struct timeval tv;
    onbd_mgmt_udp_abort = 0;
    
    for(;;)
    {
        if(onbd_mgmt_udp_abort)
        {
            log_debug("test\n");
            break;
        }   

        tv.tv_sec  = 0;
        tv.tv_usec = 100000;
        
        if(aponbd_sta_check_cb() == 1)
            break;

        select(0, NULL, NULL, NULL, &tv);
    }
}
#endif

void onbd_mgmt_udp_init(void)
{
    struct sockaddr_in addr;

    sudp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    log_debug("\nstd sk one shot sock num=%d\n",sudp);    

    addr.sin_family = AF_INET;         // host byte order
    log_debug("test\n");
    addr.sin_port = htons(41200);     // short, network byte order
    log_debug("test\n");
    addr.sin_addr.s_addr = ((u32_t)0x00000000UL); // automatically fill with my IP
    memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));
    log_debug("test\n");

	if(bind(sudp, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        log_debug("test\n");
		closesocket(sudp);
		sudp = -1;
		return;
	}    

	onbd_get_device_info();
}

void onbd_mgmt_udp_stop(void)
{   
    if(sudp >= 0)
    {
        closesocket(sudp);
        sudp = -1;
    }
}

void
hnt_onbd_stop(void)
{
    log_debug("test\n");
    aponbd_finish();
    onbd_mgmt_smartconfig_stop();
    onbd_mgmt_udp_stop();
    onbd_udp_stop();
    onbd_done();
}

void 
hnt_onbd_start(void)
{
    log_debug("test\n");
#ifdef CONFIG_AIRKISS_SUPPORT
    hnt_airkiss_start();
#endif
    system_init_done();
        
    aponbd_ap_init();

    onbd_mgmt_udp_init();
    //onbd_mgmt_sta_check();
}
#endif

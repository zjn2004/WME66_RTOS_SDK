/*
 * Copyright (c) 2014 Seaing, Ltd.
 * All Rights Reserved. 
 * Seaing Confidential and Proprietary.
 * 
 * FILENAME: onbd.h
 * 
 * DESCRIPTION: device onboarding process
 * 
 * Date Created: 2015/04/08
 * 
 * Author: Yi Aiguo (yiaiguo@seaing.net)
 */
#ifndef _ONBD_H
#define _ONBD_H


#define SECURITY_DEVICE_SUPPORT 0

#define MODEL_DESC_LEN                  256
#define NOTIFY_INTERVAL_SECONDS         1
#define ONBD_UDP_TIMES     50
#define APONBD_UDP_TIMES     180
#define APONBD_SENT_ACK1_TIMES  5

enum {
    ONBD_MGMT_UDP_MSG_TYPE_DEVICE_INFO      = 0x1,
    ONBD_MGMT_UDP_MSG_TYPE_ACK              = 0x2,  
    ONBD_MGMT_UDP_MSG_TYPE_CONFIG_INFO      = 0x3,  
    ONBD_MGMT_UDP_MSG_TYPE_SEARCH           = 0x5,  
    ONBD_MGMT_UDP_MSG_TYPE_DISCOVER         = 0x7,
    ONBD_MGMT_UDP_MSG_TYPE_ATCMD_DEBUG      = 0xFD,
    ONBD_MGMT_UDP_MSG_TYPE_ERROR            = 0xFF,
};

#define FRAME_TYPE_MANAGEMENT 0
#define FRAME_TYPE_CONTROL 1
#define FRAME_TYPE_DATA 2
#define FRAME_SUBTYPE_PROBE_REQUEST 0x04
#define FRAME_SUBTYPE_PROBE_RESPONSE 0x05
#define FRAME_SUBTYPE_BEACON 0x08
#define FRAME_SUBTYPE_AUTH 0x0b
#define FRAME_SUBTYPE_DEAUTH 0x0c
#define FRAME_SUBTYPE_DATA 0x14

#define CHANNEL_SWITCH_TIME   100      /*100ms*/
#define CHANNEL_SWITCH_TOTAL_TIME   10000      /*10s*/

#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif

/*
 * | 0x01 | 0x00 | 0x5e | mac2 | mac1 | mac0 |
 * Preamble Packet:
 *  1.mac2 = 0x55, mac1=0x33, mac0=data_len
 *  2.mac2 = 0x55, mac1=0x44, mac0=data_len
 * Data Packet:
 *  mac2 = 0x33, mac1=index, mac0=data
 */
#define PREAMBLE1_MAC2 55
#define PREAMBLE1_MAC1 33
#define PREAMBLE2_MAC2 55
#define PREAMBLE2_MAC1 44
#define DATA_MAC2      33

#define MAC_LEN 6
#define MAX_DATA_LEN 256
#define MAX_CHAN_NUM 14
//#define WAIT_TIME_MS 500
#define PREAMBLE_SEARCH_LIMIT 30
#define MAGIC_NUMBER 0x55
#define MCAST_SEARCH_LIMIT 100

enum {
    STEP_START = 0,  /* start sniffer */
    STEP_PREAMBLE1,  /* search Preamble1 */
    STEP_PREAMBLE2,  /* search Preamble2 */
    STEP_CHECK,      /* compare src addr and data len */
    STEP_NEXT_CHAN,  /* switch to next channel */
    STEP_RECV_DATA,  /* resemble data */
    STEP_DECRYPT,    /* decrypt data */
    STEP_SET_NETWORK,/* config wifi network */
    STEP_AIRKISS,  /* airkiss flag */
    STEP_APONBD_START,
    STEP_APONBD_STATUS_ROUTER_SETTING_ACK,
    STEP_UDP_SEND,
    STEP_DONE,
    STEP_EXIT,
};

/*ap onbd code here */
enum {
    APONBD_STATUS_INIT = STEP_EXIT + 1,
    APONBD_STATUS_START,
    APONBD_STATUS_STA_CONN,
    APONBD_STATUS_ROUTER_SETTING,
    APONBD_STATUS_ROUTER_SETTING_ACK,
    APONBD_STATUS_SEARCH,
    APONBD_STATUS_SEARCH_ACK,
    STEP_ONBD_UDP_SEND,
};


#define MODEL_DESC_LEN                  256
#define NOTIFY_INTERVAL_SECONDS         1
#define ONBD_UDP_TIMES     50
#define APONBD_UDP_TIMES     180
#define APONBD_SENT_ACK1_TIMES  5

#define CHANNEL_AP_TOTAL_TIME   500      /*5s*/

#define IEEE80211_FCTL_TODS         0x0100
#define IEEE80211_FCTL_FROMDS		0x0200
struct ieee80211_hdr {
    u16 frame_control;
    u16 duration_id;
    u8 addr1[6];
    u8 addr2[6];
    u8 addr3[6];
    u16 seq_ctrl;
    u8 addr4[6];
} ;

struct RxControl {
    signed rssi:8;
    unsigned rate:4;
    unsigned is_group:1;
    unsigned:1;
    unsigned sig_mode:2;
    unsigned legacy_length:12;
    unsigned damatch0:1;
    unsigned damatch1:1;
    unsigned bssidmatch0:1;
    unsigned bssidmatch1:1;
    unsigned MCS:7;
    unsigned CWB:1;
    unsigned HT_length:16;
    unsigned Smoothing:1;
    unsigned Not_Sounding:1;
    unsigned:1;
    unsigned Aggregation:1;
    unsigned STBC:2;
    unsigned FEC_CODING:1;
    unsigned SGI:1;
    unsigned rxend_state:8;
    unsigned ampdu_cnt:8;
    unsigned channel:4;
    unsigned:12;
};
 
struct Ampdu_Info
{
  uint16 length;
  uint16 seq;
  uint8  address3[6];
};

struct sniffer_buf {
    struct RxControl rx_ctrl;
    uint8_t  buf[36];
    uint16_t cnt;
    struct Ampdu_Info ampdu_info[1];
};

typedef struct framectrl_80211{    
    //buf[0]    
    u8 Protocol:2;    
    u8 Type:2;    
    u8 Subtype:4;    
    //buf[1]    
    u8 ToDS:1;    
    u8 FromDS:1;    
    u8 MoreFlag:1;    
    u8 Retry:1;    
    u8 PwrMgmt:1;    
    u8 MoreData:1;   
    u8 Protectedframe:1;    
    u8 Order:1;
} framectrl_80211,*lpframectrl_80211;

typedef struct probe_request_80211{	
struct framectrl_80211 framectrl;	
    uint16 duration;	
    uint8 rdaddr[6];	
    uint8 tsaddr[6];	
    uint8 bssid[6];	
    uint16 number;
} probe_request, *pprobe_request;

typedef struct tagged_parameter{	
    /* SSID parameter */	
    uint8 tag_number;	
    uint8 tag_length;
} tagged_parameter, *ptagged_parameter;

static int ICACHE_FLASH_ATTR
ieee80211_has_tods(u16 fc)
{
	return (fc & IEEE80211_FCTL_TODS) != 0;
}
/**
 * ieee80211_has_a4 - check if IEEE80211_FCTL_TODS and IEEE80211_FCTL_FROMDS are set
 * @fc: frame control bytes in little-endian byteorder
 */
static int ICACHE_FLASH_ATTR
ieee80211_has_a4(u16 fc)
{
	u16 tmp = IEEE80211_FCTL_TODS | IEEE80211_FCTL_FROMDS;
	return (fc & tmp) == tmp;
}

/**
 * ieee80211_has_fromds - check if IEEE80211_FCTL_FROMDS is set
 * @fc: frame control bytes in little-endian byteorder
 */
static int ICACHE_FLASH_ATTR
ieee80211_has_fromds(u16 fc)
{
	return (fc & IEEE80211_FCTL_FROMDS) != 0;
}

static u8 *ICACHE_FLASH_ATTR
ieee80211_get_BSSID(struct ieee80211_hdr *hdr)
{
        if (ieee80211_has_tods(hdr->frame_control))
                return hdr->addr1;
        else
                return hdr->addr2;        
}
/**
 * ieee80211_get_DA - get pointer to DA
 * @hdr: the frame
 *
 * Given an 802.11 frame, this function returns the offset
 * to the destination address (DA). It does not verify that
 * the header is long enough to contain the address, and the
 * header must be long enough to contain the frame control
 * field.
 */
static u8 *ICACHE_FLASH_ATTR
ieee80211_get_DA(struct ieee80211_hdr *hdr)
{
	if (ieee80211_has_tods(hdr->frame_control))
		return hdr->addr3;
	else
		return hdr->addr1;
}

static u8 *ICACHE_FLASH_ATTR
ieee80211_get_SA(struct ieee80211_hdr *hdr)
{
	if (ieee80211_has_a4(hdr->frame_control))
		return hdr->addr4;
	if (ieee80211_has_fromds(hdr->frame_control))
		return hdr->addr3;
	return hdr->addr2;
}

void hnt_onbd_start(void);

#endif  /* _ONBD_H */

/*
 * Copyright (c) 2014 Seaing, Ltd.
 * All Rights Reserved. 
 * Seaing Confidential and Proprietary.
 */


#ifndef __HNT_INTERFACE_H__
#define __HNT_INTERFACE_H__
//#include "ip_addr.h"
#include "esp_common.h"
#include "user_config.h"
typedef sint8 err_t;

typedef void *hntconn_handle;
typedef void (* hntconn_connect_callback)(void *arg);
typedef void (* hntconn_reconnect_callback)(void *arg, sint8 err);

/* Definitions for error constants. */

#define HNTCONN_OK          0    /* No error, everything OK. */
#define HNTCONN_MEM        -1    /* Out of memory error.     */
#define HNTCONN_TIMEOUT    -3    /* Timeout.                 */
#define HNTCONN_RTE        -4    /* Routing problem.         */
#define HNTCONN_INPROGRESS  -5    /* Operation in progress    */

#define HNTCONN_ABRT       -8    /* Connection aborted.      */
#define HNTCONN_RST        -9    /* Connection reset.        */
#define HNTCONN_CLSD       -10   /* Connection closed.       */
#define HNTCONN_CONN       -11   /* Not connected.           */

#define HNTCONN_ARG        -12   /* Illegal argument.        */
#define HNTCONN_ISCONN     -15   /* Already connected.       */

/** Protocol family and type of the hntconn */
enum hntconn_type {
    HNTCONN_INVALID    = 0,
    /* HNTCONN_TCP Group */
    HNTCONN_TCP        = 0x10,
    /* HNTCONN_UDP Group */
    HNTCONN_UDP        = 0x20,
};

/** Current state of the hntconn. Non-TCP hntconn are always in state HNTCONN_NONE! */
enum hntconn_state {
    HNTCONN_NONE,
    HNTCONN_WAIT,
    HNTCONN_LISTEN,
    HNTCONN_CONNECT,
    HNTCONN_WRITE,
    HNTCONN_READ,
    HNTCONN_CLOSE
};

typedef struct _hnt_tcp {
    int remote_port;
    int local_port;
    uint8 local_ip[4];
    uint8 remote_ip[4];
    hntconn_connect_callback connect_callback;
    hntconn_reconnect_callback reconnect_callback;
    hntconn_connect_callback disconnect_callback;
	hntconn_connect_callback write_finish_fn;
} hnt_tcp;

typedef struct _hnt_udp {
    int remote_port;
    int local_port;
    uint8 local_ip[4];
	uint8 remote_ip[4];
} hnt_udp;

typedef struct _hnt_remot_info{
	enum hntconn_state state;
	int remote_port;
	uint8 remote_ip[4];
}hnt_remot_info;

/** A callback prototype to inform about events for a hntconn */
typedef void (* hntconn_recv_callback)(void *arg, char *pdata, unsigned short len);
typedef void (* hntconn_sent_callback)(void *arg);

/** A hntconn descriptor */
struct hntconn {
    /** type of the hntconn (TCP, UDP) */
    enum hntconn_type type;
    /** current state of the hntconn */
    enum hntconn_state state;
    union {
        hnt_tcp *tcp;
        hnt_udp *udp;
    } proto;
    /** A callback function that is informed about events for this hntconn */
    hntconn_recv_callback recv_callback;
    hntconn_sent_callback sent_callback;
    uint8 link_cnt;
    void *reverse;
};

enum hntconn_option{
	HNTCONN_START = 0x00,
	HNTCONN_REUSEADDR = 0x01,
	HNTCONN_NODELAY = 0x02,
	HNTCONN_COPY = 0x04,
	HNTCONN_KEEPALIVE = 0x08,
	HNTCONN_END
};

enum hntconn_level{
	HNTCONN_KEEPIDLE,
	HNTCONN_KEEPINTVL,
	HNTCONN_KEEPCNT
};

enum {
	HNTCONN_IDLE = 0,
	HNTCONN_CLIENT,
	HNTCONN_SERVER,
	HNTCONN_BOTH,
	HNTCONN_MAX
};

struct hntconn_packet{
	uint16 sent_length;		/* sent length successful*/
	uint16 snd_buf_size;	/* Available buffer size for sending  */
	uint16 snd_queuelen;	/* Available buffer space for sending */
	uint16 total_queuelen;	/* total Available buffer space for sending */
	uint32 packseqno;		/* seqno to be sent */
	uint32 packseq_nxt;		/* seqno expected */
	uint32 packnum;
};

struct hnt_mdns_info {
	char *host_name;
	char *server_name;
	uint16 server_port;
	unsigned long ipAddr;
	char *txt_data[10];
};

/******************************************************************************
 * FunctionName : hntconn_connect
 * Description  : The function given as the connect
 * Parameters   : hntconn -- the hntconn used to listen the connection
 * Returns      : none
*******************************************************************************/

sint8 hntconn_connect(struct hntconn *hntconn);

/******************************************************************************
 * FunctionName : hntconn_disconnect
 * Description  : disconnect with host
 * Parameters   : hntconn -- the hntconn used to disconnect the connection
 * Returns      : none
*******************************************************************************/

sint8 hntconn_disconnect(struct hntconn *hntconn);

/******************************************************************************
 * FunctionName : hntconn_delete
 * Description  : disconnect with host
 * Parameters   : hntconn -- the hntconn used to disconnect the connection
 * Returns      : none
*******************************************************************************/

sint8 hntconn_delete(struct hntconn *hntconn);

/******************************************************************************
 * FunctionName : hntconn_accept
 * Description  : The function given as the listen
 * Parameters   : hntconn -- the hntconn used to listen the connection
 * Returns      : none
*******************************************************************************/

sint8 hntconn_accept(struct hntconn *hntconn);

/******************************************************************************
 * FunctionName : hntconn_create
 * Description  : sent data for client or server
 * Parameters   : hntconn -- hntconn to the data transmission
 * Returns      : result
*******************************************************************************/

sint8 hntconn_create(struct hntconn *hntconn);

/******************************************************************************
 * FunctionName : hntconn_tcp_get_max_con
 * Description  : get the number of simulatenously active TCP connections
 * Parameters   : none
 * Returns      : none
*******************************************************************************/

uint8 hntconn_tcp_get_max_con(void);

/******************************************************************************
 * FunctionName : hntconn_tcp_set_max_con
 * Description  : set the number of simulatenously active TCP connections
 * Parameters   : num -- total number
 * Returns      : none
*******************************************************************************/

sint8 hntconn_tcp_set_max_con(uint8 num);

/******************************************************************************
 * FunctionName : hntconn_tcp_get_max_con_allow
 * Description  : get the count of simulatenously active connections on the server
 * Parameters   : hntconn -- hntconn to get the count
 * Returns      : result
*******************************************************************************/

sint8 hntconn_tcp_get_max_con_allow(struct hntconn *hntconn);

/******************************************************************************
 * FunctionName : hntconn_tcp_set_max_con_allow
 * Description  : set the count of simulatenously active connections on the server
 * Parameters   : hntconn -- hntconn to set the count
 * 				  num -- support the connection number
 * Returns      : result
*******************************************************************************/

sint8 hntconn_tcp_set_max_con_allow(struct hntconn *hntconn, uint8 num);

/******************************************************************************
 * FunctionName : hntconn_regist_time
 * Description  : used to specify the time that should be called when don't recv data
 * Parameters   : hntconn -- the hntconn used to the connection
 * 				  interval -- the timer when don't recv data
 * Returns      : none
*******************************************************************************/

sint8 hntconn_regist_time(struct hntconn *hntconn, uint32 interval, uint8 type_flag);

/******************************************************************************
 * FunctionName : hntconn_get_connection_info
 * Description  : used to specify the function that should be called when disconnect
 * Parameters   : hntconn -- hntconn to set the err callback
 *                discon_cb -- err callback function to call when err
 * Returns      : none
*******************************************************************************/

sint8 hntconn_get_connection_info(struct hntconn *phntconn, hnt_remot_info **pcon_info, uint8 typeflags);

/******************************************************************************
 * FunctionName : hntconn_get_packet_info
 * Description  : get the packet info with host
 * Parameters   : hntconn -- the hntconn used to disconnect the connection
 * 				  infoarg -- the packet info
 * Returns      : the errur code
*******************************************************************************/

sint8 hntconn_get_packet_info(struct hntconn *hntconn, struct hntconn_packet* infoarg);

/******************************************************************************
 * FunctionName : hntconn_regist_sentcb
 * Description  : Used to specify the function that should be called when data
 *                has been successfully delivered to the remote host.
 * Parameters   : struct hntconn *hntconn -- hntconn to set the sent callback
 *                hntconn_sent_callback sent_cb -- sent callback function to
 *                call for this hntconn when data is successfully sent
 * Returns      : none
*******************************************************************************/

sint8 hntconn_regist_sentcb(struct hntconn *hntconn, hntconn_sent_callback sent_cb);

/******************************************************************************
 * FunctionName : hntconn_regist_sentcb
 * Description  : Used to specify the function that should be called when data
 *                has been successfully delivered to the remote host.
 * Parameters   : hntconn -- hntconn to set the sent callback
 *                sent_cb -- sent callback function to call for this hntconn
 *                when data is successfully sent
 * Returns      : none
*******************************************************************************/

sint8 hntconn_regist_write_finish(struct hntconn *hntconn, hntconn_connect_callback write_finish_fn);

/******************************************************************************
 * FunctionName : hntconn_sent
 * Description  : sent data for client or server
 * Parameters   : hntconn -- hntconn to set for client or server
 *                psent -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/

sint8 hntconn_sent(struct hntconn *hntconn, uint8 *psent, uint16 length);

/******************************************************************************
 * FunctionName : hntconn_regist_connectcb
 * Description  : used to specify the function that should be called when
 *                connects to host.
 * Parameters   : hntconn -- hntconn to set the connect callback
 *                connect_cb -- connected callback function to call when connected
 * Returns      : none
*******************************************************************************/

sint8 hntconn_regist_connectcb(struct hntconn *hntconn, hntconn_connect_callback connect_cb);

/******************************************************************************
 * FunctionName : hntconn_regist_recvcb
 * Description  : used to specify the function that should be called when recv
 *                data from host.
 * Parameters   : hntconn -- hntconn to set the recv callback
 *                recv_cb -- recv callback function to call when recv data
 * Returns      : none
*******************************************************************************/

sint8 hntconn_regist_recvcb(struct hntconn *hntconn, hntconn_recv_callback recv_cb);

/******************************************************************************
 * FunctionName : hntconn_regist_reconcb
 * Description  : used to specify the function that should be called when connection
 *                because of err disconnect.
 * Parameters   : hntconn -- hntconn to set the err callback
 *                recon_cb -- err callback function to call when err
 * Returns      : none
*******************************************************************************/

sint8 hntconn_regist_reconcb(struct hntconn *hntconn, hntconn_reconnect_callback recon_cb);

/******************************************************************************
 * FunctionName : hntconn_regist_disconcb
 * Description  : used to specify the function that should be called when disconnect
 * Parameters   : hntconn -- hntconn to set the err callback
 *                discon_cb -- err callback function to call when err
 * Returns      : none
*******************************************************************************/

sint8 hntconn_regist_disconcb(struct hntconn *hntconn, hntconn_connect_callback discon_cb);

/******************************************************************************
 * FunctionName : hntconn_port
 * Description  : access port value for client so that we don't end up bouncing
 *                all connections at the same time .
 * Parameters   : none
 * Returns      : access port value
*******************************************************************************/

uint32 hntconn_port(void);

/******************************************************************************
 * FunctionName : hntconn_set_opt
 * Description  : access port value for client so that we don't end up bouncing
 *                all connections at the same time .
 * Parameters   : none
 * Returns      : access port value
*******************************************************************************/

sint8 hntconn_set_opt(struct hntconn *hntconn, uint8 opt);

/******************************************************************************
 * FunctionName : hntconn_clear_opt
 * Description  : clear the option for connections so that we don't end up bouncing
 *                all connections at the same time .
 * Parameters   : hntconn -- the hntconn used to set the connection
 * 				  opt -- the option for clear
 * Returns      : the result
*******************************************************************************/

sint8 hntconn_clear_opt(struct hntconn *hntconn, uint8 opt);

/******************************************************************************
 * FunctionName : hntconn_set_keepalive
 * Description  : access level value for connection so that we set the value for
 * 				  keep alive
 * Parameters   : hntconn -- the hntconn used to set the connection
 * 				  level -- the connection's level
 * 				  value -- the value of time(s)
 * Returns      : access port value
*******************************************************************************/

sint8 hntconn_set_keepalive(struct hntconn *hntconn, uint8 level, void* optarg);

/******************************************************************************
 * FunctionName : hntconn_get_keepalive
 * Description  : access level value for connection so that we get the value for
 * 				  keep alive
 * Parameters   : hntconn -- the hntconn used to get the connection
 * 				  level -- the connection's level
 * Returns      : access keep alive value
*******************************************************************************/

sint8 hntconn_get_keepalive(struct hntconn *hntconn, uint8 level, void *optarg);

/******************************************************************************
 * TypedefName : dns_found_callback
 * Description : Callback which is invoked when a hostname is found.
 * Parameters  : name -- pointer to the name that was looked up.
 *               ipaddr -- pointer to an ip_addr_t containing the IP address of
 *               the hostname, or NULL if the name could not be found (or on any
 *               other error).
 *               callback_arg -- a user-specified callback argument passed to
 *               dns_gethostbyname
*******************************************************************************/

typedef void (*hnt_dns_found_callback)(const char *name, ip_addr_t *ipaddr, void *callback_arg);

/******************************************************************************
 * FunctionName : hntconn_gethostbyname
 * Description  : Resolve a hostname (string) into an IP address.
 * Parameters   : phntconn -- hntconn to resolve a hostname
 *                hostname -- the hostname that is to be queried
 *                addr -- pointer to a ip_addr_t where to store the address if 
 *                it is already cached in the dns_table (only valid if ESPCONN_OK
 *                is returned!)
 *                found -- a callback function to be called on success, failure
 *                or timeout (only if ERR_INPROGRESS is returned!)
 * Returns      : err_t return code
 *                - ESPCONN_OK if hostname is a valid IP address string or the host
 *                  name is already in the local names table.
 *                - ESPCONN_INPROGRESS enqueue a request to be sent to the DNS server
 *                  for resolution if no errors are present.
 *                - ESPCONN_ARG: dns client not initialized or invalid hostname
*******************************************************************************/

err_t hntconn_gethostbyname(struct hntconn *phntconn, const char *hostname, ip_addr_t *addr, hnt_dns_found_callback found);

/******************************************************************************
 * FunctionName : hntconn_encry_connect
 * Description  : The function given as connection
 * Parameters   : hntconn -- the hntconn used to connect with the host
 * Returns      : none
*******************************************************************************/

sint8 hntconn_secure_connect(struct hntconn *hntconn);

/******************************************************************************
 * FunctionName : hntconn_secure_disconnect
 * Description  : The function given as the disconnection
 * Parameters   : hntconn -- the hntconn used to disconnect with the host
 * Returns      : none
*******************************************************************************/
sint8 hntconn_secure_disconnect(struct hntconn *hntconn);

/******************************************************************************
 * FunctionName : hntconn_encry_sent
 * Description  : sent data for client or server
 * Parameters   : hntconn -- hntconn to set for client or server
 * 				  psent -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/

sint8 hntconn_secure_sent(struct hntconn *hntconn, uint8 *psent, uint16 length);

/******************************************************************************
 * FunctionName : hntconn_secure_set_size
 * Description  : set the buffer size for client or server
 * Parameters   : level -- set for client or server
 * 				  1: client,2:server,3:client and server
 * 				  size -- buffer size
 * Returns      : true or false
*******************************************************************************/

bool hntconn_secure_set_size(uint8 level, uint16 size);

/******************************************************************************
 * FunctionName : hntconn_secure_get_size
 * Description  : get buffer size for client or server
 * Parameters   : level -- set for client or server
 *				  1: client,2:server,3:client and server
 * Returns      : buffer size for client or server
*******************************************************************************/

sint16 hntconn_secure_get_size(uint8 level);

/******************************************************************************
 * FunctionName : hntconn_secure_accept
 * Description  : The function given as the listen
 * Parameters   : hntconn -- the hntconn used to listen the connection
 * Returns      : none
*******************************************************************************/

sint8 hntconn_secure_accept(struct hntconn *hntconn);

/******************************************************************************
 * FunctionName : hntconn_igmp_join
 * Description  : join a multicast group
 * Parameters   : host_ip -- the ip address of udp server
 * 				  multicast_ip -- multicast ip given by user
 * Returns      : none
*******************************************************************************/
sint8 hntconn_igmp_join(ip_addr_t *host_ip, ip_addr_t *multicast_ip);

/******************************************************************************
 * FunctionName : hntconn_igmp_leave
 * Description  : leave a multicast group
 * Parameters   : host_ip -- the ip address of udp server
 * 				  multicast_ip -- multicast ip given by user
 * Returns      : none
*******************************************************************************/
sint8 hntconn_igmp_leave(ip_addr_t *host_ip, ip_addr_t *multicast_ip);

/******************************************************************************
 * FunctionName : hntconn_recv_hold
 * Description  : hold tcp receive
 * Parameters   : hntconn -- hntconn to hold
 * Returns      : none
*******************************************************************************/
sint8 hntconn_recv_hold(struct hntconn *phntconn);

/******************************************************************************
 * FunctionName : hntconn_recv_unhold
 * Description  : unhold tcp receive
 * Parameters   : hntconn -- hntconn to unhold
 * Returns      : none
*******************************************************************************/
sint8 hntconn_recv_unhold(struct hntconn *phntconn);

/******************************************************************************
 * FunctionName : hntconn_mdns_init
 * Description  : register a device with mdns
 * Parameters   : ipAddr -- the ip address of device
 * 				  hostname -- the hostname of device
 * Returns      : none
*******************************************************************************/

void hntconn_mdns_init(struct hnt_mdns_info *info);
/******************************************************************************
 * FunctionName : hntconn_mdns_close
 * Description  : close a device with mdns
 * Parameters   : a
 * Returns      : none
*******************************************************************************/

void hntconn_mdns_close(void);
/******************************************************************************
 * FunctionName : hntconn_mdns_server_register
 * Description  : register a device with mdns
 * Parameters   : a
 * Returns      : none
*******************************************************************************/
void hntconn_mdns_server_register(void);

/******************************************************************************
 * FunctionName : hntconn_mdns_server_unregister
 * Description  : unregister a device with mdns
 * Parameters   : a
 * Returns      : none
*******************************************************************************/
void hntconn_mdns_server_unregister(void);

/******************************************************************************
 * FunctionName : hntconn_mdns_get_servername
 * Description  : get server name of device with mdns
 * Parameters   : a
 * Returns      : none
*******************************************************************************/

char* hntconn_mdns_get_servername(void);
/******************************************************************************
 * FunctionName : hntconn_mdns_set_servername
 * Description  : set server name of device with mdns
 * Parameters   : a
 * Returns      : none
*******************************************************************************/
void hntconn_mdns_set_servername(const char *name);

/******************************************************************************
 * FunctionName : hntconn_mdns_set_hostname
 * Description  : set host name of device with mdns
 * Parameters   : a
 * Returns      : none
*******************************************************************************/
void hntconn_mdns_set_hostname(char *name);

/******************************************************************************
 * FunctionName : hntconn_mdns_get_hostname
 * Description  : get host name of device with mdns
 * Parameters   : a
 * Returns      : none
*******************************************************************************/
char* hntconn_mdns_get_hostname(void);

/******************************************************************************
 * FunctionName : hntconn_mdns_disable
 * Description  : disable a device with mdns
 * Parameters   : a
 * Returns      : none
*******************************************************************************/
void hntconn_mdns_disable(void);

/******************************************************************************
 * FunctionName : hntconn_mdns_enable
 * Description  : disable a device with mdns
 * Parameters   : a
 * Returns      : none
*******************************************************************************/
void hntconn_mdns_enable(void);
/******************************************************************************
 * FunctionName : hntconn_dns_setserver
 * Description  : Initialize one of the DNS servers.
 * Parameters   : numdns -- the index of the DNS server to set must
 * 				  be < DNS_MAX_SERVERS = 2
 * 			      dnsserver -- IP address of the DNS server to set
 *  Returns     : none
*******************************************************************************/
void hntconn_dns_setserver(char numdns, ip_addr_t *dnsserver);

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

void 
hnt_xmppcli_start(void);

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


#define WIFI_LED_STATUS_OFFLINE                        0
#define WIFI_LED_STATUS_CONNECTTED_TO_AP               1
#define WIFI_LED_STATUS_CONNECTING_TO_AP               2
#define WIFI_LED_STATUS_RECEIVE_CONFIG                 3
#define WIFI_LED_STATUS_CONNECTED_TO_SERVER            4

void 
hnt_wifi_led_func_regist(void *func);

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
#define printf os_printf
#define sprintf os_sprintf
//#define os_memset os_memset
//#define os_memcpy os_memcpy
#define strcmp os_strcmp
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
#define HAVE_TLS 1
//#define DLMPD_SUPPORT 1

#endif

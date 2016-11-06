/* iksemel (XML parser for Jabber)
** Copyright (C) 2000-2004 Gurer Ozen
** This code is free software; you can redistribute it and/or
** modify it under the terms of GNU Lesser General Public License.
*/
#include "esp_common.h" 
#include "c_types.h"
#include "freertos/queue.h"

#include "iksemel/iksemel.h"
#include "xmpp/xmppcli.h"
#include "xmpp/xmppcli_cwmp.h"
#include "mgmt/mgmt.h"
#include "mgmt/timer.h"
#include "hnt_interface.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
//#include "espconn.h"
#include "ledctl/ledctl.h"

xQueueHandle xmpp_task_q;

extern struct hnt_factory_param g_hnt_factory_param;
extern customInfo_t *DeviceCustomInfo;
extern uint8 onbd_udp_notify_times;

uint8 opt_log = 1;

/* connection flags */
#if defined(HAVE_TLS) || defined(CLIENT_SSL_ENABLE)
unsigned char *default_certificate;
unsigned int default_certificate_len = 0;
unsigned char *default_private_key;
unsigned int default_private_key_len = 0;
#endif

#ifdef HAVE_TLS
uint8 opt_use_tls = 1;
#else
uint8 opt_use_tls = 0;
#endif

uint8 opt_use_sasl = 1;
uint8 opt_use_plain = 1;

int 
xmpp_send_raw(struct session *sess, char *data, size_t size);
int 
user_xmppclient_stop(void);

struct session sess;

LOCAL ip_addr_t xmpp_server_ip;
LOCAL uint8 xmpp_conn_status = 0;

uint32 local_time_pre = 0;
uint32 remote_time_sec = 0;
uint32 remote_time_msec = 0;

uint32 ICACHE_FLASH_ATTR
get_current_time(void)
{
    if (remote_time_sec)
        return remote_time_sec + (system_get_time()-local_time_pre)/(1000*1000);            
    else
        return 0;            
}

uint8 ICACHE_FLASH_ATTR
get_xmpp_conn_status(void)
{
    return xmpp_conn_status;
}

/** Create a bare JID from a JID.
 *  
 *  @param ctx the Strophe context object
 *  @param jid the JID
 *
 *  @return an allocated string with the bare JID or NULL on an error
 */
char *ICACHE_FLASH_ATTR
xmpp_jid_bare(const char *jid)
{
    char *result;
    const char *c;

    if (jid == NULL) return NULL;
    c = strchr(jid, '/');
    if (c == NULL) return iks_strdup(jid);

    result = (char *)malloc(c-jid+1);
    if (result != NULL) {
	memcpy(result, jid, c-jid);
	result[c-jid] = '\0';
    }

    return result;
}

char *ICACHE_FLASH_ATTR
xmpp_conn_get_bound_jid(void)
{
    return sess.bound_jid;
}

char * ICACHE_FLASH_ATTR
create_unique_id(char *prefix)
{
    static unsigned long unique_id;
    static char result_str[32];

    memset(result_str, 0, sizeof(result_str));
    unique_id++;
    if (prefix != NULL) {
        sprintf(result_str, "prof_%s_%lu", prefix, unique_id);
    } else {
        sprintf(result_str, "prof_%lu", unique_id);
    }

    return result_str;
}

void ICACHE_FLASH_ATTR
xmpp_setup_filter (struct session *sess)
{
	if (sess->filter) iks_filter_delete (sess->filter);
	sess->filter = iks_filter_new ();
}
LOCAL int ICACHE_FLASH_ATTR
_iq_handler(struct session *sess, ikspak *pak)
{

	iks *cwmp_body;

    log_debug("test\n");
	if(!(cwmp_body = iks_find_child(pak->x, STANZA_NAME_QUERY))) 
	    return IKS_FILTER_EAT;
            
    CwmpRpcProcess(NULL, iks_find_attrib(pak->x, STANZA_ATTR_FROM), 
                   iks_find_attrib(pak->x, STANZA_ATTR_ID),
                   cwmp_body);
    log_debug("test\n");
	return IKS_FILTER_EAT;
}

LOCAL int ICACHE_FLASH_ATTR
_pong_handler(struct session *sess, ikspak *pak)
{
    char *time_stamp;
    char second_buffer[16] = {0};
    char msec_buffer[16] = {0};
    
    if((pak->subtype == IKS_TYPE_RESULT) || (pak->subtype == IKS_TYPE_ERROR))
    {
        os_timer_disarm(&sess->xmpp_pong_timer);
        sess->ping_timeout_times = 0;

        if(time_stamp = iks_find_attrib(pak->x, STANZA_ATTR_TIMESTAMP))
        {
            printf("local time %02d:%02d:%02d\n", (get_current_time()/3600)%24 + 8,
                (get_current_time()/60)%60,(get_current_time())%60);
            
            log_debug("time_stamp = %s\n", time_stamp);
            memcpy(second_buffer, time_stamp, strlen(time_stamp) - 3);
            memcpy(msec_buffer, time_stamp + (strlen(time_stamp) - 3), 3);
            log_debug("second_buffer = %s\n", second_buffer);
            log_debug("msec_buffer = %s\n", msec_buffer);
            local_time_pre = system_get_time();
            remote_time_sec = atoi(second_buffer);
            remote_time_msec = atoi(msec_buffer);
            log_debug("local_time_pre = %u\n", local_time_pre);
            log_debug("remote_time_sec = %u\n", remote_time_sec);
            log_debug("remote_time_msec = %u\n", remote_time_msec);

            printf("remote time %02d:%02d:%02d\n", (remote_time_sec/3600)%24 + 8,
                (remote_time_sec/60)%60,(remote_time_sec)%60);

            printf("set time %02d:%02d:%02d\n", (get_current_time()/3600)%24 + 8,
                (get_current_time()/60)%60,(get_current_time())%60);    
            
            log_debug("current time = %u\n", get_current_time());
//            hnt_platform_timer_init();
        }

        iks_filter_remove_hook(sess->filter, (iksFilterHook *)_pong_handler);
        
        return IKS_FILTER_EAT;
    }

    return IKS_FILTER_PASS;
}

LOCAL void ICACHE_FLASH_ATTR
_pong_timed_handler(void * parg)
{
    struct session *sess = (struct session *)parg;

    sess->ping_timeout_times ++;
    iks_filter_remove_hook(sess->filter, (iksFilterHook *)_pong_handler);

    if (sess->ping_timeout_times == PING_TIMEOUT_TIMES) {
        printf("ping timeout\n");
        system_restart();
    }
}
LOCAL void ICACHE_FLASH_ATTR
_ping_timed_handler(void * parg)
{
    u32 msg = MSG_SEND_PING;
    xQueueSend(xmpp_task_q, (void *)&msg, 0);            
}

LOCAL void ICACHE_FLASH_ATTR
_send_ping(void * parg)
{
    log_debug("test\n");
    printf("local time %02d:%02d:%02d\n", (get_current_time()/3600)%24 + 8,
        (get_current_time()/60)%60,(get_current_time())%60);

    struct session *sess = (struct session *)parg;
	iks *ping = NULL;
	ping = iks_make_iq(IKS_TYPE_GET, STANZA_NS_PING);
	if(ping) {
        char *ping_id = create_unique_id("ping");
        iks_insert_attrib (ping, "id", ping_id);
		iks_send(sess->prs, ping);
		iks_delete(ping);

        os_timer_disarm(&sess->xmpp_pong_timer);
        os_timer_setfn(&sess->xmpp_pong_timer, (os_timer_func_t *)_pong_timed_handler, sess);
        os_timer_arm(&sess->xmpp_pong_timer, PONG_TIMEOUT, 0);

        log_debug("iks_filter_add_rule:_pong_handler=%p\n", (iksFilterHook *) _pong_handler);
        iks_filter_add_rule(sess->filter, (iksFilterHook *) _pong_handler, sess, 
                            (IKS_RULE_TYPE + IKS_RULE_ID),
                            IKS_PAK_IQ,
                            IKS_TYPE_NONE,
                            ping_id,
                            NULL,
                            NULL,
                            NULL);
            
	}
}

#ifdef HAVE_TLS
/******************************************************************************
 * FunctionName : user_esp_platform_connect_cb
 * Description  : A new incoming connection has been connected.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
xmpp_tls_connect_cb(void *arg)
{
    struct espconn *pespconn = arg;

    log_debug("xmpp_tls_connect_cb\n");

    xmpp_send_header();   
}
static int ICACHE_FLASH_ATTR
handshake (struct stream_data *data)
{
    log_debug("system_get_free_heap_size = %d\n", system_get_free_heap_size()); 

    espconn_secure_half_connect(sess.pespconn);
    log_debug("system_get_free_heap_size = %d\n", system_get_free_heap_size()); 

	data->flags &= (~SF_TRY_SECURE);
	data->flags |= SF_SECURE;

//	iks_send_header (data->prs, data->server);
    espconn_regist_connectcb(sess.pespconn, xmpp_tls_connect_cb);
    
	return IKS_OK;
}
#endif

LOCAL int ICACHE_FLASH_ATTR
xmpp_conn_handler (struct session *sess, ikspak *pak)
{
	iks *presence = NULL;
    log_debug("test\n");

    xmpp_conn_status = 1;

    printf("local time %02d:%02d:%02d\n", (get_current_time()/3600)%24 + 8,
        (get_current_time()/60)%60,(get_current_time())%60);
    
/* presence online */
    presence = iks_make_pres(IKS_SHOW_AVAILABLE, "Online");
    if (presence) {
		iks_send(sess->prs, presence);
		iks_delete(presence);    
	}
	
/* add iq customer handle */
    log_debug("iks_filter_add_rule:_iq_handler=%p\n", (iksFilterHook *) _iq_handler);
    iks_filter_add_rule(sess->filter, (iksFilterHook *) _iq_handler, sess, 
                        (IKS_RULE_TYPE),
                        IKS_PAK_IQ,
                        IKS_TYPE_NONE,
                        NULL,
                        NULL,
                        NULL,
                        NULL);
    start_cwmp_inform();

/* inform task */
    os_timer_disarm(&sess->xmpp_inform_timer);
    os_timer_setfn(&sess->xmpp_inform_timer, (os_timer_func_t *)cwmp_inform_handler, sess);
    os_timer_arm(&sess->xmpp_inform_timer, inform_intval_seconds_*1000, 1);

/* sampling task */
    os_timer_disarm(&sess->xmpp_sampling_timer);
    os_timer_setfn(&sess->xmpp_sampling_timer, (os_timer_func_t *)cwmp_sampling_handler, sess);
    os_timer_arm(&sess->xmpp_sampling_timer, sampling_intval_seconds_*1000, 1);

/* ping task */    
    _ping_timed_handler(sess);
    os_timer_disarm(&sess->xmpp_ping_timer);
    os_timer_setfn(&sess->xmpp_ping_timer, (os_timer_func_t *)_ping_timed_handler, sess);
    os_timer_arm(&sess->xmpp_ping_timer, PING_TIMEOUT, 1);
    
    iks_filter_remove_hook(sess->filter, (iksFilterHook *)xmpp_conn_handler);
	return IKS_FILTER_EAT;
}

LOCAL int ICACHE_FLASH_ATTR
xmpp_bind_handler (struct session *sess, ikspak *pak)
{
    log_debug("test\n");

    if (sess->authorized) {
        iks *t = NULL;
        if (sess->features & IKS_STREAM_SESSION) { 
            iks *binding = iks_find_child(pak->x, "bind");
            if (binding) {
                log_debug("test\n");
                iks *jid_stanza = iks_find_child(binding, "jid");
                if (jid_stanza) {
                    sess->bound_jid = iks_strdup(iks_cdata(iks_child(jid_stanza)));
                    log_debug("test:%s\n", sess->bound_jid);
                }
            }
            iks_filter_add_rule(sess->filter, (iksFilterHook *) xmpp_conn_handler, sess,
                        (IKS_RULE_TYPE + IKS_RULE_SUBTYPE + IKS_RULE_ID),
                        IKS_PAK_IQ,
                        IKS_TYPE_RESULT,
                        "auth",
                        NULL,
                        NULL,
                        NULL);        
            t = iks_make_session ();
            iks_insert_attrib (t, "id", "auth");
            iks_send (sess->prs, t);
            iks_delete (t);
        }        
    }
    
    iks_filter_remove_hook(sess->filter, (iksFilterHook *)xmpp_bind_handler);
	return IKS_FILTER_EAT;
}

int ICACHE_FLASH_ATTR
xmpp_stream_handler (struct session *sess, int type, iks *node)
{
    int ret = IKS_OK;

	switch (type) {
		case IKS_NODE_START:
            log_debug("test\n");
			if (!opt_use_sasl) {
				iks *x;
				char *sid = NULL;
                log_debug("test\n");

				if (!opt_use_plain) sid = iks_find_attrib (node, "id");
				x = iks_make_auth (sess->acc, sess->pass, sid);
				iks_insert_attrib (x, "id", "auth");
				iks_send (sess->prs, x);
				iks_delete (x);
			}
			break;

		case IKS_NODE_NORMAL:
            log_debug("test\n");		
			if (strcmp ("stream:features", iks_name (node)) == 0) {
				sess->features = iks_stream_features (node);
				if (opt_use_sasl) {
				
                    log_debug("test\n");
					if (opt_use_tls && !iks_is_secure (sess->prs)) {
					    if (sess->features & IKS_STREAM_STARTTLS){
                            log_debug("test\n");
            				iks_start_tls (sess->prs);
            				break;
            			}
					}    
					if (sess->authorized) {
						iks *t;
						if (sess->features & IKS_STREAM_BIND) {
                            iks_filter_add_rule(sess->filter, (iksFilterHook *) xmpp_bind_handler, sess,
                                                (IKS_RULE_TYPE + IKS_RULE_SUBTYPE + IKS_RULE_ID),
                                                IKS_PAK_IQ,
                                                IKS_TYPE_RESULT,
                                                "bind",
                                                NULL,
                                                NULL,
                                                NULL);
                                
							t = iks_make_resource_bind (sess->acc);
							iks_insert_attrib (t, "id", "bind");
							iks_send (sess->prs, t);
							iks_delete (t);
						}
					} else {
                        log_debug("test\n");
						if (sess->features & IKS_STREAM_SASL_MD5)
							iks_start_sasl (sess->prs, IKS_SASL_DIGEST_MD5, sess->acc->user, sess->pass);
						else if (sess->features & IKS_STREAM_SASL_PLAIN)
							iks_start_sasl (sess->prs, IKS_SASL_PLAIN, sess->acc->user, sess->pass);
					}
				}
			} else if (strcmp ("failure", iks_name (node)) == 0) {
				log_error ("sasl authentication failed\n");
			} else if (strcmp ("success", iks_name (node)) == 0) {
				sess->authorized = 1;
				iks_send_header (sess->prs, sess->acc->server);
#ifdef HAVE_TLS
			} else if (strcmp ("proceed", iks_name (node)) == 0) {
				handshake(iks_user_data(sess->prs));
#endif
			} else {
				ikspak *pak;

                log_debug("test\n");
				pak = iks_packet (node);
				iks_filter_packet (sess->filter, pak);
			}
			break;

		case IKS_NODE_STOP:
			log_error ("server disconnected\n");
		case IKS_NODE_ERROR:
			log_error ("stream error\n");
			user_xmppclient_restart();
			break;
	}
    log_debug("test\n");

	if (node) iks_delete (node);
	return ret;
}
void ICACHE_FLASH_ATTR
on_log (struct session *sess, const char *data, size_t size, int is_incoming)
{
	if (iks_is_secure (sess->prs)) printf("Sec");
	if (is_incoming) printf("RECV"); else printf("SEND");
	printf("[%s]\n", data);
}

int ICACHE_FLASH_ATTR
xmpp_send(iks *x)
{   

    return iks_send(sess.prs, x);
}

int ICACHE_FLASH_ATTR
xmpp_send_raw(struct session *sess, char *data, size_t size)
{
    int ret = 0;

    log_debug("system_get_free_heap_size = %d, size = %d\n", system_get_free_heap_size(), size); 

    if (iks_is_secure(sess->prs)){
        send(sess->xmpp_sock, data, size,0);
    }
    else{
        ret = send(sess->xmpp_sock, data, size, 0);
        if(ret < 0) {
            printf("send failed, ret=%d\n", ret);
        }
    }
    log_debug("test\n");

    return IKS_OK;
}

int ICACHE_FLASH_ATTR
xmpp_send_header(void)
{
    return iks_send_header(sess.prs, sess.acc->server);
}


void ICACHE_FLASH_ATTR
xmpp_connect (int xmpp_socket, char *jabber_id, char *pass)
{
	int e;

	memset (&sess, 0, sizeof (sess));
    log_debug("system_get_free_heap_size = %d\n", system_get_free_heap_size()); 
	sess.prs = iks_stream_new (IKS_NS_CLIENT, &sess, (iksStreamHook *) xmpp_stream_handler);
    log_debug("system_get_free_heap_size = %d\n", system_get_free_heap_size()); 
	sess.xmpp_sock = xmpp_socket;
	if (opt_log) iks_set_log_hook (sess.prs, (iksLogHook *) on_log);
	iks_set_send_hook(sess.prs, (iksSendHook *) xmpp_send_raw);
    log_debug("system_get_free_heap_size = %d\n", system_get_free_heap_size()); 
	sess.acc = iks_id_new (iks_parser_stack (sess.prs), jabber_id);
    log_debug("system_get_free_heap_size = %d\n", system_get_free_heap_size()); 
	if (NULL == sess.acc->resource) {
		/* user gave no resource name, use the default */
		char *tmp;
		static int res_id = 0; 
		tmp = iks_malloc (strlen (sess.acc->user) + strlen (sess.acc->server) + 9 + 3);
		//sprintf (tmp, "%s@%s/%d", sess.acc->user, sess.acc->server, system_get_time());
		sprintf (tmp, "%s@%s/LinkusDevice", sess.acc->user, sess.acc->server);
		sess.acc = iks_id_new (iks_parser_stack (sess.prs), tmp);
		iks_free (tmp);
	}
    log_debug("system_get_free_heap_size = %d\n", system_get_free_heap_size()); 
	
	sess.pass = pass;

	xmpp_setup_filter (&sess);
}

int ICACHE_FLASH_ATTR
xmpp_recv (char *pusrdata, unsigned short length)
{
	struct stream_data *data = iks_user_data (sess.prs);
	int len, ret;

    if (data->logHook) data->logHook (data->user_data, (const char *)pusrdata, (size_t)length, 1);
    ret = iks_parse (sess.prs, pusrdata, length, 0);
    if (ret != IKS_OK) return ret;
    //if (!data->trans) {
        /* stream hook called iks_disconnect */
        //	return IKS_NET_NOCONN;
    //}

	return IKS_OK;
}

int ICACHE_FLASH_ATTR
user_xmppclient_disconnect (void)
{
    //system_restart();
    xmpp_conn_status = 0;

    if(sess.prs != NULL)
    {
        iks_parser_delete(sess.prs);
        sess.prs = NULL;
    }
    //iks_disconnect(sess.prs);
    sess.authorized = 0;
    if(sess.bound_jid != NULL)
    {
        iks_free(sess.bound_jid);
        sess.bound_jid = NULL;
    }

    if(sess.filter != NULL)
    {
        iks_filter_delete(sess.filter);
        sess.filter = NULL;
    }

    os_timer_disarm(&sess.xmpp_inform_timer);
    os_timer_disarm(&sess.xmpp_sampling_timer);
    os_timer_disarm(&sess.xmpp_ping_timer);
    os_timer_disarm(&sess.xmpp_pong_timer);
}

/******************************************************************************
 * FunctionName : user_esp_platform_discon
 * Description  : A new incoming connection has been disconnected.
 * Parameters   : espconn -- the espconn used to disconnect with host
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_xmppclient_discon(void)
{
    sint8 ret;
    log_debug("test\n");
    xmpp_conn_status = 0;

//    espconn_disconnect(pespconn);
}

void ICACHE_FLASH_ATTR
user_xmppclient_restart_cb(void)
{
    user_xmppclient_discon();
    user_xmppclient_disconnect();   
}

int ICACHE_FLASH_ATTR
user_xmppclient_restart(void)
{
    xmpp_conn_status = 0;
    user_xmppclient_restart_cb();
}

int ICACHE_FLASH_ATTR
user_xmppclient_stop(void)
{
    user_xmppclient_discon();
    user_xmppclient_disconnect();   
}

int xmpp_sock = -1;
#define XMPP_PORT	5222
#define XMPP_RX_BUF_SIZE 2048
u8 xmpp_rx_buf[XMPP_RX_BUF_SIZE] = {0};

int xmpp_start(void)
{
	struct sockaddr_in sin;
	struct hostent* HostEntry;
         
    for(;;) {
        printf("gethostbyname %s\n",g_hnt_factory_param.xmpp_server);
        
        HostEntry = gethostbyname(g_hnt_factory_param.xmpp_server);
        
        if(HostEntry)
        {
            break;
        }
        else{
            printf("resolve name server error\n");
            vTaskDelay(100 / portTICK_RATE_MS);  // 100 ms
        }
    }
    printf("connect to %d.%d.%d.%d\n", HostEntry->h_addr_list[0][0],
        HostEntry->h_addr_list[0][1],HostEntry->h_addr_list[0][2],
        HostEntry->h_addr_list[0][3]
        );
    
    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET; 
    memcpy(&sin.sin_addr.s_addr, HostEntry->h_addr_list[0], 4);
	sin.sin_port=htons(XMPP_PORT);
	xmpp_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    printf("xmpp socket num=%d\n", xmpp_sock);

	if (connect(xmpp_sock, (struct sockaddr *)&sin, sizeof(struct sockaddr)) != 0)        
	{
		printf("connect failed! socket num=%d\n", xmpp_sock);
//        xmpp_stop();        
		return -1;
	}
	else
	{	
		printf("connect success! next step xmpp_connect.\n");
        xmpp_connect(xmpp_sock, g_hnt_factory_param.xmpp_jid, g_hnt_factory_param.xmpp_password);
        
        //xmpp_connect(xmpp_sock, "d15100006011002@seaing.net", "xphbxHFz7NU");
        //xmpp_connect(xmpp_sock, "d15220010000002@seaing.net", "152BhqGZn4k");
        
        xmpp_send_header();
//        xmpp_ready = 1;
	}	
	return 0;    
}


void ICACHE_FLASH_ATTR
hnt_xmppcli_start(void)
{
    int rx_len = 0;
    int ret;
    fd_set fds;
    int maxsock;    
    struct timeval tv;
    u32 msg;
    
    os_printf("%s\n", __func__);
#if 0
    if(DeviceCustomInfo != NULL)
    {
        if((DeviceCustomInfo->inform_interval != 0) &&
           (DeviceCustomInfo->sampling_interval != 0) &&
           (DeviceCustomInfo->sampling_interval < DeviceCustomInfo->inform_interval))
        {
            inform_intval_seconds_ = DeviceCustomInfo->inform_interval;
            sampling_intval_seconds_ = DeviceCustomInfo->sampling_interval;
        }
    }
#endif    
    xmpp_start();

    xmpp_task_q = xQueueCreate((unsigned portBASE_TYPE)4, sizeof(u32));
    
    for(;;) 
    {   

        if(xmpp_sock >= 0)
        {
        
            FD_ZERO(&fds);
            FD_SET(xmpp_sock, &fds);
            
            tv.tv_sec = 0;
            tv.tv_usec = 100000;
            
            maxsock = xmpp_sock;
            
            ret = select(maxsock + 1, &fds, NULL, NULL, &tv);
            if(ret > 0) 
            {
                if (FD_ISSET(xmpp_sock, &fds))
                {
                    rx_len = 0;
                    memset(xmpp_rx_buf, 0, XMPP_RX_BUF_SIZE);

                    rx_len = recv(xmpp_sock, xmpp_rx_buf, XMPP_RX_BUF_SIZE, 0);
                    if(rx_len > 0)
                    {
                        printf("rx_len=%d\n", rx_len);
                        xmpp_recv(xmpp_rx_buf, rx_len);
                        continue;                        
                    }                  
                    else
                    {                   
                        printf("recv error, rx_len=%d\n", rx_len);
//                        xmpp_err();
                    }
                }
            }
            else if(ret < 0) {
                printf("select error!!\n");
                printf("select ret=%d\n", ret);
//                xmpp_err();
            }

            if (xQueueReceive(xmpp_task_q, (void *)&msg, (portTickType)1/*portMAX_DELAY*/))
            {
                switch(msg)
                {
                 case MSG_SEND_PING:
                     _send_ping(&sess);
                     break;
                 case MSG_XMPP_STOP:
                     printf("xmpp stop!!\n"); 
//                     xmpp_err();
                     break; 
                 default:
                     break;
                }
            }

            continue;
        }
    }

}


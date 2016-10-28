/* iksemel (XML parser for Jabber)
** Copyright (C) 2000-2004 Gurer Ozen
** This code is free software; you can redistribute it and/or
** modify it under the terms of GNU Lesser General Public License.
*/
#include "esp_common.h" 



#include "espconn.h"
#include "xmpp/xmppcli.h"
#include "xmpp/xmppcli_cwmp.h"
#include "iksemel/iksemel.h"
#include "mgmt/mgmt.h"
#include "mgmt/timer.h"
#include "hnt_interface.h"

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

/* stuff we keep per session */
struct session {
    struct espconn *pespconn; 
    /* out packet filter */
    iksfilter *filter;  
	iksparser *prs;
	iksid *acc;
	char *pass;
	char *bound_jid;
	int features;
	int authorized;
	os_timer_t xmpp_inform_timer;
	os_timer_t xmpp_sampling_timer;
	os_timer_t xmpp_ping_timer;
	os_timer_t xmpp_pong_timer;
	int ping_timeout_times;
};

int 
xmpp_send_raw(struct session *sess, char *data, size_t size);
LOCAL void 
user_xmppclient_dns_found(const char *name, ip_addr_t *ipaddr, void *arg);
LOCAL void 
user_xmppclient_connect(struct espconn *pespconn);
int 
user_xmppclient_stop(void);

struct session sess;
LOCAL struct espconn XMPPClientEspconn;
LOCAL struct _esp_tcp xmpp_tcp;
LOCAL ip_addr_t xmpp_server_ip;
LOCAL uint8 xmpp_conn_status = 0;

uint32 local_time_pre = 0;
uint32 remote_time_sec = 0;
uint32 remote_time_msec = 0;
uint32 ICACHE_FLASH_ATTR
get_current_time(void)
{
    if (remote_time_sec == 0)
    {
        //        return system_get_time()/(1000 * 1000) + system_mktime(2015,1,1,1,45,30);
    }
    else
    {
        return remote_time_sec + (system_get_time()-local_time_pre)/(1000*1000);            
    }
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
            log_debug("time_stamp = %s\n", time_stamp);
            memcpy(second_buffer, time_stamp, strlen(time_stamp) - 3);
            memcpy(msec_buffer, time_stamp + (strlen(time_stamp) - 3), 3);
            log_debug("time_stamp = %s\n", time_stamp);
            log_debug("second_buffer = %s\n", second_buffer);
            log_debug("msec_buffer = %s\n", msec_buffer);
            local_time_pre = system_get_time();
            remote_time_sec = atoi(second_buffer);
            remote_time_msec = atoi(msec_buffer);
            log_debug("local_time_pre = %u\n", local_time_pre);
            log_debug("remote_time_sec = %u\n", remote_time_sec);
            log_debug("remote_time_msec = %u\n", remote_time_msec);
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
        log_debug("&ping = %p\n", &ping);
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

    xmpp_send_header(pespconn);   
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

//    log_debug("test:time = %lld\n", system_mktime(2014, 11, 17, 10, 20, 30));
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
    log_debug("system_get_free_heap_size = %d, size = %d\n", system_get_free_heap_size(), size); 

    if (iks_is_secure(sess->prs))
#ifdef HAVE_TLS
        espconn_secure_sent(sess->pespconn, data, size);
#else
        espconn_sent(sess->pespconn, data, size);
#endif
    else
#ifdef CLIENT_SSL_ENABLE
        espconn_secure_sent(sess->pespconn, data, size);
#else
        espconn_sent(sess->pespconn, data, size);
#endif

    log_debug("test\n");

    return IKS_OK;
}

int ICACHE_FLASH_ATTR
xmpp_send_header(struct espconn *pespconn)
{
    return iks_send_header(sess.prs, sess.acc->server);
}


void ICACHE_FLASH_ATTR
xmpp_connect (void *pespconn, char *jabber_id, char *pass)
{
	int e;

	memset (&sess, 0, sizeof (sess));
    log_debug("system_get_free_heap_size = %d\n", system_get_free_heap_size()); 
	sess.prs = iks_stream_new (IKS_NS_CLIENT, &sess, (iksStreamHook *) xmpp_stream_handler);
    log_debug("system_get_free_heap_size = %d\n", system_get_free_heap_size()); 
	sess.pespconn = (struct espconn *)pespconn;
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
user_xmppclient_discon(struct espconn *pespconn)
{
    sint8 ret;
    log_debug("test\n");
    xmpp_conn_status = 0;
#ifdef CLIENT_SSL_ENABLE
    espconn_secure_disconnect(pespconn);
#else 
    if (sess.prs && iks_is_secure(sess.prs))
#ifdef HAVE_TLS
    espconn_secure_disconnect(pespconn);
#else
    espconn_disconnect(pespconn);
#endif   
#endif
}

void ICACHE_FLASH_ATTR
user_xmppclient_restart_cb(void)
{
    user_xmppclient_discon(&XMPPClientEspconn);
    user_xmppclient_disconnect();   

    wifi_led_status_action(WIFI_LED_STATUS_CONNECTING_TO_AP);    
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
    user_xmppclient_discon(&XMPPClientEspconn);
    user_xmppclient_disconnect();   
}

/******************************************************************************
 * FunctionName : user_esp_platform_discon_cb
 * Description  : disconnect successfully with the host
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_xmppclient_discon_cb(void *arg)
{
    struct espconn *pespconn = arg;

    log_debug("user_esp_platform_discon_cb\n");

    if (pespconn == NULL) {
        return;
    }

    user_xmppclient_restart();    
}

/******************************************************************************
 * FunctionName : user_xmppclient_sent
 * Description  : Processing the application data and sending it to the host
 * Parameters   : pespconn -- the espconn used to connetion with the host
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_xmppclient_sent(struct espconn *pespconn)
{
    xmpp_send_header(pespconn);
}

/******************************************************************************
 * FunctionName : user_xmppclient_sent_cb
 * Description  : Data has been sent successfully and acknowledged by the remote host.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_xmppclient_sent_cb(void *arg)
{
    struct espconn *pespconn = arg;

    log_debug("test\n");
}

/******************************************************************************
 * FunctionName : user_xmppclient_recv_cb
 * Description  : Processing the received data from the server
 * Parameters   : arg -- Additional argument to pass to the callback function
 *                pusrdata -- The received data (or NULL when the connection has been closed!)
 *                length -- The length of received data
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_xmppclient_recv_cb(void *arg, char *pusrdata, unsigned short length)
{
    struct espconn *pespconn = arg;
    sint8 ret = 0;

    log_debug(" length:%d\n", length);
    ret = xmpp_recv(pusrdata, length);
    if (ret != 0)
        user_xmppclient_restart();
}

/******************************************************************************
 * FunctionName : user_esp_platform_recon_cb
 * Description  : The connection had an error and is already deallocated.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_xmppclient_recon_cb(void *arg, sint8 err)
{
    struct espconn *pespconn = (struct espconn *)arg;

    printf("xmpp recon\n");
    xmpp_conn_status = 0;
    
    user_xmppclient_restart();
}

/******************************************************************************
 * FunctionName : user_esp_platform_connect_cb
 * Description  : A new incoming connection has been connected.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_xmppclient_connect_cb(void *arg)
{
    wifi_led_status_action(WIFI_LED_STATUS_CONNECTED_TO_SERVER);
    
    struct espconn *pespconn = arg;

    log_debug("system_get_free_heap_size = %d\n", system_get_free_heap_size()); 

    printf("xmpp_connect:[%s][%s]\n",g_hnt_factory_param.xmpp_jid,g_hnt_factory_param.xmpp_password);

    xmpp_connect(&XMPPClientEspconn, 
        g_hnt_factory_param.xmpp_jid, g_hnt_factory_param.xmpp_password);

    log_debug("system_get_free_heap_size = %d\n", system_get_free_heap_size()); 

    espconn_regist_recvcb(pespconn, user_xmppclient_recv_cb);
    espconn_regist_sentcb(pespconn, user_xmppclient_sent_cb);
    user_xmppclient_sent(pespconn);    
}

/******************************************************************************
 * FunctionName : user_esp_platform_connect
 * Description  : The function given as the connect with the host
 * Parameters   : espconn -- the espconn used to connect the connection
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_xmppclient_connect(struct espconn *pespconn)
{
    log_debug("test\n");

#ifdef CLIENT_SSL_ENABLE
    espconn_secure_connect(pespconn);
#else
    espconn_connect(pespconn);
#endif
}

/******************************************************************************
 * FunctionName : user_esp_platform_dns_found
 * Description  : dns found callback
 * Parameters   : name -- pointer to the name that was looked up.
 *                ipaddr -- pointer to an ip_addr_t containing the IP address of
 *                the hostname, or NULL if the name could not be found (or on any
 *                other error).
 *                callback_arg -- a user-specified callback argument passed to
 *                dns_gethostbyname
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_xmppclient_dns_found(const char *name, ip_addr_t *ipaddr, void *arg)
{
    struct espconn *pespconn = (struct espconn *)arg;

    if (ipaddr == NULL) {
        log_debug("NULL\n");
        return;
    }
    log_debug("system_get_free_heap_size = %d\n", system_get_free_heap_size()); 

    log_debug(" %d.%d.%d.%d\n",
            *((uint8 *)&ipaddr->addr), *((uint8 *)&ipaddr->addr + 1),
            *((uint8 *)&ipaddr->addr + 2), *((uint8 *)&ipaddr->addr + 3));

    if (xmpp_server_ip.addr == 0 && ipaddr->addr != 0) {
        xmpp_server_ip.addr = ipaddr->addr;
        memcpy(pespconn->proto.tcp->remote_ip, &ipaddr->addr, 4);

        pespconn->proto.tcp->local_port = espconn_port();

#ifdef CLIENT_SSL_ENABLE
        pespconn->proto.tcp->remote_port = 5223;
#else
        pespconn->proto.tcp->remote_port = 5222;
#endif

        espconn_regist_connectcb(pespconn, user_xmppclient_connect_cb);
        espconn_regist_disconcb(pespconn, user_xmppclient_discon_cb);
        espconn_regist_reconcb(pespconn, user_xmppclient_recon_cb);
        user_xmppclient_connect(pespconn);        
        log_debug("system_get_free_heap_size = %d\n", system_get_free_heap_size()); 
    }
}

LOCAL void ICACHE_FLASH_ATTR
user_xmppclient_start_dns(struct espconn *pespconn)
{
    xmpp_server_ip.addr = 0;
    
    for(;;) 
    {
        printf("espconn_gethostbyname:[%s]\n",g_hnt_factory_param.xmpp_server);
        if (ESPCONN_OK == espconn_gethostbyname(
                pespconn, g_hnt_factory_param.xmpp_server, 
                &xmpp_server_ip, user_xmppclient_dns_found))
        {
            break;
        }
        else
            printf("resolve name server error\n");  

        vTaskDelay(100 / portTICK_RATE_MS);  // 100 ms
    }
    
}

void ICACHE_FLASH_ATTR
hnt_xmppcli_start(void)
{
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
    
    XMPPClientEspconn.proto.tcp = &xmpp_tcp;
    XMPPClientEspconn.type = ESPCONN_TCP;
    XMPPClientEspconn.state = ESPCONN_NONE;

    user_xmppclient_start_dns(&XMPPClientEspconn);
}


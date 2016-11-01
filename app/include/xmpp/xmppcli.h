#ifndef XMPPCLI_H
#define XMPPCLI_H

#ifndef PONG_TIMEOUT
#define PONG_TIMEOUT (3 * 1000) /* 3 seconds */
#endif
#ifndef PING_TIMEOUT
#define PING_TIMEOUT (120 * 1000) /* 120 seconds */
#endif

#ifndef PING_TIMEOUT_TIMES
#define PING_TIMEOUT_TIMES 3 
#endif

#define STANZA_NAME_IQ "iq"
#define STANZA_NAME_PRESENCE "presence"
#define STANZA_NAME_PRIORITY "priority"
#define STANZA_NAME_SHOW "show"
#define STANZA_NAME_STATUS "status"
#define STANZA_NAME_QUERY "query"
#define STANZA_NAME_PING "ping"

#define STANZA_ATTR_TIMESTAMP "timestamp"
#define STANZA_ATTR_DEVICEID "deviceid"
#define STANZA_ATTR_PASSWORD "password"
#define STANZA_ATTR_TO "to"
#define STANZA_ATTR_FROM "from"
#define STANZA_ATTR_ID "id"


#define STANZA_TYPE_SUBSCRIBE "subscribe"
#define STANZA_TYPE_SUBSCRIBED "subscribed"
#define STANZA_TYPE_UNSUBSCRIBED "unsubscribed"
#define STANZA_TYPE_GET "get"
#define STANZA_TYPE_SET "set"
#define STANZA_TYPE_ERROR "error"
#define STANZA_TYPE_RESULT "result"

#define STANZA_NS_PING "urn:xmpp:ping"
#define STANZA_NS_CWMPRPC "jabber:iq:cwmprpc"
#define STANZA_NS_CWMP "urn:dslforum-org:cwmp-1-0"

/* stuff we keep per session */
struct session {
    int xmpp_sock;
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


char *
create_unique_id(char *prefix);
char *
xmpp_conn_get_bound_jid(void);
char *
xmpp_jid_bare(const char *jid);
void 
hnt_xmppcli_start(void);
uint32
get_current_time(void);
#endif

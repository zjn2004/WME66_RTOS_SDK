#if 0
#include "hnt_interface.h"
#include "espconn.h"

/******************************************************************************
 * FunctionName : hntconn_connect
 * Description  : The function given as the connect
 * Parameters   : hntconn -- the hntconn used to listen the connection
 * Returns      : none
*******************************************************************************/

sint8 ICACHE_FLASH_ATTR hntconn_connect(struct hntconn *hntconn)
{
    return espconn_connect((struct espconn *)hntconn);
}

/******************************************************************************
 * FunctionName : hntconn_disconnect
 * Description  : disconnect with host
 * Parameters   : hntconn -- the hntconn used to disconnect the connection
 * Returns      : none
*******************************************************************************/

sint8 ICACHE_FLASH_ATTR hntconn_disconnect(struct hntconn *hntconn)
{
    return espconn_disconnect((struct espconn *)hntconn);
}

/******************************************************************************
 * FunctionName : hntconn_delete
 * Description  : disconnect with host
 * Parameters   : hntconn -- the hntconn used to disconnect the connection
 * Returns      : none
*******************************************************************************/

sint8 ICACHE_FLASH_ATTR hntconn_delete(struct hntconn *hntconn)
{
    return espconn_delete((struct espconn *)hntconn);
}

/******************************************************************************
 * FunctionName : hntconn_accept
 * Description  : The function given as the listen
 * Parameters   : hntconn -- the hntconn used to listen the connection
 * Returns      : none
*******************************************************************************/

sint8 ICACHE_FLASH_ATTR hntconn_accept(struct hntconn *hntconn)
{
    return espconn_accept((struct espconn *)hntconn);
}

/******************************************************************************
 * FunctionName : hntconn_create
 * Description  : sent data for client or server
 * Parameters   : hntconn -- hntconn to the data transmission
 * Returns      : result
*******************************************************************************/

sint8 ICACHE_FLASH_ATTR hntconn_create(struct hntconn *hntconn)
{
    return espconn_create((struct espconn *)hntconn);
}

/******************************************************************************
 * FunctionName : hntconn_tcp_get_max_con
 * Description  : get the number of simulatenously active TCP connections
 * Parameters   : none
 * Returns      : none
*******************************************************************************/

uint8 ICACHE_FLASH_ATTR hntconn_tcp_get_max_con(void)
{
    return espconn_tcp_get_max_con();
}

/******************************************************************************
 * FunctionName : hntconn_tcp_set_max_con
 * Description  : set the number of simulatenously active TCP connections
 * Parameters   : num -- total number
 * Returns      : none
*******************************************************************************/

sint8 ICACHE_FLASH_ATTR hntconn_tcp_set_max_con(uint8 num)
{
    return espconn_tcp_set_max_con(num);
}

/******************************************************************************
 * FunctionName : hntconn_tcp_get_max_con_allow
 * Description  : get the count of simulatenously active connections on the server
 * Parameters   : hntconn -- hntconn to get the count
 * Returns      : result
*******************************************************************************/

sint8 ICACHE_FLASH_ATTR hntconn_tcp_get_max_con_allow(struct hntconn *hntconn)
{
    return espconn_tcp_get_max_con_allow((struct espconn *)hntconn);
}

/******************************************************************************
 * FunctionName : hntconn_tcp_set_max_con_allow
 * Description  : set the count of simulatenously active connections on the server
 * Parameters   : hntconn -- hntconn to set the count
 * 				  num -- support the connection number
 * Returns      : result
*******************************************************************************/

sint8 ICACHE_FLASH_ATTR hntconn_tcp_set_max_con_allow(struct hntconn *hntconn, uint8 num)
{
    return espconn_tcp_set_max_con_allow((struct espconn *)hntconn, num);
}

/******************************************************************************
 * FunctionName : hntconn_regist_time
 * Description  : used to specify the time that should be called when don't recv data
 * Parameters   : hntconn -- the hntconn used to the connection
 * 				  interval -- the timer when don't recv data
 * Returns      : none
*******************************************************************************/

sint8 ICACHE_FLASH_ATTR hntconn_regist_time(struct hntconn *hntconn, uint32 interval, uint8 type_flag)
{
    return espconn_regist_time((struct espconn *)hntconn, interval, type_flag);
}

/******************************************************************************
 * FunctionName : hntconn_get_connection_info
 * Description  : used to specify the function that should be called when disconnect
 * Parameters   : hntconn -- hntconn to set the err callback
 *                discon_cb -- err callback function to call when err
 * Returns      : none
*******************************************************************************/

sint8 ICACHE_FLASH_ATTR hntconn_get_connection_info(struct hntconn *phntconn, hnt_remot_info **pcon_info, uint8 typeflags)
{
    return espconn_get_connection_info((struct espconn *)phntconn, (remot_info **)pcon_info, typeflags);
}

/******************************************************************************
 * FunctionName : hntconn_get_packet_info
 * Description  : get the packet info with host
 * Parameters   : hntconn -- the hntconn used to disconnect the connection
 * 				  infoarg -- the packet info
 * Returns      : the errur code
*******************************************************************************/

sint8 ICACHE_FLASH_ATTR hntconn_get_packet_info(struct hntconn *hntconn, struct hntconn_packet* infoarg)
{
    return espconn_get_packet_info((struct espconn *)hntconn, (struct espconn_packet *)infoarg);
}

/******************************************************************************
 * FunctionName : hntconn_regist_sentcb
 * Description  : Used to specify the function that should be called when data
 *                has been successfully delivered to the remote host.
 * Parameters   : struct hntconn *hntconn -- hntconn to set the sent callback
 *                hntconn_sent_callback sent_cb -- sent callback function to
 *                call for this hntconn when data is successfully sent
 * Returns      : none
*******************************************************************************/

sint8 ICACHE_FLASH_ATTR hntconn_regist_sentcb(struct hntconn *hntconn, hntconn_sent_callback sent_cb)
{
    return espconn_regist_sentcb((struct espconn *)hntconn, (espconn_sent_callback)sent_cb);
}

/******************************************************************************
 * FunctionName : hntconn_regist_sentcb
 * Description  : Used to specify the function that should be called when data
 *                has been successfully delivered to the remote host.
 * Parameters   : hntconn -- hntconn to set the sent callback
 *                sent_cb -- sent callback function to call for this hntconn
 *                when data is successfully sent
 * Returns      : none
*******************************************************************************/

sint8 ICACHE_FLASH_ATTR hntconn_regist_write_finish(struct hntconn *hntconn, hntconn_connect_callback write_finish_fn)
{
    return espconn_regist_write_finish((struct espconn *)hntconn, (espconn_connect_callback)write_finish_fn);
}

/******************************************************************************
 * FunctionName : hntconn_sent
 * Description  : sent data for client or server
 * Parameters   : hntconn -- hntconn to set for client or server
 *                psent -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/

sint8 ICACHE_FLASH_ATTR hntconn_sent(struct hntconn *hntconn, uint8 *psent, uint16 length)
{
    return espconn_sent((struct espconn *)hntconn, psent, length);
}

/******************************************************************************
 * FunctionName : hntconn_regist_connectcb
 * Description  : used to specify the function that should be called when
 *                connects to host.
 * Parameters   : hntconn -- hntconn to set the connect callback
 *                connect_cb -- connected callback function to call when connected
 * Returns      : none
*******************************************************************************/

sint8 ICACHE_FLASH_ATTR hntconn_regist_connectcb(struct hntconn *hntconn, hntconn_connect_callback connect_cb)
{
    return espconn_regist_connectcb((struct espconn *)hntconn, (espconn_connect_callback)connect_cb);
}

/******************************************************************************
 * FunctionName : hntconn_regist_recvcb
 * Description  : used to specify the function that should be called when recv
 *                data from host.
 * Parameters   : hntconn -- hntconn to set the recv callback
 *                recv_cb -- recv callback function to call when recv data
 * Returns      : none
*******************************************************************************/

sint8 ICACHE_FLASH_ATTR hntconn_regist_recvcb(struct hntconn *hntconn, hntconn_recv_callback recv_cb)
{
    return espconn_regist_recvcb((struct espconn *)hntconn, (espconn_recv_callback)recv_cb);
}

/******************************************************************************
 * FunctionName : hntconn_regist_reconcb
 * Description  : used to specify the function that should be called when connection
 *                because of err disconnect.
 * Parameters   : hntconn -- hntconn to set the err callback
 *                recon_cb -- err callback function to call when err
 * Returns      : none
*******************************************************************************/

sint8 ICACHE_FLASH_ATTR hntconn_regist_reconcb(struct hntconn *hntconn, hntconn_reconnect_callback recon_cb)
{
    return espconn_regist_reconcb((struct espconn *)hntconn, (espconn_reconnect_callback)recon_cb);
}

/******************************************************************************
 * FunctionName : hntconn_regist_disconcb
 * Description  : used to specify the function that should be called when disconnect
 * Parameters   : hntconn -- hntconn to set the err callback
 *                discon_cb -- err callback function to call when err
 * Returns      : none
*******************************************************************************/

sint8 ICACHE_FLASH_ATTR hntconn_regist_disconcb(struct hntconn *hntconn, hntconn_connect_callback discon_cb)
{
    return espconn_regist_disconcb((struct espconn *)hntconn, (espconn_connect_callback)discon_cb);
}

/******************************************************************************
 * FunctionName : hntconn_port
 * Description  : access port value for client so that we don't end up bouncing
 *                all connections at the same time .
 * Parameters   : none
 * Returns      : access port value
*******************************************************************************/

uint32 ICACHE_FLASH_ATTR hntconn_port(void)
{
    return espconn_port();
}

/******************************************************************************
 * FunctionName : hntconn_set_opt
 * Description  : access port value for client so that we don't end up bouncing
 *                all connections at the same time .
 * Parameters   : none
 * Returns      : access port value
*******************************************************************************/

sint8 ICACHE_FLASH_ATTR hntconn_set_opt(struct hntconn *hntconn, uint8 opt)
{
    return espconn_set_opt((struct espconn *)hntconn, opt);
}

/******************************************************************************
 * FunctionName : hntconn_clear_opt
 * Description  : clear the option for connections so that we don't end up bouncing
 *                all connections at the same time .
 * Parameters   : hntconn -- the hntconn used to set the connection
 * 				  opt -- the option for clear
 * Returns      : the result
*******************************************************************************/

sint8 ICACHE_FLASH_ATTR hntconn_clear_opt(struct hntconn *hntconn, uint8 opt)
{
    return espconn_clear_opt((struct espconn *)hntconn, opt);
}

/******************************************************************************
 * FunctionName : hntconn_set_keepalive
 * Description  : access level value for connection so that we set the value for
 * 				  keep alive
 * Parameters   : hntconn -- the hntconn used to set the connection
 * 				  level -- the connection's level
 * 				  value -- the value of time(s)
 * Returns      : access port value
*******************************************************************************/

sint8 ICACHE_FLASH_ATTR hntconn_set_keepalive(struct hntconn *hntconn, uint8 level, void* optarg)
{
    return espconn_set_keepalive((struct espconn *)hntconn, level, optarg);
}

/******************************************************************************
 * FunctionName : hntconn_get_keepalive
 * Description  : access level value for connection so that we get the value for
 * 				  keep alive
 * Parameters   : hntconn -- the hntconn used to get the connection
 * 				  level -- the connection's level
 * Returns      : access keep alive value
*******************************************************************************/

sint8 ICACHE_FLASH_ATTR hntconn_get_keepalive(struct hntconn *hntconn, uint8 level, void *optarg)
{
    return espconn_get_keepalive((struct espconn *)hntconn, level, optarg);
}

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

//typedef void (*dns_found_callback)(const char *name, ip_addr_t *ipaddr, void *callback_arg);

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

err_t ICACHE_FLASH_ATTR hntconn_gethostbyname(struct hntconn *phntconn, const char *hostname, ip_addr_t *addr, hnt_dns_found_callback found)
{
    return espconn_gethostbyname((struct espconn *)phntconn, hostname, addr, (dns_found_callback)found);
}

/******************************************************************************
 * FunctionName : hntconn_encry_connect
 * Description  : The function given as connection
 * Parameters   : hntconn -- the hntconn used to connect with the host
 * Returns      : none
*******************************************************************************/

sint8 ICACHE_FLASH_ATTR hntconn_secure_connect(struct hntconn *hntconn)
{
    return espconn_secure_connect((struct espconn *)hntconn);
}

/******************************************************************************
 * FunctionName : hntconn_encry_disconnect
 * Description  : The function given as the disconnection
 * Parameters   : hntconn -- the hntconn used to disconnect with the host
 * Returns      : none
*******************************************************************************/
sint8 ICACHE_FLASH_ATTR hntconn_secure_disconnect(struct hntconn *hntconn)
{
    return espconn_secure_disconnect((struct espconn *)hntconn);
}

/******************************************************************************
 * FunctionName : hntconn_encry_sent
 * Description  : sent data for client or server
 * Parameters   : hntconn -- hntconn to set for client or server
 * 				  psent -- data to send
 *                length -- length of data to send
 * Returns      : none
*******************************************************************************/

sint8 ICACHE_FLASH_ATTR hntconn_secure_sent(struct hntconn *hntconn, uint8 *psent, uint16 length)
{
    return espconn_secure_sent((struct espconn *)hntconn, psent, length);
}

/******************************************************************************
 * FunctionName : hntconn_secure_set_size
 * Description  : set the buffer size for client or server
 * Parameters   : level -- set for client or server
 * 				  1: client,2:server,3:client and server
 * 				  size -- buffer size
 * Returns      : true or false
*******************************************************************************/

bool ICACHE_FLASH_ATTR hntconn_secure_set_size(uint8 level, uint16 size)
{
    return hntconn_secure_set_size(level, size);
}

/******************************************************************************
 * FunctionName : hntconn_secure_get_size
 * Description  : get buffer size for client or server
 * Parameters   : level -- set for client or server
 *				  1: client,2:server,3:client and server
 * Returns      : buffer size for client or server
*******************************************************************************/

sint16 ICACHE_FLASH_ATTR hntconn_secure_get_size(uint8 level)
{
    return hntconn_secure_get_size(level);
}

/******************************************************************************
 * FunctionName : hntconn_secure_accept
 * Description  : The function given as the listen
 * Parameters   : hntconn -- the hntconn used to listen the connection
 * Returns      : none
*******************************************************************************/

sint8 ICACHE_FLASH_ATTR hntconn_secure_accept(struct hntconn *hntconn)
{
    return espconn_secure_accept((struct espconn *)hntconn);
}

/******************************************************************************
 * FunctionName : hntconn_igmp_join
 * Description  : join a multicast group
 * Parameters   : host_ip -- the ip address of udp server
 * 				  multicast_ip -- multicast ip given by user
 * Returns      : none
*******************************************************************************/
sint8 ICACHE_FLASH_ATTR hntconn_igmp_join(ip_addr_t *host_ip, ip_addr_t *multicast_ip)
{
    return espconn_igmp_join(host_ip, multicast_ip);
}

/******************************************************************************
 * FunctionName : hntconn_igmp_leave
 * Description  : leave a multicast group
 * Parameters   : host_ip -- the ip address of udp server
 * 				  multicast_ip -- multicast ip given by user
 * Returns      : none
*******************************************************************************/
sint8 ICACHE_FLASH_ATTR hntconn_igmp_leave(ip_addr_t *host_ip, ip_addr_t *multicast_ip)
{
    return espconn_igmp_leave(host_ip, multicast_ip);
}

/******************************************************************************
 * FunctionName : hntconn_recv_hold
 * Description  : hold tcp receive
 * Parameters   : hntconn -- hntconn to hold
 * Returns      : none
*******************************************************************************/
sint8 ICACHE_FLASH_ATTR hntconn_recv_hold(struct hntconn *phntconn)
{
    return espconn_recv_hold((struct espconn *)phntconn);
}

/******************************************************************************
 * FunctionName : hntconn_recv_unhold
 * Description  : unhold tcp receive
 * Parameters   : hntconn -- hntconn to unhold
 * Returns      : none
*******************************************************************************/
sint8 ICACHE_FLASH_ATTR hntconn_recv_unhold(struct hntconn *phntconn)
{
    return espconn_recv_unhold((struct espconn *)phntconn);
}

/******************************************************************************
 * FunctionName : hntconn_mdns_init
 * Description  : register a device with mdns
 * Parameters   : ipAddr -- the ip address of device
 * 				  hostname -- the hostname of device
 * Returns      : none
*******************************************************************************/

void ICACHE_FLASH_ATTR hntconn_mdns_init(struct hnt_mdns_info *info)
{
    espconn_mdns_init((struct mdns_info *)info);
}
/******************************************************************************
 * FunctionName : hntconn_mdns_close
 * Description  : close a device with mdns
 * Parameters   : a
 * Returns      : none
*******************************************************************************/

void ICACHE_FLASH_ATTR hntconn_mdns_close(void)
{
    espconn_mdns_close();
}
/******************************************************************************
 * FunctionName : hntconn_mdns_server_register
 * Description  : register a device with mdns
 * Parameters   : a
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR hntconn_mdns_server_register(void)
{
    espconn_mdns_server_register();
}

/******************************************************************************
 * FunctionName : hntconn_mdns_server_unregister
 * Description  : unregister a device with mdns
 * Parameters   : a
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR hntconn_mdns_server_unregister(void)
{
    espconn_mdns_server_unregister();
}

/******************************************************************************
 * FunctionName : hntconn_mdns_get_servername
 * Description  : get server name of device with mdns
 * Parameters   : a
 * Returns      : none
*******************************************************************************/

char* ICACHE_FLASH_ATTR hntconn_mdns_get_servername(void)
{
    return espconn_mdns_get_servername();
}
/******************************************************************************
 * FunctionName : hntconn_mdns_set_servername
 * Description  : set server name of device with mdns
 * Parameters   : a
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR hntconn_mdns_set_servername(const char *name)
{
    espconn_mdns_set_servername(name);
}

/******************************************************************************
 * FunctionName : hntconn_mdns_set_hostname
 * Description  : set host name of device with mdns
 * Parameters   : a
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR hntconn_mdns_set_hostname(char *name)
{
    espconn_mdns_set_servername(name);
}

/******************************************************************************
 * FunctionName : hntconn_mdns_get_hostname
 * Description  : get host name of device with mdns
 * Parameters   : a
 * Returns      : none
*******************************************************************************/
char* ICACHE_FLASH_ATTR hntconn_mdns_get_hostname(void)
{
    return espconn_mdns_get_hostname();
}

/******************************************************************************
 * FunctionName : hntconn_mdns_disable
 * Description  : disable a device with mdns
 * Parameters   : a
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR hntconn_mdns_disable(void)
{
    espconn_mdns_disable();
}

/******************************************************************************
 * FunctionName : hntconn_mdns_enable
 * Description  : disable a device with mdns
 * Parameters   : a
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR hntconn_mdns_enable(void)
{
    espconn_mdns_enable();
}
/******************************************************************************
 * FunctionName : hntconn_dns_setserver
 * Description  : Initialize one of the DNS servers.
 * Parameters   : numdns -- the index of the DNS server to set must
 * 				  be < DNS_MAX_SERVERS = 2
 * 			      dnsserver -- IP address of the DNS server to set
 *  Returns     : none
*******************************************************************************/
void ICACHE_FLASH_ATTR hntconn_dns_setserver(char numdns, ip_addr_t *dnsserver)
{
    espconn_dns_setserver(numdns, dnsserver);
}
#endif

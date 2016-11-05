/*
 * ESPRSSIF MIT License
 *
 * Copyright (c) 2015 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "esp_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "driver/uart.h"

#include "hnt_interface.h"

static void ICACHE_FLASH_ATTR wifi_event_hand_function(System_Event_t * event) 
{  
    u32_t addr;

    switch (event->event_id) 
    {  
        case EVENT_STAMODE_CONNECTED:     
            os_printf("EVENT_STAMODE_CONNECTED ssid:%s,channel %d\n",
                event->event_info.connected.ssid,event->event_info.connected.channel);     
            break;
        case EVENT_STAMODE_DISCONNECTED:        
            os_printf("EVENT_STAMODE_DISCONNECTED ssid:%s,reason %d\n",
                event->event_info.disconnected.ssid,event->event_info.disconnected.reason);     
//            wifi_led_set_status(WIFI_LED_CONNECTING_AP);        
            break;  
        case EVENT_STAMODE_AUTHMODE_CHANGE:     
            break;  
        case EVENT_STAMODE_GOT_IP:     
            addr = ntohl(event->event_info.got_ip.ip.addr);
            os_printf("EVENT_STAMODE_GOT_IP %d.%d.%d.%d\n", 
                (addr>>24)&0x000000ff,
                (addr>>16)&0x000000ff,
                (addr>>8)&0x000000ff,
                (addr>>0)&0x000000ff);        
//            wifi_led_set_status(WIFI_LED_CONNECTED_AP);
            break;  
        case EVENT_STAMODE_DHCP_TIMEOUT:    
            os_printf("EVENT_STAMODE_DHCP_TIMEOUT\n");     
            break;  
        default:        
            break;  
    }
}


/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_init(void)
{
    struct hnt_mgmt_factory_param factory_param;
    
    user_custom_init();

    printf("\nSDK version:%s\n", system_get_sdk_version());

    memset(&factory_param,0,sizeof(struct hnt_mgmt_factory_param));
    sprintf(factory_param.device_type,"4013");
    sprintf(factory_param.device_id,"15414013000001");
    sprintf(factory_param.password,"74914252");
    sprintf(factory_param.xmpp_server,"xmpp.seaing.net");
    sprintf(factory_param.xmpp_jid,"d15414013000001@seaing.net");
    sprintf(factory_param.xmpp_password,"H93wwzeCXms");

    hnt_mgmt_factory_init(&factory_param);

	wifi_set_event_handler_cb(wifi_event_hand_function);

    /* need to set opmode before you set config */
    wifi_set_opmode(STATION_MODE);
    
    char sta_mac[6] = {0xC4, 0xCD, 0x45, 0x21, 0x16, 0xC7};
    wifi_set_macaddr(STATION_IF, sta_mac);
    
    hnt_platform_init();
}


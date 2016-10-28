/******************************************************************************
  * Copyright (c) 2014 Seaing, Ltd.
  * All Rights Reserved. 
  * Seaing Confidential and Proprietary.
*******************************************************************************/
#include "esp_common.h"

#include "hnt_interface.h"
#include "mgmt/mgmt.h"
#include "xmpp/xmppcli.h"

#define DEVICE_RESET_MAX_COUNT 10
LOCAL os_timer_t client_timer;
LOCAL uint8 device_recon_count = 0;


void ICACHE_FLASH_ATTR xmppcli_task(void *pvParameters) 
{
	os_printf("heap_size %d\n", system_get_free_heap_size());
	while (1) {
		int ret = wifi_station_get_connect_status();   // wait for sys wifi connected OK.
		if (ret == STATION_GOT_IP)
			break;
		vTaskDelay(100 / portTICK_RATE_MS);	 // 100 ms
	}
    
	hnt_xmppcli_start();
	vTaskDelete(NULL);
}


void ICACHE_FLASH_ATTR
hnt_platform_init(void)
{
    struct station_config sta_conf;
    wifi_station_get_config(&sta_conf);
    if(strlen(sta_conf.ssid) == 0 && strlen(sta_conf.password) == 0) 
    {
        wifi_led_status_action(WIFI_LED_STATUS_RECEIVE_CONFIG);
        bzero(&sta_conf, sizeof(struct station_config));
        
        sprintf(sta_conf.ssid, "TP-LINK_A04F");
        sprintf(sta_conf.password, "12345678");
        wifi_station_set_config(&sta_conf);
        os_printf("%s\n", __func__);

    }
    else
    {
        wifi_led_status_action(WIFI_LED_STATUS_CONNECTING_TO_AP);
    }
    
	xTaskCreate(xmppcli_task, "xmppcli_task",(256*4), NULL, 2, NULL);    
}


/******************************************************************************
  * Copyright (c) 2014 Seaing, Ltd.
  * All Rights Reserved. 
  * Seaing Confidential and Proprietary.
*******************************************************************************/
#include "esp_common.h"
#include "iksemel/iksemel.h"

#include "hnt_interface.h"
#include "mgmt/mgmt.h"
#include "xmpp/xmppcli.h"
#include "ledctl/ledctl.h"

void ICACHE_FLASH_ATTR xmppcli_task(void *pvParameters) 
{
    os_printf("%s\n", __func__);

	os_printf("heap_size %d\n", system_get_free_heap_size());
	while (1) {
		int ret = wifi_station_get_connect_status();   // wait for sys wifi connected OK.
		if (ret == STATION_GOT_IP)
			break;
		vTaskDelay(100 / portTICK_RATE_MS);	 // 100 ms
	}
    
    hnt_wifi_led_status_action(WIFI_LED_ON);
    
	hnt_xmppcli_start();
    
	vTaskDelete(NULL);
}


void ICACHE_FLASH_ATTR
hnt_platform_init(void)
{
    struct station_config sta_conf;

    hnt_wifi_led_status_action(WIFI_LED_BLINK_SLOW);
    bzero(&sta_conf, sizeof(struct station_config));
    
    sprintf(sta_conf.ssid, "TP-LINK_A04F");
    sprintf(sta_conf.password, "12345678");
    wifi_station_set_config(&sta_conf);
    os_printf("%s\n", __func__);

	xTaskCreate(xmppcli_task, "xmppcli_task",(256*6), NULL, 2, NULL); 
}



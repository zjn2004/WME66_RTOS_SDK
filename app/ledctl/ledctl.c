#include "esp_common.h"

#include "ledctl/ledctl.h"

hnt_wifi_led_func wifi_led_status_func = NULL;

void ICACHE_FLASH_ATTR
hnt_wifi_led_status_action(int status)
{
    if(wifi_led_status_func != NULL)
        wifi_led_status_func(status);
}

void ICACHE_FLASH_ATTR
hnt_wifi_led_func_regist(void *func)
{
    wifi_led_status_func = (hnt_wifi_led_func)func;
}

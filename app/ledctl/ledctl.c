#include "esp_common.h"

#include "ledctl/ledctl.h"

#define WIFI_LED_BLINK_FAST     3
#define WIFI_LED_BLINK_SLOW     2
#define WIFI_LED_ON             1
#define WIFI_LED_OFF            0

hnt_wifi_led_func wifi_led_status_func = NULL;
void ICACHE_FLASH_ATTR
wifi_led_on(void)
{    
    if(wifi_led_status_func)
        wifi_led_status_func(WIFI_LED_ON);
}

void ICACHE_FLASH_ATTR
wifi_led_off(void)
{
    if(wifi_led_status_func)
        wifi_led_status_func(WIFI_LED_OFF);
}

void ICACHE_FLASH_ATTR
wifi_led_blink(int on_time, int off_time)
{
    if(wifi_led_status_func == NULL)
        return;

    if(on_time < 400)
        wifi_led_status_func(WIFI_LED_BLINK_FAST);
    else 
        wifi_led_status_func(WIFI_LED_BLINK_SLOW);
}

void ICACHE_FLASH_ATTR
wifi_led_status_action(int status)
{
    if(wifi_led_status_func != NULL)
        wifi_led_status_func(status);
}

void ICACHE_FLASH_ATTR
hnt_wifi_led_func_regist(void *func)
{
    wifi_led_status_func = (hnt_wifi_led_func)func;
}

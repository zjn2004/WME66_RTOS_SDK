#ifndef _LEDCTL_H_
#define _LEDCTL_H_

#define WIFI_LED_BLINK_FAST     3
#define WIFI_LED_BLINK_SLOW     2
#define WIFI_LED_ON             1
#define WIFI_LED_OFF            0

typedef void (*hnt_wifi_led_func)(unsigned char);

void ICACHE_FLASH_ATTR
hnt_wifi_led_status_action(int status);

void ICACHE_FLASH_ATTR
hnt_wifi_led_func_regist(void *func);

#endif /*_LEDCTL_H_*/
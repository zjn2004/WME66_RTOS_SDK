#ifndef _LEDCTL_H_
#define _LEDCTL_H_


typedef void (*hnt_wifi_led_func)(unsigned char);
void wifi_led_on(void);
void wifi_led_off(void);
void wifi_led_blink(int on_time, int off_time);

#endif /*_LEDCTL_H_*/
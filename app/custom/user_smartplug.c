#include "user_config.h"

#include "esp_common.h"
#include "custom/user_smartplug.h"
#include "hnt_interface.h"

#include "driver/key.h"
#include "driver/gpio.h"
#include "driver/uart.h"

#include "ledctl/ledctl.h"

#if USER_SMARTPLUG

LOCAL uint8 smartplug_switch_status = 0;

LOCAL os_timer_t smartplug_status_led_timer;
LOCAL uint8_t wifi_led_status = 0;

LOCAL struct single_key_param *single_key[SMARTPLUG_KEY_NUM];
LOCAL struct keys_param keys;
LOCAL uint8 key_long_press_flag = 0;

void ICACHE_FLASH_ATTR
smartplug_power_led_status(unsigned char power_led_status)
{
    if(power_led_status)//on
        GPIO_OUTPUT_SET(GPIO_ID_PIN(SMARTPLUG_POWER_LED_GPIO), GPIO_VALUE_0);
    else//off
        GPIO_OUTPUT_SET(GPIO_ID_PIN(SMARTPLUG_POWER_LED_GPIO), GPIO_VALUE_1);
}

int ICACHE_FLASH_ATTR
eSmartPlugGetPower(char *paramName, char *value, int size)
{        
    sprintf(value, "%d", smartplug_switch_status);

    return 0;
}

int ICACHE_FLASH_ATTR
eSmartPlugSetPower(char *paramName, char *value)
{
    smartplug_switch_status = atoi(value);   
    smartplug_switch_status = (smartplug_switch_status? GPIO_VALUE_1 : GPIO_VALUE_0);
    
    GPIO_OUTPUT_SET(SMARTPLUG_CTRL_IO_GPIO, smartplug_switch_status);
    if(smartplug_switch_status)
        smartplug_power_led_status(1);
    else
        smartplug_power_led_status(0);

    return 0;    
}

void ICACHE_FLASH_ATTR
smartplug_gpio_status_init(void)
{   
	GPIO_ConfigTypeDef pGPIOConfig;
	pGPIOConfig.GPIO_IntrType = GPIO_PIN_INTR_DISABLE;
	pGPIOConfig.GPIO_Pullup = GPIO_PullUp_EN;
	pGPIOConfig.GPIO_Mode = GPIO_Mode_Output;
    
	pGPIOConfig.GPIO_Pin = GPIO_Pin_12;	//power led
	gpio_config(&pGPIOConfig);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(12), 1);	//close power led
	
	pGPIOConfig.GPIO_Pin = GPIO_Pin_13;	//wifi led
	gpio_config(&pGPIOConfig);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(13), 1);	//close wifi led   
	
	pGPIOConfig.GPIO_Pin = GPIO_Pin_15;	//relay control
	gpio_config(&pGPIOConfig);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(15), 0);	//close relay control	
}

void ICACHE_FLASH_ATTR
smartplug_wifi_status_led_cb(void)
{
    wifi_led_status = (wifi_led_status ? GPIO_VALUE_0 : GPIO_VALUE_1);
    GPIO_OUTPUT_SET(GPIO_ID_PIN(SMARTPLUG_WIFI_LED_GPIO), wifi_led_status);
}

void ICACHE_FLASH_ATTR
smartplug_wifi_status_led(uint8 led_setting)
{
	os_printf("[%s][%d] wifi led %d \n\r",__FUNCTION__,__LINE__,led_setting);

    if(led_setting == WIFI_LED_OFF)
    {
        os_timer_disarm(&smartplug_status_led_timer);          
        wifi_led_status = 0;
        GPIO_OUTPUT_SET(GPIO_ID_PIN(SMARTPLUG_WIFI_LED_GPIO), GPIO_VALUE_1);
    } 
    else if(led_setting == WIFI_LED_BLINK_FAST)
    {
        os_timer_disarm(&smartplug_status_led_timer);          
        os_timer_setfn(&smartplug_status_led_timer, (os_timer_func_t *)smartplug_wifi_status_led_cb, NULL);
        os_timer_arm(&smartplug_status_led_timer, 250, 1);
    }
    else if(led_setting == WIFI_LED_BLINK_SLOW)
    {
        os_timer_disarm(&smartplug_status_led_timer);          
        os_timer_setfn(&smartplug_status_led_timer, (os_timer_func_t *)smartplug_wifi_status_led_cb, NULL);
        os_timer_arm(&smartplug_status_led_timer, 500, 1);
    }
    else if(led_setting == WIFI_LED_ON)
    {
        os_timer_disarm(&smartplug_status_led_timer);          
        wifi_led_status = 1;
        GPIO_OUTPUT_SET(GPIO_ID_PIN(SMARTPLUG_WIFI_LED_GPIO), GPIO_VALUE_0);
    }    
}

LOCAL void ICACHE_FLASH_ATTR
smartplug_key_short_press(void)
{
	os_printf("[%s][%d]\n\r",__FUNCTION__,__LINE__);

    if(key_long_press_flag) 
    {           
        key_long_press_flag = 0;           
        return ;       
    }

    if(smartplug_switch_status)
        eSmartPlugSetPower(NULL, "0");        
    else
        eSmartPlugSetPower(NULL, "1");
    
//    hnt_device_status_change();
}

LOCAL void ICACHE_FLASH_ATTR
smartplug_key_long_press(void)
{
    os_printf("[%s][%d]\n\r",__FUNCTION__,__LINE__);
    
    key_long_press_flag = 1;           

    system_restore();
    vTaskDelay(100);
    system_restart();
}


void ICACHE_FLASH_ATTR
smartplug_key_button_init(void)
{
	single_key[0] = key_init_single(SMARTPLUG_KEY1_IO_GPIO, SMARTPLUG_KEY1_IO_MUX, SMARTPLUG_KEY1_IO_FUNC,
                                	smartplug_key_long_press, smartplug_key_short_press);

	keys.key_num = SMARTPLUG_KEY_NUM;
	keys.single_key = single_key;
	key_init(&keys);
}

customInfo_t customInfo =
{
    SMARTPLUG_DEVICEINFO_SOFTWARE_VERSION,
    SMARTPLUG_DEVICEINFO_MODEL_NAME,
    SMARTPLUG_DEVICEINFO_MANUFACTURER,
    SMARTPLUG_XMPP_INFORM_INTERVAL_SEC,
    SMARTPLUG_XMPP_SAMPLING_INTERVAL_SEC
};


deviceParameter_t DeviceParamList[] = {
{HNT_XMPP_PATH_FLAG_NORMAL,SMARTPLUG_PATH_DEVICE_SWITCH, eSmartPlugGetPower, eSmartPlugSetPower,NULL}
};

void
user_custom_init(void)
{        
    uart_init_new(BIT_RATE_115200,BIT_RATE_115200,UART0,FALSE);

	os_printf("%s,%d\n", __FUNCTION__,__LINE__);
    
    hnt_xmpp_custom_info_regist(&customInfo);
    hnt_xmpp_param_array_regist(&DeviceParamList[0], sizeof(DeviceParamList)/sizeof(DeviceParamList[0]));

    hnt_wifi_led_func_regist(smartplug_wifi_status_led);

    smartplug_gpio_status_init();
    smartplug_key_button_init();
}
#endif

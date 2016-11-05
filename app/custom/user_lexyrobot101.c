#include "user_config.h"

#include "esp_common.h"

#include "user_uart/user_uart.h"
#include "hnt_interface.h"
#include "custom/user_lexyrobot101.h"
#include "ledctl/ledctl.h"
#include "driver/gpio.h"

#if USER_LEXY_ROBOT_R101

static uart_get_lexy_param_t gLexyAllParam;

void ICACHE_FLASH_ATTR
lexyrobot101_gpio_status_init(void)
{   
	GPIO_ConfigTypeDef pGPIOConfig;
	pGPIOConfig.GPIO_IntrType = GPIO_PIN_INTR_DISABLE;
	pGPIOConfig.GPIO_Pullup = GPIO_PullUp_EN;
	pGPIOConfig.GPIO_Mode = GPIO_Mode_Output;
	
	pGPIOConfig.GPIO_Pin = GPIO_Pin_12;	//wifi led
	gpio_config(&pGPIOConfig);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(LEXYROBOT101_WIFI_LED_GPIO), 0);	//close wifi led   
}

LOCAL os_timer_t lexyrobot101_status_led_timer;
LOCAL uint8_t wifi_led_status = 0;
LOCAL uint8_t wifi_led_blink_index = 0;
LOCAL uint8_t g_led_setting = 0;

void ICACHE_FLASH_ATTR
lexyrobot101_wifi_status_led_cb(void *arg)
{
    uint8 led_setting = *(uint8 *)arg;

    if(led_setting == WIFI_LED_SMARTCONFIG)
    {
        wifi_led_status = (wifi_led_status ? GPIO_VALUE_0 : GPIO_VALUE_1);
        GPIO_OUTPUT_SET(GPIO_ID_PIN(LEXYROBOT101_WIFI_LED_GPIO), wifi_led_status);
    }
    else if(led_setting == WIFI_LED_CONNECTING_AP)
    {
        if(wifi_led_blink_index%4 == 2)
            GPIO_OUTPUT_SET(GPIO_ID_PIN(LEXYROBOT101_WIFI_LED_GPIO), GPIO_VALUE_1);
        else
            GPIO_OUTPUT_SET(GPIO_ID_PIN(LEXYROBOT101_WIFI_LED_GPIO), GPIO_VALUE_0);
            
        wifi_led_blink_index++;        
    }
    else if(led_setting == WIFI_LED_CONNECTED_AP)
    {
        if((wifi_led_blink_index%8 == 2)
            ||(wifi_led_blink_index%8 == 4))
            GPIO_OUTPUT_SET(GPIO_ID_PIN(LEXYROBOT101_WIFI_LED_GPIO), GPIO_VALUE_1);
        else
            GPIO_OUTPUT_SET(GPIO_ID_PIN(LEXYROBOT101_WIFI_LED_GPIO), GPIO_VALUE_0);
            
        wifi_led_blink_index++;    
    }
}

void ICACHE_FLASH_ATTR
lexyrobot101_wifi_status_led(uint8 led_setting)
{
	os_printf("[%s][%d] wifi led %d \n\r",__FUNCTION__,__LINE__,led_setting);
    g_led_setting = led_setting;

    if(led_setting == WIFI_LED_OFF)
    {
        os_timer_disarm(&lexyrobot101_status_led_timer);          
        wifi_led_status = 0;
        GPIO_OUTPUT_SET(GPIO_ID_PIN(LEXYROBOT101_WIFI_LED_GPIO), GPIO_VALUE_0);
    } 
    else if(led_setting == WIFI_LED_SMARTCONFIG)
    {
        os_timer_disarm(&lexyrobot101_status_led_timer);          
        os_timer_setfn(&lexyrobot101_status_led_timer, (os_timer_func_t *)lexyrobot101_wifi_status_led_cb, &g_led_setting);
        os_timer_arm(&lexyrobot101_status_led_timer, 250, 1);
    }
    else if(led_setting == WIFI_LED_CONNECTING_AP)
    {
        os_timer_disarm(&lexyrobot101_status_led_timer);          
        os_timer_setfn(&lexyrobot101_status_led_timer, (os_timer_func_t *)lexyrobot101_wifi_status_led_cb, &g_led_setting);
        os_timer_arm(&lexyrobot101_status_led_timer, 500, 1);
    }
    else if(led_setting == WIFI_LED_CONNECTED_AP)
    {
        os_timer_disarm(&lexyrobot101_status_led_timer);          
        os_timer_setfn(&lexyrobot101_status_led_timer, (os_timer_func_t *)lexyrobot101_wifi_status_led_cb, &g_led_setting);
        os_timer_arm(&lexyrobot101_status_led_timer, 500, 1);        
    }
    else if(led_setting == WIFI_LED_CONNECTED_SERVER)
    {
        os_timer_disarm(&lexyrobot101_status_led_timer);          
        wifi_led_status = 1;
        GPIO_OUTPUT_SET(GPIO_ID_PIN(LEXYROBOT101_WIFI_LED_GPIO), GPIO_VALUE_1);
    }    
}

static int eLexyRobotWifiFactoryResetRsp(u8 msg_seq)
{
    int ret;
    msg_set_common_rsp_t msg;

    memset(&msg, 0, sizeof(msg));
    msg.hdr.op = OP_EVT_ACK;
    msg.hdr.seq = msg_seq;
    msg.hdr.type = TYPE_EVENT_WIFI_RESET;
    msg.hdr.len = DATA_LEN(msg);
    msg.ret = 0;
    
    ret = uart_tx_without_rsp((char*)&msg, 
                      sizeof(msg));
    
    if(ret != OP_OK) return Internal_Error;
    
    return 0;
}

LOCAL void ICACHE_FLASH_ATTR
system_factory_reset(void)
{
    os_printf("[%s][%d]\n\r",__FUNCTION__,__LINE__);

    setSmartConfigFlag(1);   // set smartconfig
    vTaskDelay(100);

//    system_restore();

    system_restart();
}

static int eLexyGetTimeRsp(u8 msg_seq)
{
    int ret;
    msg_get_time_rsp_t msg;
    u32 current_time;        

    memset(&msg, 0, sizeof(msg));
    msg.hdr.op = OP_EVT_ACK;
    msg.hdr.seq = msg_seq;
    msg.hdr.type = TYPE_EVENT_GET_TIME;
    msg.hdr.len = DATA_LEN(msg);
    
    current_time = alink_get_time();
    log_printf("current time [%08x]s,today[%02d:%02d:%02d]\n",
        current_time,(current_time/3600+8)%24,
        (current_time/60)%60,current_time%60);  
    
    INT_TO_BYTE(current_time,
        msg.currentTime[0],msg.currentTime[1],
        msg.currentTime[2],msg.currentTime[3]);
    
    ret = uart_tx_without_rsp((char*)&msg, 
                      sizeof(msg));
    
    if(ret != OP_OK) return Internal_Error;
    
    return 0;
}

static void uart_event_process(u8 *buf, u32 len)
{
    u8 checksum = 0;
    int i;
    
    uart_msg_hdr_t *hdr = (uart_msg_hdr_t *)buf;

    if(hdr->op != OP_EVT)
        return;

    msg_event_t *eventMsg = (msg_event_t *)buf;

    for (i = 2; i < len - 2; i++) {
        checksum += buf[i];
    }
    
    if (buf[i] != checksum) {
        printf("recv checksum %02x,expect checksum:%02x error\n",
            buf[i],checksum);
        return;
    }
    
    switch (eventMsg->hdr.type){
        case TYPE_EVENT_ALARM:
            log_printf("alarm,data:0x%02x!\n", eventMsg->data[0]);            
            gLexyAllParam.alarm = eventMsg->data[0];            
            gLexyAllParam.lastGetTime = system_get_time();
            hnt_device_status_change();                        
            break;     
        case TYPE_EVENT_WIFI_RESET:
            log_printf("wifi reset,data:%x!\n", eventMsg->data[0]);
            eLexyRobotWifiFactoryResetRsp(eventMsg->hdr.seq);
            system_factory_reset();
            break; 
        case TYPE_EVENT_WORKING_STATUS:
            log_printf("working status,data:0x%02x!\n", eventMsg->data[0]);
            gLexyAllParam.workingStatus = eventMsg->data[0];            
            gLexyAllParam.lastGetTime = system_get_time();
            hnt_device_status_change();                        
            break;
#if 0            
        case TYPE_EVENT_UPGRADE_READY:
            log_printf("robot bootloader mode,req for upgrade,flag %d!\n",gSlaveDevUpgradeSuccess);
            if(!gSlaveDevUpgradeSuccess)
                slaveDev_req_upgrade_handle();
            break; 
#endif            
        case TYPE_EVENT_BATTERY_CHANGE:
            log_printf("battery,data:%d!\n", eventMsg->data[0]);            
            gLexyAllParam.battery = eventMsg->data[0];            
            gLexyAllParam.lastGetTime = system_get_time();
            hnt_device_status_change();                        
            break; 
        case TYPE_EVENT_GET_TIME:
            eLexyGetTimeRsp(eventMsg->hdr.seq);
            break;
        default:
            break;            
    }

    return;    
}

static int eLexyGetAllStatus(void)
{    
    int ret;
    msg_get_common_t msg;
    msg_get_robot_status_rsp_t rspMsg, rspMsgCheck;   

    u32 current_time;        
    current_time = alink_get_time();
    log_printf("current time [%08x]s,today[%02d:%02d:%02d]\n",
        current_time,(current_time/3600+8)%24,
        (current_time/60)%60,current_time%60);  
    
    memset(&msg, 0, sizeof(msg));
    memset(&rspMsg, 0, sizeof(rspMsg));
    msg.hdr.op = OP_GET;
    msg.hdr.type = TYPE_GET_WORKING_STATUS;
    msg.hdr.len = DATA_LEN(msg);
    
    ret = uart_tx_with_rsp((char*)&msg, 
                      sizeof(msg),
                      (char*)&rspMsg,
                      sizeof(rspMsg));

    if(ret != OP_OK) return Internal_Error;
    
    memset(&rspMsgCheck, 0, sizeof(rspMsgCheck));
    rspMsgCheck.hdr.syn[0] = SYN0;
    rspMsgCheck.hdr.syn[1] = SYN1;    
    rspMsgCheck.hdr.op = msg.hdr.op + 1;
    rspMsgCheck.hdr.seq = msg.hdr.seq;
    rspMsgCheck.hdr.type = msg.hdr.type;
    rspMsgCheck.hdr.len = DATA_LEN(rspMsgCheck);
    if (memcmp(&rspMsgCheck.hdr, &rspMsg.hdr, sizeof(uart_msg_hdr_t)) != 0) {
        log_printf("msg check error\n");
        return Internal_Error;
    } 

    log_printf("workingStatus      = 0x%02x\n", rspMsg.workingStatus);
    log_printf("battery            = %02d\n", rspMsg.battery);
    log_printf("warning            = 0x%02x\n", rspMsg.alarm);


    gLexyAllParam.workingStatus = rspMsg.workingStatus;            
    gLexyAllParam.battery = rspMsg.battery;            
    gLexyAllParam.alarm = rspMsg.alarm;            
    gLexyAllParam.lastGetTime = system_get_time();
        
    return 0;
}


static int eGetLexyCleaningRobotStatus(char *paramName, char *value)
{    
    int ret;
    
    log_printf("paramName:%s\n", paramName);
    
    if((gLexyAllParam.lastGetTime == 0)
        ||((system_get_time() - gLexyAllParam.lastGetTime) > (1*1000*1000)))
    {       
        ret = eLexyGetAllStatus();
        if(ret != OP_OK) return Internal_Error;
    }    

    if(memcmp(paramName,LEXY_ROBOT_101_WORKING_MODE,sizeof(LEXY_ROBOT_101_WORKING_MODE)) == 0)
    {
        sprintf(value, "0x%02x", gLexyAllParam.workingStatus);
    }
    else if(memcmp(paramName,LEXY_ROBOT_101_BATTERY,sizeof(LEXY_ROBOT_101_BATTERY)) == 0)
    {
        sprintf(value, "%d", gLexyAllParam.battery);      
    }
    else if(memcmp(paramName,LEXY_ROBOT_101_ERR,sizeof(LEXY_ROBOT_101_ERR)) == 0)
    {
        sprintf(value, "0x%02x", gLexyAllParam.alarm);
    }

    return 0;
}


static int eSetLexyCleaningRobotPower(char *paramName, char *value)
{
    int ret;
    msg_set_power_t msg;
    msg_set_common_rsp_t rspMsg, rspMsgCheck;
    uint8_t powerStatus ;

    if (strcmp(value, "on") == 0 || strcmp(value, "1") == 0)
        powerStatus = 0x01;
    else if (strcmp(value, "off") == 0 || strcmp(value, "0") == 0)
        powerStatus = 0x00;
    else
    {
        printf("invalid input value[%s]\n",value);
        return -1;
    }

    memset(&msg, 0, sizeof(msg));
    memset(&rspMsg, 0, sizeof(rspMsg));
    msg.hdr.op = OP_SET;
    msg.hdr.type = TYPE_SET_POWER;
    msg.hdr.len = DATA_LEN(msg);
    msg.power = powerStatus;
    
    ret = uart_tx_with_rsp((char*)&msg, 
                      sizeof(msg),
                      (char*)&rspMsg,
                      sizeof(rspMsg));
    
    if(ret != OP_OK) return -1;
    
    memset(&rspMsgCheck, 0, sizeof(rspMsgCheck));
    rspMsgCheck.hdr.syn[0] = SYN0;        
    rspMsgCheck.hdr.syn[1] = SYN1;
    rspMsgCheck.hdr.op = msg.hdr.op + 1;
    rspMsgCheck.hdr.seq = msg.hdr.seq;
    rspMsgCheck.hdr.type = msg.hdr.type;
    rspMsgCheck.hdr.len = DATA_LEN(rspMsgCheck);
    if (memcmp(&rspMsgCheck.hdr, &rspMsg.hdr, sizeof(uart_msg_hdr_t)) != 0) {
        printf("msg check error\n");
        return -1;
    }
    if(rspMsg.ret != OP_OK)
    {
        printf("return %d,Request_Denied\n",rspMsg.ret);
        return -1;
    }
    
    gLexyAllParam.workingStatus = 0x07;            
    gLexyAllParam.lastGetTime = system_get_time();
    
    return 0;
}


static int eSetLexyCleaningRobotDirection(char *paramName, char *value)
{
    int ret;
    msg_set_direction_t msg;
    msg_set_common_rsp_t rspMsg, rspMsgCheck;   
    uint8_t direction ;

    if (strcmp(value, "forward") == 0 || strcmp(value, "0") == 0)
        direction = 0x30;
    else if (strcmp(value, "left") == 0 || strcmp(value, "1") == 0)
        direction = 0x31;    
    else if (strcmp(value, "right") == 0 || strcmp(value, "2") == 0)
        direction = 0x32;    
    else if (strcmp(value, "stop") == 0 || strcmp(value, "3") == 0)
        direction = 0x33;        
    else
    {
        printf("invalid input value[%s]\n",value);
        return -1;
    }

    memset(&msg, 0, sizeof(msg));
    memset(&rspMsg, 0, sizeof(rspMsg));
    msg.hdr.op = OP_SET;
    msg.hdr.type = TYPE_SET_DIRECTION;
    msg.hdr.len = DATA_LEN(msg);
    msg.direction = direction;

    ret = uart_tx_with_rsp((char*)&msg, 
                      sizeof(msg),
                      (char*)&rspMsg,
                      sizeof(rspMsg));
    
    if(ret != OP_OK) return -1;
    
    memset(&rspMsgCheck, 0, sizeof(rspMsgCheck));
    rspMsgCheck.hdr.syn[0] = SYN0;        
    rspMsgCheck.hdr.syn[1] = SYN1;
    rspMsgCheck.hdr.op = msg.hdr.op + 1;
    rspMsgCheck.hdr.seq = msg.hdr.seq;
    rspMsgCheck.hdr.type = msg.hdr.type;
    rspMsgCheck.hdr.len = DATA_LEN(rspMsgCheck);
    if (memcmp(&rspMsgCheck.hdr, &rspMsg.hdr, sizeof(uart_msg_hdr_t)) != 0) {
        printf("msg check error\n");
        return -1;
    }

    if(rspMsg.ret != OP_OK)
    {
        printf("return %d,Request_Denied\n",rspMsg.ret);
        return -1;
    }
    
    gLexyAllParam.workingStatus = 0x0f;            
    gLexyAllParam.lastGetTime = system_get_time();
    return 0;
}

static int eSetLexyCleaningRobotWorkingMode(char *paramName, char *value)
{
    int ret;
    msg_set_working_mode_t msg;
    msg_set_common_rsp_t rspMsg, rspMsgCheck;   
    uint8_t mode ;

    if (strcmp(value, "0x08") == 0)
        mode = 0x51;
    else if (strcmp(value, "0x09") == 0)
        mode = 0x53;    
    else if (strcmp(value, "0x0a") == 0)
        mode = 0x52;    
    else if (strcmp(value, "0x0b") == 0)
        mode = 0x54;        

    memset(&msg, 0, sizeof(msg));
    memset(&rspMsg, 0, sizeof(rspMsg));
    msg.hdr.op = OP_SET;
    msg.hdr.type = TYPE_SET_WORKING_MODE;
    msg.hdr.len = DATA_LEN(msg);
    msg.workingMode = mode;

    ret = uart_tx_with_rsp((char*)&msg, 
                      sizeof(msg),
                      (char*)&rspMsg,
                      sizeof(rspMsg));
    
    if(ret != OP_OK) return -1;
    
    memset(&rspMsgCheck, 0, sizeof(rspMsgCheck));
    rspMsgCheck.hdr.syn[0] = SYN0;        
    rspMsgCheck.hdr.syn[1] = SYN1;
    rspMsgCheck.hdr.op = msg.hdr.op + 1;
    rspMsgCheck.hdr.seq = msg.hdr.seq;
    rspMsgCheck.hdr.type = msg.hdr.type;
    rspMsgCheck.hdr.len = DATA_LEN(rspMsgCheck);
    if (memcmp(&rspMsgCheck.hdr, &rspMsg.hdr, sizeof(uart_msg_hdr_t)) != 0) {
        printf("msg check error\n");
        return -1;
    }

    if(rspMsg.ret != OP_OK)
    {
        printf("return %d,Request_Denied\n",rspMsg.ret);
        return -1;
    }
    
    gLexyAllParam.workingStatus = mode;            
    gLexyAllParam.lastGetTime = system_get_time();
    
    return 0;
}


deviceParameter_t DeviceParamList[] = {
{LEXY_ROBOT_101_WORKING_MODE, eGetLexyCleaningRobotStatus, eSetLexyCleaningRobotWorkingMode},
{LEXY_ROBOT_101_BATTERY, eGetLexyCleaningRobotStatus, NULL},
{LEXY_ROBOT_101_ERR, eGetLexyCleaningRobotStatus, NULL},

{LEXY_ROBOT_101_POWER, NULL, eSetLexyCleaningRobotPower},
{LEXY_ROBOT_101_DIRECTION, NULL, eSetLexyCleaningRobotDirection},
//{LEXY_CLEANING_ROBOT_DUST_COLLECTOR, NULL, eSetLexyCleaningRobotDustCollector},
//{LEXY_CLEANING_ROBOT_RESERVATION, eGetLexyCleaningRobotReservation, eSetLexyCleaningRobotReservation},
//{LEXY_CLEANING_ROBOT_CLEANTIMES, eGetLexyCleaningRobotCleanTimes, eSetLexyCleaningRobotCleanTimes},
};

customInfo_t customInfo;
char alink_tpsk[64+1]={0};

void ICACHE_FLASH_ATTR
hnt_custom_info_init(void)
{
	strcpy(customInfo.name, DEV_NAME);
	strcpy(customInfo.sn, DEV_SN);
	strcpy(customInfo.model, DEV_MODEL);
	strcpy(customInfo.brand, DEV_BRAND);

	strcpy(customInfo.type, DEV_TYPE);
	strcpy(customInfo.version, DEV_VERSION);
	strcpy(customInfo.category, DEV_CATEGORY);
	strcpy(customInfo.manufacturer, DEV_MANUFACTURE);

	strcpy(customInfo.key, ALINK_KEY);
	strcpy(customInfo.secret, ALINK_SECRET);
    
	strcpy(customInfo.key_sandbox, ALINK_KEY_SANDBOX);
	strcpy(customInfo.secret_sandbox, ALINK_SECRET_SANDBOX);

    hnt_custom_info_regist(&customInfo);
    
	strcpy(alink_tpsk, ALINK_TPSK);
    hnt_vendor_tpsk_regist(alink_tpsk);        
}

void
user_custom_init(void)
{       
	os_printf("%s,%d\n", __FUNCTION__,__LINE__);
    
    hnt_custom_info_init();
    
    hnt_uart_event_func_regist(uart_event_process);

    hnt_param_array_regist(&DeviceParamList[0], sizeof(DeviceParamList)/sizeof(DeviceParamList[0]));

/*wifi led install */
    lexyrobot101_gpio_status_init();

    hnt_wifi_led_func_regist(lexyrobot101_wifi_status_led);
}
#endif

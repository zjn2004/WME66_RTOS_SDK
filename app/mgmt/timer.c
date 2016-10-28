//#include "ets_sys.h"
//#include "os_type.h"
//#include "mem.h"
//#include "osapi.h"
#if 0
#include "esp_common.h"
#include "user_interface.h"
#include "hnt_interface.h"
#include "mgmt/timer.h"
#include "espconn.h"

struct hnt_internal_wait_timer_param {
    uint16 on_off;
    uint16 timer_index;
    int wait_time_second;
};

typedef struct hnt_platform_wait_timer_param {
    int timestamp;
    int type;
    int saveTimestamp;
    hnt_timer_group_param_t group[MAX_TIMER_COUNT];
}hnt_platform_wait_timer_param_t;

struct wait_param {
    uint8 on_off[2*MAX_TIMER_COUNT];
    uint8 timer_index[2*MAX_TIMER_COUNT];
    uint16 action_number;
    uint32 min_time_backup;
};

LOCAL os_timer_t device_timer;
uint32 min_wait_second;
hnt_platform_wait_timer_param_t *wait_timer_param = NULL;
struct wait_param pwait_action;
hnt_platform_timer_action_func timer_action_func = NULL;

void user_hnt_platform_timer_action(struct hnt_internal_wait_timer_param *wt_param, uint16 count);
void user_hnt_platform_timer_write_to_flash(void);

/******************************************************************************
 * FunctionName : split
 * Description  : split string p1 according to sting p2 and save the splits
 * Parameters   : p1 , p2 ,splits[]
 * Returns      : the number of splits
*******************************************************************************/
uint16 ICACHE_FLASH_ATTR
split(char *p1, char *p2, char *splits[])
{
    int i = 0;
    int j = 0;

    while (i != -1) {
        int start = i;
        int end = indexof(p1, p2, start);

        if (end == -1) {
            end = os_strlen(p1);
        }

        char *p = (char *) zalloc(100);
        memcpy(p, p1 + start, end - start);
        p[end - start] = '\0';
        splits[j] = p;
        j++;
        i = end + 1;

        if (i > os_strlen(p1)) {
            break;
        }
    }

    return j;
}

/******************************************************************************
 * FunctionName : indexof
 * Description  : calculate the offset of p2 relate to start of p1
 * Parameters   : p1,p1,start
 * Returns      : the offset of p2 relate to the start
*******************************************************************************/
int ICACHE_FLASH_ATTR
indexof(char *p1, char *p2, int start)
{
    char *find = (char *)os_strstr(p1 + start, p2);

    if (find != NULL) {
        return (find - p1);
    }

    return -1;
}

/******************************************************************************
 * FunctionName : hnt_platform_find_min_time
 * Description  : find the minimum wait second in timer list
 * Parameters   : timer_wait_param -- param of timer action and wait time param
 *                count -- The number of timers given by server
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
hnt_platform_find_min_time(struct hnt_internal_wait_timer_param *wt_param , uint16 count)
{
    uint16 i = 0;
    min_wait_second = 0xFFFFFFF;

    for (i = 0; i < count ; i++) {
        log_debug("test\n");
        if (wt_param[i].wait_time_second < min_wait_second && wt_param[i].wait_time_second >= 0) {
            min_wait_second = wt_param[i].wait_time_second;
        }
    }
}


/******************************************************************************
 * FunctionName : hnt_platform_fix_time_second_get
 * Description  : get fix time real time 
 * Parameters   : time_second_day -- second by day
 *                
 * Returns      : time_second_real -- real time
*******************************************************************************/

int ICACHE_FLASH_ATTR
hnt_platform_fix_time_second_get(int time_second_day)
{
    int start_time_second_day = (wait_timer_param->saveTimestamp - 1388937600) % ONE_DAY_SECONDS;
    int time_second_real = 0;

    if(time_second_day > start_time_second_day)
         time_second_real = wait_timer_param->saveTimestamp - start_time_second_day + time_second_day;
    else
         time_second_real = wait_timer_param->saveTimestamp - start_time_second_day + time_second_day + ONE_DAY_SECONDS;

    return time_second_real;
}

/******************************************************************************
 * FunctionName : hnt_platform_check_fix_time
 * Description  : check fix time 
 * Parameters   : on_time_second -- on time second
 *                      off_time_second-- off time second 
 * Returns      : 0: on/off fix time is not  
*******************************************************************************/

int ICACHE_FLASH_ATTR
hnt_platform_check_fix_time(int on_time_second, int off_time_second)
{

    int current_time_second = 0;
    int on_time_second_real = -1;
    int off_time_second_real = -1;
    
    if(on_time_second != -1)
    {
       on_time_second_real = hnt_platform_fix_time_second_get(on_time_second);
    }

    if(off_time_second != -1)
    {
        off_time_second_real = hnt_platform_fix_time_second_get(off_time_second);
    }

    current_time_second = wait_timer_param->timestamp;

    log_debug("Current_Time:%d On_Time:%d Off_Time:%d\n",current_time_second,on_time_second_real,off_time_second_real);
    if(on_time_second_real == -1 && current_time_second < off_time_second_real  )
        return 0; 
    if(current_time_second < on_time_second_real && off_time_second_real == -1)
        return 0;
    if(current_time_second < on_time_second_real || current_time_second < off_time_second_real)
        return 0;
    return -1;
    
}

/******************************************************************************
 * FunctionName : user_hnt_platform_timer_first_start
 * Description  : calculate the wait time of each timer
 * Parameters   : count -- The number of timers given by server
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_hnt_platform_timer_first_start(void)
{
    int i = 0;
    uint16 count = 0;
    struct hnt_internal_wait_timer_param wt_param[16*MAX_TIMER_COUNT] = {0};

    log_debug("current timestamp= %ds\n", wait_timer_param->timestamp);
    log_debug("min_wait_second= %ds\n", min_wait_second);
    log_debug("timestamp+min_wait_second= %ds\n", wait_timer_param->timestamp+min_wait_second);

    if(get_xmpp_conn_status() == 1)
    {
        wait_timer_param->timestamp = get_current_time();
        log_debug("wait_timer_param->timestamp= %ds\n", wait_timer_param->timestamp);
    }
    else
    {
        wait_timer_param->timestamp += min_wait_second;
    }
    
    if(wait_timer_param->type == TIMER_TYPE_FIXDTIME)
    {
#if HNT_TIMER_FIX_SUPPORT
        for(i = 0; i < MAX_TIMER_COUNT; i++)
        {
            if(wait_timer_param->group[i].enable != TIMER_ONE_ENABLE)
                continue;
                
            if((wait_timer_param->group[i].on_fix_wait_time_second != 0) && 
               (wait_timer_param->group[i].on_fix_wait_time_second > wait_timer_param->timestamp))
            {
                log_debug("test\n");
                wt_param[count].wait_time_second = wait_timer_param->group[i].on_fix_wait_time_second - 
                                                   wait_timer_param->timestamp;  
                wt_param[count].on_off = TIMER_MODE_ON;  
                count++;
            }
            if((wait_timer_param->group[i].off_fix_wait_time_second != 0) && 
               (wait_timer_param->group[i].off_fix_wait_time_second > wait_timer_param->timestamp))
            {
                log_debug("test\n");
                wt_param[count].wait_time_second = wait_timer_param->group[i].off_fix_wait_time_second - 
                                                    wait_timer_param->timestamp;              
                wt_param[count].on_off = TIMER_MODE_OFF;  
                count++;
            }
        }
#endif
    }
    else if(wait_timer_param->type == TIMER_TYPE_LOOP_PERIOD)
    {
#if HNT_TIMER_LOOP_SUPPORT
        for(i = 0; i < MAX_TIMER_COUNT; i++)
        {
            if(wait_timer_param->group[i].enable != TIMER_ONE_ENABLE)
                continue;
                
            if(wait_timer_param->group[i].on_loop_wait_time_second != 0)
            {
                log_debug("test\n");
                wt_param[count].wait_time_second = wait_timer_param->group[i].on_loop_wait_time_second - 
                                (wait_timer_param->timestamp % wait_timer_param->group[i].on_loop_wait_time_second);              
                wt_param[count].on_off = TIMER_MODE_ON;  
                count++;
            }
            if(wait_timer_param->group[i].off_loop_wait_time_second != 0)
            {
                log_debug("test\n");
                wt_param[count].wait_time_second = wait_timer_param->group[i].off_loop_wait_time_second - 
                                (wait_timer_param->timestamp % wait_timer_param->group[i].off_loop_wait_time_second);              
                wt_param[count].on_off = TIMER_MODE_OFF;  
                count++;
            }
        }
#endif
    }
    else if(wait_timer_param->type == TIMER_TYPE_LOOP_WEEK)
    {
#if HNT_TIMER_WEEK_SUPPORT
        if(wait_timer_param->group[DELAY_GROUP_INDEX].enable == TIMER_ONE_ENABLE)
        {
            log_debug("test\n");
            int delay_timer_flag = 2;
            if(wait_timer_param->group[DELAY_GROUP_INDEX].on_week_wait_time_second != -1)
            {
                if (wait_timer_param->group[DELAY_GROUP_INDEX].on_week_wait_time_second + 
                    wait_timer_param->group[DELAY_GROUP_INDEX].weekday >  wait_timer_param->timestamp) {
                    wt_param[count].wait_time_second = wait_timer_param->group[DELAY_GROUP_INDEX].on_week_wait_time_second + 
                                                       wait_timer_param->group[DELAY_GROUP_INDEX].weekday - 
                                                       wait_timer_param->timestamp;

                    wt_param[count].on_off = TIMER_MODE_ON;  
                    wt_param[count].timer_index = DELAY_GROUP_INDEX;  
                    count++;
                    delay_timer_flag--;
                }
            }
            
            if(wait_timer_param->group[DELAY_GROUP_INDEX].off_week_wait_time_second != -1)
            {
                if (wait_timer_param->group[DELAY_GROUP_INDEX].off_week_wait_time_second + 
                    wait_timer_param->group[DELAY_GROUP_INDEX].weekday >  wait_timer_param->timestamp) {
                    wt_param[count].wait_time_second = wait_timer_param->group[DELAY_GROUP_INDEX].off_week_wait_time_second + 
                                                       wait_timer_param->group[DELAY_GROUP_INDEX].weekday - 
                                                       wait_timer_param->timestamp;

                    wt_param[count].on_off = TIMER_MODE_OFF;  
                    wt_param[count].timer_index = DELAY_GROUP_INDEX;  
                    count++;
                    delay_timer_flag--;
                }
            }
            
            if(delay_timer_flag == 2)
            {
                wait_timer_param->group[DELAY_GROUP_INDEX].enable = TIMER_ONE_DISABLE;
                user_hnt_platform_timer_write_to_flash();      
                if(timer_action_func != NULL)
                    timer_action_func(TIMER_MODE_ACTION_TIMER_CONFIG_CHANGED, -1, -1);
            }
        }
        
        
        int monday_wait_time = (wait_timer_param->timestamp - 1388937600) % ONE_WEEK_SECONDS;   
        int day_wait_time =  (wait_timer_param->timestamp - 1388937600) % ONE_DAY_SECONDS;
        int week;
        int week_wait_time = 0;
        log_debug("day_wait_time = %d", day_wait_time);
        log_debug("day_wait_time2 = %d", (wait_timer_param->timestamp - 1388937600)%ONE_DAY_SECONDS);
        for(i = 1; i < MAX_TIMER_COUNT; i++)
        {        
            if(wait_timer_param->group[i].enable != TIMER_ONE_ENABLE)
                continue;

            for(week = 0; week < 8; week++)
            {
                if(CHECK_FLAG_BIT(wait_timer_param->group[i].weekday, week))
                {
                    week_wait_time = week*ONE_DAY_SECONDS;
                    if(wait_timer_param->group[i].on_week_wait_time_second != -1)
                    {
                        log_debug("test\n");
                        if(week == 7)/*for fix time deal*/
                        {
                            if(hnt_platform_check_fix_time(wait_timer_param->group[i].on_week_wait_time_second,
                                wait_timer_param->group[i].off_week_wait_time_second) == -1)
                            {
                                wait_timer_param->group[i].enable = TIMER_ONE_DISABLE;
                                user_hnt_platform_timer_write_to_flash();
                                if(timer_action_func != NULL)
                                    timer_action_func(TIMER_MODE_ACTION_TIMER_CONFIG_CHANGED, -1, -1);
                                continue;
                            }
                            else
                            {
                                log_debug("day_wait_time: %d on_time:%d\n",day_wait_time,wait_timer_param->group[i].on_week_wait_time_second);
                                if (wait_timer_param->group[i].on_week_wait_time_second > day_wait_time)
                                    wt_param[count].wait_time_second = wait_timer_param->group[i].on_week_wait_time_second
                                                                        -day_wait_time;
                                else
                                    wt_param[count].wait_time_second = ONE_DAY_SECONDS - day_wait_time + 
                                                                        wait_timer_param->group[i].on_week_wait_time_second;
                            }
                        }
                        else
                        {
                            if (wait_timer_param->group[i].on_week_wait_time_second + week_wait_time > monday_wait_time) {
                                wt_param[count].wait_time_second = wait_timer_param->group[i].on_week_wait_time_second - 
                                                                   monday_wait_time + week_wait_time ;
                            } else {
                                wt_param[count].wait_time_second = ONE_WEEK_SECONDS - monday_wait_time + 
                                                                   wait_timer_param->group[i].on_week_wait_time_second +
                                                                   week_wait_time;
                            }
                        }
                        if(i == NIGHTMODE_GROUP_INDEX)
                        {
                            wt_param[count].on_off = TIMER_MODE_ACTION_NIGHTMODE_ON;  
                        }
                        else
                        {
                            wt_param[count].on_off = TIMER_MODE_ON;  
                        }
                        wt_param[count].timer_index = i;  
                        count++;
                    }
                    if(wait_timer_param->group[i].off_week_wait_time_second != -1)
                    {
                        log_debug("test\n");
                        if(week == 7)/*for fix time deal*/
                        {
                            if(hnt_platform_check_fix_time(wait_timer_param->group[i].on_week_wait_time_second,
                                wait_timer_param->group[i].off_week_wait_time_second) == -1)
                            {
                                wait_timer_param->group[i].enable = TIMER_ONE_DISABLE;
                                user_hnt_platform_timer_write_to_flash();
                                if(timer_action_func != NULL)
                                    timer_action_func(TIMER_MODE_ACTION_TIMER_CONFIG_CHANGED, -1, -1);
                                continue;
                            }
                            else
                            {
                                log_debug("day_wait_time: %d off_time:%d\n",day_wait_time,wait_timer_param->group[i].off_week_wait_time_second);
                                if (wait_timer_param->group[i].off_week_wait_time_second > day_wait_time)
                                    wt_param[count].wait_time_second = wait_timer_param->group[i].off_week_wait_time_second
                                                                        -day_wait_time;
                                else
                                    wt_param[count].wait_time_second = ONE_DAY_SECONDS - day_wait_time+
                                                                        wait_timer_param->group[i].off_week_wait_time_second;
                            }               
                        }
                        else
                        {
                            if (wait_timer_param->group[i].off_week_wait_time_second + week_wait_time > monday_wait_time) {
                                wt_param[count].wait_time_second = wait_timer_param->group[i].off_week_wait_time_second - 
                                                                   monday_wait_time + week_wait_time;
                            } else {
                                wt_param[count].wait_time_second = ONE_WEEK_SECONDS - monday_wait_time + 
                                                                   wait_timer_param->group[i].off_week_wait_time_second +
                                                                   week_wait_time;
                            }
                        }
                        if(i == NIGHTMODE_GROUP_INDEX)
                        {
                            wt_param[count].on_off = TIMER_MODE_ACTION_NIGHTMODE_OFF;  
                        }
                        else
                        {
                            wt_param[count].on_off = TIMER_MODE_OFF;  
                        }
                        wt_param[count].timer_index = i;  
                        count++;
                    }
                 }
            }
        }
#endif
    }
    
    for(i = 0; i < count; i++)
    {
        log_debug("wt_param[%d].wait_time_second = %d\n", i, wt_param[i].wait_time_second);
        log_debug("wt_param[%d].on_off = %d\n", i, wt_param[i].on_off);
        log_debug("wt_param[%d].timer_index = %d\n", i, wt_param[i].timer_index);
    }
    
    hnt_platform_find_min_time(wt_param, count);
    if((min_wait_second == 0) || (count == 0)) {
        os_timer_disarm(&device_timer);
        log_debug("test\n");
    	return;
    }

    user_hnt_platform_timer_action(wt_param, count);
}

/******************************************************************************
 * FunctionName : user_hnt_platform_device_action
 * Description  : Execute the actions of minimum wait time
 * Parameters   : pwait_action -- point the list of actions which need execute
 *
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_hnt_platform_device_action(struct wait_param *pwait_action)
{
    uint8 i = 0;
    uint8 timer_index = 0;
    uint16 action_number = pwait_action->action_number;

    log_debug("there is %d action at the same time\n", pwait_action->action_number);

    for (i = 0; i < action_number; i++) {
        timer_index = pwait_action->timer_index[i];
        log_debug("%d:%d\n", timer_index, pwait_action->on_off[i]);

        if(pwait_action->on_off[i] == TIMER_MODE_ON)
        {
            if(timer_action_func != NULL)
                timer_action_func(pwait_action->on_off[i], timer_index,
                wait_timer_param->group[timer_index].on_week_wait_time_second);
        }
        else if(pwait_action->on_off[i] == TIMER_MODE_OFF)
        {
            if(timer_action_func != NULL)
                timer_action_func(pwait_action->on_off[i], timer_index,
                wait_timer_param->group[timer_index].off_week_wait_time_second);
        }
        else if(pwait_action->on_off[i] == TIMER_MODE_ACTION_NIGHTMODE_ON)
        {
            if(timer_action_func != NULL)
                timer_action_func(pwait_action->on_off[i], timer_index,
                wait_timer_param->group[timer_index].on_week_wait_time_second);
        }
        else if(pwait_action->on_off[i] == TIMER_MODE_ACTION_NIGHTMODE_OFF) 
        {
            if(timer_action_func != NULL)
                timer_action_func(pwait_action->on_off[i], timer_index,
                wait_timer_param->group[timer_index].off_week_wait_time_second);
        } 
        else 
        {
            log_debug("test\n");
            return;
        }
    }
    user_hnt_platform_timer_first_start();
}

/******************************************************************************
 * FunctionName : hnt_platform_timer_start
 * Description  : Processing the message about timer from the server
 * Parameters   : timer_wait_param -- The received data from the server
 *                count -- the espconn used to connetion with the host
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_hnt_platform_wait_time_overflow_check(struct wait_param *pwait_action)
{
    log_debug("min_wait_second = %d\n", min_wait_second);

    if (pwait_action->min_time_backup >= 3600) {
        os_timer_disarm(&device_timer);
        os_timer_setfn(&device_timer, (os_timer_func_t *)user_hnt_platform_wait_time_overflow_check, pwait_action);
        os_timer_arm(&device_timer, 3600000, 0);
        log_debug("min_wait_second is extended\n");
    } else {
        os_timer_disarm(&device_timer);
        os_timer_setfn(&device_timer, (os_timer_func_t *)user_hnt_platform_device_action, pwait_action);
        os_timer_arm(&device_timer, pwait_action->min_time_backup * 1000, 0);
        log_debug("min_wait_second is = %dms\n", pwait_action->min_time_backup * 1000);
    }

    pwait_action->min_time_backup -= 3600;
}

void ICACHE_FLASH_ATTR
user_hnt_platform_timer_write_to_flash(void)
{
    struct hnt_mgmt_saved_param param;
    memset(&param, 0, sizeof(param));

    hnt_mgmt_load_param(&param);

	uint16 i = 0;

    memcpy(param.group, wait_timer_param->group, sizeof(wait_timer_param->group));
    param.saveTimestamp = wait_timer_param->saveTimestamp;

	for(i=0;i<MAX_TIMER_COUNT;i++)
	{
		log_debug("group%d :On:%d Off:%d week:%x enable:%d\n",i,wait_timer_param->group[i].on_week_wait_time_second,wait_timer_param->group[i].off_week_wait_time_second,
										wait_timer_param->group[i].weekday,wait_timer_param->group[i].enable);
	}
	hnt_mgmt_save_param(&param);
}

/******************************************************************************
 * FunctionName : user_hnt_platform_timer_action
 * Description  : Processing the message about timer from the server
 * Parameters   : timer_wait_param -- The received data from the server
 *                count -- the espconn used to connetion with the host
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_hnt_platform_timer_action(struct hnt_internal_wait_timer_param *wt_param, uint16 count)
{
    uint16 i = 0;
    uint16 action_number;

    memset(&pwait_action, 0, sizeof(pwait_action)); 
    action_number = 0;

    for (i = 0; i < count ; i++) {
        if (wt_param[i].wait_time_second == min_wait_second) {
            pwait_action.on_off[action_number] = wt_param[i].on_off;
            pwait_action.timer_index[action_number] = wt_param[i].timer_index;
            log_debug("*****%d:%d*****\n", wt_param[i].timer_index, wt_param[i].on_off);
            action_number++;
        }
    }

    pwait_action.action_number = action_number;
    pwait_action.min_time_backup = min_wait_second;
    user_hnt_platform_wait_time_overflow_check(&pwait_action);
}

void ICACHE_FLASH_ATTR
user_platform_timer_stop(void)
{
    if(wait_timer_param != NULL)
    {
        free(wait_timer_param);
        wait_timer_param = NULL;
    }
}
/******************************************************************************
 * FunctionName : hnt_platform_timer_start
 * Description  : Processing the message about timer from the server
 * Parameters   : pbuffer -- The received data from the server

 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
hnt_platform_timer_start(char *pbuffer)
{
    int str_begin = 0;
    int str_end = 0;
    char *pstr_start = NULL;
    char *pstr_end = NULL;
    char *pstr = NULL;
    char temp_str[256] = {0};
    uint16 count = 0;
    char *timer_splits[20] = {NULL};
    int i;
    int fix_index = 0;
    int loop_index = 0;
    int week_index = 0;
    int timestamp = 0;

    log_debug("test\n");
    if(timer_action_func == NULL) 
        return;

    if(wait_timer_param == NULL)
        wait_timer_param = (hnt_platform_wait_timer_param_t *)zalloc(sizeof(hnt_platform_wait_timer_param_t));

    memset(&(wait_timer_param->group[0]), 0, (MAX_TIMER_COUNT-1)*sizeof(hnt_timer_group_param_t));        
    min_wait_second  = 0;
    log_debug("test\n");

    json_get_value(pbuffer, "timestamp", temp_str, sizeof(temp_str) - 1);
    log_debug("test:temp_str = %s\n", temp_str);
    timestamp = atoi(temp_str);
    wait_timer_param->timestamp = get_current_time();

    memset(temp_str, 0, sizeof(temp_str));
    json_get_value(pbuffer, "type", temp_str, sizeof(temp_str) - 1);
    log_debug("test:temp_str = %s\n", temp_str);
    if(temp_str[0] == 'f')
        wait_timer_param->type = TIMER_TYPE_FIXDTIME;
    else if(temp_str[0] == 'l')
        wait_timer_param->type = TIMER_TYPE_LOOP_PERIOD;
    else if(temp_str[0] == 'w')
        wait_timer_param->type = TIMER_TYPE_LOOP_WEEK;
    else
    {
        user_platform_timer_stop();
        return;
    }
    log_debug("test\n");

    memset(temp_str, 0, sizeof(temp_str));
    json_get_value(pbuffer, "enable", temp_str, sizeof(temp_str) - 1);
    log_debug("test:temp_str = %s\n", temp_str);
    count = split(temp_str , ";" , timer_splits);
    log_debug("test:count = %d\n", count);
    fix_index = 0;
    loop_index = 0;
    week_index = 0;

    for(i = 0; (i < count) && (i < MAX_TIMER_COUNT-1); i++)
    {
        pstr = timer_splits[i];
        if(*pstr == '1')
            wait_timer_param->group[fix_index++].enable = TIMER_ONE_ENABLE;
        else if(*pstr == '0')
            wait_timer_param->group[fix_index++].enable = TIMER_ONE_DISABLE; 
        else
        {
            free(timer_splits[i]);
            return; 
        }
        
        free(timer_splits[i]);
    }
    
    memset(temp_str, 0, sizeof(temp_str));
    json_get_value(pbuffer, "on", temp_str, sizeof(temp_str) - 1);
    log_debug("test:temp_str = %s\n", temp_str);
    count = split(temp_str , ";" , timer_splits);
    log_debug("test:count = %d\n", count);
    for(i = 0; (i < count) && (i < MAX_TIMER_COUNT-1); i++)
    {
        pstr = timer_splits[i];
        log_debug("test:timer_splits[%d] = %s\n", i, timer_splits[i]);
        if (*pstr == 'f')
        {
#if HNT_TIMER_FIX_SUPPORT
            wait_timer_param->group[++fix_index].on_fix_wait_time_second = atoi(pstr + 1);
#endif
        }
        else if(*pstr == 'l')
        {
#if HNT_TIMER_LOOP_SUPPORT
            wait_timer_param->group[++loop_index].on_loop_wait_time_second = atoi(pstr + 1);
#endif
        }
        else if(*pstr == 'w')
        {
#if HNT_TIMER_WEEK_SUPPORT
            wait_timer_param->group[++week_index].on_week_wait_time_second = atoi(pstr + 1);
#endif
        }
        else if(*pstr == 'd')
        {
            wait_timer_param->group[0].on_week_wait_time_second = atoi(pstr + 1);            
        }
        
        free(timer_splits[i]);
    }
    log_debug("test\n");

    memset(temp_str, 0, sizeof(temp_str));
    json_get_value(pbuffer, "off", temp_str, sizeof(temp_str) - 1);
    log_debug("test:temp_str = %s\n", temp_str);
    count = split(temp_str , ";" , timer_splits);
    log_debug("test:count = %d\n", count);
    fix_index = 0;
    loop_index = 0;
    week_index = 0;
    for(i = 0; (i < count) && (i < MAX_TIMER_COUNT-1); i++)
    {
        pstr = timer_splits[i];
        log_debug("test:timer_splits[%d] = %s\n", i, timer_splits[i]);
        if (*pstr == 'f')
        {
#if HNT_TIMER_FIX_SUPPORT
            wait_timer_param->group[++fix_index].off_fix_wait_time_second = atoi(pstr + 1);
#endif
        }
        else if(*pstr == 'l')
        {
#if HNT_TIMER_LOOP_SUPPORT
            wait_timer_param->group[++loop_index].off_loop_wait_time_second = atoi(pstr + 1);
#endif
        }
        else if(*pstr == 'w')
        {
#if HNT_TIMER_WEEK_SUPPORT
            wait_timer_param->group[++week_index].off_week_wait_time_second = atoi(pstr + 1);
#endif
        }
        else if(*pstr == 'd')
        {
            wait_timer_param->group[0].off_week_wait_time_second = atoi(pstr + 1);            
        }
        
        free(timer_splits[i]);
    }
    log_debug("test\n");
#if HNT_TIMER_WEEK_SUPPORT
    memset(temp_str, 0, sizeof(temp_str));
    json_get_value(pbuffer, "weekday", temp_str, sizeof(temp_str) - 1);
    log_debug("test:temp_str = %s\n", temp_str);
    count = split(temp_str , ";" , timer_splits);
    log_debug("test:count = %d\n", count);
    week_index = 0;
    for(i = 0; (i < count) && (i < MAX_TIMER_COUNT-1); i++)
    {
        wait_timer_param->group[i].weekday = 0;
        pstr = timer_splits[i];
        log_debug("test:timer_splits[%d] = %s\n", i, timer_splits[i]);
        while(*pstr != '\0')
        {
            week_index = *pstr - '1';
            log_debug("test week = %d\n", week_index);
            SET_FLAG_BIT(wait_timer_param->group[i].weekday, week_index);
            pstr++;
        }
        free(timer_splits[i]);
    }

    wait_timer_param->group[DELAY_GROUP_INDEX].weekday = timestamp;//wait_timer_param->timestamp;
    wait_timer_param->saveTimestamp = get_current_time();
#endif    
    user_hnt_platform_timer_write_to_flash();

    user_hnt_platform_timer_first_start();
}

void ICACHE_FLASH_ATTR
hnt_platform_timer_get(char *buffer)
{
    int len = 0;
    int i = 0;
    int timer_count = 0;
    int week_index = 0;

    if(wait_timer_param == NULL)
        return;
    
    struct hnt_mgmt_saved_param param;
    memset(&param, 0, sizeof(param));
    hnt_mgmt_load_param(&param);
    
    len = snprintf(buffer, sizeof(buffer), "{\"type\":\"w\",\"timestamp\":\"%d\",\"enable\":\"",
                                    wait_timer_param->group[DELAY_GROUP_INDEX].weekday);

    for(i = 0; i < MAX_TIMER_COUNT-1; i++)
    {    
        if((wait_timer_param->group[i].enable != TIMER_ONE_DISABLE) && 
            (wait_timer_param->group[i].enable != TIMER_ONE_ENABLE)
            )
        {
            log_debug("test\n");
            break;  
        }
    }

    timer_count = i;
    if(timer_count == 0)
    {
        log_debug("test\n");
        len += snprintf(buffer+len, sizeof(buffer) - len, "\",\"weekday\":\"\",\"on\":\"\",\"off\":\"\"}");        
    }
    else
    {
        log_debug("test\n");
        for(i = 0; i < timer_count - 1; i++)
        {
            len += snprintf(buffer+len, sizeof(buffer)-len, "%d;", wait_timer_param->group[i].enable - 1);
        }
        len += snprintf(buffer+len, sizeof(buffer)-len, "%d\",\"weekday\":\"", wait_timer_param->group[i].enable - 1);

        for(i = 0; i < timer_count - 1; i++)
        {
            if(i == 0)
                len += snprintf(buffer+len, sizeof(buffer)-len, "-1");
            else
            {
                for((week_index = 0); 
                    week_index < 8; 
                    week_index++)
                {
                    if(CHECK_FLAG_BIT(wait_timer_param->group[i].weekday, week_index))
                        len += snprintf(buffer+len, sizeof(buffer)-len, "%d", week_index+1);
                }
            }
            len += snprintf(buffer+len, sizeof(buffer)-len, ";");
        }

        if(i == 0)
            len += snprintf(buffer+len, sizeof(buffer)-len, "-1");
        else
        {
            for(week_index = 0; 
             week_index < 8; 
             week_index++)
            {
                if(CHECK_FLAG_BIT(wait_timer_param->group[i].weekday, week_index))
                    len += snprintf(buffer+len, sizeof(buffer)-len, "%d", week_index+1);
            }
        }
        len += snprintf(buffer+len, sizeof(buffer)-len, "\",\"on\":\"");

        
        for(i = 0; i < timer_count - 1; i++)
        {
            if(i == 0)
                len += snprintf(buffer+len, sizeof(buffer)-len, "d%d;", 
                                wait_timer_param->group[i].on_week_wait_time_second);
            else            
                len += snprintf(buffer+len, sizeof(buffer)-len, "w%d;", 
                            wait_timer_param->group[i].on_week_wait_time_second);
        }
        
        if(i == 0)
            len += snprintf(buffer+len, sizeof(buffer)-len, "d%d\",\"off\":\"", 
                            wait_timer_param->group[i].on_week_wait_time_second);
        else
            len += snprintf(buffer+len, sizeof(buffer)-len, "w%d\",\"off\":\"", 
                        wait_timer_param->group[i].on_week_wait_time_second);

        for(i = 0; i < timer_count - 1; i++)
        {
            if(i == 0)
                len += snprintf(buffer+len, sizeof(buffer)-len, "d%d;", 
                            wait_timer_param->group[i].off_week_wait_time_second);
            else            
                len += snprintf(buffer+len, sizeof(buffer)-len, "w%d;", 
                            wait_timer_param->group[i].off_week_wait_time_second);
        }
        if(i == 0)
            len += snprintf(buffer+len, sizeof(buffer)-len, "d%d\"}", 
                            wait_timer_param->group[i].off_week_wait_time_second);
        else
            len += snprintf(buffer+len, sizeof(buffer)-len, "w%d\"}", 
                        wait_timer_param->group[i].off_week_wait_time_second);
    }

    log_debug("%s", buffer);
}

int ICACHE_FLASH_ATTR
hnt_platform_night_mode_start(uint8_t mode, uint32 normalTime, uint32 closeTime)
{
    if(mode == NIGHT_MODE_NORMAL)/*normal mode*/
    {
        if(timer_action_func != NULL)
            timer_action_func(TIMER_MODE_ACTION_NIGHTMODE_ON, -1, -1);
    }
    else if(mode == NIGHT_MODE_CLOSE)/*close mode*/
    {        
        if(timer_action_func != NULL)
            timer_action_func(TIMER_MODE_ACTION_NIGHTMODE_OFF, -1, -1);
    }
    else if(mode == NIGHT_TIMER_MODE)
    {
        log_debug("night mode = %d\n", mode);
        int day_wait_time =  (get_current_time()- 1388937600) % ONE_DAY_SECONDS;
        log_debug("day_wait_time = %d\n", day_wait_time);
        if((day_wait_time > normalTime) && (day_wait_time < closeTime))
        {
            log_debug("test\n");
            if(timer_action_func != NULL)
                timer_action_func(TIMER_MODE_ACTION_NIGHTMODE_ON, -1, -1);
        }
        else if((day_wait_time > closeTime) && (day_wait_time > normalTime))
        {
            log_debug("test\n");
            if(timer_action_func != NULL)
                timer_action_func(TIMER_MODE_ACTION_NIGHTMODE_OFF, -1, -1);
        }
        else if((day_wait_time < closeTime) && (day_wait_time < normalTime))
        {
            log_debug("test\n");
            if(timer_action_func != NULL)
                timer_action_func(TIMER_MODE_ACTION_NIGHTMODE_OFF, -1, -1);
        }
    }
    
    return 0;

}

void ICACHE_FLASH_ATTR
hnt_platform_nightmode_timer_start(uint8_t mode, uint32 normalTime, uint32 closeTime)
{
    log_debug("test\n");

    if(wait_timer_param == NULL)
        wait_timer_param = (hnt_platform_wait_timer_param_t *)zalloc(sizeof(hnt_platform_wait_timer_param_t));

    memset(&(wait_timer_param->group[NIGHTMODE_GROUP_INDEX]), 0, sizeof(hnt_timer_group_param_t));        
    min_wait_second  = 0;
    log_debug("test\n");

    wait_timer_param->timestamp = get_current_time();
    wait_timer_param->type = TIMER_TYPE_LOOP_WEEK;
    wait_timer_param->group[NIGHTMODE_GROUP_INDEX].enable = mode;    
    wait_timer_param->group[NIGHTMODE_GROUP_INDEX].weekday = 0x7F;
    wait_timer_param->group[NIGHTMODE_GROUP_INDEX].on_week_wait_time_second = (normalTime * 60);
    wait_timer_param->group[NIGHTMODE_GROUP_INDEX].off_week_wait_time_second = (closeTime * 60);

    user_hnt_platform_timer_write_to_flash();
    user_hnt_platform_timer_first_start();
}

int ICACHE_FLASH_ATTR
hnt_platform_night_mode_set(char *buff)
{
    uint8      nightmode_status;
    uint32     normal_time = 21600; /*6:00AM*/
    uint32     close_time  = 79200; /*10:00PM*/

    if(timer_action_func == NULL) 
        return;

    char temp_str[64] = {0};
    json_get_value(buff, "mode", temp_str, sizeof(temp_str) - 1);
    nightmode_status = atoi(temp_str);

    if(nightmode_status == NIGHT_TIMER_MODE)
    {
        memset(temp_str, 0, sizeof(temp_str));
        json_get_value(buff, "normalTime", temp_str, sizeof(temp_str) - 1);
        normal_time = atoi(temp_str);
        
        memset(temp_str, 0, sizeof(temp_str));
        json_get_value(buff, "closeTime", temp_str, sizeof(temp_str) - 1);
        close_time = atoi(temp_str);
    }
    else if(nightmode_status == NIGHT_MODE_CLOSE || nightmode_status == NIGHT_MODE_NORMAL)
    {
        normal_time = -1;
        close_time = -1;
    }
    else
    {
        return -1;
    }

    log_debug("status:%d normal_time:%d closeTime:%d\n",nightmode_status,normal_time,close_time);

    hnt_platform_night_mode_start(nightmode_status, normal_time*60, close_time*60);     
    hnt_platform_nightmode_timer_start(nightmode_status, normal_time, close_time);

    return 0;
        
}

int ICACHE_FLASH_ATTR
hnt_platform_night_mode_get(char *buff)
{           
    snprintf(buff, sizeof(buff), "{\"mode\":\"%d\",\"normalTime\":\"%d\",\"closeTime\":\"%d\"}",
                                        wait_timer_param->group[NIGHTMODE_GROUP_INDEX].enable,
                                        wait_timer_param->group[NIGHTMODE_GROUP_INDEX].on_week_wait_time_second/60,
                                        wait_timer_param->group[NIGHTMODE_GROUP_INDEX].off_week_wait_time_second/60);
    
    log_debug("%s\n", buff);
    return 0;
}


#ifdef SECOND_TIMES
LOCAL os_timer_t device_timer2;
uint32 min_wait_second2;
hnt_platform_wait_timer_param_t *wait_timer_param2 = NULL;
struct wait_param pwait_action2;
hnt_platform_timer_action_func timer_action_func2 = NULL;

void user_hnt_platform_timer_action2(struct hnt_internal_wait_timer_param *wt_param, uint16 count);
void user_hnt_platform_timer_write_to_flash2(void);

/******************************************************************************
 * FunctionName : hnt_platform_find_min_time
 * Description  : find the minimum wait second in timer list
 * Parameters   : timer_wait_param -- param of timer action and wait time param
 *                count -- The number of timers given by server
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
hnt_platform_find_min_time2(struct hnt_internal_wait_timer_param *wt_param , uint16 count)
{
    uint16 i = 0;
    min_wait_second2 = 0xFFFFFFF;

    for (i = 0; i < count ; i++) {
        log_debug("test\n");
        if (wt_param[i].wait_time_second < min_wait_second2 && wt_param[i].wait_time_second >= 0) {
            min_wait_second2 = wt_param[i].wait_time_second;
        }
    }
}


/******************************************************************************
 * FunctionName : hnt_platform_fix_time_second_get
 * Description  : get fix time real time 
 * Parameters   : time_second_day -- second by day
 *                
 * Returns      : time_second_real -- real time
*******************************************************************************/

int ICACHE_FLASH_ATTR
hnt_platform_fix_time_second_get2(int time_second_day)
{
    int start_time_second_day = (wait_timer_param2->saveTimestamp - 1388937600) % ONE_DAY_SECONDS;
    int time_second_real = 0;

    if(time_second_day > start_time_second_day)
         time_second_real = wait_timer_param2->saveTimestamp - start_time_second_day + time_second_day;
    else
         time_second_real = wait_timer_param2->saveTimestamp - start_time_second_day + time_second_day + ONE_DAY_SECONDS;

    return time_second_real;
}

/******************************************************************************
 * FunctionName : hnt_platform_check_fix_time
 * Description  : check fix time 
 * Parameters   : on_time_second -- on time second
 *                      off_time_second-- off time second 
 * Returns      : 0: on/off fix time is not  
*******************************************************************************/

int ICACHE_FLASH_ATTR
hnt_platform_check_fix_time2(int on_time_second, int off_time_second)
{

    int current_time_second = 0;
    int on_time_second_real = -1;
    int off_time_second_real = -1;
    
    if(on_time_second != -1)
    {
       on_time_second_real = hnt_platform_fix_time_second_get2(on_time_second);
    }

    if(off_time_second != -1)
    {
        off_time_second_real = hnt_platform_fix_time_second_get2(off_time_second);
    }

    current_time_second = wait_timer_param2->timestamp;

    log_debug("Current_Time:%d On_Time:%d Off_Time:%d\n",current_time_second,on_time_second_real,off_time_second_real);
    if(on_time_second_real == -1 && current_time_second < off_time_second_real  )
        return 0; 
    if(current_time_second < on_time_second_real && off_time_second_real == -1)
        return 0;
    if(current_time_second < on_time_second_real || current_time_second < off_time_second_real)
        return 0;
    return -1;
    
}

/******************************************************************************
 * FunctionName : user_hnt_platform_timer_first_start
 * Description  : calculate the wait time of each timer
 * Parameters   : count -- The number of timers given by server
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_hnt_platform_timer_first_start2(void)
{
    int i = 0;
    uint16 count = 0;
    struct hnt_internal_wait_timer_param wt_param[16*MAX_TIMER_COUNT] = {0};

    log_debug("current timestamp= %ds\n", wait_timer_param2->timestamp);
    log_debug("min_wait_second= %ds\n", min_wait_second2);
    log_debug("timestamp+min_wait_second= %ds\n", wait_timer_param2->timestamp+min_wait_second2);

    if(get_xmpp_conn_status() == 1)
    {
        wait_timer_param2->timestamp = get_current_time();
        log_debug("wait_timer_param2->timestamp= %ds\n", wait_timer_param2->timestamp);
    }
    else
    {
        wait_timer_param2->timestamp += min_wait_second2;
    }
    
    if(wait_timer_param2->type == TIMER_TYPE_FIXDTIME)
    {
#if HNT_TIMER_FIX_SUPPORT
        for(i = 0; i < MAX_TIMER_COUNT; i++)
        {
            if(wait_timer_param2->group[i].enable != TIMER_ONE_ENABLE)
                continue;
                
            if((wait_timer_param2->group[i].on_fix_wait_time_second != 0) && 
               (wait_timer_param2->group[i].on_fix_wait_time_second > wait_timer_param2->timestamp))
            {
                log_debug("test\n");
                wt_param[count].wait_time_second = wait_timer_param2->group[i].on_fix_wait_time_second - 
                                                   wait_timer_param2->timestamp;  
                wt_param[count].on_off = TIMER_MODE_ON;  
                count++;
            }
            if((wait_timer_param2->group[i].off_fix_wait_time_second != 0) && 
               (wait_timer_param2->group[i].off_fix_wait_time_second > wait_timer_param2->timestamp))
            {
                log_debug("test\n");
                wt_param[count].wait_time_second = wait_timer_param2->group[i].off_fix_wait_time_second - 
                                                    wait_timer_param2->timestamp;              
                wt_param[count].on_off = TIMER_MODE_OFF;  
                count++;
            }
        }
#endif
    }
    else if(wait_timer_param2->type == TIMER_TYPE_LOOP_PERIOD)
    {
#if HNT_TIMER_LOOP_SUPPORT
        for(i = 0; i < MAX_TIMER_COUNT; i++)
        {
            if(wait_timer_param2->group[i].enable != TIMER_ONE_ENABLE)
                continue;
                
            if(wait_timer_param2->group[i].on_loop_wait_time_second != 0)
            {
                log_debug("test\n");
                wt_param[count].wait_time_second = wait_timer_param2->group[i].on_loop_wait_time_second - 
                                (wait_timer_param2->timestamp % wait_timer_param2->group[i].on_loop_wait_time_second);              
                wt_param[count].on_off = TIMER_MODE_ON;  
                count++;
            }
            if(wait_timer_param2->group[i].off_loop_wait_time_second != 0)
            {
                log_debug("test\n");
                wt_param[count].wait_time_second = wait_timer_param2->group[i].off_loop_wait_time_second - 
                                (wait_timer_param2->timestamp % wait_timer_param2->group[i].off_loop_wait_time_second);              
                wt_param[count].on_off = TIMER_MODE_OFF;  
                count++;
            }
        }
#endif
    }
    else if(wait_timer_param2->type == TIMER_TYPE_LOOP_WEEK)
    {
#if HNT_TIMER_WEEK_SUPPORT
        if(wait_timer_param2->group[DELAY_GROUP_INDEX].enable == TIMER_ONE_ENABLE)
        {
            log_debug("test\n");
            int delay_timer_flag = 2;
            if(wait_timer_param2->group[DELAY_GROUP_INDEX].on_week_wait_time_second != -1)
            {
                if (wait_timer_param2->group[DELAY_GROUP_INDEX].on_week_wait_time_second + 
                    wait_timer_param2->group[DELAY_GROUP_INDEX].weekday >  wait_timer_param2->timestamp) {
                    wt_param[count].wait_time_second = wait_timer_param2->group[DELAY_GROUP_INDEX].on_week_wait_time_second + 
                                                       wait_timer_param2->group[DELAY_GROUP_INDEX].weekday - 
                                                       wait_timer_param2->timestamp;

                    wt_param[count].on_off = TIMER_MODE_ON;  
                    wt_param[count].timer_index = DELAY_GROUP_INDEX;  
                    count++;
                    delay_timer_flag--;
                }
            }
            
            if(wait_timer_param2->group[DELAY_GROUP_INDEX].off_week_wait_time_second != -1)
            {
                if (wait_timer_param2->group[DELAY_GROUP_INDEX].off_week_wait_time_second + 
                    wait_timer_param2->group[DELAY_GROUP_INDEX].weekday >  wait_timer_param2->timestamp) {
                    wt_param[count].wait_time_second = wait_timer_param2->group[DELAY_GROUP_INDEX].off_week_wait_time_second + 
                                                       wait_timer_param2->group[DELAY_GROUP_INDEX].weekday - 
                                                       wait_timer_param2->timestamp;

                    wt_param[count].on_off = TIMER_MODE_OFF;  
                    wt_param[count].timer_index = DELAY_GROUP_INDEX;  
                    count++;
                    delay_timer_flag--;
                }
            }
            
            if(delay_timer_flag == 2)
            {
                wait_timer_param2->group[DELAY_GROUP_INDEX].enable = TIMER_ONE_DISABLE;
                user_hnt_platform_timer_write_to_flash2();      
                if(timer_action_func2 != NULL)
                    timer_action_func2(TIMER_MODE_ACTION_TIMER_CONFIG_CHANGED, -1, -1);
            }
        }
        
        
        int monday_wait_time = (wait_timer_param2->timestamp - 1388937600) % ONE_WEEK_SECONDS;   
        int day_wait_time =  (wait_timer_param2->timestamp - 1388937600) % ONE_DAY_SECONDS;
        int week;
        int week_wait_time = 0;
        log_debug("day_wait_time = %d", day_wait_time);
        log_debug("day_wait_time2 = %d", (wait_timer_param2->timestamp - 1388937600)%ONE_DAY_SECONDS);
        for(i = 1; i < MAX_TIMER_COUNT; i++)
        {        
            if(wait_timer_param2->group[i].enable != TIMER_ONE_ENABLE)
                continue;

            for(week = 0; week < 8; week++)
            {
                if(CHECK_FLAG_BIT(wait_timer_param2->group[i].weekday, week))
                {
                    week_wait_time = week*ONE_DAY_SECONDS;
                    if(wait_timer_param2->group[i].on_week_wait_time_second != -1)
                    {
                        log_debug("test\n");
                        if(week == 7)/*for fix time deal*/
                        {
                            if(hnt_platform_check_fix_time2(wait_timer_param2->group[i].on_week_wait_time_second,
                                wait_timer_param2->group[i].off_week_wait_time_second) == -1)
                            {
                                wait_timer_param2->group[i].enable = TIMER_ONE_DISABLE;
                                user_hnt_platform_timer_write_to_flash2();
                                if(timer_action_func2 != NULL)
                                    timer_action_func2(TIMER_MODE_ACTION_TIMER_CONFIG_CHANGED, -1, -1);
                                continue;
                            }
                            else
                            {
                                log_debug("day_wait_time: %d on_time:%d\n",day_wait_time,wait_timer_param2->group[i].on_week_wait_time_second);
                                if (wait_timer_param2->group[i].on_week_wait_time_second > day_wait_time)
                                    wt_param[count].wait_time_second = wait_timer_param2->group[i].on_week_wait_time_second
                                                                        -day_wait_time;
                                else
                                    wt_param[count].wait_time_second = ONE_DAY_SECONDS - day_wait_time + 
                                                                        wait_timer_param2->group[i].on_week_wait_time_second;
                            }
                        }
                        else
                        {
                            if (wait_timer_param2->group[i].on_week_wait_time_second + week_wait_time > monday_wait_time) {
                                wt_param[count].wait_time_second = wait_timer_param2->group[i].on_week_wait_time_second - 
                                                                   monday_wait_time + week_wait_time ;
                            } else {
                                wt_param[count].wait_time_second = ONE_WEEK_SECONDS - monday_wait_time + 
                                                                   wait_timer_param2->group[i].on_week_wait_time_second +
                                                                   week_wait_time;
                            }
                        }
                        if(i == NIGHTMODE_GROUP_INDEX)
                        {
                            wt_param[count].on_off = TIMER_MODE_ACTION_NIGHTMODE_ON;  
                        }
                        else
                        {
                            wt_param[count].on_off = TIMER_MODE_ON;  
                        }
                        wt_param[count].timer_index = i;  
                        count++;
                    }
                    if(wait_timer_param2->group[i].off_week_wait_time_second != -1)
                    {
                        log_debug("test\n");
                        if(week == 7)/*for fix time deal*/
                        {
                            if(hnt_platform_check_fix_time2(wait_timer_param2->group[i].on_week_wait_time_second,
                                wait_timer_param2->group[i].off_week_wait_time_second) == -1)
                            {
                                wait_timer_param2->group[i].enable = TIMER_ONE_DISABLE;
                                user_hnt_platform_timer_write_to_flash2();
                                if(timer_action_func2 != NULL)
                                    timer_action_func2(TIMER_MODE_ACTION_TIMER_CONFIG_CHANGED, -1, -1);
                                continue;
                            }
                            else
                            {
                                log_debug("day_wait_time: %d off_time:%d\n",day_wait_time,wait_timer_param2->group[i].off_week_wait_time_second);
                                if (wait_timer_param2->group[i].off_week_wait_time_second > day_wait_time)
                                    wt_param[count].wait_time_second = wait_timer_param2->group[i].off_week_wait_time_second
                                                                        -day_wait_time;
                                else
                                    wt_param[count].wait_time_second = ONE_DAY_SECONDS - day_wait_time+
                                                                        wait_timer_param2->group[i].off_week_wait_time_second;
                            }               
                        }
                        else
                        {
                            if (wait_timer_param2->group[i].off_week_wait_time_second + week_wait_time > monday_wait_time) {
                                wt_param[count].wait_time_second = wait_timer_param2->group[i].off_week_wait_time_second - 
                                                                   monday_wait_time + week_wait_time;
                            } else {
                                wt_param[count].wait_time_second = ONE_WEEK_SECONDS - monday_wait_time + 
                                                                   wait_timer_param2->group[i].off_week_wait_time_second +
                                                                   week_wait_time;
                            }
                        }
                        if(i == NIGHTMODE_GROUP_INDEX)
                        {
                            wt_param[count].on_off = TIMER_MODE_ACTION_NIGHTMODE_OFF;  
                        }
                        else
                        {
                            wt_param[count].on_off = TIMER_MODE_OFF;  
                        }
                        wt_param[count].timer_index = i;  
                        count++;
                    }
                 }
            }
        }
#endif
    }
    
    for(i = 0; i < count; i++)
    {
        log_debug("wt_param[%d].wait_time_second = %d\n", i, wt_param[i].wait_time_second);
        log_debug("wt_param[%d].on_off = %d\n", i, wt_param[i].on_off);
        log_debug("wt_param[%d].timer_index = %d\n", i, wt_param[i].timer_index);
    }
    
    hnt_platform_find_min_time2(wt_param, count);
    if((min_wait_second2 == 0) || (count == 0)) {
        os_timer_disarm(&device_timer2);
        log_debug("test\n");
    	return;
    }

    user_hnt_platform_timer_action2(wt_param, count);
}

/******************************************************************************
 * FunctionName : user_hnt_platform_device_action
 * Description  : Execute the actions of minimum wait time
 * Parameters   : pwait_action2 -- point the list of actions which need execute
 *
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_hnt_platform_device_action2(struct wait_param *pwait_action2)
{
    uint8 i = 0;
    uint8 timer_index = 0;
    uint16 action_number = pwait_action2->action_number;

    log_debug("there is %d action at the same time\n", pwait_action2->action_number);

    for (i = 0; i < action_number; i++) {
        timer_index = pwait_action2->timer_index[i];
        log_debug("%d:%d\n", timer_index, pwait_action2->on_off[i]);

        if(pwait_action2->on_off[i] == TIMER_MODE_ON)
        {
            if(timer_action_func2 != NULL)
                timer_action_func2((pwait_action2->on_off[i] + TIMER_MODE_TIMER2_OFFSET), timer_index,
                wait_timer_param2->group[timer_index].on_week_wait_time_second);
        }
        else if(pwait_action2->on_off[i] == TIMER_MODE_OFF)
        {
            if(timer_action_func2 != NULL)
                timer_action_func2((pwait_action2->on_off[i] + TIMER_MODE_TIMER2_OFFSET), timer_index,
                wait_timer_param2->group[timer_index].off_week_wait_time_second);
        }
        else if(pwait_action2->on_off[i] == TIMER_MODE_ACTION_NIGHTMODE_ON)
        {
            if(timer_action_func2 != NULL)
                timer_action_func2((pwait_action2->on_off[i] + TIMER_MODE_TIMER2_OFFSET), timer_index,
                wait_timer_param2->group[timer_index].on_week_wait_time_second);
        }
        else if(pwait_action2->on_off[i] == TIMER_MODE_ACTION_NIGHTMODE_OFF) 
        {
            if(timer_action_func2 != NULL)
                timer_action_func2((pwait_action2->on_off[i] + TIMER_MODE_TIMER2_OFFSET), timer_index,
                wait_timer_param2->group[timer_index].off_week_wait_time_second);
        } 
        else 
        {
            log_debug("test\n");
            return;
        }
    }
    user_hnt_platform_timer_first_start2();
}

/******************************************************************************
 * FunctionName : hnt_platform_timer_start
 * Description  : Processing the message about timer from the server
 * Parameters   : timer_wait_param -- The received data from the server
 *                count -- the espconn used to connetion with the host
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_hnt_platform_wait_time_overflow_check2(struct wait_param *pwait_action2)
{
    log_debug("min_wait_second = %d\n", min_wait_second2);

    if (pwait_action2->min_time_backup >= 3600) {
        os_timer_disarm(&device_timer2);
        os_timer_setfn(&device_timer2, (os_timer_func_t *)user_hnt_platform_wait_time_overflow_check2, pwait_action2);
        os_timer_arm(&device_timer2, 3600000, 0);
        log_debug("min_wait_second2 is extended\n");
    } else {
        os_timer_disarm(&device_timer2);
        os_timer_setfn(&device_timer2, (os_timer_func_t *)user_hnt_platform_device_action2, pwait_action2);
        os_timer_arm(&device_timer2, pwait_action2->min_time_backup * 1000, 0);
        log_debug("min_wait_second2 is = %dms\n", pwait_action2->min_time_backup * 1000);
    }

    pwait_action2->min_time_backup -= 3600;
}

void ICACHE_FLASH_ATTR
user_hnt_platform_timer_write_to_flash2(void)
{
    struct hnt_mgmt_saved_param param;
    memset(&param, 0, sizeof(param));

    hnt_mgmt_load_param(&param);

	uint16 i = 0;

    memcpy(param.group2, wait_timer_param2->group, sizeof(wait_timer_param2->group));
    param.saveTimestamp2 = wait_timer_param2->saveTimestamp;

	for(i=0;i<MAX_TIMER_COUNT;i++)
	{
		log_debug("group%d :On:%d Off:%d week:%x enable:%d\n",i,wait_timer_param2->group[i].on_week_wait_time_second,wait_timer_param2->group[i].off_week_wait_time_second,
										wait_timer_param2->group[i].weekday,wait_timer_param2->group[i].enable);
	}
	hnt_mgmt_save_param(&param);
}

/******************************************************************************
 * FunctionName : user_hnt_platform_timer_action
 * Description  : Processing the message about timer from the server
 * Parameters   : timer_wait_param -- The received data from the server
 *                count -- the espconn used to connetion with the host
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_hnt_platform_timer_action2(struct hnt_internal_wait_timer_param *wt_param, uint16 count)
{
    uint16 i = 0;
    uint16 action_number;

    memset(&pwait_action2, 0, sizeof(pwait_action2)); 
    action_number = 0;

    for (i = 0; i < count ; i++) {
        if (wt_param[i].wait_time_second == min_wait_second2) {
            pwait_action2.on_off[action_number] = wt_param[i].on_off;
            pwait_action2.timer_index[action_number] = wt_param[i].timer_index;
            log_debug("*****%d:%d*****\n", wt_param[i].timer_index, wt_param[i].on_off);
            action_number++;
        }
    }

    pwait_action2.action_number = action_number;
    pwait_action2.min_time_backup = min_wait_second2;
    user_hnt_platform_wait_time_overflow_check2(&pwait_action2);
}

void ICACHE_FLASH_ATTR
user_platform_timer_stop2(void)
{
    if(wait_timer_param2 != NULL)
    {
        free(wait_timer_param2);
        wait_timer_param2 = NULL;
    }
}
/******************************************************************************
 * FunctionName : hnt_platform_timer_start
 * Description  : Processing the message about timer from the server
 * Parameters   : pbuffer -- The received data from the server

 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
hnt_platform_timer_start2(char *pbuffer)
{
    int str_begin = 0;
    int str_end = 0;
    char *pstr_start = NULL;
    char *pstr_end = NULL;
    char *pstr = NULL;
    char temp_str[256] = {0};
    uint16 count = 0;
    char *timer_splits[20] = {NULL};
    int i;
    int fix_index = 0;
    int loop_index = 0;
    int week_index = 0;
    int timestamp = 0;

    log_debug("test\n");
    if(timer_action_func2 == NULL) 
        return;

    if(wait_timer_param2 == NULL)
        wait_timer_param2 = (hnt_platform_wait_timer_param_t *)zalloc(sizeof(hnt_platform_wait_timer_param_t));

    memset(&(wait_timer_param2->group[0]), 0, (MAX_TIMER_COUNT-1)*sizeof(hnt_timer_group_param_t));        
    min_wait_second2  = 0;
    log_debug("test\n");

    json_get_value(pbuffer, "timestamp", temp_str, sizeof(temp_str) - 1);
    log_debug("test:temp_str = %s\n", temp_str);
    timestamp = atoi(temp_str);
    wait_timer_param2->timestamp = get_current_time();

    memset(temp_str, 0, sizeof(temp_str));
    json_get_value(pbuffer, "type", temp_str, sizeof(temp_str) - 1);
    log_debug("test:temp_str = %s\n", temp_str);
    if(temp_str[0] == 'f')
        wait_timer_param2->type = TIMER_TYPE_FIXDTIME;
    else if(temp_str[0] == 'l')
        wait_timer_param2->type = TIMER_TYPE_LOOP_PERIOD;
    else if(temp_str[0] == 'w')
        wait_timer_param2->type = TIMER_TYPE_LOOP_WEEK;
    else
    {
        user_platform_timer_stop2();
        return;
    }
    log_debug("test\n");

    memset(temp_str, 0, sizeof(temp_str));
    json_get_value(pbuffer, "enable", temp_str, sizeof(temp_str) - 1);
    log_debug("test:temp_str = %s\n", temp_str);
    count = split(temp_str , ";" , timer_splits);
    log_debug("test:count = %d\n", count);
    fix_index = 0;
    loop_index = 0;
    week_index = 0;

    for(i = 0; (i < count) && (i < MAX_TIMER_COUNT-1); i++)
    {
        pstr = timer_splits[i];
        if(*pstr == '1')
            wait_timer_param2->group[fix_index++].enable = TIMER_ONE_ENABLE;
        else if(*pstr == '0')
            wait_timer_param2->group[fix_index++].enable = TIMER_ONE_DISABLE; 
        else
        {
            free(timer_splits[i]);
            return; 
        }
        
        free(timer_splits[i]);
    }
    
    memset(temp_str, 0, sizeof(temp_str));
    json_get_value(pbuffer, "on", temp_str, sizeof(temp_str) - 1);
    log_debug("test:temp_str = %s\n", temp_str);
    count = split(temp_str , ";" , timer_splits);
    log_debug("test:count = %d\n", count);
    for(i = 0; (i < count) && (i < MAX_TIMER_COUNT-1); i++)
    {
        pstr = timer_splits[i];
        log_debug("test:timer_splits[%d] = %s\n", i, timer_splits[i]);
        if (*pstr == 'f')
        {
#if HNT_TIMER_FIX_SUPPORT
            wait_timer_param2->group[++fix_index].on_fix_wait_time_second = atoi(pstr + 1);
#endif
        }
        else if(*pstr == 'l')
        {
#if HNT_TIMER_LOOP_SUPPORT
            wait_timer_param2->group[++loop_index].on_loop_wait_time_second = atoi(pstr + 1);
#endif
        }
        else if(*pstr == 'w')
        {
#if HNT_TIMER_WEEK_SUPPORT
            wait_timer_param2->group[++week_index].on_week_wait_time_second = atoi(pstr + 1);
#endif
        }
        else if(*pstr == 'd')
        {
            wait_timer_param2->group[0].on_week_wait_time_second = atoi(pstr + 1);            
        }
        
        free(timer_splits[i]);
    }
    log_debug("test\n");

    memset(temp_str, 0, sizeof(temp_str));
    json_get_value(pbuffer, "off", temp_str, sizeof(temp_str) - 1);
    log_debug("test:temp_str = %s\n", temp_str);
    count = split(temp_str , ";" , timer_splits);
    log_debug("test:count = %d\n", count);
    fix_index = 0;
    loop_index = 0;
    week_index = 0;
    for(i = 0; (i < count) && (i < MAX_TIMER_COUNT-1); i++)
    {
        pstr = timer_splits[i];
        log_debug("test:timer_splits[%d] = %s\n", i, timer_splits[i]);
        if (*pstr == 'f')
        {
#if HNT_TIMER_FIX_SUPPORT
            wait_timer_param2->group[++fix_index].off_fix_wait_time_second = atoi(pstr + 1);
#endif
        }
        else if(*pstr == 'l')
        {
#if HNT_TIMER_LOOP_SUPPORT
            wait_timer_param2->group[++loop_index].off_loop_wait_time_second = atoi(pstr + 1);
#endif
        }
        else if(*pstr == 'w')
        {
#if HNT_TIMER_WEEK_SUPPORT
            wait_timer_param2->group[++week_index].off_week_wait_time_second = atoi(pstr + 1);
#endif
        }
        else if(*pstr == 'd')
        {
            wait_timer_param2->group[0].off_week_wait_time_second = atoi(pstr + 1);            
        }
        
        free(timer_splits[i]);
    }
    log_debug("test\n");
#if HNT_TIMER_WEEK_SUPPORT
    memset(temp_str, 0, sizeof(temp_str));
    json_get_value(pbuffer, "weekday", temp_str, sizeof(temp_str) - 1);
    log_debug("test:temp_str = %s\n", temp_str);
    count = split(temp_str , ";" , timer_splits);
    log_debug("test:count = %d\n", count);
    week_index = 0;
    for(i = 0; (i < count) && (i < MAX_TIMER_COUNT-1); i++)
    {
        wait_timer_param2->group[i].weekday = 0;
        pstr = timer_splits[i];
        log_debug("test:timer_splits[%d] = %s\n", i, timer_splits[i]);
        while(*pstr != '\0')
        {
            week_index = *pstr - '1';
            log_debug("test week = %d\n", week_index);
            SET_FLAG_BIT(wait_timer_param2->group[i].weekday, week_index);
            pstr++;
        }
        free(timer_splits[i]);
    }

    wait_timer_param2->group[DELAY_GROUP_INDEX].weekday = timestamp;//wait_timer_param2->timestamp;
    wait_timer_param2->saveTimestamp = get_current_time();
#endif    
    user_hnt_platform_timer_write_to_flash2();

    user_hnt_platform_timer_first_start2();
}

void ICACHE_FLASH_ATTR
hnt_platform_timer_get2(char *buffer)
{
    int len = 0;
    int i = 0;
    int timer_count = 0;
    int week_index = 0;

    if(wait_timer_param2 == NULL)
        return;
        
    struct hnt_mgmt_saved_param param;
    memset(&param, 0, sizeof(param));
    hnt_mgmt_load_param(&param);
    
    len = snprintf(buffer, sizeof(buffer), "{\"type\":\"w\",\"timestamp\":\"%d\",\"enable\":\"",
                                    wait_timer_param2->group[DELAY_GROUP_INDEX].weekday);

    for(i = 0; i < MAX_TIMER_COUNT-1; i++)
    {    
        if((wait_timer_param2->group[i].enable != TIMER_ONE_DISABLE) && 
            (wait_timer_param2->group[i].enable != TIMER_ONE_ENABLE)
            )
        {
            log_debug("test\n");
            break;  
        }
    }

    timer_count = i;
    if(timer_count == 0)
    {
        log_debug("test\n");
        len += snprintf(buffer+len, sizeof(buffer) - len, "\",\"weekday\":\"\",\"on\":\"\",\"off\":\"\"}");        
    }
    else
    {
        log_debug("test\n");
        for(i = 0; i < timer_count - 1; i++)
        {
            len += snprintf(buffer+len, sizeof(buffer)-len, "%d;", wait_timer_param2->group[i].enable - 1);
        }
        len += snprintf(buffer+len, sizeof(buffer)-len, "%d\",\"weekday\":\"", wait_timer_param2->group[i].enable - 1);

        for(i = 0; i < timer_count - 1; i++)
        {
            if(i == 0)
                len += snprintf(buffer+len, sizeof(buffer)-len, "-1");
            else
            {
                for((week_index = 0); 
                    week_index < 8; 
                    week_index++)
                {
                    if(CHECK_FLAG_BIT(wait_timer_param2->group[i].weekday, week_index))
                        len += snprintf(buffer+len, sizeof(buffer)-len, "%d", week_index+1);
                }
            }
            len += snprintf(buffer+len, sizeof(buffer)-len, ";");
        }

        if(i == 0)
            len += snprintf(buffer+len, sizeof(buffer)-len, "-1");
        else
        {
            for(week_index = 0; 
             week_index < 8; 
             week_index++)
            {
                if(CHECK_FLAG_BIT(wait_timer_param2->group[i].weekday, week_index))
                    len += snprintf(buffer+len, sizeof(buffer)-len, "%d", week_index+1);
            }
        }
        len += snprintf(buffer+len, sizeof(buffer)-len, "\",\"on\":\"");

        
        for(i = 0; i < timer_count - 1; i++)
        {
            if(i == 0)
                len += snprintf(buffer+len, sizeof(buffer)-len, "d%d;", 
                                wait_timer_param2->group[i].on_week_wait_time_second);
            else            
                len += snprintf(buffer+len, sizeof(buffer)-len, "w%d;", 
                            wait_timer_param2->group[i].on_week_wait_time_second);
        }
        
        if(i == 0)
            len += snprintf(buffer+len, sizeof(buffer)-len, "d%d\",\"off\":\"", 
                            wait_timer_param2->group[i].on_week_wait_time_second);
        else
            len += snprintf(buffer+len, sizeof(buffer)-len, "w%d\",\"off\":\"", 
                        wait_timer_param2->group[i].on_week_wait_time_second);

        for(i = 0; i < timer_count - 1; i++)
        {
            if(i == 0)
                len += snprintf(buffer+len, sizeof(buffer)-len, "d%d;", 
                            wait_timer_param2->group[i].off_week_wait_time_second);
            else            
                len += snprintf(buffer+len, sizeof(buffer)-len, "w%d;", 
                            wait_timer_param2->group[i].off_week_wait_time_second);
        }
        if(i == 0)
            len += snprintf(buffer+len, sizeof(buffer)-len, "d%d\"}", 
                            wait_timer_param2->group[i].off_week_wait_time_second);
        else
            len += snprintf(buffer+len, sizeof(buffer)-len, "w%d\"}", 
                        wait_timer_param2->group[i].off_week_wait_time_second);
    }

    log_debug("%s", buffer);
}
#endif

void ICACHE_FLASH_ATTR
hnt_platform_timer_init(void)
{
    if(timer_action_func == NULL)
        return;

    timer_action_func(TIMER_MODE_SET_TIME, 0, 0);
        
    if(wait_timer_param != NULL)    
        return;
       
    struct hnt_mgmt_saved_param param;
    memset(&param, 0, sizeof(param));
    hnt_mgmt_load_param(&param);

    wait_timer_param = (hnt_platform_wait_timer_param_t *)zalloc(sizeof(hnt_platform_wait_timer_param_t));
    memset(wait_timer_param, 0, sizeof(hnt_platform_wait_timer_param_t));        
    min_wait_second  = 0;
    wait_timer_param->timestamp = get_current_time();    
    wait_timer_param->type = TIMER_TYPE_LOOP_WEEK;
    wait_timer_param->saveTimestamp = param.saveTimestamp;    
    memcpy(wait_timer_param->group, param.group, sizeof(wait_timer_param->group));

    if((wait_timer_param->group[NIGHTMODE_GROUP_INDEX].enable != NIGHT_MODE_NORMAL) &&
        (wait_timer_param->group[NIGHTMODE_GROUP_INDEX].enable != NIGHT_MODE_CLOSE) &&
        (wait_timer_param->group[NIGHTMODE_GROUP_INDEX].enable != NIGHT_TIMER_MODE))
    {
        wait_timer_param->group[NIGHTMODE_GROUP_INDEX].enable = NIGHT_MODE_NORMAL;
    }

    hnt_platform_night_mode_start(wait_timer_param->group[NIGHTMODE_GROUP_INDEX].enable,
                                wait_timer_param->group[NIGHTMODE_GROUP_INDEX].on_week_wait_time_second,
                                wait_timer_param->group[NIGHTMODE_GROUP_INDEX].off_week_wait_time_second);     

    user_hnt_platform_timer_first_start();


#ifdef SECOND_TIMER
    if(wait_timer_param2 != NULL)    
        return;

    wait_timer_param2 = (hnt_platform_wait_timer_param_t *)zalloc(sizeof(hnt_platform_wait_timer_param_t));
    memset(wait_timer_param2, 0, sizeof(hnt_platform_wait_timer_param_t));        
    min_wait_second2  = 0;
    wait_timer_param2->timestamp = get_current_time();    
    wait_timer_param2->type = TIMER_TYPE_LOOP_WEEK;
    wait_timer_param2->saveTimestamp = param.saveTimestamp2;    
    memcpy(wait_timer_param2->group, param.group2, sizeof(wait_timer_param2->group));
#endif
}

void ICACHE_FLASH_ATTR
hnt_platform_timer_action_regist(void *timer_action_cb)
{
    timer_action_func = (hnt_platform_timer_action_func)timer_action_cb;
#ifdef SECOND_TIMES
    timer_action_func2 = (hnt_platform_timer_action_func)timer_action_cb;
#endif
}

#endif

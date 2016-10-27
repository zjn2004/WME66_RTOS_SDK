#ifndef __MGMT_TIMER_H__
#define __MGMT_TIMER_H__

#define NIGHT_MODE_NORMAL          0
#define NIGHT_MODE_CLOSE         1
#define NIGHT_TIMER_MODE       2     


#define TIMER_TYPE_FIXDTIME   1
#define TIMER_TYPE_LOOP_PERIOD 2
#define TIMER_TYPE_LOOP_WEEK    3
#define TIMER_ONE_ENABLE            2
#define TIMER_ONE_DISABLE           1
#define ONE_DAY_SECONDS        (24 * 3600)
#define ONE_WEEK_SECONDS        (7 * ONE_DAY_SECONDS)


void hnt_platform_timer_init(void);


#endif

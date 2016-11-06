#ifndef _USER_SMARTPLUG_H
#define _USER_SMARTPLUG_H

#define GPIO_VALUE_0 0
#define GPIO_VALUE_1 1

/* LED */
#define SMARTPLUG_POWER_LED_GPIO     12

#define SMARTPLUG_WIFI_LED_GPIO      13


/* KEY */
#define SMARTPLUG_KEY_NUM          1

#define SMARTPLUG_KEY1_IO_MUX       PERIPHS_IO_MUX_MTMS_U
#define SMARTPLUG_KEY1_IO_GPIO      14
#define SMARTPLUG_KEY1_IO_FUNC      FUNC_GPIO14

#if 0
#define SMARTPLUG_KEY2_IO_MUX       PERIPHS_IO_MUX_GPIO4_U
#define SMARTPLUG_KEY2_IO_GPIO      4
#define SMARTPLUG_KEY2_IO_FUNC      FUNC_GPIO4
#endif

/* CONTROL */
#define SMARTPLUG_CTRL_IO_MUX     PERIPHS_IO_MUX_MTDO_U
#define SMARTPLUG_CTRL_IO_GPIO    15
#define SMARTPLUG_CTRL_IO_FUNC    FUNC_GPIO15

/* PATH */
#define SMARTPLUG_PATH_DEVICE_SWITCH              "Device.SmartPlug.Socket.1.Switch"
#define SMARTPLUG_PATH_DEVICE_TIMER               "Device.DeviceInfo.Timer"
#define SMARTPLUG_PATH_DEVICE_WELCOME             "Device.DeviceInfo.Welcome"
#define SMARTPLUG_PATH_DEVICE_NIGHTMODE           "Device.DeviceInfo.NightMode"
#define SMARTPLUG_PATH_DEVICE_WIFILED             "Device.DeviceInfo.WiFiLed"

/* CUSTOM INFO */
#define SMARTPLUG_DEVICEINFO_SOFTWARE_VERSION     "BSPS002-V1.0.1.0074"
#define SMARTPLUG_DEVICEINFO_MODEL_NAME           "BSPS002"
#define SMARTPLUG_DEVICEINFO_MANUFACTURER         "Seaing Technology Co.,Ltd., www.seaing.net"
#define SMARTPLUG_XMPP_INFORM_INTERVAL_SEC        3600
#define SMARTPLUG_XMPP_SAMPLING_INTERVAL_SEC      300

#endif /*_USER_SMARTPLUG_H */

#include "esp_common.h" 

#include "hnt_interface.h"
#include "xmpp/xmppcli_cwmp_parameters.h"
#include "xmpp/xmppcli_cwmp.h"
#include "upgrade.h"
#include "mgmt/mgmt.h"

#define DEVICEINFO_SOFTWARE_VERSION     "SD001-V1.0.1.0002"
#define DEVICEINFO_MODEL_NAME           "SD001"
#define DEVICEINFO_MANUFACTURER         "Seaing Technology Co.,Ltd., www.seaing.net"

extern struct upgrade_server_info *xmpp_download_server;
extern struct hnt_factory_param g_hnt_factory_param;

deviceParameterTable_t DeviceCustomParamTable = {NULL, 0};
customInfo_t *DeviceCustomInfo = NULL;
hnt_xmpp_slavedev_upgrade_process_func slave_upgrade_process = NULL;


void ICACHE_FLASH_ATTR
cwmp_datatype_set_bool(char *value, uint8_t data)
{
    sprintf(value, "%s", data?"true":"false");
}

void ICACHE_FLASH_ATTR
cwmp_datatype_set_uint(char *value, uint32_t data)
{
    sprintf(value, "%d", data);
}

/* */
int ICACHE_FLASH_ATTR
getInformIntervalFunc(char *paramName, char *value, int size)
{
    log_debug("test\n");
    sprintf(value, "%d", inform_intval_seconds_);
    return Success;
}

int ICACHE_FLASH_ATTR
getSamplingIntervalFunc(char *paramName, char *value, int size)
{
    log_debug("test\n");
    sprintf(value, "%d", sampling_intval_seconds_);
    return Success;
}

int ICACHE_FLASH_ATTR
getDeviceTypeFunc(char *paramName, char *value, int size)
{
    log_debug("test\n");
    
    sprintf(value, "%d", g_hnt_factory_param.device_type);
    return Success;
}

int ICACHE_FLASH_ATTR
getXMPPJIDFunc(char *paramName, char *value, int size)
{
    log_debug("test\n");
    
    sprintf(value, "%s", g_hnt_factory_param.xmpp_jid);
    return Success;
}

int ICACHE_FLASH_ATTR
getManufacturerFunc(char *paramName, char *value, int size)
{
    log_debug("test\n");

    if((DeviceCustomInfo != NULL) && DeviceCustomInfo->deviceInfoManufacturer != NULL)
        sprintf(value, "%s", DeviceCustomInfo->deviceInfoManufacturer);
    else
        sprintf(value, "%s", DEVICEINFO_MANUFACTURER);

    return Success;
}
int ICACHE_FLASH_ATTR
getModelNameFunc(char *paramName, char *value, int size)
{
    log_debug("test\n");

    if((DeviceCustomInfo != NULL) && DeviceCustomInfo->deviceInfoModelName != NULL)
        sprintf(value, "%s", DeviceCustomInfo->deviceInfoModelName);
    else
        sprintf(value, "%s", DEVICEINFO_MODEL_NAME);

    return Success;
}
int ICACHE_FLASH_ATTR
getSerialNumberFunc(char *paramName, char *value, int size)
{
    log_debug("test\n");
    
    sprintf(value, "%s", g_hnt_factory_param.device_id);
    return Success;
}
int ICACHE_FLASH_ATTR
getSoftwareVersionFunc(char *paramName, char *value, int size)
{
    log_debug("test\n");

    if((DeviceCustomInfo != NULL) && DeviceCustomInfo->deviceInfoSv != NULL)
        sprintf(value, "%s", DeviceCustomInfo->deviceInfoSv);
    else
        sprintf(value, "%s", DEVICEINFO_SOFTWARE_VERSION);
        
    return Success;
}

int ICACHE_FLASH_ATTR
getUpgradeStatusFunc(char *paramName, char *value, int size)
{
    log_debug("test\n");
#if 0    
    if(xmpp_download_server != NULL)
        sprintf(value, "%d", (xmpp_download_server->upgrade_status & 0x0F));
    else
        sprintf(value, "%d", 0);
#endif    
    return Success;
}

int ICACHE_FLASH_ATTR
getUpgradeProgressFunc(char *paramName, char *value, int size)
{
    log_debug("test\n");
#if 0    
    if(xmpp_download_server != NULL)
        sprintf(value, "%d", xmpp_download_server->upgrade_progress);
    else
        sprintf(value, "%d", 0);
#endif    
    return Success;
}

int ICACHE_FLASH_ATTR
getMACAdressFunc(char *paramName, char *value, int size)
{
    log_debug("test\n");
    sprintf(value, MACSTR, MAC2STR(g_hnt_factory_param.wlan_mac));

    return Success;
}

int ICACHE_FLASH_ATTR
getWifiLinkSsidFunc(char *paramName, char *value, int size)
{
    log_debug("test\n");    
    struct station_config sta_conf;
    wifi_station_get_config(&sta_conf);
    
    sprintf(value, "%s", sta_conf.ssid);

    return Success;
}

deviceGeneralParameter_t DeviceGeneralParamTable[] = {
//    {"Device.DeviceInfo.DeviceType", getXMPPJIDFunc, NULL},
    {"Device.DeviceInfo.XMPPJID", getXMPPJIDFunc, NULL},
    {"Device.DeviceInfo.Manufacturer", getManufacturerFunc, NULL},
#if PROFILE_FLOORHEAT 
    /*floor heat need special model name, move to customer*/
#else
    {"Device.DeviceInfo.ModelName", getModelNameFunc, NULL},
#endif    
    {"Device.DeviceInfo.SerialNumber", getSerialNumberFunc, NULL},
    {"Device.DeviceInfo.SoftwareVersion", getSoftwareVersionFunc, NULL},
    {"Device.DeviceInfo.UpgradeStatus", getUpgradeStatusFunc, NULL},
    {"Device.DeviceInfo.UpgradeProgress", getUpgradeProgressFunc, NULL},
    {"Device.DeviceInfo.MACAddress", getMACAdressFunc, NULL},
    {"Device.WANDevice.1.WANWIFIInterfaceConfig.WifiLinkSsid", getWifiLinkSsidFunc, NULL},
//    {"Device.Inform.HistoryData.InformInterval", getInformIntervalFunc, NULL},
//    {"Device.Inform.HistoryData.SamplingInterval", getSamplingIntervalFunc, NULL},
    {NULL,NULL,NULL}
};

void ICACHE_FLASH_ATTR
hnt_xmpp_custom_info_regist(customInfo_t *customInfo)
{
    DeviceCustomInfo = customInfo;
}

void ICACHE_FLASH_ATTR
hnt_xmpp_param_array_regist(deviceParameter_t *custom_xmpp_param_array, uint32 array_size)
{
    DeviceCustomParamTable.tableSize = array_size;
    DeviceCustomParamTable.table = custom_xmpp_param_array;
}

void ICACHE_FLASH_ATTR
hnt_xmpp_slavedev_upgrade_func_regist(void *func)
{
    slave_upgrade_process = (hnt_xmpp_slavedev_upgrade_process_func)func;
}


void ICACHE_FLASH_ATTR
hnt_xmpp_notif_update_data(char *path_fullname, char *para_value)
{
    parameterValueList_s paramList;

    if (get_xmpp_conn_status() == 0)    
        return;

    memset(&paramList, 0, sizeof(paramList)); 
    paramList.size = 1;
    paramList.parameterValueStructs = (parameterValueStruct_s *)malloc(sizeof(parameterValueStruct_s));
    memset(paramList.parameterValueStructs, 0, sizeof(parameterValueStruct_s));

    paramList.parameterValueStructs->Name = strdup(path_fullname);
    paramList.parameterValueStructs->Value = strdup(para_value);
    
    CwmpInformConfigChanged(&paramList); 
    CwmpInformUpdateData(&paramList);
    CwmpCheckOneAlarm(paramList.parameterValueStructs->Name, paramList.parameterValueStructs->Value);
    free(paramList.parameterValueStructs->Name);
    free(paramList.parameterValueStructs->Value);
    free(paramList.parameterValueStructs);    
}


void ICACHE_FLASH_ATTR
hnt_xmpp_notif_real_data_changed(char *path_fullname, char *para_value)
{
    parameterValueList_s paramList;

    if (get_xmpp_conn_status() == 0)    
        return;

    memset(&paramList, 0, sizeof(paramList)); 
    paramList.size = 1;
    paramList.parameterValueStructs = (parameterValueStruct_s *)malloc(sizeof(parameterValueStruct_s));
    memset(paramList.parameterValueStructs, 0, sizeof(parameterValueStruct_s));

    paramList.parameterValueStructs->Name = strdup(path_fullname);
    paramList.parameterValueStructs->Value = strdup(para_value);
    
    CwmpInformRealDataChanged(&paramList);
    free(paramList.parameterValueStructs->Name);
    free(paramList.parameterValueStructs->Value);
    free(paramList.parameterValueStructs);    
}

void ICACHE_FLASH_ATTR
hnt_xmpp_notif_config_changed(char *path_fullname, char *para_value)
{
    parameterValueList_s paramList;

    if (get_xmpp_conn_status() == 0)    
        return;

    memset(&paramList, 0, sizeof(paramList)); 
    paramList.size = 1;
    paramList.parameterValueStructs = (parameterValueStruct_s *)malloc(sizeof(parameterValueStruct_s));
    memset(paramList.parameterValueStructs, 0, sizeof(parameterValueStruct_s));

    paramList.parameterValueStructs->Name = strdup(path_fullname);
    paramList.parameterValueStructs->Value = strdup(para_value);
    
    CwmpInformConfigChanged(&paramList); 

    free(paramList.parameterValueStructs->Name);
    free(paramList.parameterValueStructs->Value);
    free(paramList.parameterValueStructs);    
}

void ICACHE_FLASH_ATTR
hnt_xmpp_notif_alarm(char *alarmInfo, int level)
{
    if (get_xmpp_conn_status() == 0)    
        return;
    
    CwmpInformAlarm(alarmInfo, level);
}

void ICACHE_FLASH_ATTR
hnt_xmpp_notif_device_info(void)
{
    if (get_xmpp_conn_status() == 0)    
        return;
        
    CwmpInformDeviceInfo();
}

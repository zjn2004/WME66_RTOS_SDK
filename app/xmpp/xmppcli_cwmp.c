#include "esp_common.h" 

#include "hnt_interface.h"
#include "espconn.h"
#include "upgrade.h"
#include "iksemel/iksemel.h"

#include "xmpp/xmppcli.h"
#include "xmpp/xmppcli_cwmp.h"
#include "xmpp/xmppcli_cwmp_parameters.h"
#include "mgmt/mgmt.h"

extern deviceGeneralParameter_t DeviceGeneralParamTable[];
extern deviceParameterTable_t DeviceCustomParamTable;
extern hnt_xmpp_slavedev_upgrade_process_func slave_upgrade_process;
extern customInfo_t *DeviceCustomInfo;

#define XMPP_INFORM_INTERVAL_SEC     3600
#define XMPP_SAMPLING_INTERVAL_SEC           300

int inform_data_en_ = 1;
int inform_alarm_en_ = 1;
unsigned int inform_intval_seconds_ = XMPP_INFORM_INTERVAL_SEC;
unsigned int sampling_intval_seconds_ = XMPP_SAMPLING_INTERVAL_SEC;

LOCAL historyDataInform_s *m_historyDataStructs = NULL;
LOCAL os_timer_t xmpp_rpc_timer;
const char* fault_get_codestring(int fault_code);
void SendCwmpRpcRsp(void *h, const char *to, const char *iq_id, 
                        xmpp_stanza_t *rsp_el);

#ifdef DLMPD_SUPPORT
void ICACHE_FLASH_ATTR
SendCwmpRpcErrorRsp(void * h)
{
    char s[10] = {0};
    xmpp_stanza_t* auth_el = xmpp_stanza_create("CwmprpcAuthenticationResponse");
    xmpp_stanza_t* status_el = xmpp_stanza_create_within(QN_Status, auth_el);
    xmpp_stanza_t* status_str_el = xmpp_stanza_create_within(QN_StatusString, auth_el);
    
    snprintf(s, sizeof(s), "%d", Request_Denied);
    xmpp_stanza_set_contents(status_el, s);
    xmpp_stanza_set_contents(status_str_el, fault_get_codestring(Request_Denied)); 

    xmpp_stanza_add_child_release(auth_el, status_el);
    xmpp_stanza_add_child_release(auth_el, status_str_el);
    
    SendCwmpRpcRsp(h, NULL, NULL, auth_el);
}

void ICACHE_FLASH_ATTR
dlmpd_send(void * h, xmpp_stanza_t *rsp_el)
{
    char *buffer = NULL;
    
    buffer = iks_string (iks_stack (rsp_el), rsp_el);
    BuildSendAndCloseCwmpSoapResp(h, buffer, strlen(buffer));  
}

void ICACHE_FLASH_ATTR
dlmpd_event_send(xmpp_stanza_t *inform_el)
{
    char *buffer = NULL;
#if 1
    if (get_xmpp_conn_status() == 1) 
    {    
        return;
    }
#endif

    xmpp_stanza_t* propertyset_el = xmpp_stanza_create_within("e:propertyset", inform_el);   
    xmpp_stanza_t* property_el = xmpp_stanza_create_within("e:property", inform_el);
    xmpp_stanza_t* property_name_el = xmpp_stanza_create_within("cwmprpcInform", inform_el);

    xmpp_stanza_set_attribute(propertyset_el, "xmlns:e", "urn:schemas-upnp-org:event-1-0");
    xmpp_stanza_add_child_release(property_name_el, inform_el);
    xmpp_stanza_add_child_release(property_el, property_name_el);
    xmpp_stanza_add_child_release(propertyset_el, property_el);
    
  
    buffer = iks_string (iks_stack (propertyset_el), propertyset_el);
    upnp_event_var_change_notify(buffer, strlen(buffer));

    xmpp_stanza_release(propertyset_el);
}
#endif

/*------------------------------------------------------------------------------------------
cwmp general utility function
--------------------------------------------------------------------------------------------
*/
int ICACHE_FLASH_ATTR
GetDeviceParameterValue(char* path, char *value, int len)
{
    int ret_code = Invalid_Arguments;
    deviceGeneralParameter_t *deviceGeneralParam;
    deviceParameter_t *deviceParam;  
    int i;
    
    for (deviceGeneralParam = DeviceGeneralParamTable; 
         deviceGeneralParam->paramName;
         deviceGeneralParam++) {        
        if (strcmp(path, deviceGeneralParam->paramName) == 0)
        {
            if(deviceGeneralParam->getParameterFunc)
            {
                ret_code = deviceGeneralParam->getParameterFunc(path, value, len);
            }
            else
                ret_code = Request_Denied;
            return ret_code;
        }
    }

    for (i = 0, deviceParam = DeviceCustomParamTable.table; 
         i < DeviceCustomParamTable.tableSize;
         i++, deviceParam++) {        
        if (strcmp(path, deviceParam->paramName) == 0)
        {
            if(deviceParam->getParameterFunc)
                ret_code = deviceParam->getParameterFunc(path, value, len);
            else
                ret_code = Request_Denied;
            break;
        }
    }
    
    return ret_code;
}

int ICACHE_FLASH_ATTR
SetDeviceParameterValue(char* path, char *value)
{
    int ret_code;
    deviceGeneralParameter_t *deviceGeneralParam;
    deviceParameter_t *deviceParam;  
    int i;
    
    for (deviceGeneralParam = DeviceGeneralParamTable; 
         deviceGeneralParam->paramName;
         deviceGeneralParam++) {        
        if (strcmp(path, deviceGeneralParam->paramName) == 0)
        {
            if(deviceGeneralParam->setParameterFunc)
                ret_code = deviceGeneralParam->setParameterFunc(path, value);
            else
                ret_code = Request_Denied;
            return ret_code;
        }
    }    

    for (i = 0, deviceParam = DeviceCustomParamTable.table; 
         i < DeviceCustomParamTable.tableSize;
         i++, deviceParam++) {        
        if (strcmp(path, deviceParam->paramName) == 0)
        {
            if(deviceParam->setParameterFunc)
                ret_code = deviceParam->setParameterFunc(path, value);
            else
                ret_code = Request_Denied;
            break;
        }
    }
         
    return ret_code;
}

xmpp_stanza_t* ICACHE_FLASH_ATTR
GetDeviceInfo(xmpp_stanza_t* inform_el)
{
    char** entry_list;
    unsigned int entry_num, i;
    char paramName[128];

    parameterValueList_s paramList;
    parameterValueStruct_s *paramValueStruct;
    log_debug("test\n");

    memset(&paramList, 0, sizeof(parameterValueList_s));
    //Get device info param list
#if 0
    if ((rc = qmibtree_remote_param_names(qdbus_, 
                                          "Device.Inform.ParamList.", 
                                          2, 
                                          &qmibtree_rc,
                                          &entry_list, 
                                          &entry_num)) == QDBUS_RC_OK) 
    {
        paramList.size = entry_num;
        paramList.parameterValueStructs = calloc(1, sizeof(parameterValueStruct_s) * paramList.size);
        for (i = 0; i < entry_num; i++)  
        {
            paramValueStruct = &(paramList.parameterValueStructs[i]);
            qmibtree_remote_param_read_v2_valstr(qdbus_, 
                     entry_list[i], 
                     "Name",
                     paramName,
                     &qmibtree_rc);
            //printf("param name:%s\n", paramName);
            
            char *value;
            qmibtree_remote_param_read(qdbus_, 
                                     paramName, 
                                     &qmibtree_rc, 
                                     &value);
            if (qmibtree_rc_ok((int)qmibtree_rc)) 
            {
                paramValueStruct->Name = strdup(paramName); 
                paramValueStruct->Value = strdup(value);                 
            }
            
        }
        qmibtree_remote_param_names_free(entry_list);  
    }
#endif
    //Build inform body --DeviceInfo    
    xmpp_stanza_t* devinfo_el = xmpp_stanza_create_within(QN_DeviceInfo, inform_el);
    xmpp_stanza_t* devinfo_pl_el = xmpp_stanza_create_within(QN_ParameterList, inform_el);    
    log_debug("test\n");

    for(i = 0; i < paramList.size; i++) 
    {        
        paramValueStruct = &(paramList.parameterValueStructs[i]);
        
        xmpp_stanza_t* pvs_el = xmpp_stanza_create_within(QN_ParameterValueStruct, inform_el);
        xmpp_stanza_t* pvs_name_el = xmpp_stanza_create_within(QN_ParameterValueStruct_Name, inform_el);
        xmpp_stanza_t* pvs_value_el = xmpp_stanza_create_within(QN_ParameterValueStruct_Value, inform_el);

        xmpp_stanza_set_contents(pvs_name_el, paramValueStruct->Name);
        xmpp_stanza_set_contents(pvs_value_el, paramValueStruct->Value);
        
        xmpp_stanza_add_child_release(pvs_el, pvs_name_el);
        xmpp_stanza_add_child_release(pvs_el, pvs_value_el);
            
        xmpp_stanza_add_child_release(devinfo_pl_el, pvs_el);
        free(paramValueStruct->Name);
        free(paramValueStruct->Value);
    }
    log_debug("test\n");

    xmpp_stanza_add_child_release(devinfo_el, devinfo_pl_el);

    if (paramList.parameterValueStructs)
        free(paramList.parameterValueStructs);
    log_debug("test\n");

    return devinfo_el;
}

const char* ICACHE_FLASH_ATTR
fault_get_codestring(int fault_code)
{
    switch (fault_code) {
    case 0:
        return "Success";
    case 9000: /* Method not supported */
        return "Method not supported";
    case 9001: /* Request denied (no reason specified) */
        return "Request denied";
    case 9002: /* Internal error */
        return "Internal error";
    case 9003: /* Invalid arguments */
        return "Invalid arguments";
    case 9004: /* Resources exceeded */
        return "Resources exceeded";       
    case 9005: /* Invalid parameter name */
        return "Invalid parameter name";
    case 9006: /* Invalid parameter type */
        return "Invalid parameter type";
    case 9007: /* Invalid parameter value */
        return "Invalid parameter value";
    case 9008: /* Attempt to set a non-writable parameter */
        return "Attempt to set a non-writable parameter";
    case 9010: /* Download failure */
        return "Download failure";
    case 9011: /* Upload failure */
	    return "Upload failure";
    case 9012: /* File transfer server authentication failure */
	    return "File transfer server authentication failure";
    case Upgrade_Failure: /* Upgrade firmware fail */
	    return "Upgrade firmware failed";
    case Downloading_Firmware: 
	    return "Downloading Firmware";  
    case Upgrading_Firmware: 
	    return "Upgrading Firmware";   
    default:
        break;
    }
    return "Internal error";
}

/*------------------------------------------------------------------------------------------
Inform function
--------------------------------------------------------------------------------------------
*/

void ICACHE_FLASH_ATTR
SendCwmpInform(xmpp_stanza_t * const inform_el) 
{
    // Make sure we are actually connected.
    #if 1
    if (get_xmpp_conn_status() == 0) 
    {    
        xmpp_stanza_release(inform_el);  
        return;
    }
    #endif
    
    xmpp_stanza_t* iq_stanza;
    xmpp_stanza_t* query_el;
    char *to;
    log_debug("test\n");
    
    iq_stanza = xmpp_stanza_create_within(STANZA_NAME_IQ, inform_el);
    xmpp_stanza_set_type(iq_stanza, STANZA_TYPE_GET);
    xmpp_stanza_set_id(iq_stanza, create_unique_id(NULL));
    log_debug("test\n");

    to = xmpp_jid_bare(xmpp_conn_get_bound_jid());
    xmpp_stanza_set_attribute(iq_stanza, STANZA_ATTR_TO, to);
    free(to);

    log_debug("test\n");

    query_el = xmpp_stanza_create_within(STANZA_NAME_QUERY, inform_el);
    xmpp_stanza_set_ns(query_el, STANZA_NS_CWMPRPC);
    
    xmpp_stanza_add_child_release(query_el, inform_el);
    xmpp_stanza_add_child_release(iq_stanza, query_el);
    log_debug("test\n");

    xmpp_send(iq_stanza);

    xmpp_stanza_release(iq_stanza);  
}

void ICACHE_FLASH_ATTR
CwmpInformHistoryData(void)
{
    xmpp_stanza_t* inform_el;
    xmpp_stanza_t* devinfo_el;
    xmpp_stanza_t* reason_el;
    int count = 0;
    
    inform_el = xmpp_stanza_create(QN_CWMP_INFORM);
    xmpp_stanza_set_ns(inform_el, STANZA_NS_CWMP);

    devinfo_el = GetDeviceInfo(inform_el);
    log_debug("test\n");

    reason_el = xmpp_stanza_create_within(QN_Reason, inform_el);
    xmpp_stanza_set_contents(reason_el, "Data");
    log_debug("test\n");

    //Build inform body --Data    
    xmpp_stanza_t* data_el = xmpp_stanza_create_within(QN_Data, inform_el);
    xmpp_stanza_t* data_pl_el = xmpp_stanza_create_within(QN_ParameterList, inform_el);    

    historyDataInform_s *m_it;
    historyData_s *v_it;
    log_debug("test\n");
    
    for (m_it = m_historyDataStructs; m_it; m_it = m_it->next) 
    {
        count++;
    
        log_debug("test\n");
        char dataList[512];
        int len = 0;
        xmpp_stanza_t* pvs_el = xmpp_stanza_create_within(QN_ParameterValueStruct, inform_el);
        xmpp_stanza_t* pvs_name_el = xmpp_stanza_create_within(QN_ParameterValueStruct_Name, inform_el);
        xmpp_stanza_t* pvs_value_el = xmpp_stanza_create_within(QN_ParameterValueStruct_Value, inform_el);

        log_debug("test\n");

        xmpp_stanza_set_contents(pvs_name_el, m_it->name);

        memset(dataList, sizeof(dataList), 0);
        len = sprintf(dataList, "{[");
        if (m_it->historyDatas != NULL)
        {
            
        log_debug("test\n");
            len += sprintf(dataList+len, "{'time':%d,'value':%s}", m_it->historyDatas->sampleTime, m_it->historyDatas->value);

            for(v_it = m_it->historyDatas->next; v_it; v_it = v_it->next)   
            {        
                len += sprintf(dataList+len, ",{'time':%d,'value':%s}", 
                v_it->sampleTime, v_it->value);
                
                log_debug("test\n");
            }
        } 
        len += sprintf(dataList+len, "]}");
        
        log_debug("dataList:%s\n", dataList);
        xmpp_stanza_set_contents(pvs_value_el, dataList);
        log_debug("test\n");

        xmpp_stanza_add_child_release(pvs_el, pvs_name_el);
        xmpp_stanza_add_child_release(pvs_el, pvs_value_el);
        log_debug("test\n");
        
        xmpp_stanza_add_child_release(data_pl_el, pvs_el);
    }

        
    xmpp_stanza_add_child_release(data_el, data_pl_el);
    log_debug("test\n");


    //build inform
    xmpp_stanza_add_child_release(inform_el, devinfo_el);
    xmpp_stanza_add_child_release(inform_el, reason_el);
    xmpp_stanza_add_child_release(inform_el, data_el);

    if(count > 0)
    {
        //Send to server
        SendCwmpInform(inform_el);
    }
    else
    {
        xmpp_stanza_release(inform_el);  
    }
    
    //clear history data
    for(m_it = m_historyDataStructs; 
        m_historyDataStructs; 
        m_it = m_historyDataStructs) 
    {
    
    log_debug("test\n");
        m_historyDataStructs = m_historyDataStructs->next;
        for(v_it = m_it->historyDatas; 
            m_it->historyDatas; 
            v_it = m_it->historyDatas)       
        {
        
        log_debug("test\n");
            m_it->historyDatas = m_it->historyDatas->next;
            free(v_it->value);
            free(v_it);       
        } 
        
        log_debug("test\n");
        free(m_it->name);
        free(m_it);
    }
}

void ICACHE_FLASH_ATTR
CwmpInformConfigChanged(parameterValueList_s *paramValueList)
{
    int i;
    struct cwmp_parameterValueStruct_t *valueChangeParam;
    
    xmpp_stanza_t* inform_el;
    xmpp_stanza_t* devinfo_el;
    xmpp_stanza_t* reason_el;

    inform_el = xmpp_stanza_create(QN_CWMP_INFORM); 
    xmpp_stanza_set_ns(inform_el, STANZA_NS_CWMP);

    devinfo_el = GetDeviceInfo(inform_el);
    log_debug("test\n");

    reason_el = xmpp_stanza_create_within(QN_Reason, inform_el);
    xmpp_stanza_set_contents(reason_el, "ConfigChanged");
    
    log_debug("test\n");
    //Build inform body --ConfigChanged      
    xmpp_stanza_t* conf_chg_el = xmpp_stanza_create_within(QN_ConfigChanged, inform_el);       
    xmpp_stanza_t* pl_el = xmpp_stanza_create_within(QN_ParameterList, inform_el);    
    
    for(i = 0; i < paramValueList->size; i++) {
        valueChangeParam = &(paramValueList->parameterValueStructs[i]);
        if(valueChangeParam->FaultCode == Success)
        {
            char* value;
            xmpp_stanza_t* pvs_el = xmpp_stanza_create_within(QN_ParameterValueStruct, inform_el);
            xmpp_stanza_t* pvs_name_el = xmpp_stanza_create_within(QN_ParameterValueStruct_Name, inform_el);
            xmpp_stanza_t* pvs_value_el = xmpp_stanza_create_within(QN_ParameterValueStruct_Value, inform_el);
            log_debug("test\n");
            xmpp_stanza_set_contents(pvs_name_el, valueChangeParam->Name);
            xmpp_stanza_set_contents(pvs_value_el, valueChangeParam->Value);
            xmpp_stanza_add_child_release(pvs_el, pvs_name_el);
            xmpp_stanza_add_child_release(pvs_el, pvs_value_el);
            
            xmpp_stanza_add_child_release(pl_el, pvs_el);
        }
    }

    xmpp_stanza_add_child_release(conf_chg_el, pl_el);

    //build inform
    xmpp_stanza_add_child_release(inform_el, devinfo_el);
    xmpp_stanza_add_child_release(inform_el, reason_el);
    xmpp_stanza_add_child_release(inform_el, conf_chg_el);
    log_debug("test\n");

    SendCwmpInform(inform_el);    
#ifdef DLMPD_SUPPORT
    dlmpd_event_send(inform_el);
#endif
}

void ICACHE_FLASH_ATTR
CwmpInformUpdateData(parameterValueList_s *paramValueList)
{
    int i;
    struct cwmp_parameterValueStruct_t *valueChangeParam;
    
    xmpp_stanza_t* inform_el;
    xmpp_stanza_t* devinfo_el;
    xmpp_stanza_t* reason_el;

    inform_el = xmpp_stanza_create(QN_CWMP_INFORM); 
    xmpp_stanza_set_ns(inform_el, STANZA_NS_CWMP);

    devinfo_el = GetDeviceInfo(inform_el);
    log_debug("test\n");

    reason_el = xmpp_stanza_create_within(QN_Reason, inform_el);
    xmpp_stanza_set_contents(reason_el, "UpdateData");
    
    log_debug("test\n");
    //Build inform body --ConfigChanged      
    xmpp_stanza_t* conf_chg_el = xmpp_stanza_create_within(QN_Data, inform_el);       
    xmpp_stanza_t* pl_el = xmpp_stanza_create_within(QN_ParameterList, inform_el);    
    
    for(i = 0; i < paramValueList->size; i++) {
        char* value;
        xmpp_stanza_t* pvs_el = xmpp_stanza_create_within(QN_ParameterValueStruct, inform_el);
        xmpp_stanza_t* pvs_name_el = xmpp_stanza_create_within(QN_ParameterValueStruct_Name, inform_el);
        xmpp_stanza_t* pvs_value_el = xmpp_stanza_create_within(QN_ParameterValueStruct_Value, inform_el);
        valueChangeParam = &(paramValueList->parameterValueStructs[i]);
        log_debug("test\n");
        xmpp_stanza_set_contents(pvs_name_el, valueChangeParam->Name);
        xmpp_stanza_set_contents(pvs_value_el, valueChangeParam->Value);
        xmpp_stanza_add_child_release(pvs_el, pvs_name_el);
        xmpp_stanza_add_child_release(pvs_el, pvs_value_el);
        
        xmpp_stanza_add_child_release(pl_el, pvs_el);
    }

    xmpp_stanza_add_child_release(conf_chg_el, pl_el);

    //build inform
    xmpp_stanza_add_child_release(inform_el, devinfo_el);
    xmpp_stanza_add_child_release(inform_el, reason_el);
    xmpp_stanza_add_child_release(inform_el, conf_chg_el);
    log_debug("test\n");

    SendCwmpInform(inform_el);    
}

void ICACHE_FLASH_ATTR
CwmpInformUpdateData2(void)
{
    deviceParameter_t *updateParam;
    int ret_code;
    char value[MAX_PARAM_VALUE_LEN];
    int i;
    int count = 0;

    
    xmpp_stanza_t* inform_el = xmpp_stanza_create(QN_CWMP_INFORM);    
    xmpp_stanza_set_ns(inform_el, STANZA_NS_CWMP);
    
    xmpp_stanza_t* devinfo_el = GetDeviceInfo(inform_el);
    xmpp_stanza_t* reason_el = xmpp_stanza_create_within(QN_Reason, inform_el);

    xmpp_stanza_set_contents(reason_el, "UpdateData");
    
    //Build inform body --UpdateData     
    xmpp_stanza_t* data_el = xmpp_stanza_create_within(QN_Data, inform_el);       
    xmpp_stanza_t* pl_el = xmpp_stanza_create_within(QN_ParameterList, inform_el);    

    
    for (i = 0, updateParam = DeviceCustomParamTable.table;
         i < DeviceCustomParamTable.tableSize;
         i++, updateParam++) {
            if(!(updateParam->flag & HNT_XMPP_PATH_FLAG_UPDATE))
                continue;    
            count++;
            memset(value, 0, sizeof(value));
            ret_code = GetDeviceParameterValue(updateParam->paramName, value, MAX_PARAM_VALUE_LEN);
            
	            if (ret_code == Success) {
	                xmpp_stanza_t* pvs_el = xmpp_stanza_create_within(QN_ParameterValueStruct, inform_el);
	                xmpp_stanza_t* pvs_name_el = xmpp_stanza_create_within(QN_ParameterValueStruct_Name, inform_el);
	                xmpp_stanza_t* pvs_value_el = xmpp_stanza_create_within(QN_ParameterValueStruct_Value, inform_el);
                    xmpp_stanza_set_contents(pvs_name_el, updateParam->paramName);
                    xmpp_stanza_set_contents(pvs_value_el, value);

                    xmpp_stanza_add_child_release(pvs_el, pvs_name_el);
                    xmpp_stanza_add_child_release(pvs_el, pvs_value_el);

                    xmpp_stanza_add_child_release(pl_el, pvs_el);
	            }
            }            
    xmpp_stanza_add_child_release(data_el, pl_el);

    //build inform
    xmpp_stanza_add_child_release(inform_el, devinfo_el);
    xmpp_stanza_add_child_release(inform_el, reason_el);
    xmpp_stanza_add_child_release(inform_el, data_el);

    if(count > 0)
    {
        SendCwmpInform(inform_el);
    }
    else
    {
        xmpp_stanza_release(inform_el);  
    }
}

void CwmpInformRealDataChanged(parameterValueList_s *paramValueList)
{
    int i;
    struct cwmp_parameterValueStruct_t *realDataChangeParam;
    
    xmpp_stanza_t* inform_el;
    xmpp_stanza_t* devinfo_el;
    xmpp_stanza_t* reason_el;

    inform_el = xmpp_stanza_create(QN_CWMP_INFORM); 
    xmpp_stanza_set_ns(inform_el, STANZA_NS_CWMP);

    devinfo_el = GetDeviceInfo(inform_el);
    log_debug("test\n");

    reason_el = xmpp_stanza_create_within(QN_Reason, inform_el);
    xmpp_stanza_set_contents(reason_el, "RealDataChanged");
    
    log_debug("test\n");
    //Build inform body --RealDataChanged      
    xmpp_stanza_t* realdata_changed_el = xmpp_stanza_create_within(QN_Data, inform_el);       
    
    for(i = 0; i < paramValueList->size; i++) {
        realDataChangeParam = &(paramValueList->parameterValueStructs[i]);
        xmpp_stanza_t* pvs_path_el = xmpp_stanza_create_within(QN_DataPath, inform_el);
        xmpp_stanza_set_attribute(pvs_path_el, QN_DataPath_Name, realDataChangeParam->Name);
        xmpp_stanza_set_contents(pvs_path_el, realDataChangeParam->Value);
        
        xmpp_stanza_add_child_release(realdata_changed_el, pvs_path_el);
    }

    //build inform
    xmpp_stanza_add_child_release(inform_el, devinfo_el);
    xmpp_stanza_add_child_release(inform_el, reason_el);
    xmpp_stanza_add_child_release(inform_el, realdata_changed_el);
    log_debug("test\n");

    SendCwmpInform(inform_el);    
}

void ICACHE_FLASH_ATTR
SendCwmpInformRealDataChanged(xmpp_stanza_t * const data_el) 
{
    xmpp_stanza_t* inform_el;
    xmpp_stanza_t* devinfo_el;
    xmpp_stanza_t* reason_el;

    inform_el = xmpp_stanza_create_within(QN_CWMP_INFORM, data_el); 
    xmpp_stanza_set_ns(inform_el, STANZA_NS_CWMP);

    devinfo_el = GetDeviceInfo(inform_el);
    log_debug("test\n");

    reason_el = xmpp_stanza_create_within(QN_Reason, inform_el);
    xmpp_stanza_set_contents(reason_el, "RealDataChanged");

    //build inform
    xmpp_stanza_add_child_release(inform_el, devinfo_el);
    xmpp_stanza_add_child_release(inform_el, reason_el);
    xmpp_stanza_add_child_release(inform_el, data_el);
    log_debug("test\n");

    SendCwmpInform(inform_el);    
}

void CwmpInformAllRealDataChanged(void)
{
    int i;
    char *value = (char *)zalloc(MAX_PARAM_VALUE_LEN);
    int ret_code;
    int len = 0;
    int total_len = 0;
    deviceGeneralParameter_t *deviceGeneralParam;
    deviceParameter_t *deviceParam;  
    struct cwmp_parameterValueStruct_t *valueChangeParam;
        
    log_debug("test\n");
    //Build inform body --RealDataChanged      
    xmpp_stanza_t* realdata_changed_el = xmpp_stanza_create(QN_Data);       

    for (deviceGeneralParam = DeviceGeneralParamTable; 
         deviceGeneralParam->paramName;
         deviceGeneralParam++) {   
            memset(value, 0, MAX_PARAM_VALUE_LEN);
            ret_code = GetDeviceParameterValue(deviceGeneralParam->paramName, value, MAX_PARAM_VALUE_LEN);
            
	            if (ret_code == Success) {
	                xmpp_stanza_t* pvs_path_el = xmpp_stanza_create_within(QN_DataPath, realdata_changed_el);
                    xmpp_stanza_set_attribute(pvs_path_el, QN_DataPath_Name, deviceGeneralParam->paramName);
                    xmpp_stanza_set_contents(pvs_path_el, value);

                    xmpp_stanza_add_child_release(realdata_changed_el, pvs_path_el);
	            }
            }
    log_debug("system_get_free_heap_size = %d\n", system_get_free_heap_size()); 
    SendCwmpInformRealDataChanged(realdata_changed_el);

    realdata_changed_el = xmpp_stanza_create(QN_Data);
    for (i = 0, deviceParam = DeviceCustomParamTable.table; 
         i < DeviceCustomParamTable.tableSize;
         i++, deviceParam++) {        
            memset(value, 0, MAX_PARAM_VALUE_LEN);
            ret_code = GetDeviceParameterValue(deviceParam->paramName, value, MAX_PARAM_VALUE_LEN);
            len = os_strlen(value);
            if(total_len + len > 400)
            {
                SendCwmpInformRealDataChanged(realdata_changed_el);
                realdata_changed_el = xmpp_stanza_create(QN_Data);   
                total_len = len;
            }
            else
            {
                total_len += len;
            }
            log_debug("system_get_free_heap_size = %d\n", system_get_free_heap_size()); 
            
	            if (ret_code == Success) {
	                xmpp_stanza_t* pvs_path_el = xmpp_stanza_create_within(QN_DataPath, realdata_changed_el);
                    log_debug("system_get_free_heap_size = %d\n", system_get_free_heap_size()); 
                    xmpp_stanza_set_attribute(pvs_path_el, QN_DataPath_Name, deviceParam->paramName);
                    xmpp_stanza_set_contents(pvs_path_el, value);

                    xmpp_stanza_add_child_release(realdata_changed_el, pvs_path_el);
	            }
            }
    log_debug("system_get_free_heap_size = %d\n", system_get_free_heap_size()); 
    SendCwmpInformRealDataChanged(realdata_changed_el);
    
    free(value);  
}

void ICACHE_FLASH_ATTR
CwmpInformDeviceInfo(void)
{
    char value[MAX_PARAM_VALUE_LEN];  
    int ret_code;
    xmpp_stanza_t* inform_el = xmpp_stanza_create(QN_CWMP_INFORM);    
    xmpp_stanza_set_ns(inform_el, STANZA_NS_CWMP);
    //Build inform body --DeviceInfo    
    xmpp_stanza_t* devinfo_el = xmpp_stanza_create_within(QN_DeviceInfo, inform_el);
    xmpp_stanza_t* reason_el = xmpp_stanza_create_within(QN_Reason, inform_el);
    //Build inform body --Data     
    xmpp_stanza_t* data_el = xmpp_stanza_create_within(QN_Data, inform_el);       
    
    xmpp_stanza_set_contents(reason_el, "DeviceInfo");

    xmpp_stanza_t* type_el = xmpp_stanza_create_within(QN_DEVICEINFO_TYPE, inform_el);
    xmpp_stanza_t* version_el = xmpp_stanza_create_within(QN_DEVICEINFO_VERSION, inform_el);

    memset(value, 0, sizeof(value));
    getDeviceTypeFunc(NULL, value, MAX_PARAM_VALUE_LEN);
    xmpp_stanza_set_contents(type_el, value);

    memset(value, 0, sizeof(value));
    getSoftwareVersionFunc(NULL, value, MAX_PARAM_VALUE_LEN);
    xmpp_stanza_set_contents(version_el, value);

    xmpp_stanza_add_child_release(devinfo_el, type_el);
    xmpp_stanza_add_child_release(devinfo_el, version_el);

    memset(value, 0, sizeof(value));    
    ret_code = GetDeviceParameterValue("Device.DeviceInfo.SlaveDevList", value, MAX_PARAM_VALUE_LEN);
    if(ret_code == 0)
    {
        xmpp_stanza_t* slaveDevList_el = xmpp_stanza_create_within(QN_DEVICEINFO_SLAVEDEVLIST, inform_el);
        xmpp_stanza_set_contents(slaveDevList_el, value);
        xmpp_stanza_add_child_release(devinfo_el, slaveDevList_el);
    }
   
    //build inform
    xmpp_stanza_add_child_release(inform_el, devinfo_el);
    xmpp_stanza_add_child_release(inform_el, reason_el);
    xmpp_stanza_add_child_release(inform_el, data_el);

    SendCwmpInform(inform_el);
}

void ICACHE_FLASH_ATTR
CwmpInformAlarm(char *alarmInfo, int level)
{
    char time_now[32] = {0};
    
    xmpp_stanza_t* inform_el;
    xmpp_stanza_t* devinfo_el;
    xmpp_stanza_t* reason_el;

    inform_el = xmpp_stanza_create(QN_CWMP_INFORM); 
    xmpp_stanza_set_ns(inform_el, STANZA_NS_CWMP);

    devinfo_el = GetDeviceInfo(inform_el);

    reason_el = xmpp_stanza_create_within(QN_Reason, inform_el);
    xmpp_stanza_set_contents(reason_el, "Alarm");

    //Build inform body --Alarm      
    xmpp_stanza_t* alarm_el = xmpp_stanza_create_within(QN_Alarm, inform_el);
    xmpp_stanza_t* level_el = xmpp_stanza_create_within(QN_Level, inform_el);
    xmpp_stanza_t* time_el = xmpp_stanza_create_within(QN_Time, inform_el);
    xmpp_stanza_t* text_el = xmpp_stanza_create_within(QN_Text, inform_el);
        
    sprintf(time_now, "%d", get_current_time());
    xmpp_stanza_set_contents(time_el, time_now);

    memset(time_now, 0, sizeof(time_now));
    sprintf(time_now, "%d", level);
    xmpp_stanza_set_contents(level_el, time_now);
    xmpp_stanza_set_contents(text_el, alarmInfo);

    xmpp_stanza_add_child_release(alarm_el, level_el);
    xmpp_stanza_add_child_release(alarm_el, time_el);
    xmpp_stanza_add_child_release(alarm_el, text_el);    

    //build inform
    xmpp_stanza_add_child_release(inform_el, devinfo_el);    
    xmpp_stanza_add_child_release(inform_el, reason_el);    
    xmpp_stanza_add_child_release(inform_el, alarm_el);    
    //printf("%s", inform_el->Str().c_str());
    
    SendCwmpInform(inform_el);
#ifdef DLMPD_SUPPORT
    dlmpd_event_send(inform_el);
#endif
}

void ICACHE_FLASH_ATTR
CwmpSamplingData()
{
    deviceParameter_t *historyParam;
    int ret_code;
    char value[MAX_PARAM_VALUE_LEN];
    int i;
    
    for (i = 0, historyParam = DeviceCustomParamTable.table;
         i < DeviceCustomParamTable.tableSize;
         i++, historyParam++) {
            if(!(historyParam->flag & HNT_XMPP_PATH_FLAG_HISTORY))
                continue;     
            log_debug("param name:%s\n", historyParam->paramName);
            historyDataInform_s *m_it;
            historyDataInform_s *m_pre;
            
            log_debug("test\n");
            for (m_it = m_pre = m_historyDataStructs; m_it; m_it = m_it->next) 
            {
            
            log_debug("test\n");
                if (strcmp(m_it->name, historyParam->paramName) == 0) 
                {
                    break;
                }
                m_pre = m_it;
            }

            if (m_it == NULL)
            {
                log_debug("test\n");
                m_it = (historyDataInform_s *)calloc(1, sizeof(historyDataInform_s));
                memset(m_it, 0, sizeof(historyDataInform_s));
                m_it->name = strdup(historyParam->paramName);
                if (m_pre != NULL)
                {
                    m_pre->next = m_it;
                }
                else
                {
                    m_historyDataStructs = m_it;
                }
            }
            
            log_debug("test\n");  
            memset(value, 0, sizeof(value));
            ret_code = GetDeviceParameterValue(historyParam->paramName, value, MAX_PARAM_VALUE_LEN);
            if (ret_code == Success) {
                historyData_s *historyData = (historyData_s *)calloc(1, sizeof(historyData_s));
                memset(historyData, 0, sizeof(historyData_s));
                historyData->sampleTime = get_current_time();
                historyData->value = strdup(value);
                
                log_debug("test\n");
                if (m_it->historyDatas == NULL)
                {
                    log_debug("test\n");
                    m_it->historyDatas = historyData;
                }
                else
                {
                
                    log_debug("test\n");
                    historyData->next = m_it->historyDatas;
                    m_it->historyDatas = historyData;
                }
                
                log_debug("test\n");
            }     
        }
}

void ICACHE_FLASH_ATTR
CwmpCheckAlarm(void)
{
    deviceParameter_t *alarmParam;
    int ret_code;
    char value[MAX_PARAM_VALUE_LEN];
    int i;

    log_debug("test\n");
    for (i = 0, alarmParam = DeviceCustomParamTable.table;
         i < DeviceCustomParamTable.tableSize;
         i++, alarmParam++) {  
            if(!(alarmParam->flag & HNT_XMPP_PATH_FLAG_ALARM))
                continue;     
                
            int f_value;

            memset(value, 0, sizeof(value));
            ret_code = GetDeviceParameterValue(alarmParam->paramName, value, MAX_PARAM_VALUE_LEN);
            
            if (ret_code != Success) continue;

            if (strcmp(value, "true") == 0)
                 f_value = 1;
            else if (strcmp(value, "false") == 0)
                 f_value = 0;
            else
                 f_value = atoi(value);

            if(alarmParam->alarmDesc == NULL)
                continue;
                
            if(f_value > alarmParam->alarmDesc->LimitMax || f_value < alarmParam->alarmDesc->LimitMin) {
                log_debug("test\n");
                CwmpInformAlarm(alarmParam->alarmDesc->Description, ALARM_LEVEL_HIGH);
            }
            log_debug("test\n");
      }    
}

void ICACHE_FLASH_ATTR
CwmpCheckOneAlarm(char *path_name, char *para_value)
{
    deviceParameter_t *alarmParam;
    int i;

    log_debug("test\n");
    for (i = 0, alarmParam = DeviceCustomParamTable.table;
         i < DeviceCustomParamTable.tableSize;
         i++, alarmParam++) {     
            if(!(alarmParam->flag & HNT_XMPP_PATH_FLAG_ALARM))
                 continue;
                 
            int f_value;

            if (strcmp(alarmParam->paramName, path_name) != 0)
                continue;

            if (strcmp(para_value, "true") == 0)
                 f_value = 1;
            else if (strcmp(para_value, "false") == 0)
                 f_value = 0;
            else
                 f_value = atoi(para_value);

            if(alarmParam->alarmDesc == NULL)
                continue;
                
            if(f_value > alarmParam->alarmDesc->LimitMax || f_value < alarmParam->alarmDesc->LimitMin) {
                log_debug("test\n");
                CwmpInformAlarm(alarmParam->alarmDesc->Description, ALARM_LEVEL_HIGH);
            }
            log_debug("test\n");
      }    
}

void ICACHE_FLASH_ATTR
start_cwmp_inform(void)
{
    if(inform_data_en_) {
        CwmpInformDeviceInfo(); 
        os_timer_disarm(&xmpp_rpc_timer);
        os_timer_setfn(&xmpp_rpc_timer, (os_timer_func_t *)CwmpInformUpdateData2, NULL);
        os_timer_arm(&xmpp_rpc_timer, 1000, 0);
    }
}

int ICACHE_FLASH_ATTR
cwmp_inform_handler(void * parg)
{  
    log_debug("test\n");

    if(inform_data_en_) 
    {        
        log_debug("system_get_free_heap_size = %d\n", system_get_free_heap_size()); 
        CwmpInformHistoryData();
        
        log_debug("system_get_free_heap_size = %d\n", system_get_free_heap_size()); 
        CwmpInformUpdateData2();
        
        log_debug("system_get_free_heap_size = %d\n", system_get_free_heap_size()); 
    }

    return 1;
}

int ICACHE_FLASH_ATTR
cwmp_sampling_handler(void * parg)
{    
    log_debug("test\n");
    if(inform_data_en_) 
    {
        CwmpSamplingData();       
    }
    
    if(inform_alarm_en_) 
    {
        log_debug("test\n");
        CwmpCheckAlarm();
    }

    return 1;
}

/*------------------------------------------------------------------------------------------
Cwmp RPC Method function
--------------------------------------------------------------------------------------------
*/

static void ICACHE_FLASH_ATTR
DoSetParameterValues(parameterValueList_s *paramValueList)
{
    int i;
    int ret_code;
    for(i = 0; i < paramValueList->size; i++)
    {
        log_debug("test\n");
        parameterValueStruct_s *paramValueStruct = paramValueList->parameterValueStructs + i;
        log_debug("name = %s\n", paramValueStruct->Name);
        log_debug("value = %s\n", paramValueStruct->Value);
        ret_code = SetDeviceParameterValue(paramValueStruct->Name, paramValueStruct->Value);
        
        paramValueStruct->FaultCode = ret_code;
    }
}
static void ICACHE_FLASH_ATTR
DoGetParameterValues(parameterValueList_s *paramValueList)
{  
    int i;   
    int ret_code;
    char *value = (char *)zalloc(512);
    for(i = 0; i < paramValueList->size; i++)
    {
        memset(value, 0, 512);
        parameterValueStruct_s *paramValueStruct = paramValueList->parameterValueStructs + i;
        ret_code = GetDeviceParameterValue(paramValueStruct->Name, value, 512);
        paramValueStruct->FaultCode = ret_code;
        if(ret_code == Success)
            paramValueStruct->Value = strdup(value);
        log_debug("Name = %s, Value = %s\n", paramValueStruct->Name, paramValueStruct->Value);
    }
    free(value);
}

void ICACHE_FLASH_ATTR
SendCwmpRpcRsp(void *h, const char *to, const char *iq_id, 
                        xmpp_stanza_t *rsp_el)
{   
    if (get_xmpp_conn_status() == 0) 
    {
        xmpp_stanza_release(rsp_el);      
        return;
    }

    xmpp_stanza_t* iq_stanza;
    xmpp_stanza_t* query_el;  

    iq_stanza = xmpp_stanza_create_within(STANZA_NAME_IQ, rsp_el);
    xmpp_stanza_set_type(iq_stanza, STANZA_TYPE_RESULT);
    xmpp_stanza_set_id(iq_stanza, iq_id);
    xmpp_stanza_set_attribute(iq_stanza, STANZA_ATTR_TO, to);

    query_el = xmpp_stanza_create_within(STANZA_NAME_QUERY, rsp_el);
    xmpp_stanza_set_ns(query_el, STANZA_NS_CWMPRPC);
    
    xmpp_stanza_add_child_release(query_el, rsp_el);
    xmpp_stanza_add_child_release(iq_stanza, query_el);

#ifdef DLMPD_SUPPORT
    if (h != NULL)
        dlmpd_send(h, iq_stanza);
#endif

    xmpp_send(iq_stanza);
    
    xmpp_stanza_release(iq_stanza);      
}

static void ICACHE_FLASH_ATTR
SendXmppLogout(void)
{
    if (get_xmpp_conn_status() == 0) 
    {
        return;
    }

    xmpp_stanza_t *presence;

    /* presence unavailable */
    presence = xmpp_stanza_create("presence");
    if (presence) {   
        char *from;
        from = xmpp_jid_bare(xmpp_conn_get_bound_jid());
        xmpp_stanza_set_attribute(presence, STANZA_ATTR_FROM, from);
        free(from);
        xmpp_stanza_set_type(presence, "unavailable");  
        xmpp_send(presence);
        xmpp_stanza_release(presence);      
    }
}

static void ICACHE_FLASH_ATTR
SendSetParameterValuesResponse(void *h, const char *to, const char *iq_id,
                                            parameterValueList_s *paramValueList)
{
    parameterValueStruct_s *paramValueStruct;
    int i;    
    xmpp_stanza_t* spv_rsp_el;
    xmpp_stanza_t* status_el;
    xmpp_stanza_t* status_str_el;
    int has_fault = 0;
    char s[10];

    spv_rsp_el = xmpp_stanza_create(QN_CWMP_SPV_RSP);
    xmpp_stanza_set_ns(spv_rsp_el, STANZA_NS_CWMP);

    status_el = xmpp_stanza_create_within(QN_Status, spv_rsp_el);

    status_str_el = xmpp_stanza_create_within(QN_StatusString, spv_rsp_el);
    
    for(i = 0; i < paramValueList->size; i++) 
    {
        paramValueStruct = paramValueList->parameterValueStructs + i;
        if(paramValueStruct->FaultCode != Success) 
        {
            has_fault = 1;
        }
    }
        
    if(has_fault) 
    {        
        sprintf(s, "%d", Invalid_Arguments);
        xmpp_stanza_set_contents(status_el, s);
        xmpp_stanza_set_contents(status_str_el, fault_get_codestring(Invalid_Arguments)); 
    }
    else 
    {
        xmpp_stanza_set_contents(status_el, "0");
        xmpp_stanza_set_contents(status_str_el, fault_get_codestring(Success)); 
    }


    xmpp_stanza_add_child_release(spv_rsp_el, status_el);
    xmpp_stanza_add_child_release(spv_rsp_el, status_str_el);

    if(has_fault) 
    {        
        xmpp_stanza_t* fault_list_el = xmpp_stanza_create_within(QN_FaultList, spv_rsp_el);

        for(i = 0; i < paramValueList->size; i++) 
        {
            paramValueStruct = paramValueList->parameterValueStructs + i;
            if(paramValueStruct->FaultCode != Success) 
            {
                xmpp_stanza_t* fault_el = xmpp_stanza_create_within(QN_Fault, spv_rsp_el);
                xmpp_stanza_t* pn_el = xmpp_stanza_create_within(QN_ParameterName, spv_rsp_el);
                xmpp_stanza_t* fault_code_el = xmpp_stanza_create_within(QN_FaultCode, spv_rsp_el);
                xmpp_stanza_t* fault_str_el = xmpp_stanza_create_within(QN_FaultString, spv_rsp_el);

                xmpp_stanza_set_contents(pn_el, paramValueStruct->Name);

                memset(s, 0, sizeof(s));
                sprintf(s, "%d", paramValueStruct->FaultCode);
                
                xmpp_stanza_set_contents(fault_code_el, s);
                xmpp_stanza_set_contents(fault_str_el, (fault_get_codestring(paramValueStruct->FaultCode)));                

                xmpp_stanza_add_child_release(fault_el, pn_el);
                xmpp_stanza_add_child_release(fault_el, fault_code_el);  
                xmpp_stanza_add_child_release(fault_el, fault_str_el);

                xmpp_stanza_add_child_release(fault_list_el, fault_el);
            }
        }
        xmpp_stanza_add_child_release(spv_rsp_el, fault_list_el);
    }

    SendCwmpRpcRsp(h, to, iq_id, spv_rsp_el);
}

static void ICACHE_FLASH_ATTR
SendGetParameterValuesResponse(void *h, const char *to, const char *iq_id,
                                            parameterValueList_s *paramValueList)
{
    parameterValueStruct_s *paramValueStruct;
    int i; 
    xmpp_stanza_t* gpv_rsp_el;
    xmpp_stanza_t* status_el;
    xmpp_stanza_t* status_str_el;
    int has_fault = 0;
    char s[10];

    gpv_rsp_el = xmpp_stanza_create(QN_CWMP_GPV_RSP);
    xmpp_stanza_set_ns(gpv_rsp_el, STANZA_NS_CWMP);

    status_el = xmpp_stanza_create_within(QN_Status, gpv_rsp_el);
    status_str_el = xmpp_stanza_create_within(QN_StatusString, gpv_rsp_el);
    
    for(i = 0; i < paramValueList->size; i++)
    {
        paramValueStruct = paramValueList->parameterValueStructs + i;
        if(paramValueStruct->FaultCode != Success) 
        {
            has_fault = 1;
        }
    }
        
    if(has_fault) 
    {  
        sprintf(s, "%d", Invalid_Parameter_Name);
        xmpp_stanza_set_contents(status_el, s);
        xmpp_stanza_set_contents(status_str_el, fault_get_codestring(Invalid_Parameter_Name)); 
    }
    else 
    {
        xmpp_stanza_set_contents(status_el, "0");
        xmpp_stanza_set_contents(status_str_el, fault_get_codestring(Success)); 
    }    

    xmpp_stanza_add_child_release(gpv_rsp_el, status_el);
    xmpp_stanza_add_child_release(gpv_rsp_el, status_str_el);
    if(has_fault) 
    {        
        xmpp_stanza_t* fault_list_el = xmpp_stanza_create_within(QN_FaultList, gpv_rsp_el);
        
        for(i = 0; i < paramValueList->size; i++)
        {            
            paramValueStruct = paramValueList->parameterValueStructs + i;
            if(paramValueStruct->FaultCode != Success) 
            {
                xmpp_stanza_t* fault_el = xmpp_stanza_create_within(QN_Fault, gpv_rsp_el);
                xmpp_stanza_t* pn_el = xmpp_stanza_create_within(QN_ParameterName, gpv_rsp_el);
                xmpp_stanza_t* fault_code_el = xmpp_stanza_create_within(QN_FaultCode, gpv_rsp_el);
                xmpp_stanza_t* fault_str_el = xmpp_stanza_create_within(QN_FaultString, gpv_rsp_el);

                xmpp_stanza_set_contents(pn_el, paramValueStruct->Name);

                memset(s, 0, sizeof(s));
                sprintf(s, "%d", paramValueStruct->FaultCode);

                xmpp_stanza_set_contents(fault_code_el, s);
                xmpp_stanza_set_contents(fault_str_el, fault_get_codestring(paramValueStruct->FaultCode));


                xmpp_stanza_add_child_release(fault_el, pn_el);
                xmpp_stanza_add_child_release(fault_el, fault_code_el);
                xmpp_stanza_add_child_release(fault_el, fault_str_el);

                xmpp_stanza_add_child_release(fault_list_el, fault_el);
            }
        }

        xmpp_stanza_add_child_release(gpv_rsp_el, fault_list_el);
    }
    else 
    {//success
        xmpp_stanza_t* pl_el = xmpp_stanza_create_within(QN_ParameterList, gpv_rsp_el);
        
        for(i = 0; i < paramValueList->size; i++)
        {
            paramValueStruct = paramValueList->parameterValueStructs + i;
            if(paramValueStruct->FaultCode == Success) 
            {
                xmpp_stanza_t* pvs_el = xmpp_stanza_create_within(QN_ParameterValueStruct, gpv_rsp_el);
                xmpp_stanza_t* pvs_name_el = xmpp_stanza_create_within(QN_ParameterValueStruct_Name, gpv_rsp_el);
                xmpp_stanza_t* pvs_value_el = xmpp_stanza_create_within(QN_ParameterValueStruct_Value, gpv_rsp_el);

                xmpp_stanza_set_contents(pvs_name_el, paramValueStruct->Name);
                xmpp_stanza_set_contents(pvs_value_el, paramValueStruct->Value);

                xmpp_stanza_add_child_release(pvs_el, pvs_name_el);
                xmpp_stanza_add_child_release(pvs_el, pvs_value_el);

                xmpp_stanza_add_child_release(pl_el, pvs_el);
            }
        }

        xmpp_stanza_add_child_release(gpv_rsp_el, pl_el);  
    }

    SendCwmpRpcRsp(h, to, iq_id, gpv_rsp_el);    
}

static void ICACHE_FLASH_ATTR
SendRebootResponse(void *h, const char *to, const char *iq_id)
{
    xmpp_stanza_t* reboot_rsp_el;
    xmpp_stanza_t* status_el;
    xmpp_stanza_t* status_str_el;   

    reboot_rsp_el = xmpp_stanza_create(QN_CWMP_REBOOT_RSP);
    xmpp_stanza_set_ns(reboot_rsp_el, STANZA_NS_CWMP);

    status_el = xmpp_stanza_create_within(QN_Status, reboot_rsp_el);
    xmpp_stanza_set_contents(status_el, "0");

    status_str_el = xmpp_stanza_create_within(QN_StatusString, reboot_rsp_el);
    xmpp_stanza_set_contents(status_str_el, fault_get_codestring(Success));

    xmpp_stanza_add_child_release(reboot_rsp_el, status_el);
    xmpp_stanza_add_child_release(reboot_rsp_el, status_str_el);

    SendCwmpRpcRsp(h, to, iq_id, reboot_rsp_el);
}

void ICACHE_FLASH_ATTR
SendFactoryResetResponse(void *h, const char *to, const char *iq_id)
{
    xmpp_stanza_t* reset_rsp_el;
    xmpp_stanza_t* status_el;
    xmpp_stanza_t* status_str_el;   

    reset_rsp_el = xmpp_stanza_create(QN_CWMP_RESET_RSP);
    xmpp_stanza_set_ns(reset_rsp_el, STANZA_NS_CWMP);

    status_el = xmpp_stanza_create_within(QN_Status, reset_rsp_el);
    xmpp_stanza_set_contents(status_el, "0");

    status_str_el = xmpp_stanza_create_within(QN_StatusString, reset_rsp_el);
    xmpp_stanza_set_contents(status_str_el, fault_get_codestring(Success));

    xmpp_stanza_add_child_release(reset_rsp_el, status_el);
    xmpp_stanza_add_child_release(reset_rsp_el, status_str_el);

    SendCwmpRpcRsp(h, to, iq_id, reset_rsp_el);
}

LOCAL ip_addr_t xmpp_download_ip;
LOCAL os_timer_t xmpp_download_timer;
LOCAL struct espconn xmpp_download_espconn;
struct upgrade_server_info *xmpp_download_server = NULL;

void ICACHE_FLASH_ATTR
system_factory_reset(void)
{
    SendXmppLogout();
    
    os_timer_disarm(&xmpp_rpc_timer);
    os_timer_setfn(&xmpp_rpc_timer, (os_timer_func_t *)hnt_mgmt_system_factory_reset, NULL);
    os_timer_arm(&xmpp_rpc_timer, 1000, 0);
}

static void ICACHE_FLASH_ATTR
SendDownloadResponse(const char *to, const char *iq_id, int status, int progress)
{
    xmpp_stanza_t* download_rsp_el;
    xmpp_stanza_t* status_el;
    xmpp_stanza_t* status_str_el;
    xmpp_stanza_t* progress_el;
    char s[10] = {0};

    download_rsp_el = xmpp_stanza_create(QN_CWMP_DOWNLOAD_RSP);
    xmpp_stanza_set_ns(download_rsp_el, STANZA_NS_CWMP);

    status_el = xmpp_stanza_create_within(QN_Status, download_rsp_el);
    sprintf(s, "%d", status);
    xmpp_stanza_set_contents(status_el, s);
    
    status_str_el = xmpp_stanza_create_within(QN_StatusString, download_rsp_el);
    xmpp_stanza_set_contents(status_str_el, fault_get_codestring(status));

    progress_el = xmpp_stanza_create_within(QN_Progress, download_rsp_el);
    memset(s, 0, sizeof(s));
    sprintf(s, "%d", progress);
    xmpp_stanza_set_contents(progress_el, s);
    
    xmpp_stanza_add_child_release(download_rsp_el, status_el);
    xmpp_stanza_add_child_release(download_rsp_el, status_str_el);
    xmpp_stanza_add_child_release(download_rsp_el, progress_el);
    
    SendCwmpRpcRsp(NULL, to, iq_id, download_rsp_el);
}

/* parseURL()
 * arguments :
 *   url :		source string not modified
 *   hostname :	hostname destination string (size of MAXHOSTNAMELEN+1)
 *   port :		port (destination)
 *   path :		pointer to the path part of the URL
 *
 * Return values :
 *    0 - Failure
 *    1 - Success         */
LOCAL int ICACHE_FLASH_ATTR
xmpp_parseURL(const char * url, int *servertype, char * hostname, unsigned short * port, char * *path)
{
	char * p1, *p2, *p3;
	if(!url)
		return 0;
	p1 = strstr(url, "://");
	if(!p1)
		return 0;
	p1 += 3;

    if(servertype != NULL)
    {
    	if(  (url[0]=='h') && (url[1]=='t')
    	   &&(url[2]=='t') && (url[3]=='p')) 
    	{
            *servertype = SERVERTYPE_HTTP;
    	}
    	else if(  (url[0]=='f') && (url[1]=='t')
    	        &&(url[2]=='p'))
    	{
            *servertype = SERVERTYPE_FTP;
    	}
        else 
        {
    		return 0;
        }
    }
	memset(hostname, 0, MAXHOSTNAMELEN);


	p2 = strchr(p1, ':');
	p3 = strchr(p1, '/');
	if(!p3)
		return 0;
	if(!p2 || (p2>p3))
	{
		strncpy(hostname, p1, MIN(MAXHOSTNAMELEN, (int)(p3-p1)));
		if(*servertype == SERVERTYPE_HTTP)
		    *port = 80;
		else if(*servertype == SERVERTYPE_FTP)
		    *port = 21;
	}
	else
	{
		strncpy(hostname, p1, MIN(MAXHOSTNAMELEN, (int)(p2-p1)));
		*port = 0;
		p2++;
		while( (*p2 >= '0') && (*p2 <= '9'))
		{
			*port *= 10;
			*port += (unsigned short)(*p2 - '0');
			p2++;
		}
	}
	
	if(path != NULL)
	    *path = p3;
	return 1;
}

/******************************************************************************
 * FunctionName : xmpp_download_upgrade_rsp
 * Description  : Processing the downloaded data from the server
 * Parameters   : pespconn -- the espconn used to connetion with the host
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
xmpp_download_upgrade_rsp(void *arg)
{
#if 0
    struct upgrade_server_info *server = arg;
    int status = -1;
    
    if (server->upgrade_status == UPGRADE_STATUS_SUCCESS) {
        log_debug("xmpp_download_upgrade_successfully\n");
        status = Success;
    } else if (server->upgrade_status == UPGRADE_STATUS_UPGRADING){
        log_debug("xmpp_download_upgrade_Upgrading_Firmware\n");
        status = Upgrading_Firmware;        
    } else {
        log_debug("xmpp_download_upgrade_failed\n");
        status = Upgrade_Failure;        
    }

    log_debug("test\n");

    SendDownloadResponse(server->to, server->iq_id, status, server->upgrade_progress);

    log_debug("test\n");

    if (server->upgrade_status == UPGRADE_STATUS_SUCCESS || server->upgrade_status == UPGRADE_STATUS_FAIL) {
        log_debug("test\n");
        if (server->file_type == UPGRADE_FILETYPE_MAIN_IMAGE)
        {
            SendXmppLogout();
            os_timer_disarm(&xmpp_rpc_timer);
            if (server->upgrade_status == UPGRADE_STATUS_SUCCESS)
                os_timer_setfn(&xmpp_rpc_timer, (os_timer_func_t *)system_upgrade_reboot, NULL);
            else
                os_timer_setfn(&xmpp_rpc_timer, (os_timer_func_t *)system_restart, NULL);
                
            os_timer_arm(&xmpp_rpc_timer, 1000, 0);
        }
        else if (server->file_type == UPGRADE_FILETYPE_SLAVEDEV_IMAGE)
        {
            if (server->upgrade_status == UPGRADE_STATUS_SUCCESS)
            {
                if(slave_upgrade_process != NULL)
                    slave_upgrade_process();
            }
        }
        
        free(server->url);
        server->url = NULL;
        free(server->file_md5);
        server->file_md5 = NULL;
        free(server->iq_id);
        server->iq_id = NULL;
        free(server->to);
        server->to = NULL;
        free(server);
        server = NULL;
        xmpp_download_server = NULL;
    }
#endif    
}

#if 0
/******************************************************************************
 * FunctionName : user_esp_platform_dns_found
 * Description  : dns found callback
 * Parameters   : name -- pointer to the name that was looked up.
 *                ipaddr -- pointer to an ip_addr_t containing the IP address of
 *                the hostname, or NULL if the name could not be found (or on any
 *                other error).
 *                callback_arg -- a user-specified callback argument passed to
 *                dns_gethostbyname
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
xmpp_download_dns_found(const char *name, ip_addr_t *ipaddr, void *arg)
{
    struct espconn *pespconn = (struct espconn *)arg;

    if (ipaddr == NULL) {
        log_debug("NULL\n");
        return;
    }

    log_debug(" %d.%d.%d.%d\n",
            *((uint8 *)&ipaddr->addr), *((uint8 *)&ipaddr->addr + 1),
            *((uint8 *)&ipaddr->addr + 2), *((uint8 *)&ipaddr->addr + 3));

    if (xmpp_download_ip.addr == 0 && ipaddr->addr != 0) {
        os_timer_disarm(&xmpp_download_timer);
        xmpp_download_ip.addr = ipaddr->addr;
        xmpp_download_server->ip[0] = *((uint8 *)&ipaddr->addr);
        xmpp_download_server->ip[1] = *((uint8 *)&ipaddr->addr+1);
        xmpp_download_server->ip[2] = *((uint8 *)&ipaddr->addr+2);
        xmpp_download_server->ip[3] = *((uint8 *)&ipaddr->addr+3);
        
#if 0//def UPGRADE_SSL_ENABLE   
        if (system_upgrade_start_ssl(xmpp_download_server) == false) {
#else
    
        if (system_upgrade_start(xmpp_download_server) == false) {
#endif
            log_debug("upgrade is already started\n");
        }  
    }
}

/******************************************************************************
 * FunctionName : user_esp_platform_dns_check_cb
 * Description  : 1s time callback to check dns found
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
xmpp_download_dns_check_cb(void *arg)
{
    char *host = arg;

    log_debug("test\n");

    espconn_gethostbyname(&xmpp_download_espconn, host, &xmpp_download_ip, xmpp_download_dns_found);

    os_timer_arm(&xmpp_download_timer, 1000, 0);
}

LOCAL void ICACHE_FLASH_ATTR
xmpp_download_start_dns(char *host)
{
    ip_addr_t tmp_ip;

    xmpp_download_espconn.proto.tcp = (esp_tcp *)zalloc(sizeof(esp_tcp));
    xmpp_download_espconn.type = ESPCONN_TCP;
    xmpp_download_espconn.state = ESPCONN_NONE;
    system_upgrade_flag_set(UPGRADE_FLAG_IDLE);

    xmpp_download_ip.addr = 0;
    if (0 == espconn_gethostbyname(&xmpp_download_espconn, host, &xmpp_download_ip, xmpp_download_dns_found))
    {
        tmp_ip.addr = xmpp_download_ip.addr;
        xmpp_download_ip.addr = 0;
        xmpp_download_dns_found(host, &xmpp_download_ip, &xmpp_download_espconn);
    }
    else
    {
        os_timer_disarm(&xmpp_download_timer);
        os_timer_setfn(&xmpp_download_timer, (os_timer_func_t *)xmpp_download_dns_check_cb, host);
        os_timer_arm(&xmpp_download_timer, 1000, 0);
    }
}
#endif
static void ICACHE_FLASH_ATTR
CwmpRpcSetParameterValues(void *h, const char *to, const char *iq_id, 
                                    xmpp_stanza_t * const cwmprpc_elm)
{       
    int i;
    xmpp_stanza_t *el_pl;
    xmpp_stanza_t *el_pvs;
    parameterValueStruct_s *tmp_paramValueStruct_p;
    parameterValueList_s paramList;
    parameterValueStruct_s *paramValueStruct;

    log_debug("system_get_free_heap_size = %d\n", system_get_free_heap_size()); 

    el_pl = xmpp_stanza_get_child_by_name(cwmprpc_elm, QN_ParameterList);
    if(el_pl == NULL) 
    {
        log_error("Expect <ParameterList> tag\n");
        return;
    }   
    log_debug("test\n");

    memset(&paramList, 0, sizeof(parameterValueList_s));
    //Get all the parameter/value pair
    
    log_debug("test\n");
    for(el_pvs = xmpp_stanza_get_children(el_pl); el_pvs; 
        el_pvs = xmpp_stanza_get_next (el_pvs)) 
    { 
        log_debug("test\n");
        if(strcmp(xmpp_stanza_get_name(el_pvs), QN_ParameterValueStruct) == 0)
        {            
            log_debug("test\n");
            tmp_paramValueStruct_p = (parameterValueStruct_s *)realloc(paramList.parameterValueStructs, (sizeof(parameterValueStruct_s) * (paramList.size + 1)));
            if(tmp_paramValueStruct_p == NULL)
                goto mem_free;

            paramList.size++;
            paramList.parameterValueStructs = tmp_paramValueStruct_p;
            paramValueStruct = &(paramList.parameterValueStructs[paramList.size - 1]);
            memset(paramValueStruct, 0, sizeof(parameterValueStruct_s));
            log_debug("test\n");
                    
            paramValueStruct->Name = strdup(xmpp_stanza_get_text(xmpp_stanza_get_child_by_name(el_pvs, QN_ParameterValueStruct_Name)));
                log_debug("test\n");

            paramValueStruct->Value = strdup(xmpp_stanza_get_text(xmpp_stanza_get_child_by_name(el_pvs, QN_ParameterValueStruct_Value)));
                log_debug("test\n");

            paramValueStruct->FaultCode = 0;
                
                log_debug("test\n");
        }
    }
    log_debug("test\n");

    DoSetParameterValues(&paramList);   
    SendSetParameterValuesResponse(h, to, iq_id, &paramList);
    CwmpInformConfigChanged(&paramList);
    CwmpInformRealDataChanged(&paramList);

mem_free:
    log_debug("test\n");
    for(i = 0; i < paramList.size; i++)
    {
        paramValueStruct = &(paramList.parameterValueStructs[i]);
        free(paramValueStruct->Name);
        free(paramValueStruct->Value);
    }
    log_debug("test\n");

    free(paramList.parameterValueStructs);  
    
    log_debug("system_get_free_heap_size = %d\n", system_get_free_heap_size()); 
}

static void ICACHE_FLASH_ATTR
CwmpRpcGetParameterValues(void *h, const char *to, const char *iq_id, 
                                    xmpp_stanza_t * const cwmprpc_elm)
{
    int i;
    xmpp_stanza_t* el_pn, *el_str;    
    parameterValueStruct_s *tmp_paramValueStruct_p;
    parameterValueList_s paramList;
    parameterValueStruct_s *paramValueStruct;

    log_debug("system_get_free_heap_size = %d\n", system_get_free_heap_size()); 
    paramList.size = 0;
    el_pn = xmpp_stanza_get_child_by_name(cwmprpc_elm, QN_ParameterNames);
    if(el_pn == NULL) 
    {
        log_error("Expect <ParameterNames> tag\n");
        return;
    }   

    memset(&paramList, 0, sizeof(parameterValueList_s));

    for(el_str = xmpp_stanza_get_children(el_pn); el_str; 
        el_str = xmpp_stanza_get_next(el_str)) 
    {
    
        log_debug("el_str:name:%s\n", xmpp_stanza_get_name(el_str));
        log_debug("el_str:cdata:%s\n", xmpp_stanza_get_text(el_str));
        if(strcmp(xmpp_stanza_get_name(el_str), QN_string) == 0)
        {
            tmp_paramValueStruct_p = (parameterValueStruct_s *)realloc(paramList.parameterValueStructs, (sizeof(parameterValueStruct_s) * (paramList.size + 1)));
            if(tmp_paramValueStruct_p == NULL)
                goto mem_free;

            paramList.size++;  
            paramList.parameterValueStructs = tmp_paramValueStruct_p;
            paramValueStruct = &(paramList.parameterValueStructs[paramList.size - 1]);
            memset(paramValueStruct, 0, sizeof(parameterValueStruct_s));
                    
            paramValueStruct->Name = strdup(xmpp_stanza_get_text(el_str));
            paramValueStruct->FaultCode = 0;
        }
    }

    log_debug("system_get_free_heap_size = %d\n", system_get_free_heap_size()); 

    DoGetParameterValues(&paramList); 
    
    log_debug("system_get_free_heap_size = %d\n", system_get_free_heap_size()); 
    SendGetParameterValuesResponse(h, to, iq_id, &paramList);

mem_free:
    for(i = 0; i < paramList.size; i++)
    {
        paramValueStruct = &(paramList.parameterValueStructs[i]);
        free(paramValueStruct->Name);
        free(paramValueStruct->Value);
    }
    
    free(paramList.parameterValueStructs);      
    log_debug("system_get_free_heap_size = %d\n", system_get_free_heap_size()); 
}

static void ICACHE_FLASH_ATTR
CwmpRpcReboot(void *h, const char *to, const char *iq_id, 
                                    xmpp_stanza_t * const cwmprpc_elm)
{
    SendRebootResponse(h, to, iq_id);
    wifi_led_status_action(WIFI_LED_STATUS_OFFLINE);
    SendXmppLogout();

    os_timer_disarm(&xmpp_rpc_timer);
    os_timer_setfn(&xmpp_rpc_timer, (os_timer_func_t *)system_restart, NULL);
    os_timer_arm(&xmpp_rpc_timer, 1000, 0);
}

static void ICACHE_FLASH_ATTR
CwmpRpcFactoryReset(void *h, const char *to, const char *iq_id, 
                                    xmpp_stanza_t * const cwmprpc_elm)
{
    SendFactoryResetResponse(h, to, iq_id);
    wifi_led_status_action(WIFI_LED_STATUS_RECEIVE_CONFIG);
    system_factory_reset();
}

static void ICACHE_FLASH_ATTR
CwmpRpcDownload(const char *to, const char *iq_id, 
                                    xmpp_stanza_t * const cwmprpc_elm)
{
#if 0
    xmpp_stanza_t* filetype_el;
    xmpp_stanza_t* url_el;
    xmpp_stanza_t* username_el;
    xmpp_stanza_t* pwd_el;
    xmpp_stanza_t* md5_el;
    xmpp_stanza_t* size_el;
    xmpp_stanza_t* surl_el;
    xmpp_stanza_t* smd5_el;
    xmpp_stanza_t* ssize_el;    
    xmpp_stanza_t* slaveType_el;    
    xmpp_stanza_t* slaveSN_el;
    int status = Upgrade_Failure;
    int progress = 0;
    int upgrade_file_type = 0;

    filetype_el = xmpp_stanza_get_child_by_name(cwmprpc_elm, QN_FileType);
    if(filetype_el == NULL) 
    {
        log_error("Expect <FileType> tag\n");
        goto fail;
    }   

    url_el = xmpp_stanza_get_child_by_name(cwmprpc_elm, QN_URL);
    if(url_el == NULL) 
    {
        log_error("Expect <URL> tag\n");
        goto fail;
    }   

    username_el = xmpp_stanza_get_child_by_name(cwmprpc_elm, QN_Username);
    if(username_el == NULL) 
    {
        log_error("Expect <Username> tag\n");
        goto fail;
    }   
    
    pwd_el = xmpp_stanza_get_child_by_name(cwmprpc_elm, QN_Password);
    if(pwd_el == NULL) 
    {
        log_error("Expect <Password> tag\n");
        goto fail;
    }   
    
    md5_el = xmpp_stanza_get_child_by_name(cwmprpc_elm, QN_MD5);
    if(md5_el == NULL) 
    {
        log_error("Expect <MD5> tag\n");
        goto fail;
    } 
    
    size_el = xmpp_stanza_get_child_by_name(cwmprpc_elm, QN_SIZE);
    if(size_el == NULL) 
    {
        log_error("Expect <Size> tag\n");
        goto fail;
    }   
    
    surl_el = xmpp_stanza_get_child_by_name(cwmprpc_elm, QN_SURL);
    if(surl_el == NULL) 
    {
        log_error("Expect <SURL> tag\n");
        goto fail;
    }   

    smd5_el = xmpp_stanza_get_child_by_name(cwmprpc_elm, QN_SMD5);
    if(smd5_el == NULL) 
    {
        log_error("Expect <SMD5> tag\n");
        goto fail;
    }   

    ssize_el = xmpp_stanza_get_child_by_name(cwmprpc_elm, QN_SSIZE);
    if(size_el == NULL) 
    {
        log_error("Expect <SSize> tag\n");
        goto fail;
    }   
    
    slaveType_el = xmpp_stanza_get_child_by_name(cwmprpc_elm, QN_SlaveType);
    if(slaveType_el == NULL) {
        log_error("Expect <SlaveType> tag\n");
    }     

    slaveSN_el = xmpp_stanza_get_child_by_name(cwmprpc_elm, QN_SlaveSN);
    if(slaveSN_el == NULL) {
        log_error("Expect <SlaveSN> tag\n");
    }      
    
    char *filetype = xmpp_stanza_get_text(filetype_el);
    char *username = xmpp_stanza_get_text(username_el);
    char *pwd = xmpp_stanza_get_text(pwd_el);
    char *size;
    char *url;
    char *md5;

    if(filetype == NULL) 
        goto fail;

    if(strcmp(filetype, FILETYPE_FIRWARE_UPGRADE_IMAGE) == 0)
    {
        upgrade_file_type = UPGRADE_FILETYPE_MAIN_IMAGE;
        if(system_upgrade_userbin_check() == USER_BIN1)
        {
            url = xmpp_stanza_get_text(surl_el);
            md5 = xmpp_stanza_get_text(smd5_el);
            size = xmpp_stanza_get_text(ssize_el);
        }
        else
        {
            url = xmpp_stanza_get_text(url_el);
            md5 = xmpp_stanza_get_text(md5_el);        
            size = xmpp_stanza_get_text(size_el);
        }
    }
    else if(strcmp(filetype, FILETYPE_SLAVEDEV_FIRWARE_UPGRADE_IMAGE) == 0)
    {
        upgrade_file_type = UPGRADE_FILETYPE_SLAVEDEV_IMAGE;
        
        url = xmpp_stanza_get_text(url_el);
        md5 = xmpp_stanza_get_text(md5_el);        
        size = xmpp_stanza_get_text(size_el);
    }
    else
    {
        goto fail;
    }
        
    char host[MAXHOSTNAMELEN+1] = {0};
    unsigned short port;
    char *filename = NULL;
    int servertype = SERVERTYPE_UNKNOWN;    
    char *p;
    uint32 file_size = 0;
    
    if(xmpp_parseURL(url, &servertype, host, &port, &filename) == 0)
    {
        log_debug("test\n");
        goto fail;
    }   

    if(strcmp(size, "null") != 0)
    {
        file_size = atoi(size);
    }

    status = Downloading_Firmware;

    if(xmpp_download_server == NULL)
    {
        xmpp_download_server = (struct upgrade_server_info *)zalloc(sizeof(struct upgrade_server_info));
        xmpp_download_server->port = port;
        xmpp_download_server->file_type = upgrade_file_type;
        xmpp_download_server->url = (uint8 *)zalloc(128+strlen(filename)+strlen(host));
        xmpp_download_server->file_md5 = strdup(md5);
        xmpp_download_server->to = strdup(to);
        xmpp_download_server->iq_id = strdup(iq_id);
        xmpp_download_server->check_cb = xmpp_download_upgrade_rsp;
        xmpp_download_server->check_times = 240000;
        xmpp_download_server->file_size = file_size;
        
        os_sprintf(xmpp_download_server->url, "GET %s HTTP/1.0\r\nHost: %s:%d\r\n"
                                              "Connection: keep-alive\r\n"
                                              "User-Agent: Wget\r\n\r\n", filename, host, port);    
        log_debug("%s\n", xmpp_download_server->url);
        
        xmpp_download_start_dns(host); 
        status = Downloading_Firmware;
    }
    else
    {
        status = xmpp_download_server->upgrade_status & 0x0F;
        progress = xmpp_download_server->upgrade_progress;
    }
    
fail:
    SendDownloadResponse(to, iq_id, status, progress);
#endif    
}

void ICACHE_FLASH_ATTR
CwmpRpcProcess(void *h, const char *to, const char *iq_id, 
                      iks * const cwmprpc_body)
{
    iks* cwmprpc_el_;
    const char *method_name;
    int i, len;

    log_debug("test\n");
    if(!(cwmprpc_el_ = iks_child(cwmprpc_body)) || 
        !(method_name = iks_name(cwmprpc_el_)))
    {
        log_error("Format error!\n");
        return;
    }
    
    log_debug("RPC: %s\n", method_name);
    if(strcmp(method_name, QN_CWMP_SPV) == 0)
    {
        CwmpRpcSetParameterValues(h, to, iq_id, cwmprpc_el_);
    }
    else if(strcmp(method_name, QN_CWMP_GPV) == 0)
    {
        CwmpRpcGetParameterValues(h, to, iq_id, cwmprpc_el_);
    }
    else if(strcmp(method_name, QN_CWMP_REBOOT) == 0)
    {
        CwmpRpcReboot(h, to, iq_id, cwmprpc_el_);
    }
    else if(strcmp(method_name, QN_CWMP_RESET) == 0)
    {
        CwmpRpcFactoryReset(h, to, iq_id, cwmprpc_el_);
    }
    else if(strcmp(method_name, QN_CWMP_DOWNLOAD) == 0)
    {
        CwmpRpcDownload(to, iq_id, cwmprpc_el_);
    }
    else
    {
        log_error("RPC: %s not supported!\n", method_name);
    }
}


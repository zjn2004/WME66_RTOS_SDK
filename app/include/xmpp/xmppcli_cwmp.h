#ifndef XMPPCLI_CWMP_H
#define XMPPCLI_CWMP_H

#include "iksemel/iksemel.h"

extern unsigned int sampling_intval_seconds_;
extern unsigned int inform_intval_seconds_;

#define strdup iks_strdup
#define xmpp_stanza_t iks
#define xmpp_stanza_get_children(x) iks_child(x)
#define xmpp_stanza_get_next(x) iks_next_tag(x)
#define xmpp_stanza_get_child_by_name(x, name) iks_find_child(x, name)
#define xmpp_stanza_get_name(x) iks_name(x)
#define xmpp_stanza_get_text(x) iks_cdata(iks_child(x))
#define xmpp_stanza_set_contents(x, content) iks_insert_cdata(x, content, 0)
#define xmpp_stanza_create(x) iks_new(x)
#define xmpp_stanza_create_within(name, s) iks_new_within(name, iks_stack(s))
#define xmpp_stanza_set_ns(iq_stanza, iq_ns) iks_insert_attrib(iq_stanza, "xmlns", iq_ns)
#define xmpp_stanza_set_type(iq_stanza, iq_type) iks_insert_attrib(iq_stanza, "type", iq_type)
#define xmpp_stanza_set_id(iq_stanza, iq_id) iks_insert_attrib(iq_stanza, "id", iq_id)
#define xmpp_stanza_set_attribute(iq_stanza, attribute, name) iks_insert_attrib(iq_stanza, attribute, name)
#define xmpp_stanza_add_child_release(x, y) iks_insert_node(x, y)
#define xmpp_stanza_release(x) iks_delete(x)

#define QN_CWMP_SPV                     "SetParameterValues"
#define QN_CWMP_SPV_RSP                 "SetParameterValuesResponse"
#define QN_CWMP_GPV                     "GetParameterValues"
#define QN_CWMP_GPV_RSP                 "GetParameterValuesResponse"
#define QN_CWMP_REBOOT                  "Reboot" 
#define QN_CWMP_REBOOT_RSP              "RebootResponse" 
#define QN_CWMP_RESET                   "FactoryReset" 
#define QN_CWMP_RESET_RSP               "FactoryResetResponse" 
#define QN_CWMP_DOWNLOAD                "Download" 
#define QN_CWMP_DOWNLOAD_RSP            "DownloadResponse" 
#define QN_CWMP_INFORM                  "Inform" 

#define QN_DeviceInfo                   "DeviceInfo"
#define QN_Reason                       "Reason"
#define QN_Data                         "Data"
#define QN_Alarm                        "Alarm"
#define QN_ConfigChanged                "ConfigChanged"


#define QN_ParameterList                "ParameterList"
#define QN_ParameterNames               "ParameterNames"
#define QN_ParameterValueStruct         "ParameterValueStruct"
#define QN_ParameterValueStruct_Name    "Name"
#define QN_ParameterValueStruct_Value   "Value"
#define QN_Status                       "Status"
#define QN_StatusString                 "StatusString"
#define QN_Progress                     "Progress"
#define QN_FaultList                    "FaultList"
#define QN_Fault                        "Fault"
#define QN_ParameterName                "ParameterName"
#define QN_FaultCode                    "FaultCode"
#define QN_FaultString                  "FaultString"
#define QN_string                       "string"
#define QN_Time                         "Time"
#define QN_Text                         "Text"
#define QN_Level                        "Level"
#define QN_FileType                     "FileType"
#define QN_URL                          "URL"
#define QN_Username                     "Username"
#define QN_Password                     "Password"
#define QN_MD5                          "MD5"
#define QN_SIZE                          "Size"
#define QN_SURL                          "SURL"
#define QN_SMD5                          "SMD5"
#define QN_SSIZE                         "SSize"
#define QN_SlaveType                    "SlaveType"
#define QN_SlaveSN                      "SlaveSN"

#define QN_DEVICEINFO_TYPE              "Type"
#define QN_DEVICEINFO_VERSION           "Version"
#define QN_DEVICEINFO_SLAVEDEVLIST      "SlaveDevList"

#define QN_DataPath         "Path"
#define QN_DataPath_Name    "name"

enum {
    ALARM_LEVEL_HIGH = 0,          /*need send text message*/
    ALARM_LEVEL_MEDIUM,            /*not used*/
    ALARM_LEVEL_LOW,              /*don't need send text message*/
};

enum { 
    Success = 0,
    Method_Not_Supported = 9000,
    Request_Denied = 9001,
    Internal_Error = 9002,
    Invalid_Arguments = 9003,    
    Resources_Exceeded = 9004,
    Invalid_Parameter_Name = 9005,
    Invalid_Parameter_Type = 9006,
    Invalid_Parameter_Value = 9007,    
    Upgrade_Failure = 9800,
    Downloading_Firmware = 9801,
    Upgrading_Firmware = 9802,
};

#define FILETYPE_FIRWARE_UPGRADE_IMAGE              "1 Firmware Upgrade Image"
#define FILETYPE_SLAVEDEV_FIRWARE_UPGRADE_IMAGE     "4 SlaveDev Firmware Upgrade Image"

#define MAX_PARAM_VALUE_LEN  512

#define DEVICE_INFORM_INTERVAL          600
#define DEVICE_SAMPLING_INTERVAL        60

#define MAXHOSTNAMELEN  64
#define MIN(a, b) (((a) < (b))?(a) : (b))
enum {
    SERVERTYPE_HTTP = 1,
    SERVERTYPE_FTP,
    SERVERTYPE_UNKNOWN
};

typedef struct cwmp_parameterValueStruct_t
{
    char *Name;	
    char *Value;	
    int FaultCode; //ret code
}parameterValueStruct_s;

typedef struct cwmp_parameterValueList_t
{  
    struct cwmp_parameterValueStruct_t *parameterValueStructs;
    int size;
}parameterValueList_s;

typedef struct valueChangeParam_t
{
    char *param;
    char *value;
}valueChangeParam_s;

typedef struct valueChangeParamList_t
{
    int count;
    struct valueChangeParam_t *paramArray;
}valueChangeParamList_s;


typedef struct historyData_t
{
    unsigned int sampleTime;
    char *value;
    struct historyData_t *next;
}historyData_s;

typedef struct historyDataInform_t
{
    char *name;
    struct historyData_t *historyDatas;
    struct historyDataInform_t *next;
}historyDataInform_s;


/*------------------------------------------------------------------------------------------
Cwmp function external to main 
--------------------------------------------------------------------------------------------
*/
int 
cwmp_device_auth(char *auth_devid, char *auth_passwd);

void 
start_cwmp_inform(void);
 
void 
system_factory_reset(void);

void 
CwmpCheckOneAlarm(char *path_name, char *para_value);

int 
cwmp_inform_handler(void * parg);

int 
cwmp_sampling_handler(void * parg);

/*------------------------------------------------------------------------------------------
Cwmp RPC Method function external to soap
--------------------------------------------------------------------------------------------
*/

void
CwmpRpcProcess(void *h, const char *to, const char *iq_id, 
                      iks * const cwmprpc_body);

#endif

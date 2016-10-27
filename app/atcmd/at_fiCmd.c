/*
 * File	: at_baseCmd.c
 * This file is part of Seaing's AT+ command set program.
 * Copyright (C) 2013 - 2016, Seaing Systems
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of version 3 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
//#include <stdlib.h>
#include "esp_common.h"
#include "atcmd/at.h"
#include "atcmd/at_fiCmd.h"
#include "hnt_interface.h"
#include "atcmd/at_version.h"
//#include "driver/uart_register.h"
//#include "gpio.h"
#include "mgmt/mgmt.h"

#define GPIO_VALUE_0 0
#define GPIO_VALUE_1 1

/** @defgroup AT_FICMD_Functions
  * @{
  */

/**
  * @brief  Copy param from receive data to dest.
  * @param  pDest: point to dest
  * @param  pSrc: point to source
  * @param  maxLen: copy max number of byte
  * @retval the length of param
  *   @arg -1: failure
  */
int8_t ICACHE_FLASH_ATTR
at_dataStrCpy(void *pDest, const void *pSrc, int8_t maxLen)
{
//  assert(pDest!=NULL && pSrc!=NULL);

  char *pTempD = pDest;
  const char *pTempS = pSrc;
  int8_t len;

  if(*pTempS != '\"')
  {
    return -1;
  }
  pTempS++;
  for(len=0; len<maxLen; len++)
  {
    if(*pTempS == '\"')
    {
      *pTempD = '\0';
      break;
    }
    else
    {
      *pTempD++ = *pTempS++;
    }
  }
  if(len == maxLen)
  {
    return -1;
  }
  return len;
}

  
void ICACHE_FLASH_ATTR
at_queryCmdCfstamac(uint8_t id)
{
    char temp[64];

    struct hnt_mgmt_factory_param tmp_factory_param;   
    os_memset(&tmp_factory_param, 0, sizeof(tmp_factory_param));
    hnt_mgmt_load_factory_param(&tmp_factory_param);
  
    os_sprintf(temp, "%s:", at_fun[id].at_cmdName);
    uart0_sendStr(temp);

    os_sprintf(temp, "\""MACSTR"\"\r\n", MAC2STR(tmp_factory_param.wlan_mac));
    uart0_sendStr(temp);
    at_backOk;
}

void ICACHE_FLASH_ATTR
at_setupCmdCfstamac(uint8_t id, char *pPara)
{
	int8_t len,i;
  uint8 bssid[6];
  char temp[64];
  struct hnt_mgmt_factory_param tmp_factory_param;   
  
	pPara++;
	
  len = at_dataStrCpy(temp, pPara, 32);
  if(len != 17)
  {
    at_backError;
    return;
  }

  pPara++; 
 
  for(i=0;i<6;i++)
  {
    bssid[i] = strtol(pPara,&pPara,16);
    pPara += 1;
  }
 
  log_debug(MACSTR"\r\n", MAC2STR(bssid));

  os_memset(&tmp_factory_param, 0, sizeof(tmp_factory_param));
  hnt_mgmt_load_factory_param(&tmp_factory_param);

  os_memcpy(tmp_factory_param.wlan_mac, bssid, 6);
  hnt_mgmt_save_factory_param(&tmp_factory_param);

	at_backOk;
}

void ICACHE_FLASH_ATTR
at_queryCmdCfdevicetype(uint8_t id)
{
    char temp[128];

    struct hnt_mgmt_factory_param tmp_factory_param;   
    os_memset(&tmp_factory_param, 0, sizeof(tmp_factory_param));
    hnt_mgmt_load_factory_param(&tmp_factory_param);
  
    os_sprintf(temp, "%s:", at_fun[id].at_cmdName);
    uart0_sendStr(temp);
    tmp_factory_param.device_type[DEVICEINFO_STRING_LEN-1] = '\0';

    os_sprintf(temp, "\"%s\"\r\n", tmp_factory_param.device_type);
    uart0_sendStr(temp);
    at_backOk;
}

void ICACHE_FLASH_ATTR
at_setupCmdCfdevicetype(uint8_t id, char *pPara)
{
	int8_t len;
  char temp[64];
  struct hnt_mgmt_factory_param tmp_factory_param;   
  
	pPara++;
	
  len = at_dataStrCpy(temp, pPara, DEVICEINFO_STRING_LEN);
  if(len == -1)
  {
    at_backError;
    return;
  }

  log_debug("%s\r\n", temp);

  os_memset(&tmp_factory_param, 0, sizeof(tmp_factory_param));
  hnt_mgmt_load_factory_param(&tmp_factory_param);

  os_memset(tmp_factory_param.device_type, 0, DEVICEINFO_STRING_LEN);
  os_memcpy(tmp_factory_param.device_type, temp, len);
  tmp_factory_param.device_type[len] = '\0';  
  hnt_mgmt_save_factory_param(&tmp_factory_param);

	at_backOk;
}

void ICACHE_FLASH_ATTR
at_queryCmdCfdeviceid(uint8_t id)
{
    char temp[128];

    struct hnt_mgmt_factory_param tmp_factory_param;   
    os_memset(&tmp_factory_param, 0, sizeof(tmp_factory_param));
    hnt_mgmt_load_factory_param(&tmp_factory_param);
  
    os_sprintf(temp, "%s:", at_fun[id].at_cmdName);
    uart0_sendStr(temp);
    tmp_factory_param.device_id[DEVICEINFO_STRING_LEN-1] = '\0';

    os_sprintf(temp, "\"%s\"\r\n", tmp_factory_param.device_id);
    uart0_sendStr(temp);
    at_backOk;
}

void ICACHE_FLASH_ATTR
at_setupCmdCfdeviceid(uint8_t id, char *pPara)
{
	int8_t len;
  char temp[64];
  struct hnt_mgmt_factory_param tmp_factory_param;   
  
	pPara++;
	
  len = at_dataStrCpy(temp, pPara, DEVICEINFO_STRING_LEN);
  if(len == -1)
  {
    at_backError;
    return;
  }

  log_debug("%s\r\n", temp);

  os_memset(&tmp_factory_param, 0, sizeof(tmp_factory_param));
  hnt_mgmt_load_factory_param(&tmp_factory_param);

  os_memcpy(tmp_factory_param.device_id, temp, len);
  tmp_factory_param.device_id[len] = '\0';
  hnt_mgmt_save_factory_param(&tmp_factory_param);

	at_backOk;
}

void ICACHE_FLASH_ATTR
at_queryCmdCfpassword(uint8_t id)
{
    char temp[128];

    struct hnt_mgmt_factory_param tmp_factory_param;   
    os_memset(&tmp_factory_param, 0, sizeof(tmp_factory_param));
    hnt_mgmt_load_factory_param(&tmp_factory_param);
  
    os_sprintf(temp, "%s:", at_fun[id].at_cmdName);
    uart0_sendStr(temp);
    tmp_factory_param.password[DEVICEINFO_STRING_LEN-1] = '\0';

    os_sprintf(temp, "\"%s\"\r\n", tmp_factory_param.password);
    uart0_sendStr(temp);
    at_backOk;
}

void ICACHE_FLASH_ATTR
at_setupCmdCfpassword(uint8_t id, char *pPara)
{
	int8_t len;
  char temp[64];
  struct hnt_mgmt_factory_param tmp_factory_param;   
  
	pPara++;
	
  len = at_dataStrCpy(temp, pPara, DEVICEINFO_STRING_LEN);
  if(len == -1)
  {
    at_backError;
    return;
  }

  log_debug("%s\r\n", temp);

  os_memset(&tmp_factory_param, 0, sizeof(tmp_factory_param));
  hnt_mgmt_load_factory_param(&tmp_factory_param);

  os_memcpy(tmp_factory_param.password, temp, len);
  tmp_factory_param.password[len] = '\0';
  hnt_mgmt_save_factory_param(&tmp_factory_param);

	at_backOk;
}

void ICACHE_FLASH_ATTR
at_queryCmdCfxmppserver(uint8_t id)
{
    char temp[128];

    struct hnt_mgmt_factory_param tmp_factory_param;   
    os_memset(&tmp_factory_param, 0, sizeof(tmp_factory_param));
    hnt_mgmt_load_factory_param(&tmp_factory_param);
  
    os_sprintf(temp, "%s:", at_fun[id].at_cmdName);
    uart0_sendStr(temp);
    tmp_factory_param.xmpp_server[63] = '\0';

    os_sprintf(temp, "\"%s\"\r\n", tmp_factory_param.xmpp_server);
    uart0_sendStr(temp);
    at_backOk;
}

void ICACHE_FLASH_ATTR
at_setupCmdCfxmppserver(uint8_t id, char *pPara)
{
	int8_t len;
  char temp[64];
  struct hnt_mgmt_factory_param tmp_factory_param;   
  
	pPara++;
	
  len = at_dataStrCpy(temp, pPara, DEVICEINFO_STRING_LEN);
  if(len == -1)
  {
    at_backError;
    return;
  }

  log_debug("%s\r\n", temp);

  os_memset(&tmp_factory_param, 0, sizeof(tmp_factory_param));
  hnt_mgmt_load_factory_param(&tmp_factory_param);

  os_memcpy(tmp_factory_param.xmpp_server, temp, len);
  tmp_factory_param.xmpp_server[len] = '\0';
  hnt_mgmt_save_factory_param(&tmp_factory_param);

	at_backOk;
}

void ICACHE_FLASH_ATTR
at_queryCmdCfxmppjid(uint8_t id)
{
    char temp[128];

    struct hnt_mgmt_factory_param tmp_factory_param;   
    os_memset(&tmp_factory_param, 0, sizeof(tmp_factory_param));
    hnt_mgmt_load_factory_param(&tmp_factory_param);
  
    os_sprintf(temp, "%s:", at_fun[id].at_cmdName);
    uart0_sendStr(temp);
    tmp_factory_param.xmpp_jid[63] = '\0';

    os_sprintf(temp, "\"%s\"\r\n", tmp_factory_param.xmpp_jid);
    uart0_sendStr(temp);
    at_backOk;
}

void ICACHE_FLASH_ATTR
at_setupCmdCfxmppjid(uint8_t id, char *pPara)
{
	int8_t len;
  char temp[64];
  struct hnt_mgmt_factory_param tmp_factory_param;   
  
	pPara++;
	
  len = at_dataStrCpy(temp, pPara, DEVICEINFO_STRING_LEN);
  if(len == -1)
  {
    at_backError;
    return;
  }

  log_debug("%s\r\n", temp);

  os_memset(&tmp_factory_param, 0, sizeof(tmp_factory_param));
  hnt_mgmt_load_factory_param(&tmp_factory_param);

  os_memcpy(tmp_factory_param.xmpp_jid, temp, len);
  tmp_factory_param.xmpp_jid[len] = '\0';
  hnt_mgmt_save_factory_param(&tmp_factory_param);

	at_backOk;
}

void ICACHE_FLASH_ATTR
at_queryCmdCfxmpppwd(uint8_t id)
{
    char temp[128];

    struct hnt_mgmt_factory_param tmp_factory_param;   
    os_memset(&tmp_factory_param, 0, sizeof(tmp_factory_param));
    hnt_mgmt_load_factory_param(&tmp_factory_param);
  
    os_sprintf(temp, "%s:", at_fun[id].at_cmdName);
    uart0_sendStr(temp);
    tmp_factory_param.xmpp_password[63] = '\0';

    os_sprintf(temp, "\"%s\"\r\n", tmp_factory_param.xmpp_password);
    uart0_sendStr(temp);
    at_backOk;
}

void ICACHE_FLASH_ATTR
at_setupCmdCfxmpppwd(uint8_t id, char *pPara)
{
	int8_t len;
  char temp[64];
  struct hnt_mgmt_factory_param tmp_factory_param;   
  
	pPara++;
	
  len = at_dataStrCpy(temp, pPara, DEVICEINFO_STRING_LEN);
  if(len == -1)
  {
    at_backError;
    return;
  }

  log_debug("%s\r\n", temp);

  os_memset(&tmp_factory_param, 0, sizeof(tmp_factory_param));
  hnt_mgmt_load_factory_param(&tmp_factory_param);

  os_memcpy(tmp_factory_param.xmpp_password, temp, len);
  tmp_factory_param.xmpp_password[len] = '\0';
  hnt_mgmt_save_factory_param(&tmp_factory_param);

	at_backOk;
}

void ICACHE_FLASH_ATTR
at_queryCmdCfptoinfo(uint8_t id)
{
    char temp[64];
#if 1//def HNT_PTO_SUPPORT

    struct hnt_mgmt_factory_param tmp_factory_param;   
    os_memset(&tmp_factory_param, 0, sizeof(tmp_factory_param));
    hnt_mgmt_load_factory_param(&tmp_factory_param);
  
    os_sprintf(temp, "%s:", at_fun[id].at_cmdName);
    uart0_sendStr(temp);
    
    os_sprintf(temp, "\"%02x:%02x:%02x:%02x\"\r\n", 
                    tmp_factory_param.pto_info[0], 
                    tmp_factory_param.pto_info[1], 
                    tmp_factory_param.pto_info[2], 
                    tmp_factory_param.pto_info[3]);
    uart0_sendStr(temp);
#endif
    at_backOk;
}

void ICACHE_FLASH_ATTR
at_setupCmdCfptoinfo(uint8_t id, char *pPara)
{
	int8_t len;
  char temp[64] = {0};
  int i;
  uint8 pto_info[4] = {0};

#if 1//def HNT_PTO_SUPPORT
  struct hnt_mgmt_factory_param tmp_factory_param;   
  
	pPara++;
	
    len = at_dataStrCpy(temp, pPara, 32);
    if(len != 11)
    {
      at_backError;
      return;
    }
    
    pPara++; 
    
    for(i=0;i<4;i++)
    {
      pto_info[i] = strtol(pPara,&pPara,16);
      pPara += 1;
    }

  log_debug("%s\r\n", temp);

  os_memset(&tmp_factory_param, 0, sizeof(tmp_factory_param));
  hnt_mgmt_load_factory_param(&tmp_factory_param);

  os_memcpy(tmp_factory_param.pto_info, pto_info, 4);
  hnt_mgmt_save_factory_param(&tmp_factory_param);
#endif
	at_backOk;
}

void ICACHE_FLASH_ATTR
at_queryCmdCfdeviceinfo(uint8_t id)
{
    char temp[128];

    struct hnt_mgmt_factory_param tmp_factory_param;   
    os_memset(&tmp_factory_param, 0, sizeof(tmp_factory_param));
    hnt_mgmt_load_factory_param(&tmp_factory_param);
  
    os_sprintf(temp, "%s:", at_fun[id].at_cmdName);
    uart0_sendStr(temp);
    tmp_factory_param.device_type[DEVICEINFO_STRING_LEN-1] = '\0';
    tmp_factory_param.device_id[DEVICEINFO_STRING_LEN-1] = '\0';
    tmp_factory_param.password[DEVICEINFO_STRING_LEN-1] = '\0';
    tmp_factory_param.xmpp_server[DEVICEINFO_STRING_LEN-1] = '\0';
    tmp_factory_param.xmpp_jid[DEVICEINFO_STRING_LEN-1] = '\0';
    tmp_factory_param.xmpp_password[DEVICEINFO_STRING_LEN-1] = '\0';

    if(tmp_factory_param.device_type[0] == 0xFF)
        tmp_factory_param.device_type[0] = '\0';
    if(tmp_factory_param.device_id[0] == 0xFF)
        tmp_factory_param.device_id[0] = '\0';
    if(tmp_factory_param.password[0] == 0xFF)
        tmp_factory_param.password[0] = '\0';
    if(tmp_factory_param.xmpp_server[0] == 0xFF)
        tmp_factory_param.xmpp_server[0] = '\0';
    if(tmp_factory_param.xmpp_jid[0] == 0xFF)
        tmp_factory_param.xmpp_jid[0] = '\0';
    if(tmp_factory_param.xmpp_password[0] == 0xFF)
        tmp_factory_param.xmpp_password[0] = '\0';
        
    os_sprintf(temp, "\r\ndevice_type = \"%s\"\r\n", tmp_factory_param.device_type);
    uart0_sendStr(temp);
    os_sprintf(temp, "device_id =  \"%s\"\r\n", tmp_factory_param.device_id);
    uart0_sendStr(temp);
    os_sprintf(temp, "password = \"%s\"\r\n", tmp_factory_param.password);
    uart0_sendStr(temp);
    os_sprintf(temp, "xmpp_server = \"%s\"\r\n", tmp_factory_param.xmpp_server);
    uart0_sendStr(temp);
    os_sprintf(temp, "xmpp_jid = \"%s\"\r\n", tmp_factory_param.xmpp_jid);
    uart0_sendStr(temp);
    os_sprintf(temp, "xmpp_password = \"%s\"\r\n", tmp_factory_param.xmpp_password);
    uart0_sendStr(temp);
    os_sprintf(temp, "sta_mac = \""MACSTR"\"\r\n", MAC2STR(tmp_factory_param.wlan_mac));
    uart0_sendStr(temp);
    at_backOk;
}

void ICACHE_FLASH_ATTR
at_CfdeviceinfoInternal(char *temp)
{
  char *pPara;
  char mac[64];
  char *start, *end;
  int i;
  struct hnt_mgmt_factory_param tmp_factory_param;   
  
  log_debug("%s\r\n", temp);

  os_memset(&tmp_factory_param, 0, sizeof(tmp_factory_param));
  hnt_mgmt_load_factory_param(&tmp_factory_param);

  start = end = temp;
  while((*end != ' ') && (*end != '\0'))end++;

  if(end-start > DEVICEINFO_STRING_LEN)
    return;
  
  os_memset(tmp_factory_param.device_type, 0, sizeof(tmp_factory_param.device_type));
  os_memcpy(tmp_factory_param.device_type, start, end-start);

  while(*end == ' ')end++;

  start = end;
  while((*end != ' ') && (*end != '\0'))end++;
  if(end-start > DEVICEINFO_STRING_LEN)
    return;
  os_memset(tmp_factory_param.device_id, 0, sizeof(tmp_factory_param.device_id));
  os_memcpy(tmp_factory_param.device_id, start, end-start);

  while(*end == ' ')end++;
  start = end;
  while((*end != ' ') && (*end != '\0'))end++;
  if(end-start > DEVICEINFO_STRING_LEN)
    return;
  os_memset(tmp_factory_param.password, 0, sizeof(tmp_factory_param.password));
  os_memcpy(tmp_factory_param.password, start, end-start);

  while(*end == ' ')end++;
  start = end;
  while((*end != ' ') && (*end != '\0'))end++;
  os_memset(tmp_factory_param.xmpp_server, 0, sizeof(tmp_factory_param.xmpp_server));
  os_memcpy(tmp_factory_param.xmpp_server, start, end-start);

#if 0
  while(*end == ' ')end++;
  start = end;
  while((*end != ' ') && (*end != '\0'))end++;
  if(end-start > DEVICEINFO_STRING_LEN)
    return;
  os_memset(tmp_factory_param.xmpp_jid, 0, sizeof(tmp_factory_param.xmpp_jid));
  os_memcpy(tmp_factory_param.xmpp_jid, start, end-start);
#endif

  while(*end == ' ')end++;
  start = end;
  while((*end != ' ') && (*end != '\0'))end++;
  if(end-start > DEVICEINFO_STRING_LEN)
    return;
  os_memset(tmp_factory_param.xmpp_password, 0, sizeof(tmp_factory_param.xmpp_password));
  os_memcpy(tmp_factory_param.xmpp_password, start, end-start);

  while(*end == ' ')end++;
  start = end;
  while((*end != ' ') && (*end != '\0'))end++;
  if(end-start > DEVICEINFO_STRING_LEN)
    return;
  os_memset(mac, 0, sizeof(mac));
  os_memcpy(mac, start, end-start);
  pPara = mac;
  for(i=0;i<6;i++)
  {
    tmp_factory_param.wlan_mac[i] = strtol(pPara,&pPara,16);
    pPara += 1;
  }

#if 0
    log_debug("test:%s\n", tmp_factory_param.xmpp_jid);
  start = &tmp_factory_param.xmpp_jid[1];
  end = os_strchr(start, '@');
  if((end == NULL) || (end-start > DEVICEINFO_STRING_LEN))
    return;
  os_memset(tmp_factory_param.device_id, 0, sizeof(tmp_factory_param.device_id));
  os_memcpy(tmp_factory_param.device_id, start, end-start);
#endif

  os_memset(tmp_factory_param.xmpp_jid, 0, sizeof(tmp_factory_param.xmpp_jid));
  tmp_factory_param.xmpp_jid[0] = 'd';
  os_strcpy(&tmp_factory_param.xmpp_jid[1], tmp_factory_param.device_id);
  os_strcpy(&tmp_factory_param.xmpp_jid[os_strlen(tmp_factory_param.xmpp_jid)], "@seaing.net");
  
  hnt_mgmt_save_factory_param(&tmp_factory_param);

}
void ICACHE_FLASH_ATTR
at_setupCmdCfdeviceinfo(uint8_t id, char *pPara)
{
    int8_t len;
    char temp[256];
  
	pPara++;
	
  len = at_dataStrCpy(temp, pPara, 120);
  if(len == -1)
  {
    at_backError;
    return;
  }

  log_debug("%s\r\n", temp);
  
  at_CfdeviceinfoInternal(temp);

	at_backOk;
}

LOCAL void ICACHE_FLASH_ATTR
gpio_set_func(int pin)
{
    switch(pin)
    {
        case 1:
            PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U,FUNC_GPIO1);
            break;   
        case 3:
            PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U,FUNC_GPIO3);
            break;   
        case 6:
            PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_CLK_U,FUNC_GPIO1);
            break;   
        case 7:
            PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_DATA0_U,FUNC_GPIO1);
            break;   
        case 8:
            PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_DATA1_U,FUNC_GPIO1);
            break;   
        case 9:
            PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_DATA2_U,FUNC_GPIO9);
            break;   
        case 10:
            PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_DATA3_U,FUNC_GPIO10);
            break;   
        case 11:
            PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_CMD_U,FUNC_GPIO12);
            break;         
        case 12:
            PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U,FUNC_GPIO12);
            break;        
        case 13:
            PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U,FUNC_GPIO13);
            break;
        case 14:
            PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U,FUNC_GPIO14);
            break;
        case 15:
            PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U,FUNC_GPIO15);
            break;
        case 0:
            PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U,FUNC_GPIO0);
            break;
        case 2:
            PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U,FUNC_GPIO2);
            break;
        case 4:
            PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U,FUNC_GPIO4);
            break;
        case 5:
            PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U,FUNC_GPIO5);
            break;
        default:
            break;
    }
}

LOCAL void ICACHE_FLASH_ATTR
gpio_test_error_msg(int out_pin, int input_pin, int value)
{
    char temp[64];

    os_memset(temp, 0, sizeof(temp));
    os_sprintf(temp, "error %d->%d(%d)\r\n", out_pin, input_pin, value);
    uart0_sendStr(temp);                
}

LOCAL int ICACHE_FLASH_ATTR
gpio_check_pin(int out_pin, int input_pin)
{
    int error = 0;
    
    log_debug("gpio_check_pin: out_pin:%d, input_pin:%d\r\n", out_pin, input_pin);

    gpio_set_func(out_pin);
    gpio_set_func(input_pin);
    
    GPIO_DIS_OUTPUT(input_pin);

    GPIO_OUTPUT_SET(out_pin, GPIO_VALUE_1);
    if(GPIO_INPUT_GET(input_pin) != GPIO_VALUE_1)
    {
        gpio_test_error_msg(out_pin, input_pin, GPIO_VALUE_1);
        error = 1;
    }

    GPIO_OUTPUT_SET(out_pin, GPIO_VALUE_0);
    if(GPIO_INPUT_GET(input_pin) != GPIO_VALUE_0)
    {
        gpio_test_error_msg(out_pin, input_pin, GPIO_VALUE_0);
        error = 1;
    }

    return error;    
}

LOCAL int ICACHE_FLASH_ATTR
gpio16_output_check_pin(int input_pin)
{
    int error = 0;

    log_debug("gpio_check_pin: out_pin:16, input_pin:%d\r\n", input_pin);

    gpio_set_func(input_pin);
    GPIO_DIS_OUTPUT(input_pin);
    
    gpio16_output_conf();
    gpio16_output_set(GPIO_VALUE_1);
    if(GPIO_INPUT_GET(input_pin) != GPIO_VALUE_1)
    {
        gpio_test_error_msg(16, input_pin, GPIO_VALUE_1);    
        error = 1;
    }

    gpio16_output_set(GPIO_VALUE_0);
    if(GPIO_INPUT_GET(input_pin) != GPIO_VALUE_0)
    {
        gpio_test_error_msg(16, input_pin, GPIO_VALUE_0);    
        error = 1;
    }
    
    return error;
}

LOCAL int ICACHE_FLASH_ATTR
gpio16_input_check_pin(int out_pin)
{
    int error = 0;

    log_debug("gpio_check_pin: out_pin:%d, input_pin:16\r\n", out_pin);

    gpio_set_func(out_pin);
    
    gpio16_input_conf();
    GPIO_OUTPUT_SET(out_pin, GPIO_VALUE_1);
    if(gpio16_input_get() != GPIO_VALUE_1)
    {
        gpio_test_error_msg(out_pin, 16, GPIO_VALUE_1);    
        error = 1;
    }

    GPIO_OUTPUT_SET(out_pin, GPIO_VALUE_0);
    if(gpio16_input_get() != GPIO_VALUE_0)
    {
        gpio_test_error_msg(out_pin, 16, GPIO_VALUE_0);    
        error = 1;
    }
    
    return error;
}

LOCAL int ICACHE_FLASH_ATTR
gpio16_input_check(int high)
{
    int error = 0;
    int gpio_value = high ? GPIO_VALUE_1 : GPIO_VALUE_0;
    
    log_debug("gpio_check: %d, input_pin:16\r\n", gpio_value);
    
    gpio16_input_conf();
    if(gpio16_input_get() != gpio_value)
    {
        gpio_test_error_msg(0, 16, gpio_value);    
        error = 1;
    }

    return error;
}

/**
  * @brief  Execution commad of gpio test
  * @param  id: commad id number
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_exeCmdGpioTest(uint8_t id)
{
    int error = 0;

    //key_stop();

/*gpio test low --16 */
    error |= gpio16_input_check(0);
    
/*gpio test 0 --4 */
    error |= gpio_check_pin(0, 4);
    error |= gpio_check_pin(4, 0);
    
/*gpio test 2 --5 */
    error |= gpio_check_pin(2, 5);
    error |= gpio_check_pin(5, 2);
    
/*gpio test 12 --14 */
    error |= gpio_check_pin(12, 14);
    error |= gpio_check_pin(14, 12); 
    
/*gpio test 13 --15 */
    error |= gpio_check_pin(13, 15);
    error |= gpio_check_pin(15, 13);
    
    if(error == 0)
        at_backOk;
    else
        at_backError;
}

/**
  * @brief  Execution commad of gpio test
  * @param  id: commad id number
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_exeCmdGpioTest2(uint8_t id)
{
    int error = 0;

    //key_stop();

/*gpio test low --16 */
    error |= gpio16_input_check(0);
    
/*gpio test 0 --4 */
    error |= gpio_check_pin(0, 4);
    error |= gpio_check_pin(4, 0);
    
/*gpio test 2 --5 */
    error |= gpio_check_pin(2, 5);
    error |= gpio_check_pin(5, 2);
    
/*gpio test 12 --14 */
//    error |= gpio_check_pin(12, 14);
//    error |= gpio_check_pin(14, 12); 
    
/*gpio test 13 --15 */
    error |= gpio_check_pin(13, 15);
    error |= gpio_check_pin(15, 13);
    
    if(error == 0)
        at_backOk;
    else
        at_backError;
}

/**
  * @brief  Execution commad of gpio test
  * @param  id: commad id number
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_exeCmdAdc(uint8_t id)
{
    char temp[64] = {0};
    
    os_sprintf(temp,"%d", system_adc_read());
    uart0_sendStr(temp);
    at_backOk;
}

/**
  * @brief  Execution commad of gpio test
  * @param  id: commad id number
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_exeCmdPto(uint8_t id)
{
    char temp[64] = {0};
    
    os_sprintf(temp,"%08x", system_get_chip_id());
    uart0_sendStr(temp);
    at_backOk;
}

void ICACHE_FLASH_ATTR
at_queryCmdSwitch(uint8_t id)
{
    char temp[32] = {0};
    char buffer[32] = {0};
#if 0//PROFILE_SMARTPLUG
    os_sprintf(temp, "%s:", at_fun[id].at_cmdName);
    uart0_sendStr(temp);
    
    os_memset(buffer, 0, sizeof(buffer));
    eSmartPlugGetSwitchStatus(NULL, buffer, sizeof(buffer));
    os_sprintf(temp, "\"%s\"\r\n", buffer);
    uart0_sendStr(temp);
#endif    
    at_backOk;
}

void ICACHE_FLASH_ATTR
at_setupCmdSwitch(uint8_t id, char *pPara)
{
     int8_t len;
    char temp[64];
    
      pPara++;

    len = at_dataStrCpy(temp, pPara, DEVICEINFO_STRING_LEN);
    if(len == -1)
    {
      at_backError;
      return;
    }
#if 0//PROFILE_SMARTPLUG
    smartplug_set_switch_cb(temp);

    //eSmartPlugSetSwitchInternal(NULL, temp);
#endif
      at_backOk;

}
/**
  * @}
  */
void ICACHE_FLASH_ATTR
at_queryCmdDebugTest(uint8_t id)
{
    char temp[256] = {0};
    char buffer[256] = {0};
    
    os_sprintf(temp, "%s:", at_fun[id].at_cmdName);
    uart0_sendStr(temp);
#if 0//PROFILE_SMARTPLUG
    
    os_memset(buffer, 0, sizeof(buffer));
    eSmartPlugGetTimer(NULL, buffer, sizeof(buffer));
    os_sprintf(temp, "\"%s\"\r\n", buffer);
    uart0_sendStr(temp);
    os_memset(buffer, 0, sizeof(buffer));
    eSmartPlugGetWelcome(NULL, buffer, sizeof(buffer));
    os_sprintf(temp, "\"%s\"\r\n", buffer);
    uart0_sendStr(temp);
#endif    
    at_backOk;
}

void ICACHE_FLASH_ATTR
at_setupCmdDebugTest(uint8_t id, char *pPara)
{
	int8_t len;
  char temp[128];
  
	pPara++;
	
  len = at_dataStrCpy(temp, pPara, 120);
  if(len == -1)
  {
    at_backError;
    return;
  }
#if 0//PROFILE_SMARTPLUG
  
  log_debug("%s\r\n", pPara);
#if 1
  os_strcpy(temp, "{\"type\":\"w\",\"on\": \"d-1;w21600\",\"off\":\"d30;w64800\",\"weekday\":\"-1;1234567\",\"enable\":\"1;1\"}");
  log_debug("%s\r\n", temp);
  eSmartPlugSetTimer(NULL, temp);

  os_memset(temp, 0, sizeof(temp));
  os_strcpy(temp, "{\"mode\": \"1\", \"mac\":\"0c:1d:af:da:f8:0a\"}");
  welcome_path_handle(pPara);

#else
//  os_strcpy(temp, "{\"mode\": \"1\", \"mac\":\"0c:1d:af:da:f8:0a\"}");
    welcome_path_handle(pPara);
#endif
#endif
	at_backOk;
}


/*
 * File	: at_cmd.h
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
#ifndef __AT_CMD_H
#define __AT_CMD_H

#include "atcmd/at.h"
//#include "at_wifiCmd.h"
//#include "at_ipCmd.h"
#include "atcmd/at_baseCmd.h"
#include "atcmd/at_fiCmd.h"

#define at_cmdNum   sizeof(at_fun)/sizeof(at_funcationType)

at_funcationType at_fun[]={
  {NULL, 0, NULL, NULL, NULL, at_exeCmdNull},
  {"E", 1, NULL, NULL, at_setupCmdE, NULL},
  {"+RST", 4, NULL, NULL, NULL, at_exeCmdRst},
  {"+GMR", 4, NULL, NULL, NULL, at_exeCmdGmr},
  {"+SV", 3, NULL, NULL, NULL, at_exeCmdSoftVersion},
  {"+RESET", 6, NULL, NULL, NULL, at_exeCmdFactoryReset},
  {"+GPIOTEST", 9, NULL, NULL, NULL, at_exeCmdGpioTest},
  {"+GPIOTESTS", 10, NULL, NULL, NULL, at_exeCmdGpioTest2},
  {"+ADC", 4, NULL, NULL, NULL, at_exeCmdAdc},
  {"+PTO", 4, NULL, NULL, NULL, at_exeCmdPto},
  {"+LOGOFF", 7, NULL, NULL, NULL, at_exeCmdLogOff},
  {"+ERASEF", 7, NULL, NULL, NULL, at_exeCmdEraseF},
#if 0
  {"+GSLP", 5, NULL, NULL, at_setupCmdGslp, NULL},
  {"+IPR", 4, NULL, NULL, at_setupCmdIpr, NULL},
#ifdef ali
  {"+UPDATE", 7, NULL, NULL, NULL, at_exeCmdUpdate},
#endif
  {"+CWMODE", 7, at_testCmdCwmode, at_queryCmdCwmode, at_setupCmdCwmode, NULL},
  {"+CWJAP", 6, NULL, at_queryCmdCwjap, at_setupCmdCwjap, NULL},
  {"+CWLAP", 6, NULL, NULL, at_setupCmdCwlap, at_exeCmdCwlap},
  {"+CWQAP", 6, at_testCmdCwqap, NULL, NULL, at_exeCmdCwqap},
  {"+CWSAP", 6, NULL, at_queryCmdCwsap, at_setupCmdCwsap, NULL},
  {"+CWLIF", 6, NULL, NULL, NULL, at_exeCmdCwlif},
  {"+CWDHCP", 7, NULL, at_queryCmdCwdhcp, at_setupCmdCwdhcp, NULL},
  {"+CIFSR", 6, at_testCmdCifsr, NULL, at_setupCmdCifsr, at_exeCmdCifsr},
  {"+CIPSTAMAC", 10, NULL, at_queryCmdCipstamac, at_setupCmdCipstamac, NULL},
  {"+CIPAPMAC", 9, NULL, at_queryCmdCipapmac, at_setupCmdCipapmac, NULL},
  {"+CIPSTA", 7, NULL, at_queryCmdCipsta, at_setupCmdCipsta, NULL},
  {"+CIPAP", 6, NULL, at_queryCmdCipap, at_setupCmdCipap, NULL},
  {"+CIPSTATUS", 10, at_testCmdCipstatus, NULL, NULL, at_exeCmdCipstatus},
  {"+CIPSTART", 9, at_testCmdCipstart, NULL, at_setupCmdCipstart, NULL},
  {"+CIPCLOSE", 9, at_testCmdCipclose, NULL, at_setupCmdCipclose, at_exeCmdCipclose},
  {"+CIPSEND", 8, at_testCmdCipsend, NULL, at_setupCmdCipsend, at_exeCmdCipsend},
  {"+CIPMUX", 7, NULL, at_queryCmdCipmux, at_setupCmdCipmux, NULL},
  {"+CIPSERVER", 10, NULL, NULL,at_setupCmdCipserver, NULL},
  {"+CIPMODE", 8, NULL, at_queryCmdCipmode, at_setupCmdCipmode, NULL},
  {"+CIPSTO", 7, NULL, at_queryCmdCipsto, at_setupCmdCipsto, NULL},
  {"+CIUPDATE", 9, NULL, NULL, NULL, at_exeCmdCiupdate},
  {"+CIPING", 7, NULL, NULL, NULL, at_exeCmdCiping},
  {"+CIPAPPUP", 9, NULL, NULL, NULL, at_exeCmdCipappup},
#endif
  {"+CFSTAMAC", 9, NULL, at_queryCmdCfstamac, at_setupCmdCfstamac, NULL},
  {"+CFDEVICETYPE", 13, NULL, at_queryCmdCfdevicetype, at_setupCmdCfdevicetype, NULL},
  {"+CFDEVICEID", 11, NULL, at_queryCmdCfdeviceid, at_setupCmdCfdeviceid, NULL},
  {"+CFPASSWORD", 11, NULL, at_queryCmdCfpassword, at_setupCmdCfpassword, NULL},
  {"+CFXMPPSERVER", 13, NULL, at_queryCmdCfxmppserver, at_setupCmdCfxmppserver, NULL},
  {"+CFXMPPJID", 10, NULL, at_queryCmdCfxmppjid, at_setupCmdCfxmppjid, NULL},
  {"+CFXMPPPWD", 10, NULL, at_queryCmdCfxmpppwd, at_setupCmdCfxmpppwd, NULL},
  {"+CFPTOINFO", 10, NULL, at_queryCmdCfptoinfo, at_setupCmdCfptoinfo, NULL},
  {"+CFDEVICEINFO", 13, NULL, at_queryCmdCfdeviceinfo, at_setupCmdCfdeviceinfo, NULL},
  {"+DBGTST", 7, NULL, at_queryCmdDebugTest, at_setupCmdDebugTest, NULL},
  {"+SWITCH", 7, NULL, at_queryCmdSwitch, at_setupCmdSwitch, NULL},
#ifdef ali
  {"+MPINFO", 7, NULL, NULL, at_setupCmdMpinfo, NULL}
#endif
};

#endif

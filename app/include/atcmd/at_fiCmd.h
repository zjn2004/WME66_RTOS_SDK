/*
 * File	: at_fiCmd.h
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
#ifndef __AT_FICMD_H
#define __AT_FICMD_H

extern at_funcationType at_fun[];

void at_queryCmdCfstamac(uint8_t id);
void at_setupCmdCfstamac(uint8_t id, char *pPara);
void at_queryCmdCfdevicetype(uint8_t id);
void at_setupCmdCfdevicetype(uint8_t id, char *pPara);
void at_queryCmdCfdeviceid(uint8_t id);
void at_setupCmdCfdeviceid(uint8_t id, char *pPara);
void at_queryCmdCfpassword(uint8_t id);
void at_setupCmdCfpassword(uint8_t id, char *pPara);
void at_queryCmdCfxmppserver(uint8_t id);
void at_setupCmdCfxmppserver(uint8_t id, char *pPara);
void at_queryCmdCfxmppjid(uint8_t id);
void at_setupCmdCfxmppjid(uint8_t id, char *pPara);
void at_queryCmdCfxmpppwd(uint8_t id);
void at_setupCmdCfxmpppwd(uint8_t id, char *pPara);
void at_queryCmdCfptoinfo(uint8_t id);
void at_setupCmdCfptoinfo(uint8_t id, char *pPara);
void at_queryCmdCfdeviceinfo(uint8_t id);
void at_setupCmdCfdeviceinfo(uint8_t id, char *pPara);
void at_exeCmdGpioTest(uint8_t id);
void at_exeCmdGpioTest2(uint8_t id);
void at_exeCmdAdc(uint8_t id);
void at_exeCmdPto(uint8_t id);
void at_queryCmdDebugTest(uint8_t id);
void at_setupCmdDebugTest(uint8_t id, char *pPara);
void at_queryCmdSwitch(uint8_t id);
void at_setupCmdSwitch(uint8_t id, char *pPara);
void at_setupCmdBreathMode(uint8_t id, char *pPara);
void at_setupCmdFlashMode(uint8_t id,char * pPara);
void at_setupCmdLightSense(uint8_t id,char * pPara);
void at_setupCmdQuickSleep(uint8_t id,char * pPara);
void at_setupCmdNightMode(uint8_t id,char * pPara);
void at_queryCmdNightMode(uint8_t id);
void at_queryCmdElect(uint8_t id);
void at_queryCmdSense(uint8_t id);
void at_queryCmdKeyMode(uint8_t id);
void at_setupCmdKeyMode(uint8_t id,char * pPara);

#endif

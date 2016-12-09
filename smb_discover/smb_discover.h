/*******************************************************************************
	Copyright (C), 2013-2016, TP-LINK TECHNOLOGIES CO., LTD.
	File name:    smb_discover.h
	Version:      1.0
	Description:  
	    Global configuration and tools for smb_discover utility.

	Author:		Andrew Chang
	Create Date: 	2016-11-15
--------------------------------------------------------------
	History: 
		2016-11-15: File created as "smb_discover.h".

--------------------------------------------------------------
	GPL declaration:

	     This program is free software: you can redistribute it and/or modify it 
	 under the terms of the GNU General Public License as published by the Free 
	 Software Foundation, either version 3 of the License, or any later version.
	     This program is distributed in the hope that it will be useful, but WITH-
	 OUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
	 FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
	 more details. 
	     You should have received a copy of the GNU General Public License along
	 with this program. If not, see <http://www.gnu.org/licenses/>.

********************************************************************************/

#ifndef __SMB_DISCOVER_H__
#define __SMB_DISCOVER_H__

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <utmp.h>

#include "cJSON.h"


#ifndef BOOL
#define BOOL	int
#define FALSE	(0)
#define TRUE	(!FALSE)
#endif

#ifndef NULL
#define NULL	((void *)0)
#endif

#define cJSON_IsNumber(pObj)		(cJSON_Number == ((pObj)->type))
#define cJSON_IsString(pObj)		(cJSON_String == ((pObj)->type))
#define cJSON_IsBool(pObj)			((cJSON_True == ((pObj)->type)) || (cJSON_False == ((pObj)->type)))
#define cJSON_IsArray(pObj)			(cJSON_Array == ((pObj)->type))


#define IPv4_STR_LEN_MAX	15	/* sizeof("255.255.255.255") - 1 */
#define SMB_USER_LEN_MAX	UT_NAMESIZE
#define SMB_PASS_LEN_MAX	255
#define DIR_NAME_LEN_MAX	NAME_MAX


#ifdef SMB_DISCO_DEBUG
#define SMBD_DEBUG(fmt, args...)		printf("--- SMBD ("__FILE__", %d): "fmt"\n", __LINE__, ##args)
#define SMBD_INFO(fmt, args...)			SMBD_DEBUG(fmt, ##args)
#define SMBD_ERROR(fmt, args...)		printf("ERR SMBD ("__FILE__", %d): "fmt"\n", __LINE__, ##args)
#else
#define SMBD_DEBUG(fmt, args...)
#define SMBD_INFO(fmt, args...)			printf("[%03d] smb_disco: "fmt"\n", __LINE__, ##args)
#define SMBD_ERROR(fmt, args...)		fprintf(stderr, "[%03d] smb_disco: "fmt"\n", __LINE__, ##args)
#endif


/********/
/* type declarations */

typedef struct _APP_CONTEXT {
	char               smb_ipv4_url[IPv4_STR_LEN_MAX + 6 + 1];	/* append prefix "smb://" */
	char               smb_username[SMB_USER_LEN_MAX + 1];
	char               smb_password[SMB_PASS_LEN_MAX + 1];
	char               smb_srv_path[DIR_NAME_LEN_MAX + 1];
	int                smb_timeout;
	size_t             smb_max_dir_count;
	BOOL               should_daemonize;			/* will only daemonize after reporting self */
	BOOL               is_list_workgroup_mode;
	BOOL               is_list_computer_mode;
	cJSON             *smb_workgroups;
} SmbDiscoverContext_st;


#endif	/* EOF */


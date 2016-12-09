/*******************************************************************************
	Copyright (C), 2013-2016, TP-LINK TECHNOLOGIES CO., LTD.
	File name:    util_nmblookup.c
	Version:      1.0
	Description:  
	    nmblookup utility.

	Author:		Andrew Chang
	Create Date: 	2016-12-01
--------------------------------------------------------------
	History: 
		2016-12-01: File created as "util_nmblookup.c".

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

/********/
/* self headers */
#include "util_nmblookup.h"

#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/********
 * SMB headers
 */
#include <stdint.h>
#define HAVE_BLKSIZE_T	1
#define HAVE_BLKCNT_T	1
#define blksize_t	long
#define blkcnt_t	long
#define __ERROR__XX__DONT_USE_FALSE	(0)
#define __ERROR__XX__DONT_USE_TRUE	(!FALSE)
#include <includes.h>
#include <nameserv.h>


/********/
/* internal libsmb operations */

#ifdef SMB_DISCO_DEBUG
static char *_query_flags(int flags)
{
	static fstring ret1;
	fstrcpy(ret1, "");

	if (flags & NM_FLAGS_RS) fstrcat(ret1, "Response ");
	if (flags & NM_FLAGS_AA) fstrcat(ret1, "Authoritative ");
	if (flags & NM_FLAGS_TC) fstrcat(ret1, "Truncated ");
	if (flags & NM_FLAGS_RD) fstrcat(ret1, "Recursion_Desired ");
	if (flags & NM_FLAGS_RA) fstrcat(ret1, "Recursion_Available ");
	if (flags & NM_FLAGS_B)  fstrcat(ret1, "Broadcast ");

	return ret1;
}
#endif

static int _list_IP_in_workgroup(cJSON *jsonArray)
{
	int ret = 0;	/* only returns -1 when system error occurred */
	int tmp, ifCount;
	int smbIpCount = 0;
	uint8_t flags = 0;
	const char *workgroupName = jsonArray->string;
	
	const struct in_addr *broadcast = NULL;
	NTSTATUS status = NT_STATUS_NOT_FOUND;
	struct sockaddr_storage *ipList = NULL;

	//SMBD_DEBUG("Try to list %s", workgroupName);

	/* search for servers */
	ifCount = iface_count();
	for (tmp = 0; 
		(NULL == ipList) && (tmp < ifCount);
		tmp ++)
	{
		struct sockaddr_storage bcast_ss;
	
		broadcast = iface_n_bcast_v4(tmp);
		in_addr_to_sockaddr_storage(&bcast_ss, *broadcast);
		if (broadcast) {
 			status = name_query(workgroupName, 0, TRUE, TRUE,
							&bcast_ss, talloc_tos(),
							&ipList, &smbIpCount, &flags);

			SMBD_DEBUG("%d server(s) found in workgroup \"%s\"", smbIpCount, workgroupName);
			SMBD_DEBUG("Flags: %s", _query_flags(flags));		/* Please refer to nameserv.h, macro prefix "NM_FLAGS_..." */
		}
	}

	/* listing servers */
	if (NT_STATUS_IS_OK(status) && smbIpCount) {
		char addr[INET6_ADDRSTRLEN] = "";

		for (tmp = 0; tmp < smbIpCount; tmp ++)
		{
			print_sockaddr(addr, sizeof(addr), &ipList[tmp]);
			cJSON_AddItemToArray(jsonArray, cJSON_CreateString(addr));
		}
	}

	/* ends */
	if (ipList) {
		TALLOC_FREE(ipList);
		ipList = NULL;
	}
	return ret;
}


/********/
/* smbtree invoke */
cJSON *util_nmblookup(SmbDiscoverContext_st *pAppConf)
{
	cJSON *response = NULL;
	cJSON *retNameObject = NULL;
	BOOL isSystemError = FALSE;
	TALLOC_CTX *smbFrame = NULL;

	/* prepare cJSON */
	if (1) {
		response = cJSON_CreateObject();
		if (NULL == response) {
			int errCopy = errno;
			SMBD_ERROR("Failed to allocate JSON object: %s", strerror(errCopy));
			goto ENDS;
		}
		else {
			cJSON_AddStringToObject(response, "command", "nmblookup");
		}
	}

	/* query required workgroups */
	if (1) {
		cJSON *allNames = pAppConf->smb_workgroups;
		if (NULL == allNames) {
			SMBD_ERROR("No legal SMB workgroup specified");
			goto ENDS;
		}

		retNameObject = cJSON_CreateObject();
		if (NULL == retNameObject) {
			int errCopy = errno;
			SMBD_ERROR("Failed to allocate name object: %s", strerror(errCopy));
			goto ENDS;
		}
		else {
			cJSON_AddItemToObject(response, "computers", retNameObject);
		}

		cJSON *eachName = allNames->child;
		while(eachName)
		{
			cJSON_AddItemToObject(retNameObject, eachName->valuestring, cJSON_CreateArray());
			eachName = eachName->next;
		}
		
	}

	/* prepare SMB libs */
	if (1) {
		smbFrame = talloc_stackframe();

		if (smbFrame) {
			lp_load(get_dyn_CONFIGFILE(), TRUE, FALSE, FALSE, TRUE);
			load_interfaces();
		}
		else {
			SMBD_ERROR("Failed to prepare SMB enviroments");
			goto ENDS;
		}
	}

	/* search all IP addresses under corresponding workgroup */
	if (1) {		
		cJSON *jsonWorkgroup = retNameObject->child;
		while(jsonWorkgroup)
		{
			if (cJSON_Array == jsonWorkgroup->type) {
				_list_IP_in_workgroup(jsonWorkgroup);
			}

			jsonWorkgroup = jsonWorkgroup->next;
		}
		
	}

	/* done */
	SMBD_DEBUG("%s() done", __func__);

ENDS:
	/* return */
	if (response)
	{
		/* return response by status */
		if (isSystemError) {
			cJSON_AddStringToObject(response, "fail", "system error");
		}
		else {
			/* OK and informations already been added */
		}
	}

	if (smbFrame){
		TALLOC_FREE(smbFrame);
		smbFrame = NULL;
	}

	return response;
}


int util_nmblookup_check_config(SmbDiscoverContext_st *pAppConf)
{
	if (NULL == pAppConf->smb_workgroups) {
		SMBD_ERROR("Please specify at least one workgroup.");
		return -1;
	}
	return 0;
}


/* EOF */


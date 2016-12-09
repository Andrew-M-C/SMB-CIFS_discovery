/*******************************************************************************
	Copyright (C), 2013-2016, TP-LINK TECHNOLOGIES CO., LTD.
	File name:    util_smbclient.c
	Version:      1.0
	Description:  
	    SMB client read utility.

	Author:		Andrew Chang
	Create Date: 	2016-11-29
--------------------------------------------------------------
	History: 
		2016-11-29: File created as "util_smbclient.c".

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
#include "util_smbtree.h"

#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/********
 * SMB headers
 *     in order to reference libsmb.h, I have to define many data 
 * structures and include additional headers.
 *     The only header I need is just libsmb.h. You may comment out
 * each line below and see what happen when building the program.
 */

/* === special === */
/* here are texts from "winbind_nss_config.h" */
/* I'm trying really hard not to include anything from smb.h with the
   result of some silly looking redeclaration of structures. */

#ifndef FSTRING_LEN
#define FSTRING_LEN 256
typedef char fstring[FSTRING_LEN];
#define fstrcpy(d,s) safe_strcpy((d),(s),sizeof(fstring)-1)
#endif
/* === special ends === */

#include <stdint.h>
#define HAVE_BLKSIZE_T	1
#define HAVE_BLKCNT_T	1
#define blksize_t	long
#define blkcnt_t	long
#define __ERROR__XX__DONT_USE_FALSE	(0)
#define __ERROR__XX__DONT_USE_TRUE	(!FALSE)
#include <includes.h>
#include <libsmb/libsmb.h>
#include <popt_common.h>
#include <libsmb/clirap.h>
#include <librpc/gen_ndr/svcctl.h>


/********/
/* internal libsmb operations */

static char *g_localWorkgroup = "WORKGROUP";

static inline const char *_local_workgroup(void)
{
	return g_localWorkgroup;
}

static void _each_workgroup_callback(
						const char *workgroup,
						uint32 serverType,
						const char *comment,
						void *arg)
{
	cJSON *jsonWorkgroups = (cJSON *)arg;
	cJSON_AddItemToArray(jsonWorkgroups, cJSON_CreateString(workgroup));
	SMBD_DEBUG("%s servertype: %d", workgroup, serverType);		/* Please refer to svcctl.h, macro prefix "SV_TYPE_..." */

	return;
}


/********/
/* smbtree invoke */
cJSON *util_smbtree(SmbDiscoverContext_st *pAppConf)
{
	cJSON *response = NULL;
	BOOL isConnectMasterBrowserFailed = FALSE;
	BOOL isSystemError = FALSE;
	char *masterWorkgfroup = NULL;
	TALLOC_CTX *smbFrame = NULL;
	TALLOC_CTX *smbCtx = NULL;
	struct cli_state *smbCli = NULL;
	struct user_auth_info *smbUserInfo = NULL;

	/* prepare cJSON */
	if (1) {
		response = cJSON_CreateObject();
		if (NULL == response) {
			int errCopy = errno;
			SMBD_ERROR("Failed to allocate JSON object: %s", strerror(errCopy));
			goto ENDS;
		}
		else {
			cJSON_AddStringToObject(response, "command", "smbtree");
		}
	}

	/* prepare SMB libs */
	if (1) {
		smbFrame = talloc_stackframe();
		smbUserInfo = user_auth_info_init(smbFrame);
		smbCtx = talloc_tos();
		masterWorkgfroup = talloc_strdup(smbCtx, _local_workgroup());

		if (smbFrame && smbUserInfo && smbCtx && masterWorkgfroup) {
			SMBD_DEBUG("Get master workgroup: %s", masterWorkgfroup);
		}
		else {
			SMBD_ERROR("Failed to prepare SMB enviroments");
			goto ENDS;
		}
	}

	/* init auth info */
	if (1) {
		//load_case_tables();
		lp_load(get_dyn_CONFIGFILE(), TRUE, FALSE, FALSE, TRUE);
		load_interfaces();
		//popt_common_set_auth_info(smbUserInfo);
		
		smbUserInfo->username      = "root";
		smbUserInfo->domain        = NULL;
		smbUserInfo->password      = "";
		smbUserInfo->got_pass      = TRUE;
		smbUserInfo->use_kerberos  = FALSE;
		smbUserInfo->signing_state = -1;
		smbUserInfo->smb_encrypt   = FALSE;
		smbUserInfo->use_machine_account     = FALSE;
		smbUserInfo->fallback_after_kerberos = FALSE;
		smbUserInfo->use_ccache    = FALSE;
		
		//set_cmdline_auth_info_getpass(smbUserInfo);
	}

	/* search for master browser */
	if (1) {
		smbCli = get_ipc_connect_master_ip_bcast(talloc_tos(),
							smbUserInfo,
							&masterWorkgfroup);
		if (NULL == smbCli) {
			isConnectMasterBrowserFailed = TRUE;
			SMBD_ERROR("Failed to connect to master browser by broadcast");
		}
		else {
			SMBD_DEBUG("Domain: %s", smbCli->domain);
			SMBD_DEBUG("desthost: %s", smbCli->desthost);
			SMBD_DEBUG("user_name: %s", smbCli->user_name);
			SMBD_DEBUG("password: %s", smbCli->password);
			SMBD_DEBUG("server_type: %s", smbCli->server_type);
			SMBD_DEBUG("server_os: %s", smbCli->server_os);
			SMBD_DEBUG("server_domain: %s", smbCli->server_domain);
			SMBD_DEBUG("share: %s", smbCli->share);
			SMBD_DEBUG("dev: %s", smbCli->dev);

			struct sockaddr_in *sockAddr = (struct sockaddr_in *)(&(smbCli->dest_ss));
			char serverIP[32] = "";
			inet_ntop(AF_INET, &(sockAddr->sin_addr), serverIP, sizeof(serverIP));
			SMBD_DEBUG("Master browser IP: %s", serverIP);

			cJSON_AddStringToObject(response, "master browser", serverIP);
		}
	}

	/* try to enum workgroups */
	if (1) {
		cJSON *jsonWorkgroups = cJSON_CreateArray();
		if (NULL == jsonWorkgroups) {
			isSystemError = TRUE;
			goto ENDS;
		}

		cJSON_AddItemToObject(response, "workgroups", jsonWorkgroups);

		if (smbCli) {
			BOOL invokeOK = cli_NetServerEnum(
										smbCli,
										masterWorkgfroup,
										SV_TYPE_DOMAIN_ENUM,
										_each_workgroup_callback,
										jsonWorkgroups);
			if (FALSE == invokeOK) {
				SMBD_DEBUG("Failed to enumate workgroups");
			}
		}
	}

	/* done */
	SMBD_DEBUG("%s() done", __func__);

ENDS:
	/* return */
	if (response)
	{
		/* return response by status */
		if (isConnectMasterBrowserFailed) {
			cJSON_AddStringToObject(response, "fail", "no master browser");
		}
		else if (isSystemError) {
			cJSON_AddStringToObject(response, "fail", "system error");
		}
		else {
			/* OK and informations already been added */
		}
	}

	if (masterWorkgfroup) {}
	if (smbCtx) {}
	if (smbCli) {}
	if (smbUserInfo) {}

	if (smbFrame){
		TALLOC_FREE(smbFrame);
		smbFrame = NULL;
	}

	return response;
}


int util_smbtree_check_config(SmbDiscoverContext_st *pAppConf)
{
	return 0;
}


/* EOF */


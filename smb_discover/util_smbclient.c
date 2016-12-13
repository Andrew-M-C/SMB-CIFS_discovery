/*******************************************************************************
	
	File name:    util_smbclient.c
	Version:      1.0
	Description:  
	    SMB client read utility.

	Author:		Andrew Chang
	Create Date: 	2016-11-29
--------------------------------------------------------------
	History: 
		2016-11-29: File created as "util_smbclient.c".
		2016-12-13: Rewritten to use internal samba object files instead of libsmbclient.

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

#include "util_smbclient.h"

#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <stdint.h>

/********
 * SMB headers
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

#ifndef bool
#define bool	BOOL
#endif

#ifndef uint16
#define uint16	uint16_t
#endif

#ifndef uint32
#define uint32	uint32_t
#endif

#ifndef uint64
#define uint64	uint64_t
#endif

#define HAVE_BLKSIZE_T	1
#define HAVE_BLKCNT_T	1
#define blksize_t	long
#define blkcnt_t	long
#define __ERROR__XX__DONT_USE_FALSE	(0)
#define __ERROR__XX__DONT_USE_TRUE	(!FALSE)
#include <includes.h>
#include <smb.h>
#include <client.h>
//#include <werror.h>
#include <gen_ndr/srvsvc.h>
#include <gen_ndr/ndr_srvsvc_c.h>
//#include <ndr/libndr.h>
//#include <cli_pipe.h>
#include <popt_common.h>
#include <libads/ads_status.h>
#include <libsmb/proto.h>
#include <rpc_client/rpc_client.h>
#include <rpc_client/cli_pipe.h>
#include <rpc_dce.h>

#ifndef NDR_SRVSVC_VERSION
#define NDR_SRVSVC_VERSION 3.0
#endif

/********/
/* static variables */
static SmbDiscoverContext_st *g_currentAppConfig = NULL;

static struct ndr_syntax_id g_syntax_id = {
	{0x4b324fc8,0x1670,0x01d3,{0x12,0x78},{0x5a,0x47,0xbf,0x6e,0xe1,0x88}},
	NDR_SRVSVC_VERSION
};

/********/
/* internal functions */

static int _smbc_browse_dir(struct cli_state *cli, cJSON *responseArray, size_t maxDirCount)
{
	int ret = -1;
	NTSTATUS status;
	struct rpc_pipe_client *pipe_hnd = NULL;
	TALLOC_CTX *frame = talloc_stackframe();
	WERROR werr;
	struct srvsvc_NetShareInfoCtr info_ctr;
	struct srvsvc_NetShareCtr1 ctr1;
	uint32_t resume_handle = 0;
	uint32_t total_entries = 0;
	struct dcerpc_binding_handle *b;
	size_t tmp;
	size_t dirCount = 0;

	status = cli_rpc_pipe_open_noauth(cli, &g_syntax_id, &pipe_hnd);
	if (FALSE == NT_STATUS_IS_OK(status)) {SMBD_ERROR("MARK");
		SMBD_ERROR("Could not connect to srvsvc pipe: %s\n", nt_errstr(status));
		goto ENDS;
	}

	b = pipe_hnd->binding_handle;
	
	ZERO_STRUCT(info_ctr);
	ZERO_STRUCT(ctr1);
	
	info_ctr.level = 1;
	info_ctr.ctr.ctr1 = &ctr1;

	status = dcerpc_srvsvc_NetShareEnumAll(b, frame,
					      pipe_hnd->desthost,
					      &info_ctr,
					      0xffffffff,
					      &total_entries,
					      &resume_handle,
					      &werr);
	if ((FALSE == NT_STATUS_IS_OK(status))
		|| (FALSE == W_ERROR_IS_OK(werr))) 
	{
		SMBD_ERROR("Failed in dcerpc_srvsvc_NetShareEnumAll()");
		goto ENDS;
	}

	SMBD_DEBUG("maxDirCount = %d", maxDirCount);
	/* print directories */
	for (tmp = 0; 
		(tmp < info_ctr.ctr.ctr1->count) && (dirCount < maxDirCount); 
		tmp ++)
	{
		BOOL shouldShowDir = FALSE;
		struct srvsvc_NetShareInfo1 dirInfo = info_ctr.ctr.ctr1->array[tmp];

		/* check type */
		switch(dirInfo.type & 7)
		{
		case STYPE_DISKTREE:
			shouldShowDir= TRUE;
			break;
		case STYPE_DISKTREE_TEMPORARY:
		case STYPE_DISKTREE_HIDDEN:
		case STYPE_PRINTQ:
		case STYPE_PRINTQ_TEMPORARY:
		case STYPE_PRINTQ_HIDDEN:
		case STYPE_DEVICE:
		case STYPE_DEVICE_TEMPORARY:
		case STYPE_DEVICE_HIDDEN:
		case STYPE_IPC:
		case STYPE_IPC_TEMPORARY:
		case STYPE_IPC_HIDDEN:
		default:
			SMBD_DEBUG("Ignore %s, type: 0x%08X", dirInfo.name, dirInfo.type);
			shouldShowDir = FALSE;
			break;
		}

		/* check volume name */
		if (shouldShowDir)
		{
			if (dirInfo.name && dirInfo.name[0]) {
				size_t len = strlen(dirInfo.name);
				if ('$' == dirInfo.name[len - 1]) {
					SMBD_DEBUG("Ignore %s, hidden", dirInfo.name);
					shouldShowDir = FALSE;
				}
				else {
					/* OK, keep it */
				}
			}
			else {
				shouldShowDir = FALSE;
			}
		}

		/* add directory */
		if (shouldShowDir) {
			cJSON *dirObj = cJSON_CreateObject();
			if (dirObj)
			{
				dirCount ++;
				cJSON_AddItemToArray(responseArray, dirObj);
				
				cJSON_AddStringToObject(dirObj, "name", dirInfo.name);
				
				if (dirInfo.comment && dirInfo.comment[0]) {
					cJSON_AddStringToObject(dirObj, "comment", dirInfo.comment);
				}
			}
			/* end of "if (dirObj) ..." */
		}
	}

	/* OK */
	ret = 0;
ENDS:
	/* ends */
	if (pipe_hnd) {
		TALLOC_FREE(pipe_hnd);
		pipe_hnd = NULL;
	}

	if (frame) {
		TALLOC_FREE(frame);
		frame = NULL;
	}
	return 0;
}


/********/
/* libsmbclient invoke */
cJSON *util_smbclient(SmbDiscoverContext_st *pAppConf)
{
	cJSON *response = NULL;
	BOOL isTimeout = FALSE;
	BOOL isAuthFail = FALSE;
	BOOL isSysErr = FALSE;
	BOOL isSvcNotEnabled = FALSE;
	
	TALLOC_CTX *smbFrame = NULL;
	struct cli_state *cli = NULL;
	struct user_auth_info *smbUserInfo = NULL;

	g_currentAppConfig = pAppConf;

	/* allocate response */
	if (1) {
		response = cJSON_CreateObject();
		if (NULL == response) {
			int errCopy = errno;
			SMBD_ERROR("Failed to allocate JSON object: %s", strerror(errCopy));
			isSysErr = TRUE;
			goto ENDS;
		}
		else {
			cJSON_AddStringToObject(response, "command", "smbclient");
		}
	}

	/* prepare SMB libs */
	if (1) {
		smbFrame = talloc_stackframe();
		if (smbFrame) {
			smbUserInfo = user_auth_info_init(smbFrame);
		}
		else {
			SMBD_ERROR("Failed to prepare SMB frame");
			isSysErr = TRUE;
		}

		if (smbUserInfo) {
			lp_load(get_dyn_CONFIGFILE(), TRUE, FALSE, FALSE, TRUE);
			load_interfaces();
			set_global_myworkgroup("WORKGROUP");

			set_cmdline_auth_info_username(smbUserInfo, pAppConf->smb_username);
			set_cmdline_auth_info_password(smbUserInfo, pAppConf->smb_password);
			set_cmdline_auth_info_getpass(smbUserInfo);
			set_cmdline_auth_info_use_kerberos(smbUserInfo, FALSE);
			set_cmdline_auth_info_signing_state(smbUserInfo, "auto");

			SMBD_DEBUG("Username: \"%s\"", get_cmdline_auth_info_username(smbUserInfo));
			SMBD_DEBUG("Password: \"%s\"", get_cmdline_auth_info_password(smbUserInfo));
		}
		else {
			SMBD_ERROR("Failed to prepare SMB info");
			isSysErr = TRUE;
		}
	}

	/* try to access SMB server */
	if (smbFrame) {
		struct timespec callStartTime, callEndTime;

		clock_gettime(CLOCK_MONOTONIC, &callStartTime);
		cli = cli_cm_open(talloc_tos(), 		// TALLOC_CTX
						NULL, 					// cli_state
						pAppConf->smb_ipv4_url, // server
						"IPC$", 				// share
						smbUserInfo, 			// auth info
						TRUE, 					// show hdr
						FALSE, 					// force encrypt
						PROTOCOL_NT1, 			// max protocol
						0, 						// port
						0x20);					// name type
		clock_gettime(CLOCK_MONOTONIC, &callEndTime);
		
		if (NULL == cli)
		{
			SMBD_ERROR("Failed to connect to remote target");
			struct timespec callDiffTime;
			long long milisecDiff;

			if (callEndTime.tv_nsec < callStartTime.tv_nsec) {
				callEndTime.tv_nsec += 1000000000;
				callEndTime.tv_sec  -= 1;
			}

			callDiffTime.tv_sec  = callEndTime.tv_sec  - callStartTime.tv_sec;
			callDiffTime.tv_nsec = callEndTime.tv_nsec - callStartTime.tv_nsec;
			milisecDiff = callDiffTime.tv_sec * 1000 + callDiffTime.tv_nsec / 1000000;

			if (milisecDiff >= 1000) {
				isTimeout = TRUE;
			}
			else {
				isAuthFail = TRUE;
			}
		}
	}

	if (cli) {
		cJSON *dirArray = cJSON_CreateArray();
		if (dirArray)
		{
			int listStat = 0;
			cJSON_AddItemToObject(response, "dirs", dirArray);
			listStat = _smbc_browse_dir(cli, dirArray, pAppConf->smb_max_dir_count);
			if (0 != listStat) {
				isSysErr = TRUE;
			}
		}
		else
		{
			SMBD_ERROR("Failed to allocate dir array: %s", strerror(errno));
			isSysErr = TRUE;
		}
		
	}

	/* IMPORTANT: pay attention to series if-else combinations below */
	/**/
	/* timeout? */
	if (response && isTimeout) {
		cJSON_AddStringToObject(response, "fail", "timeout");
	}
	/* auth failed? */
	else if (response && isAuthFail) {
		cJSON_AddStringToObject(response, "fail", "auth");
	}
	/* no service found? */
	else if (response && isSvcNotEnabled) {
		cJSON_AddStringToObject(response, "fail", "no svc");
	}
	/* system error? */
	else if (response && isSysErr) {
		cJSON_AddStringToObject(response, "fail", "system");
	}
	/* start inquiry SMB contents */
	else if (response) {
		/* data already been generated, nothing to do */
	}/* end of "else if (response && (smbcFd > 0)) { ..." */
	else
	{}

ENDS:
	/* ends */
	if (cli) {
		cli_shutdown(cli);
		cli = NULL;
	}
	
	if (smbFrame){
		TALLOC_FREE(smbFrame);
		smbFrame = NULL;
	}
	
	return response;
}


int util_smbclient_check_config(SmbDiscoverContext_st *pAppConf)
{
	if ('\0' == pAppConf->smb_ipv4_url[0]) {
		SMBD_ERROR("Please enter a valid IPv4 SMB server.");
		return -1;
	}

	if ('\0' == (pAppConf->smb_username)[0]) {
		SMBD_DEBUG("Use anonymous username.");
		(pAppConf->smb_password)[0] = '\0';
	}

	if (0 == pAppConf->smb_max_dir_count) {
		pAppConf->smb_max_dir_count = 50;
	}

	SMBD_DEBUG("Username: %s", pAppConf->smb_username);
	SMBD_DEBUG("Password: %s", pAppConf->smb_password);

	return 0;
}


/* EOF */


/*******************************************************************************
	Copyright (C), 2013-2016, TP-LINK TECHNOLOGIES CO., LTD.
	File name:    main.c
	Version:      1.0
	Description:  
	    Main program samba utility.

	Author:		Andrew Chang
	Create Date: 	2016-11-14
--------------------------------------------------------------
	History: 
		2016-11-14: File created as "main.c".

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

#include "smb_discover.h"
#include "util_smbclient.h"
#include "util_smbtree.h"
#include "util_nmblookup.h"
#include "cJSON.h"

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include <time.h>

#include <signal.h>

/********/
/* options:
 *   -A addr     SMB server's IPv4 address to search
 *   -K socket   invoker's socket address
 *   -U username SMB server's username
 *   -P password SMB server's password
 *   -T timeout  timeout duration in miliseconds
 *   -M MAX_DIRS maximum directory count
 *   -D          daemonize
 *   -G          scan for workgroups
 *   -L GROUPS   list computers in workgroup(s)
 */


/********/
/* signal handlers */

static void _signal_handler(int signum)
{
	switch(signum)
	{
	case SIGPIPE:
		SMBD_INFO("Ignore SIGPIPE");
		break;
	case SIGINT:
	case SIGTERM:
	default:
		SMBD_INFO("Got signal %d, quit.", signum);
		exit(1);
		break;
	}

	return;
}


/********/
/* internal functions */

static int _register_signals(void)
{
	signal(SIGPIPE, _signal_handler);
    signal(SIGINT, _signal_handler);
    signal(SIGTERM, _signal_handler);

	return 0;
}

static BOOL _check_str_is_IPv4_and_translate(const char *str, char *pIpStrOut, size_t ipStrBuffLen, BOOL withSmbPrefix)
{
	if (NULL == str) {
		return FALSE;
	}

	struct in_addr ipv4Int;
	ipv4Int.s_addr = inet_addr(str);
	char pureAddrStr[IPv4_STR_LEN_MAX + 1];

	if (INADDR_NONE == ipv4Int.s_addr) {
		return FALSE;
	}

	strncpy(pureAddrStr, inet_ntoa(ipv4Int), sizeof(pureAddrStr));

	if (withSmbPrefix) {
		snprintf(pIpStrOut, ipStrBuffLen, "smb://%s", pureAddrStr);
	}
	else {
		snprintf(pIpStrOut, ipStrBuffLen, "%s", pureAddrStr);
	}
	
	SMBD_DEBUG("Got SMB server address: %s", pIpStrOut);
	
	return TRUE;
}


static int _parse_workgroups(const char *arg, SmbDiscoverContext_st *pAppConf)
{
	size_t len = strlen(arg);
	if (0 == len) {
		SMBD_ERROR("workgroup list empty");
		return -1;
	}

	pAppConf->smb_workgroups = cJSON_CreateArray();
	if (NULL == pAppConf->smb_workgroups) {
		int errCopy = errno;
		SMBD_ERROR("Failed to create JSON buffer: %s", strerror(errCopy));
		return -1;
	}

	char *pStart, *pStop;
	char *argCopy = malloc(len + 1);
	if (NULL == argCopy) {
		int errCopy = errno;
		SMBD_ERROR("Failed to create buffer: %s", strerror(errCopy));
		return -1;
	}

	strcpy(argCopy, arg);	/* this is safe because string length is already been checked */
	pStart = argCopy;
	pStop = argCopy;

	while (*pStop)
	{
		size_t partLen = 0;
	
		/* search for ':' */
		while (*pStop && (':' != *pStop))
		{
			partLen ++;
			pStop ++;
		}

		/* copy this string */
		*pStop = '\0';
		if (strlen(pStart)) {
			cJSON_AddItemToArray(pAppConf->smb_workgroups, cJSON_CreateString(pStart));
			SMBD_DEBUG("Add workgroup: %s", pStart);
		}

		/* prepare for next string */
		pStop ++;
		pStart = pStop;
	}

	if (pStart != pStop) {
		cJSON_AddItemToArray(pAppConf->smb_workgroups, cJSON_CreateString(pStart));
		SMBD_DEBUG("Add workgroup: %s", pStart);
	}

	/* return */
	if (argCopy) {
		free(argCopy);
		argCopy = NULL;
	}
	return 0;
}


static int _parse_options(int argc, char *argv[], SmbDiscoverContext_st *pAppConf)
{
	int opt;
	BOOL parse_error = FALSE;

	while ((FALSE == parse_error)
			&& ((opt = getopt(argc, argv, ":DGA:P:U:K:T:M:L:")) != -1))
	{
		switch (opt)
		{
		case ':':
			SMBD_ERROR("Option needs a value.");
			parse_error = TRUE;
			break;
			
		case 'A':
			SMBD_DEBUG("request address: %s", optarg);
			parse_error = !_check_str_is_IPv4_and_translate(optarg, 
														pAppConf->smb_ipv4_url, 
														sizeof(pAppConf->smb_ipv4_url), 
														FALSE);
			if (parse_error) {
				SMBD_ERROR("Invalid SMB server IP address: \"%s\".", optarg);
			}
			break;

		case 'U':
			if (strlen(optarg) < sizeof(pAppConf->smb_username) - 1) {
				strcpy(pAppConf->smb_username, optarg);
			}
			else {
				SMBD_ERROR("Username \"%s\" is too long.", optarg);
				parse_error = TRUE;
			}
			break;

		case 'P':
			if (strlen(optarg) < sizeof(pAppConf->smb_password) - 1) {
				strcpy(pAppConf->smb_password, optarg);
			}
			else {
				SMBD_ERROR("Password \"%s\" is too long.", optarg);
				parse_error = TRUE;
			}
			break;

		case 'T':
			pAppConf->smb_timeout = strtol(optarg, NULL, 10);
			if (pAppConf->smb_timeout < 0) {
				pAppConf->smb_timeout = 0;
			}
			break;

		case 'K':
			if (strlen(optarg) < sizeof(pAppConf->smb_srv_path) - 1) {
				strcpy(pAppConf->smb_srv_path, optarg);
			}
			break;

		case 'D':
			pAppConf->should_daemonize = TRUE;
			break;

		case 'G':
			pAppConf->is_list_workgroup_mode = TRUE;
			break;

		case 'M':
			pAppConf->smb_max_dir_count = strtol(optarg, NULL, 10);
			if (pAppConf->smb_max_dir_count <= 0) {
				pAppConf->smb_max_dir_count = 0;
			}
			break;

		case 'L':
			if (0 == _parse_workgroups(optarg, pAppConf)) {
				pAppConf->is_list_computer_mode = TRUE;
			}
			else {
				parse_error = TRUE;
			}
			break;
			
		case '?':
			SMBD_ERROR("Unknown option: %c.", optopt);
			parse_error = TRUE;
			break;

		default:
			SMBD_ERROR("Unknown error.");
			parse_error = TRUE;
			break;
		}
	}

	return parse_error ? -1 : 0;
}


/********/
/* smbclient functions */

static int _configs_check(SmbDiscoverContext_st *pAppConf)
{
	if (pAppConf->is_list_workgroup_mode) {
		return util_smbtree_check_config(pAppConf);
	}
	else if (pAppConf->is_list_computer_mode) {
		return util_nmblookup_check_config(pAppConf);
	}
	else {
		return util_smbclient_check_config(pAppConf);
	}
}


static ssize_t _report_self_to_server(SmbDiscoverContext_st *pAppConf, int fd)
{
	char reportMsg[256] = "";
	int pid = getpid();

	if (pAppConf->is_list_workgroup_mode)
	{
		snprintf(reportMsg, sizeof(reportMsg) - 1, 
				"{"
					"\"command\":\"report self\","
					"\"mode\":\"smbtree\","
					"\"pid\":%d"
				"}", 
				pid
			);
	}
	else if (pAppConf->is_list_computer_mode)
	{
		snprintf(reportMsg, sizeof(reportMsg) - 1, 
				"{"
					"\"command\":\"report self\","
					"\"mode\":\"nmblookup\","
					"\"pid\":%d"
				"}",
				pid
			);
	}
	else
	{
		snprintf(reportMsg, sizeof(reportMsg) - 1, 
				"{"
					"\"command\":\"report self\","
					"\"mode\":\"smbclient\","
					"\"IPv4\":\"%s\","
					"\"username\":\"%s\","
					"\"password\":\"%s\","
					"\"pid\":%d"
				"}",
				pAppConf->smb_ipv4_url,
				pAppConf->smb_username,
				pAppConf->smb_password,
				pid
			);
	}

	return write(fd, reportMsg, strlen(reportMsg) + 1);
}


static int _execute_smb_request(SmbDiscoverContext_st *pAppConf)
{
	int funcStatus = 0;
	int smbSrvFd = -1;
	cJSON *response = NULL;

	/* open socket to invoker */
	if (pAppConf->smb_srv_path[0]) {
		BOOL isSockOK = TRUE;
		
		smbSrvFd = socket(AF_UNIX, SOCK_STREAM, 0);
		if (smbSrvFd < 0) {
			int errCopy = errno;
			SMBD_ERROR("Failed to create socket: %s", strerror(errCopy));
			isSockOK = FALSE;
		}

		if (isSockOK) {
			int callStat;
			struct sockaddr_un address;
			memset(&address, 0, sizeof(address));

			address.sun_family = AF_UNIX;
			strncpy(address.sun_path, pAppConf->smb_srv_path, sizeof(address.sun_path));
			
			callStat = connect(smbSrvFd, (struct sockaddr *)(&address), sizeof(address));
			if (callStat < 0) {
				int errCopy = errno;
				isSockOK = FALSE;

				SMBD_ERROR("Failed to connect socket: %s", strerror(errCopy));
			}
		}

		if (FALSE == isSockOK) {
			if (smbSrvFd > 0) {
				close(smbSrvFd);
				smbSrvFd = -1;
			}
		}
	}
	else {
		smbSrvFd = 1;	/* stdout */
	}

	/* daemonize */
	if (1) {
		if (pAppConf->should_daemonize) {
			daemon(FALSE, TRUE);
			SMBD_DEBUG("daemonize with pid: %d", getpid());
		}
		else {
			SMBD_DEBUG("non-daemonize");
		}
	}

	/* report basic information to server */
	if (1) {
		_report_self_to_server(pAppConf, smbSrvFd);
	}

	/* prepare enviroments */
	if (0) {
		/* do other commands */
	}
	else if (pAppConf->is_list_workgroup_mode) {
		response = util_smbtree(pAppConf);
	}
	else if (pAppConf->is_list_computer_mode) {
		response = util_nmblookup(pAppConf);
	}
	else {
		response = util_smbclient(pAppConf);
	}

	/* send data to server */
	if (NULL == response) {
		SMBD_ERROR("No response given from utility");
		funcStatus = -1;
		goto ENDS;
	}
	else {
#ifndef SMB_DISCO_DEBUG
		char *jsonStr = cJSON_PrintUnformatted(response);
#else
		char *jsonStr = cJSON_Print(response);
#endif

		if (jsonStr)
		{
			SMBD_DEBUG("Got response:\n%s", jsonStr);

			size_t lenToSend = strlen(jsonStr) + 1;
			int writeLen = write(smbSrvFd, jsonStr, lenToSend);

			if (writeLen < 0) {
				int errCopy = errno;
				SMBD_ERROR("Failed to write response: %s", strerror(errCopy));
			}
			else if (writeLen != lenToSend) {
				SMBD_ERROR("Only %d bytes send", writeLen);
			}
			else {
				/* OK */
			}

			usleep(50000);				
			free(jsonStr);
			jsonStr = NULL;
		}
		else
		{
			int errCopy = errno;
			SMBD_ERROR("Failed to get JSON string: %s", strerror(errCopy));
		}
	}

	/* cleanup and return */
ENDS:
	if (smbSrvFd > 0) {
		close(smbSrvFd);
		smbSrvFd = -1;
	}

	if (response) {
		cJSON_Delete(response);
		response = NULL;
	}

	return funcStatus;
}



/********/
/* main() */
int main(int argc, char* argv[])
{
	SmbDiscoverContext_st appConfigBuff = {};

	SMBD_INFO("Hello samba! Built time: %s, %s", __TIME__, __DATE__);

	if (_register_signals() < 0) {
		goto ENDS;
	}

	if (_parse_options(argc, argv, &appConfigBuff) < 0) {
		goto ENDS;
	}

	if (_configs_check(&appConfigBuff) < 0) {
		goto ENDS;
	}

	if (_execute_smb_request(&appConfigBuff) < 0) {
		goto ENDS;
	}

ENDS:
	SMBD_INFO("%s (%d) exit", argv[0], getpid());
	return 0;
}



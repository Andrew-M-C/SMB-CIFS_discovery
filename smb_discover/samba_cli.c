

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


#include <talloc.h>
#include <talloc_stack.h>
#include <ntstatus.h>
#include <data_blob.h>
#define HAVE_BLKSIZE_T	1
#define HAVE_BLKCNT_T	1
#define blksize_t	long
#define blkcnt_t	long
#define __ERROR__XX__DONT_USE_FALSE	(0)
#define __ERROR__XX__DONT_USE_TRUE	(!FALSE)
#include <includes.h>
#include <smb.h>
#include <client.h>
#include <werror.h>
#include <gen_ndr/srvsvc.h>
#include <gen_ndr/ndr_srvsvc_c.h>
#include <ndr/libndr.h>
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

struct rpc_pipe_client_np_ref {
	struct cli_state *cli;
	struct rpc_pipe_client *pipe;
};


struct ndr_syntax_id g_syntax_id = {
	{0x4b324fc8,0x1670,0x01d3,{0x12,0x78},{0x5a,0x47,0xbf,0x6e,0xe1,0x88}},
	NDR_SRVSVC_VERSION
};

static int samba_rpc_pipe_client_np_ref_destructor(struct rpc_pipe_client_np_ref *np_ref)
{
	DLIST_REMOVE(np_ref->cli->pipe_list, np_ref->pipe);
	return 0;
}

static NTSTATUS samba_rpc_pipe_open_np(struct cli_state *cli,
				 const struct ndr_syntax_id *abstract_syntax,
				 struct rpc_pipe_client **presult)
{
	struct rpc_pipe_client *result;
	NTSTATUS status;
	struct rpc_pipe_client_np_ref *np_ref;
SMBD_DEBUG("MARK");
	/* sanity check to protect against crashes */

	if ( !cli ) {
		return NT_STATUS_INVALID_HANDLE;
	}
SMBD_DEBUG("MARK");
	result = TALLOC_ZERO_P(NULL, struct rpc_pipe_client);
	if (result == NULL) {
		return NT_STATUS_NO_MEMORY;
	}
SMBD_DEBUG("MARK");
	result->abstract_syntax = *abstract_syntax;
	result->transfer_syntax = ndr_transfer_syntax;
SMBD_DEBUG("cli->desthost = %p", cli->desthost);
SMBD_DEBUG("cli->desthost = \"%s\"", cli->desthost);
	result->desthost = talloc_strdup(result, cli->desthost);
	result->srv_name_slash = talloc_asprintf_strupper_m(
		result, "\\\\%s", result->desthost);

	result->max_xmit_frag = RPC_MAX_PDU_FRAG_LEN;
	result->max_recv_frag = RPC_MAX_PDU_FRAG_LEN;
SMBD_DEBUG("MARK");
	if ((result->desthost == NULL) || (result->srv_name_slash == NULL)) {
		TALLOC_FREE(result);
		return NT_STATUS_NO_MEMORY;
	}
SMBD_DEBUG("MARK");
	status = rpc_transport_np_init(result, cli, abstract_syntax,
				       &result->transport);
	if (!NT_STATUS_IS_OK(status)) {
		TALLOC_FREE(result);
		return status;
	}
SMBD_DEBUG("MARK");
	result->transport->transport = NCACN_NP;

	np_ref = talloc(result->transport, struct rpc_pipe_client_np_ref);
	if (np_ref == NULL) {
		TALLOC_FREE(result);
		return NT_STATUS_NO_MEMORY;
	}
	np_ref->cli = cli;
	np_ref->pipe = result;
SMBD_DEBUG("MARK");
	DLIST_ADD(np_ref->cli->pipe_list, np_ref->pipe);
	talloc_set_destructor(np_ref, samba_rpc_pipe_client_np_ref_destructor);

	result->binding_handle = rpccli_bh_create(result);
	if (result->binding_handle == NULL) {
		TALLOC_FREE(result);
		return NT_STATUS_NO_MEMORY;
	}
SMBD_DEBUG("MARK");
	*presult = result;
	return NT_STATUS_OK;
}


static NTSTATUS samba_cli_rpc_pipe_open(struct cli_state *cli,
				  enum dcerpc_transport_t transport,
				  const struct ndr_syntax_id *intf,
				  struct rpc_pipe_client **presult)
{
	switch (transport) {
	case NCACN_IP_TCP:
		SMBD_DEBUG("cli->desthost = %p", cli->desthost);
		SMBD_DEBUG("cli->desthost = \"%s\"", cli->desthost);
		return rpc_pipe_open_tcp(NULL, cli->desthost, intf,
					 presult);
	case NCACN_NP:
		SMBD_DEBUG("cli->desthost = %p", cli->desthost);
		SMBD_DEBUG("cli->desthost = \"%s\"", cli->desthost);
		return samba_rpc_pipe_open_np(cli, intf, presult);
	default:
		return NT_STATUS_NOT_IMPLEMENTED;
	}
}


NTSTATUS samba_cli_rpc_pipe_open_noauth_transport(struct cli_state *cli,
					    enum dcerpc_transport_t transport,
					    const struct ndr_syntax_id *intf,
					    struct rpc_pipe_client **presult)
{

	struct rpc_pipe_client *result;
	struct pipe_auth_data *auth;
	NTSTATUS status;

	status = samba_cli_rpc_pipe_open(cli, transport, intf, &result);
SMBD_DEBUG("cli = %p", cli);
SMBD_DEBUG("transport = %d", transport);
SMBD_DEBUG("intf = %p", intf);
SMBD_DEBUG("presult = %p", presult);
SMBD_DEBUG("cli->desthost = %p", cli->desthost);
SMBD_DEBUG("cli->desthost = \"%s\"", cli->desthost);
	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}
SMBD_DEBUG("cli->desthost = %p", cli->desthost);
SMBD_DEBUG("cli->desthost = \"%s\"", cli->desthost);
	status = rpccli_anon_bind_data(result, &auth);
	if (!NT_STATUS_IS_OK(status)) {
		DEBUG(0, ("rpccli_anon_bind_data returned %s\n",
			  nt_errstr(status)));
		TALLOC_FREE(result);
		return status;
	}
SMBD_DEBUG("cli->desthost = %p", cli->desthost);
SMBD_DEBUG("cli->desthost = \"%s\"", cli->desthost);
	/*
	 * This is a bit of an abstraction violation due to the fact that an
	 * anonymous bind on an authenticated SMB inherits the user/domain
	 * from the enclosing SMB creds
	 */
SMBD_DEBUG("cli->desthost = %p", cli->desthost);
SMBD_DEBUG("cli->desthost = \"%s\"", cli->desthost);
	TALLOC_FREE(auth->user_name);
	TALLOC_FREE(auth->domain);
SMBD_DEBUG("cli->desthost = %p", cli->desthost);
SMBD_DEBUG("cli->desthost = \"%s\"", cli->desthost);
	auth->user_name = talloc_strdup(auth, cli->user_name);
	auth->domain = talloc_strdup(auth, cli->domain);
	auth->user_session_key = data_blob_talloc(auth,
		cli->user_session_key.data,
		cli->user_session_key.length);

	if ((auth->user_name == NULL) || (auth->domain == NULL)) {
		TALLOC_FREE(result);
		return NT_STATUS_NO_MEMORY;
	}
SMBD_DEBUG("cli->desthost = %p", cli->desthost);
SMBD_DEBUG("cli->desthost = \"%s\"", cli->desthost);
	status = rpc_pipe_bind(result, auth);
	if (!NT_STATUS_IS_OK(status)) {
		int lvl = 0;
		if (ndr_syntax_id_equal(intf,
					&g_syntax_id)) {
			/* non AD domains just don't have this pipe, avoid
			 * level 0 statement in that case - gd */
			lvl = 3;
		}
		SMBD_DEBUG("cli_rpc_pipe_open_noauth: rpc_pipe_bind for pipe "
			    "%s failed with error %s\n",
			    get_pipe_name_from_syntax(talloc_tos(), intf),
			    nt_errstr(status));
		TALLOC_FREE(result);
		return status;
	}
SMBD_DEBUG("cli->desthost = %p", cli->desthost);
SMBD_DEBUG("cli->desthost = \"%s\"", cli->desthost);
	SMBD_DEBUG("cli_rpc_pipe_open_noauth: opened pipe %s to machine "
		  "%s and bound anonymously.\n",
		  get_pipe_name_from_syntax(talloc_tos(), intf),
		  cli->desthost);
SMBD_DEBUG("cli->desthost = %p", cli->desthost);
SMBD_DEBUG("cli->desthost = \"%s\"", cli->desthost);
	*presult = result;
	return NT_STATUS_OK;
}


/* EOF */


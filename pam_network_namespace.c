#include <netlink/route/link/veth.h>
#include <sched.h>
#include <security/pam_modules.h>
#include <security/pam_modutil.h>
#include <security/pam_ext.h>
#include <syslog.h>
#include <unistd.h>

#define DEFAULT_DEVICE_NAME "veth0"

PAM_EXTERN int pam_sm_open_session(pam_handle_t *pamh, int flags,
                                   int argc, const char **argv) {
	(void)flags;
	(void)argc;
	(void)argv;
	const char *user = NULL;
	int err;
	struct nl_sock *sock = NULL;

	if (PAM_SUCCESS != pam_get_item(pamh, PAM_USER, (const void**) &user) || user == NULL) {
		pam_syslog(pamh, LOG_ERR, "Unable to get username; using auto-generated interface name.");
		user = NULL;
	}

	if (0 != unshare(CLONE_NEWNET)) {
		pam_syslog(pamh, LOG_ERR, "Unable to unshare from parent namespace: %m");
		goto err;
	}

	if (NULL == (sock = nl_socket_alloc())) {
		pam_syslog(pamh, LOG_ERR, "Unable to allocate netlink socket: %m");
		goto err;
	}

	if (0 != (err = nl_connect(sock, NETLINK_ROUTE))) {
		pam_syslog(pamh, LOG_ERR, "Unable to connect to netlink socket: %s", nl_geterror(err));
		goto err;
	}

	/* Create veth device shared with parent */
	if (0 != (err = rtnl_link_veth_add(sock, DEFAULT_DEVICE_NAME, user, getppid()))) {
		pam_syslog(pamh, LOG_ERR, "Unable to create veth pair: %s", nl_geterror(err));
		goto err;
	}

	return PAM_SUCCESS;

err:
	if (sock) nl_socket_free(sock);

	return PAM_SESSION_ERR;
}

PAM_EXTERN int pam_sm_close_session(pam_handle_t *pamh, int flags,
                                    int argc, const char **argv) {
	(void)pamh;
	(void)flags;
	(void)argc;
	(void)argv;
	return PAM_IGNORE;
}

#ifdef PAM_STATIC
struct pam_module _pam_network_namespace_modstruct = {
     "pam_network_namespace",
     NULL,
     NULL,
     NULL,
     pam_sm_open_session,
     pam_sm_close_session,
     NULL
};
#endif

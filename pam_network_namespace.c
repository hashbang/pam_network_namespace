#include <netlink/route/link/veth.h>
#include <sched.h>
#include <security/pam_modules.h>
#include <security/pam_modutil.h>
#include <security/pam_ext.h>
#include <syslog.h>
#include <unistd.h>

PAM_EXTERN int pam_sm_open_session(pam_handle_t *pamh, int flags,
                                   int argc, const char **argv) {
	(void)flags;
	(void)argc;
	(void)argv;
	struct nl_sock *sock = NULL;

	if (0 != unshare(CLONE_NEWNET)) {
		pam_syslog(pamh, LOG_ERR, "Unable to unshare from parent namespace: %m");
		goto err;
	}

	if (NULL == (sock = nl_socket_alloc())) {
		pam_syslog(pamh, LOG_ERR, "Unable to allocate netlink socket: %m");
		goto err;
	}

	if (0 != nl_connect(sock, NETLINK_ROUTE)) {
		pam_syslog(pamh, LOG_ERR, "Unable to connect to netlink socket: %m");
		goto err;
	}

	if (0 != rtnl_link_veth_add(sock, NULL, NULL, getppid())) {
		pam_syslog(pamh, LOG_ERR, "Unable to create veth pair: %m");
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
struct pam_module _pam_namespace_modstruct = {
     "pam_network_namespace",
     NULL,
     NULL,
     NULL,
     pam_sm_open_session,
     pam_sm_close_session,
     NULL
};
#endif

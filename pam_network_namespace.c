#include <sys/socket.h> /* needed for linux/if.h */
#include <linux/if.h> /* IFF_UP */
#include <netlink/route/link.h>
#include <netlink/route/link/veth.h>
#include <sched.h>
#include <security/pam_modules.h>
#include <security/pam_modutil.h>
#include <security/pam_ext.h>
#include <syslog.h>
#include <unistd.h>

#if TEST
#include <stdio.h>
#define pam_syslog(pamh, i, ...) ((void)pamh, (void)i, printf(__VA_ARGS__), puts(""))
#define pam_get_item(pamh, i, user) ((void)pamh, (void)i, *user = getpwuid(geteuid())->pw_name, PAM_SUCCESS)
#endif

#define DEFAULT_DEVICE_NAME "veth0"

PAM_EXTERN int pam_sm_open_session(pam_handle_t *pamh, int flags,
                                   int argc, const char **argv) {
	(void)flags;
	(void)argc;
	(void)argv;
	const char *user = NULL;
	int err;
	struct nl_sock *sock = NULL;
	struct rtnl_link *lo = NULL, *lo_changes = NULL, *link = NULL, *peer = NULL;

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

	/* Bring up loopback device inside namespace equivalent to: ip link set lo up */
	if (0 != (err = rtnl_link_get_kernel(sock, 1, "lo", &lo))) {
		pam_syslog(pamh, LOG_ERR, "Unable to get looopback link: %s", nl_geterror(err));
		goto err;
	}
	if (NULL == (lo_changes = rtnl_link_alloc())) {
		pam_syslog(pamh, LOG_ERR, "Unable to allocate link");
		goto err;
	}
	rtnl_link_set_flags(lo_changes, IFF_UP);
	if (0 != (err = rtnl_link_change(sock, lo, lo_changes, 0))) {
		pam_syslog(pamh, LOG_ERR, "Unable to bring up loopback interface: %s", nl_geterror(err));
		goto err;
	}
	rtnl_link_put(lo_changes);
	rtnl_link_put(lo);
	lo_changes = NULL, lo = NULL;

	/* Create veth device shared with parent */
	if (NULL == (link = rtnl_link_veth_alloc())) {
		pam_syslog(pamh, LOG_ERR, "Unable to allocate veth");
		goto err;
	}
	peer = rtnl_link_veth_get_peer(link);
	rtnl_link_set_name(link, DEFAULT_DEVICE_NAME);
	if (user) rtnl_link_set_name(peer, user);
	rtnl_link_set_ns_pid(peer, getppid());
	if (0 != (err = rtnl_link_add(sock, link, NLM_F_CREATE))) {
		pam_syslog(pamh, LOG_ERR, "Unable to create veth pair: %s", nl_geterror(err));
		goto err;
	}
	rtnl_link_put(peer);
	rtnl_link_put(link);
	peer = NULL, link = NULL;

	/* all done */
	return PAM_SUCCESS;

err:
	if (lo_changes) rtnl_link_put(lo_changes);
	if (lo) rtnl_link_put(lo);
	if (peer) rtnl_link_put(peer);
	if (link) rtnl_link_put(link);
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


#if TEST
int main(int argc, char *argv[]) {
	if (pam_sm_open_session(NULL, 0, 0, NULL) != PAM_SUCCESS) return 1;
	if (argc <= 1) return 0;
	return execvp(argv[1], argv+1);
}
#endif

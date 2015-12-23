#include <sched.h>
#include <security/pam_modules.h>
#include <security/pam_modutil.h>
#include <security/pam_ext.h>
#include <syslog.h>

PAM_EXTERN int pam_sm_open_session(pam_handle_t *pamh, int flags,
                                   int argc, const char **argv) {
	(void)flags;
	(void)argc;
	(void)argv;
	if (0 != unshare(CLONE_NEWNET)) {
		pam_syslog(pamh, LOG_ERR, "Unable to unshare from parent namespace, %m");
		return PAM_SESSION_ERR;
	}
	return PAM_SUCCESS;
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

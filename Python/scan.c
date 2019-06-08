#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <linux/netlink.h>
#include <netlink/msg.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <linux/nl80211.h>
#include "linux_netlink_control.h"

static int nl80211_error_cb(struct sockaddr_nl *nla, struct nlmsgerr *err, void *arg) {
	int *ret = (int *) arg;
	printf("%s\n", __func__);
	*ret = err->error;
	return NL_STOP;
}

static int nl80211_finish_cb(struct nl_msg *msg, void *arg) {
	int *ret = (int *) arg;
	printf("%s\n", __func__);
	*ret = 0;
	return NL_SKIP;
}

static int nl80211_ack_cb(struct nl_msg *msg, void *arg) {
    int *ret = arg;
	printf("%s\n", __func__);
    *ret = 0;
    return NL_STOP;
}

static int nl80211_get_scan_cb(struct nl_msg *msg, void *arg) 
{
	PyObject* network_list = (PyObject*)arg;

	printf("%s\n", __func__);

    return NL_SKIP;
}

PyObject* get_scan(PyObject *self, PyObject *args)
{
	const char* interface;
	int retcode;
	struct nl_handler *sock;
	int id, index;
	char errstr[STATUS_MAX+1];

	if (!PyArg_ParseTuple(args, "s", &interface))
		return NULL;

	retcode = mac80211_connect(
					interface, 
					(void**)&sock, &id, &index, errstr);

	if (retcode != 0) {
		fprintf(stderr, "%s retcode=%d\n", __func__, retcode);
		PyErr_SetString(PyExc_RuntimeError, "failed to connect mac80211");
		return NULL;
	}

	printf("%s retcode=%d id=%d index=%d\n", __func__, retcode, id, index);

	/* starting with guts of kismet's mac80211_get_chanlist() linux_netlink_control.c */

    struct nl_msg* msg = nlmsg_alloc();

    struct nl_cb* cb = nl_cb_alloc(NL_CB_DEFAULT);

    int err = 1;

	PyObject* network_list = PyList_New(0);

    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, nl80211_get_scan_cb, network_list);
	nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, nl80211_ack_cb, &err);
    nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, nl80211_finish_cb, &err);
    nl_cb_err(cb, NL_CB_CUSTOM, nl80211_error_cb, &err);

    genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, id, 0, NLM_F_DUMP, NL80211_CMD_GET_SCAN, 0);
	nla_put_u32(msg, NL80211_ATTR_IFINDEX, index);

    if (nl_send_auto_complete((struct nl_sock *) sock, msg) < 0) {
        snprintf(errstr, STATUS_MAX, 
                "failed to fetch channels from interface '%s': failed to "
                "write netlink command", interface);
        nlmsg_free(msg);
        nl_cb_put(cb);
		mac80211_disconnect(sock);
		PyErr_SetString(PyExc_RuntimeError, errstr);
		return NULL;
    }

    while (err)
        nl_recvmsgs((struct nl_sock *) sock, cb);

    nl_cb_put(cb);
    nlmsg_free(msg);

	mac80211_disconnect(sock);
	Py_RETURN_NONE;
}


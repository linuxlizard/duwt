#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <stdint.h>
#include <stdbool.h>

#include <linux/netlink.h>
#include <netlink/msg.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>

#include <linux/nl80211.h>

#include "linux_netlink_control.h"
#include "scan.h"
#include "iw.h"
#include "ie.h"

static const bool iw_debug = true;

// iw scan.c print_bss_handler()
static struct nla_policy bss_policy[NL80211_BSS_MAX + 1] = {
	[NL80211_BSS_TSF] = { .type = NLA_U64 },
	[NL80211_BSS_FREQUENCY] = { .type = NLA_U32 },
	[NL80211_BSS_BSSID] = { },
	[NL80211_BSS_BEACON_INTERVAL] = { .type = NLA_U16 },
	[NL80211_BSS_CAPABILITY] = { .type = NLA_U16 },
	[NL80211_BSS_INFORMATION_ELEMENTS] = { },
	[NL80211_BSS_SIGNAL_MBM] = { .type = NLA_U32 },
	[NL80211_BSS_SIGNAL_UNSPEC] = { .type = NLA_U8 },
	[NL80211_BSS_STATUS] = { .type = NLA_U32 },
	[NL80211_BSS_SEEN_MS_AGO] = { .type = NLA_U32 },
	[NL80211_BSS_BEACON_IES] = { },
};

/* from kismet */
static int nl80211_error_cb(struct sockaddr_nl* Py_UNUSED(nla), struct nlmsgerr *err, void *arg) {
	int *ret = (int *) arg;
	printf("%s\n", __func__);
	*ret = err->error;
	return NL_STOP;
}

/* from kismet */
static int nl80211_finish_cb(struct nl_msg* Py_UNUSED(msg), void *arg) {
	int *ret = (int *) arg;
	printf("%s\n", __func__);
	*ret = 0;
	return NL_SKIP;
}

/* from kismet */
static int nl80211_ack_cb(struct nl_msg* Py_UNUSED(msg), void *arg) {
	int *ret = arg;
	printf("%s\n", __func__);
	*ret = 0;
	return NL_STOP;
}

static PyObject* parse_ies(struct nlattr* ies)
{
	uint8_t *ie = (uint8_t *)nla_data(ies);
	ssize_t ielen = (ssize_t)nla_len(ies);
	uint8_t *ie_end = ie + ielen;
	size_t counter=0;

	// each IE will be stored in a dict key'd by the ID
	PyObject* ie_dict = PyDict_New();
	if (!ie_dict) {
		return NULL;
	}

	PyObject* vendor_id = PyLong_FromLong(221);
	if (!vendor_id) {
		return NULL;
	}

	PyObject* extention_id = PyLong_FromLong(255);
	if (!extention_id) {
		Py_DECREF(vendor_id);
		return NULL;
	}

	PyObject* value = NULL;
	while (ie < ie_end) {
		printf("%s ie=%d\n", __func__, ie[0]);

		value = Py_BuildValue("{s:s,s:i}", 
									"name", ie_get_name(ie[0]),
									"len", (int)ie[1]
								);
//		PyObject* ie_dict = Py_BuildValue("{s:s},{s:y*}", 
//									"name", ie_get_name(ie[0]),
//									"value", PyBytes_FromStringAndSize(&ie[2], (int)ie[1])
//								);
		if (!value) {
			goto fail;
		}

		if (ie[0] == 221 || ie[0] == 255) {
			// can have multiple Vendor Specific (221) or Extension (255) IEs
			// so store the values in an array
#if 0
			PyObject* list = PyDict_GetItem(ie_dict, ie[0]==221?vendor_id:extention_id);

			if (!list) {
				// create the list
				list = PyList_New(0);
				if (!list) {
					goto fail;
				}

				if (PyDict_SetItem(ie_dict, ie[0]==221?vendor_id:extention_id, list) < 0) {
					Py_DECREF(list);
					goto fail;
				}
			}

			PyObject* bytes = PyBytes_FromStringAndSize((const char *)&ie[2], (Py_ssize_t)ie[1]);
			if (PyDict_SetItemString(value, "bytes", bytes) < 0) {
				goto fail;
			}
			Py_CLEAR(bytes);

			if (PyList_Append(list, nested_value) < 0) {
			}

			Py_DECREF(list);
#endif
		}
		else {
			if (ie_decode(ie[0], ie[1], &ie[2], value) < 0) {
				goto fail;
			}

			PyObject* bytes = PyBytes_FromStringAndSize((const char *)&ie[2], (Py_ssize_t)ie[1]);
			if (PyDict_SetItemString(value, "bytes", bytes) < 0) {
				goto fail;
			}
			Py_CLEAR(bytes);

			PyObject* key = PyLong_FromUnsignedLong(ie[0]);
			if (PyDict_SetItem(ie_dict, key, value) < 0) {
				goto fail;
			}
			Py_CLEAR(key);
		}

		Py_CLEAR(value);

		ie += ie[1] + 2;
		counter++;
	}

	Py_DECREF(vendor_id);
	Py_DECREF(extention_id);

	printf("%s found count=%zu IEs\n", __func__, counter);

	return ie_dict;

fail:
	Py_DECREF(vendor_id);
	Py_DECREF(extention_id);
	Py_XDECREF(value);

	// TODO
	// what else?

	return NULL;
}

static PyObject* parse_bss(struct nlattr* bss[])
{
	size_t i;
	int retcode;
	PyObject* bss_dict = NULL;

	bss_dict = PyDict_New();
	if (!bss_dict) {
		return NULL;
	}

	for (i=0 ; i<NL80211_BSS_MAX ; i++ ) {
		if (bss[i]) {
			printf("%s bss %zu=%p type=%d len=%d\n", 
					__func__, 
					i, (void *)bss[i], nla_type(bss[i]), nla_len(bss[i]));
		}
	}

	if (bss[NL80211_BSS_BSSID]) {
		char mac_addr[20];
		uint8_t* ptr = (uint8_t*)nla_data(bss[NL80211_BSS_BSSID]);
		retcode = PyOS_snprintf(mac_addr, 20, "%02x:%02x:%02x:%02x:%02x:%02x",
			ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);

		PyObject* bssid_str = Py_BuildValue("s", mac_addr);
		retcode = PyDict_SetItemString(bss_dict, "BSSID", bssid_str);
	}

	// nl80211.h enum nl80211_bss
	if (bss[NL80211_BSS_FREQUENCY]) {
		uint32_t freq = nla_get_u32(bss[NL80211_BSS_FREQUENCY]);
		retcode = PyDict_SetItemString(bss_dict, "frequency", PyLong_FromUnsignedLong(freq));
		if (retcode) {
			// TODO
		}
		retcode = PyDict_SetItemString(bss_dict, "channel", PyLong_FromUnsignedLong(mac80211_freq_to_chan(freq)));
		if (retcode) {
			// TODO
		}
		// TODO add above/below channel
	}
	if (bss[NL80211_BSS_BEACON_INTERVAL]) {
//		uint16_t beacon_interval = nla_get_u16(bss[NL80211_BSS_BEACON_INTERVAL]);
		// TODO
	}
	if (bss[NL80211_BSS_CAPABILITY]) {
		uint16_t capa = nla_get_u16(bss[NL80211_BSS_CAPABILITY]);
		retcode = PyDict_SetItemString(bss_dict, "capability", PyLong_FromUnsignedLong(capa));
		if (retcode) {
			// TODO
		}

	}
	if (bss[NL80211_BSS_SIGNAL_MBM]) {
		// "@NL80211_BSS_SIGNAL_MBM: signal strength of probe response/beacon
		//	in mBm (100 * dBm) (s32)"  (via nl80211.h)
		uint32_t signal = nla_get_u32(bss[NL80211_BSS_SIGNAL_MBM]);

		PyObject* ss = Py_BuildValue("{s:f,s:s}", "value", ((int)signal)/100.0, "units", "dBm");
		if (!ss) {
			return NULL;
		}

		if (PyDict_SetItemString(bss_dict, "signal_strength", ss) < 0) {
			// TODO
		}
		Py_CLEAR(ss);
	}
	if (bss[NL80211_BSS_SIGNAL_UNSPEC]) {
		// "@NL80211_BSS_SIGNAL_UNSPEC: signal strength of the probe response/beacon
		//	in unspecified units, scaled to 0..100 (u8)"  (via nl80211.h)
//		uint8_t signal_unspec = nla_get_u8(bss[NL80211_BSS_SIGNAL_UNSPEC]);
		// TODO
	}
	if (bss[NL80211_BSS_SEEN_MS_AGO]) {
		uint32_t last_seen_ms = nla_get_u32(bss[NL80211_BSS_SEEN_MS_AGO]);
		if (PyDict_SetItemString(bss_dict, "last_seen_ms", PyLong_FromUnsignedLong(last_seen_ms)) < 0) {
			// TODO
		}
	}

	// information can be duplicated between Beacon and Probe Response IEs
	if (bss[NL80211_BSS_INFORMATION_ELEMENTS] ) {
		struct nlattr *ies = bss[NL80211_BSS_INFORMATION_ELEMENTS];

		PyObject* ie_dict = parse_ies(ies);
		if (!ie_dict) {
			// TODO 
		}
		
		if (PyDict_SetItemString(bss_dict, "ie", ie_dict) < 0) {
			// TODO
		}
		Py_CLEAR(ie_dict);
	}

	// information can be duplicated between Beacon and Probe Response IEs
	if (bss[NL80211_BSS_BEACON_IES] ) {
		// TODO
	}

	return bss_dict;
}

// iw scan.c print_bss_handler()
static int nl80211_get_scan_cb(struct nl_msg *msg, void *arg) 
{
	PyObject* network_list = (PyObject*)arg;

	assert(PyList_Check(network_list));

	printf("%s\n", __func__);

	struct nlmsghdr *hdr = nlmsg_hdr(msg);
	struct genlmsghdr* gnlh = (struct genlmsghdr*)nlmsg_data(hdr);

	if (iw_debug) {
//		nl_msg_dump(msg,stdout);
//		printf("%s nlmsg max size=%ld\n", __func__, nlmsg_get_max_size(msg));
//		printf("%s msgsize=%d\n", __func__, nlmsg_datalen(hdr));
//		printf("%s genlen=%d genattrlen=%d\n", __func__, genlmsg_len(gnlh), genlmsg_attrlen(gnlh, 0));
	}

	struct nlattr* tb_msg[NL80211_ATTR_MAX + 1];
	int retcode = nla_parse(tb_msg, NL80211_ATTR_MAX, 
						genlmsg_attrdata(gnlh, 0),
						genlmsg_attrlen(gnlh, 0), NULL);
	if (retcode != 0) {
		// TODO
	}

	size_t i;
	for (i=0 ; i<NL80211_ATTR_MAX ; i++ ) {
		if (tb_msg[i]) {
			printf("%s %zu=%p type=%d len=%d\n", 
					__func__, i, (void *)tb_msg[i], nla_type(tb_msg[i]), nla_len(tb_msg[i]));
		}
	}

	if (tb_msg[NL80211_ATTR_BSS]) {
		struct nlattr *bss[NL80211_BSS_MAX + 1];
		if (nla_parse_nested(bss, NL80211_BSS_MAX,
					 tb_msg[NL80211_ATTR_BSS],
					 bss_policy)) {
			fprintf(stderr, "failed to parse nested attributes!\n");
			return NL_SKIP;
		}

		if (!bss[NL80211_BSS_BSSID]) {
			// TODO throw something (?)
		}

		PyObject* bss_dict = parse_bss(bss);
		if (!bss_dict) {
			// TODO
		}

		if (PyList_Append(network_list, bss_dict) < 0) {
			// TODO
		}
		Py_CLEAR(bss_dict);
	}

    return NL_SKIP;
}

PyObject* get_scan(PyObject* Py_UNUSED(self), PyObject* args)
{
	const char* interface;
	int retcode;
	struct nl_handler *sock;
	int id, index;
	char errstr[STATUS_MAX+1];
    struct nl_msg* msg = NULL;
    struct nl_cb* cb = NULL;
	PyObject* network_list = NULL;

	if (!PyArg_ParseTuple(args, "s", &interface)) {
		return NULL;
	}

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

	msg = nlmsg_alloc();
	if (!msg) {
		goto finally;
	}
    cb = nl_cb_alloc(NL_CB_DEFAULT);
	if (!cb) {
		goto finally;
	}

	int err = 1;

	network_list = PyList_New(0);
	if (!network_list) {
		goto finally;
	}

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

finally:
	if (cb) {
		nl_cb_put(cb);
	}
	if (msg) {
		nlmsg_free(msg);
	}

	mac80211_disconnect(sock);
	return network_list;
}


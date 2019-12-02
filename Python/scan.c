#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <stdint.h>
#include <stdbool.h>

#include <linux/netlink.h>
#include <linux/if_ether.h>
#include <netlink/msg.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>

#include <linux/nl80211.h>

#include "linux_netlink_control.h"
#include "scan.h"
#include "iw.h"
#include "ie.h"

/*
Public sources include:
	iw source code, esp. scan.c
	kismet source code
	https://stackoverflow.com/questions/18062268/using-nl80211-h-to-scan-access-points
	https://github.com/Robpol86/libnl/blob/master/example_c/scan_access_points.c
	https://www.infradead.org/~tgr/libnl/doc/api/index.html
*/

#if 0
static const bool iw_debug = true;
#endif

static struct nla_policy bss_policy[NL80211_BSS_MAX + 1] = {
	[NL80211_BSS_BSSID] = { .type = NLA_UNSPEC, .minlen =  ETH_ALEN },
	[NL80211_BSS_FREQUENCY] = { .type = NLA_U32 },
	[NL80211_BSS_TSF] = { .type = NLA_U64 },
	[NL80211_BSS_BEACON_INTERVAL] = { .type = NLA_U16 },
	[NL80211_BSS_CAPABILITY] = { .type = NLA_U16 },
	[NL80211_BSS_INFORMATION_ELEMENTS] = { },
	[NL80211_BSS_SIGNAL_MBM] = { .type = NLA_U32 },
	[NL80211_BSS_SIGNAL_UNSPEC] = { .type = NLA_U8 },
	[NL80211_BSS_STATUS] = { .type = NLA_U32 },
	[NL80211_BSS_SEEN_MS_AGO] = { .type = NLA_U32 },
	[NL80211_BSS_BEACON_IES] = { },
};

static int nl80211_error_cb(struct sockaddr_nl* Py_UNUSED(nla), struct nlmsgerr* err, void* arg)
{
	// printf("%s\n", __func__);
	int *ret = (int *)arg;
	*ret = err->error;
	return NL_STOP;
}

static int nl80211_finish_cb(struct nl_msg* Py_UNUSED(msg), void* arg)
{
	// printf("%s\n", __func__);
	int *ret = (int *)arg;
	*ret = 0;
	return NL_SKIP;
}

static int nl80211_ack_cb(struct nl_msg* Py_UNUSED(msg), void* arg)
{
	// printf("%s\n", __func__);
	int *ret = (int *)arg;
	*ret = 0;
	return NL_STOP;
}

static int nl80211_no_seq_check_cb(struct nl_msg* Py_UNUSED(msg), void* Py_UNUSED(arg))
{
	// printf("%s\n", __func__);
	return NL_OK;
}

static int nl80211_trigger_scan_cb(struct nl_msg* msg, void* arg)
{
	// printf("%s\n", __func__);
	struct genlmsghdr* gnlh = nlmsg_data(nlmsg_hdr(msg));
	int *ret = (int *)arg;

	if (gnlh->cmd == NL80211_CMD_SCAN_ABORTED) {
		// printf("%s: Result: NL80211_CMD_SCAN_ABORTED\n", __func__);
		*ret = NL80211_CMD_SCAN_ABORTED;
	}
	else if (gnlh->cmd == NL80211_CMD_NEW_SCAN_RESULTS) {
		// printf("%s: Result: NL80211_CMD_NEW_SCAN_RESULTS\n", __func__);
		*ret = NL80211_CMD_NEW_SCAN_RESULTS;
	}
	else {
		// Ignore this response unless testing / debugging
		// printf("%s: Result: NL80211_CMD_UNSPEC\n", __func__);
		*ret = NL80211_CMD_UNSPEC;
	}
	return NL_SKIP;
}

/*
  Given a python list of SSIDs, build a container message listing SSIDs to scan.

  Returns:
	  0  an ssid list message was built successfully
	< 0  an error occurred while building the message
 */
static int build_ssid_list_msg(PyObject *ssid_list, struct nl_msg* ssid_list_msg)
{
	int i, success = 0;
	Py_ssize_t ssid_list_len=0;
	char errstr[STATUS_MAX+1];
	PyObject* py_ssid;
	const char* ssid_str;
	Py_ssize_t ssid_len;

	if (!ssid_list_msg) {
		snprintf(errstr, STATUS_MAX, "%s: invalid argument", __func__);
		PyErr_SetString(PyExc_TypeError, errstr);
		success = -1;
		goto finally;
	}

	if (!ssid_list || (ssid_list == Py_None)) {
		// Treat these cases as a request for an ACTIVE scan on all SSID's visible to the radio (i.e. wildcard scan)
		nla_put(ssid_list_msg, NLA_NESTED, 0, ""); // Scan all SSIDs
		goto finally;
	}

	if (!PyList_Check(ssid_list)) {
		snprintf(errstr, STATUS_MAX, "%s: expected SSIDs as a list", __func__);
		PyErr_SetString(PyExc_TypeError, errstr);
		success = -1;
		goto finally;
	}

	ssid_list_len = PyList_Size(ssid_list);

	if ((ssid_list_len == 0) || (ssid_list_len > SCAN_REQ_MAX_SSID_COUNT)) {
		// Treat these cases as a request for an ACTIVE scan on all SSID's visible to the radio (i.e. wildcard scan)
		nla_put(ssid_list_msg, NLA_NESTED, 0, ""); // Scan all SSIDs
		goto finally;
	}

	for (i = 0; i < ssid_list_len; i++) {
		py_ssid = PyList_GetItem(ssid_list, i);

		if (py_ssid == NULL) {
			snprintf(errstr, STATUS_MAX, "%s: NULL entry in ssid list, index=%i", __func__, i);
			PyErr_SetString(PyExc_ValueError, errstr);
			success = -1;
			break;
		}

		if (!PyUnicode_Check(py_ssid)) {
			snprintf(errstr, STATUS_MAX, "%s: non-string entry in ssid list, index=%i", __func__, i);
			PyErr_SetString(PyExc_ValueError, errstr);
			success = -1;
			break;
		}

		ssid_str = PyUnicode_AsUTF8AndSize(py_ssid, &ssid_len);

		if (!ssid_str) {
			snprintf(errstr, STATUS_MAX, "%s: error accessing ssid string, index=%i", __func__, i);
			PyErr_SetString(PyExc_ValueError, errstr);
			success = -1;
			break;
		}

		if ((ssid_len == 0) || (ssid_len > SCAN_REQ_MAX_SSID_LEN)) {
			snprintf(errstr, STATUS_MAX, "%s: unsupported SSID length, index=%i", __func__, i);
			PyErr_SetString(PyExc_ValueError, errstr);
			success = -1;
			break;
		}

		success = nla_put(ssid_list_msg, NLA_NESTED, (int)ssid_len, (const void *)ssid_str);

		if (success < 0) {
			snprintf(errstr, STATUS_MAX, "%s: nla error building message, index=%i", __func__, i);
			PyErr_SetString(PyExc_ValueError, errstr);
			break;
		}
	}

finally:
	return success;
}

/*
  Given a python list of channels, build a container message listing frequencies to scan.

  Returns:
	> 0  success; the value indicates the number of frequencies built into the list message
	  0  success; the value indicates a "wildcard" scan (i.e. all channels supported in the current regulation domain)
	< 0  an error occurred while building the message
 */
static int build_freq_list_msg(PyObject* channel_list, struct nl_msg* freq_list_msg)
{
	int i, success = 0;
	Py_ssize_t channel_list_len = 0;
	char errstr[STATUS_MAX+1];
	PyObject* py_channel;
	unsigned int channel;
	unsigned int frequency;

	if (!freq_list_msg) {
		snprintf(errstr, STATUS_MAX, "%s: invalid argument", __func__);
		PyErr_SetString(PyExc_TypeError, errstr);
		success = -1;
		goto finally;
	}

	if (!channel_list || (channel_list == Py_None)) {
		// Treat these cases as a request for scan on all channels supported by the radio (i.e. wildcard scan)
		channel_list_len = 0;
		goto finally;
	}

	if (!PyList_Check(channel_list)) {
		snprintf(errstr, STATUS_MAX, "%s: expected channels as a list", __func__);
		PyErr_SetString(PyExc_TypeError, errstr);
		success = -1;
		goto finally;
	}

	channel_list_len = PyList_Size(channel_list);

	if ((channel_list_len == 0) || (channel_list_len > SCAN_REQ_MAX_FREQ_COUNT)) {
		// Treat these cases as a request for scan on all channels supported by the radio (i.e. wildcard scan)
		channel_list_len = 0;
		goto finally;
	}

	if (channel_list_len > SCAN_REQ_MAX_FREQ_COUNT) {
		snprintf(errstr, STATUS_MAX, "%s: too many channels requested", __func__);
		PyErr_SetString(PyExc_ValueError, errstr);
		success = -1;
		goto finally;
	}

	for (i = 0; i < channel_list_len; i++) {
		py_channel = PyList_GetItem(channel_list, i);

		if (py_channel == NULL) {
			snprintf(errstr, STATUS_MAX, "%s: NULL entry in channel list, index=%i", __func__, i);
			PyErr_SetString(PyExc_ValueError, errstr);
			success = -1;
			break;
		}

		if (!PyLong_Check(py_channel)) {
			snprintf(errstr, STATUS_MAX, "%s: non-integer entry in channel list, index=%i", __func__, i);
			PyErr_SetString(PyExc_ValueError, errstr);
			success = -1;
			break;
		}

		channel = PyLong_AsLong(py_channel);

		if (channel < 1 || channel > 165) {
			snprintf(errstr, STATUS_MAX, "%s: channel value out of range, index=%i", __func__, i);
			PyErr_SetString(PyExc_ValueError, errstr);
			success = -1;
			break;
		}

		frequency = mac80211_chan_to_freq(channel);
		success = nla_put(freq_list_msg, NLA_NESTED, sizeof(frequency), &frequency);

		if (success < 0) {
			snprintf(errstr, STATUS_MAX, "%s: nla error building message, index=%i", __func__, i);
			PyErr_SetString(PyExc_ValueError, errstr);
			break;
		}
	}

finally:
	// If we've built a valid frequency list message, indicate this in our return value.
	if (success == 0) {
		success = channel_list_len;
	}
	return success;
}

static PyObject* parse_ies(struct nlattr* ies)
{
	uint8_t *ie = (uint8_t *)nla_data(ies);
	ssize_t ielen = (ssize_t)nla_len(ies);
	uint8_t *ie_end = ie + ielen;
	size_t counter=0;
	PyObject* value = NULL;

	// each IE will be stored in a dict key'd by the ID
	PyObject* ie_dict = PyDict_New();
	if (!ie_dict) {
		// TODO: Throw an exception?
		return NULL;
	}

#if 0
	PyObject* vendor_id = PyLong_FromLong(221);
	if (!vendor_id) {
		// TODO: Throw an exception?
		goto fail;
	}

	PyObject* extention_id = PyLong_FromLong(255);
	if (!extention_id) {
		// TODO: Throw an exception?
		goto fail;
	}
#endif

	while (ie < ie_end) {
	//	printf("%s ie=%d\n", __func__, ie[0]);

		int retcode = 0;
		value = Py_BuildValue("{s:s,s:i}",
								"name", ie_get_name(ie[0]),
								"len", (int)ie[1]
							);
		if (!value) {
			// TODO: Throw an exception?
			goto fail;
		}

#if 0
		PyObject* ie_dict = Py_BuildValue("{s:s},{s:y*}", 
									"name", ie_get_name(ie[0]),
									"value", PyBytes_FromStringAndSize(&ie[2], (int)ie[1])
								);
#endif

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
				// TODO: Throw an exception?
				goto fail;
			}

			PyObject* bytes = PyBytes_FromStringAndSize((const char *)&ie[2], (Py_ssize_t)ie[1]);
			if (!bytes) {
				// TODO: Throw an exception?
				goto fail;
			}

			retcode = PyDict_SetItemString(value, "bytes", bytes);
			Py_CLEAR(bytes);

			if (retcode < 0) {
				// TODO: Throw an exception?
				goto fail;
			}

			PyObject* key = PyLong_FromUnsignedLong(ie[0]);
			if (!key) {
				// TODO: Throw an exception?
				goto fail;
			}

			retcode = PyDict_SetItem(ie_dict, key, value);
			Py_CLEAR(key);

			if (retcode < 0) {
				// TODO: Throw an exception?
				goto fail;
			}
		}
		Py_CLEAR(value);

		ie += ie[1] + 2;
		counter++;
	}

#if 0
	Py_CLEAR(vendor_id);
	Py_CLEAR(extention_id);
#endif

	//	printf("%s found count=%zu IEs\n", __func__, counter);

	return ie_dict;

fail:
	Py_CLEAR(value);
	Py_CLEAR(ie_dict);
#if 0
	Py_CLEAR(vendor_id);
	Py_CLEAR(extention_id);
#endif

	// TODO
	// what else?

	return NULL;
}

static PyObject* parse_bss(struct nlattr* bss[])
{
	int retcode;
	PyObject* bss_dict = PyDict_New();

	if (!bss_dict) {
		// TODO: Throw an exception?
		return NULL;
	}

#if 0
	size_t i;
	for (i=0 ; i<NL80211_BSS_MAX ; i++ ) {
		if (bss[i]) {
			printf("%s bss %zu=%p type=%d len=%d\n", 
					__func__, 
					i, (void *)bss[i], nla_type(bss[i]), nla_len(bss[i]));
		}
	}
#endif

	if (bss[NL80211_BSS_BSSID]) {
		char mac_addr[20];
		uint8_t* ptr = (uint8_t*)nla_data(bss[NL80211_BSS_BSSID]);
		PyOS_snprintf(mac_addr, 20, "%02x:%02x:%02x:%02x:%02x:%02x",
			ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);

		PyObject* bssid_str = Py_BuildValue("s", mac_addr);
		if (!bssid_str) {
			// TODO: Throw an exception?
			goto fail;
		}

		retcode = PyDict_SetItemString(bss_dict, "BSSID", bssid_str);
		Py_CLEAR(bssid_str);

		if (retcode < 0) {
			// TODO: Throw an exception?
			goto fail;
		}
	}
	// nl80211.h enum nl80211_bss
	if (bss[NL80211_BSS_TSF]) {
		uint64_t tsf = nla_get_u64(bss[NL80211_BSS_TSF]);
		retcode = PyDict_SetItemString(bss_dict, "TSF", PyLong_FromUnsignedLongLong(tsf));
		if (retcode) {
			// TODO
		}
	}

	if (bss[NL80211_BSS_FREQUENCY]) {
		uint32_t freq = nla_get_u32(bss[NL80211_BSS_FREQUENCY]);

		PyObject* py_freq = PyLong_FromUnsignedLong(freq);
		if (!py_freq) {
			// TODO: Throw an exception?
			goto fail;
		}

		retcode = PyDict_SetItemString(bss_dict, "frequency", py_freq);
		Py_CLEAR(py_freq);

		if (retcode < 0) {
			// TODO: Throw an exception?
			goto fail;
		}

		PyObject* py_chan = PyLong_FromUnsignedLong(mac80211_freq_to_chan(freq));
		if (!py_chan) {
			// TODO: Throw an exception?
			goto fail;
		}

		retcode = PyDict_SetItemString(bss_dict, "channel", py_chan);
		Py_CLEAR(py_chan);

		if (retcode < 0) {
			// TODO: Throw an exception?
			goto fail;
		}
		// TODO add above/below channel
	}
	if (bss[NL80211_BSS_BEACON_INTERVAL]) {
		// uint16_t beacon_interval = nla_get_u16(bss[NL80211_BSS_BEACON_INTERVAL]);
		// TODO
	}
	if (bss[NL80211_BSS_CAPABILITY]) {
		uint16_t capa = nla_get_u16(bss[NL80211_BSS_CAPABILITY]);

		PyObject* py_capa = PyLong_FromUnsignedLong(capa);
		if (!py_capa) {
			// TODO: Throw an exception?
			goto fail;
		}

		retcode = PyDict_SetItemString(bss_dict, "capability", py_capa);
		Py_CLEAR(py_capa);

		if (retcode < 0) {
			// TODO: Throw an exception?
			goto fail;
		}
	}
	if (bss[NL80211_BSS_SIGNAL_MBM]) {
		// "@NL80211_BSS_SIGNAL_MBM: signal strength of probe response/beacon
		//	in mBm (100 * dBm) (s32)"  (via nl80211.h)
		uint32_t signal = nla_get_u32(bss[NL80211_BSS_SIGNAL_MBM]);

		PyObject* ss = Py_BuildValue("{s:f,s:s}", "value", ((int)signal)/100.0, "units", "dBm");
		if (!ss) {
			// TODO: Throw an exception?
			goto fail;
		}

		retcode = PyDict_SetItemString(bss_dict, "signal_strength", ss);
		Py_CLEAR(ss);

		if (retcode < 0) {
			// TODO: Throw an exception?
			goto fail;
		}
	}
	if (bss[NL80211_BSS_SIGNAL_UNSPEC]) {
		// "@NL80211_BSS_SIGNAL_UNSPEC: signal strength of the probe response/beacon
		//	in unspecified units, scaled to 0..100 (u8)"  (via nl80211.h)
		//	uint8_t signal_unspec = nla_get_u8(bss[NL80211_BSS_SIGNAL_UNSPEC]);
		// TODO
	}
	if (bss[NL80211_BSS_SEEN_MS_AGO]) {
		uint32_t last_seen_ms = nla_get_u32(bss[NL80211_BSS_SEEN_MS_AGO]);

		PyObject* py_last_seen_ms = PyLong_FromUnsignedLong(last_seen_ms);
		if (!py_last_seen_ms) {
			// TODO: Throw an exception?
			goto fail;
		}

		retcode = PyDict_SetItemString(bss_dict, "last_seen_ms", py_last_seen_ms);
		Py_CLEAR(py_last_seen_ms);

		if (retcode < 0) {
			// TODO: Throw an exception?
			goto fail;
		}
	}
	// information can be duplicated between Beacon and Probe Response IEs
	if (bss[NL80211_BSS_INFORMATION_ELEMENTS] ) {
		struct nlattr *ies = bss[NL80211_BSS_INFORMATION_ELEMENTS];

		PyObject* ie_dict = parse_ies(ies);
		if (!ie_dict) {
			// TODO 
			goto fail;
		}
		
		if (PyDict_SetItemString(bss_dict, "ie", ie_dict) < 0) {
			Py_CLEAR(ie_dict);
			goto fail;
		}

		retcode = PyDict_SetItemString(bss_dict, "ie", ie_dict);
		Py_CLEAR(ie_dict);

		if (retcode < 0) {
			// TODO: Throw an exception?
			goto fail;
		}
	}
	// information can be duplicated between Beacon and Probe Response IEs
	if (bss[NL80211_BSS_BEACON_IES] ) {
		// TODO
	}

	return bss_dict;

fail:
	Py_DECREF(bss_dict);
	return NULL;
}

static int nl80211_get_scan_cb(struct nl_msg *msg, void *arg) 
{
	// printf("%s\n", __func__);
	char errstr[STATUS_MAX+1];
	PyObject* network_list = (PyObject*)arg;
	PyObject* bss_dict = NULL;
	PyGILState_STATE gstate = PyGILState_Ensure();  // Protect Py calls

	if (!PyList_Check(network_list)) {
		snprintf(errstr, STATUS_MAX, "%s: bad argument - expected a list of networks", __func__);
		PyErr_SetString(PyExc_TypeError, errstr);
		goto failure;
	}

	struct nlmsghdr *hdr = nlmsg_hdr(msg);
	struct genlmsghdr* gnlh = (struct genlmsghdr*)nlmsg_data(hdr);
	struct nlattr* tb_msg[NL80211_ATTR_MAX + 1];

#if 0
	if (iw_debug) {
		nl_msg_dump(msg,stdout);
		printf("%s nlmsg max size=%ld\n", __func__, nlmsg_get_max_size(msg));
		printf("%s msgsize=%d\n", __func__, nlmsg_datalen(hdr));
		printf("%s genlen=%d genattrlen=%d\n", __func__, genlmsg_len(gnlh), genlmsg_attrlen(gnlh, 0));
	}
#endif

	int retcode = nla_parse(tb_msg, NL80211_ATTR_MAX, 
						genlmsg_attrdata(gnlh, 0),
						genlmsg_attrlen(gnlh, 0), NULL);

	if (retcode != 0) {
		snprintf(errstr, STATUS_MAX, "%s: failed to parse attributes", __func__);
		PyErr_SetString(PyExc_RuntimeError, errstr);
		goto failure;
	}

#if 0
	size_t i;
	for (i=0 ; i<NL80211_ATTR_MAX ; i++ ) {
		if (tb_msg[i]) {
			printf("%s %zu=%p type=%d len=%d\n", 
					__func__, i, (void *)tb_msg[i], nla_type(tb_msg[i]), nla_len(tb_msg[i]));
		}
	}
#endif

	if (tb_msg[NL80211_ATTR_BSS]) {
		struct nlattr *bss[NL80211_BSS_MAX + 1];

		if (nla_parse_nested(bss, NL80211_BSS_MAX,
					 tb_msg[NL80211_ATTR_BSS],
					 bss_policy)) {
			snprintf(errstr, STATUS_MAX, "%s: failed to parse nested attributes", __func__);
			PyErr_SetString(PyExc_RuntimeError, errstr);
			goto failure;
		}

		if (!bss[NL80211_BSS_BSSID]) {
			snprintf(errstr, STATUS_MAX, "%s: missing required field: BSSID", __func__);
			PyErr_SetString(PyExc_RuntimeError, errstr);
			goto failure;
		}

		bss_dict = parse_bss(bss);
		if (!bss_dict) {
			snprintf(errstr, STATUS_MAX, "%s: failed to allocate dictionary", __func__);
			PyErr_SetString(PyExc_MemoryError, errstr);
			goto failure;
		}

		if (PyList_Append(network_list, bss_dict) < 0) {
			snprintf(errstr, STATUS_MAX, "%s: failed to append dictionary to network list", __func__);
			PyErr_SetString(PyExc_RuntimeError, errstr);
			goto failure;
		}
	}
	PyGILState_Release(gstate);
	return NL_SKIP;

failure:
	if (bss_dict) {
		Py_CLEAR(bss_dict);
	}
	PyGILState_Release(gstate);
	return NL_STOP;
}

PyObject* get_scan(PyObject* Py_UNUSED(self), PyObject* args)
{
	const char* interface;
	int retcode;            // subroutine return code
	struct nl_handler *sock;
	int family_id, if_index;
	int cb_err_code = 1;    // callback error code
	char errstr[STATUS_MAX+1];
	struct nl_msg* msg = NULL;
	struct nl_cb* cb = NULL;
	PyObject* network_list = NULL;

	if (!PyArg_ParseTuple(args, "s", &interface)) {
		snprintf(errstr, STATUS_MAX, "%s: failed to parse arguments", __func__);
		PyErr_SetString(PyExc_RuntimeError, errstr);
		return NULL;
	}

	retcode = mac80211_connect(interface, (void**)&sock, &family_id, &if_index, errstr);

	if (retcode != 0) {
		snprintf(errstr, STATUS_MAX, "%s: mac80211 failed to connect to interface '%s': %s", __func__, interface, errstr);
		PyErr_SetString(PyExc_RuntimeError, errstr);
		return NULL;
	}
	//	printf("%s retcode=%d family_id=%d if_index=%d\n", __func__, retcode, family_id, if_index);

	msg = nlmsg_alloc();
	if (!msg) {
		snprintf(errstr, STATUS_MAX, "%s: failed to allocate netlink message", __func__);
		PyErr_SetString(PyExc_MemoryError, errstr);
		goto finally;
	}
//	cb = nl_cb_alloc(NL_CB_DEFAULT);
	cb = nl_cb_alloc(NL_CB_DEBUG);
	if (!cb) {
		snprintf(errstr, STATUS_MAX, "%s: failed to allocate netlink cb", __func__);
		PyErr_SetString(PyExc_MemoryError, errstr);
		goto finally;
	}

	network_list = PyList_New(0);
	if (!network_list) {
		snprintf(errstr, STATUS_MAX, "%s: failed to allocate network list", __func__);
		PyErr_SetString(PyExc_MemoryError, errstr);
		goto finally;
	}

	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, nl80211_get_scan_cb, network_list);
	nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, nl80211_ack_cb, &cb_err_code);
	nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, nl80211_finish_cb, &cb_err_code);
	nl_cb_err(cb, NL_CB_CUSTOM, nl80211_error_cb, &cb_err_code);

	genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, family_id, 0, NLM_F_DUMP, NL80211_CMD_GET_SCAN, 0);
	nla_put_u32(msg, NL80211_ATTR_IFINDEX, if_index); // Add msg attribute: which i/f to use

	if (nl_send_auto_complete((struct nl_sock *) sock, msg) < 0) {
		snprintf(errstr, STATUS_MAX, "%s: failed to send netlink command to interface '%s'", __func__, interface);
		PyErr_SetString(PyExc_RuntimeError, errstr);
		Py_CLEAR (network_list);  // Free memory & return NULL on exception
		goto finally;
	}

	Py_BEGIN_ALLOW_THREADS
	while (cb_err_code > 0) {
		nl_recvmsgs((struct nl_sock *) sock, cb);
	}
	Py_END_ALLOW_THREADS

	if (cb_err_code < 0) {
		snprintf(errstr, STATUS_MAX, "%s: netlink command failed with callback error: %d", __func__, cb_err_code);
		PyErr_SetString(PyExc_RuntimeError, errstr);
		Py_CLEAR (network_list);  // Free memory & return NULL on exception
		goto finally;
	}

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


PyObject* request_scan(PyObject* Py_UNUSED(self), PyObject* args)
{
	PyObject* py_response = NULL;	// response from this function

	bool active_scan = false;
	const char* interface;
	PyObject* ssidlist=NULL;
	PyObject* chanlist=NULL;

	int retcode;					// subroutine return code
	struct nl_sock *sock;
	int family_id, if_index;
	char errstr[STATUS_MAX+1];

	int cb_err_code = 1;			// callback error code
	int nl_flags = 0;				// see 'man 7 netlink' for details on netlink flags

	struct nl_cb* cb = NULL;
	struct nl_msg* cmd_msg = NULL;
	struct nl_msg* ssid_list_msg = NULL;
	struct nl_msg* freq_list_msg = NULL;

	if (!PyArg_ParseTuple(args, "ps|OO", &active_scan, &interface, &ssidlist, &chanlist)) {
		snprintf(errstr, STATUS_MAX, "%s: failed to parse arguments", __func__);
		PyErr_SetString(PyExc_RuntimeError, errstr);
		return NULL;
	}

	retcode = mac80211_connect(interface, (void**)&sock, &family_id, &if_index, errstr);

	if (retcode != 0) {
		snprintf(errstr, STATUS_MAX, "%s: failed to connect to interface '%s': %s", __func__, interface, errstr);
		PyErr_SetString(PyExc_RuntimeError, errstr);
		return NULL;
	}

	// Disable libnl message sequence checking.  Don't need it for this command.
	nl_socket_disable_seq_check(sock);

	cmd_msg = nlmsg_alloc();
	if (!cmd_msg) {
		snprintf(errstr, STATUS_MAX, "%s: failed to allocate netlink message for command", __func__);
		PyErr_SetString(PyExc_MemoryError, errstr);
		goto finally;
	}

	nl_flags  = NLM_F_REQUEST;	// This is a request
	nl_flags |= NLM_F_ACK;		// We want an ACK on success
	nl_flags |= NLM_F_CREATE;	// Create request object if it doesn't already exist
	nl_flags |= NLM_F_EXCL;		// Don't replace the object if it already exists

	genlmsg_put(cmd_msg, NL_AUTO_PORT, NL_AUTO_SEQ, family_id, 0, nl_flags, NL80211_CMD_TRIGGER_SCAN, 0);
	nla_put_u32(cmd_msg, NL80211_ATTR_IFINDEX, if_index); // Add msg attribute: which i/f to use

	// Per usr/iw/nl80211.h: NL80211_ATTR_SCAN_SSIDS: nested attribute with SSIDs.
	// Leave out for passive scanning and include a zero-length SSID (wildcard) for wildcard scan.
	// If no SSID is passed, no probe requests are sent and a passive scan is performed.

	if (active_scan) {
		ssid_list_msg = nlmsg_alloc();
		if (!ssid_list_msg) {
			snprintf(errstr, STATUS_MAX, "%s: failed to allocate netlink message for SSID list", __func__);
			PyErr_SetString(PyExc_MemoryError, errstr);
			goto finally;
		}

		retcode = build_ssid_list_msg(ssidlist, ssid_list_msg);

		if (retcode < 0) {
			// Expect the subroutine to have already set the Py error
			goto finally;
		}
		else {
			// Add msg attribute: list of SSIDs to scan
			nla_put_nested(cmd_msg, NL80211_ATTR_SCAN_SSIDS, ssid_list_msg);
		}
		nlmsg_free(ssid_list_msg);
		ssid_list_msg = NULL;
	}
	// else, do a PASSIVE scan on all SSID's.

	// Per usr/iw/nl80211.h: NL80211_ATTR_SCAN_FREQUENCIES, if passed, define which channels should be scanned;
	// if not passed, all channels allowed for the current regulatory domain are used.
	// [Note: There is no "wildcard" option to indicate 'all' supported channels.]

	if (chanlist) {
		freq_list_msg = nlmsg_alloc();
		if (!freq_list_msg) {
			snprintf(errstr, STATUS_MAX, "%s: failed to allocate netlink message for frequency list", __func__);
			PyErr_SetString(PyExc_MemoryError, errstr);
			goto finally;
		}

		retcode = build_freq_list_msg(chanlist, freq_list_msg);

		if (retcode < 0) {
			// Expect the subroutine to have already set the Py error
			goto finally;
		}
		else if (retcode > 0) {
			// Add msg attribute: list of channels/frequencies to scan
			nla_put_nested(cmd_msg, NL80211_ATTR_SCAN_FREQUENCIES, freq_list_msg);
		}
		nlmsg_free(freq_list_msg);
		freq_list_msg = NULL;
	}
	// else scan all channels supported by the current reg domain.

	if (nl_send_auto_complete(sock, cmd_msg) < 0) {
		snprintf(errstr, STATUS_MAX, "%s: failed to send netlink command to interface '%s'", __func__, interface);
		PyErr_SetString(PyExc_RuntimeError, errstr);
		goto finally;
	}

	cb = nl_cb_alloc(NL_CB_DEFAULT);
	if (!cb) {
		snprintf(errstr, STATUS_MAX, "%s: failed to allocate netlink callbacks", __func__);
		PyErr_SetString(PyExc_MemoryError, errstr);
		goto finally;
	}

	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, nl80211_trigger_scan_cb, &cb_err_code);
	nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, nl80211_ack_cb, &cb_err_code);
	nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, nl80211_finish_cb, &cb_err_code);
	nl_cb_set(cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, nl80211_no_seq_check_cb, NULL);
	nl_cb_err(cb, NL_CB_CUSTOM, nl80211_error_cb, &cb_err_code);
	
	cb_err_code = 1;

	Py_BEGIN_ALLOW_THREADS
	while (cb_err_code > 0) {
		nl_recvmsgs(sock, cb);
	}
	Py_END_ALLOW_THREADS

	if (cb_err_code < 0) {
		snprintf(errstr, STATUS_MAX, "%s: netlink command failed with callback error: %d", __func__, cb_err_code);
		PyErr_SetString(PyExc_RuntimeError, errstr);
		goto finally;
	}

	py_response = Py_None; // None == 'success'
	Py_INCREF(Py_None);

finally:
	mac80211_disconnect(sock);

	if (freq_list_msg) {
		nlmsg_free(freq_list_msg);
	}

	if (ssid_list_msg) {
		nlmsg_free(ssid_list_msg);
	}

	if (cb) {
		nl_cb_put(cb);
	}

	if (cmd_msg) {
		nlmsg_free(cmd_msg);
	}

	return py_response;
}

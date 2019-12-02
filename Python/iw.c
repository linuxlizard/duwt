#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <linux/nl80211.h>

#include "linux_netlink_control.h"
#include "iw.h"
#include "scan.h"
#include "event.h"


static PyObject *
iw_get_chanlist(PyObject* Py_UNUSED(self), PyObject *args)
{
	const char* interface;
	int ret;
	char errstr[STATUS_MAX+1];

	if (!PyArg_ParseTuple(args, "s", &interface))
		return NULL;

	size_t num_chans;
	char **chanlist;

	ret = mac80211_get_chanlist(
				interface,
				0,
				errstr,
				0,
				0,
				&chanlist,
				&num_chans
				);
	if (ret < 0) {
		PyErr_SetString(PyExc_Exception, "get chanlist failed");
		return NULL;
	}


	PyObject* py_chanlist = PyList_New(num_chans);
	if (!py_chanlist) {
		return PyErr_NoMemory();
	}

	size_t i;
	for (i=0 ; i<num_chans ; i++ ) {
		ret = PyList_SetItem(py_chanlist, i, PyLong_FromString(chanlist[i], NULL, 10));
	}

	return py_chanlist;
}

static PyMethodDef IW_Methods[] = {
	{"get_chanlist", iw_get_chanlist, METH_VARARGS, "Get channel list"},
	{"request_scan", request_scan, METH_VARARGS, "Start a scan"},
	{"get_scan", get_scan, METH_VARARGS, "Get scan results"},
	{"event_recv", start_event_listen, METH_VARARGS, "Monitor & report events"},

	{NULL, NULL, 0, NULL}		/* Sentinel */
};

PyDoc_STRVAR(iw_doc,
"Implementation module for nl80211 operations.\n\
\n\
See the nl80211 module for documentation.");


static struct PyModuleDef iw_module = {
	PyModuleDef_HEAD_INIT,
	"iw",   /* name of module */
	iw_doc, /* module documentation, may be NULL */
	-1,	   /* size of per-interpreter state of the module,
				 or -1 if the module keeps state in global variables. */
	IW_Methods,
	NULL,
	NULL,
	NULL,
	NULL
};


PyMODINIT_FUNC
PyInit__iw(void)
{
	PyObject* m = PyModule_Create(&iw_module);
	
	if (!m) {
		return NULL;
	}
	PyModule_AddIntMacro(m, WLAN_CAPABILITY_ESS);
	PyModule_AddIntMacro(m, WLAN_CAPABILITY_IBSS);
	PyModule_AddIntMacro(m, WLAN_CAPABILITY_CF_POLLABLE);
	PyModule_AddIntMacro(m, WLAN_CAPABILITY_CF_POLL_REQUEST);
	PyModule_AddIntMacro(m, WLAN_CAPABILITY_PRIVACY);
	PyModule_AddIntMacro(m, WLAN_CAPABILITY_SHORT_PREAMBLE);
	PyModule_AddIntMacro(m, WLAN_CAPABILITY_PBCC);
	PyModule_AddIntMacro(m, WLAN_CAPABILITY_CHANNEL_AGILITY);
	PyModule_AddIntMacro(m, WLAN_CAPABILITY_SPECTRUM_MGMT);
	PyModule_AddIntMacro(m, WLAN_CAPABILITY_QOS);
	PyModule_AddIntMacro(m, WLAN_CAPABILITY_SHORT_SLOT_TIME);
	PyModule_AddIntMacro(m, WLAN_CAPABILITY_APSD);
	PyModule_AddIntMacro(m, WLAN_CAPABILITY_RADIO_MEASURE);
	PyModule_AddIntMacro(m, WLAN_CAPABILITY_DSSS_OFDM);
	PyModule_AddIntMacro(m, WLAN_CAPABILITY_DEL_BACK);
	PyModule_AddIntMacro(m, WLAN_CAPABILITY_IMM_BACK);

	return m;
}

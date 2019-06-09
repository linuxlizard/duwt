#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <linux/nl80211.h>

#include "linux_netlink_control.h"
#include "scan.h"

static PyObject *IW_Error;

#if 0
static PyObject *
spam_system(PyObject *self, PyObject *args)
{
	const char *command;
	int sts;

	if (!PyArg_ParseTuple(args, "s", &command))
		return NULL;
	sts = system(command);
	return PyLong_FromLong(sts);
}



PyMODINIT_FUNC
PyInit_spam(void)
{
	PyObject *m;

	m = PyModule_Create(&spammodule);
	if (m == NULL)
		return NULL;

	SpamError = PyErr_NewException("spam.error", NULL, NULL);
	Py_INCREF(SpamError);
	PyModule_AddObject(m, "error", SpamError);
	return m;
}
#endif

static PyObject *
iw_hello(PyObject* Py_UNUSED(self), PyObject *args)
{
	const char *command;
	int sts=0;

	if (!PyArg_ParseTuple(args, "s", &command))
		return NULL;
	return Py_BuildValue("y", "hello");
	if (sts < 0) {
		PyErr_SetString(IW_Error, "System command failed");
		return NULL;
	}
	return PyLong_FromLong(sts);
}

static PyObject *
iw_get_chanlist(PyObject* Py_UNUSED(self), PyObject *args)
{
	const char* interface;
	int ret;
	char errstr[STATUS_MAX+1];

	if (!PyArg_ParseTuple(args, "s", &interface))
		return NULL;

	size_t num_chans;
	char **chan_list;

	ret = mac80211_get_chanlist(
				interface,
				0,
				errstr,
				0,
				0,
				&chan_list,
				&num_chans
				);
	
	printf("ret=%d\n", ret);
	printf("num_chans=%zu\n", num_chans);

	PyObject* chanlist = PyList_New(num_chans);
	if (!chanlist) {
		return PyErr_NoMemory();
	}

	size_t i;
	for (i=0 ; i<num_chans ; i++ ) {
		ret = PyList_SetItem(chanlist, i, PyLong_FromString(chan_list[i], NULL, 10));
	}

//int mac80211_get_chanlist(const char *interface, unsigned int extended_flags, char *errstr,
//        unsigned int default_ht20, unsigned int expand_ht20,
//        char ***ret_chanlist, size_t *ret_chanlist_len);

	return chanlist;
}

static PyMethodDef IW_Methods[] = {
	{"hello",  iw_hello, METH_VARARGS, "Hello, world"},
	{"get_chanlist",  iw_get_chanlist, METH_VARARGS, "Hello, world"},
	{"get_scan",  get_scan, METH_VARARGS, "Hello, world"},

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
PyInit_iw(void)
{
	return PyModule_Create(&iw_module);
}

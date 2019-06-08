#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <linux/nl80211.h>

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
iw_hello(PyObject *self, PyObject *args)
{
	const char *command;
	int sts;

	if (!PyArg_ParseTuple(args, "s", &command))
		return NULL;
//	sts = system(command);
	return Py_BuildValue("y", "hello");
	if (sts < 0) {
		PyErr_SetString(IW_Error, "System command failed");
		return NULL;
	}
	return PyLong_FromLong(sts);
}


static PyMethodDef IW_Methods[] = {
	{"hello",  iw_hello, METH_VARARGS, "Hello, world"},

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
	IW_Methods 
};


PyMODINIT_FUNC
PyInit_iw(void)
{
	return PyModule_Create(&iw_module);
}

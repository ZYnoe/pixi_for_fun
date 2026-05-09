#include <Python.h>

static PyObject* say_hello(PyObject* self, PyObject* args) {
    return PyUnicode_FromString("Hello from C, imported by Python!");
}

static PyMethodDef CHelloMethods[] = {
    {"say_hello", say_hello, METH_NOARGS, "Return a hello message from C."},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef c_hello_module = {
    PyModuleDef_HEAD_INIT,
    "_c_hello",
    "A tiny C extension module.",
    -1,
    CHelloMethods
};

PyMODINIT_FUNC PyInit__c_hello(void) {
    return PyModule_Create(&c_hello_module);
}

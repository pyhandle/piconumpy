#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "structmember.h"

#include "hpy.h"

typedef struct {
  PyObject_HEAD
      /* Type-specific fields go here. */
      double *data;
  int size;
} ArrayObject;

static void Array_dealloc(ArrayObject *self) {
  free(self->data);
  Py_TYPE(self)->tp_free((PyObject *)self);
}

static int Array_init(ArrayObject *self, PyObject *args, PyObject *kwds) {
  static char *kwlist[] = {"data", NULL};
  int index;
  PyObject *data = NULL, *item;

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O", kwlist, &data))
    return -1;

  if (!PyList_Check(data)) {
    PyErr_SetString(PyExc_TypeError, "parameter must be a list");
    return -1;
  }

  self->size = (int)PyList_Size(data);

  self->data = (double *)malloc(self->size * sizeof(double));
  if (self->data == NULL) {
    PyErr_NoMemory();
    return -1;
  }

  for (index = 0; index < self->size; index++) {
    item = PyList_GET_ITEM(data, index);
    self->data[index] = PyFloat_AsDouble(item);
  }

  return 0;
}

static PyMemberDef Array_members[] = {
    {"size", T_INT, offsetof(ArrayObject, size), 0, "size of the array"},
    {NULL} /* Sentinel */
};

static PyObject *Array_tolist(ArrayObject *self, PyObject *Py_UNUSED(ignored)) {
  int index;
  PyObject *result, *item;
  result = PyList_New(self->size);
  for (index = 0; index < self->size; index++) {
    item = PyFloat_FromDouble(self->data[index]);
    PyList_SetItem(result, index, item);
  }
  return result;
};

static ArrayObject *Array_empty(int size);

static ArrayObject *Array_multiply(PyObject *o1, PyObject *o2) {
  int index;
  double number;
  PyObject *obj_number = NULL;
  ArrayObject *result = NULL, *arr = NULL;

  if (PyNumber_Check(o2)) {
    obj_number = o2;
    arr = (ArrayObject *)o1;
  } else if (PyNumber_Check(o1)) {
    obj_number = o1;
    arr = (ArrayObject *)o2;
  }

  if (PyNumber_Check(o1) | PyNumber_Check(o2)) {
    number = PyFloat_AsDouble(obj_number);
    result = Array_empty(arr->size);
    for (index = 0; index < arr->size; index++) {
      result->data[index] = arr->data[index] * number;
    }
  }

  return result;
};

static ArrayObject *Array_add(PyObject *o1, PyObject *o2) {
  int index;
  ArrayObject *result = NULL, *a1, *a2;
  a1 = (ArrayObject *)o1;
  a2 = (ArrayObject *)o2;

  if (a1->size != a2->size)
    return result;

  result = Array_empty(a1->size);
  for (index = 0; index < a1->size; index++) {
    result->data[index] = a1->data[index] + a2->data[index];
  }

  return result;
};

static ArrayObject *Array_divide(PyObject *o1, PyObject *o2) {
  int index;
  double number;
  ArrayObject *result = NULL, *a1;

  if (!PyNumber_Check(o2)) {
    return result;
  }
  a1 = (ArrayObject *)o1;
  number = PyFloat_AsDouble(o2);
  result = Array_empty(a1->size);
  for (index = 0; index < a1->size; index++) {
    result->data[index] = a1->data[index] / number;
  }

  return result;
};

Py_ssize_t Array_length(ArrayObject *arr) {
  Py_ssize_t result = (Py_ssize_t)arr->size;
  return result;
};

PyObject *Array_item(ArrayObject *arr, Py_ssize_t index) {
  PyObject *item = NULL;
  if (index < 0 || index >= arr->size) {
    return item;
  }
  item = PyFloat_FromDouble(arr->data[index]);
  return item;
};

static PyMethodDef Array_methods[] = {
    {"tolist", (PyCFunction)Array_tolist, METH_NOARGS,
     "Return the data as a list"},
    {NULL} /* Sentinel */
};

static PyType_Slot Array_type_slots[] = {
    {Py_tp_new, PyType_GenericNew},
    {Py_tp_init, (initproc)Array_init},
    {Py_tp_dealloc, (destructor)Array_dealloc},
    {Py_tp_members, Array_members},
    {Py_tp_methods, Array_methods},
    {Py_nb_multiply, (binaryfunc)Array_multiply},
    {Py_nb_add, (binaryfunc)Array_add},
    {Py_nb_true_divide, (binaryfunc)Array_divide},
    {Py_sq_length, (lenfunc)Array_length},
    {Py_sq_item, (ssizeargfunc)Array_item},
    {0, NULL},
};

static HPyType_Spec Array_type_spec = {
    .name = "_piconumpy_hpy.array",
    .basicsize = sizeof(ArrayObject),
    .itemsize = 0,
    .flags = HPy_TPFLAGS_DEFAULT,
    .legacy_slots = Array_type_slots,
};

PyTypeObject *ptr_ArrayType;
HPy h_ArrayType;

static ArrayObject *Array_empty(int size) {
  ArrayObject *new_array = NULL;
  new_array = PyObject_New(ArrayObject, ptr_ArrayType);
  new_array->size = size;
  new_array->data = (double *)malloc(size * sizeof(double));
  if (new_array->data == NULL) {
     PyErr_NoMemory();
     return NULL;
  }
  return new_array;
};

/* XXX add the docstring: "Create an empty array" */
HPyDef_METH(empty, "empty", empty_impl, HPyFunc_O)
static HPy empty_impl(HPyContext ctx, HPy module, HPy arg) {
  int size;
  size = (int)HPyLong_AsLong(ctx, arg);
  PyObject *result = (PyObject *)Array_empty(size);
  HPy h_result = HPy_FromPyObject(ctx, result);
  Py_DECREF(result);
  return h_result;
};


static HPyDef *module_defines[] = {
    &empty,
    NULL
};

static HPyModuleDef piconumpymodule = {
    HPyModuleDef_HEAD_INIT,
    .m_name = "_piconumpy_hpy",
    .m_doc = "piconumpy implemented with the HPy API.",
    .m_size = -1,
    .defines = module_defines,
};

HPy_MODINIT(_piconumpy_hpy)
static HPy init__piconumpy_hpy_impl(HPyContext ctx) {
  HPy hm = HPyModule_Create(ctx, &piconumpymodule);
  if (HPy_IsNull(hm))
    return HPy_NULL;

  h_ArrayType = HPyType_FromSpec(ctx, &Array_type_spec);
  if (HPy_IsNull(h_ArrayType))
      return HPy_NULL;
  ptr_ArrayType = (PyTypeObject *)HPy_AsPyObject(ctx, h_ArrayType);

  if (HPy_SetAttr_s(ctx, hm, "array", h_ArrayType) < 0) {
    HPy_Close(ctx, h_ArrayType);
    Py_DECREF(ptr_ArrayType);
    HPy_Close(ctx, hm);
    return HPy_NULL;
  }

  return hm;
}
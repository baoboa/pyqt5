// This contains the implementation of the QQmlListPropertyWrapper type.
//
// Copyright (c) 2016 Riverbank Computing Limited <info@riverbankcomputing.com>
// 
// This file is part of PyQt5.
// 
// This file may be used under the terms of the GNU General Public License
// version 3.0 as published by the Free Software Foundation and appearing in
// the file LICENSE included in the packaging of this file.  Please review the
// following information to ensure the GNU General Public License version 3.0
// requirements will be met: http://www.gnu.org/copyleft/gpl.html.
// 
// If you do not wish to use this file under the terms of the GPL version 3.0
// then you may purchase a commercial license.  For more information contact
// info@riverbankcomputing.com.
// 
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.


#include <Python.h>

#include <sip.h>

#include "qpyqmllistpropertywrapper.h"


// Forward declarations.
extern "C" {
static void QQmlListPropertyWrapper_dealloc(PyObject *self);
static Py_ssize_t QQmlListPropertyWrapper_sq_length(PyObject *self);
static PyObject *QQmlListPropertyWrapper_sq_concat(PyObject *self,
        PyObject *other);
static PyObject *QQmlListPropertyWrapper_sq_repeat(PyObject *self,
        Py_ssize_t count);
static PyObject *QQmlListPropertyWrapper_sq_item(PyObject *self, Py_ssize_t i);
#if PY_MAJOR_VERSION < 3
static PyObject *QQmlListPropertyWrapper_sq_slice(PyObject *self,
        Py_ssize_t i1, Py_ssize_t i2);
#endif
static int QQmlListPropertyWrapper_sq_ass_item(PyObject *self, Py_ssize_t i,
        PyObject *value);
#if PY_MAJOR_VERSION < 3
static int QQmlListPropertyWrapper_sq_ass_slice(PyObject *self, Py_ssize_t i1,
        Py_ssize_t i2, PyObject *value);
#endif
static int QQmlListPropertyWrapper_sq_contains(PyObject *self,
        PyObject *value);
static PyObject *QQmlListPropertyWrapper_sq_inplace_concat(PyObject *self,
        PyObject *other);
static PyObject *QQmlListPropertyWrapper_sq_inplace_repeat(PyObject *self,
        Py_ssize_t count);
}

static PyObject *get_list(PyObject *self);


// The sequence methods.
PySequenceMethods QQmlListPropertyWrapper_as_sequence = {
    QQmlListPropertyWrapper_sq_length,
    QQmlListPropertyWrapper_sq_concat,
    QQmlListPropertyWrapper_sq_repeat,
    QQmlListPropertyWrapper_sq_item,
#if PY_MAJOR_VERSION < 3
    QQmlListPropertyWrapper_sq_slice,
#else
    0,
#endif
    QQmlListPropertyWrapper_sq_ass_item,
#if PY_MAJOR_VERSION < 3
    QQmlListPropertyWrapper_sq_ass_slice,
#else
    0,
#endif
    QQmlListPropertyWrapper_sq_contains,
    QQmlListPropertyWrapper_sq_inplace_concat,
    QQmlListPropertyWrapper_sq_inplace_repeat,
};

// The QQmlListPropertyWrapper type object.
PyTypeObject qpyqml_QQmlListPropertyWrapper_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
#if PY_VERSION_HEX >= 0x02050000
    "PyQt5.QtQml.QQmlListPropertyWrapper",  /* tp_name */
#else
    const_cast<char *>("PyQt5.QtQml.QQmlListPropertyWrapper"),  /* tp_name */
#endif
    sizeof (qpyqml_QQmlListPropertyWrapper),    /* tp_basicsize */
    0,                      /* tp_itemsize */
    QQmlListPropertyWrapper_dealloc,    /* tp_dealloc */
    0,                      /* tp_print */
    0,                      /* tp_getattr */
    0,                      /* tp_setattr */
    0,                      /* tp_compare */
    0,                      /* tp_repr */
    0,                      /* tp_as_number */
    &QQmlListPropertyWrapper_as_sequence,   /* tp_as_sequence */
    0,                      /* tp_as_mapping */
    0,                      /* tp_hash */
    0,                      /* tp_call */
    0,                      /* tp_str */
    0,                      /* tp_getattro */
    0,                      /* tp_setattro */
    0,                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,     /* tp_flags */
    0,                      /* tp_doc */
    0,                      /* tp_traverse */
    0,                      /* tp_clear */
    0,                      /* tp_richcompare */
    0,                      /* tp_weaklistoffset */
    0,                      /* tp_iter */
    0,                      /* tp_iternext */
    0,                      /* tp_methods */
    0,                      /* tp_members */
    0,                      /* tp_getset */
    0,                      /* tp_base */
    0,                      /* tp_dict */
    0,                      /* tp_descr_get */
    0,                      /* tp_descr_set */
    0,                      /* tp_dictoffset */
    0,                      /* tp_init */
    0,                      /* tp_alloc */
    0,                      /* tp_new */
    0,                      /* tp_free */
    0,                      /* tp_is_gc */
    0,                      /* tp_bases */
    0,                      /* tp_mro */
    0,                      /* tp_cache */
    0,                      /* tp_subclasses */
    0,                      /* tp_weaklist */
    0,                      /* tp_del */
    0,                      /* tp_version_tag */
#if PY_VERSION_HEX >= 0x03040000
    0,                      /* tp_finalize */
#endif
};


// Create the wrapper object.
PyObject *qpyqml_QQmlListPropertyWrapper_New(QQmlListProperty<QObject> *prop,
        PyObject *list)
{
    qpyqml_QQmlListPropertyWrapper *obj;

    obj = PyObject_NEW(qpyqml_QQmlListPropertyWrapper,
            &qpyqml_QQmlListPropertyWrapper_Type);

    if (!obj)
        return 0;

    obj->qml_list_property = prop;
    obj->py_list = list;

    return (PyObject *)obj;
}


// The type dealloc slot.
static void QQmlListPropertyWrapper_dealloc(PyObject *self)
{
    delete ((qpyqml_QQmlListPropertyWrapper *)self)->qml_list_property;

    PyObject_DEL(self);
}


// Return the underlying list.  Return 0 and raise an exception if there wasn't
// one.
static PyObject *get_list(PyObject *self)
{
    PyObject *list = ((qpyqml_QQmlListPropertyWrapper *)self)->py_list;

    if (!list)
    {
        PyErr_SetString(PyExc_TypeError,
                "there is no object bound to QQmlListProperty");
        return 0;
    }

    // Make sure it has sequence methods.
    if (!list->ob_type->tp_as_sequence)
    {
        PyErr_SetString(PyExc_TypeError,
                "object bound to QQmlListProperty is not a sequence");
        return 0;
    }

    return list;
}


// The proxy sequence methods.

static Py_ssize_t QQmlListPropertyWrapper_sq_length(PyObject *self)
{
    PyObject *list = get_list(self);

    if (!list)
        return -1;

    return list->ob_type->tp_as_sequence->sq_length(list);
}

static PyObject *QQmlListPropertyWrapper_sq_concat(PyObject *self,
        PyObject *other)
{
    PyObject *list = get_list(self);

    if (!list)
        return 0;

    return list->ob_type->tp_as_sequence->sq_concat(list, other);
}

static PyObject *QQmlListPropertyWrapper_sq_repeat(PyObject *self,
        Py_ssize_t count)
{
    PyObject *list = get_list(self);

    if (!list)
        return 0;

    return list->ob_type->tp_as_sequence->sq_repeat(list, count);
}

static PyObject *QQmlListPropertyWrapper_sq_item(PyObject *self, Py_ssize_t i)
{
    PyObject *list = get_list(self);

    if (!list)
        return 0;

    return list->ob_type->tp_as_sequence->sq_item(list, i);
}

#if PY_MAJOR_VERSION < 3
static PyObject *QQmlListPropertyWrapper_sq_slice(PyObject *self,
        Py_ssize_t i1, Py_ssize_t i2)
{
    PyObject *list = get_list(self);

    if (!list)
        return 0;

    return list->ob_type->tp_as_sequence->sq_slice(list, i1, i2);
}
#endif

static int QQmlListPropertyWrapper_sq_ass_item(PyObject *self, Py_ssize_t i,
        PyObject *value)
{
    PyObject *list = get_list(self);

    if (!list)
        return -1;

    return list->ob_type->tp_as_sequence->sq_ass_item(list, i, value);
}

#if PY_MAJOR_VERSION < 3
static int QQmlListPropertyWrapper_sq_ass_slice(PyObject *self, Py_ssize_t i1,
        Py_ssize_t i2, PyObject *value)
{
    PyObject *list = get_list(self);

    if (!list)
        return -1;

    return list->ob_type->tp_as_sequence->sq_ass_slice(list, i1, i2, value);
}
#endif

static int QQmlListPropertyWrapper_sq_contains(PyObject *self, PyObject *value)
{
    PyObject *list = get_list(self);

    if (!list)
        return -1;

    return list->ob_type->tp_as_sequence->sq_contains(list, value);
}

static PyObject *QQmlListPropertyWrapper_sq_inplace_concat(PyObject *self,
        PyObject *other)
{
    PyObject *list = get_list(self);

    if (!list)
        return 0;

    return list->ob_type->tp_as_sequence->sq_inplace_concat(list, other);
}

static PyObject *QQmlListPropertyWrapper_sq_inplace_repeat(PyObject *self,
        Py_ssize_t count)
{
    PyObject *list = get_list(self);

    if (!list)
        return 0;

    return list->ob_type->tp_as_sequence->sq_inplace_repeat(list, count);
}

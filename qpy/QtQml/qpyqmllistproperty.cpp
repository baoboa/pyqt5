// This is the implementation of the QQmlListProperty class.
//
// Copyright (c) 2014 Riverbank Computing Limited <info@riverbankcomputing.com>
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

#include <QObject>
#include <QQmlListProperty>

#include "qpyqmllistproperty.h"
#include "qpyqml_listdata.h"

#include "sipAPIQtQml.h"


// Forward declarations.
extern "C" {
static PyObject *QQmlListProperty_call(PyObject *, PyObject *args,
        PyObject *kwds);
}

static void list_append(QQmlListProperty<QObject> *p, QObject *el);
static QObject *list_at(QQmlListProperty<QObject> *p, int idx);
static void list_clear(QQmlListProperty<QObject> *p);
static int list_count(QQmlListProperty<QObject> *p);
static void bad_result(PyObject *py_res, const char *context);


// The type's doc-string.
PyDoc_STRVAR(QQmlListProperty_doc,
"QQmlListProperty(type, object, list)\n"
"QQmlListProperty(type, object, append=None, count=None, at=None, clear=None)");


// This implements the QQmlListProperty Python type.  It is a sub-type of the
// standard string type that is callable.
PyTypeObject qpyqml_QQmlListProperty_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    SIP_TPNAME_CAST("PyQt5.QtQml.QQmlListProperty"),
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    QQmlListProperty_call,
    0,
    0,
    0,
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|
#if PY_MAJOR_VERSION >= 3
    Py_TPFLAGS_UNICODE_SUBCLASS,
#else
    Py_TPFLAGS_STRING_SUBCLASS,
#endif
    QQmlListProperty_doc,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
#if PY_VERSION_HEX >= 0x03040000
    0,
#endif
};


// The QQmlListProperty init function.
static PyObject *QQmlListProperty_call(PyObject *, PyObject *args,
        PyObject *kwds)
{
    PyObject *py_type, *py_obj, *py_list = 0, *py_append = 0, *py_count = 0,
            *py_at = 0, *py_clear = 0;

    static const char *kwlist[] = {"type", "object", "list", "append", "count",
            "at", "clear", 0};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "OO|O!OOOO:QQmlListProperty",
            const_cast<char **>(kwlist), &py_type, &py_obj, &PyList_Type,
            &py_list, &py_append, &py_count, &py_at, &py_clear))
        return 0;

    // Check the type object.
    if (!PyObject_TypeCheck(py_type, &PyType_Type))
    {
        PyErr_Format(PyExc_TypeError,
                "type argument must be of type 'type', not '%s'",
                Py_TYPE(py_type)->tp_name);
        return 0;
    }

    // Get the C++ QObject.
    int iserr = 0;
    QObject *obj = reinterpret_cast<QObject *>(sipForceConvertToType(py_obj,
            sipType_QObject, 0, SIP_NOT_NONE|SIP_NO_CONVERTORS, 0, &iserr));

    if (iserr)
    {
        PyErr_Format(PyExc_TypeError,
                "object argument must be of type 'QObject', not '%s'",
                Py_TYPE(py_obj)->tp_name);
        return 0;
    }

    // If we have a list then check we have no callables.
    if (py_list && (py_append || py_count || py_at || py_clear))
    {
        PyErr_SetString(PyExc_TypeError,
                "cannot specify a list and a list function");
        return 0;
    }

    // Get a list wrapper with the C++ QObject as its parent.
    ListData *list_data = new ListData(py_type, py_obj, py_list, py_append,
            py_count, py_at, py_clear, obj);

    // Create the C++ QDeclarativeListProperty<QObject> with the list data as
    // the data.  Note that we will create a new one each time the property
    // getter is called - however, under normal usage, this should only be
    // once.  Also note that the callables will not be reached by the garbage
    // collector.
    QQmlListProperty<QObject> *prop = new QQmlListProperty<QObject>(obj,
            list_data, ((py_list || py_append) ? list_append : 0),
            ((py_list || py_count) ? list_count : 0),
            ((py_list || py_at) ? list_at : 0),
            ((py_list || py_clear) ? list_clear : 0));

    // Convert it to a Python object.
    static const sipTypeDef *mapped_type = 0;

    if (!mapped_type)
        mapped_type = sipFindType("QQmlListProperty<QObject>");

    Q_ASSERT(mapped_type);

    // Make sure ownership is with Python.
    PyObject *prop_obj = sipConvertFromNewType(prop, mapped_type, py_obj);

    if (!prop_obj)
    {
        delete prop;
        return 0;
    }

    return prop_obj;
}


// Append to the list.
static void list_append(QQmlListProperty<QObject> *p, QObject *el)
{
    SIP_BLOCK_THREADS

    ListData *ldata = reinterpret_cast<ListData *>(p->data);
    bool ok = false;

    // Convert the element to a Python object and check the type.
    PyObject *py_el = sipConvertFromType(el, sipType_QObject, 0);

    if (py_el)
    {
        if (!PyObject_TypeCheck(py_el, (PyTypeObject *)ldata->py_type))
        {
            PyErr_Format(PyExc_TypeError,
                "list element must be of type '%s', not '%s'",
                    ((PyTypeObject *)ldata->py_type)->tp_name,
                    Py_TYPE(py_el)->tp_name);
        }
        else if (ldata->py_list)
        {
            if (PyList_Append(ldata->py_list, py_el) == 0)
                ok = true;
        }
        else
        {
            // Call the function.
            PyObject *py_res = PyObject_CallFunctionObjArgs(ldata->py_append,
                    ldata->py_obj, py_el, NULL);

            if (py_res)
            {
                if (py_res == Py_None)
                    ok = true;
                else
                    bad_result(py_res, "append");

                Py_DECREF(py_res);
            }
        }

        Py_DECREF(py_el);
    }

    if (!ok)
        pyqt5_qtqml_err_print();

    SIP_UNBLOCK_THREADS
}


// Get the length of the list.
static int list_count(QQmlListProperty<QObject> *p)
{
    int res = -1;

    SIP_BLOCK_THREADS

    ListData *ldata = reinterpret_cast<ListData *>(p->data);

    if (ldata->py_list)
    {
        res = PyList_Size(ldata->py_list);
    }
    else
    {
        // Call the function.
        PyObject *py_res = PyObject_CallFunctionObjArgs(ldata->py_count,
                ldata->py_obj, NULL);

        if (py_res)
        {
            PyErr_Clear();

            res = SIPLong_AsLong(py_res);

            if (PyErr_Occurred())
            {
                bad_result(py_res, "count");

                res = -1;
            }

            Py_DECREF(py_res);
        }
    }

    if (res < 0)
    {
        pyqt5_qtqml_err_print();
        res = 0;
    }

    SIP_UNBLOCK_THREADS

    return res;
}


// Get an item from the list.
static QObject *list_at(QQmlListProperty<QObject> *p, int idx)
{
    QObject *qobj = 0;

    SIP_BLOCK_THREADS

    ListData *ldata = reinterpret_cast<ListData *>(p->data);

    if (ldata->py_list)
    {
        PyObject *py_el = PyList_GetItem(ldata->py_list, idx);

        if (py_el)
        {
            int iserr = 0;
            qobj = reinterpret_cast<QObject *>(sipForceConvertToType(py_el,
                    sipType_QObject, 0, SIP_NO_CONVERTORS, 0, &iserr));
        }
    }
    else
    {
        // Call the function.
        PyObject *py_res = PyObject_CallFunction(ldata->py_at,
                const_cast<char *>("Ni"), ldata->py_obj, idx);

        if (py_res)
        {
            int iserr = 0;
            qobj = reinterpret_cast<QObject *>(sipForceConvertToType(py_res,
                    sipType_QObject, 0, SIP_NO_CONVERTORS, 0, &iserr));

            if (iserr)
                bad_result(py_res, "at");

            Py_DECREF(py_res);
        }
    }

    if (!qobj)
        pyqt5_qtqml_err_print();

    SIP_UNBLOCK_THREADS

    return qobj;
}


// Clear the list.
static void list_clear(QQmlListProperty<QObject> *p)
{
    SIP_BLOCK_THREADS

    ListData *ldata = reinterpret_cast<ListData *>(p->data);
    bool ok = false;

    if (ldata->py_list)
    {
        if (PyList_SetSlice(ldata->py_list, 0, PyList_Size(ldata->py_list), NULL) == 0)
            ok = true;
    }
    else
    {
        // Call the function.
        PyObject *py_res = PyObject_CallFunctionObjArgs(ldata->py_clear,
                ldata->py_obj, NULL);

        if (py_res)
        {
            if (py_res == Py_None)
                ok = true;
            else
                bad_result(py_res, "clear");

            Py_DECREF(py_res);
        }
    }

    if (!ok)
        pyqt5_qtqml_err_print();

    SIP_UNBLOCK_THREADS
}


// Raise an exception for an unexpected result.
static void bad_result(PyObject *py_res, const char *context)
{
#if PY_MAJOR_VERSION >= 3
    PyErr_Format(PyExc_TypeError, "unexpected result from %s function: %S",
            context, py_res);
#else
    PyObject *py_res_s = PyObject_Str(py_res);

    if (py_res_s != NULL)
    {
        PyErr_Format(PyExc_TypeError, "unexpected result from %s function: %s",
                context, PyString_AsString(py_res_s));

        Py_DECREF(py_res_s);
    }
#endif
}

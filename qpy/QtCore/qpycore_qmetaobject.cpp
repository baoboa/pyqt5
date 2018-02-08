// This is the support code for QMetaObject.
//
// Copyright (c) 2018 Riverbank Computing Limited <info@riverbankcomputing.com>
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

#include <QGenericArgument>
#include <QGenericReturnArgument>

#include "qpycore_api.h"
#include "qpycore_chimera.h"
#include "qpycore_misc.h"

#include "sipAPIQtCore.h"


// Forward declarations.
static PyObject *ArgumentStorage_New(PyObject *type, PyObject *data);
#if defined(SIP_USE_PYCAPSULE)
extern "C" {static void ArgumentStorage_delete(PyObject *cap);}
#else
extern "C" {static void ArgumentStorage_delete(void *stv);}
#endif


// Return a wrapped QGenericArgument for the given type and Python object.
PyObject *qpycore_ArgumentFactory(PyObject *type, PyObject *data)
{
    PyObject *as_obj = ArgumentStorage_New(type, data);

    if (!as_obj)
    {
        Chimera::raiseParseException(type, "a Q_ARG()");
        return 0;
    }

#if defined(SIP_USE_PYCAPSULE)
    Chimera::Storage *st = reinterpret_cast<Chimera::Storage *>(
            PyCapsule_GetPointer(as_obj, NULL));
#else
    Chimera::Storage *st = reinterpret_cast<Chimera::Storage *>(
            PyCObject_AsVoidPtr(as_obj));
#endif

    QGenericArgument *arg = new QGenericArgument(
            st->type()->name().constData(), st->address());

    PyObject *ga_obj = sipConvertFromNewType(arg, sipType_QGenericArgument, 0);

    if (ga_obj)
    {
        // Stash the storage in the user field so that everything will be
        // properly garbage collected.
        sipSetUserObject((sipSimpleWrapper *)ga_obj, as_obj);
    }
    else
    {
        delete arg;
        Py_DECREF(as_obj);
    }

    return ga_obj;
}


// Return a wrapped QGenericReturnArgument for the given type.
PyObject *qpycore_ReturnFactory(PyObject *type)
{
    PyObject *as_obj = ArgumentStorage_New(type, 0);

    if (!as_obj)
    {
        Chimera::raiseParseException(type, "a Q_RETURN_ARG()");
        return 0;
    }

#if defined(SIP_USE_PYCAPSULE)
    Chimera::Storage *st = reinterpret_cast<Chimera::Storage *>(
            PyCapsule_GetPointer(as_obj, NULL));
#else
    Chimera::Storage *st = reinterpret_cast<Chimera::Storage *>(
            PyCObject_AsVoidPtr(as_obj));
#endif

    QGenericReturnArgument *arg = new QGenericReturnArgument(
            st->type()->name().constData(), st->address());

    PyObject *gra_obj = sipConvertFromNewType(arg,
            sipType_QGenericReturnArgument, 0);

    if (gra_obj)
    {
        // Stash the storage in the user field so that everything will be
        // properly garbage collected.
        sipSetUserObject((sipSimpleWrapper *)gra_obj, as_obj);
    }
    else
    {
        delete arg;
        Py_DECREF(as_obj);
    }

    return gra_obj;
}


// Return the Python result from a QGenericReturnArgument.
PyObject *qpycore_ReturnValue(PyObject *gra_obj)
{
    PyObject *as_obj = sipGetUserObject((sipSimpleWrapper *)gra_obj);

#if defined(SIP_USE_PYCAPSULE)
    Chimera::Storage *st = reinterpret_cast<Chimera::Storage *>(
            PyCapsule_GetPointer(as_obj, NULL));
#else
    Chimera::Storage *st = reinterpret_cast<Chimera::Storage *>(
            PyCObject_AsVoidPtr(as_obj));
#endif

    return st->toPyObject();
}


#if defined(SIP_USE_PYCAPSULE)
// The PyCapsule destructor for the ArgumentStorage type.
static void ArgumentStorage_delete(PyObject *cap)
{
    Chimera::Storage *st = reinterpret_cast<Chimera::Storage *>(
            PyCapsule_GetPointer(cap, NULL));
    const Chimera *ct = st->type();

    delete st;
    delete ct;
}
#else
// The PyCObject destructor for the ArgumentStorage type.
static void ArgumentStorage_delete(void *stv)
{
    Chimera::Storage *st = reinterpret_cast<Chimera::Storage *>(stv);
    const Chimera *ct = st->type();

    delete st;
    delete ct;
}
#endif


// Returns a Python object wrapping an ArgumentStorage instance.
static PyObject *ArgumentStorage_New(PyObject *type, PyObject *data)
{
    const Chimera *ct = Chimera::parse(type);

    if (!ct)
        return 0;

    Chimera::Storage *st;

    if (data)
        st = ct->fromPyObjectToStorage(data);
    else
        st = ct->storageFactory();

    if (!st)
    {
        delete ct;
        return 0;
    }

    // Convert to a Python object.
#if defined(SIP_USE_PYCAPSULE)
    PyObject *as_obj = PyCapsule_New(st, NULL, ArgumentStorage_delete);
#else
    PyObject *as_obj = PyCObject_FromVoidPtr(st, ArgumentStorage_delete);
#endif

    if (!as_obj)
    {
        delete st;
        delete ct;
    }

    return as_obj;
}

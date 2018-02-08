// This is the implementation of the various Chimera helpers.
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

#include <QDBusArgument>
#include <QDBusObjectPath>
#include <QDBusSignature>
#include <QDBusVariant>
#include <QMetaType>

#include "qpydbus_chimera_helpers.h"

#include "sipAPIQtDBus.h"


// Forward declarations.
static PyObject *from_variant_type(const QDBusArgument &arg);
static PyObject *from_array_type(const QDBusArgument &arg);
static PyObject *from_structure_type(const QDBusArgument &arg);
static PyObject *from_map_type(const QDBusArgument &arg);
static PyObject *from_qstring(const QString &arg);
static PyObject *from_qvariant(const QVariant &arg);


// Convert a QVariant to a Python object.
bool qpydbus_from_qvariant_convertor(const QVariant &var, PyObject **objp)
{
    // Handle QDBusObjectPath.
    if (var.userType() == qMetaTypeId<QDBusObjectPath>())
    {
        *objp = from_qstring(var.value<QDBusObjectPath>().path());

        return true;
    }

    // Handle QDBusSignature.
    if (var.userType() == qMetaTypeId<QDBusSignature>())
    {
        *objp = from_qstring(var.value<QDBusSignature>().signature());

        return true;
    }

    // Handle QDBusVariant.
    if (var.userType() == qMetaTypeId<QDBusVariant>())
    {
        *objp = from_qvariant(var.value<QDBusVariant>().variant());

        return true;
    }

    // Anything else must be a QDBusArgument.
    if (var.userType() != qMetaTypeId<QDBusArgument>())
        return false;

    QDBusArgument arg = var.value<QDBusArgument>();

    switch (arg.currentType())
    {
    case QDBusArgument::BasicType:
        *objp = from_qvariant(arg.asVariant());
        break;

    case QDBusArgument::VariantType:
        *objp = from_variant_type(arg);
        break;

    case QDBusArgument::ArrayType:
        *objp = from_array_type(arg);
        break;

    case QDBusArgument::StructureType:
        *objp = from_structure_type(arg);
        break;

    case QDBusArgument::MapType:
        *objp = from_map_type(arg);
        break;

    default:
        PyErr_Format(PyExc_TypeError, "unsupported DBus argument type %d",
                (int)arg.currentType());
        *objp = 0;
    }

    return true;
}


// Convert a QDBusArgument variant type to a Python object.
static PyObject *from_variant_type(const QDBusArgument &arg)
{
    QDBusVariant dbv;

    arg >> dbv;

    return from_qvariant(dbv.variant());
}


// Convert a QDBusArgument array type to a Python object.
static PyObject *from_array_type(const QDBusArgument &arg)
{
    QVariantList vl;

    arg.beginArray();

    while (!arg.atEnd())
        vl.append(arg.asVariant());

    arg.endArray();

    PyObject *obj = PyList_New(vl.count());

    if (!obj)
        return 0;

    for (int i = 0; i < vl.count(); ++i)
    {
        PyObject *itm = from_qvariant(vl.at(i));

        if (!itm)
        {
            Py_DECREF(obj);
            return 0;
        }

        PyList_SetItem(obj, i, itm);
    }

    return obj;
}


// Convert a QDBusArgument structure type to a Python object.
static PyObject *from_structure_type(const QDBusArgument &arg)
{
    QVariantList vl;

    arg.beginStructure();

    while (!arg.atEnd())
        vl.append(arg.asVariant());

    arg.endStructure();

    PyObject *obj = PyTuple_New(vl.count());

    if (!obj)
        return 0;

    for (int i = 0; i < vl.count(); ++i)
    {
        PyObject *itm = from_qvariant(vl.at(i));

        if (!itm)
        {
            Py_DECREF(obj);
            return 0;
        }

        PyTuple_SetItem(obj, i, itm);
    }

    return obj;
}


// Convert a QDBusArgument map type to a Python object.
static PyObject *from_map_type(const QDBusArgument &arg)
{
    PyObject *obj = PyDict_New();

    if (!obj)
        return 0;

    arg.beginMap();

    while (!arg.atEnd())
    {
        arg.beginMapEntry();

        PyObject *key = from_qvariant(arg.asVariant());
        PyObject *value = from_qvariant(arg.asVariant());

        arg.endMapEntry();

        if (!key || !value)
        {
            Py_XDECREF(key);
            Py_XDECREF(value);
            Py_DECREF(obj);

            return 0;
        }

        int rc = PyDict_SetItem(obj, key, value);

        Py_DECREF(key);
        Py_DECREF(value);

        if (rc < 0)
        {
            Py_DECREF(obj);

            return 0;
        }
    }

    arg.endMap();

    return obj;
}


// Convert a QString to a Python object.
static PyObject *from_qstring(const QString &arg)
{
    QString *heap = new QString(arg);
    PyObject *obj = sipConvertFromNewType(heap, sipType_QString, 0);

    if (!obj)
        delete heap;

    return obj;
}


// Convert a QVariant to a Python object.
static PyObject *from_qvariant(const QVariant &arg)
{
    QVariant *heap = new QVariant(arg);
    PyObject *obj = sipConvertFromNewType(heap, sipType_QVariant, 0);

    if (!obj)
        delete heap;

    return obj;
}

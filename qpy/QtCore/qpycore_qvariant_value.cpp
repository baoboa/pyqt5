// This implements the conversion of a QVariant to a Python object and is part
// of the public API.
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

#include <QVariant>

#include "qpycore_chimera.h"

#include "sipAPIQtCore.h"


// Forward declarations.
static PyObject *convert_hash(const Chimera *ct, const QVariantHash &value);
static PyObject *convert_list(const Chimera *ct, const QVariantList &value);
static PyObject *convert_map(const Chimera *ct, const QVariantMap &value);
static PyObject *convert(const Chimera *ct, const QVariant &value);
static int add_variant_to_dict(const Chimera *ct, PyObject *dict,
        const QString &key, const QVariant &value);


// Convert a QVariant to a Python object according to an optional type.
PyObject *pyqt5_from_qvariant_by_type(QVariant &value, PyObject *type)
{
    PyObject *value_obj;

    if (type)
    {
        const Chimera *ct = Chimera::parse(type);

        if (!ct)
            return 0;

        QVariant::Type wanted = static_cast<QVariant::Type>(ct->metatype());

        // Get QVariant to do a conversion if there is one to do.
        if (value.isValid() && ct->metatype() < static_cast<int>(QVariant::UserType))
        {
            // If we have a QStringList but are not wanting one then convert it
            // to a QVariantList.
            if (wanted != QVariant::StringList && value.type() == QVariant::StringList)
                value.convert(QVariant::List);

            // If we have a container but are not wanting one then assume we
            // want a container with elements of the wanted type.
            if (wanted != QVariant::List && value.type() == QVariant::List)
            {
                // If we have a QVariantList but we wanted a QStringList then
                // assume each variant is a string.
                if (wanted == QVariant::StringList)
                    value_obj = convert(ct, value);
                else
                    value_obj = convert_list(ct, value.toList());
            }
            else if (wanted != QVariant::Map && value.type() == QVariant::Map)
            {
                value_obj = convert_map(ct, value.toMap());
            }
            else if (wanted != QVariant::Hash && value.type() == QVariant::Hash)
            {
                value_obj = convert_hash(ct, value.toHash());
            }
            else
            {
                value_obj = convert(ct, value);
            }
        }
        else if (!value.isValid())
        {
            // Convert an invalid value to the default value of the requested
            // type.
            if (ct->py_type())
                value_obj = PyObject_CallObject(ct->py_type(), NULL);
            else
                value_obj = ct->toPyObject(QVariant(wanted));
        }
        else
        {
            // This is likely to fail and the exception will say why.
            value_obj = ct->toPyObject(value);
        }

        delete ct;
    }
    else
    {
        QVariant *heap = new QVariant(value);
        value_obj = sipConvertFromNewType(heap, sipType_QVariant, 0);

        if (!value_obj)
            delete heap;
    }

    return value_obj;
}


// Convert a QVariantList to a list of Python objects.
static PyObject *convert_list(const Chimera *ct, const QVariantList &value)
{
    PyObject *list = PyList_New(value.size());

    if (!list)
        return 0;

    for (int i = 0; i < value.size(); ++i)
    {
        PyObject *el = convert(ct, value.at(i));

        if (!el)
        {
            Py_DECREF(list);
            return 0;
        }

        PyList_SetItem(list, i, el);
    }

    return list;
}


// Convert a QVariantMap to a dict of Python objects.
static PyObject *convert_map(const Chimera *ct, const QVariantMap &value)
{
    PyObject *dict = PyDict_New();

    if (!dict)
        return 0;

    for (QVariantMap::const_iterator it = value.constBegin(); it != value.constEnd(); ++it)
    {
        if (add_variant_to_dict(ct, dict, it.key(), it.value()) < 0)
        {
            Py_DECREF(dict);
            return 0;
        }
    }

    return dict;
}


// Convert a QVariantHash to a dict of Python objects.
static PyObject *convert_hash(const Chimera *ct, const QVariantHash &value)
{
    PyObject *dict = PyDict_New();

    if (!dict)
        return 0;

    for (QVariantHash::const_iterator it = value.constBegin(); it != value.constEnd(); ++it)
    {
        if (add_variant_to_dict(ct, dict, it.key(), it.value()) < 0)
        {
            Py_DECREF(dict);
            return 0;
        }
    }

    return dict;
}


// Convert a QVariant to a Python object.
static PyObject *convert(const Chimera *ct, const QVariant &value)
{
    QVariant converted = value;

    if (!converted.convert(static_cast<QVariant::Type>(ct->metatype())))
        converted = value;

    return ct->toPyObject(converted);
}


// Add a QVariant to a Python dict with a QString key.
static int add_variant_to_dict(const Chimera *ct, PyObject *dict,
        const QString &key, const QVariant &value)
{
    QString *key_heap = new QString(key);
    PyObject *key_obj = sipConvertFromNewType(key_heap, sipType_QString, 0);

    if (!key_obj)
    {
        delete key_heap;
        return 0;
    }

    PyObject *value_obj = convert(ct, value);

    if (!value_obj)
    {
        Py_DECREF(key_obj);
        return 0;
    }

    int rc = PyDict_SetItem(dict, key_obj, value_obj);

    Py_DECREF(key_obj);
    Py_DECREF(value_obj);

    return rc;
}

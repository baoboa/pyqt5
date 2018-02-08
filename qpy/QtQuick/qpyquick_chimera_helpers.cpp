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

#include <QList>
#include <QMetaType>
#include <QObject>

#include "qpyquick_chimera_helpers.h"

#include "sipAPIQtQuick.h"


// Forward declarations.
static int QList_QObject_metatype();
static bool to_QList_QObject(PyObject *obj, QList<QObject *>&cpp);


// Convert a QVariant to a Python object.
bool qpyquick_from_qvariant_convertor(const QVariant *varp, PyObject **objp)
{
    // Check we handle the meta-type.
    if (varp->userType() != QList_QObject_metatype())
        return false;

    const QList<QObject *> *cpp = reinterpret_cast<const QList<QObject *> *>(varp->data());

    PyObject *obj = PyList_New(cpp->count());

    if (obj)
    {
        for (int i = 0; i < cpp->count(); ++i)
        {
            PyObject *itm = sipConvertFromType(cpp->at(i), sipType_QObject, 0);

            if (!itm)
            {
                Py_DECREF(obj);
                obj = 0;
                break;
            }

            PyList_SetItem(obj, i, itm);
        }
    }

    *objp = obj;

    return true;
}


// Convert a Python object to a QVariant.
bool qpyquick_to_qvariant_convertor(PyObject *obj, QVariant &var, bool *okp)
{
    int metatype = QList_QObject_metatype();

    // Check the meta-type has been defined.
    if (metatype == 0)
        return false;

    QList<QObject *> qlo;

    // A failure to convert isn't an error, just the wrong type of Python
    // object.
    if (!to_QList_QObject(obj, qlo))
        return false;

    var = QVariant(metatype, &qlo);
    *okp = true;

    return true;
}


// Convert a Python object to QVariant data.
bool qpyquick_to_qvariant_data_convertor(PyObject *obj, void *data,
        int metatype, bool *okp)
{
    // Check we handle the meta-type.
    if (metatype != QList_QObject_metatype())
        return false;

    QList<QObject *> qlo;

    // A failure to convert isn't an error, just the wrong type of Python
    // object.
    if (!to_QList_QObject(obj, qlo))
        return false;

    *reinterpret_cast<QList<QObject *> *>(data) = qlo;
    *okp = true;

    return true;
}


// Get the metatype for QList<QObject *> or 0 if it hasn't been registered.
static int QList_QObject_metatype()
{
    static int lo_metatype = 0;

    // We look each time until we find it rather than rely on any particular
    // behaviour from QtQuick.
    if (lo_metatype == 0)
        lo_metatype = QMetaType::type("QList<QObject*>");

    return lo_metatype;
}


// Convert a Python list object to a QList<QObject*> and return true if the
// conversion was successful.  An empty list is never converted and left to the
// QtCore module to handle.
static bool to_QList_QObject(PyObject *obj, QList<QObject *>&cpp)
{
    if (!PyList_CheckExact(obj) || PyList_Size(obj) == 0)
        return false;

    for (Py_ssize_t i = 0; i < PyList_Size(obj); ++i)
    {
        PyObject *val_obj = PyList_GetItem(obj, i);

        if (!val_obj)
            return false;

        int iserr = 0;

        QObject *val = reinterpret_cast<QObject *>(sipForceConvertToType(
                val_obj, sipType_QObject, 0, SIP_NO_CONVERTORS, 0, &iserr));

        if (iserr)
            return false;

        cpp.append(val);
    }

    return true;
}

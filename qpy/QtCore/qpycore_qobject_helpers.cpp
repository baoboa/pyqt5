// This implements the helpers for QObject.
//
// Copyright (c) 2013 Riverbank Computing Limited <info@riverbankcomputing.com>
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


#include <ctype.h>

#include <Python.h>

#include <QMetaObject>
#include <QMetaType>
#include <QObject>
#include <QVariant>

#include "qpycore_chimera.h"
#include "qpycore_pyqtboundsignal.h"
#include "qpycore_pyqtproperty.h"
#include "qpycore_pyqtpyobject.h"
#include "qpycore_pyqtslot.h"
#include "qpycore_qobject_helpers.h"
#include "qpycore_sip.h"
#include "qpycore_types.h"


// Forward declarations.
static int qt_metacall_worker(sipSimpleWrapper *pySelf, PyTypeObject *pytype,
        sipTypeDef *base, QMetaObject::Call _c, int _id, void **_a);


// This is the helper for all implementations of QObject::metaObject().
const QMetaObject *qpycore_qobject_metaobject(sipSimpleWrapper *pySelf,
        sipTypeDef *base)
{
    // Return the dynamic meta-object if there is one.
    if (pySelf && ((pyqtWrapperType *)Py_TYPE(pySelf))->metaobject)
        return ((pyqtWrapperType *)Py_TYPE(pySelf))->metaobject->mo;

    // Fall back to the static Qt meta-object.
    return reinterpret_cast<const QMetaObject *>(((pyqt4ClassTypeDef *)base)->qt4_static_metaobject);
}


// This is the helper for all implementations of QObject::qt_metacall().
int qpycore_qobject_qt_metacall(sipSimpleWrapper *pySelf, sipTypeDef *base,
        QMetaObject::Call _c, int _id, void **_a)
{
    // Check if the Python object has gone.
    if (!pySelf)
        return -1;

    SIP_BLOCK_THREADS
    _id = qt_metacall_worker(pySelf, Py_TYPE(pySelf), base, _c, _id, _a);
    SIP_UNBLOCK_THREADS

    return _id;
}


// This does the real work for all implementations of QObject::qt_metacall().
static int qt_metacall_worker(sipSimpleWrapper *pySelf, PyTypeObject *pytype,
        sipTypeDef *base, QMetaObject::Call _c, int _id, void **_a)
{
    // See if this is a wrapped C++ type rather than a Python sub-type.
    if (pytype == sipTypeAsPyTypeObject(base))
        return _id;

    _id = qt_metacall_worker(pySelf, pytype->tp_base, base, _c, _id, _a);

    if (_id < 0)
        return _id;

    pyqtWrapperType *pyqt_wt = (pyqtWrapperType *)pytype;
    qpycore_metaobject *qo = pyqt_wt->metaobject;

    bool ok = true;

    if (_c == QMetaObject::InvokeMetaMethod)
    {
        if (_id < qo->nr_signals + qo->pslots.count())
        {
            if (_id < qo->nr_signals)
            {
                QObject *qthis = reinterpret_cast<QObject *>(sipGetCppPtr(pySelf, sipType_QObject));

                Py_BEGIN_ALLOW_THREADS
                QMetaObject::activate(qthis, qo->mo, _id, _a);
                Py_END_ALLOW_THREADS
            }
            else
            {
                PyQtSlot *slot = qo->pslots.at(_id - qo->nr_signals);

                ok = slot->invoke(_a, (PyObject *)pySelf, _a[0]);
            }
        }

        _id -= qo->nr_signals + qo->pslots.count();
    }
    else if (_c == QMetaObject::ReadProperty)
    {
        if (_id < qo->pprops.count())
        {
            qpycore_pyqtProperty *prop = qo->pprops.at(_id);

            if (prop->pyqtprop_get)
            {
                PyObject *py = PyObject_CallFunction(prop->pyqtprop_get,
                        const_cast<char *>("O"), pySelf);

                if (py)
                {
                    ok = prop->pyqtprop_parsed_type->fromPyObject(py, _a[0]);
                    Py_DECREF(py);
                }
                else
                {
                    ok = false;
                }
            }
        }

        _id -= qo->pprops.count();
    }
    else if (_c == QMetaObject::WriteProperty)
    {
        if (_id < qo->pprops.count())
        {
            qpycore_pyqtProperty *prop = qo->pprops.at(_id);

            if (prop->pyqtprop_set)
            {
                PyObject *py = prop->pyqtprop_parsed_type->toPyObject(_a[0]);

                if (py)
                {
                    PyObject *res = PyObject_CallFunction(prop->pyqtprop_set,
                            const_cast<char *>("OO"), pySelf, py);

                    if (res)
                        Py_DECREF(res);
                    else
                        ok = false;

                    Py_DECREF(py);
                }
                else
                {
                    ok = false;
                }
            }
        }

        _id -= qo->pprops.count();
    }
    else if (_c == QMetaObject::ResetProperty)
    {
        if (_id < qo->pprops.count())
        {
            qpycore_pyqtProperty *prop = qo->pprops.at(_id);

            if (prop->pyqtprop_reset)
            {
                PyObject *py = PyObject_CallFunction(prop->pyqtprop_reset,
                        const_cast<char *>("O"), pySelf);

                if (py)
                    Py_DECREF(py);
                else
                    ok = false;
            }
        }

        _id -= qo->pprops.count();
    }
    else if (_c == QMetaObject::QueryPropertyDesignable)
        _id -= qo->pprops.count();
    else if (_c == QMetaObject::QueryPropertyScriptable)
        _id -= qo->pprops.count();
    else if (_c == QMetaObject::QueryPropertyStored)
        _id -= qo->pprops.count();
    else if (_c == QMetaObject::QueryPropertyEditable)
        _id -= qo->pprops.count();
    else if (_c == QMetaObject::QueryPropertyUser)
        _id -= qo->pprops.count();

    // Handle any Python errors.
    if (!ok)
    {
        PyErr_Print();
        return -1;
    }

    return _id;
}


// This is the helper for all implementations of QObject::qt_metacast().
bool qpycore_qobject_qt_metacast(sipSimpleWrapper *pySelf,
        const sipTypeDef *base, const char *_clname, void **sipCpp)
{
    *sipCpp = 0;

    if (!_clname)
        return true;

    // Check if the Python object has gone.
    if (!pySelf)
        return true;

    bool is_py_class = false;

    SIP_BLOCK_THREADS

    PyObject *mro = Py_TYPE(pySelf)->tp_mro;

    for (SIP_SSIZE_T i = 0; i < PyTuple_GET_SIZE(mro); ++i)
    {
        PyTypeObject *pytype = (PyTypeObject *)PyTuple_GET_ITEM(mro, i);

        if (!PyObject_IsInstance((PyObject *)pytype, (PyObject *)&qpycore_pyqtWrapperType_Type))
            continue;

        const sipTypeDef *td = ((sipWrapperType *)pytype)->type;

        if (qstrcmp(pytype->tp_name, _clname) == 0)
        {
            if (td == base)
                *sipCpp = sipGetAddress(pySelf);
            else
                *sipCpp = sipGetMixinAddress(pySelf, td);

            is_py_class = true;
            break;
        }

        if (((pyqt4ClassTypeDef *)td)->qt_interface && qstrcmp(((pyqt4ClassTypeDef *)td)->qt_interface, _clname) == 0)
        {
            *sipCpp = sipGetMixinAddress(pySelf, td);
            is_py_class = true;
            break;
        }
    }

    SIP_UNBLOCK_THREADS

    return is_py_class;
}


// This is a helper for QObject.staticMetaObject %GetCode.
PyObject *qpycore_qobject_staticmetaobject(PyObject *type_obj)
{
    pyqtWrapperType *pyqt_wt = (pyqtWrapperType *)type_obj;
    const QMetaObject *mo;

    if (pyqt_wt->metaobject)
    {
        // It's a sub-type of a wrapped type.
        mo = pyqt_wt->metaobject->mo;
    }
    else
    {
        // It's a wrapped type.
        pyqt4ClassTypeDef *p4ctd = (pyqt4ClassTypeDef *)((sipWrapperType *)pyqt_wt)->type;

        if (!p4ctd)
        {
            /*
             * This is a side effect of a wrapped class not being fully ready
             * until sip's meta-class's __init__() has run (rather than after
             * its __new__() method as might be expected).
             */
            PyErr_SetString(PyExc_AttributeError,
                    "staticMetaObject isn't available until the meta-class's __init__ returns");
            return 0;
        }

        mo = reinterpret_cast<const QMetaObject *>(p4ctd->qt4_static_metaobject);
    }

    return sipConvertFromType(const_cast<QMetaObject *>(mo), sipType_QMetaObject, 0);
}

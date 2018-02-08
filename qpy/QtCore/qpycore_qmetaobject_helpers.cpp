// This implements the helpers for QMetaObject.
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

#include <QByteArray>
#include <QMetaMethod>
#include <QMetaObject>
#include <QObject>

#include "qpycore_api.h"
#include "qpycore_chimera.h"
#include "qpycore_misc.h"
#include "qpycore_objectified_strings.h"
#include "qpycore_public_api.h"

#include "sipAPIQtCore.h"


// Forward declarations.
static void connect(QObject *qobj, PyObject *slot_obj,
        const QByteArray &slot_nm, const QByteArray &args);


void qpycore_qmetaobject_connectslotsbyname(QObject *qobj,
        PyObject *qobj_wrapper)
{
    // Get the class attributes.
    PyObject *dir = PyObject_Dir((PyObject *)Py_TYPE(qobj_wrapper));

    if (!dir)
        return;

    PyObject *slot_obj = 0;

    for (Py_ssize_t li = 0; li < PyList_Size(dir); ++li)
    {
        PyObject *name_obj = PyList_GetItem(dir, li);

        // Get the slot object.
        Py_XDECREF(slot_obj);
        slot_obj = PyObject_GetAttr(qobj_wrapper, name_obj);

        if (!slot_obj)
            continue;

        // Ignore it if it is not a callable.
        if (!PyCallable_Check(slot_obj))
            continue;

        // Use the signature attribute instead of the name if there is one.
        PyObject *sigattr = PyObject_GetAttr(slot_obj,
                qpycore_dunder_pyqtsignature);

        if (sigattr)
        {
            for (Py_ssize_t i = 0; i < PyList_Size(sigattr); ++i)
            {
                PyObject *decoration = PyList_GetItem(sigattr, i);
                Chimera::Signature *sig = Chimera::Signature::fromPyObject(decoration);
                QByteArray args = sig->arguments();

                if (!args.isEmpty())
                    connect(qobj, slot_obj, sig->name(), args);
            }

            Py_DECREF(sigattr);
        }
        else
        {
            const char *ascii_name = sipString_AsASCIIString(&name_obj);

            if (!ascii_name)
                continue;

            PyErr_Clear();

            connect(qobj, slot_obj, QByteArray(ascii_name), QByteArray());

            Py_DECREF(name_obj);
        }
    }

    Py_XDECREF(slot_obj);
    Py_DECREF(dir);
}


// Connect up a particular slot name, with optional arguments.
static void connect(QObject *qobj, PyObject *slot_obj,
        const QByteArray &slot_nm, const QByteArray &args)
{
    // Ignore if it's not an autoconnect slot.
    if (!slot_nm.startsWith("on_"))
        return;

    // Extract the names of the emitting object and the signal.
    int i;

    i = slot_nm.lastIndexOf('_');

    if (i - 3 < 1 || i + 1 >= slot_nm.size())
        return;

    QByteArray ename = slot_nm.mid(3, i - 3);
    QByteArray sname = slot_nm.mid(i + 1);

    // Find the emitting object and get its meta-object.
    QObject *eobj = qobj->findChild<QObject *>(ename);

    if (!eobj)
        return;

    const QMetaObject *mo = eobj->metaObject();

    // Got through the methods looking for a matching signal.
    for (int m = 0; m < mo->methodCount(); ++m)
    {
        QMetaMethod mm = mo->method(m);

        if (mm.methodType() != QMetaMethod::Signal)
            continue;

        QByteArray sig(mm.methodSignature());

        if (Chimera::Signature::name(sig) != sname)
            continue;

        // If we have slot arguments then they must match as well.
        if (!args.isEmpty() && Chimera::Signature::arguments(sig) != args)
            continue;

        QObject *receiver;
        QByteArray slot_sig;

        if (pyqt5_get_connection_parts(slot_obj, eobj, sig.constData(), false, &receiver, slot_sig) != sipErrorNone)
            continue;

        // Add the type character.
        sig.prepend('2');

        // Connect the signal.
        QObject::connect(eobj, sig.constData(), receiver, slot_sig.constData());
    }
}

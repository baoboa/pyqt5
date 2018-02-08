// This is the implementation of the QPyDBusReply class.
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

#include <QDBusMessage>
#include <QDBusPendingCall>

#include "qpydbusreply.h"

#include "sipAPIQtDBus.h"


// Extract a reply from a message.  The GIL should be held.
QPyDBusReply::QPyDBusReply(const QDBusMessage &reply)
{
    *this = reply;
}


// Extract a reply from a pending call.  The GIL should be held.
QPyDBusReply::QPyDBusReply(const QDBusPendingCall &call)
{
    *this = call;
}


// Extract a reply from an error.
QPyDBusReply::QPyDBusReply(const QDBusError &error)
{
    _q_value = 0;
    _q_is_valid = !error.isValid();
    _q_error = error;
}


// Copy a reply.  The GIL should be held.
QPyDBusReply::QPyDBusReply(const QPyDBusReply &other)
{
    _q_value = other._q_value;
    Py_XINCREF(_q_value);

    _q_value_variant = other._q_value_variant;
    _q_is_valid = other._q_is_valid;
    _q_error = other._q_error;
}


// A reply created from a QDBusReply<> mapped type convertor.
QPyDBusReply::QPyDBusReply(PyObject *q_value, bool q_is_valid, const QDBusError &q_error) : _q_value(q_value), _q_is_valid(q_is_valid), _q_error(q_error)
{
}


// The dtor.  The GIL should be held.
QPyDBusReply::~QPyDBusReply()
{
    Py_XDECREF(_q_value);
}


// Return the value of the reply.  The GIL should be held.
PyObject *QPyDBusReply::value(PyObject *type) const
{
    if (!_q_is_valid)
    {
        PyErr_SetString(PyExc_ValueError, "QDBusReply value is invalid");
        return 0;
    }

    // Return any explicit value.
    if (_q_value)
    {
        if (type)
        {
            PyErr_SetString(PyExc_ValueError,
                    "'type' argument cannot be used with an explcitly typed reply");
            return 0;
        }

        Py_INCREF(_q_value);
        return _q_value;
    }

    QVariant val(_q_value_variant);

    return pyqt5_qtdbus_from_qvariant_by_type(val, type);
}


// Extract a reply from a message.  The GIL should be held.
QPyDBusReply &QPyDBusReply::operator=(const QDBusMessage &reply)
{
    _q_value = 0;
    _q_error = reply;
    _q_is_valid = !_q_error.isValid();

    if (_q_is_valid)
    {
        QList<QVariant> results = reply.arguments();

        if (results.count() == 0)
        {
            Py_INCREF(Py_None);
            _q_value = Py_None;
        }
        else
        {
            // Convert this later if requested.
            _q_value_variant = results.at(0);
        }
    }

    return *this;
}


// Extract a reply from a pending call.  The GIL should be held.
QPyDBusReply &QPyDBusReply::operator=(const QDBusPendingCall &call)
{
    QDBusPendingCall other(call);

    Py_BEGIN_ALLOW_THREADS
    other.waitForFinished();
    Py_END_ALLOW_THREADS

    return *this = other.reply();
}

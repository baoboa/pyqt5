// This is the implementation of the QPyDBusPendingReply class.
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

#include "qpydbuspendingreply.h"

#include "sipAPIQtDBus.h"


// Default ctor.
QPyDBusPendingReply::QPyDBusPendingReply() : QDBusPendingReply<void>()
{
}


// Copy a reply.
QPyDBusPendingReply::QPyDBusPendingReply(const QPyDBusPendingReply &other)
        : QDBusPendingReply<void>(other)
{
}


// Extract a reply from another pending call.
QPyDBusPendingReply::QPyDBusPendingReply(const QDBusPendingCall &call)
        : QDBusPendingReply<void>(call)
{
}


// Extract a reply from a message.
QPyDBusPendingReply::QPyDBusPendingReply(const QDBusMessage &reply)
        : QDBusPendingReply<void>(reply)
{
}


// Return the value of the reply.  The GIL should be held.
PyObject *QPyDBusPendingReply::value(PyObject *type) const
{
    QVariant val;

    // This will wait for the call to complete.
    Py_BEGIN_ALLOW_THREADS
    val = argumentAt(0);
    Py_END_ALLOW_THREADS

    return pyqt5_qtdbus_from_qvariant_by_type(val, type);
}

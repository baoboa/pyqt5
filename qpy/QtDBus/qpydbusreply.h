// This is the definition of the QPyDBusReply class.
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


#ifndef _QPYDBUSREPLY_H
#define _QPYDBUSREPLY_H


#include <Python.h>

#include <QDBusError>
#include <QVariant>


class QDBusMessage;
class QDBusPendingCall;


class QPyDBusReply
{
public:
    QPyDBusReply(const QDBusMessage &reply);
    QPyDBusReply(const QDBusPendingCall &call);
    QPyDBusReply(const QDBusError &error = QDBusError());
    QPyDBusReply(const QPyDBusReply &other);
    QPyDBusReply(PyObject *q_value, bool q_is_valid, const QDBusError &q_error);
    ~QPyDBusReply();

    inline const QDBusError &error() const {return _q_error;}
    inline bool isValid() const {return _q_is_valid;}

    PyObject *value(PyObject *type) const;

    QPyDBusReply &operator=(const QDBusMessage &reply);
    QPyDBusReply &operator=(const QDBusPendingCall &call);

private:
    PyObject *_q_value;
    QVariant _q_value_variant;
    bool _q_is_valid;
    QDBusError _q_error;
};

#endif

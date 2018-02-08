// This defines the interfaces for the pyqtMethodProxy type.
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


#ifndef _QPYCORE_PYQTMETHODPROXY_H
#define _QPYCORE_PYQTMETHODPROXY_H


#include <Python.h>

#include <QtGlobal>
#include <QByteArray>

#include "qpycore_namespace.h"


QT_BEGIN_NAMESPACE
class QObject;
QT_END_NAMESPACE


extern "C" {

// This defines the structure of a pyqtMethodProxy object.
typedef struct _qpycore_pyqtMethodProxy {
    PyObject_HEAD

    // The QObject whose method being proxied.
    QObject *qobject;

    // The method index in the QObject's meta-object.
    int method_index;

    // The fully qualified Python name of the method.
    const QByteArray *py_name;
} qpycore_pyqtMethodProxy;

}


// The type object.
extern PyTypeObject *qpycore_pyqtMethodProxy_TypeObject;


bool qpycore_pyqtMethodProxy_init_type();

PyObject *qpycore_pyqtMethodProxy_New(QObject *qobject, int method_index,
        const QByteArray &py_name);


#endif

// This defines the interfaces for the QQmlListPropertyWrapper type.
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


#ifndef _QPYQMLLISTPROPERTYWRAPPER_H
#define _QPYQMLLISTPROPERTYWRAPPER_H


#include <Python.h>

#include <QObject>
#include <QQmlListProperty>


extern "C" {

// This defines the structure of a QQmlListPropertyWrapper object.  This serves
// two purposes.  Firstly it wraps a pointer to a QQmlListProperty like a
// capsule would.  Secondly it acts as a proxy for the underlying Python list.
typedef struct _qpyqml_QQmlListPropertyWrapper {
    PyObject_HEAD

    // The QML list property that is being wrapped.  It is owned by the
    // wrapper.
    QQmlListProperty<QObject> *qml_list_property;

    // A borrowed reference to the underlying Python list.
    PyObject *py_list;
} qpyqml_QQmlListPropertyWrapper;

}


// The type object.
extern PyTypeObject *qpyqml_QQmlListPropertyWrapper_TypeObject;


bool qpyqml_QQmlListPropertyWrapper_init_type();
PyObject *qpyqml_QQmlListPropertyWrapper_New(QQmlListProperty<QObject> *prop,
        PyObject *list);


#endif

// This contains definitions related to the support for Python objects and Qt's
// meta-type system.
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


#ifndef _QPYCORE_PYQTPYOBJECT_H
#define _QPYCORE_PYQTPYOBJECT_H


#include <Python.h>

#include <QMetaType>
#include <QDataStream>


// This class is used to wrap a PyObject so it can be passed around Qt's
// meta-type system while maintaining its reference count.
class PyQt_PyObject
{
public:
    PyQt_PyObject(PyObject *py);
    PyQt_PyObject();
    PyQt_PyObject(const PyQt_PyObject &other);
    ~PyQt_PyObject();

    PyQt_PyObject &operator=(const PyQt_PyObject &other);

    // The Python object being wrapped.
    PyObject *pyobject;
    
    // The Qt meta-type id.
    static int metatype;
};

QDataStream &operator<<(QDataStream &out, const PyQt_PyObject &obj);
QDataStream &operator>>(QDataStream &in, PyQt_PyObject &obj);

Q_DECLARE_METATYPE(PyQt_PyObject)


#endif

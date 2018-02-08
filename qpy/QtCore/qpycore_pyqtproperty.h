// This contains the definitions for the implementation of pyqtProperty.
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


#ifndef _QPYCORE_PYQTPROPERTY_H
#define _QPYCORE_PYQTPROPERTY_H


#include <Python.h>


class Chimera;


extern "C" {

// This defines the structure of a PyQt property.
typedef struct {
    PyObject_HEAD

    // The fget/getter/read callable.
    PyObject *pyqtprop_get;

    // The fset/setter/write callable.
    PyObject *pyqtprop_set;

    // The fdel/deleter callable.
    PyObject *pyqtprop_del;

    // The docstring.
    PyObject *pyqtprop_doc;

    // The freset/reset callable.
    PyObject *pyqtprop_reset;

    // The notify signal.
    PyObject *pyqtprop_notify;

    // The property type.
    PyObject *pyqtprop_type;

    // The parsed type information.
    const Chimera *pyqtprop_parsed_type;

    // The DESIGNABLE, SCRIPTABLE, STORED, USER, CONSTANT and FINAL flags.
    unsigned pyqtprop_flags;

    // The REVISION.
    int pyqtprop_revision;

    // The property's sequence number that determines the position of the
    // property in the QMetaObject.
    uint pyqtprop_sequence;
} qpycore_pyqtProperty;

}


// The type object.
extern PyTypeObject *qpycore_pyqtProperty_TypeObject;


bool qpycore_pyqtProperty_init_type();


#endif

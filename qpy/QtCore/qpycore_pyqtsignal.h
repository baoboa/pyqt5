// This defines the interfaces for the pyqtSignal type.
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


#ifndef _QPYCORE_PYQTSIGNAL_H
#define _QPYCORE_PYQTSIGNAL_H


#include <Python.h>
#include <sip.h>

#include <QByteArray>
#include <QList>

#include "qpycore_chimera.h"


extern "C" {

// This defines the structure of a PyQt signal.
typedef struct _qpycore_pyqtSignal {
    PyObject_HEAD

    // The default signal.  This is the head of the linked list of overloads
    // and holds references to rest of the list elements.
    struct _qpycore_pyqtSignal *default_signal;

    // The next overload in the list.
    struct _qpycore_pyqtSignal *next;

    // The docstring.
    const char *docstring;

    // The optional parameter names.
    const QList<QByteArray> *parameter_names;

    // The revision.
    int revision;

    // The parsed signature, not set if there is an emitter.
    Chimera::Signature *parsed_signature;

    // An optional emitter.
    pyqt5EmitFunc emitter;

    // The non-signal overloads (if any).  This is only set for the default.
    PyMethodDef *non_signals;
} qpycore_pyqtSignal;


int qpycore_get_lazy_attr(const sipTypeDef *td, PyObject *dict);

}


// The type object.
extern PyTypeObject *qpycore_pyqtSignal_TypeObject;


bool qpycore_pyqtSignal_init_type();
qpycore_pyqtSignal *qpycore_pyqtSignal_New(const char *signature,
        bool *fatal = 0);
qpycore_pyqtSignal *qpycore_find_signal(qpycore_pyqtSignal *ps,
        PyObject *subscript, const char *context);
void qpycore_set_signal_name(qpycore_pyqtSignal *ps, const char *type_name,
        const char *name);
PyObject *qpycore_call_signal_overload(qpycore_pyqtSignal *ps, PyObject *bound,
        PyObject *args, PyObject *kw);


#endif

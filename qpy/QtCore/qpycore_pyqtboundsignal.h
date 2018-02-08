// This defines the interfaces for the pyqtBoundSignal type.
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


#ifndef _QPYCORE_PYQTBOUNDSIGNAL_H
#define _QPYCORE_PYQTBOUNDSIGNAL_H


#include <Python.h>
#include <sip.h>

#include "qpycore_pyqtsignal.h"
#include "qpycore_namespace.h"


QT_BEGIN_NAMESPACE
class QByteArray;
class QObject;
QT_END_NAMESPACE


extern "C" {

// This defines the structure of a bound PyQt signal.
typedef struct {
    PyObject_HEAD

    // The unbound signal.
    qpycore_pyqtSignal *unbound_signal;

    // A borrowed reference to the wrapped QObject that is bound to the signal.
    PyObject *bound_pyobject;

    // The QObject that is bound to the signal.
    QObject *bound_qobject;
} qpycore_pyqtBoundSignal;

}


// The type object.
extern PyTypeObject *qpycore_pyqtBoundSignal_TypeObject;


bool qpycore_pyqtBoundSignal_init_type();
PyObject *qpycore_pyqtBoundSignal_New(qpycore_pyqtSignal *unbound_signal,
        PyObject *bound_pyobject, QObject *bound_qobject);
sipErrorState qpycore_get_receiver_slot_signature(PyObject *slot,
        QObject *transmitter, const Chimera::Signature *signal_signature,
        bool single_shot, QObject **receiver, QByteArray &slot_signature);


#endif

// This defines the public API provided by PyQt to external packages.
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


#ifndef _QPYCORE_PUBLIC_API_H
#define _QPYCORE_PUBLIC_API_H


#include <Python.h>
#include <sip.h>

#include "qpycore_namespace.h"


QT_BEGIN_NAMESPACE
class QByteArray;
struct QMetaObject;
class QObject;
class QVariant;
QT_END_NAMESPACE


// A replacement for PyErr_Print().  In PyQt v5.4 it raises a deprecation
// warning and calls PyErr_Print().  In PyQt v5.5 and later it passes the text
// of the exception and traceback to qFatal().
void pyqt5_err_print();

// Convert a Python list to a standard C array of command line arguments and an
// argument count.
char **pyqt5_from_argv_list(PyObject *argv_list, int &argc);

// Convert a QVariant to a Python object according to an optional Python type.
PyObject *pyqt5_from_qvariant_by_type(QVariant &value, PyObject *type);

// Get the receiver object and slot signature to allow a signal to be connected
// to an optional transmitter.  single_shot is true if the signal will only
// ever be emitted once.  Returns the error state.  If this is sipErrorFail
// then a Python exception will have been raised.
sipErrorState pyqt5_get_connection_parts(PyObject *slot, QObject *transmitter,
        const char *signal_signature, bool single_shot, QObject **receiver,
        QByteArray &slot_signature);

// Get the QMetaObject instance for a Python type.  The Python type must be a
// sub-type of QObject's Python type.
const QMetaObject *pyqt5_get_qmetaobject(PyTypeObject *type);

// Get the transmitter object and signal signature from a bound signal.
// Returns the error state.  If this is sipErrorFail then a Python exception
// will have been raised.
sipErrorState pyqt5_get_pyqtsignal_parts(PyObject *signal,
        QObject **transmitter, QByteArray &signal_signature);

// Get the receiver object and slot signature from a callable decorated with
// pyqtSlot.  Returns the error state.  If this is sipErrorFail then a Python
// exception will have been raised.
sipErrorState pyqt5_get_pyqtslot_parts(PyObject *slot, QObject **receiver,
        QByteArray &slot_signature);

// Get the signature string for a bound or unbound signal.  If the signal is
// bound, and the given transmitter is specified, then it must be bound to the
// transmitter.  Returns the error state.  If this is sipErrorFail then a
// Python exception will have been raised.
sipErrorState pyqt5_get_signal_signature(PyObject *signal,
        const QObject *transmitter, QByteArray &signal_signature);

// Register a convertor function that converts a QVariant to a Python object.
// The convertor will return true if the QVariant was handled, so that no other
// convertor need be tried.  If the Python object returned was 0 then there was
// an error and a Python exception raised.
void pyqt5_register_from_qvariant_convertor(
        bool (*convertor)(const QVariant &, PyObject **));

// Register a convertor function that converts a Python object to a QVariant.
// The convertor will return true if the Python object was handled, so that no
// other convertor need be tried.  If the flag set was false then there was an
// error and a Python exception raised.
void pyqt5_register_to_qvariant_convertor(
        bool (*convertor)(PyObject *, QVariant &, bool *));

// Register a convertor function that converts a Python object to the
// pre-allocated data of a QVariant with a specific meta-type.  The convertor
// will return true if the Python object was handled, so that no other
// convertor need be tried.  If the flag set was false then there was an error
// and a Python exception raised.
void pyqt5_register_to_qvariant_data_convertor(
        bool (*convertor)(PyObject *, void *, int, bool *));

// Update an Python list from a standard C array of command line arguments and
// an argument count.
void pyqt5_update_argv_list(PyObject *argv_list, int argc, char **argv);


#endif

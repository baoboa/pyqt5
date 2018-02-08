// This implements the public API provided by PyQt to external packages.
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


#include <QByteArray>
#include <QHash>

#include "qpycore_chimera.h"
#include "qpycore_objectified_strings.h"
#include "qpycore_public_api.h"
#include "qpycore_pyqtboundsignal.h"
#include "qpycore_pyqtsignal.h"
#include "qpycore_types.h"


// A replacement for PyErr_Print() that passes the exception to qFatal().
void pyqt5_err_print()
{
#if PY_MAJOR_VERSION >= 3
#define CONST_CAST(s)   s
#else
#define CONST_CAST(s)   const_cast<char *>(s)
#endif

    static bool recursing = false;

    if (recursing)
        return;

    recursing = true;

    // Save the exception in case of new exceptions raised here.
    PyObject *exception, *value, *traceback;

    PyErr_Fetch(&exception, &value, &traceback);

    // Get the standard exception hook.
    static PyObject *original_hook = 0;

    if (!original_hook)
        original_hook = PySys_GetObject(CONST_CAST("__excepthook__"));

    // See if the application has installed its own hook.
    PyObject *hook = PySys_GetObject(CONST_CAST("excepthook"));

    if (hook != original_hook)
    {
        // This will invoke the application's hook.
        PyErr_Restore(exception, value, traceback);
        PyErr_Print();
    }
    else
    {
        // Make sure we have the StringIO ctor.
        static PyObject *stringio_ctor = 0;

        if (!stringio_ctor)
        {
            PyObject *io_module;

#if PY_MAJOR_VERSION >= 3
            io_module = PyImport_ImportModule("io");
#else
            PyErr_Clear();
            io_module = PyImport_ImportModule("cStringIO");

            if (!io_module)
            {
                PyErr_Clear();
                io_module = PyImport_ImportModule("StringIO");
            }
#endif

            if (io_module)
            {
                stringio_ctor = PyObject_GetAttrString(io_module, "StringIO");
                Py_DECREF(io_module);
            }
        }

        // Create a StringIO object and replace sys.stderr with it.
        PyObject *new_stderr = 0, *old_stderr;

        if (stringio_ctor)
        {
            old_stderr = PySys_GetObject(CONST_CAST("stderr"));

            if (old_stderr)
            {
                new_stderr = PyObject_CallObject(stringio_ctor, NULL);

                if (new_stderr)
                {
                    // Make sure the old stderr doesn't get garbage collected
                    // when it is replaced.
                    Py_INCREF(old_stderr);

                    if (PySys_SetObject(CONST_CAST("stderr"), new_stderr) < 0)
                    {
                        Py_DECREF(old_stderr);
                        Py_DECREF(new_stderr);
                        new_stderr = 0;
                    }
                }
            }
        }

        // Restore the exception and print it.
        PyErr_Restore(exception, value, traceback);
        PyErr_Print();

        // This will be passed to qFatal() if we can't get the detailed text.
        QByteArray message("Unhandled Python exception");

        // Extract the detailed text if it was redirected.
        if (new_stderr)
        {
            // Restore sys.stderr.
            PySys_SetObject(CONST_CAST("stderr"), old_stderr);
            Py_DECREF(old_stderr);

            // Extract the text.
            PyObject *text = PyObject_CallMethod(new_stderr,
                    CONST_CAST("getvalue"), NULL);

            if (text)
            {
                // Strip the text as qFatal() likes to add a newline.
                PyObject *stripped = PyObject_CallMethod(text,
                        CONST_CAST("strip"), NULL);

                if (stripped)
                {
                    Py_DECREF(text);
                    text = stripped;
                }

                // Encode the text using the encoding of the original
                // sys.stderr.

#if PY_MAJOR_VERSION >= 3
                PyObject *encoding = PyObject_GetAttrString(old_stderr,
                        "encoding");

                if (encoding)
                {
                    PyObject *encoding_bytes = PyUnicode_AsUTF8String(
                            encoding);

                    if (encoding_bytes)
                    {
                        Q_ASSERT(PyBytes_Check(encoding_bytes));

                        PyObject *bytes = PyUnicode_AsEncodedString(text,
                                PyBytes_AsString(encoding_bytes), "strict");

                        if (bytes)
                        {
                            Q_ASSERT(PyBytes_Check(bytes));

                            message = QByteArray(PyBytes_AsString(bytes),
                                    PyBytes_Size(bytes));

                            Py_DECREF(bytes);
                        }

                        Py_DECREF(encoding_bytes);
                    }

                    Py_DECREF(encoding);
                }
#else
                char *buffer;
                Py_ssize_t length;

                if (PyString_AsStringAndSize(text, &buffer, &length) == 0)
                    message = QByteArray(buffer, length);
#endif

                Py_DECREF(text);
            }

            Py_DECREF(new_stderr);
        }

        // qFatal() may not call abort.
        Py_BEGIN_ALLOW_THREADS
        qFatal("%s", message.data());
        Py_END_ALLOW_THREADS
    }

    recursing = false;
}


// Convert a Python argv list to a conventional C argc count and argv array.
char **pyqt5_from_argv_list(PyObject *argv_list, int &argc)
{
    argc = PyList_Size(argv_list);

    // Allocate space for two copies of the argument pointers, plus the
    // terminating NULL.
    char **argv = new char *[2 * (argc + 1)];

    // Convert the list.
    for (int a = 0; a < argc; ++a)
    {
        PyObject *arg_obj = PyList_GetItem(argv_list, a);
        char *arg;

        if (PyUnicode_Check(arg_obj))
        {
            QByteArray ba_arg = qpycore_PyObject_AsQString(arg_obj).toLocal8Bit();
            arg = qstrdup(ba_arg.constData());
        }
        else if (SIPBytes_Check(arg_obj))
        {
            arg = qstrdup(SIPBytes_AsString(arg_obj));
        }
        else
        {
            arg = const_cast<char *>("invalid");
        }

        argv[a] = argv[a + argc + 1] = arg;
    }

    argv[argc + argc + 1] = argv[argc] = NULL;

    return argv;
}


// Get the receiver object and slot signature for a connection.
sipErrorState pyqt5_get_connection_parts(PyObject *slot, QObject *transmitter,
        const char *signal_signature, bool single_shot, QObject **receiver,
        QByteArray &slot_signature)
{ 
    static QHash<QByteArray, const Chimera::Signature *> cache;

    QByteArray key(signal_signature);
    const Chimera::Signature *parsed_signal_signature = cache.value(key);

    if (!parsed_signal_signature)
    {
        parsed_signal_signature = Chimera::parse(key, "a signal argument");

        if (!parsed_signal_signature)
            return sipErrorFail;

        cache.insert(key, parsed_signal_signature);
    }

    return qpycore_get_receiver_slot_signature(slot, transmitter,
            parsed_signal_signature, single_shot, receiver, slot_signature);
}



// Get the transmitter object and signal signature from a bound signal.
sipErrorState pyqt5_get_pyqtsignal_parts(PyObject *signal,
        QObject **transmitter, QByteArray &signal_signature)
{
    if (PyObject_TypeCheck(signal, qpycore_pyqtBoundSignal_TypeObject))
    {
        qpycore_pyqtBoundSignal *bs = (qpycore_pyqtBoundSignal *)signal;

        *transmitter = bs->bound_qobject;
        signal_signature = bs->unbound_signal->parsed_signature->signature;

        return sipErrorNone;
    }

    return sipErrorContinue;
}


// Get the receiver object and slot signature from a decorated callable.
sipErrorState pyqt5_get_pyqtslot_parts(PyObject *slot, QObject **receiver,
        QByteArray &slot_signature)
{
    PyObject *py_receiver, *decorations;
    int is_err;
    void *qobj;
    Chimera::Signature *sig;
    sipMethodDef slot_m;

    // Get the QObject.
    if (!sipGetMethod(slot, &slot_m))
        goto bad_callable;

    py_receiver = slot_m.pm_self;
    if (!py_receiver)
        goto bad_callable;

    is_err = 0;

    qobj = sipForceConvertToType(py_receiver, sipType_QObject, 0,
            SIP_NO_CONVERTORS, 0, &is_err);

    if (is_err)
        goto bad_callable;

    *receiver = reinterpret_cast<QObject *>(qobj);

    // Get the decoration.
    decorations = PyObject_GetAttr(slot, qpycore_dunder_pyqtsignature);

    if (!decorations)
        goto bad_callable;

    // Use the first one ignoring any others.
    sig = Chimera::Signature::fromPyObject(PyList_GetItem(decorations, 0));
    Py_DECREF(decorations);

    slot_signature = sig->signature;
    slot_signature.prepend('1');

    return sipErrorNone;

bad_callable:
    PyErr_SetString(PyExc_TypeError,
            "callable must be a method of a QtCore.QObject instance decorated "
            "by QtCore.pyqtSlot");

    return sipErrorFail;
}


// Get the signature string for a bound or unbound signal.
sipErrorState pyqt5_get_signal_signature(PyObject *signal,
        const QObject *transmitter, QByteArray &signal_signature)
{
    qpycore_pyqtSignal *ps;

    if (PyObject_TypeCheck(signal, qpycore_pyqtBoundSignal_TypeObject))
    {
        qpycore_pyqtBoundSignal *bs = (qpycore_pyqtBoundSignal *)signal;

        if (transmitter && bs->bound_qobject != transmitter)
        {
            PyErr_SetString(PyExc_ValueError,
                    "signal is bound to a different QObject");
            return sipErrorFail;
        }

        ps = bs->unbound_signal;
    }
    else if (PyObject_TypeCheck(signal, qpycore_pyqtSignal_TypeObject))
    {
        ps = (qpycore_pyqtSignal *)signal;
    }
    else
    {
        return sipErrorContinue;
    }

    signal_signature = ps->parsed_signature->signature;

    return sipErrorNone;
}


// Register a convertor function that converts a QVariant to a Python object.
void pyqt5_register_from_qvariant_convertor(
        bool (*convertor)(const QVariant &, PyObject **))
{
    Chimera::registeredFromQVariantConvertors.append(convertor);
}


// Register a convertor function that converts a Python object to a QVariant.
void pyqt5_register_to_qvariant_convertor(
        bool (*convertor)(PyObject *, QVariant &, bool *))
{
    Chimera::registeredToQVariantConvertors.append(convertor);
}


// Register a convertor function that converts a Python object to the
// pre-allocated data of a QVariant with a specific meta-type.
void pyqt5_register_to_qvariant_data_convertor(
        bool (*convertor)(PyObject *, void *, int, bool *))
{
    Chimera::registeredToQVariantDataConvertors.append(convertor);
}


// Remove arguments from the Python argv list that have been removed from the
// C argv array.
void pyqt5_update_argv_list(PyObject *argv_list, int argc, char **argv)
{
    for (int a = 0, na = 0; a < argc; ++a)
    {
        // See if it was removed.
        if (argv[na] == argv[a + argc + 1])
            ++na;
        else
            PyList_SetSlice(argv_list, na, na + 1, 0);
    }
}


// Get the QMetaObject instance for a Python type.
const QMetaObject *pyqt5_get_qmetaobject(PyTypeObject *type)
{
    return qpycore_get_qmetaobject((sipWrapperType *)type);
}

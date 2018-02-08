// This contains the implementation of the pyqtBoundSignal type.
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

#include <QtGlobal>
#include <QByteArray>
#include <QMetaObject>

#include "qpycore_api.h"
#include "qpycore_chimera.h"
#include "qpycore_misc.h"
#include "qpycore_objectified_strings.h"
#include "qpycore_pyqtboundsignal.h"
#include "qpycore_pyqtpyobject.h"
#include "qpycore_pyqtsignal.h"
#include "qpycore_pyqtslotproxy.h"

#include "sipAPIQtCore.h"


#if PY_VERSION_HEX >= 0x02050000
#define CHAR_CAST(s)    (s)
#else
#define CHAR_CAST(s)    (const_cast<char *>(s))
#endif


// The type object.
PyTypeObject *qpycore_pyqtBoundSignal_TypeObject;


// Forward declarations.
extern "C" {
static PyObject *pyqtBoundSignal_call(PyObject *self, PyObject *args,
        PyObject *kw);
static void pyqtBoundSignal_dealloc(PyObject *self);
static PyObject *pyqtBoundSignal_repr(PyObject *self);
static PyObject *pyqtBoundSignal_get_doc(PyObject *self, void *);
static PyObject *pyqtBoundSignal_get_signal(PyObject *self, void *);
static PyObject *pyqtBoundSignal_connect(PyObject *self, PyObject *args,
        PyObject *kwd_args);
static PyObject *pyqtBoundSignal_disconnect(PyObject *self, PyObject *args);
static PyObject *pyqtBoundSignal_emit(PyObject *self, PyObject *args);
static PyObject *pyqtBoundSignal_mp_subscript(PyObject *self,
        PyObject *subscript);
}

static PyObject *disconnect(qpycore_pyqtBoundSignal *bs, QObject *qrx,
        const char *slot);
static bool do_emit(QObject *qtx, int signal_index,
        const Chimera::Signature *parsed_signature, const char *docstring,
        PyObject *sigargs);
static bool get_receiver(PyObject *slot,
        const Chimera::Signature *signal_signature, QObject **receiver,
        QByteArray &slot_signature);
static void slot_signature_from_decorations(QByteArray &slot_signature,
        const Chimera::Signature *signal_signature, PyObject *decorations);
static QByteArray slot_signature_from_signal(
        const Chimera::Signature *signal_signature,
        const QByteArray &slot_name, int nr_args);
static sipErrorState get_receiver_slot_signature(PyObject *slot,
        QObject *transmitter, const Chimera::Signature *signal_signature,
        bool single_shot, QObject **receiver, QByteArray &slot_signature,
        bool unique_connection_check, int no_receiver_check);
static void add_slot_prefix(QByteArray &slot_signature);


// Doc-strings.
PyDoc_STRVAR(pyqtBoundSignal_connect_doc,
"connect(slot, type=Qt.AutoConnection, no_receiver_check=False)\n"
"\n"
"slot is either a Python callable or another signal.\n"
"type is a Qt.ConnectionType.\n"
"no_receiver_check is True to disable the check that the receiver's C++\n"
"instance still exists when the signal is emitted.\n");

PyDoc_STRVAR(pyqtBoundSignal_disconnect_doc,
"disconnect([slot])\n"
"\n"
"slot is an optional Python callable or another signal.  If it is omitted\n"
"then the signal is disconnected from everything it is connected to.");

PyDoc_STRVAR(pyqtBoundSignal_emit_doc,
"emit(*args)\n"
"\n"
"*args are the values that will be passed as arguments to all connected\n"
"slots.");

PyDoc_STRVAR(pyqtBoundSignal_signal_doc,
"The signature of the signal that would be returned by SIGNAL()");


// Define the methods.
static PyMethodDef pyqtBoundSignal_methods[] = {
    {CHAR_CAST("connect"), (PyCFunction)pyqtBoundSignal_connect,
            METH_VARARGS|METH_KEYWORDS,
            CHAR_CAST(pyqtBoundSignal_connect_doc)},
    {CHAR_CAST("disconnect"), pyqtBoundSignal_disconnect,
            METH_VARARGS, CHAR_CAST(pyqtBoundSignal_disconnect_doc)},
    {CHAR_CAST("emit"), pyqtBoundSignal_emit,
            METH_VARARGS, CHAR_CAST(pyqtBoundSignal_emit_doc)},
    {0, 0, 0, 0}
};


// The getters/setters.
static PyGetSetDef pyqtBoundSignal_getset[] = {
    {(char *)"__doc__", pyqtBoundSignal_get_doc, NULL, NULL, NULL},
    {(char *)"signal", pyqtBoundSignal_get_signal, NULL,
            (char *)pyqtBoundSignal_signal_doc, NULL},
    {NULL, NULL, NULL, NULL, NULL}
};


#if PY_VERSION_HEX >= 0x03040000
// Define the slots.
static PyType_Slot qpycore_pyqtBoundSignal_Slots[] = {
    {Py_tp_new,         (void *)PyType_GenericNew},
    {Py_tp_dealloc,     (void *)pyqtBoundSignal_dealloc},
    {Py_tp_repr,        (void *)pyqtBoundSignal_repr},
    {Py_tp_call,        (void *)pyqtBoundSignal_call},
    {Py_mp_subscript,   (void *)pyqtBoundSignal_mp_subscript},
    {Py_tp_methods,     pyqtBoundSignal_methods},
    {Py_tp_getset,      pyqtBoundSignal_getset},
    {0,                 0}
};


// Define the type.
static PyType_Spec qpycore_pyqtBoundSignal_Spec = {
    "PyQt5.QtCore.pyqtBoundSignal",
    sizeof (qpycore_pyqtBoundSignal),
    0,
    Py_TPFLAGS_DEFAULT,
    qpycore_pyqtBoundSignal_Slots
};
#else
// Define the mapping methods.
static PyMappingMethods pyqtBoundSignal_as_mapping = {
    0,                      /* mp_length */
    pyqtBoundSignal_mp_subscript,   /* mp_subscript */
    0,                      /* mp_ass_subscript */
};


// Define the type.
static PyTypeObject qpycore_pyqtBoundSignal_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    CHAR_CAST("PyQt5.QtCore.pyqtBoundSignal"),  /* tp_name */
    sizeof (qpycore_pyqtBoundSignal),   /* tp_basicsize */
    0,                      /* tp_itemsize */
    pyqtBoundSignal_dealloc,    /* tp_dealloc */
    0,                      /* tp_print */
    0,                      /* tp_getattr */
    0,                      /* tp_setattr */
    0,                      /* tp_compare */
    pyqtBoundSignal_repr,   /* tp_repr */
    0,                      /* tp_as_number */
    0,                      /* tp_as_sequence */
    &pyqtBoundSignal_as_mapping,    /* tp_as_mapping */
    0,                      /* tp_hash */
    pyqtBoundSignal_call,   /* tp_call */
    0,                      /* tp_str */
    0,                      /* tp_getattro */
    0,                      /* tp_setattro */
    0,                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,     /* tp_flags */
    0,                      /* tp_doc */
    0,                      /* tp_traverse */
    0,                      /* tp_clear */
    0,                      /* tp_richcompare */
    0,                      /* tp_weaklistoffset */
    0,                      /* tp_iter */
    0,                      /* tp_iternext */
    pyqtBoundSignal_methods,    /* tp_methods */
    0,                      /* tp_members */
    pyqtBoundSignal_getset,     /* tp_getset */
    0,                      /* tp_base */
    0,                      /* tp_dict */
    0,                      /* tp_descr_get */
    0,                      /* tp_descr_set */
    0,                      /* tp_dictoffset */
    0,                      /* tp_init */
    0,                      /* tp_alloc */
    PyType_GenericNew,      /* tp_new */
    0,                      /* tp_free */
    0,                      /* tp_is_gc */
    0,                      /* tp_bases */
    0,                      /* tp_mro */
    0,                      /* tp_cache */
    0,                      /* tp_subclasses */
    0,                      /* tp_weaklist */
    0,                      /* tp_del */
    0,                      /* tp_version_tag */
#if PY_VERSION_HEX >= 0x03040000
    0,                      /* tp_finalize */
#endif
};
#endif


// The __doc__ getter.
static PyObject *pyqtBoundSignal_get_doc(PyObject *self, void *)
{
    qpycore_pyqtBoundSignal *bs = (qpycore_pyqtBoundSignal *)self;

    const char *docstring = bs->unbound_signal->docstring;

    if (!docstring)
    {
        Py_INCREF(Py_None);
        return Py_None;
    }

    if (*docstring == '\1')
        ++docstring;

    return
#if PY_MAJOR_VERSION >= 3
        PyUnicode_FromString
#else
        PyString_FromString
#endif
            (docstring);
}


// The __signal__ getter.
static PyObject *pyqtBoundSignal_get_signal(PyObject *self, void *)
{
    qpycore_pyqtBoundSignal *bs = (qpycore_pyqtBoundSignal *)self;

    return
#if PY_MAJOR_VERSION >= 3
        PyUnicode_FromString
#else
        PyString_FromString
#endif
            (bs->unbound_signal->parsed_signature->signature.constData());
}


// The type repr slot.
static PyObject *pyqtBoundSignal_repr(PyObject *self)
{
    qpycore_pyqtBoundSignal *bs = (qpycore_pyqtBoundSignal *)self;

    QByteArray name = bs->unbound_signal->parsed_signature->name();

    return
#if PY_MAJOR_VERSION >= 3
        PyUnicode_FromFormat
#else
        PyString_FromFormat
#endif
            ("<bound PYQT_SIGNAL %s of %s object at %p>", name.constData() + 1,
                    sipPyTypeName(Py_TYPE(bs->bound_pyobject)),
                    bs->bound_pyobject);
}


// The type call slot.
static PyObject *pyqtBoundSignal_call(PyObject *self, PyObject *args,
        PyObject *kw)
{
    qpycore_pyqtBoundSignal *bs = (qpycore_pyqtBoundSignal *)self;

    return qpycore_call_signal_overload(bs->unbound_signal, bs->bound_pyobject,
            args, kw);
}


// The type dealloc slot.
static void pyqtBoundSignal_dealloc(PyObject *self)
{
    qpycore_pyqtBoundSignal *bs = (qpycore_pyqtBoundSignal *)self;

    Py_XDECREF((PyObject *)bs->unbound_signal);

    PyObject_Del(self);
}


// Initialise the type and return true if there was no error.
bool qpycore_pyqtBoundSignal_init_type()
{
#if PY_VERSION_HEX >= 0x03040000
    qpycore_pyqtBoundSignal_TypeObject = (PyTypeObject *)PyType_FromSpec(
            &qpycore_pyqtBoundSignal_Spec);

    return qpycore_pyqtBoundSignal_TypeObject;
#else
    if (PyType_Ready(&qpycore_pyqtBoundSignal_Type) < 0)
        return false;

    qpycore_pyqtBoundSignal_TypeObject = &qpycore_pyqtBoundSignal_Type;

    return true;
#endif
}


// Create a bound signal.
PyObject *qpycore_pyqtBoundSignal_New(qpycore_pyqtSignal *unbound_signal,
        PyObject *bound_pyobject, QObject *bound_qobject)
{
    qpycore_pyqtBoundSignal *bs = (qpycore_pyqtBoundSignal *)PyType_GenericNew(
            qpycore_pyqtBoundSignal_TypeObject, 0, 0);

    if (bs)
    {
        Py_INCREF((PyObject *)unbound_signal);
        bs->unbound_signal = unbound_signal;

        bs->bound_pyobject = bound_pyobject;
        bs->bound_qobject = bound_qobject;
    }

    return (PyObject *)bs;
}


// The mapping subscript slot.
static PyObject *pyqtBoundSignal_mp_subscript(PyObject *self,
        PyObject *subscript)
{
    qpycore_pyqtBoundSignal *bs = (qpycore_pyqtBoundSignal *)self;

    qpycore_pyqtSignal *ps = qpycore_find_signal(bs->unbound_signal, subscript,
            "a bound signal type argument");

    if (!ps)
        return 0;

    // Create a new bound signal.
    return qpycore_pyqtBoundSignal_New(ps, bs->bound_pyobject,
            bs->bound_qobject);
}


// Connect a signal.
static PyObject *pyqtBoundSignal_connect(PyObject *self, PyObject *args,
        PyObject *kwd_args)
{
    qpycore_pyqtBoundSignal *bs = (qpycore_pyqtBoundSignal *)self;

    static const char *kwds[] = {
        "slot",
        "type",
        "no_receiver_check",
        0
    };

    PyObject *py_slot, *py_type = 0;
    int no_receiver_check = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwd_args,
#if PY_VERSION_HEX >= 0x03030000
                "O|Op:connect",
#else
                "O|Oi:connect",
#endif
                const_cast<char **>(kwds), &py_slot, &py_type, &no_receiver_check))
        return 0;

    Qt::ConnectionType q_type = Qt::AutoConnection;

    if (py_type)
    {
        int v = sipConvertToEnum(py_type, sipType_Qt_ConnectionType);

        if (PyErr_Occurred())
        {
            PyErr_Format(PyExc_TypeError,
                    "Qt.ConnectionType expected, not '%s'",
                    sipPyTypeName(Py_TYPE(py_slot)));

            return 0;
        }

        q_type = static_cast<Qt::ConnectionType>(v);
    }

    QObject *q_tx = bs->bound_qobject, *q_rx;
    Chimera::Signature *signal_signature = bs->unbound_signal->parsed_signature;
    QByteArray slot_signature;

    sipErrorState estate = get_receiver_slot_signature(py_slot, q_tx,
            signal_signature, false, &q_rx, slot_signature,
            ((q_type & Qt::UniqueConnection) == Qt::UniqueConnection),
            no_receiver_check);

    if (estate != sipErrorNone)
    {
        if (estate == sipErrorContinue)
            sipBadCallableArg(0, py_slot);

        return 0;
    }

    // Connect the signal to the slot and handle any errors.

    bool ok;

    Py_BEGIN_ALLOW_THREADS
    ok = QObject::connect(q_tx, signal_signature->signature.constData(), q_rx,
            slot_signature.constData(), q_type);
    Py_END_ALLOW_THREADS

    if (!ok)
    {
        QByteArray slot_name = Chimera::Signature::name(slot_signature);

        PyErr_Format(PyExc_TypeError, "connect() failed between %s and %s()",
                signal_signature->py_signature.constData(),
                slot_name.constData() + 1);

        return 0;
    }

    Py_INCREF(Py_None);
    return Py_None;
}


// Get the receiver object and slot signature from a callable or signal.
sipErrorState qpycore_get_receiver_slot_signature(PyObject *slot,
        QObject *transmitter, const Chimera::Signature *signal_signature,
        bool single_shot, QObject **receiver, QByteArray &slot_signature)
{
    return get_receiver_slot_signature(slot, transmitter, signal_signature,
            single_shot, receiver, slot_signature, false, 0);
}


// Get the receiver object and slot signature from a callable or signal.
// Optionally disable the receiver check.
static sipErrorState get_receiver_slot_signature(PyObject *slot,
        QObject *transmitter, const Chimera::Signature *signal_signature,
        bool single_shot, QObject **receiver, QByteArray &slot_signature,
        bool unique_connection_check, int no_receiver_check)
{
    // See if the slot is a signal.
    if (PyObject_TypeCheck(slot, qpycore_pyqtBoundSignal_TypeObject))
    {
        qpycore_pyqtBoundSignal *bs = (qpycore_pyqtBoundSignal *)slot;

        *receiver = bs->bound_qobject;
        slot_signature = bs->unbound_signal->parsed_signature->signature;

        return sipErrorNone;
    }

    // Make sure the slot is callable.
    if (!PyCallable_Check(slot))
        return sipErrorContinue;

    // See if the slot can be used directly (ie. it wraps a Qt slot) or if it
    // needs a proxy.
    if (!get_receiver(slot, signal_signature, receiver, slot_signature))
        return sipErrorFail;

    if (slot_signature.isEmpty())
    {
        slot_signature = PyQtSlotProxy::proxy_slot_signature;

        // Create a proxy for the slot.
        PyQtSlotProxy *proxy;

        if (unique_connection_check)
        {
            proxy = PyQtSlotProxy::findSlotProxy(transmitter,
                    signal_signature->signature, slot);

            if (proxy)
            {
                // We give more information than we could if it was a Qt slot
                // but to be consistent we raise a TypeError even though it's
                // not the most appropriate for the type of error.
                PyErr_SetString(PyExc_TypeError, "connection is not unique");
                return sipErrorFail;
            }
        }

        Py_BEGIN_ALLOW_THREADS

        proxy = new PyQtSlotProxy(slot, transmitter, signal_signature,
                single_shot);

        if (no_receiver_check)
            proxy->disableReceiverCheck();

        if (proxy->metaObject())
        {
            if (*receiver)
                proxy->moveToThread((*receiver)->thread());

            *receiver = proxy;
        }
        else
        {
            delete proxy;
            proxy = 0;
        }

        Py_END_ALLOW_THREADS

        if (!proxy)
            return sipErrorFail;
    }

    return sipErrorNone;
}


// Disconnect all of a QObject's signals.
PyObject *qpycore_qobject_disconnect(const QObject *q_obj)
{
    PyObject *res_obj;
    bool ok;

    Py_BEGIN_ALLOW_THREADS
    ok = q_obj->disconnect();
    Py_END_ALLOW_THREADS

    if (ok)
    {
        res_obj = Py_None;
        Py_INCREF(res_obj);
    }
    else
    {
        PyErr_SetString(PyExc_TypeError, "disconnect() of all signals failed");
        res_obj = 0;
    }

    PyQtSlotProxy::deleteSlotProxies(q_obj, QByteArray());

    return res_obj;
}


// Disconnect a signal.
static PyObject *pyqtBoundSignal_disconnect(PyObject *self, PyObject *args)
{
    qpycore_pyqtBoundSignal *bs = (qpycore_pyqtBoundSignal *)self;

    PyObject *py_slot = 0, *res_obj;
    Chimera::Signature *signal_signature = bs->unbound_signal->parsed_signature;

    if (!PyArg_ParseTuple(args, "|O:disconnect", &py_slot))
        return 0;

    // See if we are disconnecting everything from the overload.
    if (!py_slot)
    {
        res_obj = disconnect(bs, 0, 0);

        PyQtSlotProxy::deleteSlotProxies(bs->bound_qobject,
                signal_signature->signature);

        return res_obj;
    }

    // See if the slot is a signal.
    if (PyObject_TypeCheck(py_slot, qpycore_pyqtBoundSignal_TypeObject))
    {
        qpycore_pyqtBoundSignal *slot_bs = (qpycore_pyqtBoundSignal *)py_slot;

        return disconnect(bs, slot_bs->bound_qobject,
                slot_bs->unbound_signal->parsed_signature->signature.constData());
    }

    if (!PyCallable_Check(py_slot))
    {
        sipBadCallableArg(0, py_slot);
        return 0;
    }

    // See if the slot has been used directly (ie. it wraps a Qt slot) or if it
    // has a proxy.
    QObject *q_rx;
    QByteArray slot_signature;

    if (!get_receiver(py_slot, signal_signature, &q_rx, slot_signature))
        return 0;

    if (!slot_signature.isEmpty())
        return disconnect(bs, q_rx, slot_signature.constData());

    PyQtSlotProxy *proxy = PyQtSlotProxy::findSlotProxy(bs->bound_qobject,
            signal_signature->signature, py_slot);

    if (!proxy)
    {
        PyErr_Format(PyExc_TypeError, "'%s' object is not connected",
                sipPyTypeName(Py_TYPE(py_slot)));

        return 0;
    }

    res_obj = disconnect(bs, proxy,
            PyQtSlotProxy::proxy_slot_signature.constData());

    proxy->disable();

    return res_obj;
}


// Disonnect a signal from a slot and handle any errors.
static PyObject *disconnect(qpycore_pyqtBoundSignal *bs, QObject *qrx,
        const char *slot)
{
    Chimera::Signature *signature = bs->unbound_signal->parsed_signature;
    bool ok;

    Py_BEGIN_ALLOW_THREADS
    ok = QObject::disconnect(bs->bound_qobject,
            signature->signature.constData(), qrx, slot);
    Py_END_ALLOW_THREADS

    if (!ok)
    {
        QByteArray tx_name = signature->name();

        if (slot)
        {
            QByteArray rx_name = Chimera::Signature::name(slot);

            PyErr_Format(PyExc_TypeError,
                    "disconnect() failed between '%s' and '%s'",
                    tx_name.constData() + 1, rx_name.constData() + 1);
        }
        else
        {
            PyErr_Format(PyExc_TypeError,
                    "disconnect() failed between '%s' and all its connections",
                    tx_name.constData() + 1);
        }

        return 0;
    }

    Py_INCREF(Py_None);
    return Py_None;
}


// Emit a signal.
static PyObject *pyqtBoundSignal_emit(PyObject *self, PyObject *args)
{
    qpycore_pyqtBoundSignal *bs = (qpycore_pyqtBoundSignal *)self;

    if (!bs->bound_qobject->signalsBlocked())
    {
        Q_ASSERT(PyTuple_Check(args));

        qpycore_pyqtSignal *ps = bs->unbound_signal;

        // Use the emitter if there is one.
        if (ps->emitter)
        {
            if (ps->emitter(bs->bound_qobject, args) < 0)
                return 0;
        }
        else
        {
            Chimera::Signature *signature = ps->parsed_signature;
            int mo_index = bs->bound_qobject->metaObject()->indexOfSignal(
                    signature->signature.constData() + 1);

            if (mo_index < 0)
            {
                PyErr_Format(PyExc_AttributeError,
                        "'%s' does not have a signal with the signature %s",
                        sipPyTypeName(Py_TYPE(bs->bound_pyobject)),
                        signature->signature.constData() + 1);
                return 0;
            }

            // Use the docstring if there is one and it is auto-generated.
            const char *docstring = bs->unbound_signal->docstring;

            if (!docstring || *docstring != '\1')
            {
                docstring = signature->py_signature.constData();
            }
            else
            {
                // Skip the auto-generated marker.
                ++docstring;
            }

            if (!do_emit(bs->bound_qobject, mo_index, signature, docstring, args))
                return 0;
        }
    }

    Py_INCREF(Py_None);
    return Py_None;
}


// Emit a signal based on a parsed signature.
static bool do_emit(QObject *qtx, int signal_index,
        const Chimera::Signature *parsed_signature, const char *docstring,
        PyObject *sigargs)
{
    const QList<const Chimera *> &args = parsed_signature->parsed_arguments;

    if (args.size() != PyTuple_Size(sigargs))
    {
        PyErr_Format(PyExc_TypeError,
                "%s signal has %d argument(s) but %d provided", docstring,
                args.size(), (int)PyTuple_Size(sigargs));

        return false;
    }

    // Convert the arguments.
    QList<Chimera::Storage *> values;
    void **argv = new void *[1 + args.size()];

    argv[0] = 0;

    QList<const Chimera *>::const_iterator it = args.constBegin();

    for (int a = 0; it != args.constEnd(); ++a)
    {
        PyObject *arg_obj = PyTuple_GetItem(sigargs, a);
        Chimera::Storage *val = (*it)->fromPyObjectToStorage(arg_obj);

        if (!val)
        {
            // Mimic SIP's exception text.
            PyErr_Format(PyExc_TypeError,
                    "%s.emit(): argument %d has unexpected type '%s'",
                    docstring, a + 1, sipPyTypeName(Py_TYPE(arg_obj)));

            delete[] argv;
            qDeleteAll(values.constBegin(), values.constEnd());

            return false;
        }

        argv[1 + a] = val->address();
        values << val;

        ++it;
    }

    Py_BEGIN_ALLOW_THREADS
    QMetaObject::activate(qtx, signal_index, argv);
    Py_END_ALLOW_THREADS

    delete[] argv;
    qDeleteAll(values.constBegin(), values.constEnd());

    return true;
}


// Get the receiver QObject from the slot (if there is one) and its signature
// (if it wraps a Qt slot).  Return true if there was no error.
static bool get_receiver(PyObject *slot,
        const Chimera::Signature *signal_signature, QObject **receiver,
        QByteArray &slot_signature)
{
    bool try_qt_slot = false;
    PyObject *rx_self = 0;
    QByteArray rx_name;
    sipMethodDef slot_m;
    sipCFunctionDef slot_cf;

    // Assume there isn't a QObject receiver.
    *receiver = 0;

    if (sipGetMethod(slot, &slot_m))
    {
        rx_self = slot_m.pm_self;

        // The method may be any callable so don't assume it has a __name__.
        PyObject *f_name_obj = PyObject_GetAttr(slot_m.pm_function,
                qpycore_dunder_name);
        if (!f_name_obj)
            return false;

        PyObject *f_name_owner_obj = f_name_obj;
        const char *f_name = sipString_AsASCIIString(&f_name_owner_obj);
        Py_DECREF(f_name_obj);

        if (!f_name)
            return false;

        rx_name = f_name;
        Py_DECREF(f_name_owner_obj);

        // See if this has been decorated.
        PyObject *decorations = PyObject_GetAttr(slot_m.pm_function,
                qpycore_dunder_pyqtsignature);

        if (decorations)
        {
            // Choose from the decorations.
            slot_signature_from_decorations(slot_signature, signal_signature,
                    decorations);

            Py_DECREF(decorations);

            if (slot_signature.isEmpty())
            {
                PyErr_Format(PyExc_TypeError,
                        "decorated slot has no signature compatible with %s",
                        signal_signature->py_signature.constData());
                return false;
            }
        }

        Py_XINCREF(rx_self);
    }
    else if (sipGetCFunction(slot, &slot_cf))
    {
        rx_self = slot_cf.cf_self;
        rx_name = slot_cf.cf_function->ml_name;

        // We actually want the C++ name which may (in theory) be completely
        // different.  However this will cope with the exec_ case which is
        // probably good enough.
        if (rx_name.endsWith('_'))
            rx_name.chop(1);

        try_qt_slot = true;

        Py_XINCREF(rx_self);
    }
    else
    {
        static PyObject *partial = 0;

        // Get the functools.partial type object if we haven't already got it.
        if (!partial)
        {
            PyObject *functools = PyImport_ImportModule("functools");

            if (functools)
            {
                partial = PyObject_GetAttrString(functools, "partial");
                Py_DECREF(functools);
            }
        }

        // If we know about functools.partial then remove the outer partials to
        // get to the original function.
        if (partial && PyObject_IsInstance(slot, partial) > 0)
        {
            PyObject *func = slot;
            sipMethodDef func_m;
            sipCFunctionDef func_cf;

            Py_INCREF(func);

            do
            {
                PyObject *subfunc = PyObject_GetAttrString(func, "func");

                Py_DECREF(func);

                // This should never happen.
                if (!subfunc)
                    return false;

                func = subfunc;
            }
            while (PyObject_IsInstance(func, partial) > 0);

            if (sipGetMethod(func, &func_m))
                rx_self = func_m.pm_self;
            else if (sipGetCFunction(func, &func_cf))
                rx_self = func_cf.cf_self;

            Py_XINCREF(rx_self);
            Py_DECREF(func);
        }
    }
 
    if (!rx_self)
        return true;

    int iserr = 0;
    void *rx = sipForceConvertToType(rx_self, sipType_QObject, 0,
            SIP_NO_CONVERTORS, 0, &iserr);

    Py_DECREF(rx_self);

    PyErr_Clear();

    if (iserr)
        return true;

    *receiver = reinterpret_cast<QObject *>(rx);

    // If there might be a Qt slot that can handle the arguments (or a subset
    // of them) then use it.  Otherwise we will fallback to using a proxy.
    if (try_qt_slot)
    {
        const QMetaObject *mo = (*receiver)->metaObject();

        for (int ol = signal_signature->parsed_arguments.count(); ol >= 0; --ol)
        {
            slot_signature = slot_signature_from_signal(signal_signature,
                    rx_name, ol);

            if (mo->indexOfSlot(slot_signature.constData()) >= 0)
            {
                add_slot_prefix(slot_signature);
                break;
            }

            slot_signature.clear();
        }
    }

    return true;
}


// Return the full name and signature of a Qt slot that a signal can be
// connected to, taking the slot decorators into account.
static void slot_signature_from_decorations(QByteArray &slot_signature,
        const Chimera::Signature *signal, PyObject *decorations)
{
    Chimera::Signature *candidate = 0;
    int signal_nr_args = signal->parsed_arguments.count();

    for (Py_ssize_t i = 0; i < PyList_Size(decorations); ++i)
    {
        Chimera::Signature *slot = Chimera::Signature::fromPyObject(
                PyList_GetItem(decorations, i));

        int slot_nr_args = slot->parsed_arguments.count();

        // Ignore the slot if it requires more arguments than the signal will
        // provide.
        if (slot_nr_args > signal_nr_args)
            continue;

        // Ignore the slot if any current candidate will accept more arguments.
        if (candidate && candidate->parsed_arguments.count() >= slot_nr_args)
            continue;

        for (int a = 0; a < slot_nr_args; ++a)
        {
            const Chimera *sig_arg = signal->parsed_arguments.at(a);
            const Chimera *slot_arg = slot->parsed_arguments.at(a);

            // We simply compare meta-types.
            if (sig_arg->metatype() != slot_arg->metatype())
            {
                slot = 0;
                break;
            }
        }

        // If all of the slot's arguments were Ok then this will be the best
        // candidate so far.
        if (slot)
            candidate = slot;
    }

    if (candidate)
    {
        slot_signature = candidate->signature;
        add_slot_prefix(slot_signature);
    }
}


// Return the full name and signature of the Qt slot that a signal would be
// connected to.
static QByteArray slot_signature_from_signal(
        const Chimera::Signature *signal_signature,
        const QByteArray &slot_name, int nr_args)
{
    QByteArray slot_sig = slot_name;

    slot_sig.append('(');

    for (int a = 0; a < nr_args; ++a)
    {
        if (a != 0)
            slot_sig.append(',');

        slot_sig.append(signal_signature->parsed_arguments.at(a)->name());
    }

    slot_sig.append(')');

    return slot_sig;
}


// Add the prefix to a signaturethat tells Qt it is a slot.
static void add_slot_prefix(QByteArray &slot_signature)
{
    slot_signature.prepend('1');
}

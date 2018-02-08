// This contains the implementation of the PyQtSlot class.
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

#include "qpycore_chimera.h"
#include "qpycore_pyqtslot.h"


// Create the slot for an unbound method.
PyQtSlot::PyQtSlot(PyObject *method, PyObject *type,
        const Chimera::Signature *slot_signature)
    : mfunc(method), mself(0), mself_wr(0), other(0), signature(slot_signature)
{
#if PY_MAJOR_VERSION < 3
    mclass = type;
#else
    Q_UNUSED(type)
#endif
}


// Create the slot for a callable.
PyQtSlot::PyQtSlot(PyObject *callable, const Chimera::Signature *slot_signature)
    : mfunc(0), mself(0), mself_wr(0), other(0), signature(slot_signature)
{
    sipMethodDef callable_m;

    if (sipGetMethod(callable, &callable_m))
    {
        // Save the component parts.
        mfunc = callable_m.pm_function;
        mself = callable_m.pm_self;
#if PY_MAJOR_VERSION < 3
        mclass = callable_m.pm_class;
#endif

        // Try and create a weak reference to the instance object.
        mself_wr = PyWeakref_NewRef(mself, 0);
    }
    else
    {
        // Give the slot an extra reference to keep it alive.
        Py_INCREF(callable);
        other = callable;
    }
}


// Destroy the slot.
PyQtSlot::~PyQtSlot()
{
    Py_XDECREF(mself_wr);
    Py_XDECREF(other);
}


// Invoke the slot on behalf of C++.
PyQtSlot::Result PyQtSlot::invoke(void **qargs, bool no_receiver_check) const
{
    return invoke(qargs, 0, 0, no_receiver_check);
}


// Invoke the slot on behalf of C++.
bool PyQtSlot::invoke(void **qargs, PyObject *self, void *result) const
{
    return (invoke(qargs, self, result, false) != PyQtSlot::Failed);
}


// Invoke the slot on behalf of C++.
PyQtSlot::Result PyQtSlot::invoke(void **qargs, PyObject *self, void *result,
        bool no_receiver_check) const
{
    // Get the callable.
    PyObject *callable;

    if (other)
    {
        callable = other;
        Py_INCREF(callable);
    }
    else
    {
        // Use the value we have if one wasn't supplied.
        if (!self)
            self = instance();

        // If self is NULL then we didn't have a method in the first place.
        // Instead we had a callable that has been cleared during garbage
        // collection - so we can simply ignore the invocation.
        if (!self)
            return PyQtSlot::Ignored;

        // See if the instance has gone (which isn't an error).
        if (self == Py_None)
            return PyQtSlot::Ignored;

        // If the receiver wraps a C++ object then ignore the call if it no
        // longer exists.
        if (!no_receiver_check && PyObject_TypeCheck(self, sipSimpleWrapper_Type) && !sipGetAddress((sipSimpleWrapper *)self))
            return PyQtSlot::Ignored;

        sipMethodDef callable_m;

        callable_m.pm_function = mfunc;
        callable_m.pm_self = self;
#if PY_MAJOR_VERSION < 3
        callable_m.pm_class = mclass;
#endif

        callable = sipFromMethod(&callable_m);
    }

    // Convert the C++ arguments to Python objects.
    const QList<const Chimera *> &args = signature->parsed_arguments;

    PyObject *argtup = PyTuple_New(args.size());

    if (!argtup)
        return PyQtSlot::Failed;

    QList<const Chimera *>::const_iterator it = args.constBegin();

    for (int a = 0; it != args.constEnd(); ++a)
    {
        PyObject *arg = (*it)->toPyObject(*++qargs);

        if (!arg)
        {
            Py_DECREF(argtup);
            return PyQtSlot::Failed;
        }

        PyTuple_SetItem(argtup, a, arg);

        ++it;
    }

    // Dispatch to the real slot.
    PyObject *res = call(callable, argtup);

    Py_DECREF(argtup);
    Py_DECREF(callable);

    if (!res)
        return PyQtSlot::Failed;

    // Handle any result if required.
    bool ok;

    if (result && signature->result)
        ok = signature->result->fromPyObject(res, result);
    else
        ok = true;

    Py_DECREF(res);

    return (ok ? PyQtSlot::Succeeded : PyQtSlot::Failed);
}


// See if this slot corresponds to the given callable.
bool PyQtSlot::operator==(PyObject *callable) const
{
    sipMethodDef callable_m;

    if (sipGetMethod(callable, &callable_m))
    {
        if (other)
            return false;

        return (mfunc == callable_m.pm_function
                && instance() == callable_m.pm_self
#if PY_MAJOR_VERSION < 3
                && mclass == callable_m.pm_class
#endif
                );
    }

    if (!other)
        return false;

    // See if it is a wrapped C++ method.  Note that the PyQt4 behaviour is to
    // not save a reference but to save the components (as we do with methods).
    // Hopefully it won't make a difference.  However it begs the question as
    // to whether we should do the same with methods and rely on the garbage
    // collector - is the current way of handling methods purely historical?
    sipCFunctionDef other_cf, callable_cf;

    if (sipGetCFunction(other, &other_cf) && sipGetCFunction(callable, &callable_cf))
        return (other_cf.cf_self == callable_cf.cf_self &&
                other_cf.cf_function->ml_meth == callable_cf.cf_function->ml_meth);

    return (other == callable);
}


// Get the instance object.
PyObject *PyQtSlot::instance() const
{
    // Use the weak reference if possible.
    if (mself_wr)
        return PyWeakref_GetObject(mself_wr);

    return mself;
}


// Call a single slot and return the result.
PyObject *PyQtSlot::call(PyObject *callable, PyObject *args) const
{
    PyObject *sa, *oxtype, *oxvalue, *oxtb;

    // Keep some compilers quiet.
    oxtype = oxvalue = oxtb = 0;

    // We make repeated attempts to call a slot.  If we work out that it failed
    // because of an immediate type error we try again with one less argument.
    // We keep going until we run out of arguments to drop.  This emulates the
    // Qt ability of the slot to accept fewer arguments than a signal provides.
    sa = args;
    Py_INCREF(sa);

    for (;;)
    {
        PyObject *nsa, *xtype, *xvalue, *xtb, *res;

        if ((res = PyEval_CallObject(callable, sa)) != NULL)
        {
            // Remove any previous exception.

            if (sa != args)
            {
                Py_XDECREF(oxtype);
                Py_XDECREF(oxvalue);
                Py_XDECREF(oxtb);
                PyErr_Clear();
            }

            Py_DECREF(sa);

            return res;
        }

        // Get the exception.
        PyErr_Fetch(&xtype, &xvalue, &xtb);

        // See if it is unacceptable.  An acceptable failure is a type error
        // with no traceback - so long as we can still reduce the number of
        // arguments and try again.
        if (!PyErr_GivenExceptionMatches(xtype, PyExc_TypeError) || xtb ||
            PyTuple_Size(sa) == 0)
        {
            // If there is a traceback then we must have called the slot and
            // the exception was later on - so report the exception as is.
            if (xtb)
            {
                if (sa != args)
                {
                    Py_XDECREF(oxtype);
                    Py_XDECREF(oxvalue);
                    Py_XDECREF(oxtb);
                }

                PyErr_Restore(xtype,xvalue,xtb);
            }
            else if (sa == args)
            {
                PyErr_Restore(xtype, xvalue, xtb);
            }
            else
            {
                // Discard the latest exception and restore the original one.
                Py_XDECREF(xtype);
                Py_XDECREF(xvalue);
                Py_XDECREF(xtb);

                PyErr_Restore(oxtype, oxvalue, oxtb);
            }

            break;
        }

        // If this is the first attempt, save the exception.
        if (sa == args)
        {
            oxtype = xtype;
            oxvalue = xvalue;
            oxtb = xtb;
        }
        else
        {
            Py_XDECREF(xtype);
            Py_XDECREF(xvalue);
            Py_XDECREF(xtb);
        }

        // Create the new argument tuple.
        if ((nsa = PyTuple_GetSlice(sa, 0, PyTuple_Size(sa) - 1)) == NULL)
        {
            // Tidy up.
            Py_XDECREF(oxtype);
            Py_XDECREF(oxvalue);
            Py_XDECREF(oxtb);

            break;
        }

        Py_DECREF(sa);
        sa = nsa;
    }

    Py_DECREF(sa);

    return 0;
}


// Clear the slot if it has an extra reference.
void PyQtSlot::clearOther()
{
    Py_CLEAR(other);
}


// Visit the slot if it has an extra reference.
int PyQtSlot::visitOther(visitproc visit, void *arg)
{
    Py_VISIT(other);

    return 0;
}

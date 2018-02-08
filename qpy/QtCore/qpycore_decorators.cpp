// This is the implementation of the pyqtSlot decorator.
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

#include <QByteArray>

#include "qpycore_api.h"
#include "qpycore_chimera.h"
#include "qpycore_misc.h"
#include "qpycore_objectified_strings.h"


// Forward declarations.
extern "C" {static PyObject *decorator(PyObject *self, PyObject *f);}


// This implements the pyqtSlot decorator.
PyObject *qpycore_pyqtslot(PyObject *args, PyObject *kwds)
{
    const char *name_str = 0;
    PyObject *res_obj = 0;
    int revision = 0;
    static const char *kwlist[] = {"name", "result", "revision", 0};

    static PyObject *no_args = 0;

    if (!no_args)
    {
        no_args = PyTuple_New(0);

        if (!no_args)
            return 0;
    }

    if (!PyArg_ParseTupleAndKeywords(no_args, kwds, "|sOi:pyqtSlot",
            const_cast<char **>(kwlist), &name_str, &res_obj, &revision))
        return 0;

    Chimera::Signature *parsed_sig = Chimera::parse(args, name_str,
            "a pyqtSlot type argument");

    if (!parsed_sig)
        return 0;

    // Sticking the revision here is an awful hack, but it saves creating
    // another data structure wrapped in a capsule.
    parsed_sig->revision = revision;

    // Parse any result type.
    if (res_obj)
    {
        parsed_sig->result = Chimera::parse(res_obj);

        if (!parsed_sig->result)
        {
            Chimera::raiseParseException(res_obj, "a pyqtSlot result");
            delete parsed_sig;
            return 0;
        }
    }

    // Wrap the parsed signature in a Python object.
    PyObject *sig_obj = Chimera::Signature::toPyObject(parsed_sig);

    if (!sig_obj)
        return 0;

    // Create the decorator function itself.  We stash the arguments in "self".
    // This may be an abuse, but it seems to be Ok.
    static PyMethodDef deco_method = {
#if PY_VERSION_HEX >= 0x02050000
        "_deco", decorator, METH_O, 0
#else
        const_cast<char *>("_deco"), decorator, METH_O, 0
#endif
    };

    PyObject *obj = PyCFunction_New(&deco_method, sig_obj);
    Py_DECREF(sig_obj);

    return obj;
}


// This is the decorator function that saves the C++ signature as a function
// attribute.
static PyObject *decorator(PyObject *self, PyObject *f)
{
    Chimera::Signature *parsed_sig = Chimera::Signature::fromPyObject(self);
    const QByteArray &sig = parsed_sig->signature;

    // Use the function name if there isn't already one.
    if (sig.startsWith('('))
    {
        // Get the function's name.
        PyObject *nobj = PyObject_GetAttr(f, qpycore_dunder_name);

        if (!nobj)
            return 0;

        PyObject *ascii_obj = nobj;
        const char *ascii = sipString_AsASCIIString(&ascii_obj);
        Py_DECREF(nobj);

        if (!ascii)
            return 0;

        parsed_sig->signature.prepend(ascii);
        parsed_sig->py_signature.prepend(ascii);
        Py_DECREF(ascii_obj);
    }

    // See if the function has already been decorated.
    PyObject *decorations = PyObject_GetAttr(f, qpycore_dunder_pyqtsignature);
    int rc;

    if (decorations)
    {
        // Insert the new decoration at the head of the existing ones so that
        // the list order matches the order they appear in the script.
        rc = PyList_Insert(decorations, 0, self);
    }
    else
    {
        PyErr_Clear();

        decorations = PyList_New(1);

        if (!decorations)
            return 0;

        Py_INCREF(self);
        PyList_SetItem(decorations, 0, self);

        // Save the new decoration.
        rc = PyObject_SetAttr(f, qpycore_dunder_pyqtsignature, decorations);
    }

    Py_DECREF(decorations);

    if (rc < 0)
        return 0;

    // Return the function.
    Py_INCREF(f);
    return f;
}

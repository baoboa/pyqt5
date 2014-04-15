// This is the implementation of pyqtProperty.
//
// Copyright (c) 2014 Riverbank Computing Limited <info@riverbankcomputing.com>
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
#include <structmember.h>

#include "qpycore_chimera.h"
#include "qpycore_pyqtproperty.h"
#include "qpycore_pyqtsignal.h"


// Forward declarations.
extern "C" {
static PyObject *pyqtProperty_getter(PyObject *self, PyObject *getter);
static PyObject *pyqtProperty_setter(PyObject *self, PyObject *setter);
static PyObject *pyqtProperty_deleter(PyObject *self, PyObject *deleter);
static PyObject *pyqtProperty_reset(PyObject *self, PyObject *reset);
static PyObject *pyqtProperty_descr_get(PyObject *self, PyObject *obj,
        PyObject *);
static int pyqtProperty_descr_set(PyObject *self, PyObject *obj,
        PyObject *value);
static PyObject *pyqtProperty_call(PyObject *self, PyObject *args,
        PyObject *kwds);
static void pyqtProperty_dealloc(PyObject *self);
static int pyqtProperty_init(PyObject *self, PyObject *args, PyObject *kwds);
static int pyqtProperty_traverse(PyObject *self, visitproc visit, void *arg);
}

static qpycore_pyqtProperty *clone(qpycore_pyqtProperty *orig);


// Doc-strings.
PyDoc_STRVAR(pyqtProperty_doc,
"pyqtProperty(type, fget=None, fset=None, freset=None, fdel=None, doc=None,\n"
"        designable=True, scriptable=True, stored=True, user=False,\n"
"        constant=False, final=False, notify=None,\n"
"        revision=0) -> property attribute\n"
"\n"
"type is the type of the property.  It is either a type object or a string\n"
"that is the name of a C++ type.\n"
"freset is a function for resetting an attribute to its default value.\n"
"designable sets the DESIGNABLE flag (the default is True for writable\n"
"properties and False otherwise).\n"
"scriptable sets the SCRIPTABLE flag.\n"
"stored sets the STORED flag.\n"
"user sets the USER flag.\n"
"constant sets the CONSTANT flag.\n"
"final sets the FINAL flag.\n"
"notify is the NOTIFY signal.\n"
"revision is the REVISION.\n"
"The other parameters are the same as those required by the standard Python\n"
"property type.  Properties defined using pyqtProperty behave as both Python\n"
"and Qt properties."
"\n"
"Decorators can be used to define new properties or to modify existing ones.");

PyDoc_STRVAR(getter_doc, "Descriptor to change the getter on a property.");
PyDoc_STRVAR(setter_doc, "Descriptor to change the setter on a property.");
PyDoc_STRVAR(deleter_doc, "Descriptor to change the deleter on a property.");
PyDoc_STRVAR(reset_doc, "Descriptor to change the reset on a property.");


// Define the attributes.
static PyMemberDef pyqtProperty_members[] = {
    {const_cast<char *>("type"), T_OBJECT,
            offsetof(qpycore_pyqtProperty, pyqtprop_type), READONLY, 0},
    {const_cast<char *>("fget"), T_OBJECT,
            offsetof(qpycore_pyqtProperty, pyqtprop_get), READONLY, 0},
    {const_cast<char *>("fset"), T_OBJECT,
            offsetof(qpycore_pyqtProperty, pyqtprop_set), READONLY, 0},
    {const_cast<char *>("fdel"), T_OBJECT,
            offsetof(qpycore_pyqtProperty, pyqtprop_del), READONLY, 0},
    {const_cast<char *>("freset"), T_OBJECT,
            offsetof(qpycore_pyqtProperty, pyqtprop_reset), READONLY, 0},
    {const_cast<char *>("__doc__"), T_OBJECT,
            offsetof(qpycore_pyqtProperty, pyqtprop_doc), READONLY, 0},
    {0, 0, 0, 0, 0}
};


// Define the methods.
static PyMethodDef pyqtProperty_methods[] = {
    {"getter", pyqtProperty_getter, METH_O, getter_doc},
    {"read", pyqtProperty_getter, METH_O, getter_doc},
    {"setter", pyqtProperty_setter, METH_O, setter_doc},
    {"write", pyqtProperty_setter, METH_O, setter_doc},
    {"deleter", pyqtProperty_deleter, METH_O, deleter_doc},
    {"reset", pyqtProperty_reset, METH_O, reset_doc},
    {0, 0, 0, 0}
};


// This implements the PyQt version of the standard Python property type.
PyTypeObject qpycore_pyqtProperty_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    SIP_TPNAME_CAST("PyQt5.QtCore.pyqtProperty"),
    sizeof (qpycore_pyqtProperty),
    0,
    pyqtProperty_dealloc,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    pyqtProperty_call,
    0,
    PyObject_GenericGetAttr,
    0,
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_GC|Py_TPFLAGS_BASETYPE,
    pyqtProperty_doc,
    pyqtProperty_traverse,
    0,
    0,
    0,
    0,
    0,
    pyqtProperty_methods,
    pyqtProperty_members,
    0,
    0,
    0,
    pyqtProperty_descr_get,
    pyqtProperty_descr_set,
    0,
    pyqtProperty_init,
    PyType_GenericAlloc,
    PyType_GenericNew,
    PyObject_GC_Del,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
#if PY_VERSION_HEX >= 0x03040000
    0,
#endif
};


// This is the sequence number to allocate to the next PyQt property to be
// defined.  This ensures that properties appear in the QMetaObject in the same
// order that they are defined in Python.
static uint pyqtprop_sequence_nr = 0;


// The pyqtProperty dealloc function.
static void pyqtProperty_dealloc(PyObject *self)
{
    qpycore_pyqtProperty *pp = (qpycore_pyqtProperty *)self;

    PyObject_GC_UnTrack(self);

    Py_XDECREF(pp->pyqtprop_get);
    Py_XDECREF(pp->pyqtprop_set);
    Py_XDECREF(pp->pyqtprop_del);
    Py_XDECREF(pp->pyqtprop_doc);
    Py_XDECREF(pp->pyqtprop_reset);
    Py_XDECREF(pp->pyqtprop_notify);
    Py_XDECREF(pp->pyqtprop_type);

    delete pp->pyqtprop_parsed_type;

    Py_TYPE(self)->tp_free(self);
}


// The descriptor getter.
static PyObject *pyqtProperty_descr_get(PyObject *self, PyObject *obj,
        PyObject *)
{
    qpycore_pyqtProperty *pp = (qpycore_pyqtProperty *)self;

    if (!obj || obj == Py_None)
    {
        Py_INCREF(self);
        return self;
    }

    if (!pp->pyqtprop_get)
    {
        PyErr_SetString(PyExc_AttributeError, "unreadable attribute");
        return 0;
    }

    return PyObject_CallFunction(pp->pyqtprop_get, const_cast<char *>("(O)"),
            obj);
}


// The descriptor setter.
static int pyqtProperty_descr_set(PyObject *self, PyObject *obj,
        PyObject *value)
{
    qpycore_pyqtProperty *pp = (qpycore_pyqtProperty *)self;
    PyObject *func, *res;

    func = (value ? pp->pyqtprop_set : pp->pyqtprop_del);

    if (!func)
    {
        PyErr_SetString(PyExc_AttributeError,
                (value ? "can't set attribute" : "can't delete attribute"));
        return -1;
    }

    if (value)
        res = PyObject_CallFunction(func, const_cast<char *>("(OO)"), obj,
                value);
    else
        res = PyObject_CallFunction(func, const_cast<char *>("(O)"), obj);

    if (!res)
        return -1;

    Py_DECREF(res);
    return 0;
}


// The pyqtProperty traverse function.
static int pyqtProperty_traverse(PyObject *self, visitproc visit, void *arg)
{
    qpycore_pyqtProperty *pp = (qpycore_pyqtProperty *)self;
    int vret;

    if (pp->pyqtprop_get)
    {
        vret = visit(pp->pyqtprop_get, arg);

        if (vret != 0)
            return vret;
    }

    if (pp->pyqtprop_set)
    {
        vret = visit(pp->pyqtprop_set, arg);

        if (vret != 0)
            return vret;
    }

    if (pp->pyqtprop_del)
    {
        vret = visit(pp->pyqtprop_del, arg);

        if (vret != 0)
            return vret;
    }

    if (pp->pyqtprop_doc)
    {
        vret = visit(pp->pyqtprop_doc, arg);

        if (vret != 0)
            return vret;
    }

    if (pp->pyqtprop_reset)
    {
        vret = visit(pp->pyqtprop_reset, arg);

        if (vret != 0)
            return vret;
    }

    if (pp->pyqtprop_notify)
    {
        vret = visit(pp->pyqtprop_notify, arg);

        if (vret != 0)
            return vret;
    }

    if (pp->pyqtprop_type)
    {
        vret = visit(pp->pyqtprop_type, arg);

        if (vret != 0)
            return vret;
    }

    return 0;
}


// The pyqtProperty init function.
static int pyqtProperty_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *type, *get = 0, *set = 0, *reset = 0, *del = 0, *doc = 0,
            *notify = 0;
    int scriptable = 1, stored = 1, user = 0, constant = 0, final = 0;
    int designable = 1, revision = 0;
    static const char *kwlist[] = {"type", "fget", "fset", "freset", "fdel",
            "doc", "designable", "scriptable", "stored", "user", "constant",
            "final", "notify", "revision", 0};
    qpycore_pyqtProperty *pp = (qpycore_pyqtProperty *)self;

    pp->pyqtprop_sequence = pyqtprop_sequence_nr++;

    if (!PyArg_ParseTupleAndKeywords(args, kwds,
            "O|OOOOOiiiiiiO!i:pyqtProperty",
            const_cast<char **>(kwlist), &type, &get, &set, &reset, &del, &doc,
            &designable, &scriptable, &stored, &user, &constant, &final,
            &qpycore_pyqtSignal_Type, &notify, &revision))
        return -1;

    if (get == Py_None)
        get = 0;

    if (set == Py_None)
        set = 0;

    if (del == Py_None)
        del = 0;

    if (reset == Py_None)
        reset = 0;

    if (notify == Py_None)
        notify = 0;

    // Parse the type.
    const Chimera *ptype = Chimera::parse(type);

    if (!ptype)
    {
        Chimera::raiseParseException(type, "a property");
        return -1;
    }

    pp->pyqtprop_parsed_type = ptype;

    Py_XINCREF(get);
    Py_XINCREF(set);
    Py_XINCREF(del);
    Py_XINCREF(doc);
    Py_XINCREF(reset);
    Py_XINCREF(notify);
    Py_INCREF(type);

    /* If no docstring was given and the getter has one, then use it. */
    if ((!doc || doc == Py_None) && get)
    {
        PyObject *get_doc = PyObject_GetAttrString(get, "__doc__");

        if (get_doc)
        {
            Py_XDECREF(doc);
            doc = get_doc;
        }
        else
        {
            PyErr_Clear();
        }
    }

    pp->pyqtprop_get = get;
    pp->pyqtprop_set = set;
    pp->pyqtprop_del = del;
    pp->pyqtprop_doc = doc;
    pp->pyqtprop_reset = reset;
    pp->pyqtprop_notify = notify;
    pp->pyqtprop_type = type;

    // ResolveEditable is always set.
    unsigned flags = 0x00080000;

    if (designable)
        flags |= 0x00001000;

    if (scriptable)
        flags |= 0x00004000;

    if (stored)
        flags |= 0x00010000;

    if (user)
        flags |= 0x00100000;

    if (constant)
        flags |= 0x00000400;

    if (final)
        flags |= 0x00000800;

    pp->pyqtprop_flags = flags;

    pp->pyqtprop_revision = revision;

    return 0;
}


// Calling the property is a shorthand way of changing the getter.
static PyObject *pyqtProperty_call(PyObject *self, PyObject *args,
        PyObject *kwds)
{
    PyObject *get;
    static const char *kwlist[] = {"fget", 0};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O:pyqtProperty",
            const_cast<char **>(kwlist), &get))
        return 0;

    return pyqtProperty_getter(self, get);
}


// Return a copy of the property with a different getter.
static PyObject *pyqtProperty_getter(PyObject *self, PyObject *getter)
{
    qpycore_pyqtProperty *pp = (qpycore_pyqtProperty *)self;

    pp = clone(pp);

    if (pp)
    {
        Py_XDECREF(pp->pyqtprop_get);

        if (getter == Py_None)
            getter = 0;
        else
            Py_INCREF(getter);

        pp->pyqtprop_get = getter;
    }

    return (PyObject *)pp;
}


// Return a copy of the property with a different setter.
static PyObject *pyqtProperty_setter(PyObject *self, PyObject *setter)
{
    qpycore_pyqtProperty *pp = (qpycore_pyqtProperty *)self;

    pp = clone(pp);

    if (pp)
    {
        Py_XDECREF(pp->pyqtprop_set);

        if (setter == Py_None)
            setter = 0;
        else
            Py_INCREF(setter);

        pp->pyqtprop_set = setter;
    }

    return (PyObject *)pp;
}


// Return a copy of the property with a different deleter.
static PyObject *pyqtProperty_deleter(PyObject *self, PyObject *deleter)
{
    qpycore_pyqtProperty *pp = (qpycore_pyqtProperty *)self;

    pp = clone(pp);

    if (pp)
    {
        Py_XDECREF(pp->pyqtprop_del);

        if (deleter == Py_None)
            deleter = 0;
        else
            Py_INCREF(deleter);

        pp->pyqtprop_del = deleter;
    }

    return (PyObject *)pp;
}


// Return a copy of the property with a different reset.
static PyObject *pyqtProperty_reset(PyObject *self, PyObject *reset)
{
    qpycore_pyqtProperty *pp = (qpycore_pyqtProperty *)self;

    pp = clone(pp);

    if (pp)
    {
        Py_XDECREF(pp->pyqtprop_reset);

        if (reset == Py_None)
            reset = 0;
        else
            Py_INCREF(reset);

        pp->pyqtprop_reset = reset;
    }

    return (PyObject *)pp;
}


// Create a clone of a property.
static qpycore_pyqtProperty *clone(qpycore_pyqtProperty *orig)
{
    qpycore_pyqtProperty *pp = (qpycore_pyqtProperty *)PyType_GenericNew(
            Py_TYPE(orig), 0, 0);

    if (pp)
    {
        pp->pyqtprop_get = orig->pyqtprop_get;
        Py_XINCREF(pp->pyqtprop_get);

        pp->pyqtprop_set = orig->pyqtprop_set;
        Py_XINCREF(pp->pyqtprop_set);

        pp->pyqtprop_del = orig->pyqtprop_del;
        Py_XINCREF(pp->pyqtprop_del);

        pp->pyqtprop_doc = orig->pyqtprop_doc;
        Py_XINCREF(pp->pyqtprop_doc);

        pp->pyqtprop_reset = orig->pyqtprop_reset;
        Py_XINCREF(pp->pyqtprop_reset);

        pp->pyqtprop_notify = orig->pyqtprop_notify;
        Py_XINCREF(pp->pyqtprop_notify);

        pp->pyqtprop_type = orig->pyqtprop_type;
        Py_XINCREF(pp->pyqtprop_type);

        pp->pyqtprop_parsed_type = new Chimera(*orig->pyqtprop_parsed_type);

        pp->pyqtprop_flags = orig->pyqtprop_flags;
        pp->pyqtprop_revision = orig->pyqtprop_revision;
        pp->pyqtprop_sequence = orig->pyqtprop_sequence;
    }

    return pp;
}

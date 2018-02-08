// This contains the support for converting QOpenGL arrays to Python objects.
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

#include "sipAPIQtGui.h"

#if defined(SIP_FEATURE_PyQt_OpenGL)

#include "qpyopengl_api.h"


#if QT_VERSION >= 0x050100

// Convert a GLint array to a Python object.
PyObject *qpyopengl_from_GLint(int *eflag, const GLint *array, Py_ssize_t len)
{
    if (len == 1)
        return SIPLong_FromLong(array[0]);

    PyObject *tuple = PyTuple_New(len);

    if (!tuple)
    {
        *eflag = 1;
        return 0;
    }

    for (Py_ssize_t i = 0; i < len; ++i)
    {
        PyObject *itm = SIPLong_FromLong(array[i]);

        if (!itm)
        {
            Py_DECREF(tuple);
            *eflag = 1;
            return 0;
        }

        PyTuple_SetItem(tuple, i, itm);
    }

    return tuple;
}


// Convert a GLuint array to a Python object.
PyObject *qpyopengl_from_GLuint(int *eflag, const GLuint *array,
        Py_ssize_t len)
{
    if (len == 1)
        return PyLong_FromUnsignedLong(array[0]);

    PyObject *tuple = PyTuple_New(len);

    if (!tuple)
    {
        *eflag = 1;
        return 0;
    }

    for (Py_ssize_t i = 0; i < len; ++i)
    {
        PyObject *itm = PyLong_FromUnsignedLong(array[i]);

        if (!itm)
        {
            Py_DECREF(tuple);
            *eflag = 1;
            return 0;
        }

        PyTuple_SetItem(tuple, i, itm);
    }

    return tuple;
}


// Convert a GLboolean array to a Python object.
PyObject *qpyopengl_from_GLboolean(int *eflag, const GLboolean *array,
        Py_ssize_t len)
{
    if (len == 1)
        return PyBool_FromLong(array[0]);

    PyObject *tuple = PyTuple_New(len);

    if (!tuple)
    {
        *eflag = 1;
        return 0;
    }

    for (Py_ssize_t i = 0; i < len; ++i)
    {
        PyObject *itm = PyBool_FromLong(array[i]);

        if (!itm)
        {
            Py_DECREF(tuple);
            *eflag = 1;
            return 0;
        }

        PyTuple_SetItem(tuple, i, itm);
    }

    return tuple;
}


// Convert a GLfloat array to a Python object.
PyObject *qpyopengl_from_GLfloat(int *eflag, const GLfloat *array,
        Py_ssize_t len)
{
    if (len == 1)
        return PyFloat_FromDouble(array[0]);

    PyObject *tuple = PyTuple_New(len);

    if (!tuple)
    {
        *eflag = 1;
        return 0;
    }

    for (Py_ssize_t i = 0; i < len; ++i)
    {
        PyObject *itm = PyFloat_FromDouble(array[i]);

        if (!itm)
        {
            Py_DECREF(tuple);
            *eflag = 1;
            return 0;
        }

        PyTuple_SetItem(tuple, i, itm);
    }

    return tuple;
}


#if defined(SIP_FEATURE_PyQt_Desktop_OpenGL)
// Convert a GLdouble array to a Python object.
PyObject *qpyopengl_from_GLdouble(int *eflag, const GLdouble *array,
        Py_ssize_t len)
{
    if (len == 1)
        return PyFloat_FromDouble(array[0]);

    PyObject *tuple = PyTuple_New(len);

    if (!tuple)
    {
        *eflag = 1;
        return 0;
    }

    for (Py_ssize_t i = 0; i < len; ++i)
    {
        PyObject *itm = PyFloat_FromDouble(array[i]);

        if (!itm)
        {
            Py_DECREF(tuple);
            *eflag = 1;
            return 0;
        }

        PyTuple_SetItem(tuple, i, itm);
    }

    return tuple;
}
#endif

#endif


#endif

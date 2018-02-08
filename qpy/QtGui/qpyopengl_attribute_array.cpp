// This contains the support for shader program attribute arrays.
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

#include <QOpenGLShaderProgram>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

#include "qpyopengl_api.h"
#include "qpyopengl_misc.h"


// Forward declaratations.
#if defined(SIP_USE_PYCAPSULE)
static void array_dtor(PyObject *capsule);
#else
static void array_dtor(void *array);
#endif


// Parse a Python object as a sequence of either QVector[234]D instances or a
// sequence of sequence of floats and return an array that can be passed to
// QGLShaderProgram::setAttributeArray().  The array is destroyed only when the
// shader is garbage collected or when replaced by another array.
const GLfloat *qpyopengl_attribute_array(PyObject *values, PyObject *shader,
        PyObject *key, int *tsize, sipErrorState *estate)
{
    // Check the key was created correctly.
    if (!key)
    {
        *estate = sipErrorFail;
        return 0;
    }

    // Get the dict that holds the converted arrays.
    PyObject *dict = sipGetUserObject((sipSimpleWrapper *)shader);

    if (!dict)
    {
        dict = PyDict_New();

        if (!dict)
        {
            Py_DECREF(key);

            *estate = sipErrorFail;
            return 0;
        }

        sipSetUserObject((sipSimpleWrapper *)shader, dict);
    }

    // Check that values is a non-empty sequence.
    values = PySequence_Fast(values, "an attribute array must be a sequence");

    if (!values)
    {
        Py_DECREF(key);

        *estate = sipErrorContinue;
        return 0;
    }

    Py_ssize_t nr_items = Sequence_Fast_Size(values);

    if (nr_items < 1)
    {
        PyErr_SetString(PyExc_TypeError,
                "an attribute array must have at least one element");

        Py_DECREF(key);
        Py_DECREF(values);

        *estate = sipErrorFail;
        return 0;
    }

    // The first element determines the type expected.
    PyObject *itm = Sequence_Fast_GetItem(values, 0);

    const sipTypeDef *td;
    Py_ssize_t nr_dim;

    if (sipCanConvertToType(itm, sipType_QVector2D, SIP_NOT_NONE))
    {
        td = sipType_QVector2D;
        nr_dim = 2;
    }
    else if (sipCanConvertToType(itm, sipType_QVector3D, SIP_NOT_NONE))
    {
        td = sipType_QVector3D;
        nr_dim = 3;
    }
    else if (sipCanConvertToType(itm, sipType_QVector4D, SIP_NOT_NONE))
    {
        td = sipType_QVector4D;
        nr_dim = 4;
    }
    else if (PySequence_Check(itm) && (nr_dim = PySequence_Size(itm)) >= 1)
    {
        td = 0;
    }
    else
    {
        PyErr_SetString(PyExc_TypeError,
                "an attribute array must be a sequence of QVector2D, "
                "QVector3D, QVector4D, or a sequence of sequences of floats");

        Py_DECREF(key);
        Py_DECREF(values);

        *estate = sipErrorFail;
        return 0;
    }

    // Create the array that will be returned.
    GLfloat *array = new GLfloat[nr_items * nr_dim];

    // Convert the values.
    GLfloat *ap = array;

    for (Py_ssize_t i = 0; i < nr_items; ++i)
    {
        int iserr = 0;

        itm = Sequence_Fast_GetItem(values, i);

        if (td)
        {
            void *cpp;

            cpp = sipForceConvertToType(itm, td, 0,
                    SIP_NOT_NONE | SIP_NO_CONVERTORS, 0, &iserr);

            if (iserr)
            {
                PyErr_Format(PyExc_TypeError,
                        "attribute array elements should all be '%s', not '%s'",
                        sipPyTypeName(sipTypeAsPyTypeObject(td)),
                        sipPyTypeName(Py_TYPE(itm)));
            }
            else if (td == sipType_QVector2D)
            {
                QVector2D *v = reinterpret_cast<QVector2D *>(cpp);

                *ap++ = v->x();
                *ap++ = v->y();
            }
            else if (td == sipType_QVector3D)
            {
                QVector3D *v = reinterpret_cast<QVector3D *>(cpp);

                *ap++ = v->x();
                *ap++ = v->y();
                *ap++ = v->z();
            }
            else if (td == sipType_QVector4D)
            {
                QVector4D *v = reinterpret_cast<QVector4D *>(cpp);

                *ap++ = v->x();
                *ap++ = v->y();
                *ap++ = v->z();
                *ap++ = v->w();
            }
        }
        else
        {
            itm = PySequence_Fast(itm,
                    "attribute array elements should all be sequences");

            if (itm)
            {
                if (Sequence_Fast_Size(itm) != nr_dim)
                {
                    PyErr_Format(PyExc_TypeError,
                            "attribute array elements should all be sequences "
#if PY_VERSION_HEX >= 0x02050000
                            "of length %zd",
#else
                            "of length %d",
#endif
                            nr_dim);

                    Py_DECREF(itm);
                    iserr = 1;
                }
                else
                {
                    PyErr_Clear();

                    for (Py_ssize_t j = 0; j < nr_dim; ++j)
                        *ap++ = PyFloat_AsDouble(Sequence_Fast_GetItem(itm, j));

                    if (PyErr_Occurred())
                    {
                        PyErr_SetString(PyExc_TypeError,
                                "attribute array elements should all be "
                                "sequences of floats");

                        Py_DECREF(itm);
                        iserr = 1;
                    }
                }
            }
            else
            {
                iserr = 1;
            }
        }

        if (iserr)
        {
            Py_DECREF(key);
            Py_DECREF(values);
            delete[] array;

            *estate = sipErrorFail;
            return 0;
        }
    }

    Py_DECREF(values);

    *tsize = nr_dim;

    // Wrap the array in a Python object so that it won't leak.
#if defined(SIP_USE_PYCAPSULE)
    PyObject *array_obj = PyCapsule_New(array, 0, array_dtor);
#else
    PyObject *array_obj = PyCObject_FromVoidPtr(array, array_dtor);
#endif

    if (!array_obj)
    {
        Py_DECREF(key);
        delete[] array;

        *estate = sipErrorFail;
        return 0;
    }

    int rc = PyDict_SetItem(dict, key, array_obj);

    Py_DECREF(key);
    Py_DECREF(array_obj);

    if (rc < 0)
    {
        *estate = sipErrorFail;
        return 0;
    }

    return array;
}


// The dtor for the saved attribute array.
#if defined(SIP_USE_PYCAPSULE)
static void array_dtor(PyObject *capsule)
{
    delete[] reinterpret_cast<GLfloat *>(PyCapsule_GetPointer(capsule, 0));
}
#else
static void array_dtor(void *array)
{
    delete[] reinterpret_cast<GLfloat *>(array);
}
#endif


#endif

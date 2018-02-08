// This contains the support for shader program uniform value arrays.
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
#include <QMatrix2x2>
#include <QMatrix2x3>
#include <QMatrix2x4>
#include <QMatrix3x2>
#include <QMatrix3x3>
#include <QMatrix3x4>
#include <QMatrix4x2>
#include <QMatrix4x3>
#include <QMatrix4x4>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

#include "qpyopengl_api.h"
#include "qpyopengl_misc.h"


// Forward declaratations.
static void delete_array(void *array, const sipTypeDef *td);
#if defined(SIP_USE_PYCAPSULE)
static void array_dtor(PyObject *capsule);
#else
static void array_dtor(void *array, void *td);
#endif


// Parse a Python object as a sequence of either QVector[234]D or
// QMatrix[234]x[234] instances, or a sequence of sequence of floats and return
// an array that can be passed to QOpenGLShaderProgram::setUniformValueArray().
// The array is destroyed only when the shader is garbage collected or when
// replaced by another array.
const void *qpyopengl_uniform_value_array(PyObject *values, PyObject *shader,
        PyObject *key, const sipTypeDef **array_type, int *array_len,
        int *tsize, sipErrorState *estate)
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
    values = PySequence_Fast(values,
            "a uniform value array must be a sequence");

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
                "a uniform value array must have at least one element");

        Py_DECREF(key);
        Py_DECREF(values);

        *estate = sipErrorFail;
        return 0;
    }

    // The first element determines the type expected.
    PyObject *itm = Sequence_Fast_GetItem(values, 0);

    const sipTypeDef *td;
    Py_ssize_t nr_dim = 0;
    void *array;

    if (sipCanConvertToType(itm, sipType_QVector2D, SIP_NOT_NONE))
    {
        td = sipType_QVector2D;
        array = new QVector2D[nr_items];
    }
    else if (sipCanConvertToType(itm, sipType_QVector3D, SIP_NOT_NONE))
    {
        td = sipType_QVector3D;
        array = new QVector3D[nr_items];
    }
    else if (sipCanConvertToType(itm, sipType_QVector4D, SIP_NOT_NONE))
    {
        td = sipType_QVector4D;
        array = new QVector4D[nr_items];
    }
    else if (sipCanConvertToType(itm, sipType_QMatrix2x2, SIP_NOT_NONE))
    {
        td = sipType_QMatrix2x2;
        array = new QMatrix2x2[nr_items];
    }
    else if (sipCanConvertToType(itm, sipType_QMatrix2x3, SIP_NOT_NONE))
    {
        td = sipType_QMatrix2x3;
        array = new QMatrix2x3[nr_items];
    }
    else if (sipCanConvertToType(itm, sipType_QMatrix2x4, SIP_NOT_NONE))
    {
        td = sipType_QMatrix2x4;
        array = new QMatrix2x4[nr_items];
    }
    else if (sipCanConvertToType(itm, sipType_QMatrix3x2, SIP_NOT_NONE))
    {
        td = sipType_QMatrix3x2;
        array = new QMatrix3x2[nr_items];
    }
    else if (sipCanConvertToType(itm, sipType_QMatrix3x3, SIP_NOT_NONE))
    {
        td = sipType_QMatrix3x3;
        array = new QMatrix3x3[nr_items];
    }
    else if (sipCanConvertToType(itm, sipType_QMatrix3x4, SIP_NOT_NONE))
    {
        td = sipType_QMatrix3x4;
        array = new QMatrix3x4[nr_items];
    }
    else if (sipCanConvertToType(itm, sipType_QMatrix4x2, SIP_NOT_NONE))
    {
        td = sipType_QMatrix4x2;
        array = new QMatrix4x2[nr_items];
    }
    else if (sipCanConvertToType(itm, sipType_QMatrix4x3, SIP_NOT_NONE))
    {
        td = sipType_QMatrix4x3;
        array = new QMatrix4x3[nr_items];
    }
    else if (sipCanConvertToType(itm, sipType_QMatrix4x4, SIP_NOT_NONE))
    {
        td = sipType_QMatrix4x4;
        array = new QMatrix4x4[nr_items];
    }
    else if (PySequence_Check(itm) && (nr_dim = PySequence_Size(itm)) >= 1)
    {
        td = 0;
        array = new GLfloat[nr_items * nr_dim];
    }
    else
    {
        PyErr_SetString(PyExc_TypeError,
                "a uniform value array must be a sequence of QVector2D, "
                "QVector3D, QVector4D, QMatrix2x2, QMatrix2x3, QMatrix2x4, "
                "QMatrix3x2, QMatrix3x3, QMatrix3x4, QMatrix4x2, QMatrix4x3, "
                "QMatrix4x4, or a sequence of sequences of floats");

        Py_DECREF(key);
        Py_DECREF(values);

        *estate = sipErrorFail;
        return 0;
    }

    // Convert the values.
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
                        "uniform value array elements should all be '%s', not "
                        "'%s'",
                        sipPyTypeName(sipTypeAsPyTypeObject(td)),
                        sipPyTypeName(Py_TYPE(itm)));
            }
            else if (td == sipType_QVector2D)
            {
                QVector2D *a = reinterpret_cast<QVector2D *>(array);

                a[i] = *reinterpret_cast<QVector2D *>(cpp);
            }
            else if (td == sipType_QVector3D)
            {
                QVector3D *a = reinterpret_cast<QVector3D *>(array);

                a[i] = *reinterpret_cast<QVector3D *>(cpp);
            }
            else if (td == sipType_QVector4D)
            {
                QVector4D *a = reinterpret_cast<QVector4D *>(array);

                a[i] = *reinterpret_cast<QVector4D *>(cpp);
            }
            else if (td == sipType_QMatrix2x2)
            {
                QMatrix2x2 *a = reinterpret_cast<QMatrix2x2 *>(array);

                a[i] = *reinterpret_cast<QMatrix2x2 *>(cpp);
            }
            else if (td == sipType_QMatrix2x3)
            {
                QMatrix2x3 *a = reinterpret_cast<QMatrix2x3 *>(array);

                a[i] = *reinterpret_cast<QMatrix2x3 *>(cpp);
            }
            else if (td == sipType_QMatrix2x4)
            {
                QMatrix2x4 *a = reinterpret_cast<QMatrix2x4 *>(array);

                a[i] = *reinterpret_cast<QMatrix2x4 *>(cpp);
            }
            else if (td == sipType_QMatrix3x2)
            {
                QMatrix3x2 *a = reinterpret_cast<QMatrix3x2 *>(array);

                a[i] = *reinterpret_cast<QMatrix3x2 *>(cpp);
            }
            else if (td == sipType_QMatrix3x3)
            {
                QMatrix3x3 *a = reinterpret_cast<QMatrix3x3 *>(array);

                a[i] = *reinterpret_cast<QMatrix3x3 *>(cpp);
            }
            else if (td == sipType_QMatrix3x4)
            {
                QMatrix3x4 *a = reinterpret_cast<QMatrix3x4 *>(array);

                a[i] = *reinterpret_cast<QMatrix3x4 *>(cpp);
            }
            else if (td == sipType_QMatrix4x2)
            {
                QMatrix4x2 *a = reinterpret_cast<QMatrix4x2 *>(array);

                a[i] = *reinterpret_cast<QMatrix4x2 *>(cpp);
            }
            else if (td == sipType_QMatrix4x3)
            {
                QMatrix4x3 *a = reinterpret_cast<QMatrix4x3 *>(array);

                a[i] = *reinterpret_cast<QMatrix4x3 *>(cpp);
            }
            else if (td == sipType_QMatrix4x4)
            {
                QMatrix4x4 *a = reinterpret_cast<QMatrix4x4 *>(array);

                a[i] = *reinterpret_cast<QMatrix4x4 *>(cpp);
            }
        }
        else
        {
            itm = PySequence_Fast(itm,
                    "uniform value array elements should all be sequences");

            if (itm)
            {
                if (Sequence_Fast_Size(itm) != nr_dim)
                {
                    PyErr_Format(PyExc_TypeError,
                            "uniform value array elements should all be "
                            "sequences of length "
#if PY_VERSION_HEX >= 0x02050000
                            "%zd",
#else
                            "%d",
#endif
                            nr_dim);

                    Py_DECREF(itm);
                    iserr = 1;
                }
                else
                {
                    GLfloat *ap = reinterpret_cast<GLfloat *>(array);

                    PyErr_Clear();

                    for (Py_ssize_t j = 0; j < nr_dim; ++j)
                        *ap++ = PyFloat_AsDouble(
                                Sequence_Fast_GetItem(itm, j));

                    if (PyErr_Occurred())
                    {
                        PyErr_SetString(PyExc_TypeError,
                                "uniform value array elements should all be "
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
            delete_array(array, td);

            *estate = sipErrorFail;
            return 0;
        }
    }

    Py_DECREF(values);

    // Wrap the array in a Python object so that it won't leak.
#if defined(SIP_USE_PYCAPSULE)
    PyObject *array_obj = PyCapsule_New(array, 0, array_dtor);

    if (array_obj && PyCapsule_SetContext(array_obj, const_cast<sipTypeDef *>(td)) != 0)
    {
        Py_DECREF(array_obj);
        array_obj = 0;
    }
#else
    PyObject *array_obj = PyCObject_FromVoidPtrAndDesc(array, const_cast<sipTypeDef *>(td), array_dtor);
#endif

    if (!array_obj)
    {
        Py_DECREF(key);
        delete_array(array, td);

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

    *array_type = td;
    *array_len = nr_items;
    *tsize = nr_dim;

    return array;
}


// Do the actual work of deleting the array.
static void delete_array(void *array, const sipTypeDef *td)
{
    if (td == sipType_QVector2D)
        delete[] reinterpret_cast<QVector2D *>(array);
    else if (td == sipType_QVector3D)
        delete[] reinterpret_cast<QVector3D *>(array);
    else if (td == sipType_QVector4D)
        delete[] reinterpret_cast<QVector4D *>(array);
    else if (td == sipType_QMatrix2x2)
        delete[] reinterpret_cast<QMatrix2x2 *>(array);
    else if (td == sipType_QMatrix2x3)
        delete[] reinterpret_cast<QMatrix2x3 *>(array);
    else if (td == sipType_QMatrix2x4)
        delete[] reinterpret_cast<QMatrix2x4 *>(array);
    else if (td == sipType_QMatrix3x2)
        delete[] reinterpret_cast<QMatrix3x2 *>(array);
    else if (td == sipType_QMatrix3x3)
        delete[] reinterpret_cast<QMatrix3x3 *>(array);
    else if (td == sipType_QMatrix3x4)
        delete[] reinterpret_cast<QMatrix3x4 *>(array);
    else if (td == sipType_QMatrix4x2)
        delete[] reinterpret_cast<QMatrix4x2 *>(array);
    else if (td == sipType_QMatrix4x3)
        delete[] reinterpret_cast<QMatrix4x3 *>(array);
    else if (td == sipType_QMatrix4x4)
        delete[] reinterpret_cast<QMatrix4x4 *>(array);
    else
        delete[] reinterpret_cast<GLfloat *>(array);
}


// The dtor for the saved array.
#if defined(SIP_USE_PYCAPSULE)
static void array_dtor(PyObject *capsule)
{
    delete_array(PyCapsule_GetPointer(capsule, 0),
            reinterpret_cast<const sipTypeDef *>(
                    PyCapsule_GetContext(capsule)));
}
#else
static void array_dtor(void *array, void *td)
{
    delete_array(array, reinterpret_cast<const sipTypeDef *>(td));
}
#endif


#endif

// This contains the support for QOpenGL value arrays.
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

#include <QtGlobal>

#if QT_VERSION >= 0x050100

#include "qpyopengl_api.h"
#include "qpyopengl_data_cache.h"
#include "qpyopengl_misc.h"


// Forward declarations.
static qpyopengl_dataCache *get_cache(PyObject *bindings);
static void *convert_values(Array *cache, PyObject *values,
        GLenum gl_type, sipErrorState *estate);
static void convert_byte(PyObject *itm, void *array, Py_ssize_t i);
static void convert_ubyte(PyObject *itm, void *array, Py_ssize_t i);
static void convert_short(PyObject *itm, void *array, Py_ssize_t i);
static void convert_ushort(PyObject *itm, void *array, Py_ssize_t i);
static void convert_int(PyObject *itm, void *array, Py_ssize_t i);
static void convert_uint(PyObject *itm, void *array, Py_ssize_t i);
static void convert_float(PyObject *itm, void *array, Py_ssize_t i);
#if defined(SIP_FEATURE_PyQt_Desktop_OpenGL)
static void convert_double(PyObject *itm, void *array, Py_ssize_t i);
#endif


// Get an array of OpenGL fundamental types from a sequence or an object that
// implements a compatible buffer.
const GLvoid *qpyopengl_value_array(sipErrorState *estate, PyObject *values,
        GLenum gl_type, PyObject *bindings)
{
    // Handle the trivial None case first.  0 has a special meaning to some
    // OpenGL calls so we allow it for all.  If this becomes a problem then we
    // can add a new variant of this function that handles None differently.
    if (values == Py_None)
        return 0;

    qpyopengl_dataCache *data_cache = get_cache(bindings);

    if (!data_cache)
    {
        *estate = sipErrorFail;
        return 0;
    }

    // Get an empty wrapper for the array.
    if (data_cache->uncached)
        data_cache->uncached->clear();
    else
        data_cache->uncached = new Array;

    return convert_values(data_cache->uncached, values, gl_type, estate);
}


// Get an array of OpenGL fundamental types from a sequence or an object that
// implements a compatible buffer.  Cache the array so that it persists until a
// similar call.
const GLvoid *qpyopengl_value_array_cached(sipErrorState *estate,
        PyObject *values, GLenum gl_type, PyObject *bindings, const char *pkey,
        GLuint skey)
{
    // Handle the trivial byte offset case first.
    PyErr_Clear();

    void *data = PyLong_AsVoidPtr(values);

    if (!PyErr_Occurred())
        return data;

    PyErr_Clear();

    qpyopengl_dataCache *data_cache = get_cache(bindings);

    if (!data_cache)
    {
        *estate = sipErrorFail;
        return 0;
    }

    if (!data_cache->pcache)
        data_cache->pcache = new PrimaryCache;

    // Get an empty wrapper for the array.
    PrimaryCacheEntry *pce = (*data_cache->pcache)[pkey];

    if (!pce)
    {
        pce = new PrimaryCacheEntry;
        data_cache->pcache->insert(pkey, pce);
    }

    Array *array;

    if (skey == 0)
    {
        array = &pce->skey_0;
    }
    else
    {
        if (!pce->skey_n)
            pce->skey_n = new SecondaryCache;

        array = (*pce->skey_n)[skey];

        if (!array)
        {
            array = new Array;
            pce->skey_n->insert(skey, array);
        }
    }

    array->clear();

    return convert_values(array, values, gl_type, estate);
}


// Get the cache for a set of bindings.
static qpyopengl_dataCache *get_cache(PyObject *bindings)
{
    // Create the cache if it doesn't already exist.
    qpyopengl_dataCache *data_cache = (qpyopengl_dataCache *)sipGetUserObject((sipSimpleWrapper *)bindings);

    if (!data_cache)
    {
        data_cache = qpyopengl_dataCache_New();

        if (data_cache)
            sipSetUserObject((sipSimpleWrapper *)bindings,
                    (PyObject *)data_cache);
    }

    return data_cache;
}


// Convert the Python values to a raw array.
static void *convert_values(Array *array, PyObject *values, GLenum gl_type,
        sipErrorState *estate)
{
#if PY_VERSION_HEX >= 0x02060300
    int rc = sipGetBufferInfo(values, &array->buffer);

    if (rc < 0)
    {
        *estate = sipErrorFail;
        return 0;
    }

    if (rc > 0)
    {
        // Check the buffer is compatible with what we need.
        GLenum array_type;

        switch (*array->buffer.bi_format)
        {
        case 'b':
            array_type = GL_BYTE;
            break;

        case 'B':
            array_type = GL_UNSIGNED_BYTE;
            break;

        case 'h':
            array_type = GL_SHORT;
            break;

        case 'H':
            array_type = GL_UNSIGNED_SHORT;
            break;

        case 'i':
            array_type = GL_INT;
            break;

        case 'I':
            array_type = GL_UNSIGNED_INT;
            break;

        case 'f':
            array_type = GL_FLOAT;
            break;

#if defined(SIP_FEATURE_PyQt_Desktop_OpenGL)
        case 'd':
            array_type = GL_DOUBLE;
            break;
#endif

        default:
            PyErr_Format(PyExc_TypeError, "unsupported buffer type '%s'",
                    array->buffer.bi_format);
            *estate = sipErrorFail;
            return 0;
        }

        if (array_type != gl_type)
        {
            PyErr_SetString(PyExc_TypeError,
                    "the buffer type is not the same as the array type");
            *estate = sipErrorFail;
            return 0;
        }

        return array->buffer.bi_buf;
    }
#else
    PyBufferProcs *bf = Py_TYPE(values)->tp_as_buffer;

    if (bf && bf->bf_getreadbuffer && bf->bf_getsegcount)
    {
        if (bf->bf_getsegcount(values, 0) != 1)
        {
            PyErr_SetString(PyExc_TypeError,
                    "single-segment buffer object expected");
            *estate = sipErrorFail;
            return 0;
        }

        if (bf->bf_getreadbuffer(values, 0, reinterpret_cast<void **>(&array)) < 0)
        {
            *estate = sipErrorFail;
            return 0;
        }

        Py_INCREF(values);
        array->buffer = values;

        return array;
    }
#endif

    PyObject *seq = PySequence_Fast(values,
            "array must be a sequence or a buffer");

    if (!seq)
    {
        *estate = sipErrorContinue;
        return 0;
    }

    Py_ssize_t nr_items = Sequence_Fast_Size(seq);

    if (nr_items < 1)
    {
        Py_DECREF(seq);

        PyErr_SetString(PyExc_TypeError,
                "array must have at least one element");
        *estate = sipErrorFail;
        return 0;
    }

    void (*convertor)(PyObject *, void *, Py_ssize_t);
    size_t element_size;

    switch (gl_type)
    {
    case GL_BYTE:
        convertor = convert_byte;
        element_size = sizeof (GLbyte);
        break;

    case GL_UNSIGNED_BYTE:
        convertor = convert_ubyte;
        element_size = sizeof (GLubyte);
        break;

    case GL_SHORT:
        convertor = convert_short;
        element_size = sizeof (GLshort);
        break;

    case GL_UNSIGNED_SHORT:
        convertor = convert_ushort;
        element_size = sizeof (GLushort);
        break;

    case GL_INT:
        convertor = convert_int;
        element_size = sizeof (GLint);
        break;

    case GL_UNSIGNED_INT:
        convertor = convert_uint;
        element_size = sizeof (GLuint);
        break;

    case GL_FLOAT:
        convertor = convert_float;
        element_size = sizeof (GLfloat);
        break;

#if defined(SIP_FEATURE_PyQt_Desktop_OpenGL)
#if GL_DOUBLE != GL_FLOAT
    case GL_DOUBLE:
        convertor = convert_double;
        element_size = sizeof (GLdouble);
        break;
#endif
#endif

    default:
        Py_DECREF(seq);

        PyErr_SetString(PyExc_TypeError, "unsupported GL element type");
        *estate = sipErrorFail;
        return 0;
    }

    void *data = sipMalloc(nr_items * element_size);

    if (!data)
    {
        Py_DECREF(seq);

        *estate = sipErrorFail;
        return 0;
    }

    for (Py_ssize_t i = 0; i < nr_items; ++i)
    {
        PyErr_Clear();

        convertor(Sequence_Fast_GetItem(seq, i), data, i);

        if (PyErr_Occurred())
        {
            sipFree(data);
            Py_DECREF(seq);

            *estate = sipErrorFail;
            return 0;
        }
    }

    Py_DECREF(seq);

    array->data = data;

    return data;
}


// Convert a Python object to a GLbyte.
static void convert_byte(PyObject *itm, void *array, Py_ssize_t i)
{
    reinterpret_cast<GLbyte *>(array)[i] = sipLong_AsSignedChar(itm);
}


// Convert a Python object to a GLubyte.
static void convert_ubyte(PyObject *itm, void *array, Py_ssize_t i)
{
    reinterpret_cast<GLubyte *>(array)[i] = sipLong_AsUnsignedChar(itm);
}


// Convert a Python object to a GLshort.
static void convert_short(PyObject *itm, void *array, Py_ssize_t i)
{
    reinterpret_cast<GLshort *>(array)[i] = sipLong_AsShort(itm);
}


// Convert a Python object to a GLushort.
static void convert_ushort(PyObject *itm, void *array, Py_ssize_t i)
{
    reinterpret_cast<GLushort *>(array)[i] = sipLong_AsUnsignedShort(itm);
}


// Convert a Python object to a GLint.
static void convert_int(PyObject *itm, void *array, Py_ssize_t i)
{
    reinterpret_cast<GLint *>(array)[i] = sipLong_AsInt(itm);
}


// Convert a Python object to a GLuint.
static void convert_uint(PyObject *itm, void *array, Py_ssize_t i)
{
    reinterpret_cast<GLuint *>(array)[i] = sipLong_AsUnsignedInt(itm);
}


// Convert a Python object to a GLfloat.
static void convert_float(PyObject *itm, void *array, Py_ssize_t i)
{
    reinterpret_cast<GLfloat *>(array)[i] = PyFloat_AsDouble(itm);
}


#if defined(SIP_FEATURE_PyQt_Desktop_OpenGL)
// Convert a Python object to a GLdouble.
static void convert_double(PyObject *itm, void *array, Py_ssize_t i)
{
    reinterpret_cast<GLdouble *>(array)[i] = PyFloat_AsDouble(itm);
}
#endif

#endif


#endif

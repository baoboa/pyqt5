// This defines the OpenGL related API provided by this library.
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


#ifndef _QPYOPENGL_API_H
#define _QPYOPENGL_API_H


#include <Python.h>

#include "sipAPIQtGui.h"

#if defined(SIP_FEATURE_PyQt_OpenGL)

#include <QOpenGLContext>

#if QT_VERSION >= 0x050100
#include <QOpenGLVersionProfile>
#endif


// Initialisation.
void qpyopengl_init();

// Support for shader arrays.
const GLfloat *qpyopengl_attribute_array(PyObject *values, PyObject *shader,
        PyObject *key, int *tsize, sipErrorState *estate);
const void *qpyopengl_uniform_value_array(PyObject *values, PyObject *shader,
        PyObject *key, const sipTypeDef **array_type, int *array_len,
        int *tsize, sipErrorState *estate);

#if QT_VERSION >= 0x050100
// Support for QOpenGLContext.versionFunctions().
PyObject *qpyopengl_version_functions(const QOpenGLContext *context,
        PyObject *py_context, const QOpenGLVersionProfile *version_profile);
#endif

#if QT_VERSION >= 0x050100
// Support for the OpenGL bindings.
const GLvoid *qpyopengl_value_array(sipErrorState *estate, PyObject *values,
        GLenum gl_type, PyObject *bindings);
const GLvoid *qpyopengl_value_array_cached(sipErrorState *estate,
        PyObject *values, GLenum gl_type, PyObject *bindings, const char *pkey,
        GLuint skey);
GLint qpyopengl_get(GLenum pname, GLenum *query);
PyObject *qpyopengl_from_GLint(int *eflag, const GLint *array, Py_ssize_t len);
PyObject *qpyopengl_from_GLuint(int *eflag, const GLuint *array,
        Py_ssize_t len);
PyObject *qpyopengl_from_GLboolean(int *eflag, const GLboolean *array,
        Py_ssize_t len);
PyObject *qpyopengl_from_GLfloat(int *eflag, const GLfloat *array,
        Py_ssize_t len);
#if defined(SIP_FEATURE_PyQt_Desktop_OpenGL)
PyObject *qpyopengl_from_GLdouble(int *eflag, const GLdouble *array,
        Py_ssize_t len);
#endif
#endif


#endif


#endif

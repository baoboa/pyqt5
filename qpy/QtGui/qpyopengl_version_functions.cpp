// This contains the support for QOpenGLContext.versionFunctions().
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

#include <QList>

#include "qpyopengl_api.h"


// Forward declarations.
int qpyopengl_add_constants(PyObject *obj);


// The cache of type objects corresponding to each set of functions.
static QList<PyTypeObject *> funcs_types;


PyObject *qpyopengl_version_functions(const QOpenGLContext *context,
        PyObject *py_context, const QOpenGLVersionProfile *version_profile)
{
    // Get a valid version profile.
    QOpenGLVersionProfile vp;

    if (version_profile && version_profile->isValid())
        vp = *version_profile;
    else
        vp = QOpenGLVersionProfile(context->format());

    // Create the functions.
    QAbstractOpenGLFunctions *funcs = context->versionFunctions(vp);

    if (!funcs)
    {
        Py_INCREF(Py_None);
        return Py_None;
    }

    // Qt doesn't allow us to introspect the functions to find which set we
    // have, so work out what it should be based on the version profile.
#if defined(QT_OPENGL_ES_2)
    QByteArray name("QOpenGLFunctions_ES2");
#else
    QByteArray name("QOpenGLFunctions_");

    QPair<int, int> version = vp.version();
    name.append(QByteArray::number(version.first));
    name.append('_');
    name.append(QByteArray::number(version.second));

    if (vp.hasProfiles())
    {
        switch (vp.profile())
        {
        case QSurfaceFormat::CoreProfile:
            name.append("_Core");
            break;

        case QSurfaceFormat::CompatibilityProfile:
            name.append("_Compatibility");
            break;

        default:
            ;
        }
    }
#endif

    // See if we already have the type object.
    PyTypeObject *funcs_type = 0;

    for (int i = 0; i < funcs_types.count(); ++i)
    {
        PyTypeObject *ft = funcs_types.at(i);

        if (name == sipPyTypeName(ft))
        {
            funcs_type = ft;
            break;
        }
    }

    if (!funcs_type)
    {
        // Get the type object.
        QByteArray module_name("PyQt5._");
        module_name.append(name);

        PyObject *module = PyImport_ImportModule(module_name.constData());

        if (!module)
            return 0;

        PyObject *obj = PyObject_GetAttrString(module, name.constData());

        Py_DECREF(module);

        if (!obj)
            return 0;

        // Sanity check the type object.
        if (PyObject_IsSubclass(obj, (PyObject *)sipTypeAsPyTypeObject(sipType_QAbstractOpenGLFunctions)) <= 0)
        {
            Py_DECREF(obj);

            PyErr_Format(PyExc_TypeError, "%s.%s has an unexpected type",
                    module_name.constData(), name.constData());

            return 0;
        }

        // Populate the type with the OpenGL constants.
        if (qpyopengl_add_constants(obj) < 0)
        {
            Py_DECREF(obj);
            return 0;
        }

        // Cache the type object.
        funcs_type = (PyTypeObject *)obj;
        funcs_types.append(funcs_type);
    }

    // Ownership is with the context.
    return sipConvertFromNewType(funcs, sipTypeFromPyTypeObject(funcs_type),
            py_context);
}

#endif


#endif

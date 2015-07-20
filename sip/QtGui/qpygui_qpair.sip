// This is the SIP interface definition for the QPair based mapped types
// specific to the QtGui module.
//
// Copyright (c) 2015 Riverbank Computing Limited <info@riverbankcomputing.com>
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


%If (Qt_5_2_0 -)

%If (PyQt_OpenGL)

%MappedType QPair<QOpenGLTexture::Filter, QOpenGLTexture::Filter>
        /DocType="tuple-of-QOpenGLTexture.Filter-QOpenGLTexture.Filter"/
{
%TypeHeaderCode
#include <QOpenGLTexture>
%End

%ConvertFromTypeCode
    return sipBuildResult(NULL, "(FF)", sipCpp->first,
            sipType_QOpenGLTexture_Filter, sipCpp->second,
            sipType_QOpenGLTexture_Filter);
%End

%ConvertToTypeCode
    if (!sipIsErr)
        return (PySequence_Check(sipPy)
#if PY_MAJOR_VERSION < 3
                && !PyString_Check(sipPy)
#endif
                && !PyUnicode_Check(sipPy));

    SIP_SSIZE_T len = PySequence_Size(sipPy);

    if (len != 2)
    {
        // A negative length should only be an internal error so let the
        // original exception stand.
        if (len >= 0)
            PyErr_Format(PyExc_TypeError,
                    "sequence has " SIP_SSIZE_T_FORMAT " elements but 2 elements are expected",
                    len);

        *sipIsErr = 1;

        return 0;
    }

    PyObject *firstobj = PySequence_ITEM(sipPy, 0);

    if (!firstobj)
    {
        *sipIsErr = 1;

        return 0;
    }

    if (!sipCanConvertToEnum(firstobj, sipType_QOpenGLTexture_Filter))
    {
        PyErr_Format(PyExc_TypeError,
                "the first element has type '%s' but 'QOpenGLTexture.Filter' is expected",
                Py_TYPE(firstobj)->tp_name);

        *sipIsErr = 1;

        return 0;
    }

    PyObject *secondobj = PySequence_ITEM(sipPy, 1);

    if (!secondobj)
    {
        Py_DECREF(firstobj);
        *sipIsErr = 1;

        return 0;
    }

    if (!sipCanConvertToEnum(secondobj, sipType_QOpenGLTexture_Filter))
    {
        PyErr_Format(PyExc_TypeError,
                "the second element has type '%s' but 'QOpenGLTexture.Filter' is expected",
                Py_TYPE(secondobj)->tp_name);

        Py_DECREF(secondobj);
        Py_DECREF(firstobj);
        *sipIsErr = 1;

        return 0;
    }

    *sipCppPtr = new QPair<QOpenGLTexture::Filter, QOpenGLTexture::Filter>(
            static_cast<QOpenGLTexture::Filter>(SIPLong_AsLong(firstobj)),
            static_cast<QOpenGLTexture::Filter>(SIPLong_AsLong(secondobj)));

    Py_DECREF(secondobj);
    Py_DECREF(firstobj);
 
    return sipGetState(sipTransferObj);
%End
};

%End

%End


%If (Qt_5_2_0 -)

%MappedType QPair<float, float> /DocType="tuple-of-float-float"/
{
%TypeHeaderCode
#include <qpair.h>
%End

%ConvertFromTypeCode
    return Py_BuildValue("(ff)", sipCpp->first, sipCpp->second);
%End

%ConvertToTypeCode
    if (!sipIsErr)
        return (PySequence_Check(sipPy)
#if PY_MAJOR_VERSION < 3
                && !PyString_Check(sipPy)
#endif
                && !PyUnicode_Check(sipPy));

    SIP_SSIZE_T len = PySequence_Size(sipPy);

    if (len != 2)
    {
        // A negative length should only be an internal error so let the
        // original exception stand.
        if (len >= 0)
            PyErr_Format(PyExc_TypeError,
                    "sequence has " SIP_SSIZE_T_FORMAT " elements but 2 elements are expected",
                    len);

        *sipIsErr = 1;

        return 0;
    }

    PyObject *firstobj = PySequence_ITEM(sipPy, 0);

    if (!firstobj)
    {
        *sipIsErr = 1;

        return 0;
    }

    PyErr_Clear();
    double first = PyFloat_AsDouble(firstobj);

    if (PyErr_Occurred())
    {
        PyErr_Format(PyExc_TypeError,
                "the first element has type '%s' but 'float' is expected",
                Py_TYPE(firstobj)->tp_name);

        *sipIsErr = 1;

        return 0;
    }

    PyObject *secondobj = PySequence_ITEM(sipPy, 1);

    if (!secondobj)
    {
        Py_DECREF(firstobj);
        *sipIsErr = 1;

        return 0;
    }

    PyErr_Clear();
    double second = PyFloat_AsDouble(secondobj);

    if (PyErr_Occurred())
    {
        PyErr_Format(PyExc_TypeError,
                "the second element has type '%s' but 'float' is expected",
                Py_TYPE(secondobj)->tp_name);

        Py_DECREF(secondobj);
        Py_DECREF(firstobj);
        *sipIsErr = 1;

        return 0;
    }

    *sipCppPtr = new QPair<float, float>(first, second);;

    Py_DECREF(secondobj);
    Py_DECREF(firstobj);
 
    return sipGetState(sipTransferObj);
%End
};

%End

// This is the SIP interface definition for the majority of the QPair based
// mapped types.
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


template<_TYPE_>
%MappedType QPair<_TYPE_, int> /DocType="tuple-of-_TYPE_-int"/
{
%TypeHeaderCode
#include <qpair.h>
%End

%ConvertFromTypeCode
    _TYPE_ *first = new _TYPE_(sipCpp->first);
    PyObject *t = sipBuildResult(NULL, "(Ni)", first, sipType__TYPE_,
            sipTransferObj, sipCpp->second);

    if (!t)
    {
        delete first;

        return 0;
    }

    return t;
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

    int firststate;
    _TYPE_ *first = reinterpret_cast<_TYPE_ *>(
            sipForceConvertToType(firstobj, sipType__TYPE_, sipTransferObj,
                    SIP_NOT_NONE, &firststate, sipIsErr));
 
    if (*sipIsErr)
    {
        PyErr_Format(PyExc_TypeError,
                "the first element has type '%s' but '_TYPE_' is expected",
                Py_TYPE(firstobj)->tp_name);

        return 0;
    }

    PyObject *secondobj = PySequence_ITEM(sipPy, 1);

    if (!secondobj)
    {
        sipReleaseType(first, sipType__TYPE_, firststate);
        Py_DECREF(firstobj);
        *sipIsErr = 1;

        return 0;
    }

    PyErr_Clear();
    int second = SIPLong_AsLong(secondobj);

    if (PyErr_Occurred())
    {
        PyErr_Format(PyExc_TypeError,
                "the second element has type '%s' but 'int' is expected",
                Py_TYPE(secondobj)->tp_name);

        Py_DECREF(secondobj);
        sipReleaseType(first, sipType__TYPE_, firststate);
        Py_DECREF(firstobj);
        *sipIsErr = 1;

        return 0;
    }

    *sipCppPtr = new QPair<_TYPE_, int>(*first, second);;

    Py_DECREF(secondobj);
    sipReleaseType(first, sipType__TYPE_, firststate);
    Py_DECREF(firstobj);
 
    return sipGetState(sipTransferObj);
%End
};


%MappedType QPair<int, int> /DocType="tuple-of-int-int"/
{
%TypeHeaderCode
#include <qpair.h>
%End

%ConvertFromTypeCode
    return Py_BuildValue("(ii)", sipCpp->first, sipCpp->second);
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
    int first = SIPLong_AsLong(firstobj);

    if (PyErr_Occurred())
    {
        PyErr_Format(PyExc_TypeError,
                "the first element has type '%s' but 'int' is expected",
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
    int second = SIPLong_AsLong(secondobj);

    if (PyErr_Occurred())
    {
        PyErr_Format(PyExc_TypeError,
                "the second element has type '%s' but 'int' is expected",
                Py_TYPE(secondobj)->tp_name);

        Py_DECREF(secondobj);
        Py_DECREF(firstobj);
        *sipIsErr = 1;

        return 0;
    }

    *sipCppPtr = new QPair<int, int>(first, second);;

    Py_DECREF(secondobj);
    Py_DECREF(firstobj);
 
    return sipGetState(sipTransferObj);
%End
};

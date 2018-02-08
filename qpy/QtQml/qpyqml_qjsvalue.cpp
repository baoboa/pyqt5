// This is the support for QJSValue.
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

#include "qpyqml_api.h"

#include "sipAPIQtQml.h"


// See if a Python object can be converted to a QJSValue.
int qpyqml_canConvertTo_QJSValue(PyObject *py)
{
    // Note that the API doesn't provide the ability to test for a constrained
    // named enum.
    if (PyObject_TypeCheck(py, sipTypeAsPyTypeObject(sipType_QJSValue_SpecialValue)))
        return 1;

    if (PyBool_Check(py))
        return 1;

    if (PyLong_Check(py))
        return 1;

#if PY_MAJOR_VERSION < 3
    if (PyInt_Check(py))
        return 1;
#endif

    if (PyFloat_Check(py))
        return 1;

    if (sipCanConvertToType(py, sipType_QString, 0))
        return 1;

    return sipCanConvertToType(py, sipType_QJSValue, SIP_NO_CONVERTORS);
}


// Convert a Python object to a QJSValue.
int qpyqml_convertTo_QJSValue(PyObject *py, PyObject *transferObj,
        QJSValue **cpp, int *isErr)
{
    if (PyObject_TypeCheck(py, sipTypeAsPyTypeObject(sipType_QJSValue_SpecialValue)))
    {
        int v = sipConvertToEnum(py, sipType_QJSValue_SpecialValue);

        if (PyErr_Occurred())
        {
            *isErr = 1;
            return 0;
        }

        *cpp = new QJSValue(static_cast<QJSValue::SpecialValue>(v));

        return sipGetState(transferObj);
    }

    if (PyBool_Check(py))
    {
        *cpp = new QJSValue(py == Py_True);

        return sipGetState(transferObj);
    }

    if (PyLong_Check(py))
    {
        *cpp = new QJSValue((int)PyLong_AsLong(py));

        return sipGetState(transferObj);
    }

#if PY_MAJOR_VERSION < 3
    if (PyInt_Check(py))
    {
        *cpp = new QJSValue((int)PyInt_AS_LONG(py));

        return sipGetState(transferObj);
    }
#endif

    if (PyFloat_Check(py))
    {
        *cpp = new QJSValue((double)PyFloat_AsDouble(py));

        return sipGetState(transferObj);
    }

    if (sipCanConvertToType(py, sipType_QString, 0))
    {
        int state;
        QString *qs = reinterpret_cast<QString *>(sipConvertToType(py,
                sipType_QString, 0, 0, &state, isErr));

        if (*isErr)
        {
            sipReleaseType(qs, sipType_QString, state);
            return 0;
        }

        *cpp = new QJSValue(*qs);

        sipReleaseType(qs, sipType_QString, state);

        return sipGetState(transferObj);
    }

    *cpp = reinterpret_cast<QJSValue *>(sipConvertToType(py, sipType_QJSValue,
            transferObj, SIP_NO_CONVERTORS, 0, isErr));

    return 0;
}

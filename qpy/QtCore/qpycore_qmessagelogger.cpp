// This is the support for QMessageLogger.
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

#include "qpycore_api.h"

#include "sipAPIQtCore.h"


// Return the current Python context.  This should be called with the GIL.
int qpycore_current_context(const char **file, const char **function)
{
    static PyObject *currentframe = 0;
    static PyObject *getframeinfo = 0;
    static PyObject *saved_file = 0;
    static PyObject *saved_function = 0;

    PyObject *frame, *info, *file_obj, *linenr_obj, *function_obj;
    int linenr;

    frame = info = NULL;

    // Make sure we have what we need from the inspect module.
    if (!currentframe || !getframeinfo)
    {
        PyObject *inspect = PyImport_ImportModule("inspect");

        if (inspect)
        {
            if (!currentframe)
                currentframe = PyObject_GetAttrString(inspect, "currentframe");

            if (!getframeinfo)
                getframeinfo = PyObject_GetAttrString(inspect, "getframeinfo");

            Py_DECREF(inspect);
        }

        if (!currentframe || !getframeinfo)
            goto py_error;
    }

    // Get the current frame.
    if ((frame = PyObject_CallFunctionObjArgs(currentframe, NULL)) == NULL)
        goto py_error;

    // Get the frame details.
    if ((info = PyObject_CallFunctionObjArgs(getframeinfo, frame, NULL)) == NULL)
        goto py_error;;

    if ((file_obj = PyTuple_GetItem(info, 0)) == NULL)
        goto py_error;

    if ((linenr_obj = PyTuple_GetItem(info, 1)) == NULL)
        goto py_error;

    if ((function_obj = PyTuple_GetItem(info, 2)) == NULL)
        goto py_error;

    Py_XDECREF(saved_file);
#if PY_MAJOR_VERSION >= 3
    saved_file = PyUnicode_AsEncodedString(file_obj, "latin_1", "ignore");
#else
    saved_file = file_obj;
    Py_INCREF(saved_file);
#endif
    *file = SIPBytes_AsString(saved_file);

    // Ignore any overflow exception.
    linenr = sipLong_AsInt(linenr_obj);

    Py_XDECREF(saved_function);
#if PY_MAJOR_VERSION >= 3
    saved_function = PyUnicode_AsEncodedString(function_obj, "latin_1", "ignore");
#else
    saved_function = function_obj;
    Py_INCREF(saved_function);
#endif
    *function = SIPBytes_AsString(saved_function);

    Py_DECREF(info);
    Py_DECREF(frame);

    return linenr;

py_error:
    Py_XDECREF(info);
    Py_XDECREF(frame);

    pyqt5_err_print();

    *file = *function = "";
    return 0;
}

// This contains the implementation of various odds and ends.
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
#include "qpycore_misc.h"

#include "sipAPIQtCore.h"


// Return true if the given type was wrapped for PyQt5.
bool qpycore_is_pyqt_class(const sipTypeDef *td)
{
    return sipCheckPluginForType(td, "PyQt5.QtCore");
}


#if PY_MAJOR_VERSION >= 3
// Python v3 doesn't have the Unicode equivalent of Python v2's
// PyString_ConcatAndDel().
void qpycore_Unicode_ConcatAndDel(PyObject **string, PyObject *newpart)
{
    PyObject *old = *string;

    if (old)
    {
        if (newpart)
            *string = PyUnicode_Concat(old, newpart);
        else
            *string = 0;

        Py_DECREF(old);
    }

    Py_XDECREF(newpart);
}
#endif

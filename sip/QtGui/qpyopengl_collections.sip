// This implements mapped types for standard Qt collections for OpenGL specific
// types.
//
// Copyright (c) 2013 Riverbank Computing Limited <info@riverbankcomputing.com>
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


// QVector<GLuint64> is implemented as a Python list of integers.
%MappedType QVector<GLuint64> /DocType="list-of-int"/
{
%TypeHeaderCode
#include <qvector.h>
#include <qopengl.h>
%End

%ConvertFromTypeCode
    // Create the list.
    PyObject *l;

    if ((l = PyList_New(sipCpp->size())) == NULL)
        return NULL;

    // Set the list elements.
    for (int i = 0; i < sipCpp->size(); ++i)
    {
        PyObject *pobj;

        // Convert to a Python long to make sure it doesn't get interpreted as
        // a signed value.
        if ((pobj = PyLong_FromUnsignedLongLong(sipCpp->value(i))) == NULL)
        {
            Py_DECREF(l);

            return NULL;
        }

        PyList_SET_ITEM(l, i, pobj);
    }

    return l;
%End

%ConvertToTypeCode
    // Check the type if that is all that is required.
    if (sipIsErr == NULL)
        return PyList_Check(sipPy);

    QVector<GLuint64> *qv = new QVector<GLuint64>;
 
    for (SIP_SSIZE_T i = 0; i < PyList_GET_SIZE(sipPy); ++i)
        qv->append(PyLong_AsUnsignedLongLong(PyList_GET_ITEM(sipPy, i)));
 
    *sipCppPtr = qv;
 
    return sipGetState(sipTransferObj);
%End
};

// This contains the support for Python objects and Qt's metatype system.
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
#include "qpycore_pyqtpyobject.h"

#include "sipAPIQtCore.h"


// The Qt metatype id.
int PyQt_PyObject::metatype;

// The current pickle protocol.
PyObject *qpycore_pickle_protocol;


// Wrap a Python object.
PyQt_PyObject::PyQt_PyObject(PyObject *py)
{
    pyobject = py;

    SIP_BLOCK_THREADS
    Py_XINCREF(pyobject);
    SIP_UNBLOCK_THREADS
}


// Create a new wrapper, with no Python object.  This is called by Qt's
// metatype system.
PyQt_PyObject::PyQt_PyObject()
{
    pyobject = 0;
}


// Create a copy of an existing wrapper.  This is called by Qt's metatype
// system.
PyQt_PyObject::PyQt_PyObject(const PyQt_PyObject &other)
{
    pyobject = other.pyobject;

    SIP_BLOCK_THREADS
    Py_XINCREF(pyobject);
    SIP_UNBLOCK_THREADS
}


// Destroy a wrapper.
PyQt_PyObject::~PyQt_PyObject()
{
    // Qt can still be tidying up after Python has gone so make sure that it
    // hasn't.
    if (Py_IsInitialized())
    {
        SIP_BLOCK_THREADS
        Py_XDECREF(pyobject);
        SIP_UNBLOCK_THREADS
    }

    pyobject = 0;
}


// Assignment operator.
PyQt_PyObject &PyQt_PyObject::operator=(const PyQt_PyObject &other)
{
    pyobject = other.pyobject;

    SIP_BLOCK_THREADS
    Py_XINCREF(pyobject);
    SIP_UNBLOCK_THREADS

    return *this;
}


// Serialise operator.
QDataStream &operator<<(QDataStream &out, const PyQt_PyObject &obj)
{
    PyObject *ser_obj = 0;
    const char *ser = 0;
    uint len = 0;

    if (obj.pyobject)
    {
        static PyObject *dumps = 0;

        SIP_BLOCK_THREADS

        if (!dumps)
        {
            PyObject *pickle = PyImport_ImportModule("pickle");

            if (pickle)
            {
                dumps = PyObject_GetAttrString(pickle, "dumps");
                Py_DECREF(pickle);
            }
        }

        if (dumps)
        {
            if (!qpycore_pickle_protocol)
            {
                Py_INCREF(Py_None);
                qpycore_pickle_protocol = Py_None;
            }

            ser_obj = PyObject_CallFunctionObjArgs(dumps, obj.pyobject,
                    qpycore_pickle_protocol, 0);

            if (ser_obj)
            {
                if (SIPBytes_Check(ser_obj))
                {
                    ser = SIPBytes_AsString(ser_obj);
                    len = SIPBytes_Size(ser_obj);
                }
                else
                {
                    Py_DECREF(ser_obj);
                    ser_obj = 0;
                }
            }
            else
            {
                pyqt5_err_print();
            }
        }

        SIP_UNBLOCK_THREADS
    }

    out.writeBytes(ser, len);

    if (ser_obj)
    {
        SIP_BLOCK_THREADS
        Py_DECREF(ser_obj);
        SIP_UNBLOCK_THREADS
    }

    return out;
}


// De-serialise operator.
QDataStream &operator>>(QDataStream &in, PyQt_PyObject &obj)
{
    char *ser;
    uint len;

    in.readBytes(ser, len);

    if (len)
    {
        static PyObject *loads = 0;

        SIP_BLOCK_THREADS

        if (!loads)
        {
            PyObject *pickle = PyImport_ImportModule("pickle");

            if (pickle)
            {
                loads = PyObject_GetAttrString(pickle, "loads");
                Py_DECREF(pickle);
            }
        }

        if (loads)
        {
            PyObject *ser_obj = SIPBytes_FromStringAndSize(ser, len);

            if (ser_obj)
            {
                obj.pyobject = PyObject_CallFunctionObjArgs(loads, ser_obj, 0);
                Py_DECREF(ser_obj);
            }
        }

        SIP_UNBLOCK_THREADS
    }

    delete[] ser;

    return in;
}

// This is the SIP specification of the QQmlListProperty mapped type.
//
// Copyright (c) 2014 Riverbank Computing Limited <info@riverbankcomputing.com>
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


%MappedType QQmlListProperty<QObject> /DocType="QQmlListProperty"/
{
%TypeHeaderCode
#include <QQmlListProperty>
%End

%TypeCode
#if defined(SIP_USE_PYCAPSULE)
extern "C" {
static void qpyqml_QQmlListProperty_delete(PyObject *capsule)
{
    delete reinterpret_cast<QQmlListProperty<QObject> *>(PyCapsule_GetPointer(capsule, 0));
}
}
#else
extern "C" {
static void qpyqml_QQmlListProperty_delete(void *ptr)
{
    delete reinterpret_cast<QQmlListProperty<QObject> *>(ptr);
}
}
#endif
%End

%ConvertFromTypeCode
#if defined(SIP_USE_PYCAPSULE)
    return PyCapsule_New(sipCpp, 0, qpyqml_QQmlListProperty_delete);
#else
    return PyCObject_FromVoidPtr(sipCpp, qpyqml_QQmlListProperty_delete);
#endif
%End

%ConvertToTypeCode
    if (sipIsErr == NULL)
    {
#if defined(SIP_USE_PYCAPSULE)
        return PyCapsule_CheckExact(sipPy);
#else
        return PyCObject_Check(sipPy);
#endif
    }

#if defined(SIP_USE_PYCAPSULE)
    *sipCppPtr = reinterpret_cast<QQmlListProperty<QObject> *>(
            PyCapsule_GetPointer(sipPy, 0));
#else
    *sipCppPtr = reinterpret_cast<QQmlListProperty<QObject> *>(
            PyCObject_AsVoidPtr(sipPy));
#endif

    // It isn't a temporary copy.
    return 0;
%End
};

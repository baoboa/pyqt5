// This is the implementation of the Chimera::Signature class.
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

#include "qpycore_chimera.h"


// Forward declarations.
#if defined(SIP_USE_PYCAPSULE)
extern "C" {static void Signature_delete(PyObject *cap);}
#else
extern "C" {static void Signature_delete(void *sig);}
#endif


// Destroy the signature and any type objects.
Chimera::Signature::~Signature()
{
    if (!_cached)
        qDeleteAll(parsed_arguments.constBegin(), parsed_arguments.constEnd());

    if (result)
        delete result;
}


// Return the parsed signature wrapped in a Python object.
PyObject *Chimera::Signature::toPyObject(Chimera::Signature *parsed_signature)
{
#if defined(SIP_USE_PYCAPSULE)
    PyObject *py = PyCapsule_New(parsed_signature, NULL, Signature_delete);
#else
    PyObject *py = PyCObject_FromVoidPtr(parsed_signature, Signature_delete);
#endif

    if (!py)
        delete parsed_signature;

    return py;
}


// Return the parsed signature extracted from a Python object.
Chimera::Signature *Chimera::Signature::fromPyObject(PyObject *py)
{
#if defined(SIP_USE_PYCAPSULE)
    return reinterpret_cast<Chimera::Signature *>(PyCapsule_GetPointer(py, NULL));
#else
    return reinterpret_cast<Chimera::Signature *>(PyCObject_AsVoidPtr(py));
#endif
}


// Return the name from a signature.
QByteArray Chimera::Signature::name(const QByteArray &signature)
{
    QByteArray nm = signature;
    int idx = signature.indexOf('(');

    if (idx >= 0)
        nm.truncate(idx);

    return nm;
}


// Return the arguments from a signature.
QByteArray Chimera::Signature::arguments(const QByteArray &signature)
{
    QByteArray args;

    int oparen = signature.indexOf('(');
    int cparen = signature.lastIndexOf(')');

    if (oparen >= 0 && cparen >= 0)
        args = signature.mid(oparen, cparen - oparen + 1);

    return args;
}


#if defined(SIP_USE_PYCAPSULE)
// The PyCapsule destructor for the Chimera::Signature type.
static void Signature_delete(PyObject *cap)
{
    delete reinterpret_cast<Chimera::Signature *>(
            PyCapsule_GetPointer(cap, NULL));
}
#else
// The PyCObject destructor for the Chimera::Signature type.
static void Signature_delete(void *sig)
{
    delete reinterpret_cast<Chimera::Signature *>(sig);
}
#endif

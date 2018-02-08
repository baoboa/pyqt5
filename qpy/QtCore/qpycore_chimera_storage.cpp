// This is the implementation of the Chimera::Storage class.
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

#include <QVariant>

#include "qpycore_chimera.h"
#include "qpycore_pyqtpyobject.h"

#include "sipAPIQtCore.h"


// Create a new storage instance containing a converted Python object.
Chimera::Storage::Storage(const Chimera *ct, PyObject *py)
    : _parsed_type(ct), _ptr_storage(0), _tmp_state(0)
{
    // We convert to a QVariant even for pointer types because this has the
    // side-effect of ensuring the object doesn't get garbage collected.
    _valid = _parsed_type->fromPyObject(py, &_value_storage);

    if (isPointerType())
    {
        int is_err = 0;

        _ptr_storage = sipForceConvertToType(py, _parsed_type->typeDef(), 0, 0,
                &_tmp_state, &is_err);

        if (is_err)
        {
            _value_storage = QVariant();
            _valid = false;
        }
    }
}


// Create a new storage instance.
Chimera::Storage::Storage(const Chimera *ct)
    : _parsed_type(ct), _ptr_storage(0), _tmp_state(0), _valid(true)
{
    if (!isPointerType())
    {
        // Create a default fundamental or value type.
        _value_storage = QVariant(_parsed_type->metatype(), (const void *)0);
    }
}


// Destroy the storage and any temporary value.
Chimera::Storage::~Storage()
{
    if (_tmp_state)
        sipReleaseType(_ptr_storage, _parsed_type->typeDef(), _tmp_state);
}


// Return the address of the storage.
void *Chimera::Storage::address()
{
    if (isPointerType())
    {
        if (_parsed_type->name().endsWith('*'))
            return &_ptr_storage;

        return _ptr_storage;
    }

    if (_parsed_type->typeDef() == sipType_QVariant)
        return &_value_storage;

    return _value_storage.data();
}


// Return true if the value is a pointer type.
bool Chimera::Storage::isPointerType() const
{
    return (_parsed_type->metatype() == PyQt_PyObject::metatype &&
            _parsed_type->typeDef());
}


// Convert the stored value to a Python object.
PyObject *Chimera::Storage::toPyObject() const
{
    // Pointer-class and mapped types are not stored in the QVariant.
    if (isPointerType())
    {
        if (!_ptr_storage)
        {
            Py_INCREF(Py_None);
            return Py_None;
        }

        return sipConvertFromType(_ptr_storage, _parsed_type->typeDef(), 0);
    }

    if (_parsed_type->typeDef() == sipType_QVariant)
        return Chimera::toAnyPyObject(_value_storage);

    return _parsed_type->toPyObject(_value_storage);
}

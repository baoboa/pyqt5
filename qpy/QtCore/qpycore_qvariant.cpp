// This is the support for QVariant.
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

#include "qpycore_api.h"
#include "qpycore_chimera.h"


// Return the Python object from the variant.
PyObject *qpycore_PyObject_FromQVariant(const QVariant &qvar)
{
    return Chimera::toAnyPyObject(qvar);
}


// Return a variant containing the Python object.  It will be converted to a
// C++ instance if possible (ie. if it is a fundamental type or a direct
// wrapped type).  Otherwise it is left as a Python object (with an increased
// reference count).  An exception is raised if there was a conversion error.
// (The type of the object will never be the cause of an error.)
QVariant qpycore_PyObject_AsQVariant(PyObject *obj, int *is_err)
{
    return Chimera::fromAnyPyObject(obj, is_err);
}

// This is the definition of the various Chimera helpers.
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


#ifndef _QPYQUICK_CHIMERAHELPERS_H
#define _QPYQUICK_CHIMERAHELPERS_H


#include <Python.h>

#include <QVariant>


// Keep these in sync. with those defined in the public API.
typedef bool (*FromQVariantConvertorFn)(const QVariant *, PyObject **);
typedef bool (*ToQVariantConvertorFn)(PyObject *, QVariant &, bool *);
typedef bool (*ToQVariantDataConvertorFn)(PyObject *, void *, int, bool *);


bool qpyquick_from_qvariant_convertor(const QVariant *varp, PyObject **objp);
bool qpyquick_to_qvariant_convertor(PyObject *obj, QVariant &var, bool *okp);
bool qpyquick_to_qvariant_data_convertor(PyObject *obj, void *data,
        int metatype, bool *okp);


#endif

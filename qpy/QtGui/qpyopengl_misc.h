// This defines the interfaces to various odd and ends.
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


#ifndef _QPYOPENGL_MISC_H
#define _QPYOPENGL_MISC_H


#include <Python.h>


// Replacements for the corresponding Python macros that use the limited API.
#define Sequence_Fast_Size(o) \
    (PyList_Check(o) ? PyList_Size(o) : PyTuple_Size(o))
#define Sequence_Fast_GetItem(o, i)\
    (PyList_Check(o) ? PyList_GetItem(o, i) : PyTuple_GetItem(o, i))


#endif

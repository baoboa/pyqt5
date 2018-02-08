// This contains the implementation of the Q_CLASSINFO support.
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

#include <QMultiHash>

#include "qpycore_api.h"
#include "qpycore_classinfo.h"

#include "sipAPIQtCore.h"


static QMultiHash<const struct _frame *, ClassInfo> class_info_hash;


// Add the given name/value pair to the current class info hash.
PyObject *qpycore_ClassInfo(const char *name, const char *value)
{
    // We need the frame we were called from, not the current one.
    struct _frame *frame = sipGetFrame(1);

    if (!frame)
    {
        PyErr_SetString(PyExc_RuntimeError, "no previous frame");
        return 0;
    }

    class_info_hash.insert(frame,
            ClassInfo(QByteArray(name), QByteArray(value)));

    Py_INCREF(Py_None);
    return Py_None;
}


// Return the current class info list.
QList<ClassInfo> qpycore_get_class_info_list()
{
    struct _frame *frame = sipGetFrame(0);
    QList<ClassInfo> class_info_list = class_info_hash.values(frame);

    class_info_hash.remove(frame);

    return class_info_list;
}

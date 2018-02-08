// This is the implementation of the various Chimera helpers.
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

#include <QGraphicsItem>

#include "qpywidgets_chimera_helpers.h"

#include "sipAPIQtWidgets.h"


// Convert a Python object to a QVariant.
bool qpywidgets_to_qvariant_convertor(PyObject *obj, QVariant &var, bool *okp)
{
    // QGraphicsItem sub-classes aren't known to the meta-object system and,
    // if we didn't do anything, would be interpreted as QObjects by the
    // default convertion code.  This would cause problems for Python
    // reimplementations of QGraphicsItem::itemChange().

    if (!sipCanConvertToType(obj, sipType_QGraphicsItem, SIP_NO_CONVERTORS))
        return false;

    int iserr = 0;

    QGraphicsItem *qgi = reinterpret_cast<QGraphicsItem *>(
            sipConvertToType(obj, sipType_QGraphicsItem, 0, SIP_NO_CONVERTORS,
                    0, &iserr));

    *okp = !iserr;

    if (!iserr)
        var.setValue(qgi);

    return true;
}

// This is the implementation of the qml_register_type() helper.
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


#include "qpyquick_register_type.h"
#include "qpyquickframebufferobject.h"
#include "qpyquickitem.h"
#include "qpyquickpainteditem.h"

#include "sipAPIQtQuick.h"


sipErrorState qpyquick_register_type(PyTypeObject *py_type,
        const QMetaObject *mo, const QByteArray &ptr_name,
        const QByteArray &list_name, QQmlPrivate::RegisterType **rtp)
{
#if QT_VERSION >= 0x050200
    if (PyType_IsSubtype(py_type, sipTypeAsPyTypeObject(sipType_QQuickFramebufferObject)))
        return ((*rtp = QPyQuickFramebufferObject::addType(py_type, mo, ptr_name, list_name)) ? sipErrorNone : sipErrorFail);
#endif

    if (PyType_IsSubtype(py_type, sipTypeAsPyTypeObject(sipType_QQuickPaintedItem)))
        return ((*rtp = QPyQuickPaintedItem::addType(py_type, mo, ptr_name, list_name)) ? sipErrorNone : sipErrorFail);

    if (PyType_IsSubtype(py_type, sipTypeAsPyTypeObject(sipType_QQuickItem)))
        return ((*rtp = QPyQuickItem::addType(py_type, mo, ptr_name, list_name)) ? sipErrorNone : sipErrorFail);

    // We don't recognise the type.
    return sipErrorContinue;
}

// This defines the interfaces for the meta-type used by PyQt.
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


#ifndef _QPYCORE_TYPES_H
#define _QPYCORE_TYPES_H


#include <QList>
#include <QMetaObject>

#include "qpycore_pyqtproperty.h"


class PyQtSlot;


// This describes a dynamic meta-object.
struct qpycore_metaobject
{
    // The meta-object built by QMetaObjectBuilder.
    QMetaObject *mo;

    // The list of properties.
    QList<qpycore_pyqtProperty *> pprops;

    // The list of slots.
    QList<PyQtSlot *> pslots;

    // The number of signals.
    int nr_signals;
};


int qpycore_new_user_type_handler(sipWrapperType *wt);
const QMetaObject *qpycore_get_qmetaobject(sipWrapperType *wt,
        const sipTypeDef *base_td = 0);


#endif

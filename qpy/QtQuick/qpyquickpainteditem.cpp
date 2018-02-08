// This is the implementation of the QPyQuickPaintedItem classes.
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

#include <QQmlListProperty>

#include "qpyquickpainteditem.h"

#include "sipAPIQtQuick.h"


// The maximum number of Python QQuickPaintedItem types.
const int NrOfQuickPaintedItemTypes = 30;

// The list of registered Python types.
static QList<PyTypeObject *> pyqt_types;

// The registration data for the canned types.
static QQmlPrivate::RegisterType canned_types[NrOfQuickPaintedItemTypes];


#define QPYQUICKPAINTEDITEM_INIT(n) \
    case n##U: \
        QPyQuickPaintedItem##n::staticMetaObject = *mo; \
        rt->typeId = qRegisterNormalizedMetaType<QPyQuickPaintedItem##n *>(ptr_name.constData()); \
        rt->listId = qRegisterNormalizedMetaType<QQmlListProperty<QPyQuickPaintedItem##n> >(list_name.constData()); \
        rt->objectSize = sizeof(QPyQuickPaintedItem##n); \
        rt->create = QQmlPrivate::createInto<QPyQuickPaintedItem##n>; \
        rt->metaObject = mo; \
        rt->attachedPropertiesFunction = QQmlPrivate::attachedPropertiesFunc<QPyQuickPaintedItem##n>(); \
        rt->attachedPropertiesMetaObject = QQmlPrivate::attachedPropertiesMetaObject<QPyQuickPaintedItem##n>(); \
        rt->parserStatusCast = QQmlPrivate::StaticCastSelector<QPyQuickPaintedItem##n,QQmlParserStatus>::cast(); \
        rt->valueSourceCast = QQmlPrivate::StaticCastSelector<QPyQuickPaintedItem##n,QQmlPropertyValueSource>::cast(); \
        rt->valueInterceptorCast = QQmlPrivate::StaticCastSelector<QPyQuickPaintedItem##n,QQmlPropertyValueInterceptor>::cast(); \
        break


// The ctor.
QPyQuickPaintedItem::QPyQuickPaintedItem(QQuickItem *parent)
        : sipQQuickPaintedItem(parent)
{
}


// Add a new Python type and return its number.
QQmlPrivate::RegisterType *QPyQuickPaintedItem::addType(PyTypeObject *type,
        const QMetaObject *mo, const QByteArray &ptr_name,
        const QByteArray &list_name)
{
    int type_nr = pyqt_types.size();

    // Check we have a spare canned type.
    if (type_nr >= NrOfQuickPaintedItemTypes)
    {
        PyErr_Format(PyExc_TypeError,
                "a maximum of %d QQuickPaintedItem types may be registered with QML",
                NrOfQuickPaintedItemTypes);
        return 0;
    }

    pyqt_types.append(type);

    QQmlPrivate::RegisterType *rt = &canned_types[type_nr];

    // Initialise those members that depend on the C++ type.
    switch (type_nr)
    {
        QPYQUICKPAINTEDITEM_INIT(0);
        QPYQUICKPAINTEDITEM_INIT(1);
        QPYQUICKPAINTEDITEM_INIT(2);
        QPYQUICKPAINTEDITEM_INIT(3);
        QPYQUICKPAINTEDITEM_INIT(4);
        QPYQUICKPAINTEDITEM_INIT(5);
        QPYQUICKPAINTEDITEM_INIT(6);
        QPYQUICKPAINTEDITEM_INIT(7);
        QPYQUICKPAINTEDITEM_INIT(8);
        QPYQUICKPAINTEDITEM_INIT(9);
        QPYQUICKPAINTEDITEM_INIT(10);
        QPYQUICKPAINTEDITEM_INIT(11);
        QPYQUICKPAINTEDITEM_INIT(12);
        QPYQUICKPAINTEDITEM_INIT(13);
        QPYQUICKPAINTEDITEM_INIT(14);
        QPYQUICKPAINTEDITEM_INIT(15);
        QPYQUICKPAINTEDITEM_INIT(16);
        QPYQUICKPAINTEDITEM_INIT(17);
        QPYQUICKPAINTEDITEM_INIT(18);
        QPYQUICKPAINTEDITEM_INIT(19);
        QPYQUICKPAINTEDITEM_INIT(20);
        QPYQUICKPAINTEDITEM_INIT(21);
        QPYQUICKPAINTEDITEM_INIT(22);
        QPYQUICKPAINTEDITEM_INIT(23);
        QPYQUICKPAINTEDITEM_INIT(24);
        QPYQUICKPAINTEDITEM_INIT(25);
        QPYQUICKPAINTEDITEM_INIT(26);
        QPYQUICKPAINTEDITEM_INIT(27);
        QPYQUICKPAINTEDITEM_INIT(28);
        QPYQUICKPAINTEDITEM_INIT(29);
    }

    return rt;
}


// Create the Python instance.
void QPyQuickPaintedItem::createPyObject(QQuickItem *parent)
{
    SIP_BLOCK_THREADS

    // Assume C++ owns everything.
    PyObject *obj = sipConvertFromNewPyType(this, pyqt_types.at(typeNr()),
            NULL, &sipPySelf, "D", parent, sipType_QQuickItem, NULL);

    if (!obj)
        pyqt5_qtquick_err_print();

    SIP_UNBLOCK_THREADS
}


// The canned type implementations.
#define QPYQUICKPAINTEDITEM_IMPL(n) \
QPyQuickPaintedItem##n::QPyQuickPaintedItem##n(QQuickItem *parent) : QPyQuickPaintedItem(parent) \
{ \
    createPyObject(parent); \
} \
QMetaObject QPyQuickPaintedItem##n::staticMetaObject


QPYQUICKPAINTEDITEM_IMPL(0);
QPYQUICKPAINTEDITEM_IMPL(1);
QPYQUICKPAINTEDITEM_IMPL(2);
QPYQUICKPAINTEDITEM_IMPL(3);
QPYQUICKPAINTEDITEM_IMPL(4);
QPYQUICKPAINTEDITEM_IMPL(5);
QPYQUICKPAINTEDITEM_IMPL(6);
QPYQUICKPAINTEDITEM_IMPL(7);
QPYQUICKPAINTEDITEM_IMPL(8);
QPYQUICKPAINTEDITEM_IMPL(9);
QPYQUICKPAINTEDITEM_IMPL(10);
QPYQUICKPAINTEDITEM_IMPL(11);
QPYQUICKPAINTEDITEM_IMPL(12);
QPYQUICKPAINTEDITEM_IMPL(13);
QPYQUICKPAINTEDITEM_IMPL(14);
QPYQUICKPAINTEDITEM_IMPL(15);
QPYQUICKPAINTEDITEM_IMPL(16);
QPYQUICKPAINTEDITEM_IMPL(17);
QPYQUICKPAINTEDITEM_IMPL(18);
QPYQUICKPAINTEDITEM_IMPL(19);
QPYQUICKPAINTEDITEM_IMPL(20);
QPYQUICKPAINTEDITEM_IMPL(21);
QPYQUICKPAINTEDITEM_IMPL(22);
QPYQUICKPAINTEDITEM_IMPL(23);
QPYQUICKPAINTEDITEM_IMPL(24);
QPYQUICKPAINTEDITEM_IMPL(25);
QPYQUICKPAINTEDITEM_IMPL(26);
QPYQUICKPAINTEDITEM_IMPL(27);
QPYQUICKPAINTEDITEM_IMPL(28);
QPYQUICKPAINTEDITEM_IMPL(29);

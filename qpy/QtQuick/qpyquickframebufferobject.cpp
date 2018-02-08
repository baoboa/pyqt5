// This is the implementation of the QPyQuickFramebufferObject classes.
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

#include "qpyquickframebufferobject.h"

#include "sipAPIQtQuick.h"


#if QT_VERSION >= 0x050200

// The maximum number of Python QQuickFramebufferObject types.
const int NrOfQuickFramebufferObjectTypes = 30;

// The list of registered Python types.
static QList<PyTypeObject *> pyqt_types;

// The registration data for the canned types.
static QQmlPrivate::RegisterType canned_types[NrOfQuickFramebufferObjectTypes];


#define QPYQUICKFRAMEBUFFEROBJECT_INIT(n) \
    case n##U: \
        QPyQuickFramebufferObject##n::staticMetaObject = *mo; \
        rt->typeId = qRegisterNormalizedMetaType<QPyQuickFramebufferObject##n *>(ptr_name.constData()); \
        rt->listId = qRegisterNormalizedMetaType<QQmlListProperty<QPyQuickFramebufferObject##n> >(list_name.constData()); \
        rt->objectSize = sizeof(QPyQuickFramebufferObject##n); \
        rt->create = QQmlPrivate::createInto<QPyQuickFramebufferObject##n>; \
        rt->metaObject = mo; \
        rt->attachedPropertiesFunction = QQmlPrivate::attachedPropertiesFunc<QPyQuickFramebufferObject##n>(); \
        rt->attachedPropertiesMetaObject = QQmlPrivate::attachedPropertiesMetaObject<QPyQuickFramebufferObject##n>(); \
        rt->parserStatusCast = QQmlPrivate::StaticCastSelector<QPyQuickFramebufferObject##n,QQmlParserStatus>::cast(); \
        rt->valueSourceCast = QQmlPrivate::StaticCastSelector<QPyQuickFramebufferObject##n,QQmlPropertyValueSource>::cast(); \
        rt->valueInterceptorCast = QQmlPrivate::StaticCastSelector<QPyQuickFramebufferObject##n,QQmlPropertyValueInterceptor>::cast(); \
        break


// The ctor.
QPyQuickFramebufferObject::QPyQuickFramebufferObject(QQuickItem *parent)
        : sipQQuickFramebufferObject(parent)
{
}


// Add a new Python type and return its number.
QQmlPrivate::RegisterType *QPyQuickFramebufferObject::addType(
        PyTypeObject *type, const QMetaObject *mo, const QByteArray &ptr_name,
        const QByteArray &list_name)
{
    int type_nr = pyqt_types.size();

    // Check we have a spare canned type.
    if (type_nr >= NrOfQuickFramebufferObjectTypes)
    {
        PyErr_Format(PyExc_TypeError,
                "a maximum of %d QQuickFramebufferObject types may be registered with QML",
                NrOfQuickFramebufferObjectTypes);
        return 0;
    }

    pyqt_types.append(type);

    QQmlPrivate::RegisterType *rt = &canned_types[type_nr];

    // Initialise those members that depend on the C++ type.
    switch (type_nr)
    {
        QPYQUICKFRAMEBUFFEROBJECT_INIT(0);
        QPYQUICKFRAMEBUFFEROBJECT_INIT(1);
        QPYQUICKFRAMEBUFFEROBJECT_INIT(2);
        QPYQUICKFRAMEBUFFEROBJECT_INIT(3);
        QPYQUICKFRAMEBUFFEROBJECT_INIT(4);
        QPYQUICKFRAMEBUFFEROBJECT_INIT(5);
        QPYQUICKFRAMEBUFFEROBJECT_INIT(6);
        QPYQUICKFRAMEBUFFEROBJECT_INIT(7);
        QPYQUICKFRAMEBUFFEROBJECT_INIT(8);
        QPYQUICKFRAMEBUFFEROBJECT_INIT(9);
        QPYQUICKFRAMEBUFFEROBJECT_INIT(10);
        QPYQUICKFRAMEBUFFEROBJECT_INIT(11);
        QPYQUICKFRAMEBUFFEROBJECT_INIT(12);
        QPYQUICKFRAMEBUFFEROBJECT_INIT(13);
        QPYQUICKFRAMEBUFFEROBJECT_INIT(14);
        QPYQUICKFRAMEBUFFEROBJECT_INIT(15);
        QPYQUICKFRAMEBUFFEROBJECT_INIT(16);
        QPYQUICKFRAMEBUFFEROBJECT_INIT(17);
        QPYQUICKFRAMEBUFFEROBJECT_INIT(18);
        QPYQUICKFRAMEBUFFEROBJECT_INIT(19);
        QPYQUICKFRAMEBUFFEROBJECT_INIT(20);
        QPYQUICKFRAMEBUFFEROBJECT_INIT(21);
        QPYQUICKFRAMEBUFFEROBJECT_INIT(22);
        QPYQUICKFRAMEBUFFEROBJECT_INIT(23);
        QPYQUICKFRAMEBUFFEROBJECT_INIT(24);
        QPYQUICKFRAMEBUFFEROBJECT_INIT(25);
        QPYQUICKFRAMEBUFFEROBJECT_INIT(26);
        QPYQUICKFRAMEBUFFEROBJECT_INIT(27);
        QPYQUICKFRAMEBUFFEROBJECT_INIT(28);
        QPYQUICKFRAMEBUFFEROBJECT_INIT(29);
    }

    return rt;
}


// Create the Python instance.
void QPyQuickFramebufferObject::createPyObject(QQuickItem *parent)
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
#define QPYQUICKFRAMEBUFFEROBJECT_IMPL(n) \
QPyQuickFramebufferObject##n::QPyQuickFramebufferObject##n(QQuickItem *parent) : QPyQuickFramebufferObject(parent) \
{ \
    createPyObject(parent); \
} \
QMetaObject QPyQuickFramebufferObject##n::staticMetaObject


QPYQUICKFRAMEBUFFEROBJECT_IMPL(0);
QPYQUICKFRAMEBUFFEROBJECT_IMPL(1);
QPYQUICKFRAMEBUFFEROBJECT_IMPL(2);
QPYQUICKFRAMEBUFFEROBJECT_IMPL(3);
QPYQUICKFRAMEBUFFEROBJECT_IMPL(4);
QPYQUICKFRAMEBUFFEROBJECT_IMPL(5);
QPYQUICKFRAMEBUFFEROBJECT_IMPL(6);
QPYQUICKFRAMEBUFFEROBJECT_IMPL(7);
QPYQUICKFRAMEBUFFEROBJECT_IMPL(8);
QPYQUICKFRAMEBUFFEROBJECT_IMPL(9);
QPYQUICKFRAMEBUFFEROBJECT_IMPL(10);
QPYQUICKFRAMEBUFFEROBJECT_IMPL(11);
QPYQUICKFRAMEBUFFEROBJECT_IMPL(12);
QPYQUICKFRAMEBUFFEROBJECT_IMPL(13);
QPYQUICKFRAMEBUFFEROBJECT_IMPL(14);
QPYQUICKFRAMEBUFFEROBJECT_IMPL(15);
QPYQUICKFRAMEBUFFEROBJECT_IMPL(16);
QPYQUICKFRAMEBUFFEROBJECT_IMPL(17);
QPYQUICKFRAMEBUFFEROBJECT_IMPL(18);
QPYQUICKFRAMEBUFFEROBJECT_IMPL(19);
QPYQUICKFRAMEBUFFEROBJECT_IMPL(20);
QPYQUICKFRAMEBUFFEROBJECT_IMPL(21);
QPYQUICKFRAMEBUFFEROBJECT_IMPL(22);
QPYQUICKFRAMEBUFFEROBJECT_IMPL(23);
QPYQUICKFRAMEBUFFEROBJECT_IMPL(24);
QPYQUICKFRAMEBUFFEROBJECT_IMPL(25);
QPYQUICKFRAMEBUFFEROBJECT_IMPL(26);
QPYQUICKFRAMEBUFFEROBJECT_IMPL(27);
QPYQUICKFRAMEBUFFEROBJECT_IMPL(28);
QPYQUICKFRAMEBUFFEROBJECT_IMPL(29);

#endif

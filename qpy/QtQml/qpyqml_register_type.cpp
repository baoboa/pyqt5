// This contains the main implementation of qmlRegisterType.
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

#include <qqmlprivate.h>
#include <QByteArray>
#include <QString>
#include <QQmlListProperty>
#include <QQmlParserStatus>
#include <QQmlPropertyValueSource>

#include "qpyqml_api.h"
#include "qpyqmlobject.h"

#include "sipAPIQtQml.h"


class QQmlPropertyValueInterceptor;


// Forward declarations.
static QQmlPrivate::RegisterType *init_type(PyTypeObject *py_type, bool ctor,
        int revision, PyTypeObject *attached);
static void complete_init(QQmlPrivate::RegisterType *rt, int revision);
static int register_type(QQmlPrivate::RegisterType *rt);


// The number of types that can be registered.
const int NrOfTypes = 60;


// The registration data for the proxy types.
static QQmlPrivate::RegisterType proxy_types[NrOfTypes];


// Register a Python type.
int qpyqml_register_type(PyTypeObject *py_type, PyTypeObject *attached)
{
    // Initialise the registration data structure.
    QQmlPrivate::RegisterType *rt = init_type(py_type, false, -1, attached);

    if (!rt)
        return -1;

    return register_type(rt);
}


// Register a library Python type.
int qpyqml_register_library_type(PyTypeObject *py_type, const char *uri,
        int major, int minor, const char *qml_name, int revision,
        PyTypeObject *attached)
{
    // Initialise the registration data structure.
    QQmlPrivate::RegisterType *rt = init_type(py_type, true, revision,
            attached);

    if (!rt)
        return -1;

    rt->uri = uri;
    rt->versionMajor = major;
    rt->versionMinor = minor;
    rt->elementName = qml_name;

    return register_type(rt);
}


// Register an uncreatable library Python type.
int qpyqml_register_uncreatable_type(PyTypeObject *py_type, const char *uri,
        int major, int minor, const char *qml_name, const QString &reason,
        int revision)
{
    // Initialise the registration data structure.
    QQmlPrivate::RegisterType *rt = init_type(py_type, false, revision, 0);

    if (!rt)
        return -1;

    rt->noCreationReason = reason;
    rt->uri = uri;
    rt->versionMajor = major;
    rt->versionMinor = minor;
    rt->elementName = qml_name;

    return register_type(rt);
}


// Register the proxy type with QML.
static int register_type(QQmlPrivate::RegisterType *rt)
{
    int type_id = QQmlPrivate::qmlregister(QQmlPrivate::TypeRegistration, rt);

    if (type_id < 0)
    {
        PyErr_SetString(PyExc_RuntimeError,
                "unable to register type with QML");
        return -1;
    }

    return type_id;
}


#define QPYQML_TYPE_INIT(n) \
    case n##U: \
        QPyQmlObject##n::staticMetaObject = *mo; \
        QPyQmlObject##n::attachedPyType = attached; \
        rt->typeId = qRegisterNormalizedMetaType<QPyQmlObject##n *>(ptr_name); \
        rt->listId = qRegisterNormalizedMetaType<QQmlListProperty<QPyQmlObject##n> >(list_name); \
        rt->objectSize = ctor ? sizeof(QPyQmlObject##n) : 0; \
        if (ctor) rt->create = QQmlPrivate::createInto<QPyQmlObject##n>; else rt->create = 0; \
        rt->attachedPropertiesFunction = attached_mo ? QPyQmlObject##n::attachedProperties : 0; \
        rt->parserStatusCast = is_parser_status ? QQmlPrivate::StaticCastSelector<QPyQmlObject##n,QQmlParserStatus>::cast() : -1; \
        rt->valueSourceCast = is_value_source ? QQmlPrivate::StaticCastSelector<QPyQmlObject##n,QQmlPropertyValueSource>::cast() : -1; \
        rt->valueInterceptorCast = QQmlPrivate::StaticCastSelector<QPyQmlObject##n,QQmlPropertyValueInterceptor>::cast(); \
        break


// This is needed for GCC v4.6 and earlier.
#define QPYQML_TYPE_IMPL(n) \
    template void QQmlPrivate::createInto<QPyQmlObject##n>(void *)

QPYQML_TYPE_IMPL(0);
QPYQML_TYPE_IMPL(1);
QPYQML_TYPE_IMPL(2);
QPYQML_TYPE_IMPL(3);
QPYQML_TYPE_IMPL(4);
QPYQML_TYPE_IMPL(5);
QPYQML_TYPE_IMPL(6);
QPYQML_TYPE_IMPL(7);
QPYQML_TYPE_IMPL(8);
QPYQML_TYPE_IMPL(9);
QPYQML_TYPE_IMPL(10);
QPYQML_TYPE_IMPL(11);
QPYQML_TYPE_IMPL(12);
QPYQML_TYPE_IMPL(13);
QPYQML_TYPE_IMPL(14);
QPYQML_TYPE_IMPL(15);
QPYQML_TYPE_IMPL(16);
QPYQML_TYPE_IMPL(17);
QPYQML_TYPE_IMPL(18);
QPYQML_TYPE_IMPL(19);
QPYQML_TYPE_IMPL(20);
QPYQML_TYPE_IMPL(21);
QPYQML_TYPE_IMPL(22);
QPYQML_TYPE_IMPL(23);
QPYQML_TYPE_IMPL(24);
QPYQML_TYPE_IMPL(25);
QPYQML_TYPE_IMPL(26);
QPYQML_TYPE_IMPL(27);
QPYQML_TYPE_IMPL(28);
QPYQML_TYPE_IMPL(29);
QPYQML_TYPE_IMPL(30);
QPYQML_TYPE_IMPL(31);
QPYQML_TYPE_IMPL(32);
QPYQML_TYPE_IMPL(33);
QPYQML_TYPE_IMPL(34);
QPYQML_TYPE_IMPL(35);
QPYQML_TYPE_IMPL(36);
QPYQML_TYPE_IMPL(37);
QPYQML_TYPE_IMPL(38);
QPYQML_TYPE_IMPL(39);
QPYQML_TYPE_IMPL(40);
QPYQML_TYPE_IMPL(41);
QPYQML_TYPE_IMPL(42);
QPYQML_TYPE_IMPL(43);
QPYQML_TYPE_IMPL(44);
QPYQML_TYPE_IMPL(45);
QPYQML_TYPE_IMPL(46);
QPYQML_TYPE_IMPL(47);
QPYQML_TYPE_IMPL(48);
QPYQML_TYPE_IMPL(49);
QPYQML_TYPE_IMPL(50);
QPYQML_TYPE_IMPL(51);
QPYQML_TYPE_IMPL(52);
QPYQML_TYPE_IMPL(53);
QPYQML_TYPE_IMPL(54);
QPYQML_TYPE_IMPL(55);
QPYQML_TYPE_IMPL(56);
QPYQML_TYPE_IMPL(57);
QPYQML_TYPE_IMPL(58);
QPYQML_TYPE_IMPL(59);


// Return a pointer to the initialised registration structure for a type.
static QQmlPrivate::RegisterType *init_type(PyTypeObject *py_type, bool ctor,
        int revision, PyTypeObject *attached)
{
    PyTypeObject *qobject_type = sipTypeAsPyTypeObject(sipType_QObject);

    // Check the type is derived from QObject and get its meta-object.
    if (!PyType_IsSubtype(py_type, qobject_type))
    {
        PyErr_SetString(PyExc_TypeError,
                "type being registered must be a sub-type of QObject");
        return 0;
    }

    const QMetaObject *mo = pyqt5_qtqml_get_qmetaobject(py_type);

    // See if the type is a parser status.
    bool is_parser_status = PyType_IsSubtype(py_type,
            sipTypeAsPyTypeObject(sipType_QQmlParserStatus));

    // See if the type is a property value source.
    bool is_value_source = PyType_IsSubtype(py_type,
            sipTypeAsPyTypeObject(sipType_QQmlPropertyValueSource));

    // Check any attached type is derived from QObject and get its meta-object.
    const QMetaObject *attached_mo;

    if (attached)
    {
        if (!PyType_IsSubtype(attached, qobject_type))
        {
            PyErr_SetString(PyExc_TypeError,
                    "attached properties type must be a sub-type of QObject");
            return 0;
        }

        attached_mo = pyqt5_qtqml_get_qmetaobject(attached);

        Py_INCREF((PyObject *)attached);
    }
    else
    {
        attached_mo = 0;
    }

    QByteArray ptr_name(sipPyTypeName(py_type));
    ptr_name.append('*');

    QByteArray list_name(sipPyTypeName(py_type));
    list_name.prepend("QQmlListProperty<");
    list_name.append('>');

    QQmlPrivate::RegisterType *rt;

    // See if we have the QQuickItem registation helper from the QtQuick
    // module.  Check each time because it could be imported at any point.

    typedef sipErrorState (*QQuickItemRegisterFn)(PyTypeObject *, const QMetaObject *, const QByteArray &, const QByteArray &, QQmlPrivate::RegisterType **);

    static QQuickItemRegisterFn qquickitem_register = 0;

    if (!qquickitem_register)
        qquickitem_register = (QQuickItemRegisterFn)sipImportSymbol(
                "qtquick_register_item");

    if (qquickitem_register)
    {
        sipErrorState estate = qquickitem_register(py_type, mo, ptr_name,
                list_name, &rt);

        if (estate == sipErrorFail)
            return 0;

        if (estate == sipErrorNone)
        {
            complete_init(rt, revision);
            return rt;
        }
    }

    // Get the type's number and check there aren't too many.
    int type_nr = QPyQmlObjectProxy::addType(py_type);

    if (type_nr >= NrOfTypes)
    {
        PyErr_Format(PyExc_TypeError,
                "a maximum of %d types may be registered with QML", NrOfTypes);
        return 0;
    }

    rt = &proxy_types[type_nr];

    // Initialise those members that depend on the C++ type.
    switch (type_nr)
    {
        QPYQML_TYPE_INIT(0);
        QPYQML_TYPE_INIT(1);
        QPYQML_TYPE_INIT(2);
        QPYQML_TYPE_INIT(3);
        QPYQML_TYPE_INIT(4);
        QPYQML_TYPE_INIT(5);
        QPYQML_TYPE_INIT(6);
        QPYQML_TYPE_INIT(7);
        QPYQML_TYPE_INIT(8);
        QPYQML_TYPE_INIT(9);
        QPYQML_TYPE_INIT(10);
        QPYQML_TYPE_INIT(11);
        QPYQML_TYPE_INIT(12);
        QPYQML_TYPE_INIT(13);
        QPYQML_TYPE_INIT(14);
        QPYQML_TYPE_INIT(15);
        QPYQML_TYPE_INIT(16);
        QPYQML_TYPE_INIT(17);
        QPYQML_TYPE_INIT(18);
        QPYQML_TYPE_INIT(19);
        QPYQML_TYPE_INIT(20);
        QPYQML_TYPE_INIT(21);
        QPYQML_TYPE_INIT(22);
        QPYQML_TYPE_INIT(23);
        QPYQML_TYPE_INIT(24);
        QPYQML_TYPE_INIT(25);
        QPYQML_TYPE_INIT(26);
        QPYQML_TYPE_INIT(27);
        QPYQML_TYPE_INIT(28);
        QPYQML_TYPE_INIT(29);
        QPYQML_TYPE_INIT(30);
        QPYQML_TYPE_INIT(31);
        QPYQML_TYPE_INIT(32);
        QPYQML_TYPE_INIT(33);
        QPYQML_TYPE_INIT(34);
        QPYQML_TYPE_INIT(35);
        QPYQML_TYPE_INIT(36);
        QPYQML_TYPE_INIT(37);
        QPYQML_TYPE_INIT(38);
        QPYQML_TYPE_INIT(39);
        QPYQML_TYPE_INIT(40);
        QPYQML_TYPE_INIT(41);
        QPYQML_TYPE_INIT(42);
        QPYQML_TYPE_INIT(43);
        QPYQML_TYPE_INIT(44);
        QPYQML_TYPE_INIT(45);
        QPYQML_TYPE_INIT(46);
        QPYQML_TYPE_INIT(47);
        QPYQML_TYPE_INIT(48);
        QPYQML_TYPE_INIT(49);
        QPYQML_TYPE_INIT(50);
        QPYQML_TYPE_INIT(51);
        QPYQML_TYPE_INIT(52);
        QPYQML_TYPE_INIT(53);
        QPYQML_TYPE_INIT(54);
        QPYQML_TYPE_INIT(55);
        QPYQML_TYPE_INIT(56);
        QPYQML_TYPE_INIT(57);
        QPYQML_TYPE_INIT(58);
        QPYQML_TYPE_INIT(59);
    }

    rt->metaObject = mo;
    rt->attachedPropertiesMetaObject = attached_mo;

    complete_init(rt, revision);

    return rt;
}


// Complete the initialisation of a type registration structure.
static void complete_init(QQmlPrivate::RegisterType *rt, int revision)
{
    if (revision < 0)
    {
        rt->version = 0;
        rt->revision = 0;
    }
    else
    {
        rt->version = 1;
        rt->revision = revision;
    }

    rt->uri = 0;
    rt->versionMajor = 0;
    rt->versionMinor = 0;
    rt->elementName = 0;
    rt->extensionObjectCreate = 0;
    rt->extensionMetaObject = 0;
    rt->customParser = 0;
}


// Return the proxy that created an object.  This is called with the GIL.
QObject *qpyqml_find_proxy_for(QObject *proxied)
{
    QSetIterator<QObject *> it(QPyQmlObjectProxy::proxies);

    while (it.hasNext())
    {
        QPyQmlObjectProxy *proxy = static_cast<QPyQmlObjectProxy *>(it.next());

        if (proxy->proxied.data() == proxied)
            return proxy;
    }

    PyErr_Format(PyExc_TypeError,
            "QObject instance at %p was not created from QML", proxied);

    return 0;
}

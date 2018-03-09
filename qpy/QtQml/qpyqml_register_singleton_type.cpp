// This contains the main implementation of qmlRegisterSingletonType.
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

#include "qpyqml_api.h"
#include "qpyqmlsingletonobject.h"

#include "sipAPIQtQml.h"


// Forward declarations.
static QQmlPrivate::RegisterSingletonType *init_type(PyTypeObject *py_type,
        PyObject *factory);
static int register_type(QQmlPrivate::RegisterSingletonType *rt);


// The number of types that can be registered.
const int NrOfTypes = 30;


// The registration data for the proxy types.
static QQmlPrivate::RegisterSingletonType proxy_types[NrOfTypes];


// Register a singleton Python type.
int qpyqml_register_singleton_type(PyTypeObject *py_type, const char *uri,
        int major, int minor, const char *type_name, PyObject *factory)
{
    // Initialise the registration data structure.
    QQmlPrivate::RegisterSingletonType *rt = init_type(py_type, factory);

    if (!rt)
        return -1;

    rt->uri = uri;
    rt->versionMajor = major;
    rt->versionMinor = minor;
    rt->typeName = type_name;

    return register_type(rt);
}


// Register the proxy type with QML.
static int register_type(QQmlPrivate::RegisterSingletonType *rt)
{
    int type_id = QQmlPrivate::qmlregister(QQmlPrivate::SingletonRegistration, rt);

    if (type_id < 0)
    {
        PyErr_SetString(PyExc_RuntimeError,
                "unable to register singleton type with QML");
        return -1;
    }

    return type_id;
}


#define QPYQML_SINGLETON_TYPE_INIT(n) \
    case n##U: \
        QPyQmlSingletonObject##n::staticMetaObject = *mo; \
        QPyQmlSingletonObject##n::factory = factory; \
        rt->qobjectApi = QPyQmlSingletonObject##n::objectFactory; \
        rt->typeId = qRegisterNormalizedMetaType<QPyQmlSingletonObject##n *>(ptr_name.constData()); \
        break


// Return a pointer to the initialised registration structure for a type.
static QQmlPrivate::RegisterSingletonType *init_type(PyTypeObject *py_type,
        PyObject *factory)
{
    // Check the type is derived from QObject.
    if (!PyType_IsSubtype(py_type, sipTypeAsPyTypeObject(sipType_QObject)))
    {
        PyErr_SetString(PyExc_TypeError,
                "type being registered must be a sub-type of QObject");
        return 0;
    }

    // Get the type's number and check there aren't too many.
    int type_nr = QPyQmlSingletonObjectProxy::addType();

    if (type_nr >= NrOfTypes)
    {
        PyErr_Format(PyExc_TypeError,
                "a maximum of %d singleton types may be registered with QML",
                NrOfTypes);
        return 0;
    }

    QQmlPrivate::RegisterSingletonType *rt = &proxy_types[type_nr];

    const QMetaObject *mo = pyqt5_qtqml_get_qmetaobject(py_type);

    QByteArray ptr_name(sipPyTypeName(py_type));
    ptr_name.append('*');

    // Initialise those members that depend on the C++ type.
    switch (type_nr)
    {
        QPYQML_SINGLETON_TYPE_INIT(0);
        QPYQML_SINGLETON_TYPE_INIT(1);
        QPYQML_SINGLETON_TYPE_INIT(2);
        QPYQML_SINGLETON_TYPE_INIT(3);
        QPYQML_SINGLETON_TYPE_INIT(4);
        QPYQML_SINGLETON_TYPE_INIT(5);
        QPYQML_SINGLETON_TYPE_INIT(6);
        QPYQML_SINGLETON_TYPE_INIT(7);
        QPYQML_SINGLETON_TYPE_INIT(8);
        QPYQML_SINGLETON_TYPE_INIT(9);
        QPYQML_SINGLETON_TYPE_INIT(10);
        QPYQML_SINGLETON_TYPE_INIT(11);
        QPYQML_SINGLETON_TYPE_INIT(12);
        QPYQML_SINGLETON_TYPE_INIT(13);
        QPYQML_SINGLETON_TYPE_INIT(14);
        QPYQML_SINGLETON_TYPE_INIT(15);
        QPYQML_SINGLETON_TYPE_INIT(16);
        QPYQML_SINGLETON_TYPE_INIT(17);
        QPYQML_SINGLETON_TYPE_INIT(18);
        QPYQML_SINGLETON_TYPE_INIT(19);
        QPYQML_SINGLETON_TYPE_INIT(20);
        QPYQML_SINGLETON_TYPE_INIT(21);
        QPYQML_SINGLETON_TYPE_INIT(22);
        QPYQML_SINGLETON_TYPE_INIT(23);
        QPYQML_SINGLETON_TYPE_INIT(24);
        QPYQML_SINGLETON_TYPE_INIT(25);
        QPYQML_SINGLETON_TYPE_INIT(26);
        QPYQML_SINGLETON_TYPE_INIT(27);
        QPYQML_SINGLETON_TYPE_INIT(28);
        QPYQML_SINGLETON_TYPE_INIT(29);
    }

    // Initialise the remaining members.
    rt->instanceMetaObject = mo;

    rt->version = 2;
    rt->uri = 0;
    rt->versionMajor = 0;
    rt->versionMinor = 0;
    rt->typeName = 0;
    rt->scriptApi = 0;
    rt->revision = 0;

    Py_INCREF(factory);

    return rt;
}

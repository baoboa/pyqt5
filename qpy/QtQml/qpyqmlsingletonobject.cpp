// This is the implementation of the QPyQmlSingletonObject classes.
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


#include "qpyqmlsingletonobject.h"

#include "sipAPIQtQml.h"


// The number of registered Python types.
int QPyQmlSingletonObjectProxy::nr_types = 0;


// Add a new Python type and return its number.
int QPyQmlSingletonObjectProxy::addType()
{
    return nr_types++;
}


// Create the QObject instance.
QObject *QPyQmlSingletonObjectProxy::createObject(QQmlEngine *engine,
        QJSEngine *scriptEngine, PyObject *factory)
{
    // This should never happen.
    if (!factory)
        return 0;

    QObject *qobject;

    SIP_BLOCK_THREADS

    PyObject *py_obj = sipCallMethod(NULL, factory, "DD", engine,
            sipType_QQmlEngine, NULL, scriptEngine, sipType_QJSEngine, NULL);

    if (py_obj)
    {
        qobject = reinterpret_cast<QObject *>(
                sipGetAddress((sipSimpleWrapper *)py_obj));

        // The engine will call the dtor and garbage collect the Python object.
        sipTransferTo(py_obj, Py_None);
        Py_DECREF(py_obj);
    }
    else
    {
        qobject = 0;
        pyqt5_qtqml_err_print();
    }

    Py_DECREF(factory);

    SIP_UNBLOCK_THREADS

    return qobject;
}


// The proxy type implementations.
#define QPYQML_SINGLETON_PROXY_IMPL(n) \
PyObject *QPyQmlSingletonObject##n::factory = 0; \
QMetaObject QPyQmlSingletonObject##n::staticMetaObject


QPYQML_SINGLETON_PROXY_IMPL(0);
QPYQML_SINGLETON_PROXY_IMPL(1);
QPYQML_SINGLETON_PROXY_IMPL(2);
QPYQML_SINGLETON_PROXY_IMPL(3);
QPYQML_SINGLETON_PROXY_IMPL(4);
QPYQML_SINGLETON_PROXY_IMPL(5);
QPYQML_SINGLETON_PROXY_IMPL(6);
QPYQML_SINGLETON_PROXY_IMPL(7);
QPYQML_SINGLETON_PROXY_IMPL(8);
QPYQML_SINGLETON_PROXY_IMPL(9);
QPYQML_SINGLETON_PROXY_IMPL(10);
QPYQML_SINGLETON_PROXY_IMPL(11);
QPYQML_SINGLETON_PROXY_IMPL(12);
QPYQML_SINGLETON_PROXY_IMPL(13);
QPYQML_SINGLETON_PROXY_IMPL(14);
QPYQML_SINGLETON_PROXY_IMPL(15);
QPYQML_SINGLETON_PROXY_IMPL(16);
QPYQML_SINGLETON_PROXY_IMPL(17);
QPYQML_SINGLETON_PROXY_IMPL(18);
QPYQML_SINGLETON_PROXY_IMPL(19);
QPYQML_SINGLETON_PROXY_IMPL(20);
QPYQML_SINGLETON_PROXY_IMPL(21);
QPYQML_SINGLETON_PROXY_IMPL(22);
QPYQML_SINGLETON_PROXY_IMPL(23);
QPYQML_SINGLETON_PROXY_IMPL(24);
QPYQML_SINGLETON_PROXY_IMPL(25);
QPYQML_SINGLETON_PROXY_IMPL(26);
QPYQML_SINGLETON_PROXY_IMPL(27);
QPYQML_SINGLETON_PROXY_IMPL(28);
QPYQML_SINGLETON_PROXY_IMPL(29);

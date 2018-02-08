// This is the definition of the QPyQmlSingletonObject classes.
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


#ifndef _QPYQMLOBJECT_H
#define _QPYQMLOBJECT_H


#include <Python.h>

#include <QObject>
#include <QJSEngine>
#include <QQmlEngine>


// Note that these classes are never instantiated.  They are used to give the
// registration structure appropriate values (although whether or not those
// values are actually used isn't clear).
class QPyQmlSingletonObjectProxy : public QObject
{
public:
    static int addType();
    static QObject *createObject(QQmlEngine *engine, QJSEngine *scriptEngine,
            PyObject *factory);

private:
    // The number of registered types.
    static int nr_types;

    QPyQmlSingletonObjectProxy(const QPyQmlSingletonObjectProxy &);
};


// The proxy type declarations.
#define QPYQML_SINGLETON_PROXY_DECL(n) \
class QPyQmlSingletonObject##n : public QPyQmlSingletonObjectProxy \
{ \
public: \
    static QMetaObject staticMetaObject; \
    static PyObject *factory; \
    static QObject *objectFactory(QQmlEngine *engine, QJSEngine *scriptEngine) \
    { \
        QObject *qo = createObject(engine, scriptEngine, factory); \
        factory = 0; \
        return qo; \
    } \
private: \
    QPyQmlSingletonObject##n(const QPyQmlSingletonObject##n &); \
}


QPYQML_SINGLETON_PROXY_DECL(0);
QPYQML_SINGLETON_PROXY_DECL(1);
QPYQML_SINGLETON_PROXY_DECL(2);
QPYQML_SINGLETON_PROXY_DECL(3);
QPYQML_SINGLETON_PROXY_DECL(4);
QPYQML_SINGLETON_PROXY_DECL(5);
QPYQML_SINGLETON_PROXY_DECL(6);
QPYQML_SINGLETON_PROXY_DECL(7);
QPYQML_SINGLETON_PROXY_DECL(8);
QPYQML_SINGLETON_PROXY_DECL(9);
QPYQML_SINGLETON_PROXY_DECL(10);
QPYQML_SINGLETON_PROXY_DECL(11);
QPYQML_SINGLETON_PROXY_DECL(12);
QPYQML_SINGLETON_PROXY_DECL(13);
QPYQML_SINGLETON_PROXY_DECL(14);
QPYQML_SINGLETON_PROXY_DECL(15);
QPYQML_SINGLETON_PROXY_DECL(16);
QPYQML_SINGLETON_PROXY_DECL(17);
QPYQML_SINGLETON_PROXY_DECL(18);
QPYQML_SINGLETON_PROXY_DECL(19);
QPYQML_SINGLETON_PROXY_DECL(20);
QPYQML_SINGLETON_PROXY_DECL(21);
QPYQML_SINGLETON_PROXY_DECL(22);
QPYQML_SINGLETON_PROXY_DECL(23);
QPYQML_SINGLETON_PROXY_DECL(24);
QPYQML_SINGLETON_PROXY_DECL(25);
QPYQML_SINGLETON_PROXY_DECL(26);
QPYQML_SINGLETON_PROXY_DECL(27);
QPYQML_SINGLETON_PROXY_DECL(28);
QPYQML_SINGLETON_PROXY_DECL(29);


#endif

// This is the definition of the QPyQuickFramebufferObject classes.
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


#ifndef _QPYQUICKFRAMEBUFFEROBJECT_H
#define _QPYQUICKFRAMEBUFFEROBJECT_H


#include <Python.h>

#include <qqmlprivate.h>
#include <QByteArray>
#include <QMetaObject>
#include <QQuickItem>

#include "sipAPIQtQuick.h"


#if QT_VERSION >= 0x050200

class QPyQuickFramebufferObject : public sipQQuickFramebufferObject
{
public:
    QPyQuickFramebufferObject(QQuickItem *parent = 0);

    virtual int typeNr() const = 0;

    static QQmlPrivate::RegisterType *addType(PyTypeObject *type,
            const QMetaObject *mo, const QByteArray &ptr_name,
            const QByteArray &list_name);
    void createPyObject(QQuickItem *parent);

private:
    QPyQuickFramebufferObject(const QPyQuickFramebufferObject &);
};


// The canned type declarations.
#define QPYQUICKFRAMEBUFFEROBJECT_DECL(n) \
class QPyQuickFramebufferObject##n : public QPyQuickFramebufferObject \
{ \
public: \
    QPyQuickFramebufferObject##n(QQuickItem *parent = 0); \
    static QMetaObject staticMetaObject; \
    virtual int typeNr() const {return n##U;} \
private: \
    QPyQuickFramebufferObject##n(const QPyQuickFramebufferObject##n &); \
}


QPYQUICKFRAMEBUFFEROBJECT_DECL(0);
QPYQUICKFRAMEBUFFEROBJECT_DECL(1);
QPYQUICKFRAMEBUFFEROBJECT_DECL(2);
QPYQUICKFRAMEBUFFEROBJECT_DECL(3);
QPYQUICKFRAMEBUFFEROBJECT_DECL(4);
QPYQUICKFRAMEBUFFEROBJECT_DECL(5);
QPYQUICKFRAMEBUFFEROBJECT_DECL(6);
QPYQUICKFRAMEBUFFEROBJECT_DECL(7);
QPYQUICKFRAMEBUFFEROBJECT_DECL(8);
QPYQUICKFRAMEBUFFEROBJECT_DECL(9);
QPYQUICKFRAMEBUFFEROBJECT_DECL(10);
QPYQUICKFRAMEBUFFEROBJECT_DECL(11);
QPYQUICKFRAMEBUFFEROBJECT_DECL(12);
QPYQUICKFRAMEBUFFEROBJECT_DECL(13);
QPYQUICKFRAMEBUFFEROBJECT_DECL(14);
QPYQUICKFRAMEBUFFEROBJECT_DECL(15);
QPYQUICKFRAMEBUFFEROBJECT_DECL(16);
QPYQUICKFRAMEBUFFEROBJECT_DECL(17);
QPYQUICKFRAMEBUFFEROBJECT_DECL(18);
QPYQUICKFRAMEBUFFEROBJECT_DECL(19);
QPYQUICKFRAMEBUFFEROBJECT_DECL(20);
QPYQUICKFRAMEBUFFEROBJECT_DECL(21);
QPYQUICKFRAMEBUFFEROBJECT_DECL(22);
QPYQUICKFRAMEBUFFEROBJECT_DECL(23);
QPYQUICKFRAMEBUFFEROBJECT_DECL(24);
QPYQUICKFRAMEBUFFEROBJECT_DECL(25);
QPYQUICKFRAMEBUFFEROBJECT_DECL(26);
QPYQUICKFRAMEBUFFEROBJECT_DECL(27);
QPYQUICKFRAMEBUFFEROBJECT_DECL(28);
QPYQUICKFRAMEBUFFEROBJECT_DECL(29);

#endif


#endif

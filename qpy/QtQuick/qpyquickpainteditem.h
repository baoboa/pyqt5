// This is the definition of the QPyQuickPaintedItem classes.
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


#ifndef _QPYQUICKPAINTEDITEM_H
#define _QPYQUICKPAINTEDITEM_H


#include <Python.h>

#include <qqmlprivate.h>
#include <QByteArray>
#include <QMetaObject>
#include <QQuickItem>

#include "sipAPIQtQuick.h"


class QPyQuickPaintedItem : public sipQQuickPaintedItem
{
public:
    QPyQuickPaintedItem(QQuickItem *parent = 0);

    virtual int typeNr() const = 0;

    static QQmlPrivate::RegisterType *addType(PyTypeObject *type,
            const QMetaObject *mo, const QByteArray &ptr_name,
            const QByteArray &list_name);
    void createPyObject(QQuickItem *parent);

private:
    QPyQuickPaintedItem(const QPyQuickPaintedItem &);
};


// The canned type declarations.
#define QPYQUICKPAINTEDITEM_DECL(n) \
class QPyQuickPaintedItem##n : public QPyQuickPaintedItem \
{ \
public: \
    QPyQuickPaintedItem##n(QQuickItem *parent = 0); \
    static QMetaObject staticMetaObject; \
    virtual int typeNr() const {return n##U;} \
private: \
    QPyQuickPaintedItem##n(const QPyQuickPaintedItem##n &); \
}


QPYQUICKPAINTEDITEM_DECL(0);
QPYQUICKPAINTEDITEM_DECL(1);
QPYQUICKPAINTEDITEM_DECL(2);
QPYQUICKPAINTEDITEM_DECL(3);
QPYQUICKPAINTEDITEM_DECL(4);
QPYQUICKPAINTEDITEM_DECL(5);
QPYQUICKPAINTEDITEM_DECL(6);
QPYQUICKPAINTEDITEM_DECL(7);
QPYQUICKPAINTEDITEM_DECL(8);
QPYQUICKPAINTEDITEM_DECL(9);
QPYQUICKPAINTEDITEM_DECL(10);
QPYQUICKPAINTEDITEM_DECL(11);
QPYQUICKPAINTEDITEM_DECL(12);
QPYQUICKPAINTEDITEM_DECL(13);
QPYQUICKPAINTEDITEM_DECL(14);
QPYQUICKPAINTEDITEM_DECL(15);
QPYQUICKPAINTEDITEM_DECL(16);
QPYQUICKPAINTEDITEM_DECL(17);
QPYQUICKPAINTEDITEM_DECL(18);
QPYQUICKPAINTEDITEM_DECL(19);
QPYQUICKPAINTEDITEM_DECL(20);
QPYQUICKPAINTEDITEM_DECL(21);
QPYQUICKPAINTEDITEM_DECL(22);
QPYQUICKPAINTEDITEM_DECL(23);
QPYQUICKPAINTEDITEM_DECL(24);
QPYQUICKPAINTEDITEM_DECL(25);
QPYQUICKPAINTEDITEM_DECL(26);
QPYQUICKPAINTEDITEM_DECL(27);
QPYQUICKPAINTEDITEM_DECL(28);
QPYQUICKPAINTEDITEM_DECL(29);


#endif

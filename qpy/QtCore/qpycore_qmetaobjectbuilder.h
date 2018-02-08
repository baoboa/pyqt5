// This defines the subset of the interface to Qt5's internal
// QMetaObjectBuilder class.  The internal representation of a QMetaObject
// changed in Qt5 (specifically revision 7) sufficiently to justify using this
// internal code.  The alternative would be to reverse engineer other internal
// data structures which would be even more fragile.
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


#ifndef _QPYCORE_QMETAOBJECTBUILDER_H
#define _QPYCORE_QMETAOBJECTBUILDER_H


#include <QByteArray>
#include <QList>
#include <QMetaObject>

QT_BEGIN_NAMESPACE


class QMetaEnumBuilder;
class QMetaMethodBuilder;
class QMetaPropertyBuilder;


class Q_CORE_EXPORT QMetaObjectBuilder
{
public:
    QMetaObjectBuilder();
    virtual ~QMetaObjectBuilder();

    void setClassName(const QByteArray &name);
    void setSuperClass(const QMetaObject *meta);
    QMetaMethodBuilder addSlot(const QByteArray &signature);
    QMetaMethodBuilder addSignal(const QByteArray &signature);
    QMetaPropertyBuilder addProperty(const QByteArray &name,
            const QByteArray &type, int notifierId=-1);
    QMetaEnumBuilder addEnumerator(const QByteArray &name);
    int addClassInfo(const QByteArray &name, const QByteArray &value);
    int addRelatedMetaObject(const QMetaObject *meta);
    int indexOfSignal(const QByteArray &signature);
    QMetaObject *toMetaObject() const;

private:
    Q_DISABLE_COPY(QMetaObjectBuilder)

    void *d;
};


class Q_CORE_EXPORT QMetaMethodBuilder
{
public:
    QMetaMethodBuilder() : _mobj(0), _index(0) {}

    void setReturnType(const QByteArray &type);
    void setParameterNames(const QList<QByteArray> &parameter_names);
    void setRevision(int revision);

private:
    const QMetaObjectBuilder *_mobj;
    int _index;
};


class Q_CORE_EXPORT QMetaPropertyBuilder
{
public:
    QMetaPropertyBuilder() : _mobj(0), _index(0) {}

    void setReadable(bool value);
    void setWritable(bool value);
    void setResettable(bool value);
    void setDesignable(bool value);
    void setScriptable(bool value);
    void setStored(bool value);
    void setUser(bool value);
    void setStdCppSet(bool value);
    void setEnumOrFlag(bool value);
    void setConstant(bool value);
    void setFinal(bool value);
    void setRevision(int revision);

private:
    const QMetaObjectBuilder *_mobj;
    int _index;
};


class Q_CORE_EXPORT QMetaEnumBuilder
{
public:
    QMetaEnumBuilder() : _mobj(0), _index(0) {}

    void setIsFlag(bool value);
    int addKey(const QByteArray &name, int value);

private:
    const QMetaObjectBuilder *_mobj;
    int _index;
};


QT_END_NAMESPACE

#endif

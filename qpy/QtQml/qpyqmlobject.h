// This is the definition of the QPyQmlObject classes.
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

#include <QAbstractItemModel>
#include <QList>
#include <QPointer>
#include <QQmlParserStatus>
#include <QQmlProperty>
#include <QQmlPropertyValueSource>
#include <QSet>
#include <QSize>


class QPyQmlObjectProxy : public QAbstractItemModel
{
public:
    QPyQmlObjectProxy(QObject *parent = 0);
    virtual ~QPyQmlObjectProxy();

    virtual const QMetaObject *metaObject() const;
    virtual void *qt_metacast(const char *_clname);
    virtual int qt_metacall(QMetaObject::Call, int, void **);

    virtual int typeNr() const = 0;

    static int addType(PyTypeObject *type);
    void createPyObject(QObject *parent);

    static QObject *createAttachedProperties(PyTypeObject *py_type,
            QObject *parent);

    static void *resolveProxy(void *proxy);

    void pyClassBegin();
    void pyComponentComplete();

    void pySetTarget(const QQmlProperty &target);

    // The set of proxies in existence.
    static QSet<QObject *> proxies;

    // The real object.
    QPointer<QObject> proxied;

    // The real object if it is an item model.
    QAbstractItemModel *proxied_model;

    // QAbstractItemModel virtuals.
    virtual QModelIndex index(int row, int column,
            const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex &child) const;
    virtual QModelIndex sibling(int row, int column, const QModelIndex &idx)
            const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual bool hasChildren(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole)
            const;
    virtual bool setData(const QModelIndex &index, const QVariant &value,
            int role = Qt::EditRole);
    virtual QVariant headerData(int section, Qt::Orientation orientation,
            int role = Qt::DisplayRole) const;
    virtual bool setHeaderData(int section, Qt::Orientation orientation,
            const QVariant &value, int role = Qt::EditRole);
    virtual QMap<int, QVariant> itemData(const QModelIndex &index) const;
    virtual bool setItemData(const QModelIndex &index,
            const QMap<int, QVariant> &roles);
    virtual QStringList mimeTypes() const;
    virtual QMimeData *mimeData(const QModelIndexList &indexes) const;
    virtual bool canDropMimeData(const QMimeData *data, Qt::DropAction action,
            int row, int column, const QModelIndex &parent) const;
    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action,
            int row, int column, const QModelIndex &parent);
    virtual Qt::DropActions supportedDropActions() const;
    virtual Qt::DropActions supportedDragActions() const;
    virtual bool insertRows(int row, int count,
            const QModelIndex &parent = QModelIndex());
    virtual bool insertColumns(int column, int count,
            const QModelIndex &parent = QModelIndex());
    virtual bool removeRows(int row, int count,
            const QModelIndex &parent = QModelIndex());
    virtual bool removeColumns(int column, int count,
            const QModelIndex &parent = QModelIndex());
    virtual bool moveRows(const QModelIndex &sourceParent, int sourceRow,
            int count, const QModelIndex &destinationParent,
            int destinationChild);
    virtual bool moveColumns(const QModelIndex &sourceParent, int sourceColumn,
            int count, const QModelIndex &destinationParent,
            int destinationChild);
    virtual void fetchMore(const QModelIndex &parent);
    virtual bool canFetchMore(const QModelIndex &parent) const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);
    virtual QModelIndex buddy(const QModelIndex &index) const;
    virtual QModelIndexList match(const QModelIndex &start, int role,
            const QVariant &value, int hits = 1,
            Qt::MatchFlags flags =
                    Qt::MatchFlags(Qt::MatchStartsWith|Qt::MatchWrap)) const;
    virtual QSize span(const QModelIndex &index) const;
    virtual QHash<int,QByteArray> roleNames() const;

protected:
    void connectNotify(const QMetaMethod &signal);

private:
    // These can by cast to sipWrapperType.
    static QList<PyTypeObject *> pyqt_types;

    // The wrapped proxied object.
    PyObject *py_proxied;

    static QByteArray signalSignature(const QMetaMethod &signal);

    QPyQmlObjectProxy(const QPyQmlObjectProxy &);
};


// The proxy type declarations.
#define QPYQML_PROXY_DECL(n) \
class QPyQmlObject##n : public QPyQmlObjectProxy, public QQmlParserStatus, public QQmlPropertyValueSource  \
{ \
public: \
    QPyQmlObject##n(QObject *parent = 0); \
    static QMetaObject staticMetaObject; \
    virtual int typeNr() const {return n##U;} \
    static PyTypeObject *attachedPyType; \
    static QObject *attachedProperties(QObject *parent); \
    virtual void classBegin(); \
    virtual void componentComplete(); \
    virtual void setTarget(const QQmlProperty &target); \
private: \
    QPyQmlObject##n(const QPyQmlObject##n &); \
}


QPYQML_PROXY_DECL(0);
QPYQML_PROXY_DECL(1);
QPYQML_PROXY_DECL(2);
QPYQML_PROXY_DECL(3);
QPYQML_PROXY_DECL(4);
QPYQML_PROXY_DECL(5);
QPYQML_PROXY_DECL(6);
QPYQML_PROXY_DECL(7);
QPYQML_PROXY_DECL(8);
QPYQML_PROXY_DECL(9);
QPYQML_PROXY_DECL(10);
QPYQML_PROXY_DECL(11);
QPYQML_PROXY_DECL(12);
QPYQML_PROXY_DECL(13);
QPYQML_PROXY_DECL(14);
QPYQML_PROXY_DECL(15);
QPYQML_PROXY_DECL(16);
QPYQML_PROXY_DECL(17);
QPYQML_PROXY_DECL(18);
QPYQML_PROXY_DECL(19);
QPYQML_PROXY_DECL(20);
QPYQML_PROXY_DECL(21);
QPYQML_PROXY_DECL(22);
QPYQML_PROXY_DECL(23);
QPYQML_PROXY_DECL(24);
QPYQML_PROXY_DECL(25);
QPYQML_PROXY_DECL(26);
QPYQML_PROXY_DECL(27);
QPYQML_PROXY_DECL(28);
QPYQML_PROXY_DECL(29);
QPYQML_PROXY_DECL(30);
QPYQML_PROXY_DECL(31);
QPYQML_PROXY_DECL(32);
QPYQML_PROXY_DECL(33);
QPYQML_PROXY_DECL(34);
QPYQML_PROXY_DECL(35);
QPYQML_PROXY_DECL(36);
QPYQML_PROXY_DECL(37);
QPYQML_PROXY_DECL(38);
QPYQML_PROXY_DECL(39);
QPYQML_PROXY_DECL(40);
QPYQML_PROXY_DECL(41);
QPYQML_PROXY_DECL(42);
QPYQML_PROXY_DECL(43);
QPYQML_PROXY_DECL(44);
QPYQML_PROXY_DECL(45);
QPYQML_PROXY_DECL(46);
QPYQML_PROXY_DECL(47);
QPYQML_PROXY_DECL(48);
QPYQML_PROXY_DECL(49);
QPYQML_PROXY_DECL(50);
QPYQML_PROXY_DECL(51);
QPYQML_PROXY_DECL(52);
QPYQML_PROXY_DECL(53);
QPYQML_PROXY_DECL(54);
QPYQML_PROXY_DECL(55);
QPYQML_PROXY_DECL(56);
QPYQML_PROXY_DECL(57);
QPYQML_PROXY_DECL(58);
QPYQML_PROXY_DECL(59);


#endif

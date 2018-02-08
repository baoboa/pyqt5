// This is the implementation of the QPyQmlObject classes.
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


#include "qpyqmlobject.h"

#include "sipAPIQtQml.h"


// Forward declarations.
static void bad_result(PyObject *res, const char *context);

// The list of registered Python types.
QList<PyTypeObject *> QPyQmlObjectProxy::pyqt_types;

// The set of proxies in existence.
QSet<QObject *> QPyQmlObjectProxy::proxies;


// The ctor.
QPyQmlObjectProxy::QPyQmlObjectProxy(QObject *parent) : QAbstractItemModel(parent),
        proxied_model(0), py_proxied(0)
{
    proxies.insert(this);
}


// The dtor.
QPyQmlObjectProxy::~QPyQmlObjectProxy()
{
    proxies.remove(this);

    SIP_BLOCK_THREADS
    Py_XDECREF(py_proxied);
    SIP_UNBLOCK_THREADS

    if (!proxied.isNull())
        delete proxied.data();
}


// Called when QML has connected to a signal of the proxied object.  Note that
// we also used to implement the corresponding disconnectNotify() to
// disconnect the signal.  However this resulted in deadlocks and shouldn't be
// necessary anyway because it was only happening when the object that made the
// connection was itself being destroyed.
void QPyQmlObjectProxy::connectNotify(const QMetaMethod &sig)
{
    QByteArray signal_sig = signalSignature(sig);

    // The signal has actually been connected to the proxy, so do the same from
    // the proxied object to the proxy.  Use Qt::UniqueConnection in case the
    // object (ie. model) is used in more than one view.
    QObject::connect(proxied, signal_sig.constData(), this,
            signal_sig.constData(), Qt::UniqueConnection);
}


// Return what SIGNAL() would return for a method.
QByteArray QPyQmlObjectProxy::signalSignature(const QMetaMethod &signal)
{
    QByteArray signal_sig(signal.methodSignature());
    signal_sig.prepend('2');

    return signal_sig;
}


// Delegate to the real object.
const QMetaObject *QPyQmlObjectProxy::metaObject() const
{
    return !proxied.isNull() ? proxied->metaObject() : QObject::metaObject();
}


// Delegate to the real object.
void *QPyQmlObjectProxy::qt_metacast(const char *_clname)
{
    return !proxied.isNull() ? proxied->qt_metacast(_clname) : 0;
}


// Delegate to the real object.
int QPyQmlObjectProxy::qt_metacall(QMetaObject::Call call, int idx, void **args)
{
    if (idx < 0)
        return idx;

    if (proxied.isNull())
        return QObject::qt_metacall(call, idx, args);

    const QMetaObject *proxied_mo = proxied->metaObject();

    // See if a signal defined in the proxied object might be being invoked.
    // Note that we used to use sender() but this proved unreliable.
    if (call == QMetaObject::InvokeMetaMethod && proxied_mo->method(idx).methodType() == QMetaMethod::Signal)
    {
        // Get the meta-object of the class that defines the signal.
        while (idx < proxied_mo->methodOffset())
        {
            proxied_mo = proxied_mo->superClass();
            Q_ASSERT(proxied_mo);
        }

        // Relay the signal to QML.
        QMetaObject::activate(this, proxied_mo,
                idx - proxied_mo->methodOffset(), args);

        return idx - (proxied_mo->methodCount() - proxied_mo->methodOffset());
    }

    return proxied->qt_metacall(call, idx, args);
}


// Add a new Python type and return its number.
int QPyQmlObjectProxy::addType(PyTypeObject *type)
{
    pyqt_types.append(type);

    return pyqt_types.size() - 1;
}


// Create the Python instance.
void QPyQmlObjectProxy::createPyObject(QObject *parent)
{
    SIP_BLOCK_THREADS

    py_proxied = sipCallMethod(NULL, (PyObject *)pyqt_types.at(typeNr()), "D",
            parent, sipType_QObject, NULL);

    if (py_proxied)
    {
        proxied = reinterpret_cast<QObject *>(
                sipGetAddress((sipSimpleWrapper *)py_proxied));

        proxied_model = qobject_cast<QAbstractItemModel *>(proxied.data());
    }
    else
    {
        pyqt5_qtqml_err_print();
    }

    SIP_UNBLOCK_THREADS
}


// Resolve any proxy.
void *QPyQmlObjectProxy::resolveProxy(void *proxy)
{
    QObject *qobj = reinterpret_cast<QObject *>(proxy);

    // We have to search for proxy instances because we have subverted the
    // usual sub-class detection mechanism.
    if (proxies.contains(qobj))
        return static_cast<QPyQmlObjectProxy *>(qobj)->proxied.data();

    // It's not a proxy.
    return proxy;
}


// Create an instance of the attached properties.
QObject *QPyQmlObjectProxy::createAttachedProperties(PyTypeObject *py_type,
        QObject *parent)
{
    QObject *qobj = 0;

    SIP_BLOCK_THREADS

    PyObject *obj = sipCallMethod(NULL, (PyObject *)py_type, "D", parent,
            sipType_QObject, NULL);

    if (obj)
    {
        qobj = reinterpret_cast<QObject *>(
                sipGetAddress((sipSimpleWrapper *)obj));

        // It should always have a parent, but just in case...
        if (parent)
            Py_DECREF(obj);
    }
    else
    {
        pyqt5_qtqml_err_print();
    }

    SIP_UNBLOCK_THREADS

    return qobj;
}


// Invoked when a class parse begins.
void QPyQmlObjectProxy::pyClassBegin()
{
    if (!py_proxied)
        return;

    SIP_BLOCK_THREADS

    bool ok = false;

    static PyObject *method_name = 0;

    if (!method_name)
#if PY_MAJOR_VERSION >= 3
        method_name = PyUnicode_FromString("classBegin");
#else
        method_name = PyString_FromString("classBegin");
#endif

    if (method_name)
    {
        PyObject *res = PyObject_CallMethodObjArgs(py_proxied, method_name,
                NULL);

        if (res)
        {
            if (res == Py_None)
                ok = true;
            else
                bad_result(res, "classBegin()");

            Py_DECREF(res);
        }
    }

    if (!ok)
        pyqt5_qtqml_err_print();

    SIP_UNBLOCK_THREADS
}


// Invoked when a component parse completes.
void QPyQmlObjectProxy::pyComponentComplete()
{
    if (!py_proxied)
        return;

    SIP_BLOCK_THREADS

    bool ok = false;

    static PyObject *method_name = 0;

    if (!method_name)
#if PY_MAJOR_VERSION >= 3
        method_name = PyUnicode_FromString("componentComplete");
#else
        method_name = PyString_FromString("componentComplete");
#endif

    if (method_name)
    {
        PyObject *res = PyObject_CallMethodObjArgs(py_proxied, method_name,
                NULL);

        if (res)
        {
            if (res == Py_None)
                ok = true;
            else
                bad_result(res, "componentComplete()");

            Py_DECREF(res);
        }
    }

    if (!ok)
        pyqt5_qtqml_err_print();

    SIP_UNBLOCK_THREADS
}


// Invoked to set the target property of a property value source.
void QPyQmlObjectProxy::pySetTarget(const QQmlProperty &target)
{
    if (!py_proxied)
        return;

    SIP_BLOCK_THREADS

    bool ok = false;

    static PyObject *method_name = 0;

    if (!method_name)
#if PY_MAJOR_VERSION >= 3
        method_name = PyUnicode_FromString("setTarget");
#else
        method_name = PyString_FromString("setTarget");
#endif

    if (method_name)
    {
        QQmlProperty *target_heap = new QQmlProperty(target);

        PyObject *py_target = sipConvertFromNewType(target_heap,
                sipType_QQmlProperty, 0);

        if (!py_target)
        {
            delete target_heap;
        }
        else
        {
            PyObject *res = PyObject_CallMethodObjArgs(py_proxied, method_name,
                    py_target, NULL);

            Py_DECREF(py_target);

            if (res)
            {
                if (res == Py_None)
                    ok = true;
                else
                    bad_result(res, "setTarget()");

                Py_DECREF(res);
            }
        }
    }

    if (!ok)
        pyqt5_qtqml_err_print();

    SIP_UNBLOCK_THREADS
}


// Raise an exception for an unexpected result.
static void bad_result(PyObject *res, const char *context)
{
#if PY_MAJOR_VERSION >= 3
    PyErr_Format(PyExc_TypeError, "unexpected result from %s: %S", context,
            res);
#else
    PyObject *res_s = PyObject_Str(res);

    if (res_s != NULL)
    {
        PyErr_Format(PyExc_TypeError, "unexpected result from %s: %s", context,
                PyString_AsString(res_s));

        Py_DECREF(res_s);
    }
#endif
}


// The proxy type implementations.
#define QPYQML_PROXY_IMPL(n) \
QPyQmlObject##n::QPyQmlObject##n(QObject *parent) : QPyQmlObjectProxy(parent) \
{ \
    createPyObject(parent); \
} \
QObject *QPyQmlObject##n::attachedProperties(QObject *parent) \
{ \
    return createAttachedProperties(attachedPyType, parent); \
} \
void QPyQmlObject##n::classBegin() \
{ \
    pyClassBegin(); \
} \
void QPyQmlObject##n::componentComplete() \
{ \
    pyComponentComplete(); \
} \
void QPyQmlObject##n::setTarget(const QQmlProperty &target) \
{ \
    pySetTarget(target); \
} \
QMetaObject QPyQmlObject##n::staticMetaObject; \
PyTypeObject *QPyQmlObject##n::attachedPyType


QPYQML_PROXY_IMPL(0);
QPYQML_PROXY_IMPL(1);
QPYQML_PROXY_IMPL(2);
QPYQML_PROXY_IMPL(3);
QPYQML_PROXY_IMPL(4);
QPYQML_PROXY_IMPL(5);
QPYQML_PROXY_IMPL(6);
QPYQML_PROXY_IMPL(7);
QPYQML_PROXY_IMPL(8);
QPYQML_PROXY_IMPL(9);
QPYQML_PROXY_IMPL(10);
QPYQML_PROXY_IMPL(11);
QPYQML_PROXY_IMPL(12);
QPYQML_PROXY_IMPL(13);
QPYQML_PROXY_IMPL(14);
QPYQML_PROXY_IMPL(15);
QPYQML_PROXY_IMPL(16);
QPYQML_PROXY_IMPL(17);
QPYQML_PROXY_IMPL(18);
QPYQML_PROXY_IMPL(19);
QPYQML_PROXY_IMPL(20);
QPYQML_PROXY_IMPL(21);
QPYQML_PROXY_IMPL(22);
QPYQML_PROXY_IMPL(23);
QPYQML_PROXY_IMPL(24);
QPYQML_PROXY_IMPL(25);
QPYQML_PROXY_IMPL(26);
QPYQML_PROXY_IMPL(27);
QPYQML_PROXY_IMPL(28);
QPYQML_PROXY_IMPL(29);
QPYQML_PROXY_IMPL(30);
QPYQML_PROXY_IMPL(31);
QPYQML_PROXY_IMPL(32);
QPYQML_PROXY_IMPL(33);
QPYQML_PROXY_IMPL(34);
QPYQML_PROXY_IMPL(35);
QPYQML_PROXY_IMPL(36);
QPYQML_PROXY_IMPL(37);
QPYQML_PROXY_IMPL(38);
QPYQML_PROXY_IMPL(39);
QPYQML_PROXY_IMPL(40);
QPYQML_PROXY_IMPL(41);
QPYQML_PROXY_IMPL(42);
QPYQML_PROXY_IMPL(43);
QPYQML_PROXY_IMPL(44);
QPYQML_PROXY_IMPL(45);
QPYQML_PROXY_IMPL(46);
QPYQML_PROXY_IMPL(47);
QPYQML_PROXY_IMPL(48);
QPYQML_PROXY_IMPL(49);
QPYQML_PROXY_IMPL(50);
QPYQML_PROXY_IMPL(51);
QPYQML_PROXY_IMPL(52);
QPYQML_PROXY_IMPL(53);
QPYQML_PROXY_IMPL(54);
QPYQML_PROXY_IMPL(55);
QPYQML_PROXY_IMPL(56);
QPYQML_PROXY_IMPL(57);
QPYQML_PROXY_IMPL(58);
QPYQML_PROXY_IMPL(59);


// The reimplementations of the QAbstractItemModel virtuals.

QModelIndex QPyQmlObjectProxy::index(int row, int column, const QModelIndex &parent) const
{
    if (!proxied.isNull() && proxied_model)
        return proxied_model->index(row, column, parent);

    return QModelIndex();
}

QModelIndex QPyQmlObjectProxy::parent(const QModelIndex &child) const
{
    if (!proxied.isNull() && proxied_model)
        return proxied_model->parent(child);

    return QModelIndex();
}

QModelIndex QPyQmlObjectProxy::sibling(int row, int column, const QModelIndex &idx) const
{
    if (!proxied.isNull() && proxied_model)
        return proxied_model->sibling(row, column, idx);

    return QModelIndex();
}

int QPyQmlObjectProxy::rowCount(const QModelIndex &parent) const
{
    if (!proxied.isNull() && proxied_model)
        return proxied_model->rowCount(parent);

    return 0;
}

int QPyQmlObjectProxy::columnCount(const QModelIndex &parent) const
{
    if (!proxied.isNull() && proxied_model)
        return proxied_model->columnCount(parent);

    return 0;
}

bool QPyQmlObjectProxy::hasChildren(const QModelIndex &parent) const
{
    if (!proxied.isNull() && proxied_model)
        return proxied_model->hasChildren(parent);

    return false;
}

QVariant QPyQmlObjectProxy::data(const QModelIndex &index, int role) const
{
    if (!proxied.isNull() && proxied_model)
        return proxied_model->data(index, role);

    return QVariant();
}

bool QPyQmlObjectProxy::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!proxied.isNull() && proxied_model)
        return proxied_model->setData(index, value, role);

    return false;
}

QVariant QPyQmlObjectProxy::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (!proxied.isNull() && proxied_model)
        return proxied_model->headerData(section, orientation, role);

    return QVariant();
}

bool QPyQmlObjectProxy::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
    if (!proxied.isNull() && proxied_model)
        return proxied_model->setHeaderData(section, orientation, value, role);

    return false;
}

QMap<int, QVariant> QPyQmlObjectProxy::itemData(const QModelIndex &index) const
{
    if (!proxied.isNull() && proxied_model)
        return proxied_model->itemData(index);

    return QMap<int, QVariant>();
}

bool QPyQmlObjectProxy::setItemData(const QModelIndex &index, const QMap<int, QVariant> &roles)
{
    if (!proxied.isNull() && proxied_model)
        return proxied_model->setItemData(index, roles);

    return false;
}

QStringList QPyQmlObjectProxy::mimeTypes() const
{
    if (!proxied.isNull() && proxied_model)
        return proxied_model->mimeTypes();

    return QStringList();
}

QMimeData *QPyQmlObjectProxy::mimeData(const QModelIndexList &indexes) const
{
    if (!proxied.isNull() && proxied_model)
        return proxied_model->mimeData(indexes);

    return 0;
}

bool QPyQmlObjectProxy::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
    if (!proxied.isNull() && proxied_model)
        return proxied_model->canDropMimeData(data, action, row, column, parent);

    return false;
}

bool QPyQmlObjectProxy::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if (!proxied.isNull() && proxied_model)
        return proxied_model->dropMimeData(data, action, row, column, parent);

    return false;
}

Qt::DropActions QPyQmlObjectProxy::supportedDropActions() const
{
    if (!proxied.isNull() && proxied_model)
        return proxied_model->supportedDropActions();

    return Qt::IgnoreAction;
}

Qt::DropActions QPyQmlObjectProxy::supportedDragActions() const
{
    if (!proxied.isNull() && proxied_model)
        return proxied_model->supportedDragActions();

    return Qt::IgnoreAction;
}

bool QPyQmlObjectProxy::insertRows(int row, int count, const QModelIndex &parent)
{
    if (!proxied.isNull() && proxied_model)
        return proxied_model->insertRows(row, count, parent);

    return false;
}

bool QPyQmlObjectProxy::insertColumns(int column, int count, const QModelIndex &parent)
{
    if (!proxied.isNull() && proxied_model)
        return proxied_model->insertColumns(column, count, parent);

    return false;
}

bool QPyQmlObjectProxy::removeRows(int row, int count, const QModelIndex &parent)
{
    if (!proxied.isNull() && proxied_model)
        return proxied_model->removeRows(row, count, parent);

    return false;
}

bool QPyQmlObjectProxy::removeColumns(int column, int count, const QModelIndex &parent)
{
    if (!proxied.isNull() && proxied_model)
        return proxied_model->removeColumns(column, count, parent);

    return false;
}

bool QPyQmlObjectProxy::moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationChild)
{
    if (!proxied.isNull() && proxied_model)
        return proxied_model->moveRows(sourceParent, sourceRow, count,
                destinationParent, destinationChild);

    return false;
}

bool QPyQmlObjectProxy::moveColumns(const QModelIndex &sourceParent, int sourceColumn, int count, const QModelIndex &destinationParent, int destinationChild)
{
    if (!proxied.isNull() && proxied_model)
        return proxied_model->moveColumns(sourceParent, sourceColumn, count,
                destinationParent, destinationChild);

    return false;
}

void QPyQmlObjectProxy::fetchMore(const QModelIndex &parent)
{
    if (!proxied.isNull() && proxied_model)
        proxied_model->fetchMore(parent);
}

bool QPyQmlObjectProxy::canFetchMore(const QModelIndex &parent) const
{
    if (!proxied.isNull() && proxied_model)
        return proxied_model->canFetchMore(parent);

    return false;
}

Qt::ItemFlags QPyQmlObjectProxy::flags(const QModelIndex &index) const
{
    if (!proxied.isNull() && proxied_model)
        return proxied_model->flags(index);

    return Qt::NoItemFlags;
}

void QPyQmlObjectProxy::sort(int column, Qt::SortOrder order)
{
    if (!proxied.isNull() && proxied_model)
        proxied_model->sort(column, order);
}

QModelIndex QPyQmlObjectProxy::buddy(const QModelIndex &index) const
{
    if (!proxied.isNull() && proxied_model)
        return proxied_model->buddy(index);

    return QModelIndex();
}

QModelIndexList QPyQmlObjectProxy::match(const QModelIndex &start, int role, const QVariant &value, int hits, Qt::MatchFlags flags) const
{
    if (!proxied.isNull() && proxied_model)
        return proxied_model->match(start, role, value, hits, flags);

    return QModelIndexList();
}

QSize QPyQmlObjectProxy::span(const QModelIndex &index) const
{
    if (!proxied.isNull() && proxied_model)
        return proxied_model->span(index);

    return QSize();
}

QHash<int, QByteArray> QPyQmlObjectProxy::roleNames() const
{
    if (!proxied.isNull() && proxied_model)
        return proxied_model->roleNames();

    return QHash<int, QByteArray>();
}

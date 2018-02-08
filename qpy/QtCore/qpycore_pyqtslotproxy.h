// This contains the definition of the PyQtSlotProxy class.
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


#ifndef _QPYCORE_PYQTSLOTPROXY_H
#define _QPYCORE_PYQTSLOTPROXY_H


#include <Python.h>

#include <QByteArray>
#include <QMultiHash>
#include <QMetaObject>
#include <QObject>

#include "qpycore_namespace.h"
#include "qpycore_chimera.h"


QT_BEGIN_NAMESPACE
class QMutex;
QT_END_NAMESPACE

class PyQtSlot;


// This class is implements a slot on behalf of Python callables.  It is
// derived from QObject but is not run through moc.  Instead the normal
// moc-generated methods are handwritten in order to implement a universal
// slot.  This requires some knowledge of the internal structure of QObject but
// it is likely that they will only change between major Qt versions.
class PyQtSlotProxy : public QObject
{
public:
    PyQtSlotProxy(PyObject *slot, QObject *transmitter,
            const Chimera::Signature *slot_signature, bool single_shot);
    ~PyQtSlotProxy();

    static const QMetaObject staticMetaObject;
    virtual const QMetaObject *metaObject() const;
    virtual void *qt_metacast(const char *);
    virtual int qt_metacall(QMetaObject::Call, int, void **);

    void unislot(void **qargs);
    void disable();

    static void deleteSlotProxies(const QObject *transmitter,
            const QByteArray &signal_signature);
    static PyQtSlotProxy *findSlotProxy(const QObject *transmitter,
            const QByteArray &signal_signature, PyObject *slot);

    void disableReceiverCheck();

    static int clearSlotProxies(const QObject *transmitter);
    static int visitSlotProxies(const QObject *transmitter, visitproc visit,
            void *arg);

    static QObject *lastSender();

    // The signature of the slot that receives connections.
    static const QByteArray proxy_slot_signature;

    // The mutex around the proxies hash.
    static QMutex *mutex;

private:
    // The last QObject sender.
    static QObject *last_sender;

    // The type of a proxy hash.
    typedef QMultiHash<const QObject *, PyQtSlotProxy *> ProxyHash;

    // The hash of proxies.
    static ProxyHash proxy_slots;

    // The proxy flags.
    int proxy_flags;

    // The normalised signature.
    QByteArray signature;

    // The QObject transmitter the proxy will be connected to (if any).
    const QObject *transmitter;

    // The slot we are proxying for.
    PyQtSlot *real_slot;

    // The meta-object.
    const QMetaObject *meta_object;

    PyQtSlotProxy(const PyQtSlotProxy &);
    PyQtSlotProxy &operator=(const PyQtSlotProxy &);
};


#endif

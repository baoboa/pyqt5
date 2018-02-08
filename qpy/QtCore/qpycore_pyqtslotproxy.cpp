// This contains the implementation of the PyQtSlotProxy class.
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

#include <QByteArray>
#include <QMetaObject>
#include <QMutex>
#include <QObject>

#include "qpycore_api.h"
#include "qpycore_chimera.h"
#include "qpycore_qmetaobjectbuilder.h"
#include "qpycore_pyqtslot.h"
#include "qpycore_pyqtslotproxy.h"

#include "sipAPIQtCore.h"


// Proxy flags.
#define PROXY_SINGLE_SHOT   0x01    // The slot will only be called once.
#define PROXY_SLOT_INVOKED  0x02    // The proxied slot is executing.
#define PROXY_SLOT_DISABLED 0x04    // The proxy deleteLater() has been called
                                    // or should be called after the slot has
                                    // finished executing.
#define PROXY_NO_RCVR_CHECK 0x08    // The existence of the receiver C++ object
                                    // should not be checked.


// The last QObject sender.
QObject *PyQtSlotProxy::last_sender = 0;


// Create a universal proxy used as a slot.  Note that this will leak if there
// is no transmitter and this is not a single shot slot.  There will be no
// meta-object if there was a problem creating the proxy.  This is called
// without the GIL.
PyQtSlotProxy::PyQtSlotProxy(PyObject *slot, QObject *q_tx,
        const Chimera::Signature *slot_signature, bool single_shot)
    : QObject(), proxy_flags(single_shot ? PROXY_SINGLE_SHOT : 0),
        signature(slot_signature->signature), transmitter(q_tx)
{
    SIP_BLOCK_THREADS
    real_slot = new PyQtSlot(slot, slot_signature);
    SIP_UNBLOCK_THREADS

    // Create a new meta-object on the heap so that it looks like it has a slot
    // of the right name and signature.
    QMetaObjectBuilder builder;

    builder.setClassName("PyQtSlotProxy");
    builder.setSuperClass(&QObject::staticMetaObject);

    builder.addSlot("unislot()");
    builder.addSlot("disable()");

    meta_object = builder.toMetaObject();

    // Detect when any transmitter is destroyed.  (Note that we used to do this
    // by making the proxy a child of the transmitter.  This doesn't work as
    // expected because QWidget destroys its children before emitting the
    // destroyed signal.)  We use a queued connection in case the proxy is also
    // connected to the same signal and we want to make sure it has a chance to
    // invoke the slot before being destroyed.
    if (transmitter)
    {
        // Add this one to the global hash.
        mutex->lock();
        proxy_slots.insert(transmitter, this);
        mutex->unlock();

        connect(transmitter, SIGNAL(destroyed(QObject *)), SLOT(disable()),
                Qt::QueuedConnection);
    }
}


// Destroy a universal proxy.  This is called without the GIL.
PyQtSlotProxy::~PyQtSlotProxy()
{
    Q_ASSERT((proxy_flags & PROXY_SLOT_INVOKED) == 0);

    if (transmitter)
    {
        mutex->lock();

        ProxyHash::iterator it(proxy_slots.find(transmitter));
        ProxyHash::iterator end(proxy_slots.end());

        while (it != end && it.key() == transmitter)
        {
            if (it.value() == this)
                it = proxy_slots.erase(it);
            else
                ++it;
        }

        mutex->unlock();
    }

    // Qt can still be tidying up after Python has gone so make sure that it
    // hasn't.
    if (Py_IsInitialized())
    {
        SIP_BLOCK_THREADS
        delete real_slot;
        SIP_UNBLOCK_THREADS
    }

    if (meta_object)
    {
        free(const_cast<QMetaObject *>(meta_object));
    }
}


// The static members of PyQtSlotProxy.
const QByteArray PyQtSlotProxy::proxy_slot_signature(SLOT(unislot()));
QMutex *PyQtSlotProxy::mutex;
PyQtSlotProxy::ProxyHash PyQtSlotProxy::proxy_slots;


const QMetaObject *PyQtSlotProxy::metaObject() const
{
    return meta_object;
}


void *PyQtSlotProxy::qt_metacast(const char *_clname)
{
    if (!_clname)
        return 0;

    if (qstrcmp(_clname, "PyQtSlotProxy") == 0)
        return static_cast<void *>(const_cast<PyQtSlotProxy *>(this));

    return QObject::qt_metacast(_clname);
}


int PyQtSlotProxy::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);

    if (_id < 0)
        return _id;

    if (_c == QMetaObject::InvokeMetaMethod)
    {
        switch (_id)
        {
        case 0:
            unislot(_a);
            break;

        case 1:
            disable();
            break;
        }

        _id -= 2;
    }

    return _id;
}


// This is the universal slot itself that dispatches to the real slot.
void PyQtSlotProxy::unislot(void **qargs)
{
    // If we are marked as disabled (possible if a queued signal has been
    // disconnected but there is still a signal in the event queue) then just
    // ignore the call.
    if (proxy_flags & PROXY_SLOT_DISABLED)
        return;

    // sender() must be called without the GIL to avoid possible deadlocks
    // between the GIL and Qt's internal thread data mutex.
    QObject *new_last_sender = sender();

    SIP_BLOCK_THREADS

    QObject *saved_last_sender = last_sender;
    last_sender = new_last_sender;

    proxy_flags |= PROXY_SLOT_INVOKED;

    switch (real_slot->invoke(qargs, (proxy_flags & PROXY_NO_RCVR_CHECK)))
    {
    case PyQtSlot::Succeeded:
        break;

    case PyQtSlot::Failed:
        pyqt5_err_print();
        break;

    case PyQtSlot::Ignored:
        proxy_flags |= PROXY_SLOT_DISABLED;
        break;
    }

    proxy_flags &= ~PROXY_SLOT_INVOKED;

    // Self destruct if we are a single shot or disabled.
    if (proxy_flags & (PROXY_SINGLE_SHOT|PROXY_SLOT_DISABLED))
    {
        // See the comment in disable() for why we deleteLater().
        deleteLater();
    }

    last_sender = saved_last_sender;

    SIP_UNBLOCK_THREADS
}


// Disable the slot by destroying it if possible, or delaying its destruction
// until the proxied slot returns.
void PyQtSlotProxy::disable()
{
    proxy_flags |= PROXY_SLOT_DISABLED;

    // Delete it if the slot isn't currently executing, otherwise it will be
    // done after the slot returns.  Note that we don't rely on deleteLater()
    // providing the necessary delay because the slot could process the event
    // loop and the proxy would be deleted too soon.
    if ((proxy_flags & PROXY_SLOT_INVOKED) == 0)
    {
        // Despite what the Qt documentation suggests, if there are outstanding
        // queued signals then we may crash so we always delete later when the
        // event queue will have been flushed.
        deleteLater();
    }
}


// Find a slot proxy connected to a transmitter.
PyQtSlotProxy *PyQtSlotProxy::findSlotProxy(const QObject *transmitter,
        const QByteArray &signal_signature, PyObject *slot)
{
    PyQtSlotProxy *proxy = 0;

    mutex->lock();

    ProxyHash::const_iterator it(proxy_slots.find(transmitter));
    ProxyHash::const_iterator end(proxy_slots.end());

    while (it != end && it.key() == transmitter)
    {
        PyQtSlotProxy *sp = it.value();

        if (!(sp->proxy_flags & PROXY_SLOT_DISABLED) && sp->signature == signal_signature && *(sp->real_slot) == slot)
        {
            proxy = sp;
            break;
        }

        ++it;
    }

    mutex->unlock();

    return proxy;
}


// Delete any slot proxies for a particular transmitter and optional signal
// signature.
void PyQtSlotProxy::deleteSlotProxies(const QObject *transmitter,
        const QByteArray &signal_signature)
{
    mutex->lock();

    ProxyHash::iterator it(proxy_slots.find(transmitter));
    ProxyHash::iterator end(proxy_slots.end());

    while (it != end && it.key() == transmitter)
    {
        PyQtSlotProxy *sp = it.value();

        if (signal_signature.isEmpty() || signal_signature == sp->signature)
        {
            it = proxy_slots.erase(it);
            sp->disable();
        }
        else
        {
            ++it;
        }
    }

    mutex->unlock();
}


// Clear the extra references of any slots connected to a transmitter.  This is
// called with the GIL.
int PyQtSlotProxy::clearSlotProxies(const QObject *transmitter)
{
    ProxyHash::iterator it(proxy_slots.find(transmitter));
    ProxyHash::iterator end(proxy_slots.end());

    while (it != end && it.key() == transmitter)
    {
        it.value()->real_slot->clearOther();

        ++it;
    }

    return 0;
}


// A thing wrapper available to the generated code.
int qpycore_clearSlotProxies(const QObject *transmitter)
{
    return PyQtSlotProxy::clearSlotProxies(transmitter);
}


// Visit the extra references of any slots connected to a transmitter.  This is
// called with the GIL.
int PyQtSlotProxy::visitSlotProxies(const QObject *transmitter,
        visitproc visit, void *arg)
{
    int vret = 0;

    ProxyHash::iterator it(proxy_slots.find(transmitter));
    ProxyHash::iterator end(proxy_slots.end());

    while (it != end && it.key() == transmitter)
    {
        if ((vret = it.value()->real_slot->visitOther(visit, arg)) != 0)
            break;

        ++it;
    }

    return vret;
}


// A thing wrapper available to the generated code.
int qpycore_visitSlotProxies(const QObject *transmitter, visitproc visit,
        void *arg)
{
    return PyQtSlotProxy::visitSlotProxies(transmitter, visit, arg);
}


// Get the last sender.
QObject *PyQtSlotProxy::lastSender()
{
    return last_sender;
}


// Disable the check that the receiver C++ object exists before invoking a
// slot.
void PyQtSlotProxy::disableReceiverCheck()
{
    proxy_flags |= PROXY_NO_RCVR_CHECK;
}

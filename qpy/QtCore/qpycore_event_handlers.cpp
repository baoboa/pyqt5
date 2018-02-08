// This is the implementation of the event handlers.
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


#include "qpycore_event_handlers.h"

#include "sipAPIQtCore.h"


// The singleton that monitors the destruction of QObject instances created by
// C++.
static PyQtMonitor *monitor = NULL;


// Forward declarations.
static void wrapped_instance_eh(void *cpp);
static void collecting_wrapper_eh(sipSimpleWrapper *self);


// Register the event handlers.
void qpycore_register_event_handlers()
{
    // Create the monitor.
    monitor = new PyQtMonitor;

    // Register the handlers.
    sipRegisterEventHandler(sipEventWrappedInstance, sipType_QObject,
            (void *)wrapped_instance_eh);
    sipRegisterEventHandler(sipEventCollectingWrapper, sipType_QObject,
            (void *)collecting_wrapper_eh);
}


// Invoked when a QObject that is created by C++ is wrapped.
static void wrapped_instance_eh(void *cpp)
{
    monitor->monitor(reinterpret_cast<QObject *>(cpp));
}


// Invoked when the Python wrapper of a QObject is garbage collected.
static void collecting_wrapper_eh(sipSimpleWrapper *self)
{
    monitor->ignore(self);
}


// Monitor a C++ created QObject instance.
void PyQtMonitor::monitor(QObject *cppInst)
{
    // Connect the monitor.
    monitored.insert(cppInst);

    // Note that the C++ instance may be in the process of being destroyed.
    // This will happen if it is the argument to the destroyed() signal.
    // Python will have forgotten about the object (even if it was created by
    // Python) so by the time it gets wrapped again to be passed to the slot
    // it will appear to have been created by C++ and will therefore be subject
    // to monitoring.  Note that subsequently calling disconnect() would cause
    // a crash - this is why we keep a separate record of C++ instances
    // currently being monitored and never explicitly disconnect the monitor.
    Py_BEGIN_ALLOW_THREADS
    connect(cppInst, SIGNAL(destroyed(QObject *)),
            SLOT(on_destroyed(QObject *)), Qt::UniqueConnection);
    Py_END_ALLOW_THREADS
}


// Stop monitoring a C++ created QObject instance.
void PyQtMonitor::ignore(sipSimpleWrapper *pyObj)
{
    void *addr = sipGetAddress(pyObj);

    if (addr)
        // Note the reason given above we do not disconnect the monitor.
        monitored.remove(reinterpret_cast<QObject *>(addr));
}


// Invoked when a monitored C++ created QObject instance is destroyed.
void PyQtMonitor::on_destroyed(QObject *cppInst)
{
    QSet<QObject *>::iterator it = monitored.find(cppInst);

    // See if we are currently monitoring this instance.
    if (it != monitored.end())
    {
        monitored.erase(it);

        if (sipGetInterpreter())
        {
            PyObject *pyObj = sipGetPyObject(cppInst, sipType_QObject);

            if (pyObj)
                sipInstanceDestroyed((sipSimpleWrapper *)pyObj);
        }
    }
}

// This implements the helper for QObject.__getattr__().
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
#include <QHash>
#include <QMetaMethod>
#include <QObject>

#include "qpycore_api.h"
#include "qpycore_pyqtboundsignal.h"
#include "qpycore_pyqtmethodproxy.h"
#include "qpycore_pyqtsignal.h"


// See if we can find an attribute in the Qt meta-type system.  This is
// primarily to support access to JavaScript (e.g. QQuickItem) so we don't
// support overloads.
PyObject *qpycore_qobject_getattr(const QObject *qobj, PyObject *py_qobj,
        const char *name)
{
    const QMetaObject *mo = qobj->metaObject();

    // Try and find a method with the name.
    QMetaMethod method;
    int method_index = -1;

    // Count down to allow overrides (assuming they are possible).
    for (int m = mo->methodCount() - 1; m >= 0; --m)
    {
        method = mo->method(m);

        if (method.methodType() == QMetaMethod::Constructor)
            continue;

        // Get the method name.
        QByteArray mname(method.methodSignature());
        int idx = mname.indexOf('(');

        if (idx >= 0)
            mname.truncate(idx);

        if (mname == name)
        {
            method_index = m;
            break;
        }
    }

    if (method_index >= 0)
    {
        // Get the value to return.  Note that this is recreated each time.  We
        // could put a descriptor in the type dictionary to satisfy the request
        // in future but the typical use case is getting a value from a C++
        // proxy (e.g. QDeclarativeItem) and we can't assume that what is being
        // proxied is the same each time.
        if (method.methodType() == QMetaMethod::Signal)
        {
            // We need to keep explicit references to the unbound signals
            // (because we don't use the type dictionary to do so) because they
            // own the parsed signature which may be needed by a PyQtSlotProxy
            // at some point.
            typedef QHash<QByteArray, PyObject *> SignalHash;

            static SignalHash *sig_hash = 0;

            // For crappy compilers.
            if (!sig_hash)
                sig_hash = new SignalHash;

            PyObject *sig_obj;

            QByteArray sig_str = method.methodSignature();

            SignalHash::const_iterator it = sig_hash->find(sig_str);

            if (it == sig_hash->end())
            {
                sig_obj = (PyObject *)qpycore_pyqtSignal_New(
                        sig_str.constData());

                if (!sig_obj)
                    return 0;

                sig_hash->insert(sig_str, sig_obj);
            }
            else
            {
                sig_obj = it.value();
            }

            return qpycore_pyqtBoundSignal_New((qpycore_pyqtSignal *)sig_obj,
                    py_qobj, const_cast<QObject *>(qobj));
        }

        // Respect the 'private' nature of __ names.
        if (name[0] != '_' || name[1] != '_')
        {
            QByteArray py_name(sipPyTypeName(Py_TYPE(py_qobj)));
            py_name.append('.');
            py_name.append(name);

            return qpycore_pyqtMethodProxy_New(const_cast<QObject *>(qobj),
                    method_index, py_name);
        }
    }

    // Replicate the standard Python exception.
    PyErr_Format(PyExc_AttributeError, "'%s' object has no attribute '%s'",
            sipPyTypeName(Py_TYPE(py_qobj)), name);

    return 0;
}

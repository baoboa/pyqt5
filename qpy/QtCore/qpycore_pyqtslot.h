// This defines the definition of the PyQtSlot class.
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


#ifndef _QPYCORE_PYQTSLOT_H
#define _QPYCORE_PYQTSLOT_H


#include <Python.h>

#include "qpycore_chimera.h"


class PyQtSlot
{
public:
    enum Result
    {
        Succeeded,  // The invocation was successful.
        Failed,     // The invocation failed.
        Ignored     // The invocation was ignore because the receiver has gone.
    };

    PyQtSlot(PyObject *method, PyObject *type,
            const Chimera::Signature *slot_signature);
    PyQtSlot(PyObject *callable, const Chimera::Signature *slot_signature);
    ~PyQtSlot();

    PyQtSlot::Result invoke(void **qargs, bool no_receiver_check) const;
    bool invoke(void **qargs, PyObject *self, void *result) const;
    const Chimera::Signature *slotSignature() const {return signature;}

    void clearOther();
    int visitOther(visitproc visit, void *arg);

    bool operator==(PyObject *callable) const;

private:
    PyQtSlot::Result invoke(void **qargs, PyObject *self, void *result,
            bool no_receiver_check) const;
    PyObject *call(PyObject *callable, PyObject *args) const;
    PyObject *instance() const;

    PyObject *mfunc;
    PyObject *mself;
#if PY_MAJOR_VERSION < 3
    PyObject *mclass;
#endif
    PyObject *mself_wr;
    PyObject *other;

    const Chimera::Signature *signature;

    // By forcing the use of pointers we don't need to worry about managing the
    // reference counts of the component Python objects.
    PyQtSlot(const PyQtSlot &);
    PyQtSlot &operator=(const PyQtSlot &);
};


#endif

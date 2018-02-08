// This contains the implementation of the Q_ENUMS and Q_FLAGS support.
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

#include <QMultiHash>

#include "qpycore_chimera.h"
#include "qpycore_enums_flags.h"

#include "sipAPIQtCore.h"


// Forward declarations.
static PyObject *parse_enums_flags(PyObject *args, bool flags);


// The enums and flags defined in each frame.
static QMultiHash<const struct _frame *, EnumsFlags> enums_flags_hash;


// Add the given Q_ENUMS() arguments to the current enums/flags hash.
PyObject *qpycore_Enums(PyObject *args)
{
    return parse_enums_flags(args, false);
}


// Add the given Q_FLAGS() arguments to the current enums/flags hash.
PyObject *qpycore_Flags(PyObject *args)
{
    return parse_enums_flags(args, true);
}


// Add the given Q_ENUMS() or Q_FLAGS() arguments to the current enums/flags
// hash.
static PyObject *parse_enums_flags(PyObject *args, bool flags)
{
    // We need the frame we were called from, not the current one.
    struct _frame *frame = sipGetFrame(1);

    if (!frame)
    {
        PyErr_SetString(PyExc_RuntimeError, "no previous frame");
        return 0;
    }

    // Each argument is a separate enum/flag.
    for (Py_ssize_t i = 0; i < PyTuple_Size(args); ++i)
    {
        PyObject *arg = PyTuple_GetItem(args, i);

        // Check the argument's type is type.
        if (!PyType_Check(arg))
        {
            PyErr_Format(PyExc_TypeError,
                    "arguments to %s() must be type objects",
                    (flags ? "Q_FLAGS" : "Q_ENUMS"));
            return 0;
        }

        // Create the basic enum/flag.
        EnumsFlags enums_flags(sipPyTypeName((PyTypeObject *)arg), flags);

        // Go through the type dictionary looking for int attributes.
        Py_ssize_t pos = 0;
        PyObject *key, *value, *dict;

        dict = sipPyTypeDict((PyTypeObject *)arg);

        while (PyDict_Next(dict, &pos, &key, &value))
        {
            PyErr_Clear();

            int i_value = sipLong_AsInt(value);

            if (PyErr_Occurred())
                continue;

            const char *s_key = sipString_AsASCIIString(&key);

            if (!s_key)
                continue;

            enums_flags.keys.insert(s_key, i_value);

            Py_DECREF(key);
        }

        enums_flags_hash.insert(frame, enums_flags);

        Chimera::registerPyEnum(arg);
    }

    // Make sure there are no exceptions left after failed value conversions.
    PyErr_Clear();

    Py_INCREF(Py_None);
    return Py_None;
}


// Return the current enums/flags list.
QList<EnumsFlags> qpycore_get_enums_flags_list()
{
    struct _frame *frame = sipGetFrame(0);
    QList<EnumsFlags> enums_flags_list = enums_flags_hash.values(frame);

    enums_flags_hash.remove(frame);

    return enums_flags_list;
}

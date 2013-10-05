// This is the support for QString.
//
// Copyright (c) 2013 Riverbank Computing Limited <info@riverbankcomputing.com>
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
#include <string.h>

#include <QString>
#include <QTextCodec>
#include <QVector>

#include "qpycore_sip.h"


// Work out if we should enable PEP 393 support.  This is complicated by the
// broken LLVM that XCode v4 installs.
#if PY_VERSION_HEX >= 0x03030000
#if defined(Q_OS_MAC)
#if !defined(__llvm__) || defined(__clang__)
// Python v3.3 on a Mac using either g++ or Clang, but not LLVM.
#define PYQT_PEP_393
#endif
#else
// Python v3.3 on a non-Mac.
#define PYQT_PEP_393
#endif
#endif


// Convert a QString to a Python Unicode object.
PyObject *qpycore_PyObject_FromQString(const QString &qstr)
{
    PyObject *obj;

#if defined(PYQT_PEP_393)
    obj = PyUnicode_FromKindAndData(PyUnicode_2BYTE_KIND, qstr.constData(),
            qstr.length());
#elif defined(Py_UNICODE_WIDE)
    QVector<uint> ucs4 = qstr.toUcs4();

    if ((obj = PyUnicode_FromUnicode(NULL, ucs4.size())) == NULL)
        return NULL;

    memcpy(PyUnicode_AS_UNICODE(obj), ucs4.constData(),
            ucs4.size() * sizeof (Py_UNICODE));
#else
    if ((obj = PyUnicode_FromUnicode(NULL, qstr.length())) == NULL)
        return NULL;

    memcpy(PyUnicode_AS_UNICODE(obj), qstr.utf16(),
            qstr.length() * sizeof (Py_UNICODE));
#endif

    return obj;
}


// Convert a Python Unicode object to a QString.
QString qpycore_PyObject_AsQString(PyObject *obj)
{
#if defined(PYQT_PEP_393)
    SIP_SSIZE_T len = PyUnicode_GET_LENGTH(obj);

    switch (PyUnicode_KIND(obj))
    {
    case PyUnicode_1BYTE_KIND:
        return QString::fromLatin1((char *)PyUnicode_1BYTE_DATA(obj), len);

    case PyUnicode_2BYTE_KIND:
        // The (QChar *) cast should be safe.
        return QString((QChar *)PyUnicode_2BYTE_DATA(obj), len);

    case PyUnicode_4BYTE_KIND:
        return QString::fromUcs4(PyUnicode_4BYTE_DATA(obj), len);
    }

    return QString();
#elif defined(Py_UNICODE_WIDE)
    return QString::fromUcs4((const uint *)PyUnicode_AS_UNICODE(obj),
            PyUnicode_GET_SIZE(obj));
#else
    return QString::fromUtf16((const ushort *)PyUnicode_AS_UNICODE(obj),
            PyUnicode_GET_SIZE(obj));
#endif
}

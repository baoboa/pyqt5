/*
 * This is the Qt Designer plugin that collects all the Python plugins it can
 * find as a widget collection to Designer.
 *
 * Copyright (c) 2018 Riverbank Computing Limited <info@riverbankcomputing.com>
 * 
 * This file is part of PyQt5.
 * 
 * This file may be used under the terms of the GNU General Public License
 * version 3.0 as published by the Free Software Foundation and appearing in
 * the file LICENSE included in the packaging of this file.  Please review the
 * following information to ensure the GNU General Public License version 3.0
 * requirements will be met: http://www.gnu.org/copyleft/gpl.html.
 * 
 * If you do not wish to use this file under the terms of the GPL version 3.0
 * then you may purchase a commercial license.  For more information contact
 * info@riverbankcomputing.com.
 * 
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */


#include "pluginloader.h"

#include <stdlib.h>

#include <QtGlobal>
#include <QtPlugin>
#include <QCoreApplication>
#include <QDir>
#include <QLibrary>
#include <QStringList>
#include <QVector>
#include <QtDesigner>

#include "../qpy/QtDesigner/qpydesignercustomwidgetplugin.h"


// Construct the collection of Python widgets.
PyCustomWidgets::PyCustomWidgets(QObject *parent) : QObject(parent),
        sys_path(0), sip_unwrapinstance(0), qtdesigner_custom(0)
{
    // Get the default list of directories to search.  These correspond to a
    // standard "python" subdirectory of all the places that Designer looks for
    // its own plugins.
    QStringList default_dirs;

    QStringList path_list = QCoreApplication::libraryPaths();
    foreach (const QString &path, path_list)
        default_dirs.append(path + QDir::separator() +
                QLatin1String("designer") + QDir::separator() +
                QLatin1String("python"));

    default_dirs.append(QDir::homePath() + QDir::separator() +
            QLatin1String(".designer") + QDir::separator() +
            QLatin1String("plugins") + QDir::separator() +
            QLatin1String("python"));

    // Get the list of directories to search.
    QStringList dirs;
    char *pyqt_path = getenv("PYQTDESIGNERPATH");

    if (pyqt_path)
    {
#if defined(Q_OS_WIN)
        QLatin1Char sep(';');
#else
        QLatin1Char sep(':');
#endif

        QStringList pyqt_dirs = QString::fromLatin1(pyqt_path).split(sep);

        for (QStringList::const_iterator it = pyqt_dirs.constBegin(); it != pyqt_dirs.constEnd(); ++it)
            if ((*it).isEmpty())
                dirs << default_dirs;
            else
                dirs.append(QDir(*it).canonicalPath());
    }
    else
        dirs = default_dirs;

    // Go through each directory.
    for (int i = 0; i < dirs.size(); ++i)
    {
        QString dir = dirs.at(i);

        // Get a list of all candidate plugin modules.  We sort by name to
        // provide control over the order they are imported.
        QStringList candidates = QDir(dir).entryList(QDir::Files, QDir::Name);
        QStringList plugins;

        for (int p = 0; p < candidates.size(); ++p)
        {
            QStringList parts = candidates.at(p).split('.');

            if (parts.size() != 2)
                continue;

            if (!parts.at(1).startsWith("py"))
                continue;

            const QString &plugin = parts.at(0);

            if (!plugin.endsWith("plugin"))
                continue;

            if (plugins.contains(plugin))
                continue;

            plugins.append(plugin);
        }

        // Skip if there is nothing of interest in this directory.
        if (!plugins.size())
            continue;

        // Make sure the interpreter is initialised.  Leave this as late as
        // possible.
        if (!Py_IsInitialized())
        {
            QLibrary library(PYTHON_LIB);

            library.setLoadHints(QLibrary::ExportExternalSymbolsHint);

            if (!library.load())
                return;

            Py_Initialize();
        }

        // Import the plugins with the GIL held.
#if defined(WITH_THREAD)
        PyGILState_STATE gil_state = PyGILState_Ensure();
#endif

        bool fatal = importPlugins(dir, plugins);

#if defined(WITH_THREAD)
        PyGILState_Release(gil_state);
#endif

        if (fatal)
            break;
    }
}


// Import the plugins from a directory.
bool PyCustomWidgets::importPlugins(const QString &dir, const QStringList &plugins)
{
    // Make sure we have sys.path.
    if (!sys_path)
    {
        sys_path = getModuleAttr("sys", "path");

        if (!sys_path)
            return true;
    }

    // Make sure we have sip.unwrapinstance.
    if (!sip_unwrapinstance)
    {
        sip_unwrapinstance = getModuleAttr("sip", "unwrapinstance");

        if (!sip_unwrapinstance)
            return true;
    }

    // Convert the directory to a Python object with native separators.
    QString native_dir = QDir::toNativeSeparators(dir);

#if PY_VERSION_HEX >= 0x03030000
    PyObject *dobj = PyUnicode_FromKindAndData(PyUnicode_2BYTE_KIND, native_dir.constData(), native_dir.length());

    if (!dobj)
    {
        PyErr_Print();
        return false;
    }
#elif PY_VERSION_HEX >= 0x03000000
#if defined(Py_UNICODE_WIDE)
    QVector<uint> ucs4 = native_dir.toUcs4();

    PyObject *dobj = PyUnicode_FromUnicode(0, ucs4.size());

    if (!dobj)
    {
        PyErr_Print();
        return false;
    }

    memcpy(PyUnicode_AS_UNICODE(dobj), ucs4.constData(),
            ucs4.size() * sizeof (Py_UNICODE));
#else
    PyObject *dobj = PyUnicode_FromUnicode(0, native_dir.length());

    if (!dobj)
    {
        PyErr_Print();
        return false;
    }

    memcpy(PyUnicode_AS_UNICODE(dobj), native_dir.utf16(),
            native_dir.length() * sizeof (Py_UNICODE));
#endif
#else
    PyObject *dobj = PyString_FromString(native_dir.toLatin1().constData());

    if (!dobj)
    {
        PyErr_Print();
        return false;
    }
#endif

    // Add the directory to sys.path.
    int rc = PyList_Append(sys_path, dobj);
    Py_DECREF(dobj);

    if (rc < 0)
    {
        PyErr_Print();
        return false;
    }

    // Import each plugin.
    for (int plug = 0; plug < plugins.size(); ++plug)
    {
        PyObject *plug_mod = PyImport_ImportModule(plugins.at(plug).toLatin1().data());

        if (!plug_mod)
        {
            PyErr_Print();
            continue;
        }

        // Make sure we have QPyDesignerCustomWidgetPlugin.  We make sure this
        // is after the import of the first plugin to allow that plugin to
        // change any API versions.
        if (!qtdesigner_custom)
        {
            qtdesigner_custom = getModuleAttr("PyQt5.QtDesigner", "QPyDesignerCustomWidgetPlugin");

            if (!qtdesigner_custom)
                return true;
        }

        // Go through the module looking for types that implement
        // QDesignerCustomWidgetInterface (ie. by deriving from
        // QPyDesignerCustomWidgetPlugin).
        PyObject *mod_dict = PyModule_GetDict(plug_mod);
        PyObject *key, *value;
        Py_ssize_t pos = 0;

        while (PyDict_Next(mod_dict, &pos, &key, &value))
        {
            if (!PyType_Check(value))
                continue;

            if (value == qtdesigner_custom)
                continue;

            if (!PyType_IsSubtype((PyTypeObject *)value, (PyTypeObject *)qtdesigner_custom))
                continue;

            // Create the plugin instance.  Note that we don't give it a
            // parent, which make things easier.  It also means that Python
            // owns the instance so we don't decrement the reference count so
            // that it doesn't get garbage collected.
            PyObject *plugobj = PyObject_CallObject(value, NULL);

            if (!plugobj)
            {
                PyErr_Print();
                continue;
            }

            // Get the address of the C++ instance.
            PyObject *plugaddr = PyObject_CallFunctionObjArgs(sip_unwrapinstance, plugobj, NULL);

            if (!plugaddr)
            {
                Py_DECREF(plugobj);
                PyErr_Print();
                continue;
            }

            void *addr = PyLong_AsVoidPtr(plugaddr);
            Py_DECREF(plugaddr);

            widgets.append(reinterpret_cast<QPyDesignerCustomWidgetPlugin *>(addr));
        }

        Py_DECREF(plug_mod);
    }

    // No fatal errors.
    return false;
}


// Return the list of custom widgets.
QList<QDesignerCustomWidgetInterface *> PyCustomWidgets::customWidgets() const
{
    return widgets;
}


// Return the named attribute object from the named module.
PyObject *PyCustomWidgets::getModuleAttr(const char *module, const char *attr)
{
    PyObject *mod = PyImport_ImportModule(module);

    if (!mod)
    {
        PyErr_Print();
        return 0;
    }

    PyObject *obj = PyObject_GetAttrString(mod, attr);

    Py_DECREF(mod);

    if (!obj)
    {
        PyErr_Print();
        return 0;
    }

    return obj;
}

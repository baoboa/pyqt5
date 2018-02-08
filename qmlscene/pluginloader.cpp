/*
 * This is the qmlscene plugin that collects all the Python plugins it can
 * find.
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


#include <stdlib.h>

#include <Python.h>

#include <QCoreApplication>
#include <QDir>
#include <QLibrary>
#include <QLibraryInfo>
#include <QVector>
#include <QQmlEngine>

#include "pluginloader.h"


// Construct the C++ plugin.
PyQt5QmlPlugin::PyQt5QmlPlugin(QObject *parent) : QQmlExtensionPlugin(parent),
        py_plugin_obj(0), sip(0)
{
    // Make sure the interpreter is initialised.
    if (!Py_IsInitialized())
    {
        QLibrary library(PYTHON_LIB);

        library.setLoadHints(QLibrary::ExportExternalSymbolsHint);

        if (library.load())
        {
            Py_Initialize();

            getSipAPI();

#ifdef WITH_THREAD
            // Make sure we don't have the GIL.
            PyEval_InitThreads();
            PyEval_SaveThread();
#endif
        }
    }
}


// Destroy the C++ plugin.
PyQt5QmlPlugin::~PyQt5QmlPlugin()
{
    if (Py_IsInitialized())
    {
#ifdef WITH_THREAD
        PyGILState_STATE gil = PyGILState_Ensure();
#endif

        Py_XDECREF(py_plugin_obj);

#ifdef WITH_THREAD
        PyGILState_Release(gil);
#endif
    }
}


// Initialize the QML engine.
void PyQt5QmlPlugin::initializeEngine(QQmlEngine *engine, const char *uri)
{
    if (!Py_IsInitialized() || !py_plugin_obj || !sip)
        return;

#ifdef WITH_THREAD
    PyGILState_STATE gil = PyGILState_Ensure();
#endif

    const sipTypeDef *td = sip->api_find_type("QQmlEngine");

    if (!td)
    {
        PyErr_SetString(PyExc_AttributeError,
                "unable to find type for QQmlEngine");
    }
    else
    {
        PyObject *engine_obj = sip->api_convert_from_type(engine, td, 0);

        if (!engine_obj)
        {
            td = 0;
        }
        else
        {
            PyObject *res_obj = PyObject_CallMethod(py_plugin_obj,
                    const_cast<char *>("initializeEngine"),
                    const_cast<char *>("Os"), engine_obj, uri);

            Py_DECREF(engine_obj);

            if (res_obj != Py_None)
            {
                if (res_obj)
#if PY_MAJOR_VERSION >= 3
                    PyErr_Format(PyExc_TypeError,
                            "unexpected result from initializeEngine(): %S",
                            res_obj);
#else
                {
                    PyObject *res_s = PyObject_Str(res_obj);

                    if (res_s != NULL)
                    {
                        PyErr_Format(PyExc_TypeError,
                                "unexpected result from initializeEngine(): %s",
                                PyString_AsString(res_s));

                        Py_DECREF(res_s);
                    }
                }
#endif

                td = 0;
            }

            Py_XDECREF(res_obj);
        }
    }

    if (!td)
        PyErr_Print();

#ifdef WITH_THREAD
    PyGILState_Release(gil);
#endif
}


// Register the plugin's types.
void PyQt5QmlPlugin::registerTypes(const char *uri)
{
    // Construct the import path that the engine will have used by default.
    // Unfortunately we won't know about any directories added by qmlscene's -I
    // flag.
    QStringList import_path;

    import_path << QCoreApplication::applicationDirPath();

    const char *env_path = getenv("QML2_IMPORT_PATH");

    if (env_path)
    {
#if defined(Q_OS_WIN)
        QLatin1Char sep(';');
#else
        QLatin1Char sep(':');
#endif

        QStringList env_dirs = QString::fromLatin1(env_path).split(sep, QString::SkipEmptyParts);

        foreach (QString dir_name, env_dirs)
            import_path << QDir(dir_name).canonicalPath();
    }

    import_path << QLibraryInfo::location(QLibraryInfo::Qml2ImportsPath);

    // Try and find the URI directory, check that it has a qmldir file and that
    // it has a file called *plugin.py.
    QString py_plugin;
    QString py_plugin_dir;

    foreach (QString dir_name, import_path)
    {
        dir_name.append('/');
        dir_name.append(uri);

        QDir dir(dir_name);

        if (!dir.exists() || !dir.exists("qmldir"))
            continue;

        QStringList candidates = dir.entryList(QDir::Files|QDir::Readable);

        foreach (QString py_plugin_file, candidates)
        {
            QStringList parts = py_plugin_file.split('.');

            if (parts.size() == 2 && parts.at(0).endsWith("plugin") && parts.at(1).startsWith("py"))
            {
                py_plugin = parts.at(0);
                break;
            }
        }

        if (!py_plugin.isEmpty())
        {
            py_plugin_dir = QDir::toNativeSeparators(dir.absolutePath());
            break;
        }
    }

    if (py_plugin.isEmpty())
        return;

#ifdef WITH_THREAD
    PyGILState_STATE gil = PyGILState_Ensure();
#endif

    if (!addToSysPath(py_plugin_dir) || !callRegisterTypes(py_plugin, uri))
        PyErr_Print();

#ifdef WITH_THREAD
    PyGILState_Release(gil);
#endif
}


// Call the registerTypes() function of a plugin.
bool PyQt5QmlPlugin::callRegisterTypes(const QString &py_plugin,
        const char *uri)
{
    // Import the plugin.
    PyObject *plugin_mod = PyImport_ImportModule(py_plugin.toLatin1().data());

    if (!plugin_mod)
        return false;

    // Get QQmlExtensionPlugin.  We do this after the import of the plugin to
    // allow it to fiddle with the context.
    PyObject *extension_plugin = getModuleAttr("PyQt5.QtQml",
            "QQmlExtensionPlugin");

    if (!extension_plugin)
    {
        Py_DECREF(plugin_mod);
        return false;
    }

    // Go through the module looking for a type that implements
    // QQmlExtensionPlugin.
    PyObject *mod_dict = PyModule_GetDict(plugin_mod);
    PyObject *key, *value, *plugin_type = 0;
    Py_ssize_t pos = 0;

    while (PyDict_Next(mod_dict, &pos, &key, &value))
        if (value != extension_plugin && PyType_Check(value) && PyType_IsSubtype((PyTypeObject *)value, (PyTypeObject *)extension_plugin))
        {
            plugin_type = value;
            break;
        }

    Py_DECREF(extension_plugin);

    if (!plugin_type)
    {
        PyErr_Format(PyExc_AttributeError,
                "%s does not contain an implementation of QQmlExtensionPlugin",
                py_plugin.toLatin1().constData());

        Py_DECREF(plugin_mod);
        return false;
    }

    // Create the plugin instance.
    PyObject *plugin_obj = PyObject_CallObject(plugin_type, NULL);

    Py_DECREF(plugin_mod);

    if (!plugin_obj)
        return false;

    // Call registerTypes().
    PyObject *res_obj = PyObject_CallMethod(plugin_obj,
            const_cast<char *>("registerTypes"), const_cast<char *>("s"), uri);

    if (res_obj != Py_None)
    {
        Py_DECREF(plugin_obj);

        if (res_obj)
        {
#if PY_MAJOR_VERSION >= 3
            PyErr_Format(PyExc_TypeError,
                    "unexpected result from registerTypes(): %S", res_obj);
#else
            PyObject *res_s = PyObject_Str(res_obj);

            if (res_s != NULL)
            {
                PyErr_Format(PyExc_TypeError,
                        "unexpected result from registerTypes(): %s",
                        PyString_AsString(res_s));

                Py_DECREF(res_s);
            }
#endif

            Py_DECREF(res_obj);
        }

        return false;
    }

    Py_DECREF(res_obj);

    py_plugin_obj = plugin_obj;

    return true;
}


// Add a directory to sys.path.
bool PyQt5QmlPlugin::addToSysPath(const QString &py_plugin_dir)
{
    PyObject *sys_path = getModuleAttr("sys", "path");

    if (!sys_path)
        return false;

#if PY_VERSION_HEX >= 0x03030000
    PyObject *plugin_dir_obj = PyUnicode_FromKindAndData(PyUnicode_2BYTE_KIND,
            py_plugin_dir.constData(), py_plugin_dir.length());

    if (!plugin_dir_obj)
    {
        Py_DECREF(sys_path);
        return false;
    }
#elif PY_VERSION_HEX >= 0x03000000
#if defined(Py_UNICODE_WIDE)
    QVector<uint> ucs4 = py_plugin_dir.toUcs4();

    PyObject *plugin_dir_obj = PyUnicode_FromUnicode(0, ucs4.size());

    if (!plugin_dir_obj)
    {
        Py_DECREF(sys_path);
        return false;
    }

    memcpy(PyUnicode_AS_UNICODE(plugin_dir_obj), ucs4.constData(),
            ucs4.size() * sizeof (Py_UNICODE));
#else
    PyObject *plugin_dir_obj = PyUnicode_FromUnicode(0,
            py_plugin_dir.length());

    if (!plugin_dir_obj)
    {
        Py_DECREF(sys_path);
        return false;
    }

    memcpy(PyUnicode_AS_UNICODE(plugin_dir_obj), py_plugin_dir.utf16(),
            py_plugin_dir.length() * sizeof (Py_UNICODE));
#endif
#else
    PyObject *plugin_dir_obj = PyString_FromString(
            py_plugin_dir.toLatin1().constData());

    if (!plugin_dir_obj)
    {
        Py_DECREF(sys_path);
        return false;
    }
#endif

    int rc = PyList_Append(sys_path, plugin_dir_obj);
    Py_DECREF(plugin_dir_obj);
    Py_DECREF(sys_path);

    if (rc < 0)
    {
        return false;
    }

    return true;
}


// Return the named attribute object from the named module.
PyObject *PyQt5QmlPlugin::getModuleAttr(const char *module, const char *attr)
{
    PyObject *mod = PyImport_ImportModule(module);

    if (!mod)
        return 0;

    PyObject *obj = PyObject_GetAttrString(mod, attr);

    Py_DECREF(mod);

    return obj;
}


// Get the SIP API.
void PyQt5QmlPlugin::getSipAPI()
{
#if defined(SIP_USE_PYCAPSULE)
    sip = (const sipAPIDef *)PyCapsule_Import("sip._C_API", 0);
#else
    PyObject *c_api = getModuleAttr("sip", "_C_API");

    if (c_api)
    {
        sip = (const sipAPIDef *)PyCObject_AsVoidPtr(c_api);
        Py_DECREF(c_api);
    }
#endif

    if (!sip)
        PyErr_Print();
}

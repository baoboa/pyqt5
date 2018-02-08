/*
 * This is the interface of the qmlscene plugin.
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


#ifndef _PLUGINLOADER_H
#define _PLUGINLOADER_H


#include <Python.h>
#include <sip.h>

#include <QObject>
#include <QQmlExtensionPlugin>


class PyQt5QmlPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface")

public:
    PyQt5QmlPlugin(QObject *parent = 0);
    virtual ~PyQt5QmlPlugin();

    void initializeEngine(QQmlEngine *engine, const char *uri);
    void registerTypes(const char *uri);

private:
    void getSipAPI();
    static bool addToSysPath(const QString &py_plugin_dir);
    bool callRegisterTypes(const QString &py_plugin, const char *uri);
    static PyObject *getModuleAttr(const char *module, const char *attr);

    PyObject *py_plugin_obj;
    const sipAPIDef *sip;
};


#endif

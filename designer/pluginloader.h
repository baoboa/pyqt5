/*
 * This is the interface of the Qt Designer plugin.
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

#include <QtDesigner>
#include <QList>
#include <QObject>


QT_BEGIN_NAMESPACE
class QDesignerCustomWidgetInterface;
QT_END_NAMESPACE


class PyCustomWidgets
    : public QObject, public QDesignerCustomWidgetCollectionInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QDesignerCustomWidgetCollectionInterface")
    Q_INTERFACES(QDesignerCustomWidgetCollectionInterface)

public:
    PyCustomWidgets(QObject *parent = 0);

    virtual QList<QDesignerCustomWidgetInterface *> customWidgets() const;

private:
    bool importPlugins(const QString &dir, const QStringList &plugins);
    static PyObject *getModuleAttr(const char *module, const char *attr);

    // The sys.path object if we need it.
    PyObject *sys_path;

    // The sip.unwrapinstance object if we need it.
    PyObject *sip_unwrapinstance;

    // The PyQt5.QtDesigner.QPyDesignerCustomWidgetPlugin object if we need it.
    PyObject *qtdesigner_custom;

    QList<QDesignerCustomWidgetInterface *> widgets;
};

#endif

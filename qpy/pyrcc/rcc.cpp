/****************************************************************************
**
** Copyright (c) 2017 Riverbank Computing Limited. All rights reserved.
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qfile.h>
#include <qtextstream.h>
#include <qbytearray.h>
#include <qdir.h>
#include <qstack.h>
#include <qdom.h>
#include <qdatetime.h>

#include "rcc.h"


static void qt_rcc_write_number(FILE *out, quint64 number, int width)
{
    // Write <width> bytes
    while (width-- > 0)
    {
        const quint8 tmp = number >> (width * 8);
        fprintf(out, "\\x%02x", tmp);
    }
}

void RCCFileInfo::writeDataInfo(FILE *out, int version)
{
    //pointer data
    if(flags & RCCFileInfo::Directory) {
        //name offset
        qt_rcc_write_number(out, nameOffset, 4);

        //flags
        qt_rcc_write_number(out, flags, 2);

        //child count
        qt_rcc_write_number(out, children.size(), 4);

        //first child offset
        qt_rcc_write_number(out, childOffset, 4);
    } else {
        //name offset
        qt_rcc_write_number(out, nameOffset, 4);

        //flags
        qt_rcc_write_number(out, flags, 2);

        //locale
        qt_rcc_write_number(out, locale.country(), 2);
        qt_rcc_write_number(out, locale.language(), 2);

        //data offset
        qt_rcc_write_number(out, dataOffset, 4);
    }

    fprintf(out, "\\\n");

    if (version >= 2)
    {
        const QDateTime lastModified = fileInfo.lastModified();

        qt_rcc_write_number(out,
                lastModified.isValid() ? lastModified.toMSecsSinceEpoch() : 0,
                8);

        fprintf(out, "\\\n");
    }
}

qint64 RCCFileInfo::writeDataBlob(FILE *out, qint64 offset)
{
    //capture the offset
    dataOffset = offset;

    //find the data to be written
    QFile file(fileInfo.absoluteFilePath());
    if (!file.open(QFile::ReadOnly)) {
        fprintf(stderr, "Couldn't open %s\n", fileInfo.absoluteFilePath().toLatin1().constData());
        return false;
    }
    QByteArray data = file.readAll();

#ifndef QT_NO_COMPRESS
    // Check if compression is useful for this file
    if (mCompressLevel != 0 && data.size() != 0) {
        QByteArray compressed = qCompress(reinterpret_cast<uchar *>(data.data()), data.size(), mCompressLevel);

        int compressRatio = int(100.0f * (float(data.size() - compressed.size()) / float(data.size())));
        if (compressRatio >= mCompressThreshold) {
            data = compressed;
            flags |= Compressed;
        }
    }
#endif // QT_NO_COMPRESS
    
    //write the length
    qt_rcc_write_number(out, data.size(), 4);
    fprintf(out, "\\\n");
    offset += 4;

    //write the payload
    for (int i=0; i<data.size(); i++) {
        qt_rcc_write_number(out, data.at(i), 1);
        if(!(i % 16))
            fprintf(out, "\\\n");
    }
    offset += data.size();

    //done
    fprintf(out, "\\\n");
    return offset;
}

qint64 RCCFileInfo::writeDataName(FILE *out, qint64 offset)
{
    //capture the offset
    nameOffset = offset;

    //write the length
    qt_rcc_write_number(out, name.length(), 2);
    fprintf(out, "\\\n");
    offset += 2;

    //write the hash
    qt_rcc_write_number(out, qt_hash(name), 4);
    fprintf(out, "\\\n");
    offset += 4;

    //write the name
    const QChar *unicode = name.unicode();
    for (int i=0; i<name.length(); i++) {
        qt_rcc_write_number(out, unicode[i].unicode(), 2);
        if(!(i % 16))
            fprintf(out, "\\\n");
    }
    offset += name.length()*2;

    //done
    fprintf(out, "\\\n");
    return offset;
}

RCCResourceLibrary::~RCCResourceLibrary()
{
    delete root;
}

bool RCCResourceLibrary::interpretResourceFile(QIODevice *inputDevice, QString fname, QString currentPath)
{
    if (!currentPath.isEmpty() && !currentPath.endsWith(QLatin1String("/")))
        currentPath += '/';

    QDomDocument document;
    {
        QString errorMsg;
        int errorLine, errorColumn;
        if(!document.setContent(inputDevice, &errorMsg, &errorLine, &errorColumn)) {
            fprintf(stderr, "pyrcc5 Parse Error:%s:%d:%d [%s]\n", fname.toLatin1().constData(),
                    errorLine, errorColumn, errorMsg.toLatin1().constData());
            return false;
        }
    }
    for(QDomElement root = document.firstChild().toElement(); !root.isNull();
        root = root.nextSibling().toElement()) {
        if (root.tagName() != QLatin1String(TAG_RCC))
            continue;

        for (QDomElement child = root.firstChild().toElement(); !child.isNull();
             child = child.nextSibling().toElement()) {
            if (child.tagName() == QLatin1String(TAG_RESOURCE)) {
                QLocale lang = QLocale::c();
                if (child.hasAttribute(ATTRIBUTE_LANG))
                    lang = QLocale(child.attribute(ATTRIBUTE_LANG));

                QString prefix;
                if (child.hasAttribute(ATTRIBUTE_PREFIX))
                    prefix = child.attribute(ATTRIBUTE_PREFIX);
                if (!prefix.startsWith(QLatin1String("/")))
                    prefix.prepend('/');
                if (!prefix.endsWith(QLatin1String("/")))
                    prefix += '/';


                for (QDomNode res = child.firstChild(); !res.isNull(); res = res.nextSibling()) {
                    if (res.toElement().tagName() == QLatin1String(TAG_FILE)) {

                        QString fileName(res.firstChild().toText().data());
                        if (fileName.isEmpty())
                            fprintf(stderr, "Warning: Null node in XML\n");

                        QString alias;
                        if (res.toElement().hasAttribute(ATTRIBUTE_ALIAS))
                            alias = res.toElement().attribute(ATTRIBUTE_ALIAS);
                        else
                            alias = fileName;

                        int compressLevel = mCompressLevel;
                        if (res.toElement().hasAttribute(ATTRIBUTE_COMPRESS))
                            compressLevel = res.toElement().attribute(ATTRIBUTE_COMPRESS).toInt();
                        int compressThreshold = mCompressThreshold;
                        if (res.toElement().hasAttribute(ATTRIBUTE_THRESHOLD))
                            compressThreshold = res.toElement().attribute(ATTRIBUTE_THRESHOLD).toInt();

                        // Special case for -no-compress. Overrides all other settings.
                        if (mCompressLevel == -2)
                            compressLevel = 0;

                        alias = QDir::cleanPath(alias);
                        while (alias.startsWith("../"))
                            alias.remove(0, 3);
                        alias = prefix + alias;

                        QFileInfo file(currentPath + fileName);
                        if (!file.exists()) {
                            fprintf(stderr, "Cannot find file: %s\n", fileName.toLatin1().constData());
                            continue ;
                        } else if (file.isFile()) {
                            addFile(alias, RCCFileInfo(alias.section('/', -1), file, lang,
                                                       RCCFileInfo::NoFlags, compressLevel, compressThreshold));
                        } else {
                            QDir dir;
                            if(file.isDir()) {
                                dir.setPath(file.filePath());
                            } else {
                                dir.setPath(file.path());
                                dir.setNameFilters(QStringList(file.fileName()));
                                if(alias.endsWith(file.fileName()))
                                    alias = alias.left(alias.length()-file.fileName().length());
                            }
                            if (!alias.endsWith(QLatin1String("/")))
                                alias += '/';
                            QFileInfoList children = dir.entryInfoList();
                            for(int i = 0; i < children.size(); ++i) {
                                if(children[i].fileName() != QLatin1String(".") &&
                                   children[i].fileName() != QLatin1String(".."))
                                    addFile(alias + children[i].fileName(),
                                            RCCFileInfo(children[i].fileName(), children[i], lang,
                                                        RCCFileInfo::NoFlags, compressLevel, compressThreshold));
                            }
                        }
                    }
                }
            }
        }
    }
    if(this->root == 0) {
        fprintf(stderr, "No resources in resource description.\n");
        return false;
    }
    return true;
}

bool RCCResourceLibrary::addFile(const QString &alias, const RCCFileInfo &file)
{
    if (file.fileInfo.size() > 0xffffffff) {
        fprintf(stderr, "File too big: %s",
                file.fileInfo.absoluteFilePath().toLatin1().constData());
        return false;
    }
    if(!root)
        root = new RCCFileInfo("", QFileInfo(), QLocale(), RCCFileInfo::Directory);

    RCCFileInfo *parent = root;
    const QStringList nodes = alias.split('/');
    for(int i = 1; i < nodes.size()-1; ++i) {
        const QString node = nodes.at(i);
        if(!parent->children.contains(node)) {
            RCCFileInfo *s = new RCCFileInfo(node, QFileInfo(), QLocale(), RCCFileInfo::Directory);
            s->parent = parent;
            parent->children.insert(node, s);
            parent = s;
        } else {
            parent = parent->children[node];
        }
    }

    const QString filename = nodes.at(nodes.size()-1);
    RCCFileInfo *s = new RCCFileInfo(file);
    s->parent = parent;
    parent->children.insertMulti(filename, s);
    return true;
}

bool RCCResourceLibrary::readFiles()
{
    //read in data
    if (mVerbose)
        fprintf(stderr, "Processing %d files\n", mFileNames.size());
    for (int i=0; i<mFileNames.size(); ++i) {
        QFile fileIn;
        QString fname = mFileNames.at(i), pwd;
        if(fname == "-") {
            fname = "(stdin)";
            pwd = QDir::currentPath();
            fileIn.setFileName(fname);
            if (!fileIn.open(stdin, QIODevice::ReadOnly)) {
                fprintf(stderr, "Unable to open file: %s\n", fname.toLatin1().constData());
                return false;
            }
        } else {
            pwd = QFileInfo(fname).path();
            fileIn.setFileName(fname);
            if (!fileIn.open(QIODevice::ReadOnly)) {
                fprintf(stderr, "Unable to open file: %s\n", fname.toLatin1().constData());
                return false;
            }
        }
        if (mVerbose)
            fprintf(stderr, "Interpreting %s\n", fname.toLatin1().constData());

        if (!interpretResourceFile(&fileIn, fname, pwd))
            return false;
    }
    return true;
}

QStringList
RCCResourceLibrary::dataFiles() const
{
    QStringList ret;
    QStack<RCCFileInfo*> pending;

    if (!root)
        return ret;

    pending.push(root);
    while(!pending.isEmpty()) {
        RCCFileInfo *file = pending.pop();
        for(QHash<QString, RCCFileInfo*>::iterator it = file->children.begin();
            it != file->children.end(); ++it) {
            RCCFileInfo *child = it.value();
            if(child->flags & RCCFileInfo::Directory)
                pending.push(child);
            ret.append(child->fileInfo.filePath());
        }
    }
    return ret;
}

bool RCCResourceLibrary::output(const QString &out_name)
{
    FILE *out;

    // Create the output file or use stdout if not specified.
    if (out_name.isEmpty())
    {
        out = stdout;
    }
#if defined(_MSC_VER) && _MSC_VER >= 1400
    else if (fopen_s(&out, out_name.toLocal8Bit().constData(), "w"))
#else
    else if ((out = fopen(out_name.toLocal8Bit().constData(), "w")) == NULL)
#endif
    {
        fprintf(stderr, "Unable to open %s for writing\n",
                out_name.toLatin1().constData());
        return false;
    }

    if (mVerbose)
        fprintf(stderr, "Outputting code\n");

    const char *error;

    if (!writeHeader(out))
        error = "header";
    else if (!writeDataBlobs(out))
        error = "data blob";
    else if (!writeDataNames(out))
        error = "file names";
    else if (!writeDataStructure(out, 1))
        error = "v1 data tree";
    else if (!writeDataStructure(out, 2))
        error = "v2 data tree";
    else if (!writeInitializer(out))
        error = "footer";
    else
        error = 0;

    if (out != stdout)
        fclose(out);

    if (error)
    {
        fprintf(stderr, "Couldn't write %s\n", error);
        return false;
    }

    return true;
}

bool
RCCResourceLibrary::writeHeader(FILE *out)
{
    fprintf(out, "# -*- coding: utf-8 -*-\n\n");
    fprintf(out, "# Resource object code\n");
    fprintf(out, "#\n");
    fprintf(out, "# Created by: The Resource Compiler for PyQt5 (Qt v%s)\n", QT_VERSION_STR);
    fprintf(out, "#\n");
    fprintf(out, "# WARNING! All changes made in this file will be lost!\n");
    fprintf(out, "\n");
    fprintf(out, "from PyQt5 import QtCore\n\n");

    return true;
}

bool
RCCResourceLibrary::writeDataBlobs(FILE *out)
{
    fprintf(out, "qt_resource_data = b\"\\\n");
    QStack<RCCFileInfo*> pending;

    if (!root)
        return false;

    pending.push(root);
    qint64 offset = 0;
    while(!pending.isEmpty()) {
        RCCFileInfo *file = pending.pop();
        for(QHash<QString, RCCFileInfo*>::iterator it = file->children.begin();
            it != file->children.end(); ++it) {
            RCCFileInfo *child = it.value();
            if(child->flags & RCCFileInfo::Directory)
                pending.push(child);
            else
                offset = child->writeDataBlob(out, offset);
        }
    }
    fprintf(out, "\"\n\n");
    return true;
}

bool
RCCResourceLibrary::writeDataNames(FILE *out)
{
    fprintf(out, "qt_resource_name = b\"\\\n");

    QHash<QString, int> names;
    QStack<RCCFileInfo*> pending;

    if (!root)
        return false;

    pending.push(root);
    qint64 offset = 0;
    while(!pending.isEmpty()) {
        RCCFileInfo *file = pending.pop();
        for(QHash<QString, RCCFileInfo*>::iterator it = file->children.begin();
            it != file->children.end(); ++it) {
            RCCFileInfo *child = it.value();
            if(child->flags & RCCFileInfo::Directory)
                pending.push(child);
            if(names.contains(child->name)) {
                child->nameOffset = names.value(child->name);
            } else {
                names.insert(child->name, offset);
                offset = child->writeDataName(out, offset);
            }
        }
    }
    fprintf(out, "\"\n\n");
    return true;
}

static bool qt_rcc_compare_hash(const RCCFileInfo *left, const RCCFileInfo *right)
{
    return qt_hash(left->name) < qt_hash(right->name);
}

bool RCCResourceLibrary::writeDataStructure(FILE *out, int version)
{
    fprintf(out, "qt_resource_struct_v%d = b\"\\\n", version);
    QStack<RCCFileInfo*> pending;

    if (!root)
        return false;

    //calculate the child offsets (flat)
    pending.push(root);
    int offset = 1;
    while(!pending.isEmpty()) {
        RCCFileInfo *file = pending.pop();
        file->childOffset = offset;

        //sort by hash value for binary lookup
        QList<RCCFileInfo*> children = file->children.values();
        qSort(children.begin(), children.end(), qt_rcc_compare_hash);

        //write out the actual data now
        for(int i = 0; i < children.size(); ++i) {
            RCCFileInfo *child = children.at(i);
            ++offset;
            if(child->flags & RCCFileInfo::Directory)
                pending.push(child);
        }
    }

    //write out the structure (ie iterate again!)
    pending.push(root);
    root->writeDataInfo(out, version);
    while(!pending.isEmpty()) {
        RCCFileInfo *file = pending.pop();

        //sort by hash value for binary lookup
        QList<RCCFileInfo*> children = file->children.values();
        qSort(children.begin(), children.end(), qt_rcc_compare_hash);

        //write out the actual data now
        for(int i = 0; i < children.size(); ++i) {
            RCCFileInfo *child = children.at(i);
            child->writeDataInfo(out, version);
            if(child->flags & RCCFileInfo::Directory)
                pending.push(child);
        }
    }
    fprintf(out, "\"\n\n");

    return true;
}

bool
RCCResourceLibrary::writeInitializer(FILE *out)
{
    fprintf(out, "qt_version = QtCore.qVersion().split('.')\n");
    fprintf(out, "if qt_version < ['5', '8', '0']:\n");
    fprintf(out, "    rcc_version = 1\n");
    fprintf(out, "    qt_resource_struct = qt_resource_struct_v1\n");
    fprintf(out, "else:\n");
    fprintf(out, "    rcc_version = 2\n");
    fprintf(out, "    qt_resource_struct = qt_resource_struct_v2\n");

    fprintf(out, "\n");

    fprintf(out, "def qInitResources():\n");
    fprintf(out, "    QtCore.qRegisterResourceData(rcc_version, qt_resource_struct, qt_resource_name, qt_resource_data)\n");

    fprintf(out, "\n");

    fprintf(out, "def qCleanupResources():\n");
    fprintf(out, "    QtCore.qUnregisterResourceData(rcc_version, qt_resource_struct, qt_resource_name, qt_resource_data)\n");

    fprintf(out, "\n");

    fprintf(out, "qInitResources()\n");

    return true;
}

/****************************************************************************
**
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

#include <qstring.h>
#include <qstringlist.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qlocale.h>
#include <qtextstream.h>
#include <qbytearray.h>
#include <qhash.h>
#include <qdir.h>
#include "rcc.h"

// Some static globals
static QString initName;
static bool verbose = false;
static int compressLevel = CONSTANT_COMPRESSLEVEL_DEFAULT;
static int compressThreshold = CONSTANT_COMPRESSTHRESHOLD_DEFAULT;
static QString resourceRoot;

bool processResourceFile(const QStringList &filenamesIn, const QString &filenameOut, bool list)
{
    if (verbose)
        fprintf(stderr, "PyQt4 resource compiler\n");

    //setup
    RCCResourceLibrary library;
    library.setInputFiles(filenamesIn);
    library.setInitName(initName);
    library.setVerbose(verbose);
    library.setCompressLevel(compressLevel);
    library.setCompressThreshold(compressThreshold);
    library.setResourceRoot(resourceRoot);
    if(!library.readFiles())
        return false;

    //open output
    FILE *out_fd = stdout;
    if (!filenameOut.isEmpty() && filenameOut != QLatin1String("-")) {
#if defined(_MSC_VER) && _MSC_VER >= 1400
		if (fopen_s(&out_fd, filenameOut.toLocal8Bit().constData(), "w")) {
#else
        out_fd = fopen(filenameOut.toLocal8Bit().constData(), "w");
        if(!out_fd) {
#endif
            fprintf(stderr, "Unable to open %s for writing\n", filenameOut.toLatin1().constData());
            return false;
        }
    }

    //do the task
    bool ret = true;
    if(list) {
        const QStringList data = library.dataFiles();
        for(int i = 0; i < data.size(); ++i)
            fprintf(out_fd, "%s\n", QDir::cleanPath(data.at(i)).toLatin1().constData());
    } else {
        ret = library.output(out_fd);
    }
    if(out_fd != stdout)
        fclose(out_fd);

    //done
    return ret;
}

int showHelp(const char *argv0, const QString &error)
{
    fprintf(stderr, "PyQt5 resource compiler\n");
    if (!error.isEmpty())
        fprintf(stderr, "%s: %s\n", argv0, error.toLatin1().constData());
    fprintf(stderr, "Usage: %s  [options] <inputs>\n\n"
            "Options:\n"
            "    -o file           Write output to file rather than stdout\n"
            "    -name name        Create an external initialization function with name\n"
            "    -threshold level  Threshold to consider compressing files\n"
            "    -compress level   Compress input files by level\n"
            "    -root path        Prefix resource access path with root path\n"
            "    -no-compress      Disable all compression\n"
            "    -version          Display version\n"
            "    -help             Display this information\n",
            argv0);
    return 1;
}

int main(int argc, char *argv[])
{
    QString outFilename;
    bool helpRequested = false, list = false;
    QStringList files;

    //parse options
    QString errorMsg;
    for (int i = 1; i < argc && errorMsg.isEmpty(); i++) {
        if (argv[i][0] == '-') {   // option
            QByteArray opt = argv[i] + 1;
            if (opt == "o") {
                if (!(i < argc-1)) {
                    errorMsg = QLatin1String("Missing output name");
                    break;
                }
                outFilename = argv[++i];
            } else if(opt == "name") {
                if (!(i < argc-1)) {
                    errorMsg = QLatin1String("Missing target name");
                    break;
                }
                initName = argv[++i];
            } else if(opt == "root") {
                if (!(i < argc-1)) {
                    errorMsg = QLatin1String("Missing root path");
                    break;
                }
                resourceRoot = QDir::cleanPath(argv[++i]);
                if(resourceRoot.isEmpty() || resourceRoot[0] != '/')
                    errorMsg = QLatin1String("Root must start with a /");
            } else if(opt == "compress") {
                if (!(i < argc-1)) {
                    errorMsg = QLatin1String("Missing compression level");
                    break;
                }
                compressLevel = QString(argv[++i]).toInt();
            } else if(opt == "threshold") {
                if (!(i < argc-1)) {
                    errorMsg = QLatin1String("Missing compression threshold");
                    break;
                }
                compressThreshold = QString(argv[++i]).toInt();
            } else if(opt == "verbose") {
                verbose = true;
            } else if(opt == "list") {
                list = true;
            } else if(opt == "version") {
                fprintf(stderr, "Resource Compiler for Qt version %s\n", QT_VERSION_STR);
                return 1;
            } else if(opt == "help" || opt == "h") {
                helpRequested = true;
            } else if(opt == "no-compress") {
                compressLevel = -2;
            } else {
                errorMsg = QString(QLatin1String("Unknown option: '%1'")).arg(argv[i]);
            }
        } else {
            QString fname = QString::fromLocal8Bit(argv[i]);
            if(!QFile::exists(fname)) {
                qWarning("%s: File does not exist '%s'", argv[0], argv[i]);
                return 1;
            }
            files.append(fname);
        }
    }

    if (!files.size() || !errorMsg.isEmpty() || helpRequested)
        return showHelp(argv[0], errorMsg);
    return int(!processResourceFile(files, outFilename, list));
}

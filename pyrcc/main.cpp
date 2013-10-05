/****************************************************************************
**
** Copyright (C) 1992-2005 Trolltech AS. All rights reserved.
**
** This file is part of the tools applications of the Qt Toolkit.
**
** Licensees holding a valid Qt License Agreement may use this file in
** accordance with the rights, responsibilities and obligations
** contained therein.  Please consult your licensing agreement or
** contact sales@trolltech.com if any conditions of this licensing
** agreement are not clear to you.
**
** Further information about Qt licensing is available at:
** http://www.trolltech.com/products/qt/licensing.html or by
** contacting info@trolltech.com.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
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
            if(!QFile::exists(argv[i])) {
                qWarning("%s: File does not exist '%s'", argv[0], argv[i]);
                return 1;
            }
            files.append(argv[i]);
        }
    }

    if (!files.size() || !errorMsg.isEmpty() || helpRequested)
        return showHelp(argv[0], errorMsg);
    return int(!processResourceFile(files, outFilename, list));
}

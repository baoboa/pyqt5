/**********************************************************************
** Copyright (C) 2002-2007 Detlev Offenbach <detlev@die-offenbachs.de>
**
** This is a modified version of lupdate. The original is part of Qt-Linguist.
** The copyright of the original file can be found below.
**
** This version is modified to handle python sources.
**
**   The file is provided AS IS with NO WARRANTY OF ANY KIND,
**   INCLUDING THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR
**   A PARTICULAR PURPOSE.
****************************************************************************/

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

#include <metatranslator.h>
#include <proparser.h>

#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qtextstream.h>
#include <qtextcodec.h>

#include <errno.h>
#include <string.h>

// defined in fetchtr.cpp
extern void fetchtr_py(const char *fileName, MetaTranslator *tor,
        const char *defaultContext, bool mustExist,
        const QByteArray &codecForSource, const char *tr_func,
        const char *translate_func);
extern void fetchtr_ui( const char *fileName, MetaTranslator *tor,
                         const char *defaultContext, bool mustExist );



// defined in merge.cpp
extern void merge( const MetaTranslator *tor, const MetaTranslator *virginTor, MetaTranslator *out,
                   bool verbose, bool noObsolete );

typedef QList<MetaTranslatorMessage> TML;

static void printUsage()
{
    fprintf( stderr, "Usage:\n"
             "    pylupdate5 [options] project-file\n"
             "    pylupdate5 [options] source-files -ts ts-files\n"
             "Options:\n"
             "    -help  Display this information and exit\n"
             "    -version\n"
             "           Display the version of pylupdate5 and exit\n"
             "    -verbose\n"
             "           Explain what is being done\n"
             "    -noobsolete\n"
             "           Drop all obsolete strings\n"
             "    -tr-function name\n"
             "           name() may be used instead of tr()\n"
             "    -translate-function name\n"
             "           name() may be used instead of translate()\n");
}

static void updateTsFiles( const MetaTranslator& fetchedTor,
                           const QStringList& tsFileNames, const QString& codecForTr,
                           bool noObsolete, bool verbose )
{
    QStringList::ConstIterator t = tsFileNames.begin();
    QDir dir;
    while ( t != tsFileNames.end() ) {
        QString fn = dir.relativeFilePath(*t);
        MetaTranslator tor;
        MetaTranslator out;
        tor.load( *t );
        if ( !codecForTr.isEmpty() )
            tor.setCodec( codecForTr.toLatin1() );
        if ( verbose )
            fprintf( stderr, "Updating '%s'...\n", fn.toLatin1().constData() );
 
        merge( &tor, &fetchedTor, &out, verbose, noObsolete );
        if ( noObsolete )
            out.stripObsoleteMessages();
        out.stripEmptyContexts();
        if ( !out.save(*t) ) {
#if defined(_MSC_VER) && _MSC_VER >= 1400
            char buf[100];
            strerror_s(buf, sizeof(buf), errno);
            fprintf( stderr, "pylupdate5 error: Cannot save '%s': %s\n",
                     fn.toLatin1().constData(), buf );
#else
            fprintf( stderr, "pylupdate5 error: Cannot save '%s': %s\n",
                     fn.toLatin1().constData(), strerror(errno) );
#endif
        }
        ++t;
    }
}

int main( int argc, char **argv )
{
    QString defaultContext = "@default";
    MetaTranslator fetchedTor;
    QByteArray codecForTr;
    QByteArray codecForSource;
    QStringList tsFileNames;
    QStringList uiFileNames;

    bool verbose = false;
    bool noObsolete = false;
    bool metSomething = false;
    int numFiles = 0;
    bool standardSyntax = true;
    bool metTsFlag = false;
    const char *tr_func = 0;
    const char *translate_func = 0;

    int i;

    for ( i = 1; i < argc; i++ ) {
        if ( qstrcmp(argv[i], "-ts") == 0 )
            standardSyntax = false;
    }

    for ( i = 1; i < argc; i++ ) {
        if ( qstrcmp(argv[i], "-help") == 0 ) {
            printUsage();
            return 0;
        } else if ( qstrcmp(argv[i], "-noobsolete") == 0 ) {
            noObsolete = true;
            continue;
        } else if ( qstrcmp(argv[i], "-verbose") == 0 ) {
            verbose = true;
            continue;
        } else if ( qstrcmp(argv[i], "-version") == 0 ) {
            fprintf( stderr, "pylupdate5 version %s\n", QT_VERSION_STR );
            return 0;
        } else if ( qstrcmp(argv[i], "-ts") == 0 ) {
            metTsFlag = true;
            continue;
        } else if ( qstrcmp(argv[i], "-tr-function") == 0 ) {
            if (!argv[++i])
            {
                fprintf(stderr,
                        "pylupdate5 error: missing -tr-function name\n");
                return 2;
            }

            tr_func = argv[i];
            continue;
        } else if ( qstrcmp(argv[i], "-translate-function") == 0 ) {
            if (!argv[++i])
            {
                fprintf(stderr,
                        "pylupdate5 error: missing -translate-function name\n");
                return 2;
            }

            translate_func = argv[i];
            continue;
        }

        numFiles++;

        QString fullText;

        if ( !metTsFlag ) {
            QFile f( argv[i] );
            if ( !f.open(QIODevice::ReadOnly) ) {
#if defined(_MSC_VER) && _MSC_VER >= 1400
                char buf[100];
                strerror_s(buf, sizeof(buf), errno);
                fprintf( stderr, "pylupdate5 error: Cannot open file '%s': %s\n",
                         argv[i], buf );
#else
                fprintf( stderr, "pylupdate5 error: Cannot open file '%s': %s\n",
                         argv[i], strerror(errno) );
#endif
                return 1;
            }

            QTextStream t( &f );
            fullText = t.readAll();
            f.close();
        }

        QString oldDir = QDir::currentPath();
        QDir::setCurrent( QFileInfo(argv[i]).path() );

        if ( standardSyntax ) {
            fetchedTor = MetaTranslator();
            codecForTr.clear();
            codecForSource.clear();
            tsFileNames.clear();
            uiFileNames.clear();

            QMap<QString, QString> tagMap = proFileTagMap( fullText );
            QMap<QString, QString>::Iterator it;

            for ( it = tagMap.begin(); it != tagMap.end(); ++it ) {
                QStringList toks = it.value().split(' ');
                QStringList::Iterator t;

                for ( t = toks.begin(); t != toks.end(); ++t ) {
                    if ( it.key() == "SOURCES" ) {
                        fetchtr_py((*t).toLatin1(), &fetchedTor,
                                defaultContext.toLatin1(), true,
                                codecForSource, tr_func, translate_func);
                        metSomething = true;
                    } else if ( it.key() == "TRANSLATIONS" ) {
                        tsFileNames.append( *t );
                        metSomething = true;
                    } else if ( it.key() == "CODEC" ||
                                it.key() == "DEFAULTCODEC" ||
                                it.key() == "CODECFORTR" ) {
                        codecForTr = (*t).toLatin1();
                        fetchedTor.setCodecForTr(codecForTr);
                    } else if ( it.key() == "CODECFORSRC" ) {
                        codecForSource = (*t).toLatin1();
                    } else if ( it.key() == "FORMS" ) {
                fetchtr_ui( (*t).toLatin1(), &fetchedTor, defaultContext.toLatin1(), true);
            }
                }
            }

            updateTsFiles( fetchedTor, tsFileNames, codecForTr, noObsolete, verbose );

            if ( !metSomething ) {
                fprintf( stderr,
                         "pylupdate5 warning: File '%s' does not look like a"
                         " project file\n",
                         argv[i] );
            } else if ( tsFileNames.isEmpty() ) {
                fprintf( stderr,
                         "pylupdate5 warning: Met no 'TRANSLATIONS' entry in"
                         " project file '%s'\n",
                         argv[i] );
            }
        } else {
            if ( metTsFlag ) {
                if ( QString(argv[i]).toLower().endsWith(".ts") ) {
                    QFileInfo fi( argv[i] );
                    if ( !fi.exists() || fi.isWritable() ) {
                        tsFileNames.append( argv[i] );
                    } else {
                        fprintf( stderr,
                                 "pylupdate5 warning: For some reason, I cannot"
                                 " save '%s'\n",
                                 argv[i] );
                    }
        } else {
                    fprintf( stderr,
                             "pylupdate5 error: File '%s' lacks .ts extension\n",
                             argv[i] );
                }
            } else {
                QFileInfo fi(argv[i]);
        if ( fi.suffix() == "py" || fi.suffix() == "pyw" ) {
            fetchtr_py(fi.fileName().toLatin1(), &fetchedTor,
                    defaultContext.toLatin1(), true, codecForSource, tr_func,
                    translate_func);
        } else {
            fetchtr_ui( fi.fileName().toLatin1(), &fetchedTor, defaultContext.toLatin1(), true);
        }
            }
        }
        QDir::setCurrent( oldDir );
    }

    if ( !standardSyntax )
        updateTsFiles( fetchedTor, tsFileNames, codecForTr, noObsolete, verbose );

    if ( numFiles == 0 ) {
        printUsage();
        return 1;
    }
    return 0;
}

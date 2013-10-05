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
** Copyright (C) 1992-2005 Trolltech AS. All rights reserved.
**
** This file is part of the Qt Linguist of the Qt Toolkit.
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
extern void fetchtr_py( const char *fileName, MetaTranslator *tor,
                         const char *defaultContext, bool mustExist, const QByteArray &codecForSource );
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
             "    -noobsolete\n"
             "           Drop all obsolete strings\n"
             "    -verbose\n"
             "           Explain what is being done\n"
             "    -version\n"
             "           Display the version of pylupdate5 and exit\n" );
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
                        fetchtr_py( (*t).toLatin1(), &fetchedTor, defaultContext.toLatin1(), true, codecForSource );
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
            fetchtr_py( fi.fileName().toLatin1(), &fetchedTor, defaultContext.toLatin1(), true, codecForSource );
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

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

#include "proparser.h"

#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qregexp.h>
#include <qstringlist.h>
#include <qtextstream.h>

#ifdef Q_OS_UNIX
#include <unistd.h>
#endif

#ifdef Q_OS_WIN32
#define QT_POPEN _popen
#else
#define QT_POPEN popen
#endif

QString loadFile( const QString &fileName )
{
    QFile file( fileName );
    if ( !file.open(QIODevice::ReadOnly) ) {
        fprintf( stderr, "error: Cannot load '%s': %s\n",
                 file.fileName().toLatin1().constData(),
                 file.errorString().toLatin1().constData() );
        return QString();
    }

    QTextStream in( &file );
    return in.readAll();
}

QMap<QString, QString> proFileTagMap( const QString& text )
{
    QString t = text;

    QMap<QString, QString> tagMap;
    bool stillProcess = true; // If include() has a $$tag then we need to reprocess

    while (stillProcess) {
        /*
            Strip any commments before we try to include.  We
            still need to do it after we include to make sure the
            included file does not have comments
        */
        t.replace( QRegExp(QString("#[^\n]*\n")), QString(" ") );

        /*
            Process include() commands.
            $$PWD is a special case so we have to change it while
            we know where the included file is.
        */
        QRegExp callToInclude("include\\s*\\(\\s*([^()\\s]+)\\s*\\)");
        int i = 0;
        while ( (i = callToInclude.indexIn(t, i)) != -1 ) {
            bool doneWithVar = false;
            QString fileName = callToInclude.cap(1);
            QString after = fileName.replace("$$PWD", QDir::currentPath());
            if (!tagMap.isEmpty() && after.contains("$$")) {
                QRegExp var( "\\$\\$[({]?([a-zA-Z0-9_]+)[)}]?" );
                int ii = 0;
                while ((ii = after.indexOf(var, ii)) != -1) {
                    if (tagMap.contains(var.cap(1))) {
                        after.replace(ii, var.cap(0).length(), tagMap[var.cap(1)]);
                    } else { // Couldn't find it
                        doneWithVar = true;
                        break;
                    }
                }

            }
            if (doneWithVar || !after.contains("$$")) {
                after = loadFile(after);
                QFileInfo fi(callToInclude.cap(1));
                after.replace("$$PWD", fi.path());
                t.replace( i, callToInclude.matchedLength(), after );
            }
            i += after.length();
        }

        /*
            Strip comments, merge lines ending with backslash, add
            spaces around '=' and '+=', replace '\n' with ';', and
            simplify white spaces.
        */
        t.replace( QRegExp(QString("#[^\n]*\n")), QString(" ") );
        t.replace( QRegExp(QString("\\\\[^\n\\S]*\n")), QString(" ") );
        t.replace( "=", QString(" = ") );
        t.replace( "+ =", QString(" += ") );
        t.replace( "\n", QString(";") );
        t.replace( "\r", QString("") ); // remove carriage return
        t = t.simplified();

        /*
            Populate tagMap with 'key = value' entries.
        */
        QStringList lines = t.split(';', QString::SkipEmptyParts);
        QStringList::Iterator line;
        for ( line = lines.begin(); line != lines.end(); ++line ) {
            QStringList toks = (*line).split(' ', QString::SkipEmptyParts);

            if ( toks.count() >= 3 &&
                (toks[1] == QString("=") || toks[1] == QString("+=") ||
                toks[1] == QString("*=")) ) {
                QString tag = toks.first();
                int k = tag.lastIndexOf( QChar(':') ); // as in 'unix:'
                if ( k != -1 )
                    tag = tag.mid( k + 1 );
                toks.erase( toks.begin() );

                QString action = toks.first();
                toks.erase( toks.begin() );

                if ( tagMap.contains(tag) ) {
                    if ( action == QString("=") )
                        tagMap.insert( tag, toks.join(" ") );
                    else
                        tagMap[tag] += QChar( ' ' ) + toks.join( " " );
                } else {
                    tagMap[tag] = toks.join( " " );
                }
            }
        }
        /*
            Expand $$variables within the 'value' part of a 'key = value'
            pair.
        */
        QRegExp var( "\\$\\$[({]?([a-zA-Z0-9_]+)[)}]?" );
        QMap<QString, QString>::Iterator it;
        for ( it = tagMap.begin(); it != tagMap.end(); ++it ) {
            int i = 0;
            while ( (i = var.indexIn((*it), i)) != -1 ) {
                int len = var.matchedLength();
                QString invocation = var.cap(1);
                QString after;

                if ( invocation == "system" ) {
                    // skip system(); it will be handled in the next pass
                    ++i;
                } else {
                    if ( tagMap.contains(invocation) )
                        after = tagMap[invocation];
                    else if (invocation.toLower() == "pwd")
                        after = QDir::currentPath();
                    (*it).replace( i, len, after );
                    i += after.length();
                }
            }
        }

        /*
          Execute system() calls.
        */
        QRegExp callToSystem( "\\$\\$system\\s*\\(([^()]*)\\)" );
        for ( it = tagMap.begin(); it != tagMap.end(); ++it ) {
            int i = 0;
            while ( (i = callToSystem.indexIn((*it), i)) != -1 ) {
                /*
                  This code is stolen from qmake's project.cpp file.
                  Ideally we would use the same parser, so we wouldn't
                  have this code duplication.
                */
                QString after;
                char buff[256];
                FILE *proc = QT_POPEN( callToSystem.cap(1).toLatin1().constData(), "r" );
                while ( proc && !feof(proc) ) {
                    int read_in = int(fread( buff, 1, 255, proc ));
                    if ( !read_in )
                        break;
                    for ( int i = 0; i < read_in; i++ ) {
                        if ( buff[i] == '\n' || buff[i] == '\t' )
                            buff[i] = ' ';
                    }
                    buff[read_in] = '\0';
                    after += buff;
                }
                (*it).replace( i, callToSystem.matchedLength(), after );
                i += after.length();
            }
        }
        stillProcess = callToInclude.indexIn(t) != -1;
    }
    return tagMap;
}

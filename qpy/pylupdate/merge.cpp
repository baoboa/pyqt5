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

#include <stdio.h>

#include "pylupdate.h"
#include "simtexth.h"

// defined in numberh.cpp
extern int applyNumberHeuristic( MetaTranslator *tor );
// defined in sametexth.cpp
extern int applySameTextHeuristic( MetaTranslator *tor );

typedef QList<MetaTranslatorMessage> TML;

/*
  Merges two MetaTranslator objects into the first one. The first one
  is a set of source texts and translations for a previous version of
  the internationalized program; the second one is a set of fresh
  source texts newly extracted from the source code, without any
  translation yet.
*/

void merge( const MetaTranslator *tor, const MetaTranslator *virginTor, MetaTranslator *outTor, bool noObsolete, bool verbose, const QString &filename )
{
    if (verbose)
        fprintf(stderr, "Updating '%s'...\n", filename.toLocal8Bit().constData());

    int known = 0;
    int neww = 0;
    int obsoleted = 0;
    int UntranslatedObsoleted = 0;
    int similarTextHeuristicCount = 0;
    TML all = tor->messages();
    TML::Iterator it;
    outTor->setLanguageCode(tor->languageCode());
    outTor->setSourceLanguageCode(tor->sourceLanguageCode());

    /*
      The types of all the messages from the vernacular translator
      are updated according to the virgin translator.
    */
    for ( it = all.begin(); it != all.end(); ++it ) {
        MetaTranslatorMessage::Type newType = MetaTranslatorMessage::Finished;
        MetaTranslatorMessage m = *it;

        // skip context comment
        if ( !QByteArray(m.sourceText()).isEmpty() ) {
            MetaTranslatorMessage mv = virginTor->find(m.context(), m.sourceText(), m.comment());
            if ( mv.isNull() ) {
                mv = virginTor->find(m.context(), m.comment(), m.fileName(), m.lineNumber());
                if ( mv.isNull() ) {
                    // did not find it in the virgin, mark it as obsolete
                    newType = MetaTranslatorMessage::Obsolete;
                    if ( m.type() != MetaTranslatorMessage::Obsolete )
                        obsoleted++;
                } else {
                    // Do not just accept it if its on the same line number, but different source text.
                    // Also check if the texts are more or less similar before we consider them to represent the same message...
                    // ### The QString() cast is evil
                    if (getSimilarityScore(QString(m.sourceText()), mv.sourceText()) >= textSimilarityThreshold) {
                        // It is just slightly modified, assume that it is the same string
                        m = MetaTranslatorMessage(m.context(), mv.sourceText(), m.comment(), m.fileName(), m.lineNumber(), m.translations());
                        m.setPlural(mv.isPlural());

                        // Mark it as unfinished. (Since the source text was changed it might require re-translating...)
                        newType = MetaTranslatorMessage::Unfinished;
                        ++similarTextHeuristicCount;
                    } else {
                        // The virgin and vernacular sourceTexts are so different that we could not find it.
                        newType = MetaTranslatorMessage::Obsolete;
                        if ( m.type() != MetaTranslatorMessage::Obsolete )
                            obsoleted++;
                    }
                    neww++;
                }
            } else {
                switch ( m.type() ) {
                case MetaTranslatorMessage::Finished:
                default:
                    if (m.isPlural() == mv.isPlural()) {
                        newType = MetaTranslatorMessage::Finished;
                    } else {
                        newType = MetaTranslatorMessage::Unfinished;
                    }
                    known++;
                    break;
                case MetaTranslatorMessage::Unfinished:
                    newType = MetaTranslatorMessage::Unfinished;
                    known++;
                    break;
                case MetaTranslatorMessage::Obsolete:
                    newType = MetaTranslatorMessage::Unfinished;
                    neww++;
                }

                // Always get the filename and linenumber info from the virgin Translator, in case it has changed location.
                // This should also enable us to read a file that does not have the <location> element.
                m.setFileName(mv.fileName());
                m.setLineNumber(mv.lineNumber());
                m.setPlural(mv.isPlural());             // ### why not use operator=?
            }

            if (newType == MetaTranslatorMessage::Obsolete && !m.isTranslated()) {
                ++UntranslatedObsoleted;
            }

            m.setType(newType);
            outTor->insert(m);
        }
    }

    /*
      Messages found only in the virgin translator are added to the
      vernacular translator. Among these are all the context comments.
    */
    all = virginTor->messages();

    for ( it = all.begin(); it != all.end(); ++it ) {
        MetaTranslatorMessage mv = *it;
        bool found = tor->contains(mv.context(), mv.sourceText(), mv.comment());
        if (!found) {
            MetaTranslatorMessage m = tor->find(mv.context(), mv.comment(), mv.fileName(), mv.lineNumber());
            if (!m.isNull()) {
                if (getSimilarityScore(QString(m.sourceText()), mv.sourceText()) >= textSimilarityThreshold) {
                    found = true;
                }
            } else {
                found = false;
            }
        }
        if ( !found ) {
            outTor->insert( mv );
            if ( !QByteArray(mv.sourceText()).isEmpty() )
                neww++;
        }
    }

    /*
      The same-text heuristic handles cases where a message has an
      obsolete counterpart with a different context or comment.
    */
    int sameTextHeuristicCount = applySameTextHeuristic( outTor );

    /*
      The number heuristic handles cases where a message has an
      obsolete counterpart with mostly numbers differing in the
      source text.
    */
    int sameNumberHeuristicCount = applyNumberHeuristic( outTor );

    if ( verbose ) {
        int totalFound = neww + known;
        fprintf( stderr, "    Found %d source text%s (%d new and %d already existing)\n",
            totalFound, totalFound == 1 ? "" : "s", neww, known);

        if (obsoleted) {
            if (noObsolete) {
                fprintf( stderr, "    Removed %d obsolete entr%s\n", 
                obsoleted, obsoleted == 1 ? "y" : "ies" );
            } else {
                int total = obsoleted - UntranslatedObsoleted;
                fprintf( stderr, "    Kept %d obsolete translation%s\n", 
                total, total == 1 ? "" : "s" );

                fprintf( stderr, "    Removed %d obsolete untranslated entr%s\n", 
                UntranslatedObsoleted, UntranslatedObsoleted == 1 ? "y" : "ies" );

            }
        }

        if (sameNumberHeuristicCount) 
            fprintf( stderr, "    Number heuristic provided %d translation%s\n", 
                     sameNumberHeuristicCount, sameNumberHeuristicCount == 1 ? "" : "s" );
        if (sameTextHeuristicCount) 
            fprintf( stderr, "    Same-text heuristic provided %d translation%s\n", 
                     sameTextHeuristicCount, sameTextHeuristicCount == 1 ? "" : "s" );
        if (similarTextHeuristicCount)
            fprintf( stderr, "    Similar-text heuristic provided %d translation%s\n", 
                     similarTextHeuristicCount, similarTextHeuristicCount == 1 ? "" : "s" );
    }
}

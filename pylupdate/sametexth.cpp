/****************************************************************************
**
** Copyright (C) 1992-2006 Trolltech ASA. All rights reserved.
**
** This file is part of the Qt Linguist of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://www.trolltech.com/products/qt/opensource.html
**
** If you are unsure which license is appropriate for your use, please
** review the following information:
** http://www.trolltech.com/products/qt/licensing.html or contact the
** sales department at sales@trolltech.com.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "metatranslator.h"

#include <QMap>
#include <stdio.h>

typedef QMap<QByteArray, MetaTranslatorMessage> TMM;
typedef QList<MetaTranslatorMessage> TML;

/*
  Augments a MetaTranslator with trivially derived translations.

  For example, if "Enabled:" is consistendly translated as "Eingeschaltet:" no
  matter the context or the comment, "Eingeschaltet:" is added as the
  translation of any untranslated "Enabled:" text and is marked Unfinished.

  Returns the number of additional messages that this heuristic translated.
*/

int applySameTextHeuristic( MetaTranslator *tor )
{
    TMM translated;
    TMM avoid;
    TMM::Iterator t;
    TML untranslated;
    TML::Iterator u;
    TML all = tor->messages();
    TML::Iterator it;
    int inserted = 0;

    for ( it = all.begin(); it != all.end(); ++it ) {
        if ( (*it).type() == MetaTranslatorMessage::Unfinished ) {
            if ( !(*it).isTranslated() )
                untranslated.append( *it );
        } else {
            QByteArray key = (*it).sourceText();
            t = translated.find( key );
            if ( t != translated.end() ) {
                /*
                  The same source text is translated at least two
                  different ways. Do nothing then.
                */
                if ( (*t).translations() != (*it).translations() ) {
                    translated.remove( key );
                    avoid.insert( key, *it );
                }
            } else if ( !avoid.contains(key) && (*it).isTranslated() ) {
                translated.insert( key, *it );
            }
        }
    }

    for ( u = untranslated.begin(); u != untranslated.end(); ++u ) {
        QByteArray key = (*u).sourceText();
        t = translated.find( key );
        if ( t != translated.end() ) {
            MetaTranslatorMessage m( *u );
            m.setTranslations( (*t).translations() );
            tor->insert( m );
            inserted++;
        }
    }
    return inserted;
}

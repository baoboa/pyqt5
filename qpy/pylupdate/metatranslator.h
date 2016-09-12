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

#ifndef METATRANSLATOR_H
#define METATRANSLATOR_H

#include <qmap.h>
#include <qstring.h>
#include <qlist.h>
#include <qlocale.h>
#include <qdir.h>

#include "translator.h"

#if !defined(QT_BEGIN_NAMESPACE)
#define QT_BEGIN_NAMESPACE
#define QT_END_NAMESPACE
#endif

QT_BEGIN_NAMESPACE
class QTextCodec;
QT_END_NAMESPACE

class MetaTranslatorMessage : public TranslatorMessage
{
public:
    enum Type { Unfinished, Finished, Obsolete };

    MetaTranslatorMessage();
    MetaTranslatorMessage( const char *context, const char *sourceText,
                           const char *comment, 
                           const QString &fileName, int lineNumber,
                           const QStringList& translations = QStringList(),
                           bool utf8 = false, Type type = Unfinished,
                           bool plural = false );
    MetaTranslatorMessage( const MetaTranslatorMessage& m );

    MetaTranslatorMessage& operator=( const MetaTranslatorMessage& m );

    void setType( Type nt ) { ty = nt; }
    Type type() const { return ty; }
    bool utf8() const { return utfeight; }
    bool isPlural() const { return m_plural; }
    void setPlural(bool isplural) { m_plural = isplural; }

    bool operator==( const MetaTranslatorMessage& m ) const;
    bool operator!=( const MetaTranslatorMessage& m ) const
    { return !operator==( m ); }
    bool operator<( const MetaTranslatorMessage& m ) const;
    bool operator<=( const MetaTranslatorMessage& m )
    { return !operator>( m ); }
    bool operator>( const MetaTranslatorMessage& m ) const
    { return this->operator<( m ); }
    bool operator>=( const MetaTranslatorMessage& m ) const
    { return !operator<( m ); }

private:
    bool utfeight;
    Type ty;
    bool m_plural;
};

class MetaTranslator
{
public:
    MetaTranslator();
    MetaTranslator( const MetaTranslator& tor );

    MetaTranslator& operator=( const MetaTranslator& tor );

    void clear();
    bool load( const QString& filename );
    bool save( const QString& filename ) const;
    bool release( const QString& filename, bool verbose = false,
                  bool ignoreUnfinished = false,
                  Translator::SaveMode mode = Translator::Stripped ) const;

    bool contains( const char *context, const char *sourceText,
                   const char *comment ) const;

    MetaTranslatorMessage find( const char *context, const char *sourceText,
                   const char *comment ) const;
    MetaTranslatorMessage find(const char *context, const char *comment, 
                    const QString &fileName, int lineNumber) const;

    void insert( const MetaTranslatorMessage& m );

    void stripObsoleteMessages();
    void stripEmptyContexts();

    void setCodec( const char *name ); // kill me
    void setCodecForTr( const char *name ) { setCodec(name); }
    QTextCodec *codecForTr() const { return codec; }
    QString toUnicode( const char *str, bool utf8 ) const;

    QString languageCode() const;
    QString sourceLanguageCode() const;
    static void languageAndCountry(const QString &languageCode, QLocale::Language *lang, QLocale::Country *country);
    void setLanguageCode(const QString &languageCode);
    void setSourceLanguageCode(const QString &languageCode);
    QList<MetaTranslatorMessage> messages() const;
    QList<MetaTranslatorMessage> translatedMessages() const;
    static int grammaticalNumerus(QLocale::Language language, QLocale::Country country);
    static QStringList normalizedTranslations(const MetaTranslatorMessage& m,
                    QLocale::Language lang, QLocale::Country country);

private:
    void makeFileNamesAbsolute(const QDir &oldPath);

    typedef QMap<MetaTranslatorMessage, int> TMM;
    typedef QMap<int, MetaTranslatorMessage> TMMInv;

    TMM mm;
    QByteArray codecName;
    QTextCodec *codec;
    QString m_language;     // A string beginning with a 2 or 3 letter language code (ISO 639-1 or ISO-639-2),
                            // followed by the optional country variant to distinguist between country-specific variations
                            // of the language. The language code and country code are always separated by '_'
                            // Note that the language part can also be a 3-letter ISO 639-2 code.
                            // Legal examples:
                            // 'pt'         portuguese, assumes portuguese from portugal
                            // 'pt_BR'      Brazilian portuguese (ISO 639-1 language code)
                            // 'por_BR'     Brazilian portuguese (ISO 639-2 language code)
    QString m_sourceLanguage;
};

/*
  This is a quick hack. The proper way to handle this would be
  to extend MetaTranslator's interface.
*/
#define ContextComment "QT_LINGUIST_INTERNAL_CONTEXT_COMMENT"

#endif

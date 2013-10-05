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

#ifndef METATRANSLATOR_H
#define METATRANSLATOR_H

#include <qmap.h>
#include <qstring.h>
#include <translator.h>
#include <qlist.h>
#include <qlocale.h>
#include <qdir.h>

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

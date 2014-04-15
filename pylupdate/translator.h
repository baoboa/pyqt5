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

#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include "QtCore/qobject.h"
#include "QtCore/qbytearray.h"
#include "QtCore/qstringlist.h"
#include "QtCore/qlocale.h"
#include <qtranslator.h>

class TranslatorPrivate;

#if !defined(QT_BEGIN_NAMESPACE)
#define QT_BEGIN_NAMESPACE
#define QT_END_NAMESPACE
#endif

QT_BEGIN_NAMESPACE
template <typename T> class QList;
QT_END_NAMESPACE

class TranslatorMessage
{
public:
    TranslatorMessage();
    TranslatorMessage(const char * context, const char * sourceText,
                       const char * comment,
                       const QString &fileName,
                       int lineNumber,
                       const QStringList& translations = QStringList());
    TranslatorMessage(const TranslatorMessage & m);

    TranslatorMessage & operator=(const TranslatorMessage & m);

    uint hash() const { return h; }
    const char *context() const { return cx.isNull() ? 0 : cx.constData(); }
    const char *sourceText() const { return st.isNull() ? 0 : st.constData(); }
    const char *comment() const { return cm.isNull() ? 0 : cm.constData(); }

    inline void setTranslations(const QStringList &translations);
    QStringList translations() const { return m_translations; }
    void setTranslation(const QString &translation) { m_translations = QStringList(translation); }
    QString translation() const { return m_translations.value(0); }
    bool isTranslated() const { return m_translations.count() > 1 || !m_translations.value(0).isEmpty(); }

    enum Prefix { NoPrefix, Hash, HashContext, HashContextSourceText,
                  HashContextSourceTextComment };
    void write(QDataStream & s, bool strip = false,
                Prefix prefix = HashContextSourceTextComment) const;
    Prefix commonPrefix(const TranslatorMessage&) const;

    bool operator==(const TranslatorMessage& m) const;
    bool operator!=(const TranslatorMessage& m) const
    { return !operator==(m); }
    bool operator<(const TranslatorMessage& m) const;
    bool operator<=(const TranslatorMessage& m) const
    { return !m.operator<(*this); }
    bool operator>(const TranslatorMessage& m) const
    { return m.operator<(*this); }
    bool operator>=(const TranslatorMessage& m) const
    { return !operator<(m); }

    QString fileName(void) const { return m_fileName; }
    void setFileName(const QString &fileName) { m_fileName = fileName; }
    int lineNumber(void) const { return m_lineNumber; }
    void setLineNumber(int lineNumber) { m_lineNumber = lineNumber; }
    bool isNull() const { return st.isNull() && m_lineNumber == -1 && m_translations.isEmpty(); }

private:
    uint h;
    QByteArray cx;
    QByteArray st;
    QByteArray cm;
    QStringList m_translations;
    QString     m_fileName;
    int         m_lineNumber;

    enum Tag { Tag_End = 1, Tag_SourceText16, Tag_Translation, Tag_Context16,
               Tag_Hash, Tag_SourceText, Tag_Context, Tag_Comment,
               Tag_Obsolete1 };
};
Q_DECLARE_TYPEINFO(TranslatorMessage, Q_MOVABLE_TYPE);

inline void TranslatorMessage::setTranslations(const QStringList &translations)
{ m_translations = translations; }

class Translator : public QTranslator
{
    Q_OBJECT
public:
    explicit Translator(QObject *parent = 0);
    ~Translator();

    virtual TranslatorMessage findMessage(const char *context, const char *sourceText,
                                          const char *comment = 0, 
                                          const QString &fileName = 0, int lineNumber = -1) const;
    virtual QString translate(const char *context, const char *sourceText,
                              const char *comment = 0, int = -1) const
        { return findMessage(context, sourceText, comment).translation(); }

    bool load(const QString & filename,
               const QString & directory = QString(),
               const QString & search_delimiters = QString(),
               const QString & suffix = QString());
    bool load(const uchar *data, int len);

    void clear();

    enum SaveMode { Everything, Stripped };

    bool save(const QString & filename, SaveMode mode = Everything);

    void insert(const TranslatorMessage&);
    inline void insert(const char *context, const char *sourceText, const QString &fileName, int lineNo, const QStringList &translations) {
        insert(TranslatorMessage(context, sourceText, "", fileName, lineNo, translations));
    }
    void remove(const TranslatorMessage&);
    inline void remove(const char *context, const char *sourceText) {
        remove(TranslatorMessage(context, sourceText, "", QLatin1String(""), -1));
    }
    bool contains(const char *context, const char *sourceText, const char * comment = 0) const;
    bool contains(const char *context, const char *comment, const QString &fileName, int lineNumber) const;

    void squeeze(SaveMode = Everything);
    void unsqueeze();

    QList<TranslatorMessage> messages() const;

    bool isEmpty() const;

private:
    Q_DISABLE_COPY(Translator)
    TranslatorPrivate *d;
};

static const char * const japaneseStyleForms[] = { "Unique Form", 0 };
static const char * const englishStyleForms[] = { "Singular", "Plural", 0 };
static const char * const frenchStyleForms[] = { "Singular", "Plural", 0 };
static const char * const latvianForms[] = { "Singular", "Plural", "Nullar", 0 };
static const char * const irishStyleForms[] = { "Singular", "Dual", "Plural", 0 };
static const char * const czechForms[] = { "Singular", "Dual", "Plural", 0 };
static const char * const slovakForms[] = { "Singular", "Dual", "Plural", 0 };
static const char * const macedonianForms[] = { "Singular", "Dual", "Plural", 0 };
static const char * const lithuanianForms[] = { "Singular", "Dual", "Plural", 0 };
static const char * const russianStyleForms[] = { "Singular", "Dual", "Plural", 0 };
static const char * const polishForms[] = { "Singular", "Paucal", "Plural", 0 };
static const char * const romanianForms[] =
    { "Singular", "Plural Form for 2 to 19", "Plural", 0 };
static const char * const slovenianForms[] = { "Singular", "Dual", "Trial", "Plural", 0 };
static const char * const malteseForms[] =
    { "Singular", "Plural Form for 2 to 10", "Plural Form for 11 to 19", "Plural", 0 };
static const char * const welshForms[] =
    { "Nullar", "Singular", "Dual", "Sexal", "Plural", 0 };
static const char * const arabicForms[] =
    { "Nullar", "Singular", "Dual", "Minority Plural", "Plural", "Plural Form for 100, 200, ...", 0 };

#define EOL QLocale::C

static const QLocale::Language japaneseStyleLanguages[] = {
    QLocale::Afan,
    QLocale::Armenian,
    QLocale::Bhutani,
    QLocale::Bislama,
    QLocale::Burmese,
    QLocale::Chinese,
#if QT_VERSION >= 0x050000
    QLocale::Fijian,
#else
    QLocale::FijiLanguage,
#endif
    QLocale::Guarani,
    QLocale::Hungarian,
    QLocale::Indonesian,
    QLocale::Japanese,
    QLocale::Javanese,
    QLocale::Korean,
    QLocale::Malay,
    QLocale::NauruLanguage,
    QLocale::Persian,
    QLocale::Sundanese,
    QLocale::Thai,
    QLocale::Tibetan,
    QLocale::Vietnamese,
    QLocale::Yoruba,
    QLocale::Zhuang,
    EOL
};

static const QLocale::Language englishStyleLanguages[] = {
    QLocale::Abkhazian,
    QLocale::Afar,
    QLocale::Afrikaans,
    QLocale::Albanian,
    QLocale::Amharic,
    QLocale::Assamese,
    QLocale::Aymara,
    QLocale::Azerbaijani,
    QLocale::Bashkir,
    QLocale::Basque,
    QLocale::Bengali,
    QLocale::Bihari,
    // Missing: Bokmal,
    QLocale::Bulgarian,
    QLocale::Cambodian,
    QLocale::Catalan,
    QLocale::Cornish,
    QLocale::Corsican,
    QLocale::Danish,
    QLocale::Dutch,
    QLocale::English,
    QLocale::Esperanto,
    QLocale::Estonian,
    QLocale::Faroese,
    QLocale::Finnish,
    // Missing: Friulian,
    QLocale::Frisian,
    QLocale::Galician,
    QLocale::Georgian,
    QLocale::German,
    QLocale::Greek,
    QLocale::Greenlandic,
    QLocale::Gujarati,
    QLocale::Hausa,
    QLocale::Hebrew,
    QLocale::Hindi,
    QLocale::Icelandic,
    QLocale::Interlingua,
    QLocale::Interlingue,
    QLocale::Italian,
    QLocale::Kannada,
    QLocale::Kashmiri,
    QLocale::Kazakh,
    QLocale::Kinyarwanda,
    QLocale::Kirghiz,
    QLocale::Kurdish,
    QLocale::Kurundi,
#if QT_VERSION >= 0x050000
    QLocale::Lao,
#else
    QLocale::Laothian,
#endif
    QLocale::Latin,
    // Missing: Letzeburgesch,
    QLocale::Lingala,
    QLocale::Malagasy,
    QLocale::Malayalam,
    QLocale::Marathi,
    QLocale::Mongolian,
    // Missing: Nahuatl,
    QLocale::Nepali,
    // Missing: Northern Sotho,
    QLocale::Norwegian,
#if QT_VERSION >= 0x050000
    QLocale::NorwegianNynorsk,
#else
    QLocale::Nynorsk,
#endif
    QLocale::Occitan,
    QLocale::Oriya,
    QLocale::Pashto,
    QLocale::Portuguese,
    QLocale::Punjabi,
    QLocale::Quechua,
    QLocale::RhaetoRomance,
#if QT_VERSION >= 0x050000
    QLocale::SouthernSotho,
    QLocale::Tswana,
#else
    QLocale::Sesotho,
    QLocale::Setswana,
#endif
    QLocale::Shona,
    QLocale::Sindhi,
#if QT_VERSION >= 0x050000
    QLocale::Sinhala,
    QLocale::Swati,
#else
    QLocale::Singhalese,
    QLocale::Siswati,
#endif
    QLocale::Somali,
    QLocale::Spanish,
    QLocale::Swahili,
    QLocale::Swedish,
    QLocale::Tagalog,
    QLocale::Tajik,
    QLocale::Tamil,
    QLocale::Tatar,
    QLocale::Telugu,
#if QT_VERSION >= 0x050000
    QLocale::Tongan,
#else
    QLocale::TongaLanguage,
#endif
    QLocale::Tsonga,
    QLocale::Turkish,
    QLocale::Turkmen,
    QLocale::Twi,
    QLocale::Uigur,
    QLocale::Uzbek,
    QLocale::Volapuk,
    QLocale::Wolof,
    QLocale::Xhosa,
    QLocale::Yiddish,
    QLocale::Zulu,
    EOL
};
static const QLocale::Language frenchStyleLanguages[] = {
    // keep synchronized with frenchStyleCountries
    QLocale::Breton,
    QLocale::French,
    QLocale::Portuguese,
    // Missing: Filipino,
    QLocale::Tigrinya,
    // Missing: Walloon
    EOL
};
static const QLocale::Language latvianLanguage[] = { QLocale::Latvian, EOL };
static const QLocale::Language irishStyleLanguages[] = {
    QLocale::Divehi,
    QLocale::Gaelic,
    QLocale::Inuktitut,
    QLocale::Inupiak,
    QLocale::Irish,
    QLocale::Manx,
    QLocale::Maori,
    // Missing: Sami,
    QLocale::Samoan,
    QLocale::Sanskrit,
    EOL
};
static const QLocale::Language czechLanguage[] = { QLocale::Czech, EOL };
static const QLocale::Language slovakLanguage[] = { QLocale::Slovak, EOL };
static const QLocale::Language macedonianLanguage[] = { QLocale::Macedonian, EOL };
static const QLocale::Language lithuanianLanguage[] = { QLocale::Lithuanian, EOL };
static const QLocale::Language russianStyleLanguages[] = {
    QLocale::Bosnian,
    QLocale::Byelorussian,
    QLocale::Croatian,
    QLocale::Russian,
    QLocale::Serbian,
    QLocale::SerboCroatian,
    QLocale::Ukrainian,
    EOL
};
static const QLocale::Language polishLanguage[] = { QLocale::Polish, EOL };
static const QLocale::Language romanianLanguages[] = {
    QLocale::Moldavian,
    QLocale::Romanian,
    EOL
};
static const QLocale::Language slovenianLanguage[] = { QLocale::Slovenian, EOL };
static const QLocale::Language malteseLanguage[] = { QLocale::Maltese, EOL };
static const QLocale::Language welshLanguage[] = { QLocale::Welsh, EOL };
static const QLocale::Language arabicLanguage[] = { QLocale::Arabic, EOL };

static const QLocale::Country frenchStyleCountries[] = {
    // keep synchronized with frenchStyleLanguages
    QLocale::AnyCountry,
    QLocale::AnyCountry,
    QLocale::Brazil,
    QLocale::AnyCountry
};

struct NumerusTableEntry {
    const char * const *forms;
    const QLocale::Language *languages;
    const QLocale::Country *countries;
};

static const NumerusTableEntry numerusTable[] = {
    { japaneseStyleForms, japaneseStyleLanguages, 0 },
    { englishStyleForms, englishStyleLanguages, 0 },
    { frenchStyleForms, frenchStyleLanguages, frenchStyleCountries },
    { latvianForms, latvianLanguage, 0 },
    { irishStyleForms, irishStyleLanguages, 0 },
    { czechForms, czechLanguage, 0 },
    { slovakForms, slovakLanguage, 0 },
    { macedonianForms, macedonianLanguage, 0 },
    { lithuanianForms, lithuanianLanguage, 0 },
    { russianStyleForms, russianStyleLanguages, 0 },
    { polishForms, polishLanguage, 0 },
    { romanianForms, romanianLanguages, 0 },
    { slovenianForms, slovenianLanguage, 0 },
    { malteseForms, malteseLanguage, 0 },
    { welshForms, welshLanguage, 0 },
    { arabicForms, arabicLanguage, 0 }
};

static const int NumerusTableSize = sizeof(numerusTable) / sizeof(numerusTable[0]);

bool getNumerusInfo(QLocale::Language language, QLocale::Country country,
                           QStringList *forms);

#endif // TRANSLATOR_H

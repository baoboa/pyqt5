/**********************************************************************
** Copyright (C) 2016 Riverbank Computing Limited
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

#include <qfile.h>
#include <qregexp.h>
#include <qstring.h>
#include <qtextstream.h>
#include <qstack.h>

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <QTextCodec>
#include <QtXml>

#include "pylupdate.h"

static const char MagicComment[] = "TRANSLATOR ";

static QMap<QByteArray, int> needs_Q_OBJECT;
static QMap<QByteArray, int> lacks_Q_OBJECT;

/*
  The first part of this source file is the Python tokenizer.  We skip
  most of Python; the only tokens that interest us are defined here.
*/

enum { Tok_Eof, Tok_class, Tok_return, Tok_tr,
       Tok_trUtf8, Tok_translate, Tok_Ident,
       Tok_Comment, Tok_Dot, Tok_String,
       Tok_LeftParen, Tok_RightParen,
       Tok_Comma, Tok_None, Tok_Integer};

// The names of function aliases passed on the command line.
static const char *tr_function;
static const char *translate_function;

/*
  The tokenizer maintains the following global variables. The names
  should be self-explanatory.
*/
static QByteArray yyFileName;
static int yyCh;
static char yyIdent[128];
static size_t yyIdentLen;
static char yyComment[65536];
static size_t yyCommentLen;
static char yyString[65536];
static size_t yyStringLen;
static qlonglong yyInteger;
static QStack<int> yySavedParenDepth;
static int yyParenDepth;
static int yyLineNo;
static int yyCurLineNo;
static int yyParenLineNo;
static QTextCodec *yyCodecForTr = 0;
static QTextCodec *yyCodecForSource = 0;

// the file to read from (if reading from a file)
static FILE *yyInFile;

// the string to read from and current position in the string (otherwise)
static QString yyInStr;
static int yyInPos;
// - 'rawbuf' is used to hold bytes before universal newline translation.
// - 'buf' is its higher-level counterpart, where every end-of-line appears as
//   a single '\n' character, regardless of the end-of-line style used in input
//   files.
static int buf, rawbuf;

static int (*getChar)();
static int (*peekChar)();

static bool yyParsingUtf8;

static int getTranslatedCharFromFile()
{
    int c;

    if ( rawbuf < 0 )           // Empty raw buffer?
        c = getc( yyInFile );
    else {
        c = rawbuf;
        rawbuf = -1;            // Declare the raw buffer empty.
    }

    // Universal newline translation, similar to what Python does
    if ( c == '\r' ) {
        c = getc( yyInFile ); // Last byte of a \r\n sequence?
        if ( c != '\n')
            {
                rawbuf = c; // No, put it in 'rawbuf' for later processing.
                // Logical character that will be seen by higher-level functions
                c = '\n';
            }
        // In all cases, c == '\n' here.
    }

    return c;
}

static int getCharFromFile()
{
    int c;

    if ( buf < 0 ) {            // Empty buffer?
        c = getTranslatedCharFromFile();
    } else {
        c = buf;
        buf = -1;               // Declare the buffer empty.
    }

    if ( c == '\n' )            // This is after universal newline translation
        yyCurLineNo++;          // (i.e., a "logical" newline character).

    return c;
}

static int peekCharFromFile()
{
    // Read a character, possibly performing universal newline translation,
    // and put it in 'buf' so that the next call to getCharFromFile() finds it
    // already available.
    buf = getCharFromFile();
    return buf;
}

static void startTokenizer( const char *fileName, int (*getCharFunc)(),
                            int (*peekCharFunc)(), QTextCodec *codecForTr, QTextCodec *codecForSource )
{
    yyInPos = 0;
    buf = rawbuf = -1;
    getChar = getCharFunc;
    peekChar = peekCharFunc;

    yyFileName = fileName;
    yyCh = getChar();
    yySavedParenDepth.clear();
    yyParenDepth = 0;
    yyCurLineNo = 1;
    yyParenLineNo = 1;
    yyCodecForTr = codecForTr;
    if (!yyCodecForTr)
        yyCodecForTr = QTextCodec::codecForName("ISO-8859-1");
    Q_ASSERT(yyCodecForTr);
    yyCodecForSource = codecForSource;

    yyParsingUtf8 = false;
}

static int getToken()
{
    const char tab[] = "abfnrtv";
    const char backTab[] = "\a\b\f\n\r\t\v";
    uint n;
    bool quiet;

    yyIdentLen = 0;
    yyCommentLen = 0;
    yyStringLen = 0;

    while ( yyCh != EOF ) {
        yyLineNo = yyCurLineNo;

        if ( isalpha(yyCh) || yyCh == '_' ) {
            do {
                if ( yyIdentLen < sizeof(yyIdent) - 1 )
                    yyIdent[yyIdentLen++] = (char) yyCh;
                yyCh = getChar();
            } while ( isalnum(yyCh) || yyCh == '_' );
            yyIdent[yyIdentLen] = '\0';

            // Check for names passed on the command line.
            if (tr_function && strcmp(yyIdent, tr_function) == 0)
                return Tok_tr;

            if (translate_function && strcmp(yyIdent, translate_function) == 0)
                return Tok_translate;

            bool might_be_str = false;

            switch ( yyIdent[0] ) {
                case 'N':
                    if ( strcmp(yyIdent + 1, "one") == 0 )
                        return Tok_None;
                    break;
                case 'Q':
                    if (strcmp(yyIdent + 1, "T_TR_NOOP") == 0)
                    {
                        yyParsingUtf8 = false;
                        return Tok_tr;
                    }
                    else if (strcmp(yyIdent + 1, "T_TR_NOOP_UTF8") == 0)
                    {
                        yyParsingUtf8 = true;
                        return Tok_trUtf8;
                    }
                    else if (strcmp(yyIdent + 1, "T_TRANSLATE_NOOP") == 0)
                    {
                        yyParsingUtf8 = false;
                        return Tok_translate;
                    }
                    break;
                case 'c':
                    if ( strcmp(yyIdent + 1, "lass") == 0 )
                        return Tok_class;
                    break;
                case 'f':
                    /*
                     * QTranslator::findMessage() has the same parameters as
                     * QApplication::translate().
                     */
                    if ( strcmp(yyIdent + 1, "indMessage") == 0 )
                        return Tok_translate;
                    break;
                case 'r':
                    if ( strcmp(yyIdent + 1, "eturn") == 0 )
                        return Tok_return;

                    /* Drop through. */

                case 'R':
                    if (yyIdent[1] == '\0')
                        might_be_str = true;
                    break;
                case 'b':
                case 'B':
                case 'u':
                case 'U':
                    if (yyIdent[1] == '\0')
                        might_be_str = true;
                    else if ((yyIdent[1] == 'r' || yyIdent[1] == 'R') && yyIdent[2] == '\0')
                        might_be_str = true;
                    break;
                case 't':
                    if ( strcmp(yyIdent + 1, "r") == 0 ) {
                        yyParsingUtf8 = false;
                        return Tok_tr;
                    } else if ( qstrcmp(yyIdent + 1, "rUtf8") == 0 ) {
                        yyParsingUtf8 = true;
                        return Tok_trUtf8;
                    } else if ( qstrcmp(yyIdent + 1, "ranslate") == 0 ) {
                        yyParsingUtf8 = false;
                        return Tok_translate;
                    }
                    break;
                case '_':
                    if ( strcmp(yyIdent + 1, "_tr") == 0 ) {
                        yyParsingUtf8 = false;
                        return Tok_tr;
                    } else if ( strcmp(yyIdent + 1, "_trUtf8") == 0 ) {
                        yyParsingUtf8 = true;
                        return Tok_trUtf8;
                    } else if ( qstrcmp(yyIdent + 1, "translate") == 0 ) {
                        yyParsingUtf8 = false;
                        return Tok_translate;
                    }
                    break;
            }

            /*
             * Handle the standard Python v2 and v3 string prefixes by simply
             * ignoring them.
             */

            if (!might_be_str)
                return Tok_Ident;

            if (yyCh != '"' && yyCh != '\'')
                return Tok_Ident;
        }
        {
            switch ( yyCh ) {
                case '#':
                    do {
                        yyCh = getChar();
                    } while ( yyCh != EOF && yyCh != '\n' );
                    break;
                case '"':
                case '\'':
                    int quoteChar;
                    int trippelQuote, singleQuote;
                    int in;

                    quoteChar = yyCh;
                    trippelQuote = 0;
                    singleQuote = 1;
                    in = 0;
                    yyCh = getChar();
                    quiet = false;

                    while ( yyCh != EOF ) {
                        if ( singleQuote && (yyCh == '\n' || (in && yyCh == quoteChar)) )
                            break;

                        if ( yyCh == quoteChar ) {
                            if (peekChar() == quoteChar) {
                                yyCh = getChar();
                                if (!trippelQuote) {
                                    trippelQuote = 1;
                                    singleQuote = 0;
                                    in = 1;
                                    yyCh = getChar();
                                } else {
                                    yyCh = getChar();
                                    if (yyCh == quoteChar) {
                                        trippelQuote = 0;
                                        break;
                                    }
                                }
                            } else if (trippelQuote) {
                                if ( yyStringLen < sizeof(yyString) - 1 )
                                    yyString[yyStringLen++] = (char) yyCh;
                                yyCh = getChar();
                                continue;
                            } else
                                break;
                        } else
                            in = 1;

                        if ( yyCh == '\\' ) {
                            yyCh = getChar();

                            if ( yyCh == 'x' ) {
                                QByteArray hex = "0";

                                yyCh = getChar();
                                while ( isxdigit(yyCh) ) {
                                    hex += (char) yyCh;
                                    yyCh = getChar();
                                }
#if defined(_MSC_VER) && _MSC_VER >= 1400
                                sscanf_s( hex, "%x", &n );
#else
                                sscanf( hex, "%x", &n );
#endif
                                if ( yyStringLen < sizeof(yyString) - 1 )
                                    yyString[yyStringLen++] = (char) n;
                            } else if ( yyCh >= '0' && yyCh < '8' ) {
                                QByteArray oct = "";
                                int n = 0;

                                do {
                                    oct += (char) yyCh;
                                    ++n;
                                    yyCh = getChar();
                                } while ( yyCh >= '0' && yyCh < '8' && n < 3 );
    #if defined(_MSC_VER) && _MSC_VER >= 1400
                                sscanf_s( oct, "%o", &n );
    #else
                                sscanf( oct, "%o", &n );
    #endif
                                if ( yyStringLen < sizeof(yyString) - 1 )
                                    yyString[yyStringLen++] = (char) n;
                            } else if ( yyCh == '\n' ) {
                                yyCh = getChar();
                            } else {
                                const char *p = strchr( tab, yyCh );
                                if ( yyStringLen < sizeof(yyString) - 1 )
                                    yyString[yyStringLen++] = ( p == 0 ) ?
                                            (char) yyCh : backTab[p - tab];
                                yyCh = getChar();
                            }
                        } else {
                            if (!yyCodecForSource) {
                                if ( yyParsingUtf8 && yyCh >= 0x80 && !quiet) {
                                    qWarning( "%s:%d: Non-ASCII character detected in trUtf8 string",
                                              (const char *) yyFileName, yyLineNo );
                                    quiet = true;
                                }
                                // common case: optimized
                                if ( yyStringLen < sizeof(yyString) - 1 )
                                    yyString[yyStringLen++] = (char) yyCh;
                                yyCh = getChar();
                            } else {
                                QByteArray originalBytes;
                                while ( yyCh != EOF && (trippelQuote || yyCh != '\n') && yyCh != quoteChar && yyCh != '\\' ) {
                                    if ( yyParsingUtf8 && yyCh >= 0x80 && !quiet) {
                                        qWarning( "%s:%d: Non-ASCII character detected in trUtf8 string",
                                                (const char *) yyFileName, yyLineNo );
                                        quiet = true;
                                    }
                                    originalBytes += (char)yyCh;
                                    yyCh = getChar();
                                }

                                QString unicodeStr = yyCodecForSource->toUnicode(originalBytes);
                                QByteArray convertedBytes;

                                if (!yyCodecForTr->canEncode(unicodeStr) && !quiet) {
                                    qWarning( "%s:%d: Cannot convert Python string from %s to %s",
                                              (const char *) yyFileName, yyLineNo, yyCodecForSource->name().constData(),
                                              yyCodecForTr->name().constData() );
                                    quiet = true;
                                }
                                convertedBytes = yyCodecForTr->fromUnicode(unicodeStr);

                                size_t len = qMin((size_t)convertedBytes.size(), sizeof(yyString) - yyStringLen - 1);
                                memcpy(yyString + yyStringLen, convertedBytes.constData(), len);
                                yyStringLen += len;
                            }
                        }
                    }
                    yyString[yyStringLen] = '\0';

                    if ( yyCh != quoteChar ) {
                        if (trippelQuote)
                            qWarning("%s:%d: Empty or unterminated triple quoted string",
                                    (const char *)yyFileName, yyLineNo);
                        else
                            qWarning("%s:%d: Unterminated string",
                                    (const char *)yyFileName, yyLineNo);
                    }

                    if ( yyCh == EOF ) {
                        return Tok_Eof;
                    } else {
                        yyCh = getChar();
                        return Tok_String;
                    }
                    break;
                case '(':
                    if (yyParenDepth == 0)
                        yyParenLineNo = yyCurLineNo;
                        yyParenDepth++;
                        yyCh = getChar();
                        return Tok_LeftParen;
                case ')':
                    if (yyParenDepth == 0)
                        yyParenLineNo = yyCurLineNo;
                        yyParenDepth--;
                        yyCh = getChar();
                        return Tok_RightParen;
                case ',':
                    yyCh = getChar();
                    return Tok_Comma;
                case '.':
                    yyCh = getChar();
                    return Tok_Dot;
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    {
                        QByteArray ba;
                        ba+=yyCh;
                        yyCh = getChar();
                        bool hex = yyCh == 'x';
                        if ( hex ) {
                            ba+=yyCh;
                            yyCh = getChar();
                        }
                        while ( (hex ? isxdigit(yyCh) : isdigit(yyCh)) ) {
                            ba+=yyCh;
                            yyCh = getChar();
                        }
                        bool ok;
                        yyInteger = ba.toLongLong(&ok);
                        if (ok) return Tok_Integer;
                        break;
                    }
                default:
                    yyCh = getChar();
            }
        }
    }
    return Tok_Eof;
}

/*
  The second part of this source file is the parser. It accomplishes
  a very easy task: It finds all strings inside a tr() or translate()
  call, and possibly finds out the context of the call. It supports
  three cases:
  (1) the context is specified, as in FunnyDialog.tr("Hello") or
     translate("FunnyDialog", "Hello");
  (2) the call appears within an inlined function;
  (3) the call appears within a function defined outside the class definition.
*/

static int yyTok;

static bool match( int t )
{
    bool matches = ( yyTok == t );
    if ( matches )
        yyTok = getToken();
    return matches;
}

static bool matchString( QByteArray *s )
{
    bool matches = ( yyTok == Tok_String );
    *s = "";
    while ( yyTok == Tok_String ) {
        *s += yyString;
        yyTok = getToken();
    }
    return matches;
}

static bool matchStringOrNone(QByteArray *s)
{
    bool matches = matchString(s);

    if (!matches)
    matches = match(Tok_None);

    return matches;
}

/*
 * match any expression that can return a number, which can be
 * 1. Literal number (e.g. '11')
 * 2. simple identifier (e.g. 'm_count')
 * 3. simple function call (e.g. 'size()' )
 * 4. function call on an object (e.g. 'list.size()')
 *
 * Other cases:
 * size(2,4)
 * list().size()
 * list(a,b).size(2,4)
 * etc...
 */
static bool matchExpression()
{
    if (match(Tok_Integer)) {
        return true;
    }

    int parenlevel = 0;
    while (match(Tok_Ident) || parenlevel > 0) {
        if (yyTok == Tok_RightParen) {
            if (parenlevel == 0) break;
            --parenlevel;
            yyTok = getToken();
        } else if (yyTok == Tok_LeftParen) {
            yyTok = getToken();
            if (yyTok == Tok_RightParen) {
                yyTok = getToken();
            } else {
                ++parenlevel;
            }
        } else if (yyTok == Tok_Ident) {
            continue;
        } else if (parenlevel == 0) {
            return false;
        }
    }
    return true;
}

static void parse( MetaTranslator *tor, const char *initialContext,
           const char *defaultContext )
{
    QMap<QByteArray, QByteArray> qualifiedContexts;
    QByteArray context;
    QByteArray text;
    QByteArray com;
    QByteArray functionContext = initialContext;
    QByteArray prefix;
    bool utf8 = false;

    yyTok = getToken();
    while ( yyTok != Tok_Eof ) {
        switch ( yyTok ) {
        case Tok_class:
                yyTok = getToken();
                functionContext = yyIdent;
                yyTok = getToken();
                break;
            case Tok_tr:
            case Tok_trUtf8:
                utf8 = (yyTok == Tok_trUtf8 || (yyCodecForTr && strcmp(yyCodecForTr->name(), "UTF-8") == 0));
                yyTok = getToken();
                if (match(Tok_LeftParen) && matchString(&text))
                {
                    com = "";
                    bool plural = false;

                    // Note that this isn't as rigorous as the parsing of
                    // translate() below.
                    if (match(Tok_RightParen))
                    {
                        // There is no comment or plural arguments.
                    }
                    else if (match(Tok_Comma) && matchStringOrNone(&com))
                    {
                        // There is a comment argument.
                        if (match(Tok_RightParen))
                        {
                            // There is no plural argument.
                        }
                        else if (match(Tok_Comma))
                        {
                            // There is a plural argument.
                            plural = true;
                        }
                    }

                    if (prefix.isNull())
                        context = defaultContext;
                    else if (qstrcmp(prefix, "self") == 0)
                        context = functionContext;
                    else
                        context = prefix;

                    prefix = (const char *) 0;

                    if (qualifiedContexts.contains(context))
                        context = qualifiedContexts[context];

                    if (!text.isEmpty())
                    {
                        tor->insert(MetaTranslatorMessage(context, text, com, 
                                yyFileName, yyParenLineNo,
                                QStringList(), utf8,
                                MetaTranslatorMessage::Unfinished, plural));
                    }
                }
                break;
            case Tok_translate:
                yyTok = getToken();
                if ( match(Tok_LeftParen) &&
                 matchString(&context) &&
                 match(Tok_Comma) &&
                 matchString(&text) ) {
                    com = "";
                    bool plural = false;
                    if (!match(Tok_RightParen)) {
                        // look for comment
                        if ( match(Tok_Comma) && matchStringOrNone(&com)) {
                            if (!match(Tok_RightParen)) {
                                // Look for n.
                                if (match(Tok_Comma)) {
                                    if (matchExpression() && match(Tok_RightParen)) {
                                        plural = true;
                                    } else {
                                        break;
                                    }
                                } else {
                                    break;
                                }
                            }
                        } else {
                            break;
                        }
                    }
                    if (!text.isEmpty())
                    {
                        tor->insert( MetaTranslatorMessage(context, text, com, 
                                                        yyFileName, yyParenLineNo,
                                                        QStringList(), true,
                                                        MetaTranslatorMessage::Unfinished,
                                                        plural) );
                    }
                }
                break;
            case Tok_Ident:
                if ( !prefix.isNull() )
                    prefix += ".";
                prefix += yyIdent;
                yyTok = getToken();
                if ( yyTok != Tok_Dot )
                    prefix = (const char *) 0;
                break;
            case Tok_Comment:
                com = yyComment;
                com = com.simplified();
                if ( com.left(sizeof(MagicComment) - 1) == MagicComment ) {
                    com.remove( 0, sizeof(MagicComment) - 1 );
                    int k = com.indexOf( ' ' );
                    if ( k == -1 ) {
                        context = com;
                    } else {
                        context = com.left( k );
                        com.remove( 0, k + 1 );
                        tor->insert( MetaTranslatorMessage(context, "", com,
                                                        yyFileName, yyParenLineNo,
                                                        QStringList(), false) );
    
                    }
                }
                yyTok = getToken();
                break;
            default:
                yyTok = getToken();
        }
    }

    if ( yyParenDepth != 0 )
        qWarning( "%s: Unbalanced parentheses in Python code",
            (const char *) yyFileName );
}

void fetchtr_py(const char *fileName, MetaTranslator *tor,
        const char *defaultContext, bool mustExist,
        const char *codecForSource, const char *tr_func,
        const char *translate_func)
{
    tr_function = tr_func;
    translate_function = translate_func;

#if defined(_MSC_VER) && _MSC_VER >= 1400
    if (fopen_s(&yyInFile, fileName, "r")) {
        if ( mustExist ) {
            char buf[100];
            strerror_s(buf, sizeof(buf), errno);
            fprintf( stderr,
                     "pylupdate5 error: Cannot open Python source file '%s': %s\n",
                     fileName, buf );
        }
#else
    yyInFile = fopen( fileName, "r" );
    if ( yyInFile == 0 ) {
        if ( mustExist )
            fprintf( stderr,
                     "pylupdate5 error: Cannot open Python source file '%s': %s\n",
                     fileName, strerror(errno) );
#endif
        return;
    }

    startTokenizer( fileName, getCharFromFile, peekCharFromFile, tor->codecForTr(), QTextCodec::codecForName(codecForSource) );
    parse( tor, 0, defaultContext );
    fclose( yyInFile );
}

class UiHandler : public QXmlDefaultHandler
{
public:
    UiHandler( MetaTranslator *translator, const char *fileName )
        : tor( translator ), fname( fileName ), comment( "" ) { }

    virtual bool startElement( const QString& namespaceURI,
                               const QString& localName, const QString& qName,
                               const QXmlAttributes& atts );
    virtual bool endElement( const QString& namespaceURI,
                             const QString& localName, const QString& qName );
    virtual bool characters( const QString& ch );
    virtual bool fatalError( const QXmlParseException& exception );

    virtual void setDocumentLocator(QXmlLocator *locator)
    {
        m_locator = locator;
    }
    QXmlLocator *m_locator;
private:
    void flush();

    MetaTranslator *tor;
    QByteArray fname;
    QString context;
    QString source;
    QString comment;

    QString accum;
    int m_lineNumber;
    bool trString;
};

bool UiHandler::startElement( const QString& /* namespaceURI */,
                              const QString& /* localName */,
                              const QString& qName,
                              const QXmlAttributes& atts )
{
    if ( qName == QString("item") ) {
        flush();
        if ( !atts.value(QString("text")).isEmpty() )
            source = atts.value( QString("text") );
    } else if ( qName == QString("string") ) {
        flush();
        if (atts.value(QString("notr")).isEmpty() ||
            atts.value(QString("notr")) != QString("true")) {
            trString = true;
            comment = atts.value(QString("comment"));
        } else {
            trString = false;
        }
    }
    if (trString) m_lineNumber = m_locator->lineNumber();
    accum.truncate( 0 );
    return true;
}

bool UiHandler::endElement( const QString& /* namespaceURI */,
                            const QString& /* localName */,
                            const QString& qName )
{
    accum.replace( QRegExp(QString("\r\n")), "\n" );

    if ( qName == QString("class") ) {
        if ( context.isEmpty() )
            context = accum;
    } else if ( qName == QString("string") && trString ) {
        source = accum;
    } else if ( qName == QString("comment") ) {
        comment = accum;
        flush();
    } else {
        flush();
    }
    return true;
}

bool UiHandler::characters( const QString& ch )
{
    accum += ch;
    return true;
}

bool UiHandler::fatalError( const QXmlParseException& exception )
{
    QString msg;
    msg.sprintf( "Parse error at line %d, column %d (%s).",
                 exception.lineNumber(), exception.columnNumber(),
                 exception.message().toLatin1().data() );
    fprintf( stderr, "XML error: %s\n", msg.toLatin1().data() );
    return false;
}

void UiHandler::flush()
{
    if ( !context.isEmpty() && !source.isEmpty() )
        tor->insert( MetaTranslatorMessage(context.toUtf8(), source.toUtf8(),
                                           comment.toUtf8(), QString(fname), m_lineNumber,
                                           QStringList(), true) );
    source.truncate( 0 );
    comment.truncate( 0 );
}

void fetchtr_ui( const char *fileName, MetaTranslator *tor,
                 const char * /* defaultContext */, bool mustExist )
{
    QFile f( fileName );
    if ( !f.open(QIODevice::ReadOnly) ) {
        if ( mustExist ) {
#if defined(_MSC_VER) && _MSC_VER >= 1400
            char buf[100];
            strerror_s(buf, sizeof(buf), errno);
            fprintf( stderr, "pylupdate5 error: cannot open UI file '%s': %s\n",
                     fileName, buf );
#else
            fprintf( stderr, "pylupdate5 error: cannot open UI file '%s': %s\n",
                     fileName, strerror(errno) );
#endif
        }
        return;
    }

    QXmlInputSource in( &f );
    QXmlSimpleReader reader;
    reader.setFeature( "http://xml.org/sax/features/namespaces", false );
    reader.setFeature( "http://xml.org/sax/features/namespace-prefixes", true );
    reader.setFeature( "http://trolltech.com/xml/features/report-whitespace"
                       "-only-CharData", false );
    QXmlDefaultHandler *hand = new UiHandler( tor, fileName );
    reader.setContentHandler( hand );
    reader.setErrorHandler( hand );

    if ( !reader.parse(in) )
        fprintf( stderr, "%s: Parse error in UI file\n", fileName );
    reader.setContentHandler( 0 );
    reader.setErrorHandler( 0 );
    delete hand;
    f.close();
}

/*
 * The SIP lexer.
 *
 * Copyright (c) 2013 Riverbank Computing Limited <info@riverbankcomputing.com>
 *
 * This file is part of SIP.
 *
 * This copy of SIP is licensed for use under the terms of the SIP License
 * Agreement.  See the file LICENSE for more details.
 *
 * This copy of SIP may also used under the terms of the GNU General Public
 * License v2 or v3 as published by the Free Software Foundation which can be
 * found in the files LICENSE-GPL2 and LICENSE-GPL3 included in this package.
 *
 * SIP is supplied WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "sip.h"
#include "parser.h"


#ifndef FLEX_SCANNER
#error "Only flex is supported at the moment"
#endif


#define YY_NO_UNISTD_H
#define YY_FATAL_ERROR(s)       fatallex(s)

#define MAX_INCLUDE_DEPTH       10
#define MAX_CODE_LINE_LENGTH    1000


static struct inputFile {
    sourceLocation sloc;                /* The source location. */
    YY_BUFFER_STATE bs;                 /* The flex buffer state handle. */
    char *cwd;                          /* The path part of the file name. */
    parserContext pc;                   /* The parser context. */
} inputFileStack[MAX_INCLUDE_DEPTH];

static int currentFile = -1;            /* Index of the current input file. */
static char codeLine[MAX_CODE_LINE_LENGTH + 2]; /* The current code line. */
static int codeIdx = -1;                /* Index of next code character. */
static int parenDepth = 0;              /* The current depth of (...). */

static FILE *openFile(const char *);
static void fatallex(char *);
%}

%option stack

%x code
%x ccomment
%s directive
%s directive_start

%%

^[ \t]*%API                 {BEGIN directive_start; return TK_API;}
^[ \t]*%AutoPyName          {BEGIN directive_start; return TK_AUTOPYNAME;}
^[ \t]*%CModule             {return TK_CMODULE;}
^[ \t]*%CompositeModule     {BEGIN directive_start; return TK_COMPOMODULE;}
^[ \t]*%ConsolidatedModule  {BEGIN directive_start; return TK_CONSMODULE;}
^[ \t]*%DefaultDocstringFormat  {BEGIN directive_start; return TK_DEFDOCSTRING;}
^[ \t]*%DefaultEncoding     {BEGIN directive_start; return TK_DEFENCODING;}
^[ \t]*%DefaultMetatype     {BEGIN directive_start; return TK_DEFMETATYPE;}
^[ \t]*%DefaultSupertype    {BEGIN directive_start; return TK_DEFSUPERTYPE;}
<INITIAL>^[ \t]*%End        {return TK_END;}
<directive_start>^[ \t]*%End    {BEGIN INITIAL; return TK_END;}
^[ \t]*%Exception           {return TK_EXCEPTION;}
^[ \t]*%Feature             {BEGIN directive_start; return TK_FEATURE;}
^[ \t]*%If                  {return TK_IF;}
^[ \t]*%Import              {BEGIN directive_start; return TK_IMPORT;}
^[ \t]*%Include             {BEGIN directive_start; return TK_INCLUDE;}
^[ \t]*%License             {BEGIN directive_start; return TK_LICENSE;}
^[ \t]*%MappedType          {return TK_MAPPEDTYPE;}
^[ \t]*%Module              {BEGIN directive_start; return TK_MODULE;}
^[ \t]*%OptionalInclude     {return TK_OPTINCLUDE;}
^[ \t]*%Platforms           {return TK_PLATFORMS;}
^[ \t]*%Plugin              {BEGIN directive_start; return TK_PLUGIN;}
^[ \t]*%Property            {BEGIN directive_start; return TK_PROPERTY;}
^[ \t]*%Timeline            {return TK_TIMELINE;}

class                       {return TK_CLASS;}
struct                      {return TK_STRUCT;}
public                      {return TK_PUBLIC;}
protected                   {return TK_PROTECTED;}
private                     {return TK_PRIVATE;}
signals                     {return TK_SIGNALS;}
Q_SIGNALS                   {return TK_SIGNALS;}
Q_SIGNAL                    {return TK_SIGNAL_METHOD;}
slots                       {return TK_SLOTS;}
Q_SLOTS                     {return TK_SLOTS;}
Q_SLOT                      {return TK_SLOT_METHOD;}
char                        {return TK_CHAR;}
wchar_t                     {return TK_WCHAR_T;}
bool                        {return TK_BOOL;}
short                       {return TK_SHORT;}
int                         {return TK_INT;}
long                        {return TK_LONG;}
float                       {return TK_FLOAT;}
double                      {return TK_DOUBLE;}
void                        {return TK_VOID;}
virtual                     {return TK_VIRTUAL;}
enum                        {return TK_ENUM;}
signed                      {return TK_SIGNED;}
unsigned                    {return TK_UNSIGNED;}
const                       {return TK_CONST;}
static                      {return TK_STATIC;}
true                        {return TK_TRUE_VALUE;}
false                       {return TK_FALSE_VALUE;}
NULL                        {return TK_NULL_VALUE;}
typedef                     {return TK_TYPEDEF;}
namespace                   {return TK_NAMESPACE;}
operator                    {return TK_OPERATOR;}
throw                       {return TK_THROW;}
explicit                    {return TK_EXPLICIT;}
template                    {return TK_TEMPLATE;}
::                          {return TK_SCOPE;}
\|\|                        {return TK_LOGICAL_OR;}
SIP_PYOBJECT                {return TK_PYOBJECT;}
SIP_PYTUPLE                 {return TK_PYTUPLE;}
SIP_PYLIST                  {return TK_PYLIST;}
SIP_PYDICT                  {return TK_PYDICT;}
SIP_PYCALLABLE              {return TK_PYCALLABLE;}
SIP_PYSLICE                 {return TK_PYSLICE;}
SIP_PYTYPE                  {return TK_PYTYPE;}
SIP_PYBUFFER                {return TK_PYBUFFER;}
SIP_SIGNAL                  {return TK_SIPSIGNAL;}
SIP_SLOT                    {return TK_SIPSLOT;}
SIP_ANYSLOT                 {return TK_SIPANYSLOT;}
SIP_RXOBJ_CON               {return TK_SIPRXCON;}
SIP_RXOBJ_DIS               {return TK_SIPRXDIS;}
SIP_SLOT_CON                {return TK_SIPSLOTCON;}
SIP_SLOT_DIS                {return TK_SIPSLOTDIS;}
SIP_SSIZE_T                 {return TK_SIPSSIZET;}
SIP_QOBJECT                 {return TK_QOBJECT;}
\.\.\.                      {return TK_ELLIPSIS;}

<directive>format           {return TK_FORMAT;}
<directive>get              {return TK_GET;}
<directive>id               {return TK_ID;}
<directive>keyword_arguments    {return TK_KWARGS;}
<directive>language         {return TK_LANGUAGE;}
<directive>licensee         {return TK_LICENSEE;}
<directive>name             {return TK_NAME;}
<directive>optional         {return TK_OPTIONAL;}
<directive>order            {return TK_ORDER;}
<directive>remove_leading   {return TK_REMOVELEADING;}
<directive>set              {return TK_SET;}
<directive>signature        {return TK_SIGNATURE;}
<directive>timestamp        {return TK_TIMESTAMP;}
<directive>type             {return TK_TYPE;}
<directive>use_argument_names   {return TK_USEARGNAMES;}
<directive>all_raise_py_exception   {return TK_ALLRAISEPYEXC;}
<directive>call_super_init  {return TK_CALLSUPERINIT;}
<directive>default_VirtualErrorHandler  {return TK_DEFERRORHANDLER;}
<directive>version          {return TK_VERSION;}

<directive>True             {return TK_TRUE_VALUE;}
<directive>False            {return TK_FALSE_VALUE;}


[ \t\r] {
    /* Ignore whitespace. */
    ;
}

<directive_start>\( {
    /*
     * Maintain the parenthesis depth so that we don't enter the 'code' state
     * until any arguments have been parsed.
     */
    ++parenDepth;

    BEGIN directive;

    return '(';
}

<directive>\) {
    /* Maintain the parenthesis depth. */
    --parenDepth;

    BEGIN INITIAL;

    return ')';
}

\n {
    /* Maintain the line number. */
    ++inputFileStack[currentFile].sloc.linenr;

    if (codeIdx == 0 && parenDepth == 0)
    {
        BEGIN code;
    }
}

\/\/.* {
    /* Ignore C++ style comments. */
    ;
}


-?[0-9]+[lLuU]? {
    /* A signed decimal number. */
    yylval.number = strtol(yytext,NULL,0);
    return TK_NUMBER_VALUE;
}


-?([0-9]+(\.[0-9]*)?|\.[0-9]+)([eE][-+]?[0-9]+)?[fF]? {
    /* A floating point number. */
    yylval.real = strtod(yytext,NULL);
    return TK_REAL_VALUE;
}


0x[0-9a-fA-F]+ {
    /* An unsigned hexadecimal number. */
    yylval.number = strtol(yytext,NULL,16);
    return TK_NUMBER_VALUE;
}


[_A-Za-z][_A-Za-z0-9]* {
    /* An identifier name. */
    yylval.text = sipStrdup(yytext);
    return TK_NAME_VALUE;
}


[._A-Za-z][._/A-Za-z0-9\-]*[._A-Za-z0-9] {
    /* A relative pathname. */
    yylval.text = sipStrdup(yytext);
    return TK_PATH_VALUE;
}


\"[^"\n]*["\n] {
    /* A double-quoted string. */
    char *dp, *sp;

    /* Copy the string without the quotes. */

    yylval.text = sipMalloc(strlen(yytext) + 1);

    dp = yylval.text;
    sp = yytext;

    while (*sp != '\0')
    {
        if (*sp != '"')
            *dp++ = *sp;

        ++sp;
    }

    *dp = '\0';

    return TK_STRING_VALUE;
}


\'[^'\n]*['\n] {
    /* A single-quoted character. */
    if (strlen(yytext) != 3)
        fatallex("Exactly one character expected between single quotes");

    yylval.qchar = yytext[1];

    return TK_QCHAR_VALUE;
}


\/\* {
    /* Ignore C-style comments. */
    yy_push_state(ccomment);
}
<ccomment>\n {
    ++inputFileStack[currentFile].sloc.linenr;
}
<ccomment>\*\/ {
    yy_pop_state();
}
<ccomment>. {
    ;
}


^[ \t]*%Copying {
    /* The software license. */
    codeIdx = 0;
    return TK_COPYING;
}

^[ \t]*%ConvertFromTypeCode {
    /* The start of a from-type code block. */
    codeIdx = 0;
    return TK_FROMTYPE;
}

^[ \t]*%ConvertToTypeCode {
    /* The start of a to-type code block. */
    codeIdx = 0;
    return TK_TOTYPE;
}

^[ \t]*%ConvertToSubClassCode {
    /* The start of a to-sub-class code block. */
    codeIdx = 0;
    return TK_TOSUBCLASS;
}

^[ \t]*%ExportedHeaderCode {
    /* The start of an exported header code block. */
    codeIdx = 0;
    return TK_EXPHEADERCODE;
}

^[ \t]*%Extract {
    /* The start of part of an extract. */
    codeIdx = 0;

    BEGIN directive_start;

    return TK_EXTRACT;
}

^[ \t]*%ModuleHeaderCode {
    /* The start of a module header code block. */
    codeIdx = 0;
    return TK_MODHEADERCODE;
}

^[ \t]*%TypeHeaderCode {
    /* The start of a type header code block. */
    codeIdx = 0;
    return TK_TYPEHEADERCODE;
}

^[ \t]*%PreInitialisationCode {
    /* The start of a pre-initialisation code block. */
    codeIdx = 0;
    return TK_PREINITCODE;
}

^[ \t]*%InitialisationCode {
    /* The start of an initialisation code block. */
    codeIdx = 0;
    return TK_INITCODE;
}

^[ \t]*%PostInitialisationCode {
    /* The start of a post-initialisation code block. */
    codeIdx = 0;
    return TK_POSTINITCODE;
}

^[ \t]*%FinalisationCode {
    /* The start of a class finalisation code block. */
    codeIdx = 0;
    return TK_FINALCODE;
}

^[ \t]*%UnitCode {
    /* The start of a unit code block. */
    codeIdx = 0;
    return TK_UNITCODE;
}

^[ \t]*%UnitPostIncludeCode {
    /* The start of a unit post-include code block. */
    codeIdx = 0;
    return TK_UNITPOSTINCLUDECODE;
}

^[ \t]*%ModuleCode {
    /* The start of a module code block. */
    codeIdx = 0;
    return TK_MODCODE;
}

^[ \t]*%TypeCode {
    /* The start of a type code block. */
    codeIdx = 0;
    return TK_TYPECODE;
}

^[ \t]*%MethodCode {
    /* The start of a C++ method code block. */
    codeIdx = 0;
    return TK_METHODCODE;
}

^[ \t]*%VirtualCatcherCode {
    /* The start of a C++ virtual code block. */
    codeIdx = 0;
    return TK_VIRTUALCATCHERCODE;
}

^[ \t]*%GCTraverseCode {
    /* The start of a traverse code block. */
    codeIdx = 0;
    return TK_TRAVERSECODE;
}

^[ \t]*%GCClearCode {
    /* The start of a clear code block. */
    codeIdx = 0;
    return TK_CLEARCODE;
}

^[ \t]*%BIGetBufferCode {
    /* The start of a get buffer code block. */
    codeIdx = 0;
    return TK_GETBUFFERCODE;
}

^[ \t]*%BIReleaseBufferCode {
    /* The start of a release buffer code block. */
    codeIdx = 0;
    return TK_RELEASEBUFFERCODE;
}

^[ \t]*%BIGetReadBufferCode {
    /* The start of a read buffer code block. */
    codeIdx = 0;
    return TK_READBUFFERCODE;
}

^[ \t]*%BIGetWriteBufferCode {
    /* The start of a write buffer code block. */
    codeIdx = 0;
    return TK_WRITEBUFFERCODE;
}

^[ \t]*%BIGetSegCountCode {
    /* The start of a segment count code block. */
    codeIdx = 0;
    return TK_SEGCOUNTCODE;
}

^[ \t]*%BIGetCharBufferCode {
    /* The start of a char buffer code block. */
    codeIdx = 0;
    return TK_CHARBUFFERCODE;
}

^[ \t]*%InstanceCode {
    /* The start of a create instance code block. */
    codeIdx = 0;
    return TK_INSTANCECODE;
}

^[ \t]*%PickleCode {
    /* The start of a pickle code block. */
    codeIdx = 0;
    return TK_PICKLECODE;
}

^[ \t]*%PrePythonCode {
    /* The start of a pre-Python code block. */
    deprecated("%PrePythonCode is deprecated");

    codeIdx = 0;
    return TK_PREPYCODE;
}

^[ \t]*%RaiseCode {
    /* The start of a raise Python exception code block. */
    codeIdx = 0;
    return TK_RAISECODE;
}

^[ \t]*%Docstring {
    /* The start of a docstring block. */
    codeIdx = 0;
    return TK_DOCSTRING;
}

^[ \t]*%Doc {
    /* The start of a documentation block. */
    deprecated("%Doc is deprecated, use %Extract instead");

    codeIdx = 0;
    return TK_DOC;
}

^[ \t]*%ExportedDoc {
    /* The start of an exported documentation block. */
    deprecated("%ExportedDoc is deprecated, use %Extract instead");

    codeIdx = 0;
    return TK_EXPORTEDDOC;
}

^[ \t]*%Makefile {
    /* The start of a Makefile code block. */
    deprecated("%Makefile is deprecated");

    codeIdx = 0;
    return TK_MAKEFILE;
}

^[ \t]*%AccessCode {
    /* The start of an access code block. */
    codeIdx = 0;
    return TK_ACCESSCODE;
}

^[ \t]*%GetCode {
    /* The start of a get code block. */
    codeIdx = 0;
    return TK_GETCODE;
}

^[ \t]*%SetCode {
    /* The start of a set code block. */
    codeIdx = 0;
    return TK_SETCODE;
}

^[ \t]*%VirtualErrorHandler {
    /* The start of part of a virtual error handler. */
    codeIdx = 0;

    BEGIN directive_start;

    return TK_VIRTERRORHANDLER;
}

<code>^[ \t]*%End {
    /* The end of a code block. */
    BEGIN INITIAL;
    codeIdx = -1;
    return TK_END;
}

<code>\n {
    /* The end of a code line . */
    struct inputFile *ifp;

    codeLine[codeIdx] = '\n';
    codeLine[codeIdx + 1] = '\0';
    codeIdx = 0;

    ifp = &inputFileStack[currentFile];

    yylval.codeb = sipMalloc(sizeof (codeBlock));

    yylval.codeb->frag = sipStrdup(codeLine);
    yylval.codeb->linenr = ifp->sloc.linenr++;
    yylval.codeb->filename = ifp->sloc.name;

    return TK_CODELINE;
}

<code>. {
    /* The contents of a code line. */
    if (codeIdx == MAX_CODE_LINE_LENGTH)
        fatallex("Line is too long");

    codeLine[codeIdx++] = yytext[0];
}

. {
    /* Anything else is returned as is. */
    return yytext[0];
}

%%

/*
 * Hook into EOF handling.  Return 0 if there is more to process.
 */

int yywrap()
{
    char *cwd;
    struct inputFile *ifp;

    if ((cwd = inputFileStack[currentFile].cwd) != NULL)
        free(cwd);

    ifp = &inputFileStack[currentFile--];

    /* Tell the parser if this is the end of a file. */
    parserEOF(ifp->sloc.name, &ifp->pc);

    /*
     * Tidy up this file.  Note that we don't free the filename as it will
     * still be referenced for use in error messages.
     */
    fclose(yyin);

    /* See if this was the original file. */
    if (currentFile < 0)
        return 1;

    yy_delete_buffer(YY_CURRENT_BUFFER);
    yy_switch_to_buffer(ifp->bs);

    return 0;
}


/*
 * Get the current source location.
 */
void getSourceLocation(sourceLocation *slp)
{
    int cf;

    /*
     * Assume that we will already have gone past the newline but watch out in
     * case we have gone past the end of the file.
     */
    if ((cf = currentFile) < 0)
    {
        /* This will be valid if we have been called. */
        cf = 0;
    }

    slp->linenr = inputFileStack[cf].sloc.linenr - 1;
    slp->name = inputFileStack[cf].sloc.name;
}


/*
 * Set up an input file to be read by the lexer, opening it if necessary.  TRUE
 * is returned if the file has not already been read.
 */
int setInputFile(FILE *open_fp, parserContext *pc, int optional)
{
    static stringList *all = NULL;
    char *cwd, *fullname = NULL;
    FILE *fp = open_fp;

    if (currentFile >= MAX_INCLUDE_DEPTH - 1)
        fatal("Too many nested %%Include, %%OptionalInclude or %%Import statements\n");

    if (fp != NULL || (fp = openFile(pc->filename)) != NULL)
        fullname = sipStrdup(pc->filename);
    else
    {
        char *cwd;

        /* Try the directory that contains the current file. */
        if (currentFile >= 0 && (cwd = inputFileStack[currentFile].cwd) != NULL)
        {
            fullname = concat(cwd, "/", pc->filename, NULL);

            if ((fp = openFile(fullname)) == NULL)
            {
                free(fullname);
                fullname = NULL;
            }
        }
    }

    /* Try the include path if we haven't found anything yet. */
    if (fullname == NULL)
    {
        stringList *sl;

        fullname = NULL;

        for (sl = includeDirList; sl != NULL; sl = sl -> next)
        {
            if (fullname != NULL)
                free(fullname);

            fullname = concat(sl->s, "/", pc->filename, NULL);

            if ((fp = openFile(fullname)) != NULL)
                break;
        }

        if (fp == NULL)
        {
            if (optional)
                return FALSE;

            fatal("Unable to find file \"%s\"\n", pc->filename);
        }
    }

    /*
     * If we have just opened the file, make sure that we haven't already read
     * it.  While it should never happen with normal modules (if the user
     * doesn't specify recursive %Imports or %Includes) it is likely to happen
     * with consolidated modules.
     */
    if (open_fp == NULL)
    {
        stringList *sl;

        for (sl = all; sl != NULL; sl = sl->next)
            if (strcmp(sl->s, fullname) == 0)
            {
                fclose(fp);
                return FALSE;
            }
    }

    /* Remember the filename. */
    appendString(&all, sipStrdup(fullname));

    yyin = fp;

    ++currentFile;

    /* Remember the directory containing the new file and make it "current". */
    if ((cwd = strchr(fullname, '/')) != NULL)
    {
        cwd = sipStrdup(fullname);
        *strrchr(cwd,'/') = '\0';
    }

    inputFileStack[currentFile].sloc.linenr = 1;
    inputFileStack[currentFile].sloc.name = fullname;
    inputFileStack[currentFile].pc = *pc;
    inputFileStack[currentFile].cwd = cwd;

    if (currentFile > 0)
    {
        inputFileStack[currentFile].bs = YY_CURRENT_BUFFER;
        yy_switch_to_buffer(yy_create_buffer(yyin,YY_BUF_SIZE));
    }

    return TRUE;
}


/*
 * Open a file for reading or return NULL if it doesn't exist.  Any other error
 * is fatal.
 */
static FILE *openFile(const char *name)
{
    FILE *fp;

    if ((fp = fopen(name,"r")) == NULL && errno != ENOENT)
        fatal("Error in opening file %s\n",name);

    return fp;
}


/*
 * Handle fatal yacc errors.
 */
void yyerror(char *s)
{
    if (currentFile < 0)
        fatal("%s\n", s);

    fatal("%s:%d: %s\n", inputFileStack[currentFile].sloc.name,
            inputFileStack[currentFile].sloc.linenr, s);
}


/*
 * Handle warnings while parsing.
 */
void yywarning(char *s)
{
    warning(ParserWarning, "%s:%d: %s\n",
            inputFileStack[currentFile].sloc.name,
            inputFileStack[currentFile].sloc.linenr, s);
}


/*
 * Handle deprecation warnings.
 */
void deprecated(const char *msg)
{
    warning(DeprecationWarning, "%s:%d: %s\n",
            inputFileStack[currentFile].sloc.name,
            inputFileStack[currentFile].sloc.linenr, msg);
}


/*
 * Handle fatal lex errors.
 */
static void fatallex(char *s)
{
    fatal("%s:%d: Lexical analyser error: %s\n",
        inputFileStack[currentFile].sloc.name,
        inputFileStack[currentFile].sloc.linenr,
        s);
}


/*
 * Reset the lexer state to INITIAL.  This is used by directives that allow an
 * argument to be given without parentheses to get out of the 'directive_start'
 * state before an opening parenthesis is seen in another context.
 */
void resetLexerState()
{
    BEGIN INITIAL;
}

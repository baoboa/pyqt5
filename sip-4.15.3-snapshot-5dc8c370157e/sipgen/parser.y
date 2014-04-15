/*
 * The SIP parser.
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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "sip.h"


#define MAX_NESTED_IF       10
#define MAX_NESTED_SCOPE    10

#define inMainModule()      (currentSpec->module == currentModule || currentModule->container != NULL)


static sipSpec *currentSpec;            /* The current spec being parsed. */
static stringList *neededQualifiers;    /* The list of required qualifiers. */
static stringList *excludedQualifiers;  /* The list of excluded qualifiers. */
static moduleDef *currentModule;        /* The current module being parsed. */
static mappedTypeDef *currentMappedType;    /* The current mapped type. */
static enumDef *currentEnum;            /* The current enum being parsed. */
static int sectionFlags;                /* The current section flags. */
static int currentOverIsVirt;           /* Set if the overload is virtual. */
static int currentCtorIsExplicit;       /* Set if the ctor is explicit. */
static int currentIsStatic;             /* Set if the current is static. */
static int currentIsSignal;             /* Set if the current is Q_SIGNAL. */
static int currentIsSlot;               /* Set if the current is Q_SLOT. */
static int currentIsTemplate;           /* Set if the current is a template. */
static char *previousFile;              /* The file just parsed. */
static parserContext currentContext;    /* The current context. */
static int skipStackPtr;                /* The skip stack pointer. */
static int skipStack[MAX_NESTED_IF];    /* Stack of skip flags. */
static classDef *scopeStack[MAX_NESTED_SCOPE];  /* The scope stack. */
static int sectFlagsStack[MAX_NESTED_SCOPE];    /* The section flags stack. */
static int currentScopeIdx;             /* The scope stack index. */
static int currentTimelineOrder;        /* The current timeline order. */
static classList *currentSupers;        /* The current super-class list. */
static KwArgs defaultKwArgs;            /* The default keyword arguments support. */
static int makeProtPublic;              /* Treat protected items as public. */
static int parsingCSignature;           /* An explicit C/C++ signature is being parsed. */


static const char *getPythonName(moduleDef *mod, optFlags *optflgs,
        const char *cname);
static classDef *findClass(sipSpec *pt, ifaceFileType iftype,
        apiVersionRangeDef *api_range, scopedNameDef *fqname);
static classDef *findClassWithInterface(sipSpec *pt, ifaceFileDef *iff);
static classDef *newClass(sipSpec *pt, ifaceFileType iftype,
        apiVersionRangeDef *api_range, scopedNameDef *snd,
        const char *virt_error_handler);
static void finishClass(sipSpec *, moduleDef *, classDef *, optFlags *);
static exceptionDef *findException(sipSpec *pt, scopedNameDef *fqname, int new);
static mappedTypeDef *newMappedType(sipSpec *,argDef *, optFlags *);
static enumDef *newEnum(sipSpec *pt, moduleDef *mod, mappedTypeDef *mt_scope,
        char *name, optFlags *of, int flags);
static void instantiateClassTemplate(sipSpec *pt, moduleDef *mod,
        classDef *scope, scopedNameDef *fqname, classTmplDef *tcd,
        templateDef *td, const char *pyname);
static void newTypedef(sipSpec *, moduleDef *, char *, argDef *, optFlags *);
static void newVar(sipSpec *pt, moduleDef *mod, char *name, int isstatic,
        argDef *type, optFlags *of, codeBlock *acode, codeBlock *gcode,
        codeBlock *scode, int section);
static void newCtor(moduleDef *, char *, int, signatureDef *, optFlags *,
        codeBlock *, throwArgs *, signatureDef *, int, codeBlock *);
static void newFunction(sipSpec *, moduleDef *, classDef *, mappedTypeDef *,
        int, int, int, int, int, char *, signatureDef *, int, int, optFlags *,
        codeBlock *, codeBlock *, throwArgs *, signatureDef *, codeBlock *);
static optFlag *findOptFlag(optFlags *flgs, const char *name);
static optFlag *getOptFlag(optFlags *flgs, const char *name, flagType ft);
static memberDef *findFunction(sipSpec *, moduleDef *, classDef *,
        mappedTypeDef *, const char *, int, int, int);
static void checkAttributes(sipSpec *, moduleDef *, classDef *,
        mappedTypeDef *, const char *, int);
static void newModule(FILE *fp, const char *filename);
static moduleDef *allocModule();
static void parseFile(FILE *fp, const char *name, moduleDef *prevmod,
        int optional);
static void handleEOF(void);
static void handleEOM(void);
static qualDef *findQualifier(const char *name);
static const char *getInt(const char *cp, int *ip);
static scopedNameDef *text2scopedName(ifaceFileDef *scope, char *text);
static scopedNameDef *scopeScopedName(ifaceFileDef *scope,
        scopedNameDef *name);
static void pushScope(classDef *);
static void popScope(void);
static classDef *currentScope(void);
static void newQualifier(moduleDef *, int, int, const char *, qualType);
static qualDef *allocQualifier(moduleDef *, int, int, const char *, qualType);
static void newImport(const char *filename);
static int timePeriod(const char *lname, const char *uname);
static int platOrFeature(char *,int);
static int notSkipping(void);
static void getHooks(optFlags *,char **,char **);
static int getTransfer(optFlags *optflgs);
static int getReleaseGIL(optFlags *optflgs);
static int getHoldGIL(optFlags *optflgs);
static int getDeprecated(optFlags *optflgs);
static int getAllowNone(optFlags *optflgs);
static const char *getVirtErrorHandler(optFlags *optflgs);
static const char *getDocType(optFlags *optflgs);
static const char *getDocValue(optFlags *optflgs);
static void templateSignature(signatureDef *sd, int result, classTmplDef *tcd, templateDef *td, classDef *ncd);
static void templateType(argDef *ad, classTmplDef *tcd, templateDef *td, classDef *ncd);
static int search_back(const char *end, const char *start, const char *target);
static char *type2string(argDef *ad);
static char *scopedNameToString(scopedNameDef *name);
static void addUsedFromCode(sipSpec *pt, ifaceFileList **used, const char *sname);
static int sameName(scopedNameDef *snd, const char *sname);
static int stringFind(stringList *sl, const char *s);
static void setModuleName(sipSpec *pt, moduleDef *mod, const char *fullname);
static int foundInScope(scopedNameDef *fq_name, scopedNameDef *rel_name);
static void defineClass(scopedNameDef *snd, classList *supers, optFlags *of);
static classDef *completeClass(scopedNameDef *snd, optFlags *of, int has_def);
static memberDef *instantiateTemplateMethods(memberDef *tmd, moduleDef *mod);
static void instantiateTemplateEnums(sipSpec *pt, classTmplDef *tcd,
        templateDef *td, classDef *cd, ifaceFileList **used,
        scopedNameDef *type_names, scopedNameDef *type_values);
static void instantiateTemplateVars(sipSpec *pt, classTmplDef *tcd,
        templateDef *td, classDef *cd, ifaceFileList **used,
        scopedNameDef *type_names, scopedNameDef *type_values);
static void instantiateTemplateTypedefs(sipSpec *pt, classTmplDef *tcd,
        templateDef *td, classDef *cd);
static overDef *instantiateTemplateOverloads(sipSpec *pt, overDef *tod,
        memberDef *tmethods, memberDef *methods, classTmplDef *tcd,
        templateDef *td, classDef *cd, ifaceFileList **used,
        scopedNameDef *type_names, scopedNameDef *type_values);
static void resolveAnyTypedef(sipSpec *pt, argDef *ad);
static void addTypedef(sipSpec *pt, typedefDef *tdd);
static void addVariable(sipSpec *pt, varDef *vd);
static void applyTypeFlags(moduleDef *mod, argDef *ad, optFlags *flags);
static Format convertFormat(const char *format);
static argType convertEncoding(const char *encoding);
static apiVersionRangeDef *getAPIRange(optFlags *optflgs);
static apiVersionRangeDef *convertAPIRange(moduleDef *mod, nameDef *name,
        int from, int to);
static char *convertFeaturedString(char *fs);
static scopedNameDef *text2scopePart(char *text);
static KwArgs keywordArgs(moduleDef *mod, optFlags *optflgs, signatureDef *sd,
        int need_name);
static char *strip(char *s);
static int isEnabledFeature(const char *name);
static void addProperty(sipSpec *pt, moduleDef *mod, classDef *cd,
        const char *name, const char *get, const char *set,
        codeBlock *docstring);
static moduleDef *configureModule(sipSpec *pt, moduleDef *module,
        const char *filename, const char *name, int version, int c_module,
        KwArgs kwargs, int use_arg_names, int call_super_init,
        int all_raise_py_exc, const char *def_error_handler,
        codeBlock *docstring);
static void addAutoPyName(moduleDef *mod, const char *remove_leading);
static KwArgs convertKwArgs(const char *kwargs);
static void checkAnnos(optFlags *annos, const char *valid[]);
static void checkNoAnnos(optFlags *annos, const char *msg);
static void appendCodeBlock(codeBlockList **headp, codeBlock *cb);
static void handleKeepReference(optFlags *optflgs, argDef *ad, moduleDef *mod);
static void mappedTypeAnnos(mappedTypeDef *mtd, optFlags *optflgs);
static void add_new_deref(argDef *new, argDef *orig, int isconst);
static void add_derefs(argDef *dst, argDef *src);
%}

%union {
    char            qchar;
    char            *text;
    long            number;
    double          real;
    argDef          memArg;
    signatureDef    signature;
    signatureDef    *optsignature;
    throwArgs       *throwlist;
    codeBlock       *codeb;
    valueDef        value;
    valueDef        *valp;
    optFlags        optflags;
    optFlag         flag;
    scopedNameDef   *scpvalp;
    fcallDef        fcall;
    int             boolean;
    exceptionDef    exceptionbase;
    classDef        *klass;
    apiCfg          api;
    autoPyNameCfg   autopyname;
    compModuleCfg   compmodule;
    consModuleCfg   consmodule;
    defDocstringCfg defdocstring;
    defEncodingCfg  defencoding;
    defMetatypeCfg  defmetatype;
    defSupertypeCfg defsupertype;
    exceptionCfg    exception;
    docstringCfg    docstring;
    extractCfg      extract;
    featureCfg      feature;
    licenseCfg      license;
    importCfg       import;
    includeCfg      include;
    moduleCfg       module;
    pluginCfg       plugin;
    propertyCfg     property;
    variableCfg     variable;
    vehCfg          veh;
    int             token;
}

%token          TK_API
%token          TK_AUTOPYNAME
%token          TK_DEFDOCSTRING
%token          TK_DEFENCODING
%token          TK_PLUGIN
%token          TK_VIRTERRORHANDLER
%token          TK_DOCSTRING
%token          TK_DOC
%token          TK_EXPORTEDDOC
%token          TK_EXTRACT
%token          TK_MAKEFILE
%token          TK_ACCESSCODE
%token          TK_GETCODE
%token          TK_SETCODE
%token          TK_PREINITCODE
%token          TK_INITCODE
%token          TK_POSTINITCODE
%token          TK_FINALCODE
%token          TK_UNITCODE
%token          TK_UNITPOSTINCLUDECODE
%token          TK_MODCODE
%token          TK_TYPECODE
%token          TK_PREPYCODE
%token          TK_COPYING
%token          TK_MAPPEDTYPE
%token <codeb>  TK_CODELINE
%token          TK_IF
%token          TK_END
%token <text>   TK_NAME_VALUE
%token <text>   TK_PATH_VALUE
%token <text>   TK_STRING_VALUE
%token          TK_VIRTUALCATCHERCODE
%token          TK_TRAVERSECODE
%token          TK_CLEARCODE
%token          TK_GETBUFFERCODE
%token          TK_RELEASEBUFFERCODE
%token          TK_READBUFFERCODE
%token          TK_WRITEBUFFERCODE
%token          TK_SEGCOUNTCODE
%token          TK_CHARBUFFERCODE
%token          TK_PICKLECODE
%token          TK_METHODCODE
%token          TK_INSTANCECODE
%token          TK_FROMTYPE
%token          TK_TOTYPE
%token          TK_TOSUBCLASS
%token          TK_INCLUDE
%token          TK_OPTINCLUDE
%token          TK_IMPORT
%token          TK_EXPHEADERCODE
%token          TK_MODHEADERCODE
%token          TK_TYPEHEADERCODE
%token          TK_MODULE
%token          TK_CMODULE
%token          TK_CONSMODULE
%token          TK_COMPOMODULE
%token          TK_CLASS
%token          TK_STRUCT
%token          TK_PUBLIC
%token          TK_PROTECTED
%token          TK_PRIVATE
%token          TK_SIGNALS
%token          TK_SIGNAL_METHOD
%token          TK_SLOTS
%token          TK_SLOT_METHOD
%token          TK_BOOL
%token          TK_SHORT
%token          TK_INT
%token          TK_LONG
%token          TK_FLOAT
%token          TK_DOUBLE
%token          TK_CHAR
%token          TK_WCHAR_T
%token          TK_VOID
%token          TK_PYOBJECT
%token          TK_PYTUPLE
%token          TK_PYLIST
%token          TK_PYDICT
%token          TK_PYCALLABLE
%token          TK_PYSLICE
%token          TK_PYTYPE
%token          TK_PYBUFFER
%token          TK_VIRTUAL
%token          TK_ENUM
%token          TK_SIGNED
%token          TK_UNSIGNED
%token          TK_SCOPE
%token          TK_LOGICAL_OR
%token          TK_CONST
%token          TK_STATIC
%token          TK_SIPSIGNAL
%token          TK_SIPSLOT
%token          TK_SIPANYSLOT
%token          TK_SIPRXCON
%token          TK_SIPRXDIS
%token          TK_SIPSLOTCON
%token          TK_SIPSLOTDIS
%token          TK_SIPSSIZET
%token <number> TK_NUMBER_VALUE
%token <real>   TK_REAL_VALUE
%token          TK_TYPEDEF
%token          TK_NAMESPACE
%token          TK_TIMELINE
%token          TK_PLATFORMS
%token          TK_FEATURE
%token          TK_LICENSE
%token <qchar>  TK_QCHAR_VALUE
%token          TK_TRUE_VALUE
%token          TK_FALSE_VALUE
%token          TK_NULL_VALUE
%token          TK_OPERATOR
%token          TK_THROW
%token          TK_QOBJECT
%token          TK_EXCEPTION
%token          TK_RAISECODE
%token          TK_VIRTERRORCODE
%token          TK_EXPLICIT
%token          TK_TEMPLATE
%token          TK_ELLIPSIS
%token          TK_DEFMETATYPE
%token          TK_DEFSUPERTYPE
%token          TK_PROPERTY

%token          TK_FORMAT
%token          TK_GET
%token          TK_ID
%token          TK_KWARGS
%token          TK_LANGUAGE
%token          TK_LICENSEE
%token          TK_NAME
%token          TK_OPTIONAL
%token          TK_ORDER
%token          TK_REMOVELEADING
%token          TK_SET
%token          TK_SIGNATURE
%token          TK_TIMESTAMP
%token          TK_TYPE
%token          TK_USEARGNAMES
%token          TK_ALLRAISEPYEXC
%token          TK_CALLSUPERINIT
%token          TK_DEFERRORHANDLER
%token          TK_VERSION

%type <memArg>          argvalue
%type <memArg>          argtype
%type <memArg>          cpptype
%type <memArg>          basetype
%type <memArg>          deref
%type <signature>       template
%type <signature>       arglist
%type <signature>       rawarglist
%type <signature>       cpptypelist
%type <optsignature>    optsig
%type <optsignature>    optctorsig
%type <throwlist>       optexceptions
%type <throwlist>       exceptionlist
%type <number>          optslot
%type <number>          optref
%type <number>          optconst
%type <number>          optvirtual
%type <number>          optabstract
%type <number>          optnumber
%type <value>           simplevalue
%type <valp>            value
%type <valp>            expr
%type <valp>            optassign
%type <codeb>           optaccesscode
%type <codeb>           optgetcode
%type <codeb>           optsetcode
%type <codeb>           typehdrcode
%type <codeb>           travcode
%type <codeb>           clearcode
%type <codeb>           getbufcode
%type <codeb>           releasebufcode
%type <codeb>           readbufcode
%type <codeb>           writebufcode
%type <codeb>           segcountcode
%type <codeb>           charbufcode
%type <codeb>           picklecode
%type <codeb>           finalcode
%type <codeb>           typecode
%type <codeb>           codeblock
%type <codeb>           codelines
%type <codeb>           virtualcatchercode
%type <codeb>           methodcode
%type <codeb>           instancecode
%type <codeb>           raisecode
%type <codeb>           docstring
%type <codeb>           optdocstring
%type <text>            operatorname
%type <text>            optfilename
%type <text>            optname
%type <text>            dottedname
%type <text>            name_or_string
%type <optflags>        optflags
%type <optflags>        flaglist
%type <flag>            flag
%type <flag>            flagvalue
%type <qchar>           optunop
%type <qchar>           binop
%type <scpvalp>         scopepart
%type <scpvalp>         scopedname
%type <scpvalp>         optcast
%type <fcall>           exprlist
%type <boolean>         qualifiers
%type <boolean>         oredqualifiers
%type <boolean>         optclassbody
%type <boolean>         bool_value
%type <exceptionbase>   baseexception
%type <klass>           class
%type <token>           class_access

%type <api>             api_args
%type <api>             api_arg_list
%type <api>             api_arg

%type <autopyname>      autopyname_args
%type <autopyname>      autopyname_arg_list
%type <autopyname>      autopyname_arg

%type <compmodule>      compmodule_args
%type <compmodule>      compmodule_arg_list
%type <compmodule>      compmodule_arg
%type <compmodule>      compmodule_body
%type <compmodule>      compmodule_body_directives
%type <compmodule>      compmodule_body_directive

%type <consmodule>      consmodule_args
%type <consmodule>      consmodule_arg_list
%type <consmodule>      consmodule_arg
%type <consmodule>      consmodule_body
%type <consmodule>      consmodule_body_directives
%type <consmodule>      consmodule_body_directive

%type <defdocstring>    defdocstring_args
%type <defdocstring>    defdocstring_arg_list
%type <defdocstring>    defdocstring_arg

%type <defencoding>     defencoding_args
%type <defencoding>     defencoding_arg_list
%type <defencoding>     defencoding_arg

%type <defmetatype>     defmetatype_args
%type <defmetatype>     defmetatype_arg_list
%type <defmetatype>     defmetatype_arg

%type <defsupertype>    defsupertype_args
%type <defsupertype>    defsupertype_arg_list
%type <defsupertype>    defsupertype_arg

%type <exception>       exception_body
%type <exception>       exception_body_directives
%type <exception>       exception_body_directive

%type <docstring>       docstring_args
%type <docstring>       docstring_arg_list
%type <docstring>       docstring_arg

%type <extract>         extract_args
%type <extract>         extract_arg_list
%type <extract>         extract_arg

%type <feature>         feature_args
%type <feature>         feature_arg_list
%type <feature>         feature_arg

%type <import>          import_args
%type <import>          import_arg_list
%type <import>          import_arg

%type <include>         include_args
%type <include>         include_arg_list
%type <include>         include_arg

%type <license>         license_args
%type <license>         license_arg_list
%type <license>         license_arg

%type <module>          module_args
%type <module>          module_arg_list
%type <module>          module_arg
%type <module>          module_body
%type <module>          module_body_directives
%type <module>          module_body_directive

%type <plugin>          plugin_args
%type <plugin>          plugin_arg_list
%type <plugin>          plugin_arg

%type <property>        property_args
%type <property>        property_arg_list
%type <property>        property_arg
%type <property>        property_body
%type <property>        property_body_directives
%type <property>        property_body_directive

%type <variable>        variable_body
%type <variable>        variable_body_directives
%type <variable>        variable_body_directive

%type <veh>             veh_args
%type <veh>             veh_arg_list
%type <veh>             veh_arg

%%

specification:  statement
    |   specification statement
    ;

statement:  {
            /*
             * We don't do these in parserEOF() because the parser is reading
             * ahead and that would be too early.
             */

            if (previousFile != NULL)
            {
                handleEOF();

                if (currentContext.prevmod != NULL)
                    handleEOM();

                free(previousFile);
                previousFile = NULL;
            }
    } modstatement
    ;

modstatement:   module
    |   consmodule
    |   compmodule
    |   plugin
    |   copying
    |   include
    |   optinclude
    |   import
    |   api
    |   timeline
    |   platforms
    |   feature
    |   license
    |   defdocstring
    |   defencoding
    |   defmetatype
    |   defsupertype
    |   exphdrcode
    |   modhdrcode
    |   modcode
    |   preinitcode
    |   initcode
    |   postinitcode
    |   unitcode
    |   unitpostinccode
    |   prepycode
    |   doc
    |   exporteddoc
    |   extract
    |   makefile
    |   mappedtype
    |   mappedtypetmpl
    |   virterrorhandler
    |   nsstatement
    ;

nsstatement:    ifstart
    |   ifend
    |   namespace
    |   struct
    |   class
    |   classtmpl
    |   exception
    |   typedef
    |   enum
    |   function
    |   variable
    |   typehdrcode {
            if (notSkipping())
            {
                classDef *scope = currentScope();

                if (scope == NULL)
                    yyerror("%TypeHeaderCode can only be used in a namespace, class or mapped type");

                appendCodeBlock(&scope->iff->hdrcode, $1);
            }
        }
    ;

defdocstring:    TK_DEFDOCSTRING defdocstring_args {
            if (notSkipping())
                currentModule->defdocstring = convertFormat($2.name);
        }
    ;

defdocstring_args:   TK_STRING_VALUE {
            resetLexerState();

            $$.name = $1;
        }
    |   '(' defdocstring_arg_list ')' {
            $$ = $2;
        }
    ;

defdocstring_arg_list:   defdocstring_arg
    |   defdocstring_arg_list ',' defdocstring_arg {
            $$ = $1;

            switch ($3.token)
            {
            case TK_NAME: $$.name = $3.name; break;
            }
        }
    ;

defdocstring_arg:    TK_NAME '=' TK_STRING_VALUE {
            $$.token = TK_NAME;

            $$.name = $3;
        }
    ;

defencoding:    TK_DEFENCODING defencoding_args {
            if (notSkipping())
            {
                if ((currentModule->encoding = convertEncoding($2.name)) == no_type)
                    yyerror("The %DefaultEncoding name must be one of \"ASCII\", \"Latin-1\", \"UTF-8\" or \"None\"");
            }
        }
    ;

defencoding_args:   TK_STRING_VALUE {
            resetLexerState();

            $$.name = $1;
        }
    |   '(' defencoding_arg_list ')' {
            $$ = $2;
        }
    ;

defencoding_arg_list:   defencoding_arg
    |   defencoding_arg_list ',' defencoding_arg {
            $$ = $1;

            switch ($3.token)
            {
            case TK_NAME: $$.name = $3.name; break;
            }
        }
    ;

defencoding_arg:    TK_NAME '=' TK_STRING_VALUE {
            $$.token = TK_NAME;

            $$.name = $3;
        }
    ;

plugin:     TK_PLUGIN plugin_args {
            /* Note that %Plugin is internal in SIP v4. */

            if (notSkipping())
                appendString(&currentSpec->plugins, $2.name);
        }
    ;

plugin_args:    TK_NAME_VALUE {
            resetLexerState();

            $$.name = $1;
        }
    |   '(' plugin_arg_list ')' {
            $$ = $2;
        }
    ;

plugin_arg_list:    plugin_arg
    |   plugin_arg_list ',' plugin_arg {
            $$ = $1;

            switch ($3.token)
            {
            case TK_NAME: $$.name = $3.name; break;
            }
        }
    ;

plugin_arg: TK_NAME '=' TK_NAME_VALUE {
            $$.token = TK_NAME;

            $$.name = $3;
        }
    ;

virterrorhandler:   TK_VIRTERRORHANDLER veh_args codeblock {
            if ($2.name == NULL)
                yyerror("%VirtualErrorHandler must have a 'name' argument");

            if (notSkipping())
            {
                virtErrorHandler *veh, **tailp;

                /* Check there isn't already a handler with the same name. */
                for (tailp = &currentSpec->errorhandlers; (veh = *tailp) != NULL; tailp = &veh->next)
                    if (strcmp(veh->name, $2.name) == 0)
                        break;

                if (veh != NULL)
                    yyerror("A virtual error handler with that name has already been defined");

                veh = sipMalloc(sizeof (virtErrorHandler));

                veh->name = $2.name;
                appendCodeBlock(&veh->code, $3);
                veh->mod = currentModule;
                veh->index = currentModule->nrvirterrorhandlers++;
                veh->next = NULL;

                *tailp = veh;
            }
        }
    ;

veh_args:    TK_NAME_VALUE {
            resetLexerState();

            $$.name = $1;
        }
    |   '(' veh_arg_list ')' {
            $$ = $2;
        }
    ;

veh_arg_list:    veh_arg
    |   veh_arg_list ',' veh_arg {
            $$ = $1;

            switch ($3.token)
            {
            case TK_NAME: $$.name = $3.name; break;
            }
        }
    ;

veh_arg: TK_NAME '=' TK_NAME_VALUE {
            $$.token = TK_NAME;

            $$.name = $3;
        }
    ;

api:    TK_API api_args {
            if (notSkipping())
            {
                apiVersionRangeDef *avd;

                if (findAPI(currentSpec, $2.name) != NULL)
                    yyerror("The API name in the %API directive has already been defined");

                if ($2.version < 1)
                    yyerror("The version number in the %API directive must be greater than or equal to 1");

                avd = sipMalloc(sizeof (apiVersionRangeDef));

                avd->api_name = cacheName(currentSpec, $2.name);
                avd->from = $2.version;
                avd->to = -1;

                avd->next = currentModule->api_versions;
                currentModule->api_versions = avd;

                if (inMainModule())
                    setIsUsedName(avd->api_name);
            }
        }
    ;

api_args:   TK_NAME_VALUE TK_NUMBER_VALUE {
            resetLexerState();

            deprecated("%API name and version number should be specified using the 'name' and 'version' arguments");

            $$.name = $1;
            $$.version = $2;
        }
    |   '(' api_arg_list ')' {
            $$ = $2;
        }
    ;

api_arg_list:   api_arg
    |   api_arg_list ',' api_arg {
            $$ = $1;

            switch ($3.token)
            {
            case TK_NAME: $$.name = $3.name; break;
            case TK_VERSION: $$.version = $3.version; break;
            }
        }
    ;

api_arg:    TK_NAME '=' name_or_string {
            $$.token = TK_NAME;

            $$.name = $3;
            $$.version = 0;
        }
    |   TK_VERSION '=' TK_NUMBER_VALUE {
            $$.token = TK_VERSION;

            $$.name = NULL;
            $$.version = $3;
        }
    ;

exception:  TK_EXCEPTION scopedname baseexception optflags exception_body {
            if (notSkipping())
            {
                static const char *annos[] = {
                    "Default",
                    "PyName",
                    NULL
                };

                exceptionDef *xd;
                const char *pyname;

                checkAnnos(&$4, annos);

                if (currentSpec->genc)
                    yyerror("%Exception not allowed in a C module");

                if ($5.raise_code == NULL)
                    yyerror("%Exception must have a %RaiseCode sub-directive");

                pyname = getPythonName(currentModule, &$4, scopedNameTail($2));

                checkAttributes(currentSpec, currentModule, NULL, NULL,
                        pyname, FALSE);

                xd = findException(currentSpec, $2, TRUE);

                if (xd->cd != NULL)
                    yyerror("%Exception name has already been seen as a class name - it must be defined before being used");

                if (xd->iff->module != NULL)
                    yyerror("The %Exception has already been defined");

                /* Complete the definition. */
                xd->iff->module = currentModule;
                appendCodeBlock(&xd->iff->hdrcode, $5.type_header_code);
                xd->pyname = pyname;
                xd->bibase = $3.bibase;
                xd->base = $3.base;
                appendCodeBlock(&xd->raisecode, $5.raise_code);

                if (getOptFlag(&$4, "Default", bool_flag) != NULL)
                    currentModule->defexception = xd;

                if (xd->bibase != NULL || xd->base != NULL)
                    xd->exceptionnr = currentModule->nrexceptions++;
            }
        }
    ;

baseexception:  {
            $$.bibase = NULL;
            $$.base = NULL;
        }
    |   '(' scopedname ')' {
            exceptionDef *xd;

            $$.bibase = NULL;
            $$.base = NULL;

            /* See if it is a defined exception. */
            for (xd = currentSpec->exceptions; xd != NULL; xd = xd->next)
                if (compareScopedNames(xd->iff->fqcname, $2) == 0)
                {
                    $$.base = xd;
                    break;
                }

            if (xd == NULL && $2->next == NULL && strncmp($2->name, "SIP_", 4) == 0)
            {
                /* See if it is a builtin exception. */

                static char *builtins[] = {
                    "Exception",
                    "StopIteration",
                    "StandardError",
                    "ArithmeticError",
                    "LookupError",
                    "AssertionError",
                    "AttributeError",
                    "EOFError",
                    "FloatingPointError",
                    "EnvironmentError",
                    "IOError",
                    "OSError",
                    "ImportError",
                    "IndexError",
                    "KeyError",
                    "KeyboardInterrupt",
                    "MemoryError",
                    "NameError",
                    "OverflowError",
                    "RuntimeError",
                    "NotImplementedError",
                    "SyntaxError",
                    "IndentationError",
                    "TabError",
                    "ReferenceError",
                    "SystemError",
                    "SystemExit",
                    "TypeError",
                    "UnboundLocalError",
                    "UnicodeError",
                    "UnicodeEncodeError",
                    "UnicodeDecodeError",
                    "UnicodeTranslateError",
                    "ValueError",
                    "ZeroDivisionError",
                    "WindowsError",
                    "VMSError",
                    NULL
                };

                char **cp;

                for (cp = builtins; *cp != NULL; ++cp)
                    if (strcmp($2->name + 4, *cp) == 0)
                    {
                        $$.bibase = *cp;
                        break;
                    }
            }

            if ($$.bibase == NULL && $$.base == NULL)
                yyerror("Unknown exception base type");
        }
    ;

exception_body: '{' exception_body_directives '}' ';' {
            $$ = $2;
        }
    ;

exception_body_directives:  exception_body_directive
    |   exception_body_directives exception_body_directive {
            $$ = $1;

            switch ($2.token)
            {
            case TK_RAISECODE: $$.raise_code = $2.raise_code; break;
            case TK_TYPEHEADERCODE: $$.type_header_code = $2.type_header_code; break;
            }
        }
    ;

exception_body_directive:  ifstart {
            $$.token = TK_IF;
        }
    |   ifend {
            $$.token = TK_END;
        }
    |   raisecode {
            if (notSkipping())
            {
                $$.token = TK_RAISECODE;
                $$.raise_code = $1;
            }
            else
            {
                $$.token = 0;
                $$.raise_code = NULL;
            }

            $$.type_header_code = NULL;
        }
    |   typehdrcode {
            if (notSkipping())
            {
                $$.token = TK_TYPEHEADERCODE;
                $$.type_header_code = $1;
            }
            else
            {
                $$.token = 0;
                $$.type_header_code = NULL;
            }

            $$.raise_code = NULL;
        }
    ;

raisecode:  TK_RAISECODE codeblock {
            $$ = $2;
        }
    ;

mappedtype: TK_MAPPEDTYPE basetype optflags {
            if (notSkipping())
            {
                static const char *annos[] = {
                    "AllowNone",
                    "API",
                    "DocType",
                    "NoRelease",
                    "PyName",
                    NULL
                };

                checkAnnos(&$3, annos);

                currentMappedType = newMappedType(currentSpec, &$2, &$3);
            }
        } mtdefinition
    ;

mappedtypetmpl: template TK_MAPPEDTYPE basetype optflags {
            if (notSkipping())
            {
                static const char *annos[] = {
                    "AllowNone",
                    "DocType",
                    "NoRelease",
                    NULL
                };

                int a;
                mappedTypeTmplDef *mtt;
                ifaceFileDef *iff;

                checkAnnos(&$4, annos);

                if (currentSpec->genc)
                    yyerror("%MappedType templates not allowed in a C module");

                /*
                 * Check the template arguments are basic types or simple
                 * names.
                 */
                for (a = 0; a < $1.nrArgs; ++a)
                {
                    argDef *ad = &$1.args[a];

                    if (ad->atype == defined_type && ad->u.snd->next != NULL)
                        yyerror("%MappedType template arguments must be simple names");
                }

                if ($3.atype != template_type)
                    yyerror("%MappedType template must map a template type");

                /* Check a template hasn't already been provided. */
                for (mtt = currentSpec->mappedtypetemplates; mtt != NULL; mtt = mtt->next)
                    if (compareScopedNames(mtt->mt->type.u.td->fqname, $3.u.td->fqname) == 0 && sameTemplateSignature(&mtt->mt->type.u.td->types, &$3.u.td->types, TRUE))
                        yyerror("%MappedType template for this type has already been defined");

                $3.nrderefs = 0;
                $3.argflags = 0;

                mtt = sipMalloc(sizeof (mappedTypeTmplDef));

                mtt->sig = $1;
                mtt->mt = allocMappedType(currentSpec, &$3);
                mappedTypeAnnos(mtt->mt, &$4);
                mtt->next = currentSpec->mappedtypetemplates;

                currentSpec->mappedtypetemplates = mtt;

                currentMappedType = mtt->mt;

                /* Create a dummy interface file. */
                iff = sipMalloc(sizeof (ifaceFileDef));
                iff->hdrcode = NULL;
                mtt->mt->iff = iff;
            }
        } mtdefinition
    ;

mtdefinition:   '{' mtbody '}' ';' {
            if (notSkipping())
            {
                if (currentMappedType->convfromcode == NULL)
                    yyerror("%MappedType must have a %ConvertFromTypeCode directive");

                if (currentMappedType->convtocode == NULL)
                    yyerror("%MappedType must have a %ConvertToTypeCode directive");

                currentMappedType = NULL;
            }
        }
    ;

mtbody: mtline
    |   mtbody mtline
    ;

mtline: ifstart
    |   ifend
    |   typehdrcode {
            if (notSkipping())
                appendCodeBlock(&currentMappedType->iff->hdrcode, $1);
        }
    |   typecode {
            if (notSkipping())
                appendCodeBlock(&currentMappedType->typecode, $1);
        }
    |   TK_FROMTYPE codeblock {
            if (notSkipping())
            {
                if (currentMappedType->convfromcode != NULL)
                    yyerror("%MappedType has more than one %ConvertFromTypeCode directive");

                appendCodeBlock(&currentMappedType->convfromcode, $2);
            }
        }
    |   TK_TOTYPE codeblock {
            if (notSkipping())
            {
                if (currentMappedType->convtocode != NULL)
                    yyerror("%MappedType has more than one %ConvertToTypeCode directive");

                appendCodeBlock(&currentMappedType->convtocode, $2);
            }
        }
    |   instancecode {
            if (notSkipping())
            {
                if (currentMappedType->instancecode != NULL)
                    yyerror("%MappedType has more than one %InstanceCode directive");

                appendCodeBlock(&currentMappedType->instancecode, $1);
            }
        }
    |   enum
    |   mtfunction
    ;

mtfunction: TK_STATIC cpptype TK_NAME_VALUE '(' arglist ')' optconst optexceptions optflags optsig ';' optdocstring methodcode {
            if (notSkipping())
            {
                applyTypeFlags(currentModule, &$2, &$9);

                $5.result = $2;

                newFunction(currentSpec, currentModule, NULL,
                        currentMappedType, 0, TRUE, FALSE, FALSE, FALSE, $3,
                        &$5, $7, FALSE, &$9, $13, NULL, $8, $10, $12);
            }
        }
    ;

namespace:  TK_NAMESPACE TK_NAME_VALUE {
            if (currentSpec -> genc)
                yyerror("namespace definition not allowed in a C module");

            if (notSkipping())
            {
                classDef *ns, *c_scope;
                ifaceFileDef *scope;

                if ((c_scope = currentScope()) != NULL)
                    scope = c_scope->iff;
                else
                    scope = NULL;

                ns = newClass(currentSpec, namespace_iface, NULL,
                        text2scopedName(scope, $2), NULL);

                pushScope(ns);

                sectionFlags = 0;
            }
        } optnsbody ';' {
            if (notSkipping())
            {
                if (inMainModule())
                {
                    classDef *ns = currentScope();

                    setIsUsedName(ns->iff->name);
                    setIsUsedName(ns->pyname);
                }

                popScope();
            }
        }
    ;

optnsbody:
    |   '{' nsbody '}'
    ;

nsbody:     nsstatement
    |   nsbody nsstatement
    ;

platforms:  TK_PLATFORMS {
            if (notSkipping())
            {
                qualDef *qd;

                for (qd = currentModule->qualifiers; qd != NULL; qd = qd->next)
                    if (qd->qtype == platform_qualifier)
                        yyerror("%Platforms has already been defined for this module");
            }
        }
        '{' platformlist '}' {
            if (notSkipping())
            {
                qualDef *qd;
                int nrneeded;

                /* Check that exactly one platform in the set was requested. */
                nrneeded = 0;

                for (qd = currentModule->qualifiers; qd != NULL; qd = qd->next)
                    if (qd->qtype == platform_qualifier && selectedQualifier(neededQualifiers, qd))
                        ++nrneeded;

                if (nrneeded > 1)
                    yyerror("No more than one of these %Platforms must be specified with the -t flag");
            }
        }
    ;

platformlist:   platform
    |   platformlist platform
    ;

platform:   TK_NAME_VALUE {
            newQualifier(currentModule,-1,-1,$1,platform_qualifier);
        }
    ;

feature:    TK_FEATURE feature_args {
            if (notSkipping())
                newQualifier(currentModule, -1, -1, $2.name,
                        feature_qualifier);
        }
    ;

feature_args:   TK_NAME_VALUE {
            resetLexerState();

            $$.name = $1;
        }
    |   '(' feature_arg_list ')' {
            $$ = $2;
        }
    ;

feature_arg_list:   feature_arg
    |   feature_arg_list ',' feature_arg {
            $$ = $1;

            switch ($3.token)
            {
            case TK_NAME: $$.name = $3.name; break;
            }
        }
    ;

feature_arg:    TK_NAME '=' name_or_string {
            $$.token = TK_NAME;

            $$.name = $3;
        }
    ;

timeline:   TK_TIMELINE {
            currentTimelineOrder = 0;
        }
        '{' qualifierlist '}' {
            if (notSkipping())
            {
                qualDef *qd;
                int nrneeded;

                /*
                 * Check that exactly one time slot in the set was requested.
                 */
                nrneeded = 0;

                for (qd = currentModule->qualifiers; qd != NULL; qd = qd->next)
                    if (qd->qtype == time_qualifier && selectedQualifier(neededQualifiers, qd))
                        ++nrneeded;

                if (nrneeded > 1)
                    yyerror("At most one of this %Timeline must be specified with the -t flag");

                currentModule->nrtimelines++;
            }
        }
    ;

qualifierlist:  qualifiername
    |   qualifierlist qualifiername
    ;

qualifiername:  TK_NAME_VALUE {
            newQualifier(currentModule, currentModule->nrtimelines,
                    currentTimelineOrder++, $1, time_qualifier);
        }
    ;

ifstart:    TK_IF '(' qualifiers ')' {
            if (skipStackPtr >= MAX_NESTED_IF)
                yyerror("Internal error: increase the value of MAX_NESTED_IF");

            /* Nested %Ifs are implicit logical ands. */

            if (skipStackPtr > 0)
                $3 = ($3 && skipStack[skipStackPtr - 1]);

            skipStack[skipStackPtr++] = $3;
        }
    ;

oredqualifiers: TK_NAME_VALUE {
            $$ = platOrFeature($1,FALSE);
        }
    |   '!' TK_NAME_VALUE {
            $$ = platOrFeature($2,TRUE);
        }
    |   oredqualifiers TK_LOGICAL_OR TK_NAME_VALUE {
            $$ = (platOrFeature($3,FALSE) || $1);
        }
    |   oredqualifiers TK_LOGICAL_OR '!' TK_NAME_VALUE {
            $$ = (platOrFeature($4,TRUE) || $1);
        }
    ;

qualifiers: oredqualifiers
    |   optname '-' optname {
            $$ = timePeriod($1, $3);
        }
    ;

ifend:      TK_END {
            if (skipStackPtr-- <= 0)
                yyerror("Too many %End directives");
        }
    ;

license:    TK_LICENSE license_args optflags {
            optFlag *of;

            if ($3.nrFlags != 0)
                deprecated("%License annotations are deprecated, use arguments instead");

            if ($2.type == NULL)
                if ((of = getOptFlag(&$3, "Type", string_flag)) != NULL)
                    $2.type = of->fvalue.sval;

            if ($2.licensee == NULL)
                if ((of = getOptFlag(&$3, "Licensee", string_flag)) != NULL)
                    $2.licensee = of->fvalue.sval;

            if ($2.signature == NULL)
                if ((of = getOptFlag(&$3, "Signature", string_flag)) != NULL)
                    $2.signature = of->fvalue.sval;

            if ($2.timestamp == NULL)
                if ((of = getOptFlag(&$3, "Timestamp", string_flag)) != NULL)
                    $2.timestamp = of->fvalue.sval;

            if ($2.type == NULL)
                yyerror("%License must have a 'type' argument");

            if (notSkipping())
            {
                currentModule->license = sipMalloc(sizeof (licenseDef));

                currentModule->license->type = $2.type;
                currentModule->license->licensee = $2.licensee;
                currentModule->license->sig = $2.signature;
                currentModule->license->timestamp = $2.timestamp;
            }
        }
    ;

license_args:   {
            resetLexerState();

            $$.type = NULL;
            $$.licensee = NULL;
            $$.signature = NULL;
            $$.timestamp = NULL;
        }
    |   TK_STRING_VALUE {
            $$.type = $1;
            $$.licensee = NULL;
            $$.signature = NULL;
            $$.timestamp = NULL;
        }
    |   '(' license_arg_list ')' {
            $$ = $2;
        }
    ;

license_arg_list:   license_arg
    |   license_arg_list ',' license_arg {
            $$ = $1;

            switch ($3.token)
            {
            case TK_TYPE: $$.type = $3.type; break;
            case TK_LICENSEE: $$.licensee = $3.licensee; break;
            case TK_SIGNATURE: $$.signature = $3.signature; break;
            case TK_TIMESTAMP: $$.timestamp = $3.timestamp; break;
            }
        }
    ;

license_arg:    TK_TYPE '=' TK_STRING_VALUE {
            $$.token = TK_NAME;

            $$.type = $3;
            $$.licensee = NULL;
            $$.signature = NULL;
            $$.timestamp = NULL;
        }
    |   TK_LICENSEE '=' TK_STRING_VALUE {
            $$.token = TK_LICENSEE;

            $$.type = NULL;
            $$.licensee = $3;
            $$.signature = NULL;
            $$.timestamp = NULL;
        }
    |   TK_SIGNATURE '=' TK_STRING_VALUE {
            $$.token = TK_SIGNATURE;

            $$.type = NULL;
            $$.licensee = NULL;
            $$.signature = $3;
            $$.timestamp = NULL;
        }
    |   TK_TIMESTAMP '=' TK_STRING_VALUE {
            $$.token = TK_TIMESTAMP;

            $$.type = NULL;
            $$.licensee = NULL;
            $$.signature = NULL;
            $$.timestamp = $3;
        }
    ;

defmetatype:    TK_DEFMETATYPE defmetatype_args {
            if (notSkipping())
            {
                if (currentModule->defmetatype != NULL)
                    yyerror("%DefaultMetatype has already been defined for this module");

                currentModule->defmetatype = cacheName(currentSpec, $2.name);
            }
        }
    ;

defmetatype_args:   dottedname {
            resetLexerState();

            $$.name = $1;
        }
    |   '(' defmetatype_arg_list ')' {
            $$ = $2;
        }
    ;

defmetatype_arg_list:   defmetatype_arg
    |   defmetatype_arg_list ',' defmetatype_arg {
            $$ = $1;

            switch ($3.token)
            {
            case TK_NAME: $$.name = $3.name; break;
            }
        }
    ;

defmetatype_arg:    TK_NAME '=' dottedname {
            $$.token = TK_NAME;

            $$.name = $3;
        }
    ;

defsupertype:   TK_DEFSUPERTYPE defsupertype_args {
            if (notSkipping())
            {
                if (currentModule->defsupertype != NULL)
                    yyerror("%DefaultSupertype has already been defined for this module");

                currentModule->defsupertype = cacheName(currentSpec, $2.name);
            }
        }
    ;

defsupertype_args:  dottedname {
            resetLexerState();

            $$.name = $1;
        }
    |   '(' defsupertype_arg_list ')' {
            $$ = $2;
        }
    ;

defsupertype_arg_list:  defsupertype_arg
    |   defsupertype_arg_list ',' defsupertype_arg {
            $$ = $1;

            switch ($3.token)
            {
            case TK_NAME: $$.name = $3.name; break;
            }
        }
    ;

defsupertype_arg:   TK_NAME '=' dottedname {
            $$.token = TK_NAME;

            $$.name = $3;
        }
    ;

consmodule: TK_CONSMODULE consmodule_args consmodule_body {
            if (notSkipping())
            {
                /* Make sure this is the first mention of a module. */
                if (currentSpec->module != currentModule)
                    yyerror("A %ConsolidatedModule cannot be %Imported");

                if (currentModule->fullname != NULL)
                    yyerror("%ConsolidatedModule must appear before any %Module or %CModule directive");

                setModuleName(currentSpec, currentModule, $2.name);
                appendCodeBlock(&currentModule->docstring, $3.docstring);

                setIsConsolidated(currentModule);
            }
        }
    ;

consmodule_args:    dottedname {
            resetLexerState();

            $$.name = $1;
        }
    |   '(' consmodule_arg_list ')' {
            $$ = $2;
        }
    ;

consmodule_arg_list:    consmodule_arg
    |   consmodule_arg_list ',' consmodule_arg {
            $$ = $1;

            switch ($3.token)
            {
            case TK_NAME: $$.name = $3.name; break;
            }
        }
    ;

consmodule_arg: TK_NAME '=' dottedname {
            $$.token = TK_NAME;

            $$.name = $3;
        }
    ;

consmodule_body:    {
            $$.token = 0;
            $$.docstring = NULL;
        }
    |   '{' consmodule_body_directives '}' ';' {
            $$ = $2;
        }
    ;

consmodule_body_directives: consmodule_body_directive
    |   consmodule_body_directives consmodule_body_directive {
            $$ = $1;

            switch ($2.token)
            {
            case TK_DOCSTRING: $$.docstring = $2.docstring; break;
            }
        }
    ;

consmodule_body_directive:  ifstart {
            $$.token = TK_IF;
        }
    |   ifend {
            $$.token = TK_END;
        }
    |   docstring {
            if (notSkipping())
            {
                $$.token = TK_DOCSTRING;
                $$.docstring = $1;
            }
            else
            {
                $$.token = 0;
                $$.docstring = NULL;
            }
        }
    ;

compmodule: TK_COMPOMODULE compmodule_args compmodule_body {
            if (notSkipping())
            {
                /* Make sure this is the first mention of a module. */
                if (currentSpec->module != currentModule)
                    yyerror("A %CompositeModule cannot be %Imported");

                if (currentModule->fullname != NULL)
                    yyerror("%CompositeModule must appear before any %Module directive");

                setModuleName(currentSpec, currentModule, $2.name);
                appendCodeBlock(&currentModule->docstring, $3.docstring);

                setIsComposite(currentModule);
            }
        }
    ;

compmodule_args:    dottedname {
            resetLexerState();

            $$.name = $1;
        }
    |   '(' compmodule_arg_list ')' {
            $$ = $2;
        }
    ;

compmodule_arg_list:    compmodule_arg
    |   compmodule_arg_list ',' compmodule_arg {
            $$ = $1;

            switch ($3.token)
            {
            case TK_NAME: $$.name = $3.name; break;
            }
        }
    ;

compmodule_arg: TK_NAME '=' dottedname {
            $$.token = TK_NAME;

            $$.name = $3;
        }
    ;

compmodule_body:    {
            $$.token = 0;
            $$.docstring = NULL;
        }
    |   '{' compmodule_body_directives '}' ';' {
            $$ = $2;
        }
    ;

compmodule_body_directives: compmodule_body_directive
    |   compmodule_body_directives compmodule_body_directive {
            $$ = $1;

            switch ($2.token)
            {
            case TK_DOCSTRING: $$.docstring = $2.docstring; break;
            }
        }
    ;

compmodule_body_directive:  ifstart {
            $$.token = TK_IF;
        }
    |   ifend {
            $$.token = TK_END;
        }
    |   docstring {
            if (notSkipping())
            {
                $$.token = TK_DOCSTRING;
                $$.docstring = $1;
            }
            else
            {
                $$.token = 0;
                $$.docstring = NULL;
            }
        }
    ;

module: TK_MODULE module_args module_body {
            if ($2.name == NULL)
                yyerror("%Module must have a 'name' argument");

            if (notSkipping())
                currentModule = configureModule(currentSpec, currentModule,
                        currentContext.filename, $2.name, $2.version,
                        $2.c_module, $2.kwargs, $2.use_arg_names,
                        $2.call_super_init, $2.all_raise_py_exc,
                        $2.def_error_handler, $3.docstring);
        }
    |   TK_CMODULE dottedname optnumber {
            deprecated("%CModule is deprecated, use %Module and the 'language' argument instead");

            if (notSkipping())
                currentModule = configureModule(currentSpec, currentModule,
                        currentContext.filename, $2, $3, TRUE, defaultKwArgs,
                        FALSE, -1, FALSE, NULL, NULL);
        }
    ;

module_args:    dottedname {resetLexerState();} optnumber {
            if ($3 >= 0)
                deprecated("%Module version number should be specified using the 'version' argument");

            $$.c_module = FALSE;
            $$.kwargs = defaultKwArgs;
            $$.name = $1;
            $$.use_arg_names = FALSE;
            $$.all_raise_py_exc = FALSE;
            $$.call_super_init = -1;
            $$.def_error_handler = NULL;
            $$.version = $3;
        }
    |   '(' module_arg_list ')' {
            $$ = $2;
        }
    ;

module_arg_list:    module_arg
    |   module_arg_list ',' module_arg {
            $$ = $1;

            switch ($3.token)
            {
            case TK_KWARGS: $$.kwargs = $3.kwargs; break;
            case TK_LANGUAGE: $$.c_module = $3.c_module; break;
            case TK_NAME: $$.name = $3.name; break;
            case TK_USEARGNAMES: $$.use_arg_names = $3.use_arg_names; break;
            case TK_ALLRAISEPYEXC: $$.all_raise_py_exc = $3.all_raise_py_exc; break;
            case TK_CALLSUPERINIT: $$.call_super_init = $3.call_super_init; break;
            case TK_DEFERRORHANDLER: $$.def_error_handler = $3.def_error_handler; break;
            case TK_VERSION: $$.version = $3.version; break;
            }
        }
    ;

module_arg: TK_KWARGS '=' TK_STRING_VALUE {
            $$.token = TK_KWARGS;

            $$.c_module = FALSE;
            $$.kwargs = convertKwArgs($3);
            $$.name = NULL;
            $$.use_arg_names = FALSE;
            $$.all_raise_py_exc = FALSE;
            $$.call_super_init = -1;
            $$.def_error_handler = NULL;
            $$.version = -1;
        }
    |   TK_LANGUAGE '=' TK_STRING_VALUE {
            $$.token = TK_LANGUAGE;

            if (strcmp($3, "C++") == 0)
                $$.c_module = FALSE;
            else if (strcmp($3, "C") == 0)
                $$.c_module = TRUE;
            else
                yyerror("%Module 'language' argument must be either \"C++\" or \"C\"");

            $$.kwargs = defaultKwArgs;
            $$.name = NULL;
            $$.use_arg_names = FALSE;
            $$.all_raise_py_exc = FALSE;
            $$.call_super_init = -1;
            $$.def_error_handler = NULL;
            $$.version = -1;
        }
    |   TK_NAME '=' dottedname {
            $$.token = TK_NAME;

            $$.c_module = FALSE;
            $$.kwargs = defaultKwArgs;
            $$.name = $3;
            $$.use_arg_names = FALSE;
            $$.all_raise_py_exc = FALSE;
            $$.call_super_init = -1;
            $$.def_error_handler = NULL;
            $$.version = -1;
        }
    |   TK_USEARGNAMES '=' bool_value {
            $$.token = TK_USEARGNAMES;

            $$.c_module = FALSE;
            $$.kwargs = defaultKwArgs;
            $$.name = NULL;
            $$.use_arg_names = $3;
            $$.all_raise_py_exc = FALSE;
            $$.call_super_init = -1;
            $$.def_error_handler = NULL;
            $$.version = -1;
        }
    |   TK_ALLRAISEPYEXC '=' bool_value {
            $$.token = TK_ALLRAISEPYEXC;

            $$.c_module = FALSE;
            $$.kwargs = defaultKwArgs;
            $$.name = NULL;
            $$.use_arg_names = FALSE;
            $$.all_raise_py_exc = $3;
            $$.call_super_init = -1;
            $$.def_error_handler = NULL;
            $$.version = -1;
        }
    |   TK_CALLSUPERINIT '=' bool_value {
            $$.token = TK_CALLSUPERINIT;

            $$.c_module = FALSE;
            $$.kwargs = defaultKwArgs;
            $$.name = NULL;
            $$.use_arg_names = FALSE;
            $$.all_raise_py_exc = FALSE;
            $$.call_super_init = $3;
            $$.def_error_handler = NULL;
            $$.version = -1;
        }
    |   TK_DEFERRORHANDLER '=' TK_NAME_VALUE {
            $$.token = TK_DEFERRORHANDLER;

            $$.c_module = FALSE;
            $$.kwargs = defaultKwArgs;
            $$.name = NULL;
            $$.use_arg_names = FALSE;
            $$.all_raise_py_exc = FALSE;
            $$.call_super_init = -1;
            $$.def_error_handler = $3;
            $$.version = -1;
        }
    |   TK_VERSION '=' TK_NUMBER_VALUE {
            if ($3 < 0)
                yyerror("%Module 'version' argument cannot be negative");

            $$.token = TK_VERSION;

            $$.c_module = FALSE;
            $$.kwargs = defaultKwArgs;
            $$.name = NULL;
            $$.use_arg_names = FALSE;
            $$.all_raise_py_exc = FALSE;
            $$.call_super_init = -1;
            $$.def_error_handler = NULL;
            $$.version = $3;
        }
    ;

module_body:    {
            $$.token = 0;
            $$.docstring = NULL;
        }
    |   '{' module_body_directives '}' ';' {
            $$ = $2;
        }
    ;

module_body_directives: module_body_directive
    |   module_body_directives module_body_directive {
            $$ = $1;

            switch ($2.token)
            {
            case TK_DOCSTRING: $$.docstring = $2.docstring; break;
            }
        }
    ;

module_body_directive:  ifstart {
            $$.token = TK_IF;
        }
    |   ifend {
            $$.token = TK_END;
        }
    |   autopyname {
            $$.token = TK_AUTOPYNAME;
        }
    |   docstring {
            if (notSkipping())
            {
                $$.token = TK_DOCSTRING;
                $$.docstring = $1;
            }
            else
            {
                $$.token = 0;
                $$.docstring = NULL;
            }
        }
    ;

dottedname: TK_NAME_VALUE
    |   TK_PATH_VALUE {
            /*
             * The grammar design is a bit broken and this is the easiest way
             * to allow periods in names.
             */

            char *cp;

            for (cp = $1; *cp != '\0'; ++cp)
                if (*cp != '.' && *cp != '_' && !isalnum(*cp))
                    yyerror("Invalid character in name");

            $$ = $1;
        }
    ;

optnumber:  {
            $$ = -1;
        }
    |   TK_NUMBER_VALUE
    ;

include:    TK_INCLUDE include_args {
            if ($2.name == NULL)
                yyerror("%Include must have a 'name' argument");

            if (notSkipping())
                parseFile(NULL, $2.name, NULL, $2.optional);
        }
    ;

include_args:   TK_PATH_VALUE {
            resetLexerState();

            $$.name = $1;
            $$.optional = FALSE;
        }
    |   '(' include_arg_list ')' {
            $$ = $2;
        }
    ;

include_arg_list:   include_arg
    |   include_arg_list ',' include_arg {
            $$ = $1;

            switch ($3.token)
            {
            case TK_NAME: $$.name = $3.name; break;
            case TK_OPTIONAL: $$.optional = $3.optional; break;
            }
        }
    ;

include_arg:    TK_NAME '=' TK_PATH_VALUE {
            $$.token = TK_NAME;

            $$.name = $3;
            $$.optional = FALSE;
        }
    |   TK_OPTIONAL '=' bool_value {
            $$.token = TK_OPTIONAL;

            $$.name = NULL;
            $$.optional = $3;
        }
    ;

optinclude: TK_OPTINCLUDE TK_PATH_VALUE {
            deprecated("%OptionalInclude is deprecated, use %Include and the 'optional' argument instead");

            if (notSkipping())
                parseFile(NULL, $2, NULL, TRUE);
        }
    ;

import:     TK_IMPORT import_args {
            if (notSkipping())
                newImport($2.name);
        }
    ;

import_args:    TK_PATH_VALUE {
            resetLexerState();

            $$.name = $1;
        }
    |   '(' import_arg_list ')' {
            $$ = $2;
        }
    ;

import_arg_list:    import_arg
    |   import_arg_list ',' import_arg {
            $$ = $1;

            switch ($3.token)
            {
            case TK_NAME: $$.name = $3.name; break;
            }
        }
    ;

import_arg: TK_NAME '=' TK_PATH_VALUE {
            $$.token = TK_NAME;

            $$.name = $3;
        }
    ;

optaccesscode:  {
            $$ = NULL;
        }
    |   TK_ACCESSCODE codeblock {
            $$ = $2;
        }
    ;

optgetcode: {
            $$ = NULL;
        }
    |   TK_GETCODE codeblock {
            $$ = $2;
        }
    ;

optsetcode: {
            $$ = NULL;
        }
    |   TK_SETCODE codeblock {
            $$ = $2;
        }
    ;

copying:    TK_COPYING codeblock {
            if (notSkipping())
                appendCodeBlock(&currentModule->copying, $2);
        }
    ;

exphdrcode: TK_EXPHEADERCODE codeblock {
            if (notSkipping())
                appendCodeBlock(&currentSpec->exphdrcode, $2);
        }
    ;

modhdrcode: TK_MODHEADERCODE codeblock {
            if (notSkipping())
                appendCodeBlock(&currentModule->hdrcode, $2);
        }
    ;

typehdrcode:    TK_TYPEHEADERCODE codeblock {
            $$ = $2;
        }
    ;

travcode:   TK_TRAVERSECODE codeblock {
            $$ = $2;
        }
    ;

clearcode:  TK_CLEARCODE codeblock {
            $$ = $2;
        }
    ;

getbufcode: TK_GETBUFFERCODE codeblock {
            $$ = $2;
        }
    ;

releasebufcode: TK_RELEASEBUFFERCODE codeblock {
            $$ = $2;
        }
    ;

readbufcode:    TK_READBUFFERCODE codeblock {
            $$ = $2;
        }
    ;

writebufcode:   TK_WRITEBUFFERCODE codeblock {
            $$ = $2;
        }
    ;

segcountcode:   TK_SEGCOUNTCODE codeblock {
            $$ = $2;
        }
    ;

charbufcode:    TK_CHARBUFFERCODE codeblock {
            $$ = $2;
        }
    ;

instancecode:   TK_INSTANCECODE codeblock {
            $$ = $2;
        }
    ;

picklecode: TK_PICKLECODE codeblock {
            $$ = $2;
        }
    ;

finalcode:  TK_FINALCODE codeblock {
            $$ = $2;
        }
    ;

modcode:    TK_MODCODE codeblock {
            if (notSkipping())
                appendCodeBlock(&currentModule->cppcode, $2);
        }
    ;

typecode:   TK_TYPECODE codeblock {
            $$ = $2;
        }
    ;

preinitcode:    TK_PREINITCODE codeblock {
            if (notSkipping())
                appendCodeBlock(&currentModule->preinitcode, $2);
        }
    ;

initcode:   TK_INITCODE codeblock {
            if (notSkipping())
                appendCodeBlock(&currentModule->initcode, $2);
        }
    ;

postinitcode:   TK_POSTINITCODE codeblock {
            if (notSkipping())
                appendCodeBlock(&currentModule->postinitcode, $2);
        }
    ;

unitcode:   TK_UNITCODE codeblock {
            if (notSkipping())
                appendCodeBlock(&currentModule->unitcode, $2);
        }
    ;

unitpostinccode:    TK_UNITPOSTINCLUDECODE codeblock {
            if (notSkipping())
                appendCodeBlock(&currentModule->unitpostinccode, $2);
        }
    ;

prepycode:  TK_PREPYCODE codeblock {
            /* Deprecated. */
        }
    ;

doc:        TK_DOC codeblock {
            if (notSkipping() && inMainModule())
                appendCodeBlock(&currentSpec->docs, $2);
        }
    ;

exporteddoc:    TK_EXPORTEDDOC codeblock {
            if (notSkipping())
                appendCodeBlock(&currentSpec->docs, $2);
        }
    ;

autopyname: TK_AUTOPYNAME autopyname_args {
            if (notSkipping())
                addAutoPyName(currentModule, $2.remove_leading);
        }
    ;

autopyname_args:    '(' autopyname_arg_list ')' {
            $$ = $2;
        }
    ;

autopyname_arg_list:    autopyname_arg
    |   autopyname_arg_list ',' autopyname_arg {
            $$ = $1;

            switch ($3.token)
            {
            case TK_REMOVELEADING: $$.remove_leading = $3.remove_leading; break;
            }
        }
    ;

autopyname_arg: TK_REMOVELEADING '=' TK_STRING_VALUE {
            $$.token = TK_REMOVELEADING;

            $$.remove_leading = $3;
        }
    ;

docstring:  TK_DOCSTRING docstring_args codeblock {
            $$ = $3;

            /* Format the docstring. */
            if ($2.format == deindented)
            {
                const char *cp;
                char *dp;
                int min_indent, indent, skipping;

                /* Find the common indent. */
                min_indent = -1;
                indent = 0;
                skipping = FALSE;

                for (cp = $$->frag; *cp != '\0'; ++cp)
                {
                    if (skipping)
                    {
                        /*
                         * We have handled the indent and are just looking for
                         * the end of the line.
                         */
                        if (*cp == '\n')
                            skipping = FALSE;
                    }
                    else
                    {
                        if (*cp == ' ')
                        {
                            ++indent;
                        }
                        else if (*cp != '\n')
                        {
                            if (min_indent < 0 || min_indent > indent)
                                min_indent = indent;

                            /* Ignore the remaining characters of the line. */
                            skipping = TRUE;
                        }
                    }
                }

                /* In case the last line doesn't have a trailing newline. */
                if (min_indent < 0 || min_indent > indent)
                    min_indent = indent;

                /*
                 * Go through the text again removing the common indentation.
                 */
                dp = cp = $$->frag;

                while (*cp != '\0')
                {
                    const char *start = cp;
                    int non_blank = FALSE;

                    /* Find the end of the line. */
                    while (*cp != '\n' && *cp != '\0')
                        if (*cp++ != ' ')
                            non_blank = TRUE;

                    /* Find where we are copying from. */
                    if (non_blank)
                    {
                        start += min_indent;

                        while (*start != '\n' && *start != '\0')
                            *dp++ = *start++;
                    }

                    if (*cp == '\n')
                        *dp++ = *cp++;
                }

                *dp = '\0';
            }
        }
    ;

docstring_args: {
            $$.format = currentModule->defdocstring;
        }
    |   TK_STRING_VALUE {
            resetLexerState();

            $$.format = convertFormat($1);
        }
    |   '(' docstring_arg_list ')' {
            $$ = $2;
        }
    ;

docstring_arg_list: docstring_arg
    |   docstring_arg_list ',' docstring_arg {
            $$ = $1;

            switch ($3.token)
            {
            case TK_FORMAT: $$.format = $3.format; break;
            }
        }
    ;

docstring_arg:  TK_FORMAT '=' TK_STRING_VALUE {
            $$.token = TK_FORMAT;

            $$.format = convertFormat($3);
        }
    ;

optdocstring: {
            $$ = NULL;
        }
    |   docstring
    ;

extract:    TK_EXTRACT extract_args codeblock {
            if ($2.id == NULL)
                yyerror("%Extract must have an 'id' argument");

            if (notSkipping())
                addExtractPart(currentSpec, $2.id, $2.order, $3);
        }
    ;

extract_args:   TK_NAME_VALUE {
            resetLexerState();

            $$.id = $1;
            $$.order = -1;
        }
    |   '(' extract_arg_list ')' {
            $$ = $2;
        }
    ;

extract_arg_list:   extract_arg
    |   extract_arg_list ',' extract_arg {
            $$ = $1;

            switch ($3.token)
            {
            case TK_ID: $$.id = $3.id; break;
            case TK_ORDER: $$.order = $3.order; break;
            }
        }
    ;

extract_arg:    TK_ID '=' TK_NAME_VALUE {
            $$.token = TK_ID;

            $$.id = $3;
            $$.order = -1;
        }
    |   TK_ORDER '=' TK_NUMBER_VALUE {
            $$.token = TK_ORDER;

            if ($3 < 0)
                yyerror("The 'order' of an %Extract directive must not be negative");

            $$.id = NULL;
            $$.order = $3;
        }
    ;

makefile:   TK_MAKEFILE TK_PATH_VALUE optfilename codeblock {
            /* Deprecated. */
        }
    ;

codeblock:  codelines TK_END
    ;

codelines:  TK_CODELINE
    |   codelines TK_CODELINE {
            $$ = $1;

            append(&$$->frag, $2->frag);

            free($2->frag);
            free($2);
        }
    ;

enum:       TK_ENUM optname optflags {
            if (notSkipping())
            {
                const char *annos[] = {
                    "NoScope",
                    "PyName",
                    NULL
                };

                checkAnnos(&$3, annos);

                if (sectionFlags != 0 && (sectionFlags & ~(SECT_IS_PUBLIC | SECT_IS_PROT)) != 0)
                    yyerror("Class enums must be in the public or protected sections");

                currentEnum = newEnum(currentSpec, currentModule,
                        currentMappedType, $2, &$3, sectionFlags);
            }
        } '{' optenumbody '}' ';'
    ;

optfilename:    {
            $$ = NULL;
        }
    |   TK_PATH_VALUE {
            $$ = $1;
        }
    ;

optname:    {
            $$ = NULL;
        }
    |   TK_NAME_VALUE {
            $$ = $1;
        }
    ;

optenumbody:
    |   enumbody
    ;

enumbody:   enumline
    |   enumbody enumline
    ;

enumline:   ifstart
    |   ifend
    |   TK_NAME_VALUE optenumassign optflags optcomma {
            if (notSkipping())
            {
                const char *annos[] = {
                    "PyName",
                    NULL
                };

                enumMemberDef *emd, **tail;

                checkAnnos(&$3, annos);

                /* Note that we don't use the assigned value. */
                emd = sipMalloc(sizeof (enumMemberDef));

                emd -> pyname = cacheName(currentSpec,
                        getPythonName(currentModule, &$3, $1));
                emd -> cname = $1;
                emd -> ed = currentEnum;
                emd -> next = NULL;

                checkAttributes(currentSpec, currentModule, emd->ed->ecd,
                        emd->ed->emtd, emd->pyname->text, FALSE);

                /* Append to preserve the order. */
                for (tail = &currentEnum->members; *tail != NULL; tail = &(*tail)->next)
                    ;

                *tail = emd;

                if (inMainModule())
                    setIsUsedName(emd -> pyname);
            }
        }
    ;

optcomma:
    |   ','
    ;

optenumassign:
    |   '=' value
    ;

optassign:  {
            $$ = NULL;
        }
    |   '=' expr {
            $$ = $2;
        }
    ;

expr:   value
    |   expr binop value {
            valueDef *vd;
 
            if ($1 -> vtype == string_value || $3 -> vtype == string_value)
                yyerror("Invalid binary operator for string");
 
            /* Find the last value in the existing expression. */
 
            for (vd = $1; vd -> next != NULL; vd = vd -> next)
                ;
 
            vd -> vbinop = $2;
            vd -> next = $3;

            $$ = $1;
        }
    ;

binop:  '-' {
            $$ = '-';
        }
    |   '+' {
            $$ = '+';
        }
    |   '*' {
            $$ = '*';
        }
    |   '/' {
            $$ = '/';
        }
    |   '&' {
            $$ = '&';
        }
    |   '|' {
            $$ = '|';
        }
    ;

optunop:    {
            $$ = '\0';
        }
    |   '!' {
            $$ = '!';
        }
    |   '~' {
            $$ = '~';
        }
    |   '-' {
            $$ = '-';
        }
    |   '+' {
            $$ = '+';
        }
    |   '*' {
            $$ = '*';
        }
    |   '&' {
            $$ = '&';
        }
    ;

value:      optcast optunop simplevalue {
            if ($2 != '\0' && $3.vtype == string_value)
                yyerror("Invalid unary operator for string");
 
            /* Convert the value to a simple expression on the heap. */
            $$ = sipMalloc(sizeof (valueDef));
 
            *$$ = $3;
            $$->vunop = $2;
            $$->vbinop = '\0';
            $$->cast = $1;
            $$->next = NULL;
        }
    ;

optcast:    {
            $$ = NULL;
        }
    |       '(' scopedname ')' {
            $$ = $2;
        }
    ;

scopedname: scopepart
    |   scopedname TK_SCOPE scopepart {
            if (currentSpec -> genc)
                yyerror("Scoped names are not allowed in a C module");

            appendScopedName(&$1,$3);
        }
    ;

scopepart:  TK_NAME_VALUE {
            $$ = text2scopePart($1);
        }
    ;

bool_value: TK_TRUE_VALUE {
            $$ = TRUE;
        }
    |   TK_FALSE_VALUE {
            $$ = FALSE;
        }
    ;

simplevalue:    scopedname {
            /*
             * We let the C++ compiler decide if the value is a valid one - no
             * point in building a full C++ parser here.
             */

            $$.vtype = scoped_value;
            $$.u.vscp = $1;
        }
    |   basetype '(' exprlist ')' {
            fcallDef *fcd;

            fcd = sipMalloc(sizeof (fcallDef));
            *fcd = $3;
            fcd -> type = $1;

            $$.vtype = fcall_value;
            $$.u.fcd = fcd;
        }
    |   TK_REAL_VALUE {
            $$.vtype = real_value;
            $$.u.vreal = $1;
        }
    |   TK_NUMBER_VALUE {
            $$.vtype = numeric_value;
            $$.u.vnum = $1;
        }
    |   bool_value {
            $$.vtype = numeric_value;
            $$.u.vnum = $1;
        }
    |   TK_NULL_VALUE {
            $$.vtype = numeric_value;
            $$.u.vnum = 0;
        }
    |   TK_STRING_VALUE {
            $$.vtype = string_value;
            $$.u.vstr = $1;
        }
    |   TK_QCHAR_VALUE {
            $$.vtype = qchar_value;
            $$.u.vqchar = $1;
        }
    ;

exprlist:   {
            /* No values. */

            $$.nrArgs = 0;
        }
    |   expr {
            /* The single or first expression. */

            $$.args[0] = $1;
            $$.nrArgs = 1;
        }
    |   exprlist ',' expr {
            /* Check that it wasn't ...(,expression...). */

            if ($$.nrArgs == 0)
                yyerror("First argument to function call is missing");

            /* Check there is room. */

            if ($1.nrArgs == MAX_NR_ARGS)
                yyerror("Internal error - increase the value of MAX_NR_ARGS");

            $$ = $1;

            $$.args[$$.nrArgs] = $3;
            $$.nrArgs++;
        }
    ;

typedef:    TK_TYPEDEF cpptype TK_NAME_VALUE optflags ';' {
            if (notSkipping())
            {
                const char *annos[] = {
                    "Capsule",
                    "DocType",
                    "Encoding",
                    "NoTypeName",
                    "PyInt",
                    "PyName",
                    NULL
                };

                checkAnnos(&$4, annos);

                applyTypeFlags(currentModule, &$2, &$4);
                newTypedef(currentSpec, currentModule, $3, &$2, &$4);
            }
        }
    |   TK_TYPEDEF cpptype '(' '*' TK_NAME_VALUE ')' '(' cpptypelist ')' optflags ';' {
            if (notSkipping())
            {
                const char *annos[] = {
                    "DocType",
                    "Encoding",
                    "NoTypeName",
                    "PyInt",
                    "PyName",
                    NULL
                };

                signatureDef *sig;
                argDef ftype;

                checkAnnos(&$10, annos);

                applyTypeFlags(currentModule, &$2, &$10);

                memset(&ftype, 0, sizeof (argDef));

                /* Create the full signature on the heap. */
                sig = sipMalloc(sizeof (signatureDef));
                *sig = $8;
                sig->result = $2;

                /* Create the full type. */
                ftype.atype = function_type;
                ftype.nrderefs = 1;
                ftype.u.sa = sig;

                newTypedef(currentSpec, currentModule, $5, &ftype, &$10);
            }
        }
    ;

struct:     TK_STRUCT scopedname {
            if (currentSpec -> genc && $2->next != NULL)
                yyerror("Namespaces not allowed in a C module");

            if (notSkipping())
                currentSupers = NULL;
        } superclasses optflags {
            if (notSkipping())
            {
                const char *annos[] = {
                    "Abstract",
                    "AllowNone",
                    "API",
                    "DelayDtor",
                    "Deprecated",
                    "ExportDerived",
                    "External",
                    "Metatype",
                    "Mixin",
                    "NoDefaultCtors",
                    "PyName",
                    "PyQt4Flags",
                    "PyQt4NoQMetaObject",
                    "PyQtInterface",
                    "Supertype",
                    "VirtualErrorHandler",
                    NULL
                };

                checkAnnos(&$5, annos);

                if (currentSpec->genc && currentSupers != NULL)
                    yyerror("Super-classes not allowed in a C module struct");

                defineClass($2, currentSupers, &$5);
                sectionFlags = SECT_IS_PUBLIC;
            }
        } optclassbody ';' {
            if (notSkipping())
                completeClass($2, &$5, $7);
        }
    ;

classtmpl:  template {currentIsTemplate = TRUE;} class {
            if (currentSpec->genc)
                yyerror("Class templates not allowed in a C module");

            if (notSkipping())
            {
                classTmplDef *tcd;

                /*
                 * Make sure there is room for the extra class name argument.
                 */
                if ($1.nrArgs == MAX_NR_ARGS)
                    yyerror("Internal error - increase the value of MAX_NR_ARGS");

                tcd = sipMalloc(sizeof (classTmplDef));
                tcd->sig = $1;
                tcd->cd = $3;
                tcd->next = currentSpec->classtemplates;

                currentSpec->classtemplates = tcd;
            }

            currentIsTemplate = FALSE;
        }
    ;

template:   TK_TEMPLATE '<' cpptypelist '>' {
            $$ = $3;
        }
    ;

class:  TK_CLASS scopedname {
            if (currentSpec->genc)
                yyerror("Class definition not allowed in a C module");

            if (notSkipping())
                currentSupers = NULL;
        } superclasses optflags {
            if (notSkipping())
            {
                const char *annos[] = {
                    "Abstract",
                    "AllowNone",
                    "API",
                    "DelayDtor",
                    "Deprecated",
                    "ExportDerived",
                    "External",
                    "Metatype",
                    "Mixin",
                    "NoDefaultCtors",
                    "PyName",
                    "PyQt4Flags",
                    "PyQt4NoQMetaObject",
                    "PyQtInterface",
                    "Supertype",
                    "VirtualErrorHandler",
                    NULL
                };

                checkAnnos(&$5, annos);

                defineClass($2, currentSupers, &$5);
                sectionFlags = SECT_IS_PRIVATE;
            }
        } optclassbody ';' {
            if (notSkipping())
                $$ = completeClass($2, &$5, $7);
        }
    ;

superclasses:
    |   ':' superlist
    ;

superlist:  superclass
    |   superlist ',' superclass 
    ;

superclass: class_access scopedname {
            if (notSkipping() && $1 == TK_PUBLIC)
            {
                argDef ad;
                classDef *super;
                scopedNameDef *snd = $2;

                /*
                 * This is a hack to allow typedef'ed classes to be used before
                 * we have resolved the typedef definitions.  Unlike elsewhere,
                 * we require that the typedef is defined before being used.
                 */
                for (;;)
                {
                    ad.atype = no_type;
                    ad.argflags = 0;
                    ad.nrderefs = 0;
                    ad.original_type = NULL;

                    searchTypedefs(currentSpec, snd, &ad);

                    if (ad.atype != defined_type)
                        break;

                    if (ad.nrderefs != 0 || isConstArg(&ad) || isReference(&ad))
                        break;

                    snd = ad.u.snd;
                }

                if (ad.atype != no_type)
                    yyerror("Super-class list contains an invalid type");

                /*
                 * Note that passing NULL as the API is a bug.  Instead we
                 * should pass the API of the sub-class being defined,
                 * otherwise we cannot create sub-classes of versioned classes.
                 */
                super = findClass(currentSpec, class_iface, NULL, snd);
                appendToClassList(&currentSupers, super);
            }
        }
    ;

class_access:   {
        $$ = TK_PUBLIC;
        }
    |   TK_PUBLIC {
        $$ = TK_PUBLIC;
        }
    |   TK_PROTECTED {
        $$ = TK_PROTECTED;
        }
    |   TK_PRIVATE {
        $$ = TK_PRIVATE;
        }
    ;

optclassbody:   {
            $$ = FALSE;
        }
    |   '{' classbody '}' {
            $$ = TRUE;
        }
    ;

classbody:  classline
    |   classbody classline
    ;

classline:  ifstart
    |   ifend
    |   namespace
    |   struct
    |   class
    |   exception
    |   typedef
    |   enum
    |   property
    |   docstring {
            if (notSkipping())
                appendCodeBlock(&currentScope()->docstring, $1);
        }
    |   typecode {
            if (notSkipping())
                appendCodeBlock(&currentScope()->cppcode, $1);
        }
    |   typehdrcode {
            if (notSkipping())
                appendCodeBlock(&currentScope()->iff->hdrcode, $1);
        }
    |   travcode {
            if (notSkipping())
            {
                classDef *scope = currentScope();

                if (scope->travcode != NULL)
                    yyerror("%GCTraverseCode already given for class");

                appendCodeBlock(&scope->travcode, $1);
            }
        }
    |   clearcode {
            if (notSkipping())
            {
                classDef *scope = currentScope();

                if (scope->clearcode != NULL)
                    yyerror("%GCClearCode already given for class");

                appendCodeBlock(&scope->clearcode, $1);
            }
        }
    |   getbufcode {
            if (notSkipping())
            {
                classDef *scope = currentScope();

                if (scope->getbufcode != NULL)
                    yyerror("%BIGetBufferCode already given for class");

                appendCodeBlock(&scope->getbufcode, $1);
            }
        }
    |   releasebufcode {
            if (notSkipping())
            {
                classDef *scope = currentScope();

                if (scope->releasebufcode != NULL)
                    yyerror("%BIReleaseBufferCode already given for class");

                appendCodeBlock(&scope->releasebufcode, $1);
            }
        }
    |   readbufcode {
            if (notSkipping())
            {
                classDef *scope = currentScope();

                if (scope->readbufcode != NULL)
                    yyerror("%BIGetReadBufferCode already given for class");

                appendCodeBlock(&scope->readbufcode, $1);
            }
        }
    |   writebufcode {
            if (notSkipping())
            {
                classDef *scope = currentScope();

                if (scope->writebufcode != NULL)
                    yyerror("%BIGetWriteBufferCode already given for class");

                appendCodeBlock(&scope->writebufcode, $1);
            }
        }
    |   segcountcode {
            if (notSkipping())
            {
                classDef *scope = currentScope();

                if (scope->segcountcode != NULL)
                    yyerror("%BIGetSegCountCode already given for class");

                appendCodeBlock(&scope->segcountcode, $1);
            }
        }
    |   charbufcode {
            if (notSkipping())
            {
                classDef *scope = currentScope();

                if (scope->charbufcode != NULL)
                    yyerror("%BIGetCharBufferCode already given for class");

                appendCodeBlock(&scope->charbufcode, $1);
            }
        }
    |   instancecode {
            if (notSkipping())
            {
                classDef *scope = currentScope();

                if (scope->instancecode != NULL)
                    yyerror("%InstanceCode already given for class");

                appendCodeBlock(&scope->instancecode, $1);
            }
        }
    |   picklecode {
            if (notSkipping())
            {
                classDef *scope = currentScope();

                if (scope->picklecode != NULL)
                    yyerror("%PickleCode already given for class");

                appendCodeBlock(&scope->picklecode, $1);
            }
        }
    |   finalcode {
            if (notSkipping())
            {
                classDef *scope = currentScope();

                if (scope->finalcode != NULL)
                    yyerror("%FinalisationCode already given for class");

                appendCodeBlock(&scope->finalcode, $1);
            }
        }
    |   ctor
    |   dtor
    |   varmember
    |   TK_TOSUBCLASS codeblock {
            if (notSkipping())
            {
                classDef *scope = currentScope();

                if (scope->convtosubcode != NULL)
                    yyerror("Class has more than one %ConvertToSubClassCode directive");

                appendCodeBlock(&scope->convtosubcode, $2);
            }
        }
    |   TK_TOTYPE codeblock {
            if (notSkipping())
            {
                classDef *scope = currentScope();

                if (scope->convtocode != NULL)
                    yyerror("Class has more than one %ConvertToTypeCode directive");

                appendCodeBlock(&scope->convtocode, $2);
            }
        }
    |   TK_FROMTYPE codeblock {
            if (notSkipping())
            {
                classDef *scope = currentScope();

                if (scope->convfromcode != NULL)
                    yyerror("Class has more than one %ConvertFromTypeCode directive");

                appendCodeBlock(&scope->convfromcode, $2);
            }
        }
    |   TK_PUBLIC optslot ':' {
            if (currentSpec -> genc)
                yyerror("public section not allowed in a C module");

            if (notSkipping())
                sectionFlags = SECT_IS_PUBLIC | $2;
        }
    |   TK_PROTECTED optslot ':' {
            if (currentSpec -> genc)
                yyerror("protected section not allowed in a C module");

            if (notSkipping())
                sectionFlags = SECT_IS_PROT | $2;
        }
    |   TK_PRIVATE optslot ':' {
            if (currentSpec -> genc)
                yyerror("private section not allowed in a C module");

            if (notSkipping())
                sectionFlags = SECT_IS_PRIVATE | $2;
        }
    |   TK_SIGNALS ':' {
            if (currentSpec -> genc)
                yyerror("signals section not allowed in a C module");

            if (notSkipping())
                sectionFlags = SECT_IS_SIGNAL;
        }
    ;

property:   TK_PROPERTY property_args property_body {
            if ($2.name == NULL)
                yyerror("A %Property directive must have a 'name' argument");

            if ($2.get == NULL)
                yyerror("A %Property directive must have a 'get' argument");

            if (notSkipping())
                addProperty(currentSpec, currentModule, currentScope(),
                        $2.name, $2.get, $2.set, $3.docstring);
        }
    ;

property_args:  '(' property_arg_list ')' {
            $$ = $2;
        }
    ;

property_arg_list:  property_arg
    |   property_arg_list ',' property_arg {
            $$ = $1;

            switch ($3.token)
            {
            case TK_GET: $$.get = $3.get; break;
            case TK_NAME: $$.name = $3.name; break;
            case TK_SET: $$.set = $3.set; break;
            }
        }
    ;

property_arg:   TK_GET '=' TK_NAME_VALUE {
            $$.token = TK_GET;

            $$.get = $3;
            $$.name = NULL;
            $$.set = NULL;
        }
    |   TK_NAME '=' name_or_string {
            $$.token = TK_NAME;

            $$.get = NULL;
            $$.name = $3;
            $$.set = NULL;
        }
    |   TK_SET '=' TK_NAME_VALUE {
            $$.token = TK_SET;

            $$.get = NULL;
            $$.name = NULL;
            $$.set = $3;
        }
    ;

property_body:  {
            $$.token = 0;
            $$.docstring = NULL;
        }
    |   '{' property_body_directives '}' ';' {
            $$ = $2;
        }
    ;

property_body_directives:   property_body_directive
    |   property_body_directives property_body_directive {
            $$ = $1;

            switch ($2.token)
            {
            case TK_DOCSTRING: $$.docstring = $2.docstring; break;
            }
        }
    ;

property_body_directive:    ifstart {
            $$.token = TK_IF;
        }
    |   ifend {
            $$.token = TK_END;
        }
    |   docstring {
            if (notSkipping())
            {
                $$.token = TK_DOCSTRING;
                $$.docstring = $1;
            }
            else
            {
                $$.token = 0;
                $$.docstring = NULL;
            }
        }
    ;

name_or_string: TK_NAME_VALUE
    |   TK_STRING_VALUE
    ;

optslot:    {
            $$ = 0;
        }
    |   TK_SLOTS {
            $$ = SECT_IS_SLOT;
        }
    ;

dtor:       optvirtual '~' TK_NAME_VALUE '(' ')' optexceptions optabstract optflags ';' methodcode virtualcatchercode {
            /* Note that we allow non-virtual dtors in C modules. */

            if (notSkipping())
            {
                const char *annos[] = {
                    "HoldGIL",
                    "ReleaseGIL",
                    NULL
                };

                classDef *cd = currentScope();

                checkAnnos(&$8, annos);

                if (strcmp(classBaseName(cd),$3) != 0)
                    yyerror("Destructor doesn't have the same name as its class");

                if (isDtor(cd))
                    yyerror("Destructor has already been defined");

                if (currentSpec -> genc && $10 == NULL)
                    yyerror("Destructor in C modules must include %MethodCode");

                appendCodeBlock(&cd->dealloccode, $10);
                appendCodeBlock(&cd->dtorcode, $11);
                cd -> dtorexceptions = $6;

                /*
                 * Note that we don't apply the protected/public hack to dtors
                 * as it (I think) may change the behaviour of the wrapped API.
                 */
                cd->classflags |= sectionFlags;

                if ($7)
                {
                    if (!$1)
                        yyerror("Abstract destructor must be virtual");

                    setIsAbstractClass(cd);
                }

                /*
                 * The class has a shadow if we have a virtual dtor or some
                 * dtor code.
                 */
                if ($1 || $11 != NULL)
                {
                    if (currentSpec -> genc)
                        yyerror("Virtual destructor or %VirtualCatcherCode not allowed in a C module");

                    setHasShadow(cd);
                }

                if (getReleaseGIL(&$8))
                    setIsReleaseGILDtor(cd);
                else if (getHoldGIL(&$8))
                    setIsHoldGILDtor(cd);
            }
        }
    ;

ctor:       TK_EXPLICIT {currentCtorIsExplicit = TRUE;} simplector
    |   simplector
    ;

simplector: TK_NAME_VALUE '(' arglist ')' optexceptions optflags optctorsig ';' optdocstring methodcode {
            /* Note that we allow ctors in C modules. */

            if (notSkipping())
            {
                const char *annos[] = {
                    "API",
                    "Default",
                    "Deprecated",
                    "HoldGIL",
                    "KeywordArgs",
                    "NoDerived",
                    "NoRaisesPyException",
                    "PostHook",
                    "PreHook",
                    "RaisesPyException",
                    "ReleaseGIL",
                    "Transfer",
                    NULL
                };

                checkAnnos(&$6, annos);

                if (currentSpec -> genc)
                {
                    if ($10 == NULL && $3.nrArgs != 0)
                        yyerror("Constructors with arguments in C modules must include %MethodCode");

                    if (currentCtorIsExplicit)
                        yyerror("Explicit constructors not allowed in a C module");
                }

                if ((sectionFlags & (SECT_IS_PUBLIC | SECT_IS_PROT | SECT_IS_PRIVATE)) == 0)
                    yyerror("Constructor must be in the public, private or protected sections");

                newCtor(currentModule, $1, sectionFlags, &$3, &$6, $10, $5, $7,
                        currentCtorIsExplicit, $9);
            }

            free($1);

            currentCtorIsExplicit = FALSE;
        }
    ;

optctorsig: {
            $$ = NULL;
        }
    |   '[' {
            parsingCSignature = TRUE;
        } '(' arglist ')' ']' {
            $$ = sipMalloc(sizeof (signatureDef));

            *$$ = $4;

            parsingCSignature = FALSE;
        }
    ;

optsig: {
            $$ = NULL;
        }
    |   '[' {
            parsingCSignature = TRUE;
        } cpptype '(' arglist ')' ']' {
            $$ = sipMalloc(sizeof (signatureDef));

            *$$ = $5;
            $$->result = $3;

            parsingCSignature = FALSE;
        }
    ;

optvirtual: {
            $$ = FALSE;
        }
    |   TK_VIRTUAL {
            $$ = TRUE;
        }
    ;

function:   cpptype TK_NAME_VALUE '(' arglist ')' optconst optexceptions optabstract optflags optsig ';' optdocstring methodcode virtualcatchercode {
            if (notSkipping())
            {
                applyTypeFlags(currentModule, &$1, &$9);

                $4.result = $1;

                newFunction(currentSpec, currentModule, currentScope(), NULL,
                        sectionFlags, currentIsStatic, currentIsSignal,
                        currentIsSlot, currentOverIsVirt, $2, &$4, $6, $8, &$9,
                        $13, $14, $7, $10, $12);
            }

            currentIsStatic = FALSE;
            currentIsSignal = FALSE;
            currentIsSlot = FALSE;
            currentOverIsVirt = FALSE;
        }
    |   cpptype TK_OPERATOR '=' '(' cpptype ')' ';' {
            /*
             * It looks like an assignment operator (though we don't bother to
             * check the types) so make sure it is private.
             */
            if (notSkipping())
            {
                classDef *cd = currentScope();

                if (cd == NULL || !(sectionFlags & SECT_IS_PRIVATE))
                    yyerror("Assignment operators may only be defined as private");

                setCannotAssign(cd);
            }

            currentIsStatic = FALSE;
            currentIsSignal = FALSE;
            currentIsSlot = FALSE;
            currentOverIsVirt = FALSE;
        }
    |   cpptype TK_OPERATOR operatorname '(' arglist ')' optconst optexceptions optabstract optflags optsig ';' methodcode virtualcatchercode {
            if (notSkipping())
            {
                classDef *cd = currentScope();

                /*
                 * If the scope is a namespace then make sure the operator is
                 * handled as a global.
                 */
                if (cd != NULL && cd->iff->type == namespace_iface)
                    cd = NULL;

                applyTypeFlags(currentModule, &$1, &$10);

                /* Handle the unary '+' and '-' operators. */
                if ((cd != NULL && $5.nrArgs == 0) || (cd == NULL && $5.nrArgs == 1))
                {
                    if (strcmp($3, "__add__") == 0)
                        $3 = "__pos__";
                    else if (strcmp($3, "__sub__") == 0)
                        $3 = "__neg__";
                }

                $5.result = $1;

                newFunction(currentSpec, currentModule, cd, NULL,
                        sectionFlags, currentIsStatic, currentIsSignal,
                        currentIsSlot, currentOverIsVirt, $3, &$5, $7, $9,
                        &$10, $13, $14, $8, $11, NULL);
            }

            currentIsStatic = FALSE;
            currentIsSignal = FALSE;
            currentIsSlot = FALSE;
            currentOverIsVirt = FALSE;
        }
    |   TK_OPERATOR cpptype '(' arglist ')' optconst optexceptions optabstract optflags optsig ';' methodcode virtualcatchercode {
            if (notSkipping())
            {
                char *sname;
                classDef *scope = currentScope();

                if (scope == NULL || $4.nrArgs != 0)
                    yyerror("Operator casts must be specified in a class and have no arguments");

                applyTypeFlags(currentModule, &$2, &$9);

                switch ($2.atype)
                {
                case defined_type:
                    sname = NULL;
                    break;

                case bool_type:
                case cbool_type:
                case byte_type:
                case sbyte_type:
                case ubyte_type:
                case short_type:
                case ushort_type:
                case int_type:
                case cint_type:
                case uint_type:
                    sname = "__int__";
                    break;

                case long_type:
                case ulong_type:
                case longlong_type:
                case ulonglong_type:
                    sname = "__long__";
                    break;

                case float_type:
                case cfloat_type:
                case double_type:
                case cdouble_type:
                    sname = "__float__";
                    break;

                default:
                    yyerror("Unsupported operator cast");
                }

                if (sname != NULL)
                {
                    $4.result = $2;

                    newFunction(currentSpec, currentModule, scope, NULL,
                            sectionFlags, currentIsStatic, currentIsSignal,
                            currentIsSlot, currentOverIsVirt, sname, &$4, $6,
                            $8, &$9, $12, $13, $7, $10, NULL);
                }
                else
                {
                    argList *al;

                    /* Check it doesn't already exist. */
                    for (al = scope->casts; al != NULL; al = al->next)
                        if (compareScopedNames($2.u.snd, al->arg.u.snd) == 0)
                            yyerror("This operator cast has already been specified in this class");

                    al = sipMalloc(sizeof (argList));
                    al->arg = $2;
                    al->next = scope->casts;

                    scope->casts = al;
                }
            }

            currentIsStatic = FALSE;
            currentIsSignal = FALSE;
            currentIsSlot = FALSE;
            currentOverIsVirt = FALSE;
        }
    ;

operatorname:   '+'     {$$ = "__add__";}
    |   '-'     {$$ = "__sub__";}
    |   '*'     {$$ = "__mul__";}
    |   '/'     {$$ = "__div__";}
    |   '%'     {$$ = "__mod__";}
    |   '&'     {$$ = "__and__";}
    |   '|'     {$$ = "__or__";}
    |   '^'     {$$ = "__xor__";}
    |   '<' '<'     {$$ = "__lshift__";}
    |   '>' '>'     {$$ = "__rshift__";}
    |   '+' '='     {$$ = "__iadd__";}
    |   '-' '='     {$$ = "__isub__";}
    |   '*' '='     {$$ = "__imul__";}
    |   '/' '='     {$$ = "__idiv__";}
    |   '%' '='     {$$ = "__imod__";}
    |   '&' '='     {$$ = "__iand__";}
    |   '|' '='     {$$ = "__ior__";}
    |   '^' '='     {$$ = "__ixor__";}
    |   '<' '<' '=' {$$ = "__ilshift__";}
    |   '>' '>' '=' {$$ = "__irshift__";}
    |   '~'     {$$ = "__invert__";}
    |   '(' ')'     {$$ = "__call__";}
    |   '[' ']'     {$$ = "__getitem__";}
    |   '<'     {$$ = "__lt__";}
    |   '<' '='     {$$ = "__le__";}
    |   '=' '='     {$$ = "__eq__";}
    |   '!' '='     {$$ = "__ne__";}
    |   '>'     {$$ = "__gt__";}
    |   '>' '='     {$$ = "__ge__";}
    ;

optconst:   {
            $$ = FALSE;
        }
    |   TK_CONST {
            $$ = TRUE;
        }
    ;

optabstract:    {
            $$ = 0;
        }
    |   '=' TK_NUMBER_VALUE {
            if ($2 != 0)
                yyerror("Abstract virtual function '= 0' expected");

            $$ = TRUE;
        }
    ;

optflags:   {
            $$.nrFlags = 0;
        }
    |   '/' flaglist '/' {
            $$ = $2;
        }
    ;


flaglist:   flag {
            $$.flags[0] = $1;
            $$.nrFlags = 1;
        }
    |   flaglist ',' flag {
            /* Check there is room. */

            if ($1.nrFlags == MAX_NR_FLAGS)
                yyerror("Too many optional flags");

            $$ = $1;

            $$.flags[$$.nrFlags++] = $3;
        }
    ;

flag:   TK_NAME_VALUE {
            $$.ftype = bool_flag;
            $$.fname = $1;
        }
    |   TK_NAME_VALUE '=' flagvalue {
            $$ = $3;
            $$.fname = $1;
        }
    ;

flagvalue:  dottedname {
            $$.ftype = (strchr($1, '.') != NULL) ? dotted_name_flag : name_flag;
            $$.fvalue.sval = $1;
        }
    |   TK_NAME_VALUE ':' optnumber '-' optnumber {
            apiVersionRangeDef *avd;
            int from, to;

            $$.ftype = api_range_flag;

            /* Check that the API is known. */
            if ((avd = findAPI(currentSpec, $1)) == NULL)
                yyerror("unknown API name in API annotation");

            if (inMainModule())
                setIsUsedName(avd->api_name);

            /* Unbounded values are represented by 0. */
            if ((from = $3) < 0)
                from = 0;

            if ((to = $5) < 0)
                to = 0;

            $$.fvalue.aval = convertAPIRange(currentModule, avd->api_name,
                    from, to);
        }
    |   TK_STRING_VALUE {
            $$.ftype = string_flag;
            $$.fvalue.sval = convertFeaturedString($1);
        }
    |   TK_NUMBER_VALUE {
            $$.ftype = integer_flag;
            $$.fvalue.ival = $1;
        }
    ;

methodcode: {
            $$ = NULL;
        }
    |   TK_METHODCODE codeblock {
            $$ = $2;
        }
    ;

virtualcatchercode: {
            $$ = NULL;
        }
    |   TK_VIRTUALCATCHERCODE codeblock {
            $$ = $2;
        }
    ;

arglist:    rawarglist {
            int a, nrrxcon, nrrxdis, nrslotcon, nrslotdis, nrarray, nrarraysize;

            nrrxcon = nrrxdis = nrslotcon = nrslotdis = nrarray = nrarraysize = 0;

            for (a = 0; a < $1.nrArgs; ++a)
            {
                argDef *ad = &$1.args[a];

                switch (ad -> atype)
                {
                case rxcon_type:
                    ++nrrxcon;
                    break;

                case rxdis_type:
                    ++nrrxdis;
                    break;

                case slotcon_type:
                    ++nrslotcon;
                    break;

                case slotdis_type:
                    ++nrslotdis;
                    break;
                }

                if (isArray(ad))
                    ++nrarray;

                if (isArraySize(ad))
                    ++nrarraysize;
            }

            if (nrrxcon != nrslotcon || nrrxcon > 1)
                yyerror("SIP_RXOBJ_CON and SIP_SLOT_CON must both be given and at most once");

            if (nrrxdis != nrslotdis || nrrxdis > 1)
                yyerror("SIP_RXOBJ_DIS and SIP_SLOT_DIS must both be given and at most once");

            if (nrarray != nrarraysize || nrarray > 1)
                yyerror("/Array/ and /ArraySize/ must both be given and at most once");

            $$ = $1;
        }
    ;

rawarglist: {
            /* No arguments. */

            $$.nrArgs = 0;
        }
    |   argvalue {
            /* The single or first argument. */

            $$.args[0] = $1;
            $$.nrArgs = 1;
        }
    |   rawarglist ',' argvalue {
            /* Check that it wasn't ...(,arg...). */
            if ($1.nrArgs == 0)
                yyerror("First argument of the list is missing");

            /* Check there is nothing after an ellipsis. */
            if ($1.args[$1.nrArgs - 1].atype == ellipsis_type)
                yyerror("An ellipsis must be at the end of the argument list");

            /*
             * If this argument has no default value, then the
             * previous one mustn't either.
             */
            if ($3.defval == NULL && $1.args[$1.nrArgs - 1].defval != NULL)
                yyerror("Compulsory argument given after optional argument");

            /* Check there is room. */
            if ($1.nrArgs == MAX_NR_ARGS)
                yyerror("Internal error - increase the value of MAX_NR_ARGS");

            $$ = $1;

            $$.args[$$.nrArgs] = $3;
            $$.nrArgs++;
        }
    ;

argvalue:   TK_SIPSIGNAL optname optflags optassign {
            checkNoAnnos(&$3, "SIP_SIGNAL has no annotations");

            $$.atype = signal_type;
            $$.argflags = ARG_IS_CONST;
            $$.nrderefs = 0;
            $$.name = cacheName(currentSpec, $2);
            $$.defval = $4;

            currentSpec -> sigslots = TRUE;
        }
    |   TK_SIPSLOT optname optflags optassign {
            checkNoAnnos(&$3, "SIP_SLOT has no annotations");

            $$.atype = slot_type;
            $$.argflags = ARG_IS_CONST;
            $$.nrderefs = 0;
            $$.name = cacheName(currentSpec, $2);
            $$.defval = $4;

            currentSpec -> sigslots = TRUE;
        }
    |   TK_SIPANYSLOT optname optflags optassign {
            checkNoAnnos(&$3, "SIP_ANYSLOT has no annotations");

            $$.atype = anyslot_type;
            $$.argflags = ARG_IS_CONST;
            $$.nrderefs = 0;
            $$.name = cacheName(currentSpec, $2);
            $$.defval = $4;

            currentSpec -> sigslots = TRUE;
        }
    |   TK_SIPRXCON optname optflags {
            const char *annos[] = {
                "SingleShot",
                NULL
            };

            checkAnnos(&$3, annos);

            $$.atype = rxcon_type;
            $$.argflags = 0;
            $$.nrderefs = 0;
            $$.name = cacheName(currentSpec, $2);

            if (getOptFlag(&$3, "SingleShot", bool_flag) != NULL)
                $$.argflags |= ARG_SINGLE_SHOT;

            currentSpec -> sigslots = TRUE;
        }
    |   TK_SIPRXDIS optname optflags {
            checkNoAnnos(&$3, "SIP_RXOBJ_DIS has no annotations");

            $$.atype = rxdis_type;
            $$.argflags = 0;
            $$.nrderefs = 0;
            $$.name = cacheName(currentSpec, $2);

            currentSpec -> sigslots = TRUE;
        }
    |   TK_SIPSLOTCON '(' arglist ')' optname optflags {
            checkNoAnnos(&$6, "SIP_SLOT_CON has no annotations");

            $$.atype = slotcon_type;
            $$.argflags = ARG_IS_CONST;
            $$.nrderefs = 0;
            $$.name = cacheName(currentSpec, $5);

            memset(&$3.result, 0, sizeof (argDef));
            $3.result.atype = void_type;

            $$.u.sa = sipMalloc(sizeof (signatureDef));
            *$$.u.sa = $3;

            currentSpec -> sigslots = TRUE;
        }
    |   TK_SIPSLOTDIS '(' arglist ')' optname optflags {
            checkNoAnnos(&$6, "SIP_SLOT_DIS has no annotations");

            $$.atype = slotdis_type;
            $$.argflags = ARG_IS_CONST;
            $$.nrderefs = 0;
            $$.name = cacheName(currentSpec, $5);

            memset(&$3.result, 0, sizeof (argDef));
            $3.result.atype = void_type;

            $$.u.sa = sipMalloc(sizeof (signatureDef));
            *$$.u.sa = $3;

            currentSpec -> sigslots = TRUE;
        }
    |   TK_QOBJECT optname optflags {
            checkNoAnnos(&$3, "SIP_QOBJECT has no annotations");

            $$.atype = qobject_type;
            $$.argflags = 0;
            $$.nrderefs = 0;
            $$.name = cacheName(currentSpec, $2);
        }
    |   argtype optassign {
            $$ = $1;
            $$.defval = $2;
        }
    ;

varmember:
        TK_SIGNAL_METHOD {currentIsSignal = TRUE;} simple_varmem
    |   TK_SLOT_METHOD {currentIsSlot = TRUE;} simple_varmem
    |   simple_varmem
    ;

simple_varmem:
        TK_STATIC {currentIsStatic = TRUE;} varmem
    |   varmem
    ;

varmem:
        member
    |   variable
    ;

member:
        TK_VIRTUAL {currentOverIsVirt = TRUE;} function
    |   function
    ;

variable:   cpptype TK_NAME_VALUE optflags variable_body ';' optaccesscode optgetcode optsetcode {
            if (notSkipping())
            {
                const char *annos[] = {
                    "DocType",
                    "Encoding",
                    "PyInt",
                    "PyName",
                    NULL
                };

                checkAnnos(&$3, annos);

                if ($6 != NULL)
                {
                    if ($4.access_code != NULL)
                        yyerror("%AccessCode already defined");

                    $4.access_code = $6;

                    deprecated("%AccessCode should be used a sub-directive");
                }

                if ($7 != NULL)
                {
                    if ($4.get_code != NULL)
                        yyerror("%GetCode already defined");

                    $4.get_code = $7;

                    deprecated("%GetCode should be used a sub-directive");
                }

                if ($8 != NULL)
                {
                    if ($4.set_code != NULL)
                        yyerror("%SetCode already defined");

                    $4.set_code = $8;

                    deprecated("%SetCode should be used a sub-directive");
                }

                newVar(currentSpec, currentModule, $2, currentIsStatic, &$1,
                        &$3, $4.access_code, $4.get_code, $4.set_code,
                        sectionFlags);
            }

            currentIsStatic = FALSE;
        }
    ;

variable_body:  {
            $$.token = 0;
            $$.access_code = NULL;
            $$.get_code = NULL;
            $$.set_code = NULL;
        }
    |   '{' variable_body_directives '}' {
            $$ = $2;
        }
    ;

variable_body_directives:   variable_body_directive
    |   variable_body_directives variable_body_directive {
            $$ = $1;

            switch ($2.token)
            {
            case TK_ACCESSCODE: $$.access_code = $2.access_code; break;
            case TK_GETCODE: $$.get_code = $2.get_code; break;
            case TK_SETCODE: $$.set_code = $2.set_code; break;
            }
        }
    ;

variable_body_directive:    ifstart {
            $$.token = TK_IF;
        }
    |   ifend {
            $$.token = TK_END;
        }
    |   TK_ACCESSCODE codeblock {
            if (notSkipping())
            {
                $$.token = TK_ACCESSCODE;
                $$.access_code = $2;
            }
            else
            {
                $$.token = 0;
                $$.access_code = NULL;
            }

            $$.get_code = NULL;
            $$.set_code = NULL;
        }
    |   TK_GETCODE codeblock {
            if (notSkipping())
            {
                $$.token = TK_GETCODE;
                $$.get_code = $2;
            }
            else
            {
                $$.token = 0;
                $$.get_code = NULL;
            }

            $$.access_code = NULL;
            $$.set_code = NULL;
        }
    |   TK_SETCODE codeblock {
            if (notSkipping())
            {
                $$.token = TK_SETCODE;
                $$.set_code = $2;
            }
            else
            {
                $$.token = 0;
                $$.set_code = NULL;
            }

            $$.access_code = NULL;
            $$.get_code = NULL;
        }
    ;

cpptype:    TK_CONST basetype deref optref {
            int i;

            $$ = $2;
            add_derefs(&$$, &$3);
            $$.argflags |= ARG_IS_CONST | $4;

            /*
             * Treat a prefix const as postfix but check if there isn't already
             * a postfix const.
             */
            if ($3.nrderefs > 0)
            {
                if ($$.derefs[0])
                    yyerror("'const' specified as prefix and postfix");

                $$.derefs[0] = TRUE;
            }
        }
    |   basetype deref optref {
            $$ = $1;
            add_derefs(&$$, &$2);
            $$.argflags |= $3;

            /* PyObject * is a synonym for SIP_PYOBJECT. */
            if ($1.atype == defined_type && strcmp($1.u.snd->name, "PyObject") == 0 && $1.u.snd->next == NULL && $2.nrderefs == 1 && $3 == 0)
            {
                $$.atype = pyobject_type;
                $$.nrderefs = 0;
            }
        }
    ;

argtype:    cpptype optname optflags {
            const char *annos[] = {
                "AllowNone",
                "Array",
                "ArraySize",
                "Constrained",
                "DocType",
                "DocValue",
                "Encoding",
                "GetWrapper",
                "In",
                "KeepReference",
                "NoCopy",
                "Out",
                "PyInt",
                "ResultSize",
                "Transfer",
                "TransferBack",
                "TransferThis",
                NULL
            };

            checkAnnos(&$3, annos);

            $$ = $1;
            $$.name = cacheName(currentSpec, $2);

            handleKeepReference(&$3, &$$, currentModule);

            if (getAllowNone(&$3))
                $$.argflags |= ARG_ALLOW_NONE;

            if (getOptFlag(&$3,"GetWrapper",bool_flag) != NULL)
                $$.argflags |= ARG_GET_WRAPPER;

            if (getOptFlag(&$3,"Array",bool_flag) != NULL)
                $$.argflags |= ARG_ARRAY;

            if (getOptFlag(&$3,"ArraySize",bool_flag) != NULL)
                $$.argflags |= ARG_ARRAY_SIZE;

            if (getTransfer(&$3))
                $$.argflags |= ARG_XFERRED;

            if (getOptFlag(&$3,"TransferThis",bool_flag) != NULL)
                $$.argflags |= ARG_THIS_XFERRED;

            if (getOptFlag(&$3,"TransferBack",bool_flag) != NULL)
                $$.argflags |= ARG_XFERRED_BACK;

            if (getOptFlag(&$3,"In",bool_flag) != NULL)
                $$.argflags |= ARG_IN;

            if (getOptFlag(&$3,"Out",bool_flag) != NULL)
                $$.argflags |= ARG_OUT;

            if (getOptFlag(&$3, "ResultSize", bool_flag) != NULL)
                $$.argflags |= ARG_RESULT_SIZE;

            if (getOptFlag(&$3, "NoCopy", bool_flag) != NULL)
                $$.argflags |= ARG_NO_COPY;

            if (getOptFlag(&$3,"Constrained",bool_flag) != NULL)
            {
                $$.argflags |= ARG_CONSTRAINED;

                switch ($$.atype)
                {
                case bool_type:
                    $$.atype = cbool_type;
                    break;

                case int_type:
                    $$.atype = cint_type;
                    break;

                case float_type:
                    $$.atype = cfloat_type;
                    break;

                case double_type:
                    $$.atype = cdouble_type;
                    break;
                }
            }

            applyTypeFlags(currentModule, &$$, &$3);
            $$.docval = getDocValue(&$3);
        }
    ;

optref: {
            $$ = 0;
        }
    |   '&' {
            if (currentSpec -> genc)
                yyerror("References not allowed in a C module");

            $$ = ARG_IS_REF;
        }
    ;

deref:  {
            $$.nrderefs = 0;
        }
    |   deref TK_CONST '*' {
            add_new_deref(&$$, &$1, TRUE);
        }
    |   deref '*' {
            add_new_deref(&$$, &$1, FALSE);
        }
    ;

basetype:   scopedname {
            memset(&$$, 0, sizeof (argDef));
            $$.atype = defined_type;
            $$.u.snd = $1;

            /* Try and resolve typedefs as early as possible. */
            resolveAnyTypedef(currentSpec, &$$);
        }
    |   scopedname '<' cpptypelist '>' {
            templateDef *td;

            td = sipMalloc(sizeof(templateDef));
            td->fqname = $1;
            td->types = $3;

            memset(&$$, 0, sizeof (argDef));
            $$.atype = template_type;
            $$.u.td = td;
        }
    |   TK_STRUCT scopedname {
            memset(&$$, 0, sizeof (argDef));

            /* In a C module all structures must be defined. */
            if (currentSpec -> genc)
            {
                $$.atype = defined_type;
                $$.u.snd = $2;
            }
            else
            {
                $$.atype = struct_type;
                $$.u.sname = $2;
            }
        }
    |   TK_UNSIGNED TK_SHORT {
            memset(&$$, 0, sizeof (argDef));
            $$.atype = ushort_type;
        }
    |   TK_SHORT {
            memset(&$$, 0, sizeof (argDef));
            $$.atype = short_type;
        }
    |   TK_UNSIGNED {
            memset(&$$, 0, sizeof (argDef));
            $$.atype = uint_type;
        }
    |   TK_UNSIGNED TK_INT {
            memset(&$$, 0, sizeof (argDef));
            $$.atype = uint_type;
        }
    |   TK_INT {
            memset(&$$, 0, sizeof (argDef));
            $$.atype = int_type;
        }
    |   TK_LONG {
            memset(&$$, 0, sizeof (argDef));
            $$.atype = long_type;
        }
    |   TK_UNSIGNED TK_LONG {
            memset(&$$, 0, sizeof (argDef));
            $$.atype = ulong_type;
        }
    |   TK_LONG TK_LONG {
            memset(&$$, 0, sizeof (argDef));
            $$.atype = longlong_type;
        }
    |   TK_UNSIGNED TK_LONG TK_LONG {
            memset(&$$, 0, sizeof (argDef));
            $$.atype = ulonglong_type;
        }
    |   TK_FLOAT {
            memset(&$$, 0, sizeof (argDef));
            $$.atype = float_type;
        }
    |   TK_DOUBLE {
            memset(&$$, 0, sizeof (argDef));
            $$.atype = double_type;
        }
    |   TK_BOOL {
            memset(&$$, 0, sizeof (argDef));
            $$.atype = bool_type;
        }
    |   TK_SIGNED TK_CHAR {
            memset(&$$, 0, sizeof (argDef));
            $$.atype = sstring_type;
        }
    |   TK_UNSIGNED TK_CHAR {
            memset(&$$, 0, sizeof (argDef));
            $$.atype = ustring_type;
        }
    |   TK_CHAR {
            memset(&$$, 0, sizeof (argDef));
            $$.atype = string_type;
        }
    |   TK_WCHAR_T {
            memset(&$$, 0, sizeof (argDef));
            $$.atype = wstring_type;
        }
    |   TK_VOID {
            memset(&$$, 0, sizeof (argDef));
            $$.atype = void_type;
        }
    |   TK_PYOBJECT {
            memset(&$$, 0, sizeof (argDef));
            $$.atype = pyobject_type;
        }
    |   TK_PYTUPLE {
            memset(&$$, 0, sizeof (argDef));
            $$.atype = pytuple_type;
        }
    |   TK_PYLIST {
            memset(&$$, 0, sizeof (argDef));
            $$.atype = pylist_type;
        }
    |   TK_PYDICT {
            memset(&$$, 0, sizeof (argDef));
            $$.atype = pydict_type;
        }
    |   TK_PYCALLABLE {
            memset(&$$, 0, sizeof (argDef));
            $$.atype = pycallable_type;
        }
    |   TK_PYSLICE {
            memset(&$$, 0, sizeof (argDef));
            $$.atype = pyslice_type;
        }
    |   TK_PYTYPE {
            memset(&$$, 0, sizeof (argDef));
            $$.atype = pytype_type;
        }
    |   TK_PYBUFFER {
            memset(&$$, 0, sizeof (argDef));
            $$.atype = pybuffer_type;
        }
    |   TK_SIPSSIZET {
            memset(&$$, 0, sizeof (argDef));
            $$.atype = ssize_type;
        }
    |   TK_ELLIPSIS {
            memset(&$$, 0, sizeof (argDef));
            $$.atype = ellipsis_type;
        }
    ;

cpptypelist:    cpptype {
            /* The single or first type. */

            $$.args[0] = $1;
            $$.nrArgs = 1;
        }
    |   cpptypelist ',' cpptype {
            /* Check there is nothing after an ellipsis. */
            if ($1.args[$1.nrArgs - 1].atype == ellipsis_type)
                yyerror("An ellipsis must be at the end of the argument list");

            /* Check there is room. */
            if ($1.nrArgs == MAX_NR_ARGS)
                yyerror("Internal error - increase the value of MAX_NR_ARGS");

            $$ = $1;

            $$.args[$$.nrArgs] = $3;
            $$.nrArgs++;
        }
    ;

optexceptions:  {
            $$ = NULL;
        }
    |   TK_THROW '(' exceptionlist ')' {
            if (currentSpec->genc)
                yyerror("Exceptions not allowed in a C module");

            $$ = $3;
        }
    ;

exceptionlist:  {
            /* Empty list so use a blank. */

            $$ = sipMalloc(sizeof (throwArgs));
            $$ -> nrArgs = 0;
        }
    |   scopedname {
            /* The only or first exception. */

            $$ = sipMalloc(sizeof (throwArgs));
            $$ -> nrArgs = 1;
            $$ -> args[0] = findException(currentSpec, $1, FALSE);
        }
    |   exceptionlist ',' scopedname {
            /* Check that it wasn't ...(,arg...). */

            if ($1 -> nrArgs == 0)
                yyerror("First exception of throw specifier is missing");

            /* Check there is room. */

            if ($1 -> nrArgs == MAX_NR_ARGS)
                yyerror("Internal error - increase the value of MAX_NR_ARGS");

            $$ = $1;
            $$ -> args[$$ -> nrArgs++] = findException(currentSpec, $3, FALSE);
        }
    ;

%%


/*
 * Parse the specification.
 */
void parse(sipSpec *spec, FILE *fp, char *filename, stringList *tsl,
        stringList *xfl, KwArgs kwArgs, int protHack)
{
    classTmplDef *tcd;

    /* Initialise the spec. */
 
    memset(spec, 0, sizeof (sipSpec));
    spec->genc = -1;

    currentSpec = spec;
    neededQualifiers = tsl;
    excludedQualifiers = xfl;
    currentModule = NULL;
    currentMappedType = NULL;
    currentOverIsVirt = FALSE;
    currentCtorIsExplicit = FALSE;
    currentIsStatic = FALSE;
    currentIsSignal = FALSE;
    currentIsSlot = FALSE;
    currentIsTemplate = FALSE;
    previousFile = NULL;
    skipStackPtr = 0;
    currentScopeIdx = 0;
    sectionFlags = 0;
    defaultKwArgs = kwArgs;
    makeProtPublic = protHack;

    newModule(fp, filename);
    spec->module = currentModule;

    yyparse();

    handleEOF();
    handleEOM();

    /*
     * Go through each template class and remove it from the list of classes.
     */
    for (tcd = spec->classtemplates; tcd != NULL; tcd = tcd->next)
    {
        classDef **cdp;

        for (cdp = &spec->classes; *cdp != NULL; cdp = &(*cdp)->next)
            if (*cdp == tcd->cd)
            {
                ifaceFileDef **ifdp;

                /* Remove the interface file as well. */
                for (ifdp = &spec->ifacefiles; *ifdp != NULL; ifdp = &(*ifdp)->next)
                    if (*ifdp == tcd->cd->iff)
                    {
                        *ifdp = (*ifdp)->next;
                        break;
                    }

                *cdp = (*cdp)->next;
                break;
            }
    }
}


/*
 * Tell the parser that a complete file has now been read.
 */
void parserEOF(const char *name, parserContext *pc)
{
    previousFile = sipStrdup(name);
    currentContext = *pc;
}


/*
 * Append a class definition to a class list if it doesn't already appear.
 * Append is needed specifically for the list of super-classes because the
 * order is important to Python.
 */
void appendToClassList(classList **clp,classDef *cd)
{
    classList *new;

    /* Find the end of the list. */

    while (*clp != NULL)
    {
        if ((*clp) -> cd == cd)
            return;

        clp = &(*clp) -> next;
    }

    new = sipMalloc(sizeof (classList));

    new -> cd = cd;
    new -> next = NULL;

    *clp = new;
}


/*
 * Create a new module for the current specification and make it current.
 */
static void newModule(FILE *fp, const char *filename)
{
    moduleDef *mod;

    parseFile(fp, filename, currentModule, FALSE);

    mod = allocModule();
    mod->file = filename;

    if (currentModule != NULL)
        mod->defexception = currentModule->defexception;

    currentModule = mod;
}


/*
 * Allocate and initialise the memory for a new module.
 */
static moduleDef *allocModule()
{
    moduleDef *newmod, **tailp;

    newmod = sipMalloc(sizeof (moduleDef));

    newmod->version = -1;
    newmod->defdocstring = raw;
    newmod->encoding = no_type;
    newmod->qobjclass = -1;
    newmod->nrvirthandlers = -1;

    /* -1 is reserved for variable getters. */
    newmod->next_key = -2;

    /*
     * The consolidated module support needs these to be in order that they
     * appeared.
     */
    for (tailp = &currentSpec->modules; *tailp != NULL; tailp = &(*tailp)->next)
        ;

    *tailp = newmod;

    return newmod;
}


/*
 * Switch to parsing a new file.
 */
static void parseFile(FILE *fp, const char *name, moduleDef *prevmod,
        int optional)
{
    parserContext pc;

    pc.filename = name;
    pc.ifdepth = skipStackPtr;
    pc.prevmod = prevmod;

    if (setInputFile(fp, &pc, optional))
        currentContext = pc;
}


/*
 * Find an interface file, or create a new one.
 */
ifaceFileDef *findIfaceFile(sipSpec *pt, moduleDef *mod, scopedNameDef *fqname,
        ifaceFileType iftype, apiVersionRangeDef *api_range, argDef *ad)
{
    ifaceFileDef *iff, *first_alt = NULL;

    /* See if the name is already used. */

    for (iff = pt->ifacefiles; iff != NULL; iff = iff->next)
    {
        if (compareScopedNames(iff->fqcname, fqname) != 0)
            continue;

        /*
         * If they are both versioned then assume the user knows what they are
         * doing.
         */
        if (iff->api_range != NULL && api_range != NULL && iff->module == mod)
        {
            /* Remember the first of the alternate APIs. */
            if ((first_alt = iff->first_alt) == NULL)
                first_alt = iff;

            break;
        }

        /*
         * They must be the same type except that we allow a class if we want
         * an exception.  This is because we allow classes to be used before
         * they are defined.
         */
        if (iff->type != iftype)
            if (iftype != exception_iface || iff->type != class_iface)
                yyerror("A class, exception, namespace or mapped type has already been defined with the same name");

        /* Ignore an external class declared in another module. */
        if (iftype == class_iface && iff->module != mod)
        {
            classDef *cd;

            for (cd = pt->classes; cd != NULL; cd = cd->next)
                if (cd->iff == iff)
                    break;

            if (cd != NULL && iff->module != NULL && isExternal(cd))
                continue;
        }

        /*
         * If this is a mapped type with the same name defined in a different
         * module, then check that this type isn't the same as any of the
         * mapped types defined in that module.
         */
        if (iftype == mappedtype_iface && iff->module != mod)
        {
            mappedTypeDef *mtd;

            /*
             * This is a bit of a cheat.  With consolidated modules it's
             * possible to have two implementations of a mapped type in
             * different branches of the module hierarchy.  We assume that, if
             * there really are multiple implementations in the same branch,
             * then it will be picked up in a non-consolidated build.
             */
            if (isConsolidated(pt->module))
                continue;

            for (mtd = pt->mappedtypes; mtd != NULL; mtd = mtd->next)
            {
                if (mtd->iff != iff)
                    continue;

                if (ad->atype != template_type ||
                    mtd->type.atype != template_type ||
                    sameBaseType(ad, &mtd->type))
                    yyerror("Mapped type has already been defined in another module");
            }

            /*
             * If we got here then we have a mapped type based on an existing
             * template, but with unique parameters.  We don't want to use
             * interface files from other modules, so skip this one.
             */

            continue;
        }

        /* Ignore a namespace defined in another module. */
        if (iftype == namespace_iface && iff->module != mod)
            continue;

        return iff;
    }

    iff = sipMalloc(sizeof (ifaceFileDef));

    iff->name = cacheName(pt, scopedNameToString(fqname));
    iff->api_range = api_range;

    if (first_alt != NULL)
    {
        iff->first_alt = first_alt;
        iff->next_alt = first_alt->next_alt;

        first_alt->next_alt = iff;
    }
    else
    {
        /* This is the first alternate so point to itself. */
        iff->first_alt = iff;
    }

    iff->type = iftype;
    iff->ifacenr = -1;
    iff->fqcname = fqname;
    iff->module = NULL;
    iff->hdrcode = NULL;
    iff->used = NULL;
    iff->next = pt->ifacefiles;

    pt->ifacefiles = iff;

    return iff;
}


/*
 * Find a class definition in a parse tree.
 */
static classDef *findClass(sipSpec *pt, ifaceFileType iftype,
        apiVersionRangeDef *api_range, scopedNameDef *fqname)
{
    return findClassWithInterface(pt, findIfaceFile(pt, currentModule, fqname, iftype, api_range, NULL));
}


/*
 * Find a class definition given an existing interface file.
 */
static classDef *findClassWithInterface(sipSpec *pt, ifaceFileDef *iff)
{
    classDef *cd;

    for (cd = pt -> classes; cd != NULL; cd = cd -> next)
        if (cd -> iff == iff)
            return cd;

    /* Create a new one. */
    cd = sipMalloc(sizeof (classDef));

    cd->iff = iff;
    cd->pyname = cacheName(pt, classBaseName(cd));
    cd->next = pt->classes;

    pt->classes = cd;

    return cd;
}


/*
 * Add an interface file to an interface file list if it isn't already there.
 */
void addToUsedList(ifaceFileList **ifflp, ifaceFileDef *iff)
{
    /* Make sure we don't try to add an interface file to its own list. */
    if (&iff->used != ifflp)
    {
        ifaceFileList *iffl;

        while ((iffl = *ifflp) != NULL)
        {
            /* Don't bother if it is already there. */
            if (iffl->iff == iff)
                return;

            ifflp = &iffl -> next;
        }

        iffl = sipMalloc(sizeof (ifaceFileList));

        iffl->iff = iff;
        iffl->next = NULL;

        *ifflp = iffl;
    }
}


/*
 * Find an undefined (or create a new) exception definition in a parse tree.
 */
static exceptionDef *findException(sipSpec *pt, scopedNameDef *fqname, int new)
{
    exceptionDef *xd, **tail;
    ifaceFileDef *iff;
    classDef *cd;

    iff = findIfaceFile(pt, currentModule, fqname, exception_iface, NULL, NULL);

    /* See if it is an existing one. */
    for (xd = pt->exceptions; xd != NULL; xd = xd->next)
        if (xd->iff == iff)
            return xd;

    /*
     * If it is an exception interface file then we have never seen this name
     * before.  We require that exceptions are defined before being used, but
     * don't make the same requirement of classes (for reasons of backwards
     * compatibility).  Therefore the name must be reinterpreted as a (as yet
     * undefined) class.
     */
    if (new)
    {
        if (iff->type == exception_iface)
            cd = NULL;
        else
            yyerror("There is already a class with the same name or the exception has been used before being defined");
    }
    else
    {
        if (iff->type == exception_iface)
            iff->type = class_iface;

        cd = findClassWithInterface(pt, iff);
    }

    /* Create a new one. */
    xd = sipMalloc(sizeof (exceptionDef));

    xd->exceptionnr = -1;
    xd->iff = iff;
    xd->pyname = NULL;
    xd->cd = cd;
    xd->bibase = NULL;
    xd->base = NULL;
    xd->raisecode = NULL;
    xd->next = NULL;

    /* Append it to the list. */
    for (tail = &pt->exceptions; *tail != NULL; tail = &(*tail)->next)
        ;

    *tail = xd;

    return xd;
}


/*
 * Find an undefined (or create a new) class definition in a parse tree.
 */
static classDef *newClass(sipSpec *pt, ifaceFileType iftype,
        apiVersionRangeDef *api_range, scopedNameDef *fqname,
        const char *virt_error_handler)
{
    int flags;
    classDef *cd, *scope;
    codeBlockList *hdrcode;

    if (sectionFlags & SECT_IS_PRIVATE)
        yyerror("Classes, structs and namespaces must be in the public or protected sections");

    flags = 0;

    if ((scope = currentScope()) != NULL)
    {
        if (sectionFlags & SECT_IS_PROT && !makeProtPublic)
        {
            flags = CLASS_IS_PROTECTED;

            if (scope->iff->type == class_iface)
                setHasShadow(scope);
        }

        /* Header code from outer scopes is also included. */
        hdrcode = scope->iff->hdrcode;
    }
    else
        hdrcode = NULL;

    if (pt -> genc)
    {
        /* C structs are always global types. */
        while (fqname -> next != NULL)
            fqname = fqname -> next;

        scope = NULL;
    }

    cd = findClass(pt, iftype, api_range, fqname);

    /* Check it hasn't already been defined. */
    if (iftype != namespace_iface && cd->iff->module != NULL)
        yyerror("The struct/class has already been defined");

    /* Complete the initialisation. */
    cd->classflags |= flags;
    cd->ecd = scope;
    cd->iff->module = currentModule;
    cd->virt_error_handler = virt_error_handler;

    if (currentIsTemplate)
        setIsTemplateClass(cd);

    appendCodeBlockList(&cd->iff->hdrcode, hdrcode);

    /* See if it is a namespace extender. */
    if (iftype == namespace_iface)
    {
        classDef *ns;

        for (ns = pt->classes; ns != NULL; ns = ns->next)
        {
            if (ns == cd)
                continue;

            if (ns->iff->type != namespace_iface)
                continue;

            if (compareScopedNames(ns->iff->fqcname, fqname) != 0)
                continue;

            cd->real = ns;
            break;
        }
    }

    return cd;
}


/*
 * Tidy up after finishing a class definition.
 */
static void finishClass(sipSpec *pt, moduleDef *mod, classDef *cd,
        optFlags *of)
{
    const char *pyname;
    optFlag *flg;

    /* Get the Python name and see if it is different to the C++ name. */
    pyname = getPythonName(mod, of, classBaseName(cd));

    cd->pyname = NULL;
    checkAttributes(pt, mod, cd->ecd, NULL, pyname, FALSE);
    cd->pyname = cacheName(pt, pyname);

    if ((flg = getOptFlag(of, "Metatype", dotted_name_flag)) != NULL)
        cd->metatype = cacheName(pt, flg->fvalue.sval);

    if ((flg = getOptFlag(of, "Supertype", dotted_name_flag)) != NULL)
        cd->supertype = cacheName(pt, flg->fvalue.sval);

    if (getOptFlag(of, "ExportDerived", bool_flag) != NULL)
        setExportDerived(cd);

    if (getOptFlag(of, "Mixin", bool_flag) != NULL)
        setMixin(cd);

    if ((flg = getOptFlag(of, "PyQt4Flags", integer_flag)) != NULL)
        cd->pyqt4_flags = flg->fvalue.ival;

    if (getOptFlag(of, "PyQt4NoQMetaObject", bool_flag) != NULL)
        setPyQt4NoQMetaObject(cd);

    if ((flg = getOptFlag(of, "PyQtInterface", string_flag)) != NULL)
        cd->pyqt_interface = flg->fvalue.sval;

    if (isOpaque(cd))
    {
        if (getOptFlag(of, "External", bool_flag) != NULL)
            setIsExternal(cd);
    }
    else
    {
        int seq_might, seq_not, default_to_sequence;
        memberDef *md;

        if (getOptFlag(of, "NoDefaultCtors", bool_flag) != NULL)
            setNoDefaultCtors(cd);

        if (cd -> ctors == NULL)
        {
            if (!noDefaultCtors(cd))
            {
                /* Provide a default ctor. */

                cd->ctors = sipMalloc(sizeof (ctorDef));
 
                cd->ctors->ctorflags = SECT_IS_PUBLIC;
                cd->ctors->pysig.result.atype = void_type;
                cd->ctors->cppsig = &cd->ctors->pysig;

                cd->defctor = cd->ctors;

                setCanCreate(cd);
            }
        }
        else if (cd -> defctor == NULL)
        {
            ctorDef *ct, *last = NULL;

            for (ct = cd -> ctors; ct != NULL; ct = ct -> next)
            {
                if (!isPublicCtor(ct))
                    continue;

                if (ct -> pysig.nrArgs == 0 || ct -> pysig.args[0].defval != NULL)
                {
                    cd -> defctor = ct;
                    break;
                }

                if (last == NULL)
                    last = ct;
            }

            /* The last resort is the first public ctor. */
            if (cd->defctor == NULL)
                cd->defctor = last;
        }

        if (getDeprecated(of))
            setIsDeprecatedClass(cd);

        if (cd->convtocode != NULL && getAllowNone(of))
            setClassHandlesNone(cd);

        if (getOptFlag(of,"Abstract",bool_flag) != NULL)
        {
            setIsAbstractClass(cd);
            setIsIncomplete(cd);
            resetCanCreate(cd);
        }

        /* We assume a public dtor if nothing specific was provided. */
        if (!isDtor(cd))
            setIsPublicDtor(cd);

        if (getOptFlag(of, "DelayDtor", bool_flag) != NULL)
        {
            setIsDelayedDtor(cd);
            setHasDelayedDtors(mod);
        }

        /*
         * There are subtle differences between the add and concat methods and
         * the multiply and repeat methods.  The number versions can have their
         * operands swapped and may return NotImplemented.  If the user has
         * used the /Numeric/ annotation or there are other numeric operators
         * then we use add/multiply.  Otherwise, if the user has used the
         * /Sequence/ annotation or there are indexing operators then we use
         * concat/repeat.
         */
        seq_might = seq_not = FALSE;

        for (md = cd->members; md != NULL; md = md->next)
            switch (md->slot)
            {
            case getitem_slot:
            case setitem_slot:
            case delitem_slot:
                /* This might be a sequence. */
                seq_might = TRUE;
                break;

            case sub_slot:
            case isub_slot:
            case div_slot:
            case idiv_slot:
            case mod_slot:
            case imod_slot:
            case floordiv_slot:
            case ifloordiv_slot:
            case truediv_slot:
            case itruediv_slot:
            case pos_slot:
            case neg_slot:
                /* This is definately not a sequence. */
                seq_not = TRUE;
                break;
            }

        default_to_sequence = (!seq_not && seq_might);

        for (md = cd->members; md != NULL; md = md->next)
        {
            /* Ignore if it is explicitly numeric. */
            if (isNumeric(md))
                continue;

            if (isSequence(md) || default_to_sequence)
                switch (md->slot)
                {
                case add_slot:
                    md->slot = concat_slot;
                    break;

                case iadd_slot:
                    md->slot = iconcat_slot;
                    break;

                case mul_slot:
                    md->slot = repeat_slot;
                    break;

                case imul_slot:
                    md->slot = irepeat_slot;
                    break;
                }
        }
    }

    if (inMainModule())
    {
        setIsUsedName(cd->iff->name);
        setIsUsedName(cd->pyname);
    }
}


/*
 * Return the encoded name of a template (ie. including its argument types) as
 * a scoped name.
 */
scopedNameDef *encodedTemplateName(templateDef *td)
{
    int a;
    scopedNameDef *snd;

    snd = copyScopedName(td->fqname);

    for (a = 0; a < td->types.nrArgs; ++a)
    {
        char buf[50];
        int flgs;
        scopedNameDef *arg_snd;
        argDef *ad = &td->types.args[a];

        flgs = 0;

        if (isConstArg(ad))
            flgs += 1;

        if (isReference(ad))
            flgs += 2;

        /* We use numbers so they don't conflict with names. */
        sprintf(buf, "%02d%d%d", ad->atype, flgs, ad->nrderefs);

        switch (ad->atype)
        {
        case defined_type:
            arg_snd = copyScopedName(ad->u.snd);
            break;

        case template_type:
            arg_snd = encodedTemplateName(ad->u.td);
            break;

        case struct_type:
            arg_snd = copyScopedName(ad->u.sname);
            break;

        default:
            arg_snd = NULL;
        }

        /*
         * Replace the first element of the argument name with a copy with the
         * encoding prepended.
         */
        if (arg_snd != NULL)
            arg_snd->name = concat(buf, arg_snd->name, NULL);
        else
            arg_snd = text2scopePart(sipStrdup(buf));

        appendScopedName(&snd, arg_snd);
    }

    return snd;
}


/*
 * Create a new mapped type.
 */
static mappedTypeDef *newMappedType(sipSpec *pt, argDef *ad, optFlags *of)
{
    mappedTypeDef *mtd;
    scopedNameDef *snd;
    ifaceFileDef *iff;
    const char *cname;

    /* Check that the type is one we want to map. */
    switch (ad->atype)
    {
    case defined_type:
        snd = ad->u.snd;
        cname = scopedNameTail(snd);
        break;

    case template_type:
        snd = encodedTemplateName(ad->u.td);
        cname = NULL;
        break;

    case struct_type:
        snd = ad->u.sname;
        cname = scopedNameTail(snd);
        break;

    default:
        yyerror("Invalid type for %MappedType");
    }

    iff = findIfaceFile(pt, currentModule, snd, mappedtype_iface,
            getAPIRange(of), ad);

    /* Check it hasn't already been defined. */
    for (mtd = pt->mappedtypes; mtd != NULL; mtd = mtd->next)
        if (mtd->iff == iff)
        {
            /*
             * We allow types based on the same template but with different
             * arguments.
             */
            if (ad->atype != template_type || sameBaseType(ad, &mtd->type))
                yyerror("Mapped type has already been defined in this module");
        }

    /* The module may not have been set yet. */
    iff->module = currentModule;

    /* Create a new mapped type. */
    mtd = allocMappedType(pt, ad);

    if (cname != NULL)
        mtd->pyname = cacheName(pt, getPythonName(currentModule, of, cname));

    mappedTypeAnnos(mtd, of);

    mtd->iff = iff;
    mtd->next = pt->mappedtypes;

    pt->mappedtypes = mtd;

    if (inMainModule())
    {
        setIsUsedName(mtd->cname);

        if (mtd->pyname)
            setIsUsedName(mtd->pyname);
    }

    return mtd;
}


/*
 * Allocate, initialise and return a mapped type structure.
 */
mappedTypeDef *allocMappedType(sipSpec *pt, argDef *type)
{
    mappedTypeDef *mtd;

    mtd = sipMalloc(sizeof (mappedTypeDef));

    mtd->type = *type;
    mtd->type.argflags = 0;
    mtd->type.nrderefs = 0;

    mtd->cname = cacheName(pt, type2string(&mtd->type));

    return mtd;
}


/*
 * Create a new enum.
 */
static enumDef *newEnum(sipSpec *pt, moduleDef *mod, mappedTypeDef *mt_scope,
        char *name, optFlags *of, int flags)
{
    enumDef *ed, *first_alt, *next_alt;
    classDef *c_scope;
    ifaceFileDef *scope;

    if (mt_scope != NULL)
    {
        scope = mt_scope->iff;
        c_scope = NULL;
    }
    else
    {
        if ((c_scope = currentScope()) != NULL)
            scope = c_scope->iff;
        else
            scope = NULL;
    }

    ed = sipMalloc(sizeof (enumDef));

    /* Assume the enum isn't versioned. */
    first_alt = ed;
    next_alt = NULL;

    if (name != NULL)
    {
        ed->pyname = cacheName(pt, getPythonName(mod, of, name));
        checkAttributes(pt, mod, c_scope, mt_scope, ed->pyname->text, FALSE);

        ed->fqcname = text2scopedName(scope, name);
        ed->cname = cacheName(pt, scopedNameToString(ed->fqcname));

        if (inMainModule())
        {
            setIsUsedName(ed->pyname);
            setIsUsedName(ed->cname);
        }

        /* If the scope is versioned then look for any alternate. */
        if (scope != NULL && scope->api_range != NULL)
        {
            enumDef *alt;

            for (alt = pt->enums; alt != NULL; alt = alt->next)
            {
                if (alt->module != mod || alt->fqcname == NULL)
                    continue;

                if (compareScopedNames(alt->fqcname, ed->fqcname) == 0)
                {
                    first_alt = alt->first_alt;
                    next_alt = first_alt->next_alt;
                    first_alt->next_alt = ed;

                    break;
                }
            }
        }
    }
    else
    {
        ed->pyname = NULL;
        ed->fqcname = NULL;
        ed->cname = NULL;
    }

    if (flags & SECT_IS_PROT)
    {
        if (makeProtPublic)
        {
            flags &= ~SECT_IS_PROT;
            flags |= SECT_IS_PUBLIC;
        }
        else if (c_scope != NULL)
        {
            setHasShadow(c_scope);
        }
    }

    ed->enumflags = flags;
    ed->enumnr = -1;
    ed->ecd = c_scope;
    ed->emtd = mt_scope;
    ed->first_alt = first_alt;
    ed->next_alt = next_alt;
    ed->module = mod;
    ed->members = NULL;
    ed->slots = NULL;
    ed->overs = NULL;
    ed->next = pt -> enums;

    pt->enums = ed;

    if (getOptFlag(of, "NoScope", bool_flag) != NULL)
        setIsNoScope(ed);

    return ed;
}


/*
 * Get the type values and (optionally) the type names for substitution in
 * handwritten code.
 */
void appendTypeStrings(scopedNameDef *ename, signatureDef *patt, signatureDef *src, signatureDef *known, scopedNameDef **names, scopedNameDef **values)
{
    int a;

    for (a = 0; a < patt->nrArgs; ++a)
    {
        argDef *pad = &patt->args[a];

        if (pad->atype == defined_type)
        {
            char *nam = NULL, *val;
            argDef *sad;

            /*
             * If the type names are already known then check that this is one
             * of them.
             */
            if (known == NULL)
                nam = scopedNameTail(pad->u.snd);
            else if (pad->u.snd->next == NULL)
            {
                int k;

                for (k = 0; k < known->nrArgs; ++k)
                {
                    /* Skip base types. */
                    if (known->args[k].atype != defined_type)
                        continue;

                    if (strcmp(pad->u.snd->name, known->args[k].u.snd->name) == 0)
                    {
                        nam = pad->u.snd->name;
                        break;
                    }
                }
            }

            if (nam == NULL)
                continue;

            /* Add the name. */
            appendScopedName(names, text2scopePart(nam));

            /*
             * Add the corresponding value.  For defined types we don't want 
             * any indirection or references.
             */
            sad = &src->args[a];

            if (sad->atype == defined_type)
                val = scopedNameToString(sad->u.snd);
            else
                val = type2string(sad);

            /* We do want const. */
            if (isConstArg(sad))
            {
                char *const_val = sipStrdup("const ");

                append(&const_val, val);
                free(val);

                val = const_val;
            }

            appendScopedName(values, text2scopePart(val));
        }
        else if (pad->atype == template_type)
        {
            argDef *sad = &src->args[a];

            /* These checks shouldn't be necessary, but... */
            if (sad->atype == template_type && pad->u.td->types.nrArgs == sad->u.td->types.nrArgs)
                appendTypeStrings(ename, &pad->u.td->types, &sad->u.td->types, known, names, values);
        }
    }
}


/*
 * Convert a type to a string on the heap.  The string will use the minimum
 * whitespace while still remaining valid C++.
 */
static char *type2string(argDef *ad)
{
    int i, on_heap = FALSE;
    int nr_derefs = ad->nrderefs;
    int is_reference = isReference(ad);
    char *s;

    /* Use the original type if possible. */
    if (ad->original_type != NULL && !noTypeName(ad->original_type))
    {
        s = scopedNameToString(ad->original_type->fqname);
        on_heap = TRUE;

        nr_derefs -= ad->original_type->type.nrderefs;

        if (isReference(&ad->original_type->type))
            is_reference = FALSE;
    }
    else
        switch (ad->atype)
        {
        case template_type:
            {
                templateDef *td = ad->u.td;

                s = scopedNameToString(td->fqname);
                append(&s, "<");

                for (i = 0; i < td->types.nrArgs; ++i)
                {
                    char *sub_type = type2string(&td->types.args[i]);

                    if (i > 0)
                        append(&s, ",");

                    append(&s, sub_type);
                    free(sub_type);
                }

                if (s[strlen(s) - 1] == '>')
                    append(&s, " >");
                else
                    append(&s, ">");

                on_heap = TRUE;
                break;
            }

        case struct_type:
            s = scopedNameToString(ad->u.sname);
            on_heap = TRUE;
            break;

        case defined_type:
            s = scopedNameToString(ad->u.snd);
            on_heap = TRUE;
            break;

        case ubyte_type:
        case ustring_type:
            s = "unsigned char";
            break;

        case byte_type:
        case ascii_string_type:
        case latin1_string_type:
        case utf8_string_type:
        case string_type:
            s = "char";
            break;

        case sbyte_type:
        case sstring_type:
            s = "signed char";
            break;

        case wstring_type:
            s = "wchar_t";
            break;

        case ushort_type:
            s = "unsigned short";
            break;

        case short_type:
            s = "short";
            break;

        case uint_type:
            s = "uint";
            break;

        case int_type:
        case cint_type:
            s = "int";
            break;

        case ulong_type:
            s = "unsigned long";
            break;

        case long_type:
            s = "long";
            break;

        case ulonglong_type:
            s = "unsigned long long";
            break;

        case longlong_type:
            s = "long long";
            break;

        case float_type:
        case cfloat_type:
            s = "float";
            break;

        case double_type:
        case cdouble_type:
            s = "double";
            break;

        case bool_type:
        case cbool_type:
            s = "bool";
            break;

        case void_type:
            s = "void";
            break;

        case capsule_type:
            s = "void *";
            break;

        default:
            fatal("Unsupported type argument to type2string(): %d\n", ad->atype);
        }

    /* Make sure the string is on the heap. */
    if (!on_heap)
        s = sipStrdup(s);

    while (nr_derefs-- > 0)
        append(&s, "*");

    if (is_reference)
        append(&s, "&");

    return s;
}


/*
 * Convert a scoped name to a string on the heap.
 */
static char *scopedNameToString(scopedNameDef *name)
{
    static const char scope_string[] = "::";
    size_t len;
    scopedNameDef *snd;
    char *s, *dp;

    /* Work out the length of buffer needed. */
    len = 0;

    for (snd = name; snd != NULL; snd = snd->next)
    {
        len += strlen(snd->name);

        if (snd->next != NULL)
        {
            /* Ignore the encoded part of template names. */
            if (isdigit(snd->next->name[0]))
                break;

            len += strlen(scope_string);
        }
    }

    /* Allocate and populate the buffer. */
    dp = s = sipMalloc(len + 1);

    for (snd = name; snd != NULL; snd = snd->next)
    {
        strcpy(dp, snd->name);
        dp += strlen(snd->name);

        if (snd->next != NULL)
        {
            /* Ignore the encoded part of template names. */
            if (isdigit(snd->next->name[0]))
                break;

            strcpy(dp, scope_string);
            dp += strlen(scope_string);
        }
    }

    return s;
}


/*
 * Instantiate a class template.
 */
static void instantiateClassTemplate(sipSpec *pt, moduleDef *mod,
        classDef *scope, scopedNameDef *fqname, classTmplDef *tcd,
        templateDef *td, const char *pyname)
{
    scopedNameDef *type_names, *type_values;
    classDef *cd;
    ctorDef *oct, **cttail;
    argDef *ad;
    ifaceFileList *iffl, **used;
    classList *cl;

    type_names = type_values = NULL;
    appendTypeStrings(classFQCName(tcd->cd), &tcd->sig, &td->types, NULL, &type_names, &type_values);

    /*
     * Add a mapping from the template name to the instantiated name.  If we
     * have got this far we know there is room for it.
     */
    ad = &tcd->sig.args[tcd->sig.nrArgs++];
    memset(ad, 0, sizeof (argDef));
    ad->atype = defined_type;
    ad->u.snd = classFQCName(tcd->cd);

    appendScopedName(&type_names, text2scopePart(scopedNameTail(classFQCName(tcd->cd))));
    appendScopedName(&type_values, text2scopePart(scopedNameToString(fqname)));

    /* Create the new class. */
    cd = sipMalloc(sizeof (classDef));

    /* Start with a shallow copy. */
    *cd = *tcd->cd;

    resetIsTemplateClass(cd);
    cd->pyname = cacheName(pt, pyname);
    cd->td = td;

    /* Handle the interface file. */
    cd->iff = findIfaceFile(pt, mod, fqname, class_iface,
            (scope != NULL ? scope->iff->api_range : NULL), NULL);
    cd->iff->module = mod;

    appendCodeBlockList(&cd->iff->hdrcode, tcd->cd->iff->hdrcode);

    /* Make a copy of the used list and add the enclosing scope. */
    used = &cd->iff->used;

    for (iffl = tcd->cd->iff->used; iffl != NULL; iffl = iffl->next)
        addToUsedList(used, iffl->iff);

    /* Include any scope header code. */
    if (scope != NULL)
        appendCodeBlockList(&cd->iff->hdrcode, scope->iff->hdrcode);

    if (inMainModule())
    {
        setIsUsedName(cd->iff->name);
        setIsUsedName(cd->pyname);
    }

    cd->ecd = currentScope();

    /* Handle the super-classes. */
    for (cl = cd->supers; cl != NULL; cl = cl->next)
    {
        const char *name;
        int a;

        /* Ignore defined or scoped classes. */
        if (cl->cd->iff->module != NULL || cl->cd->iff->fqcname->next != NULL)
            continue;

        name = cl->cd->iff->fqcname->name;

        for (a = 0; a < tcd->sig.nrArgs - 1; ++a)
            if (strcmp(name, scopedNameTail(tcd->sig.args[a].u.snd)) == 0)
            {
                argDef *tad = &td->types.args[a];
                classDef *icd;

                if (tad->atype == defined_type)
                    icd = findClass(pt, class_iface, NULL, tad->u.snd);
                else if (tad->atype == class_type)
                    icd = tad->u.cd;
                else
                    fatal("Template argument %s must expand to a class\n", name);

                /*
                 * Don't complain about the template argument being undefined.
                 */
                setTemplateArg(cl->cd);

                cl->cd = icd;
            }
    }

    /* Handle the enums. */
    instantiateTemplateEnums(pt, tcd, td, cd, used, type_names, type_values);

    /* Handle the variables. */
    instantiateTemplateVars(pt, tcd, td, cd, used, type_names, type_values);

    /* Handle the typedefs. */
    instantiateTemplateTypedefs(pt, tcd, td, cd);

    /* Handle the ctors. */
    cd->ctors = NULL;
    cttail = &cd->ctors;

    for (oct = tcd->cd->ctors; oct != NULL; oct = oct->next)
    {
        ctorDef *nct = sipMalloc(sizeof (ctorDef));

        /* Start with a shallow copy. */
        *nct = *oct;

        templateSignature(&nct->pysig, FALSE, tcd, td, cd);

        if (oct->cppsig == NULL)
            nct->cppsig = NULL;
        else if (oct->cppsig == &oct->pysig)
            nct->cppsig = &nct->pysig;
        else
        {
            nct->cppsig = sipMalloc(sizeof (signatureDef));

            *nct->cppsig = *oct->cppsig;

            templateSignature(nct->cppsig, FALSE, tcd, td, cd);
        }

        nct->methodcode = templateCode(pt, used, nct->methodcode, type_names, type_values);

        nct->next = NULL;
        *cttail = nct;
        cttail = &nct->next;

        /* Handle the default ctor. */
        if (tcd->cd->defctor == oct)
            cd->defctor = nct;
    }

    cd->dealloccode = templateCode(pt, used, cd->dealloccode, type_names, type_values);
    cd->dtorcode = templateCode(pt, used, cd->dtorcode, type_names, type_values);

    /* Handle the methods. */
    cd->members = instantiateTemplateMethods(tcd->cd->members, mod);
    cd->overs = instantiateTemplateOverloads(pt, tcd->cd->overs,
            tcd->cd->members, cd->members, tcd, td, cd, used, type_names,
            type_values);

    cd->cppcode = templateCode(pt, used, cd->cppcode, type_names, type_values);
    cd->iff->hdrcode = templateCode(pt, used, cd->iff->hdrcode, type_names, type_values);
    cd->convtosubcode = templateCode(pt, used, cd->convtosubcode, type_names, type_values);
    cd->convtocode = templateCode(pt, used, cd->convtocode, type_names, type_values);
    cd->travcode = templateCode(pt, used, cd->travcode, type_names, type_values);
    cd->clearcode = templateCode(pt, used, cd->clearcode, type_names, type_values);
    cd->getbufcode = templateCode(pt, used, cd->getbufcode, type_names, type_values);
    cd->releasebufcode = templateCode(pt, used, cd->releasebufcode, type_names, type_values);
    cd->readbufcode = templateCode(pt, used, cd->readbufcode, type_names, type_values);
    cd->writebufcode = templateCode(pt, used, cd->writebufcode, type_names, type_values);
    cd->segcountcode = templateCode(pt, used, cd->segcountcode, type_names, type_values);
    cd->charbufcode = templateCode(pt, used, cd->charbufcode, type_names, type_values);
    cd->instancecode = templateCode(pt, used, cd->instancecode, type_names, type_values);
    cd->picklecode = templateCode(pt, used, cd->picklecode, type_names, type_values);
    cd->finalcode = templateCode(pt, used, cd->finalcode, type_names, type_values);
    cd->next = pt->classes;

    pt->classes = cd;

    tcd->sig.nrArgs--;

    freeScopedName(type_names);
    freeScopedName(type_values);
}


/*
 * Instantiate the methods of a template class.
 */
static memberDef *instantiateTemplateMethods(memberDef *tmd, moduleDef *mod)
{
    memberDef *md, *methods, **mdtail;

    methods = NULL;
    mdtail = &methods;

    for (md = tmd; md != NULL; md = md->next)
    {
        memberDef *nmd = sipMalloc(sizeof (memberDef));

        /* Start with a shallow copy. */
        *nmd = *md;

        nmd->module = mod;

        if (inMainModule())
            setIsUsedName(nmd->pyname);

        nmd->next = NULL;
        *mdtail = nmd;
        mdtail = &nmd->next;
    }

    return methods;
}


/*
 * Instantiate the overloads of a template class.
 */
static overDef *instantiateTemplateOverloads(sipSpec *pt, overDef *tod,
        memberDef *tmethods, memberDef *methods, classTmplDef *tcd,
        templateDef *td, classDef *cd, ifaceFileList **used,
        scopedNameDef *type_names, scopedNameDef *type_values)
{
    overDef *od, *overloads, **odtail;

    overloads = NULL;
    odtail = &overloads;

    for (od = tod; od != NULL; od = od->next)
    {
        overDef *nod = sipMalloc(sizeof (overDef));
        memberDef *nmd, *omd;

        /* Start with a shallow copy. */
        *nod = *od;

        for (nmd = methods, omd = tmethods; omd != NULL; omd = omd->next, nmd = nmd->next)
            if (omd == od->common)
            {
                nod->common = nmd;
                break;
            }

        templateSignature(&nod->pysig, TRUE, tcd, td, cd);

        if (od->cppsig == &od->pysig)
            nod->cppsig = &nod->pysig;
        else
        {
            nod->cppsig = sipMalloc(sizeof (signatureDef));

            *nod->cppsig = *od->cppsig;

            templateSignature(nod->cppsig, TRUE, tcd, td, cd);
        }

        nod->methodcode = templateCode(pt, used, nod->methodcode, type_names, type_values);

        /* Handle any virtual handler. */
        if (od->virthandler != NULL)
        {
            moduleDef *mod = cd->iff->module;

            nod->virthandler = sipMalloc(sizeof (virtHandlerDef));

            /* Start with a shallow copy. */
            *nod->virthandler = *od->virthandler;

            nod->virthandler->pysig = &nod->pysig;
            nod->virthandler->cppsig = nod->cppsig;

            nod->virthandler->module = mod;
            nod->virthandler->virtcode = templateCode(pt, used, nod->virthandler->virtcode, type_names, type_values);
            nod->virthandler->next = mod->virthandlers;

            mod->virthandlers = nod->virthandler;
        }

        nod->next = NULL;
        *odtail = nod;
        odtail = &nod->next;
    }

    return overloads;
}


/*
 * Instantiate the enums of a template class.
 */
static void instantiateTemplateEnums(sipSpec *pt, classTmplDef *tcd,
        templateDef *td, classDef *cd, ifaceFileList **used,
        scopedNameDef *type_names, scopedNameDef *type_values)
{
    enumDef *ted;
    moduleDef *mod = cd->iff->module;

    for (ted = pt->enums; ted != NULL; ted = ted->next)
        if (ted->ecd == tcd->cd)
        {
            enumDef *ed;
            enumMemberDef *temd;

            ed = sipMalloc(sizeof (enumDef));

            /* Start with a shallow copy. */
            *ed = *ted;

            if (ed->fqcname != NULL)
            {
                ed->fqcname = text2scopedName(cd->iff,
                        scopedNameTail(ed->fqcname));
                ed->cname = cacheName(pt, scopedNameToString(ed->fqcname));
            }

            if (inMainModule())
            {
                if (ed->pyname != NULL)
                    setIsUsedName(ed->pyname);

                if (ed->cname != NULL)
                    setIsUsedName(ed->cname);
            }

            ed->ecd = cd;
            ed->first_alt = ed;
            ed->module = mod;
            ed->members = NULL;

            for (temd = ted->members; temd != NULL; temd = temd->next)
            {
                enumMemberDef *emd;

                emd = sipMalloc(sizeof (enumMemberDef));

                /* Start with a shallow copy. */
                *emd = *temd;
                emd->ed = ed;

                emd->next = ed->members;
                ed->members = emd;
            }

            ed->slots = instantiateTemplateMethods(ted->slots, mod);
            ed->overs = instantiateTemplateOverloads(pt, ted->overs,
                    ted->slots, ed->slots, tcd, td, cd, used, type_names,
                    type_values);

            ed->next = pt->enums;
            pt->enums = ed;
        }
}


/*
 * Instantiate the variables of a template class.
 */
static void instantiateTemplateVars(sipSpec *pt, classTmplDef *tcd,
        templateDef *td, classDef *cd, ifaceFileList **used,
        scopedNameDef *type_names, scopedNameDef *type_values)
{
    varDef *tvd;

    for (tvd = pt->vars; tvd != NULL; tvd = tvd->next)
        if (tvd->ecd == tcd->cd)
        {
            varDef *vd;

            vd = sipMalloc(sizeof (varDef));

            /* Start with a shallow copy. */
            *vd = *tvd;

            if (inMainModule())
                setIsUsedName(vd->pyname);

            vd->fqcname = text2scopedName(cd->iff,
                    scopedNameTail(vd->fqcname));
            vd->ecd = cd;
            vd->module = cd->iff->module;

            templateType(&vd->type, tcd, td, cd);

            vd->accessfunc = templateCode(pt, used, vd->accessfunc, type_names, type_values);
            vd->getcode = templateCode(pt, used, vd->getcode, type_names, type_values);
            vd->setcode = templateCode(pt, used, vd->setcode, type_names, type_values);

            addVariable(pt, vd);
        }
}


/*
 * Instantiate the typedefs of a template class.
 */
static void instantiateTemplateTypedefs(sipSpec *pt, classTmplDef *tcd,
        templateDef *td, classDef *cd)
{
    typedefDef *tdd;

    for (tdd = pt->typedefs; tdd != NULL; tdd = tdd->next)
    {
        typedefDef *new_tdd;

        if (tdd->ecd != tcd->cd)
            continue;

        new_tdd = sipMalloc(sizeof (typedefDef));

        /* Start with a shallow copy. */
        *new_tdd = *tdd;

        new_tdd->fqname = text2scopedName(cd->iff,
                scopedNameTail(new_tdd->fqname));
        new_tdd->ecd = cd;
        new_tdd->module = cd->iff->module;

        templateType(&new_tdd->type, tcd, td, cd);

        addTypedef(pt, new_tdd);
    }
}


/*
 * Replace any template arguments in a signature.
 */
static void templateSignature(signatureDef *sd, int result, classTmplDef *tcd, templateDef *td, classDef *ncd)
{
    int a;

    if (result)
        templateType(&sd->result, tcd, td, ncd);

    for (a = 0; a < sd->nrArgs; ++a)
        templateType(&sd->args[a], tcd, td, ncd);
}


/*
 * Replace any template arguments in a type.
 */
static void templateType(argDef *ad, classTmplDef *tcd, templateDef *td, classDef *ncd)
{
    int a;
    char *name;

    /* Descend into any sub-templates. */
    if (ad->atype == template_type)
    {
        templateDef *new_td = sipMalloc(sizeof (templateDef));

        /* Make a deep copy of the template definition. */
        *new_td = *ad->u.td;
        ad->u.td = new_td;

        templateSignature(&ad->u.td->types, FALSE, tcd, td, ncd);

        return;
    }

    /* Ignore if it isn't an unscoped name. */
    if (ad->atype != defined_type || ad->u.snd->next != NULL)
        return;

    name = ad->u.snd->name;

    for (a = 0; a < tcd->sig.nrArgs - 1; ++a)
        if (strcmp(name, scopedNameTail(tcd->sig.args[a].u.snd)) == 0)
        {
            argDef *tad = &td->types.args[a];

            ad->atype = tad->atype;

            /* We take the constrained flag from the real type. */
            resetIsConstrained(ad);

            if (isConstrained(tad))
                setIsConstrained(ad);

            ad->u = tad->u;

            return;
        }

    /* Handle the class name itself. */
    if (strcmp(name, scopedNameTail(classFQCName(tcd->cd))) == 0)
    {
        ad->atype = class_type;
        ad->u.cd = ncd;
        ad->original_type = NULL;
    }
}


/*
 * Replace any template arguments in a literal code block.
 */
codeBlockList *templateCode(sipSpec *pt, ifaceFileList **used,
        codeBlockList *ocbl, scopedNameDef *names, scopedNameDef *values)
{
    codeBlockList *ncbl = NULL;

    while (ocbl != NULL)
    {
        char *at = ocbl->block->frag;
        int start_of_line = TRUE;

        do
        {
            char *from = at, *first = NULL;
            codeBlock *cb;
            scopedNameDef *nam, *val, *nam_first, *val_first;

            /*
             * Don't do any substitution in lines that appear to be
             * preprocessor directives.  This prevents #include'd file names
             * being broken.
             */
            if (start_of_line)
            {
                /* Strip leading whitespace. */
                while (isspace(*from))
                    ++from;

                if (*from == '#')
                {
                    /* Skip to the end of the line. */
                    do
                        ++from;
                    while (*from != '\n' && *from != '\0');
                }
                else
                {
                    start_of_line = FALSE;
                }
            }

            /*
             * Go through the rest of this fragment looking for each of the
             * types and the name of the class itself.
             */
            nam = names;
            val = values;

            while (nam != NULL && val != NULL)
            {
                char *cp;

                if ((cp = strstr(from, nam->name)) != NULL)
                    if (first == NULL || first > cp)
                    {
                        nam_first = nam;
                        val_first = val;
                        first = cp;
                    }

                nam = nam->next;
                val = val->next;
            }

            /* Create the new fragment. */
            cb = sipMalloc(sizeof (codeBlock));

            if (at == ocbl->block->frag)
            {
                cb->filename = ocbl->block->filename;
                cb->linenr = ocbl->block->linenr;
            }
            else
                cb->filename = NULL;

            appendCodeBlock(&ncbl, cb);

            /* See if anything was found. */
            if (first == NULL)
            {
                /* We can just point to this. */
                cb->frag = at;

                /* All done with this one. */
                at = NULL;
            }
            else
            {
                static char *gen_names[] = {
                    "sipType_",
                    "sipClass_",
                    "sipEnum_",
                    "sipException_",
                    NULL
                };

                char *dp, *sp, **gn;
                int genname = FALSE;

                /*
                 * If the context in which the text is used is in the name of a
                 * SIP generated object then translate any "::" scoping to "_"
                 * and remove any const.
                 */
                for (gn = gen_names; *gn != NULL; ++gn)
                    if (search_back(first, at, *gn))
                    {
                        addUsedFromCode(pt, used, val_first->name);
                        genname = TRUE;
                        break;
                    }

                /* Fragment the fragment. */
                cb->frag = sipMalloc(first - at + strlen(val_first->name) + 1);

                strncpy(cb->frag, at, first - at);

                dp = &cb->frag[first - at];
                sp = val_first->name;

                if (genname)
                {
                    char gch;

                    if (strlen(sp) > 6 && strncmp(sp, "const ", 6) == 0)
                        sp += 6;

                    while ((gch = *sp++) != '\0')
                        if (gch == ':' && *sp == ':')
                        {
                            *dp++ = '_';
                            ++sp;
                        }
                        else
                            *dp++ = gch;

                    *dp = '\0';
                }
                else
                    strcpy(dp, sp);

                /* Move past the replaced text. */
                at = first + strlen(nam_first->name);

                if (*at == '\n')
                    start_of_line = TRUE;
            }
        }
        while (at != NULL && *at != '\0');

        ocbl = ocbl->next;
    }

    return ncbl;
}


/*
 * Return TRUE if the text at the end of a string matches the target string.
 */
static int search_back(const char *end, const char *start, const char *target)
{
    size_t tlen = strlen(target);

    if (start + tlen >= end)
        return FALSE;

    return (strncmp(end - tlen, target, tlen) == 0);
}


/*
 * Add any needed interface files based on handwritten code.
 */
static void addUsedFromCode(sipSpec *pt, ifaceFileList **used, const char *sname)
{
    ifaceFileDef *iff;
    enumDef *ed;

    for (iff = pt->ifacefiles; iff != NULL; iff = iff->next)
    {
        if (iff->type != class_iface && iff->type != exception_iface)
            continue;

        if (sameName(iff->fqcname, sname))
        {
            addToUsedList(used, iff);
            return;
        }
    }

    for (ed = pt->enums; ed != NULL; ed = ed->next)
    {
        if (ed->ecd == NULL)
            continue;

        if (sameName(ed->fqcname, sname))
        {
            addToUsedList(used, ed->ecd->iff);
            return;
        }
    }
}


/*
 * Compare a scoped name with its string equivalent.
 */
static int sameName(scopedNameDef *snd, const char *sname)
{
    while (snd != NULL && *sname != '\0')
    {
        const char *sp = snd->name;

        while (*sp != '\0' && *sname != ':' && *sname != '\0')
            if (*sp++ != *sname++)
                return FALSE;

        if (*sp != '\0' || (*sname != ':' && *sname != '\0'))
            return FALSE;

        snd = snd->next;

        if (*sname == ':')
            sname += 2;
    }

    return (snd == NULL && *sname == '\0');
}


/*
 * Compare a (possibly) relative scoped name with a fully qualified scoped name
 * while taking the current scope into account.
 */
static int foundInScope(scopedNameDef *fq_name, scopedNameDef *rel_name)
{
    classDef *scope;

    for (scope = currentScope(); scope != NULL; scope = scope->ecd)
    {
        scopedNameDef *snd;
        int found;

        snd = copyScopedName(classFQCName(scope));
        appendScopedName(&snd, copyScopedName(rel_name));

        found = (compareScopedNames(fq_name, snd) == 0);

        freeScopedName(snd);

        if (found)
            return TRUE;
    }

    return compareScopedNames(fq_name, rel_name) == 0;
}


/*
 * Create a new typedef.
 */
static void newTypedef(sipSpec *pt, moduleDef *mod, char *name, argDef *type,
        optFlags *optflgs)
{
    typedefDef *td;
    scopedNameDef *fqname;
    classDef *scope;

    scope = currentScope();
    fqname = text2scopedName((scope != NULL ? scope->iff : NULL), name);

    /* See if we are instantiating a template class. */
    if (type->atype == template_type)
    {
        classTmplDef *tcd;
        templateDef *td = type->u.td;

        for (tcd = pt->classtemplates; tcd != NULL; tcd = tcd->next)
            if (foundInScope(tcd->cd->iff->fqcname, td->fqname) &&
                sameTemplateSignature(&tcd->sig, &td->types, FALSE))
            {
                instantiateClassTemplate(pt, mod, scope, fqname, tcd, td,
                        getPythonName(mod, optflgs, name));

                /* All done. */
                return;
            }
    }

    td = sipMalloc(sizeof (typedefDef));

    td->tdflags = 0;
    td->fqname = fqname;
    td->ecd = scope;
    td->module = mod;
    td->type = *type;

    if (getOptFlag(optflgs, "Capsule", bool_flag) != NULL)
    {
        /* Make sure the type is void *. */
        if (type->atype != void_type || type->nrderefs != 1 || isConstArg(type) || isReference(type))
        {
            fatalScopedName(fqname);
            fatal(" must be a void* if /Capsule/ is specified\n");
        }

        td->type.atype = capsule_type;
        td->type.nrderefs = 0;
        td->type.u.cap = fqname;
    }

    if (getOptFlag(optflgs, "NoTypeName", bool_flag) != NULL)
        setNoTypeName(td);

    addTypedef(pt, td);
}


/*
 * Add a typedef to the list so that the list remains sorted.
 */
static void addTypedef(sipSpec *pt, typedefDef *tdd)
{
    typedefDef **tdp;

    /*
     * Check it doesn't already exist and find the position in the sorted list
     * where it should be put.
     */
    for (tdp = &pt->typedefs; *tdp != NULL; tdp = &(*tdp)->next)
    {
        int res = compareScopedNames((*tdp)->fqname, tdd->fqname);

        if (res == 0)
        {
            fatalScopedName(tdd->fqname);
            fatal(" already defined\n");
        }

        if (res > 0)
            break;
    }

    tdd->next = *tdp;
    *tdp = tdd;

    tdd->module->nrtypedefs++;
}


/*
 * Speculatively try and resolve any typedefs.  In some cases (eg. when
 * comparing template signatures) it helps to use the real type if it is known.
 * Note that this wouldn't be necessary if we required that all types be known
 * before they are used.
 */
static void resolveAnyTypedef(sipSpec *pt, argDef *ad)
{
    argDef orig = *ad;

    while (ad->atype == defined_type)
    {
        ad->atype = no_type;
        searchTypedefs(pt, ad->u.snd, ad);

        /*
         * Don't resolve to a template type as it may be superceded later on
         * by a more specific mapped type.
         */
        if (ad->atype == no_type || ad->atype == template_type)
        {
            *ad = orig;
            break;
        }
    }
}


/*
 * Return TRUE if the template signatures are the same.  A deep comparison is
 * used for mapped type templates where we want to recurse into any nested
 * templates.
 */
int sameTemplateSignature(signatureDef *tmpl_sd, signatureDef *args_sd,
        int deep)
{
    int a;

    if (tmpl_sd->nrArgs != args_sd->nrArgs)
        return FALSE;

    for (a = 0; a < tmpl_sd->nrArgs; ++a)
    {
        argDef *tmpl_ad = &tmpl_sd->args[a];
        argDef *args_ad = &args_sd->args[a];

        /*
         * If we are doing a shallow comparision (ie. for class templates) then
         * a type name in the template signature matches anything in the
         * argument signature.
         */
        if (tmpl_ad->atype == defined_type && !deep)
            continue;

        /*
         * For type names only compare the references and pointers, and do the
         * same for any nested templates.
         */
        if (tmpl_ad->atype == defined_type && args_ad->atype == defined_type)
        {
            if (isReference(tmpl_ad) != isReference(args_ad) || tmpl_ad->nrderefs != args_ad->nrderefs)
                return FALSE;
        }
        else if (tmpl_ad->atype == template_type && args_ad->atype == template_type)
        {
            if (!sameTemplateSignature(&tmpl_ad->u.td->types, &args_ad->u.td->types, deep))
                return FALSE;
        }
        else if (!sameBaseType(tmpl_ad, args_ad))
            return FALSE;
    }

    return TRUE;
}


/*
 * Create a new variable.
 */
static void newVar(sipSpec *pt, moduleDef *mod, char *name, int isstatic,
        argDef *type, optFlags *of, codeBlock *acode, codeBlock *gcode,
        codeBlock *scode, int section)
{
    varDef *var;
    classDef *escope = currentScope();
    nameDef *nd;

    /*
     * For the moment we don't support capsule variables because it needs the
     * API major version increasing.
     */
    if (type->atype == capsule_type)
        yyerror("Capsule variables not yet supported");

    /* Check the section. */
    if (section != 0)
    {
        if ((section & SECT_IS_PUBLIC) == 0)
            yyerror("Class variables must be in the public section");

        if (!isstatic && acode != NULL)
            yyerror("%AccessCode cannot be specified for non-static class variables");
    }

    if (isstatic && pt->genc)
        yyerror("Cannot have static members in a C structure");

    if (gcode != NULL || scode != NULL)
    {
        if (acode != NULL)
            yyerror("Cannot mix %AccessCode and %GetCode or %SetCode");

        if (escope == NULL)
            yyerror("Cannot specify %GetCode or %SetCode for global variables");
    }

    applyTypeFlags(mod, type, of);

    nd = cacheName(pt, getPythonName(mod, of, name));

    if (inMainModule())
        setIsUsedName(nd);

    checkAttributes(pt, mod, escope, NULL, nd->text, FALSE);

    var = sipMalloc(sizeof (varDef));

    var->pyname = nd;
    var->fqcname = text2scopedName((escope != NULL ? escope->iff : NULL),
            name);
    var->ecd = escope;
    var->module = mod;
    var->varflags = 0;
    var->type = *type;
    appendCodeBlock(&var->accessfunc, acode);
    appendCodeBlock(&var->getcode, gcode);
    appendCodeBlock(&var->setcode, scode);

    if (isstatic || (escope != NULL && escope->iff->type == namespace_iface))
        setIsStaticVar(var);

    addVariable(pt, var);
}


/*
 * Create a new ctor.
 */
static void newCtor(moduleDef *mod, char *name, int sectFlags,
        signatureDef *args, optFlags *optflgs, codeBlock *methodcode,
        throwArgs *exceptions, signatureDef *cppsig, int explicit,
        codeBlock *docstring)
{
    ctorDef *ct, **ctp;
    classDef *cd = currentScope();

    /* Check the name of the constructor. */
    if (strcmp(classBaseName(cd), name) != 0)
        yyerror("Constructor doesn't have the same name as its class");

    if (docstring != NULL)
        appendCodeBlock(&cd->docstring, docstring);

    /* Add to the list of constructors. */
    ct = sipMalloc(sizeof (ctorDef));

    if (sectFlags & SECT_IS_PROT && makeProtPublic)
    {
        sectFlags &= ~SECT_IS_PROT;
        sectFlags |= SECT_IS_PUBLIC;
    }

    /* Allow the signature to be used like an function signature. */
    memset(&args->result, 0, sizeof (argDef));
    args->result.atype = void_type;

    ct->ctorflags = sectFlags;
    ct->api_range = getAPIRange(optflgs);
    ct->pysig = *args;
    ct->cppsig = (cppsig != NULL ? cppsig : &ct->pysig);
    ct->exceptions = exceptions;
    appendCodeBlock(&ct->methodcode, methodcode);

    if (!isPrivateCtor(ct))
        setCanCreate(cd);

    if (isProtectedCtor(ct))
        setHasShadow(cd);

    if (explicit)
        setIsExplicitCtor(ct);

    getHooks(optflgs, &ct->prehook, &ct->posthook);

    if (getReleaseGIL(optflgs))
        setIsReleaseGILCtor(ct);
    else if (getHoldGIL(optflgs))
        setIsHoldGILCtor(ct);

    if (getTransfer(optflgs))
        setIsResultTransferredCtor(ct);

    if (getDeprecated(optflgs))
        setIsDeprecatedCtor(ct);

    if (!isPrivateCtor(ct))
        ct->kwargs = keywordArgs(mod, optflgs, &ct->pysig, FALSE);

    if (methodcode == NULL && getOptFlag(optflgs, "NoRaisesPyException", bool_flag) == NULL)
    {
        if (allRaisePyException(mod) || getOptFlag(optflgs, "RaisesPyException", bool_flag) != NULL)
            setRaisesPyExceptionCtor(ct);
    }

    if (getOptFlag(optflgs, "NoDerived", bool_flag) != NULL)
    {
        if (cppsig != NULL)
            yyerror("The /NoDerived/ annotation cannot be used with a C++ signature");

        if (methodcode == NULL)
            yyerror("The /NoDerived/ annotation must be used with %MethodCode");

        ct->cppsig = NULL;
    }

    if (getOptFlag(optflgs, "Default", bool_flag) != NULL)
    {
        if (cd->defctor != NULL)
            yyerror("A constructor with the /Default/ annotation has already been defined");

        cd->defctor = ct;
    }

    /* Append to the list. */
    for (ctp = &cd->ctors; *ctp != NULL; ctp = &(*ctp)->next)
        ;

    *ctp = ct;
}


/*
 * Create a new function.
 */
static void newFunction(sipSpec *pt, moduleDef *mod, classDef *c_scope,
        mappedTypeDef *mt_scope, int sflags, int isstatic, int issignal,
        int isslot, int isvirt, char *name, signatureDef *sig, int isconst,
        int isabstract, optFlags *optflgs, codeBlock *methodcode,
        codeBlock *vcode, throwArgs *exceptions, signatureDef *cppsig,
        codeBlock *docstring)
{
    static const char *annos[] = {
        "__len__",
        "API",
        "AutoGen",
        "Deprecated",
        "DocType",
        "Encoding",
        "Factory",
        "HoldGIL",
        "KeywordArgs",
        "KeepReference",
        "NewThread",
        "NoArgParser",
        "NoCopy",
        "NoRaisesPyException",
        "NoVirtualErrorHandler",
        "Numeric",
        "PostHook",
        "PreHook",
        "PyInt",
        "PyName",
        "RaisesPyException",
        "ReleaseGIL",
        "Sequence",
        "VirtualErrorHandler",
        "Transfer",
        "TransferBack",
        "TransferThis",
        NULL
    };

    const char *pyname, *virt_error_handler;
    int factory, xferback, no_arg_parser, no_virt_error_handler;
    overDef *od, **odp, **headp;
    optFlag *of;
    virtHandlerDef *vhd;

    checkAnnos(optflgs, annos);

    /* Extra checks for a C module. */
    if (pt->genc)
    {
        if (c_scope != NULL)
            yyerror("Function declaration not allowed in a struct in a C module");

        if (isstatic)
            yyerror("Static functions not allowed in a C module");

        if (exceptions != NULL)
            yyerror("Exceptions not allowed in a C module");

        /* Handle C void prototypes. */
        if (sig->nrArgs == 1)
        {
            argDef *vad = &sig->args[0];

            if (vad->atype == void_type && vad->nrderefs == 0)
                sig->nrArgs = 0;
        }
    }

    if (mt_scope != NULL)
        headp = &mt_scope->overs;
    else if (c_scope != NULL)
        headp = &c_scope->overs;
    else
        headp = &mod->overs;

    /*
     * See if the function has a non-lazy method.  These are methods that
     * Python expects to see defined in the type before any instance of the
     * type is created.
     */
    if (c_scope != NULL)
    {
        static const char *lazy[] = {
            "__getattribute__",
            "__getattr__",
            "__enter__",
            "__exit__",
            NULL
        };

        const char **l;

        for (l = lazy; *l != NULL; ++l)
            if (strcmp(name, *l) == 0)
            {
                setHasNonlazyMethod(c_scope);
                break;
            }
    }

    /* See if it is a factory method. */
    if (getOptFlag(optflgs, "Factory", bool_flag) != NULL)
        factory = TRUE;
    else
    {
        int a;

        factory = FALSE;

        /* Check /TransferThis/ wasn't specified. */
        if (c_scope == NULL || isstatic)
            for (a = 0; a < sig->nrArgs; ++a)
                if (isThisTransferred(&sig->args[a]))
                    yyerror("/TransferThis/ may only be specified in constructors and class methods");
    }

    /* See if the result is to be returned to Python ownership. */
    xferback = (getOptFlag(optflgs, "TransferBack", bool_flag) != NULL);

    if (factory && xferback)
        yyerror("/TransferBack/ and /Factory/ cannot both be specified");

    /* Create a new overload definition. */

    od = sipMalloc(sizeof (overDef));

    getSourceLocation(&od->sloc);

    /* Set the overload flags. */

    if ((sflags & SECT_IS_PROT) && makeProtPublic)
    {
        sflags &= ~SECT_IS_PROT;
        sflags |= SECT_IS_PUBLIC | OVER_REALLY_PROT;
    }

    od->overflags = sflags;

    if (issignal)
    {
        resetIsSlot(od);
        setIsSignal(od);
    }
    else if (isslot)
    {
        resetIsSignal(od);
        setIsSlot(od);
    }

    if (factory)
        setIsFactory(od);

    if (xferback)
        setIsResultTransferredBack(od);

    if (getTransfer(optflgs))
        setIsResultTransferred(od);

    if (getOptFlag(optflgs, "TransferThis", bool_flag) != NULL)
        setIsThisTransferredMeth(od);

    if (methodcode == NULL && getOptFlag(optflgs, "NoRaisesPyException", bool_flag) == NULL)
    {
        if (allRaisePyException(mod) || getOptFlag(optflgs, "RaisesPyException", bool_flag) != NULL)
            setRaisesPyException(od);
    }

    if (isProtected(od))
        setHasShadow(c_scope);

    if ((isSlot(od) || isSignal(od)) && !isPrivate(od))
    {
        if (isSignal(od))
            setHasShadow(c_scope);

        pt->sigslots = TRUE;
    }

    if (isSignal(od) && (methodcode != NULL || vcode != NULL))
        yyerror("Cannot provide code for signals");

    if (isstatic)
    {
        if (isSignal(od))
            yyerror("Static functions cannot be signals");

        if (isvirt)
            yyerror("Static functions cannot be virtual");

        setIsStatic(od);
    }

    if (isconst)
        setIsConst(od);

    if (isabstract)
    {
        if (sflags == 0)
            yyerror("Non-class function specified as abstract");

        setIsAbstract(od);
    }

    if ((of = getOptFlag(optflgs, "AutoGen", opt_name_flag)) != NULL)
    {
        if (of->fvalue.sval == NULL || isEnabledFeature(of->fvalue.sval))
            setIsAutoGen(od);
    }

    virt_error_handler = getVirtErrorHandler(optflgs);
    no_virt_error_handler = (getOptFlag(optflgs, "NoVirtualErrorHandler", bool_flag) != NULL);

    if (isvirt)
    {
        if (isSignal(od) && pluginPyQt3(pt))
            yyerror("Virtual signals aren't supported");

        setIsVirtual(od);
        setHasShadow(c_scope);

        vhd = sipMalloc(sizeof (virtHandlerDef));

        vhd->virthandlernr = -1;
        vhd->vhflags = 0;
        vhd->pysig = &od->pysig;
        vhd->cppsig = (cppsig != NULL ? cppsig : &od->pysig);
        appendCodeBlock(&vhd->virtcode, vcode);

        if (factory || xferback)
            setIsTransferVH(vhd);

        if (no_virt_error_handler)
        {
            if (virt_error_handler != NULL)
                yyerror("/VirtualErrorHandler/ and /NoVirtualErrorHandler/ provided");

            setNoErrorHandler(od);
        }
        else
        {
            od->virt_error_handler = virt_error_handler;
        }

        /*
         * Only add it to the module's virtual handlers if we are not in a
         * class template.
         */
        if (!currentIsTemplate)
        {
            vhd->module = mod;

            vhd->next = mod->virthandlers;
            mod->virthandlers = vhd;
        }
    }
    else
    {
        if (vcode != NULL)
            yyerror("%VirtualCatcherCode provided for non-virtual function");

        if (virt_error_handler != NULL)
            yyerror("/VirtualErrorHandler/ provided for non-virtual function");

        if (no_virt_error_handler)
            yyerror("/NoVirtualErrorHandler/ provided for non-virtual function");

        vhd = NULL;
    }

    od->cppname = name;
    od->pysig = *sig;
    od->cppsig = (cppsig != NULL ? cppsig : &od->pysig);
    od->exceptions = exceptions;
    appendCodeBlock(&od->methodcode, methodcode);
    od->virthandler = vhd;

    no_arg_parser = (getOptFlag(optflgs, "NoArgParser", bool_flag) != NULL);

    if (no_arg_parser)
    {
        if (methodcode == NULL)
            yyerror("%MethodCode must be supplied if /NoArgParser/ is specified");
    }

    if (getOptFlag(optflgs, "NoCopy", bool_flag) != NULL)
        setNoCopy(&od->pysig.result);

    handleKeepReference(optflgs, &od->pysig.result, mod);

    pyname = getPythonName(mod, optflgs, name);

    od->common = findFunction(pt, mod, c_scope, mt_scope, pyname,
            (methodcode != NULL), sig->nrArgs, no_arg_parser);

    if (isProtected(od))
        setHasProtected(od->common);

    if (strcmp(pyname, "__delattr__") == 0)
        setIsDelattr(od);

    if (docstring != NULL)
        appendCodeBlock(&od->common->docstring, docstring);

    od->api_range = getAPIRange(optflgs);

    if (od->api_range == NULL)
        setNotVersioned(od->common);

    if (getOptFlag(optflgs, "Numeric", bool_flag) != NULL)
    {
        if (isSequence(od->common))
            yyerror("/Sequence/ has already been specified");

        setIsNumeric(od->common);
    }

    if (getOptFlag(optflgs, "Sequence", bool_flag) != NULL)
    {
        if (isNumeric(od->common))
            yyerror("/Numeric/ has already been specified");

        setIsSequence(od->common);
    }

    /* Methods that run in new threads must be virtual. */
    if (getOptFlag(optflgs, "NewThread", bool_flag) != NULL)
    {
        argDef *res;

        if (!isvirt)
            yyerror("/NewThread/ may only be specified for virtual functions");

        /*
         * This is an arbitary limitation to make the code generator slightly
         * easier - laziness on my part.
         */
        res = &od->cppsig->result;

        if (res->atype != void_type || res->nrderefs != 0)
            yyerror("/NewThread/ may only be specified for void functions");

        setIsNewThread(od);
    }

    getHooks(optflgs, &od->prehook, &od->posthook);

    if (getReleaseGIL(optflgs))
        setIsReleaseGIL(od);
    else if (getHoldGIL(optflgs))
        setIsHoldGIL(od);

    if (getDeprecated(optflgs))
        setIsDeprecated(od);

    if (!isPrivate(od) && !isSignal(od) && (od->common->slot == no_slot || od->common->slot == call_slot))
    {
        od->kwargs = keywordArgs(mod, optflgs, &od->pysig, hasProtected(od->common));

        if (od->kwargs != NoKwArgs)
            setUseKeywordArgs(od->common);

        /*
         * If the overload is protected and defined in an imported module then
         * we need to make sure that any other overloads' keyword argument
         * names are marked as used.
         */
        if (isProtected(od) && !inMainModule())
        {
            overDef *kwod;

            for (kwod = c_scope->overs; kwod != NULL; kwod = kwod->next)
                if (kwod->common == od->common && kwod->kwargs != NoKwArgs)
                {
                    int a;

                    for (a = 0; a < kwod->pysig.nrArgs; ++a)
                    {
                        argDef *ad = &kwod->pysig.args[a];

                        if (kwod->kwargs == OptionalKwArgs && ad->defval == NULL)
                            continue;

                        if (ad->name != NULL)
                            setIsUsedName(ad->name);
                    }
                }
        }
    }

    /* See if we want to auto-generate a __len__() method. */
    if (getOptFlag(optflgs, "__len__", bool_flag) != NULL)
    {
        overDef *len;

        len = sipMalloc(sizeof (overDef));

        len->cppname = "__len__";
        len->overflags = SECT_IS_PUBLIC;
        len->pysig.result.atype = ssize_type;
        len->pysig.nrArgs = 0;
        len->cppsig = &len->pysig;

        len->common = findFunction(pt, mod, c_scope, mt_scope, len->cppname,
                TRUE, 0, FALSE);

        if ((len->methodcode = od->methodcode) == NULL)
        {
            char *buf = sipStrdup("            sipRes = (SIP_SSIZE_T)sipCpp->");
            codeBlock *code;

            append(&buf, od->cppname);
            append(&buf, "();\n");

            code = sipMalloc(sizeof (codeBlock));

            code->frag = buf;
            code->filename = "Auto-generated";
            code->linenr = 1;

            appendCodeBlock(&len->methodcode, code);
        }

        len->next = NULL;

        od->next = len;
    }
    else
    {
        od->next = NULL;
    }

    /* Append to the list. */
    for (odp = headp; *odp != NULL; odp = &(*odp)->next)
        ;

    *odp = od;
}


/*
 * Return the Python name based on the C/C++ name and any /PyName/ annotation.
 */
static const char *getPythonName(moduleDef *mod, optFlags *optflgs,
        const char *cname)
{
    const char *pname;
    optFlag *of;
    autoPyNameDef *apnd;

    /* Use the explicit name if given. */
    if ((of = getOptFlag(optflgs, "PyName", name_flag)) != NULL)
        return of->fvalue.sval;

    /* Apply any automatic naming rules. */
    pname = cname;

    for (apnd = mod->autopyname; apnd != NULL; apnd = apnd->next)
    {
        size_t len = strlen(apnd->remove_leading);

        if (strncmp(pname, apnd->remove_leading, len) == 0)
            pname += len;
    }

    return pname;
}


/*
 * Cache a name in a module.  Entries in the cache are stored in order of
 * decreasing length.
 */
nameDef *cacheName(sipSpec *pt, const char *name)
{
    nameDef *nd, **ndp;
    size_t len;

    /* Allow callers to be lazy about checking if there is really a name. */
    if (name == NULL)
        return NULL;

    /* Skip entries that are too large. */
    ndp = &pt->namecache;
    len = strlen(name);

    while (*ndp != NULL && (*ndp)->len > len)
        ndp = &(*ndp)->next;

    /* Check entries that are the right length. */
    for (nd = *ndp; nd != NULL && nd->len == len; nd = nd->next)
        if (memcmp(nd->text, name, len) == 0)
            return nd;

    /* Create a new one. */
    nd = sipMalloc(sizeof (nameDef));

    nd->nameflags = 0;
    nd->text = name;
    nd->len = len;
    nd->next = *ndp;

    *ndp = nd;

    return nd;
}


/*
 * Find (or create) an overloaded function name.
 */
static memberDef *findFunction(sipSpec *pt, moduleDef *mod, classDef *c_scope,
        mappedTypeDef *mt_scope, const char *pname, int hwcode, int nrargs,
        int no_arg_parser)
{
    static struct slot_map {
        const char *name;   /* The slot name. */
        slotType type;      /* The corresponding type. */
        int needs_hwcode;   /* Set if handwritten code is required. */
        int nrargs;         /* Nr. of arguments. */
    } slot_table[] = {
        {"__str__", str_slot, TRUE, 0},
        {"__unicode__", unicode_slot, TRUE, 0},
        {"__int__", int_slot, FALSE, 0},
        {"__long__", long_slot, FALSE, 0},
        {"__float__", float_slot, FALSE, 0},
        {"__len__", len_slot, TRUE, 0},
        {"__contains__", contains_slot, TRUE, 1},
        {"__add__", add_slot, FALSE, 1},
        {"__sub__", sub_slot, FALSE, 1},
        {"__mul__", mul_slot, FALSE, 1},
        {"__div__", div_slot, FALSE, 1},
        {"__mod__", mod_slot, FALSE, 1},
        {"__floordiv__", floordiv_slot, TRUE, 1},
        {"__truediv__", truediv_slot, FALSE, 1},
        {"__and__", and_slot, FALSE, 1},
        {"__or__", or_slot, FALSE, 1},
        {"__xor__", xor_slot, FALSE, 1},
        {"__lshift__", lshift_slot, FALSE, 1},
        {"__rshift__", rshift_slot, FALSE, 1},
        {"__iadd__", iadd_slot, FALSE, 1},
        {"__isub__", isub_slot, FALSE, 1},
        {"__imul__", imul_slot, FALSE, 1},
        {"__idiv__", idiv_slot, FALSE, 1},
        {"__imod__", imod_slot, FALSE, 1},
        {"__ifloordiv__", ifloordiv_slot, TRUE, 1},
        {"__itruediv__", itruediv_slot, FALSE, 1},
        {"__iand__", iand_slot, FALSE, 1},
        {"__ior__", ior_slot, FALSE, 1},
        {"__ixor__", ixor_slot, FALSE, 1},
        {"__ilshift__", ilshift_slot, FALSE, 1},
        {"__irshift__", irshift_slot, FALSE, 1},
        {"__invert__", invert_slot, FALSE, 0},
        {"__call__", call_slot, FALSE, -1},
        {"__getitem__", getitem_slot, FALSE, 1},
        {"__setitem__", setitem_slot, TRUE, 2},
        {"__delitem__", delitem_slot, TRUE, 1},
        {"__lt__", lt_slot, FALSE, 1},
        {"__le__", le_slot, FALSE, 1},
        {"__eq__", eq_slot, FALSE, 1},
        {"__ne__", ne_slot, FALSE, 1},
        {"__gt__", gt_slot, FALSE, 1},
        {"__ge__", ge_slot, FALSE, 1},
        {"__cmp__", cmp_slot, FALSE, 1},
        {"__bool__", bool_slot, TRUE, 0},
        {"__nonzero__", bool_slot, TRUE, 0},
        {"__neg__", neg_slot, FALSE, 0},
        {"__pos__", pos_slot, FALSE, 0},
        {"__abs__", abs_slot, TRUE, 0},
        {"__repr__", repr_slot, TRUE, 0},
        {"__hash__", hash_slot, TRUE, 0},
        {"__index__", index_slot, TRUE, 0},
        {"__iter__", iter_slot, TRUE, 0},
        {"__next__", next_slot, TRUE, 0},
        {"__setattr__", setattr_slot, TRUE, 2},
        {"__delattr__", delattr_slot, TRUE, 1},
        {NULL}
    };

    memberDef *md, **flist;
    struct slot_map *sm;
    slotType st;

    /* Get the slot type. */
    st = no_slot;

    for (sm = slot_table; sm->name != NULL; ++sm)
        if (strcmp(sm->name, pname) == 0)
        {
            if (sm->needs_hwcode && !hwcode)
                yyerror("This Python slot requires %MethodCode");

            if (sm->nrargs >= 0)
            {
                if (mt_scope == NULL && c_scope == NULL)
                {
                    /* Global operators need one extra argument. */
                    if (sm -> nrargs + 1 != nrargs)
                        yyerror("Incorrect number of arguments to global operator");
                }
                else if (sm->nrargs != nrargs)
                    yyerror("Incorrect number of arguments to Python slot");
            }

            st = sm->type;

            break;
        }

    /* Check there is no name clash. */
    checkAttributes(pt, mod, c_scope, mt_scope, pname, TRUE);

    /* See if it already exists. */
    if (mt_scope != NULL)
        flist = &mt_scope->members;
    else if (c_scope != NULL)
        flist = &c_scope->members;
    else
        flist = &mod->othfuncs;

    /* __delattr__ is implemented as __setattr__. */
    if (st == delattr_slot)
    {
        if (inMainModule())
            setIsUsedName(cacheName(pt, pname));

        st = setattr_slot;
        pname = "__setattr__";
    }

    for (md = *flist; md != NULL; md = md->next)
        if (strcmp(md->pyname->text, pname) == 0 && md->module == mod)
            break;

    if (md == NULL)
    {
        /* Create a new one. */
        md = sipMalloc(sizeof (memberDef));

        md->pyname = cacheName(pt, pname);
        md->memberflags = 0;
        md->slot = st;
        md->module = mod;
        md->next = *flist;

        *flist = md;

        if (inMainModule())
            setIsUsedName(md->pyname);

        if (no_arg_parser)
            setNoArgParser(md);
    }
    else if (noArgParser(md))
        yyerror("Another overload has already been defined that is annotated as /NoArgParser/");

    /* Global operators are a subset. */
    if (mt_scope == NULL && c_scope == NULL && st != no_slot && st != neg_slot && st != pos_slot && !isNumberSlot(md) && !isInplaceNumberSlot(md) && !isRichCompareSlot(md))
        yyerror("Global operators must be either numeric or comparison operators");

    return md;
}


/*
 * Search a set of flags for a particular one.
 */
static optFlag *findOptFlag(optFlags *flgs, const char *name)
{
    int f;

    for (f = 0; f < flgs->nrFlags; ++f)
    {
        optFlag *of = &flgs->flags[f];

        if (strcmp(of->fname, name) == 0)
            return of;
    }

    return NULL;
}


/*
 * Search a set of flags for a particular one and check its type.
 */
static optFlag *getOptFlag(optFlags *flgs, const char *name, flagType ft)
{
    optFlag *of = findOptFlag(flgs, name);

    if (of != NULL)
    {
        /* An optional name can look like a boolean or a name. */
        if (ft == opt_name_flag)
        {
            if (of->ftype == bool_flag)
            {
                of->ftype = opt_name_flag;
                of->fvalue.sval = NULL;
            }
            else if (of->ftype == name_flag)
            {
                of->ftype = opt_name_flag;
            }
        }

        /* An optional integer can look like a boolean or an integer. */
        if (ft == opt_integer_flag)
        {
            if (of->ftype == bool_flag)
            {
                of->ftype = opt_integer_flag;
                of->fvalue.ival = -1;
            }
            else if (of->ftype == integer_flag)
            {
                of->ftype = opt_integer_flag;
            }
        }

        if (ft != of->ftype)
            yyerror("Annotation has a value of the wrong type");
    }

    return of;
}


/*
 * A name is going to be used as a Python attribute name within a Python scope
 * (ie. a Python dictionary), so check against what we already know is going in
 * the same scope in case there is a clash.
 */
static void checkAttributes(sipSpec *pt, moduleDef *mod, classDef *py_c_scope,
        mappedTypeDef *py_mt_scope, const char *attr, int isfunc)
{
    enumDef *ed;
    varDef *vd;
    classDef *cd;

    /* Check the enums. */

    for (ed = pt->enums; ed != NULL; ed = ed->next)
    {
        enumMemberDef *emd;

        if (ed->pyname == NULL)
            continue;

        if (py_c_scope != NULL)
        {
            if (ed->ecd != py_c_scope)
                continue;
        }
        else if (py_mt_scope != NULL)
        {
            if (ed->emtd != py_mt_scope)
                continue;
        }
        else if (ed->ecd != NULL || ed->emtd != NULL)
        {
            continue;
        }

        if (strcmp(ed->pyname->text, attr) == 0)
            yyerror("There is already an enum in scope with the same Python name");

        for (emd = ed->members; emd != NULL; emd = emd->next)
            if (strcmp(emd->pyname->text, attr) == 0)
                yyerror("There is already an enum member in scope with the same Python name");
    }

    /*
     * Only check the members if this attribute isn't a member because we
     * can handle members with the same name in the same scope.
     */
    if (!isfunc)
    {
        memberDef *md, *membs;
        overDef *overs;

        if (py_mt_scope != NULL)
        {
            membs = py_mt_scope->members;
            overs = py_mt_scope->overs;
        }
        else if (py_c_scope != NULL)
        {
            membs = py_c_scope->members;
            overs = py_c_scope->overs;
        }
        else
        {
            membs = mod->othfuncs;
            overs = mod->overs;
        }

        for (md = membs; md != NULL; md = md->next)
        {
            overDef *od;

            if (strcmp(md->pyname->text, attr) != 0)
                continue;

            /* Check for a conflict with all overloads. */
            for (od = overs; od != NULL; od = od->next)
            {
                if (od->common != md)
                    continue;

                yyerror("There is already a function in scope with the same Python name");
            }
        }
    }

    /* If the scope was a mapped type then that's all we have to check. */
    if (py_mt_scope != NULL)
        return;

    /* Check the variables. */
    for (vd = pt->vars; vd != NULL; vd = vd->next)
    {
        if (vd->ecd != py_c_scope)
            continue;

        if (strcmp(vd->pyname->text,attr) == 0)
            yyerror("There is already a variable in scope with the same Python name");
    }

    /* Check the classes. */
    for (cd = pt->classes; cd != NULL; cd = cd->next)
    {
        if (cd->ecd != py_c_scope || cd->pyname == NULL)
            continue;

        if (strcmp(cd->pyname->text, attr) == 0 && !isExternal(cd))
            yyerror("There is already a class or namespace in scope with the same Python name");
    }

    /* Check the exceptions. */
    if (py_c_scope == NULL)
    {
        exceptionDef *xd;

        for (xd = pt->exceptions; xd != NULL; xd = xd->next)
            if (xd->pyname != NULL && strcmp(xd->pyname, attr) == 0)
                yyerror("There is already an exception with the same Python name");
    }

    /* Check the properties. */
    if (py_c_scope != NULL)
    {
        propertyDef *pd;

        for (pd = py_c_scope->properties; pd != NULL; pd = pd->next)
            if (strcmp(pd->name->text, attr) == 0)
                yyerror("There is already a property with the same name");
    }
}


/*
 * Append a code block to a list of them.
 */
static void appendCodeBlock(codeBlockList **headp, codeBlock *cb)
{
    codeBlockList *cbl;

    /* Handle the trivial case. */
    if (cb == NULL)
        return;

    /* Find the end of the list. */
    while (*headp != NULL)
    {
        /* Ignore if the block is already in the list. */
        if ((*headp)->block == cb)
            return;

        headp = &(*headp)->next;
    }

    cbl = sipMalloc(sizeof (codeBlockList));
    cbl->block = cb;

    *headp = cbl;
}


/*
 * Append a code block list to an existing list.
 */
void appendCodeBlockList(codeBlockList **headp, codeBlockList *cbl)
{
    while (cbl != NULL)
    {
        appendCodeBlock(headp, cbl->block);
        cbl = cbl->next;
    }
}


/*
 * Handle the end of a fully parsed a file.
 */
static void handleEOF()
{
    /*
     * Check that the number of nested if's is the same as when we started
     * the file.
     */

    if (skipStackPtr > currentContext.ifdepth)
        fatal("Too many %%If statements in %s\n", previousFile);

    if (skipStackPtr < currentContext.ifdepth)
        fatal("Too many %%End statements in %s\n", previousFile);
}


/*
 * Handle the end of a fully parsed a module.
 */
static void handleEOM()
{
    moduleDef *from;

    /* Check it has been named. */
    if (currentModule->name == NULL)
        fatal("No %%Module has been specified for module defined in %s\n",
                previousFile);

    from = currentContext.prevmod;

    if (from != NULL)
    {
        if (from->encoding == no_type)
            from->encoding = currentModule->encoding;

        if (isCallSuperInitUndefined(from))
            if (isCallSuperInitYes(currentModule))
                setCallSuperInitYes(from);
            else
                setCallSuperInitNo(from);
    }

    /* The previous module is now current. */
    currentModule = from;
}


/*
 * Find an existing qualifier.
 */
static qualDef *findQualifier(const char *name)
{
    moduleDef *mod;

    for (mod = currentSpec->modules; mod != NULL; mod = mod->next)
    {
        qualDef *qd;

        for (qd = mod->qualifiers; qd != NULL; qd = qd->next)
            if (strcmp(qd->name, name) == 0)
                return qd;
    }

    /* Qualifiers corresponding to the SIP version are created on the fly. */
    if (name[0] == 'S' && name[1] == 'I' && name[2] == 'P' && name[3] == '_')
    {
        const char *cp = &name[3];
        int major, minor, patch;

        cp = getInt(cp, &major);
        cp = getInt(cp, &minor);
        cp = getInt(cp, &patch);

        if (*cp != '\0')
            yyerror("Unexpected character after SIP version number");

        return allocQualifier(currentModule, -1,
                (major << 16) | (minor << 8) | patch, name, time_qualifier);
    }

    return NULL;
}


/*
 * Get an integer from string.
 */
static const char *getInt(const char *cp, int *ip)
{
    /* Handle the default value. */
    *ip = 0;

    if (*cp == '\0')
        return cp;

    /* There must be a leading underscore. */
    if (*cp++ != '_')
        yyerror("An underscore must separate the parts of a SIP version number");

    while (isdigit(*cp))
    {
        *ip *= 10;
        *ip += *cp - '0';
        ++cp;
    }

    return cp;
}


/*
 * Find an existing API.
 */
apiVersionRangeDef *findAPI(sipSpec *pt, const char *name)
{
    moduleDef *mod;

    for (mod = pt->modules; mod != NULL; mod = mod->next)
    {
        apiVersionRangeDef *avd;

        for (avd = mod->api_versions; avd != NULL; avd = avd->next)
            if (strcmp(avd->api_name->text, name) == 0)
                return avd;
    }

    return NULL;
}


/*
 * Return a copy of a scoped name.
 */
scopedNameDef *copyScopedName(scopedNameDef *snd)
{
    scopedNameDef *head;

    head = NULL;

    while (snd != NULL)
    {
        appendScopedName(&head,text2scopePart(snd -> name));
        snd = snd -> next;
    }

    return head;
}


/*
 * Append a name to a list of scopes.
 */
void appendScopedName(scopedNameDef **headp,scopedNameDef *newsnd)
{
    while (*headp != NULL)
        headp = &(*headp) -> next;

    *headp = newsnd;
}


/*
 * Free a scoped name - but not the text itself.
 */
void freeScopedName(scopedNameDef *snd)
{
    while (snd != NULL)
    {
        scopedNameDef *next = snd -> next;

        free(snd);

        snd = next;
    }
}


/*
 * Convert a text string to a scope part structure.
 */
static scopedNameDef *text2scopePart(char *text)
{
    scopedNameDef *snd;

    snd = sipMalloc(sizeof (scopedNameDef));

    snd->name = text;
    snd->next = NULL;

    return snd;
}


/*
 * Convert a text string to a fully scoped name.
 */
static scopedNameDef *text2scopedName(ifaceFileDef *scope, char *text)
{
    return scopeScopedName(scope, text2scopePart(text));
}


/*
 * Prepend any current scope to a scoped name.
 */
static scopedNameDef *scopeScopedName(ifaceFileDef *scope, scopedNameDef *name)
{
    scopedNameDef *snd;

    snd = (scope != NULL ? copyScopedName(scope->fqcname) : NULL);

    appendScopedName(&snd, name);

    return snd;
}


/*
 * Return a pointer to the tail part of a scoped name.
 */
char *scopedNameTail(scopedNameDef *snd)
{
    if (snd == NULL)
        return NULL;

    while (snd -> next != NULL)
        snd = snd -> next;

    return snd -> name;
}


/*
 * Push the given scope onto the scope stack.
 */
static void pushScope(classDef *scope)
{
    if (currentScopeIdx >= MAX_NESTED_SCOPE)
        fatal("Internal error: increase the value of MAX_NESTED_SCOPE\n");

    scopeStack[currentScopeIdx] = scope;
    sectFlagsStack[currentScopeIdx] = sectionFlags;

    ++currentScopeIdx;
}


/*
 * Pop the scope stack.
 */
static void popScope(void)
{
    if (currentScopeIdx > 0)
        sectionFlags = sectFlagsStack[--currentScopeIdx];
}


/*
 * Return non-zero if the current input should be parsed rather than be
 * skipped.
 */
static int notSkipping()
{
    return (skipStackPtr == 0 ? TRUE : skipStack[skipStackPtr - 1]);
}


/*
 * Return the value of an expression involving a time period.
 */
static int timePeriod(const char *lname, const char *uname)
{
    int this, line;
    qualDef *qd, *lower, *upper;
    moduleDef *mod;

    if (lname == NULL)
        lower = NULL;
    else if ((lower = findQualifier(lname)) == NULL || lower->qtype != time_qualifier)
        yyerror("Lower bound is not a time version");

    if (uname == NULL)
        upper = NULL;
    else if ((upper = findQualifier(uname)) == NULL || upper->qtype != time_qualifier)
        yyerror("Upper bound is not a time version");

    /* Sanity checks on the bounds. */
    if (lower == NULL && upper == NULL)
        yyerror("Lower and upper bounds cannot both be omitted");

    if (lower != NULL && upper != NULL)
    {
        if (lower->module != upper->module || lower->line != upper->line)
            yyerror("Lower and upper bounds are from different timelines");

        if (lower == upper)
            yyerror("Lower and upper bounds must be different");

        if (lower->order > upper->order)
            yyerror("Later version specified as lower bound");
    }

    /* Go through each slot in the relevant timeline. */
    if (lower != NULL)
    {
        mod = lower->module;
        line = lower->line;
    }
    else
    {
        mod = upper->module;
        line = upper->line;
    }

    /* Handle the SIP version number pseudo-timeline. */
    if (line < 0)
    {
        if (lower != NULL && lower->order > SIP_VERSION)
            return FALSE;

        if (upper != NULL && upper->order <= SIP_VERSION)
            return FALSE;

        return TRUE;
    }

    this = FALSE;

    for (qd = mod->qualifiers; qd != NULL; qd = qd->next)
    {
        if (qd->qtype != time_qualifier || qd->line != line)
            continue;

        if (lower != NULL && qd->order < lower->order)
            continue;

        if (upper != NULL && qd->order >= upper->order)
            continue;

        /*
         * This is within the required range so if it is also needed then the
         * expression is true.
         */
        if (selectedQualifier(neededQualifiers, qd))
        {
            this = TRUE;
            break;
        }
    }

    return this;
}


/*
 * Return the value of an expression involving a single platform or feature.
 */
static int platOrFeature(char *name,int optnot)
{
    int this;
    qualDef *qd;

    if ((qd = findQualifier(name)) == NULL || qd -> qtype == time_qualifier)
        yyerror("No such platform or feature");

    /* Assume this sub-expression is false. */

    this = FALSE;

    if (qd -> qtype == feature_qualifier)
    {
        if (!excludedFeature(excludedQualifiers,qd))
            this = TRUE;
    }
    else if (selectedQualifier(neededQualifiers, qd))
        this = TRUE;

    if (optnot)
        this = !this;

    return this;
}


/*
 * Return TRUE if the given qualifier is excluded.
 */
int excludedFeature(stringList *xsl,qualDef *qd)
{
    while (xsl != NULL)
    {
        if (strcmp(qd -> name,xsl -> s) == 0)
            return TRUE;

        xsl = xsl -> next;
    }

    return FALSE;
}


/*
 * Return TRUE if the given qualifier is needed.
 */
int selectedQualifier(stringList *needed_qualifiers, qualDef *qd)
{
    stringList *sl;

    for (sl = needed_qualifiers; sl != NULL; sl = sl -> next)
        if (strcmp(qd -> name,sl -> s) == 0)
            return TRUE;

    return FALSE;
}


/*
 * Return the current scope.  currentScope() is only valid if notSkipping()
 * returns non-zero.
 */
static classDef *currentScope(void)
{
    return (currentScopeIdx > 0 ? scopeStack[currentScopeIdx - 1] : NULL);
}


/*
 * Create a new qualifier.
 */
static void newQualifier(moduleDef *mod, int line, int order, const char *name,
        qualType qt)
{
    /* Check it doesn't already exist. */
    if (findQualifier(name) != NULL)
        yyerror("Version is already defined");

    allocQualifier(mod, line, order, name, qt);
}


/*
 * Allocate a new qualifier.
 */
static qualDef *allocQualifier(moduleDef *mod, int line, int order,
        const char *name, qualType qt)
{
    qualDef *qd;

    qd = sipMalloc(sizeof (qualDef));

    qd->name = name;
    qd->qtype = qt;
    qd->module = mod;
    qd->line = line;
    qd->order = order;
    qd->next = mod->qualifiers;

    mod->qualifiers = qd;

    return qd;
}


/*
 * Create a new imported module.
 */
static void newImport(const char *filename)
{
    moduleDef *from, *mod;
    moduleListDef *mld;

    /* Create a new module if it has not already been defined. */
    for (mod = currentSpec->modules; mod != NULL; mod = mod->next)
        if (strcmp(mod->file, filename) == 0)
            break;

    from = currentModule;

    if (mod == NULL)
    {
        newModule(NULL, filename);
        mod = currentModule;
    }
    else if (from->encoding == no_type)
    {
        /* Import any defaults from the already parsed module. */
        from->encoding = mod->encoding;
    }

    /* Add the new import unless it has already been imported. */
    for (mld = from->imports; mld != NULL; mld = mld->next)
        if (mld->module == mod)
            return;

    mld = sipMalloc(sizeof (moduleListDef));
    mld->module = mod;
    mld->next = from->imports;

    from->imports = mld;
}


/*
 * Set up pointers to hook names.
 */
static void getHooks(optFlags *optflgs,char **pre,char **post)
{
    optFlag *of;

    if ((of = getOptFlag(optflgs,"PreHook",name_flag)) != NULL)
        *pre = of -> fvalue.sval;
    else
        *pre = NULL;

    if ((of = getOptFlag(optflgs,"PostHook",name_flag)) != NULL)
        *post = of -> fvalue.sval;
    else
        *post = NULL;
}


/*
 * Get the /Transfer/ option flag.
 */
static int getTransfer(optFlags *optflgs)
{
    return (getOptFlag(optflgs, "Transfer", bool_flag) != NULL);
}


/*
 * Get the /ReleaseGIL/ option flag.
 */
static int getReleaseGIL(optFlags *optflgs)
{
    return (getOptFlag(optflgs, "ReleaseGIL", bool_flag) != NULL);
}


/*
 * Get the /HoldGIL/ option flag.
 */
static int getHoldGIL(optFlags *optflgs)
{
    return (getOptFlag(optflgs, "HoldGIL", bool_flag) != NULL);
}


/*
 * Get the /Deprecated/ option flag.
 */
static int getDeprecated(optFlags *optflgs)
{
    return (getOptFlag(optflgs, "Deprecated", bool_flag) != NULL);
}


/*
 * Get the /AllowNone/ option flag.
 */
static int getAllowNone(optFlags *optflgs)
{
    return (getOptFlag(optflgs, "AllowNone", bool_flag) != NULL);
}


/*
 * Get the /VirtualErrorHandler/ option flag.
 */
static const char *getVirtErrorHandler(optFlags *optflgs)
{
    optFlag *of = getOptFlag(optflgs, "VirtualErrorHandler", name_flag);

    if (of == NULL)
        return NULL;

    return of->fvalue.sval;
}


/*
 * Get the /DocType/ option flag.
 */
static const char *getDocType(optFlags *optflgs)
{
    optFlag *of = getOptFlag(optflgs, "DocType", string_flag);

    if (of == NULL)
        return NULL;

    return of->fvalue.sval;
}


/*
 * Get the /DocValue/ option flag.
 */
static const char *getDocValue(optFlags *optflgs)
{
    optFlag *of = getOptFlag(optflgs, "DocValue", string_flag);

    if (of == NULL)
        return NULL;

    return of->fvalue.sval;
}


/*
 * Return TRUE if the PyQt3 plugin was specified.
 */
int pluginPyQt3(sipSpec *pt)
{
    return stringFind(pt->plugins, "PyQt3");
}


/*
 * Return TRUE if the PyQt4 plugin was specified.
 */
int pluginPyQt4(sipSpec *pt)
{
    return stringFind(pt->plugins, "PyQt4");
}


/*
 * Return TRUE if the PyQt5 plugin was specified.
 */
int pluginPyQt5(sipSpec *pt)
{
    return stringFind(pt->plugins, "PyQt5");
}


/*
 * Return TRUE if a list of strings contains a given entry.
 */
static int stringFind(stringList *sl, const char *s)
{
    while (sl != NULL)
    {
        if (strcmp(sl->s, s) == 0)
            return TRUE;

        sl = sl->next;
    }

    return FALSE;
}


/*
 * Set the name of a module.
 */
static void setModuleName(sipSpec *pt, moduleDef *mod, const char *fullname)
{
    mod->fullname = cacheName(pt, fullname);

    if (inMainModule())
        setIsUsedName(mod->fullname);

    if ((mod->name = strrchr(fullname, '.')) != NULL)
        mod->name++;
    else
        mod->name = fullname;
}


/*
 * Define a new class and set its name.
 */
static void defineClass(scopedNameDef *snd, classList *supers, optFlags *of)
{
    classDef *cd, *c_scope = currentScope();

    cd = newClass(currentSpec, class_iface, getAPIRange(of),
            scopeScopedName((c_scope != NULL ? c_scope->iff : NULL), snd),
            getVirtErrorHandler(of));
    cd->supers = supers;

    pushScope(cd);
}


/*
 * Complete the definition of a class.
 */
static classDef *completeClass(scopedNameDef *snd, optFlags *of, int has_def)
{
    classDef *cd = currentScope();

    /* See if the class was defined or just declared. */
    if (has_def)
    {
        if (snd->next != NULL)
            yyerror("A scoped name cannot be given in a class/struct definition");

    }
    else if (cd->supers != NULL)
        yyerror("Class/struct has super-classes but no definition");
    else
        setIsOpaque(cd);

    finishClass(currentSpec, currentModule, cd, of);
    popScope();

    /*
     * Check that external classes have only been declared at the global scope.
     */
    if (isExternal(cd) && currentScope() != NULL)
        yyerror("External classes/structs can only be declared in the global scope");

    return cd;
}


/*
 * Add a variable to the list so that the list remains sorted.
 */
static void addVariable(sipSpec *pt, varDef *vd)
{
    varDef **at = &pt->vars;

    while (*at != NULL)
    {
        if (strcmp(vd->pyname->text, (*at)->pyname->text) < 0)
            break;

        at = &(*at)->next;
    }

    vd->next = *at;
    *at = vd;
}


/*
 * Update a type according to optional flags.
 */
static void applyTypeFlags(moduleDef *mod, argDef *ad, optFlags *flags)
{
    ad->doctype = getDocType(flags);

    if (getOptFlag(flags, "PyInt", bool_flag) != NULL)
    {
        if (ad->atype == string_type)
            ad->atype = byte_type;
        else if (ad->atype == sstring_type)
            ad->atype = sbyte_type;
        else if (ad->atype == ustring_type)
            ad->atype = ubyte_type;
    }

    if (ad->atype == string_type && !isArray(ad) && !isReference(ad))
    {
        optFlag *of;

        if ((of = getOptFlag(flags, "Encoding", string_flag)) == NULL)
        {
            if (mod->encoding != no_type)
                ad->atype = mod->encoding;
            else
                ad->atype = string_type;
        }
        else if ((ad->atype = convertEncoding(of->fvalue.sval)) == no_type)
            yyerror("The value of the /Encoding/ annotation must be one of \"ASCII\", \"Latin-1\", \"UTF-8\" or \"None\"");
    }
}


/*
 * Return the keyword argument support converted from a string.
 */
static KwArgs convertKwArgs(const char *kwargs)
{
    if (strcmp(kwargs, "None") == 0)
        return NoKwArgs;

    if (strcmp(kwargs, "All") == 0)
        return AllKwArgs;

    if (strcmp(kwargs, "Optional") == 0)
        return OptionalKwArgs;

    yyerror("The style of keyword argument support must be one of \"All\", \"Optional\" or \"None\"");
}


/*
 * Return the Format for a string.
 */
static Format convertFormat(const char *format)
{
    if (strcmp(format, "raw") == 0)
        return raw;

    if (strcmp(format, "deindented") == 0)
        return deindented;

    yyerror("The docstring format must be either \"raw\" or \"deindented\"");
}


/*
 * Return the argument type for a string with the given encoding or no_type if
 * the encoding was invalid.
 */
static argType convertEncoding(const char *encoding)
{
    if (strcmp(encoding, "ASCII") == 0)
        return ascii_string_type;

    if (strcmp(encoding, "Latin-1") == 0)
        return latin1_string_type;

    if (strcmp(encoding, "UTF-8") == 0)
        return utf8_string_type;

    if (strcmp(encoding, "None") == 0)
        return string_type;

    return no_type;
}


/*
 * Get the /API/ option flag.
 */
static apiVersionRangeDef *getAPIRange(optFlags *optflgs)
{
    optFlag *of;

    if ((of = getOptFlag(optflgs, "API", api_range_flag)) == NULL)
        return NULL;

    return of->fvalue.aval;
}


/*
 * Return the API range structure and version number corresponding to the
 * given API range.
 */
static apiVersionRangeDef *convertAPIRange(moduleDef *mod, nameDef *name,
        int from, int to)
{
    int index;
    apiVersionRangeDef *avd, **avdp;

    /* Handle the trivial case. */
    if (from == 0 && to == 0)
        return NULL;

    for (index = 0, avdp = &mod->api_ranges; (*avdp) != NULL; avdp = &(*avdp)->next, ++index)
    {
        avd = *avdp;

        if (avd->api_name == name && avd->from == from && avd->to == to)
            return avd;
    }

    /* The new one must be appended so that version numbers remain valid. */
    avd = sipMalloc(sizeof (apiVersionRangeDef));

    avd->api_name = name;
    avd->from = from;
    avd->to = to;
    avd->index = index;

    avd->next = NULL;
    *avdp = avd;

    return avd;
}


/*
 * Return the style of keyword argument support for a signature.
 */
static KwArgs keywordArgs(moduleDef *mod, optFlags *optflgs, signatureDef *sd,
        int need_name)
{
    KwArgs kwargs;
    optFlag *ka_anno, *no_ka_anno;

    /* Get the default. */
    kwargs = mod->kwargs;

    /*
     * Get the possible annotations allowing /KeywordArgs/ to have different
     * types of values.
     */
    ka_anno = findOptFlag(optflgs, "KeywordArgs");
    no_ka_anno = getOptFlag(optflgs, "NoKeywordArgs", bool_flag);

    if (no_ka_anno != NULL)
    {
        if (ka_anno != NULL)
            yyerror("/KeywordArgs/ and /NoKeywordArgs/ cannot both be specified");

        deprecated("/NoKeywordArgs/ is deprecated, use /KeywordArgs=\"None\" instead");

        kwargs = NoKwArgs;
    }
    else if (ka_anno != NULL)
    {
        /* A string value is the non-deprecated type. */
        if (ka_anno->ftype == string_flag)
        {
            kwargs = convertKwArgs(ka_anno->fvalue.sval);
        }
        else
        {
            deprecated("/KeywordArgs/ is deprecated, use /KeywordArgs=\"All\" instead");

            /* Get it again to check the type. */
            ka_anno = getOptFlag(optflgs, "KeywordArgs", bool_flag);
        }
    }

    /* An ellipsis cannot be used with keyword arguments. */
    if (sd->nrArgs > 0 && sd->args[sd->nrArgs - 1].atype == ellipsis_type)
        kwargs = NoKwArgs;

    if (kwargs != NoKwArgs)
    {
        int a, is_name = FALSE;

        /*
         * Mark argument names as being used and check there is at least one.
         */
        for (a = 0; a < sd->nrArgs; ++a)
        {
            argDef *ad = &sd->args[a];

            if (kwargs == OptionalKwArgs && ad->defval == NULL)
                continue;

            if (ad->name != NULL)
            {
                if (need_name || inMainModule())
                    setIsUsedName(ad->name);

                is_name = TRUE;
            }
        }

        if (!is_name)
            kwargs = NoKwArgs;
    }

    return kwargs;
}


/*
 * Extract the version of a string value optionally associated with a
 * particular feature.
 */
static char *convertFeaturedString(char *fs)
{
    while (fs != NULL)
    {
        char *next, *value;

        /* Individual values are ';' separated. */
        if ((next = strchr(fs, ';')) != NULL)
            *next++ = '\0';

        /* Features and values are ':' separated. */
        if ((value = strchr(fs, ':')) == NULL)
        {
            /* This is an unconditional value so just return it. */
            return strip(fs);
        }

        *value++ = '\0';

        if (isEnabledFeature(strip(fs)))
            return strip(value);

        fs = next;
    }

    /* No value was enabled. */
    return NULL;
}


/*
 * Return the stripped version of a string.
 */
static char *strip(char *s)
{
    while (*s == ' ')
        ++s;

    if (*s != '\0')
    {
        char *cp = &s[strlen(s) - 1];

        while (*cp == ' ')
            *cp-- = '\0';
    }

    return s;
}


/*
 * Return TRUE if the given feature is enabled.
 */
static int isEnabledFeature(const char *name)
{
    qualDef *qd;

    if ((qd = findQualifier(name)) == NULL || qd->qtype != feature_qualifier)
        yyerror("No such feature");

    return !excludedFeature(excludedQualifiers, qd);
}


/*
 * Add a property definition to a class.
 */
static void addProperty(sipSpec *pt, moduleDef *mod, classDef *cd,
        const char *name, const char *get, const char *set,
        codeBlock *docstring)
{
    propertyDef *pd;

    checkAttributes(pt, mod, cd, NULL, name, FALSE);

    pd = sipMalloc(sizeof (propertyDef));

    pd->name = cacheName(pt, name);
    pd->get = get;
    pd->set = set;
    appendCodeBlock(&pd->docstring, docstring);
    pd->next = cd->properties;

    cd->properties = pd;

    if (inMainModule())
        setIsUsedName(pd->name);
}


/*
 * Configure a module and return the (possibly new) current module.
 */
static moduleDef *configureModule(sipSpec *pt, moduleDef *module,
        const char *filename, const char *name, int version, int c_module,
        KwArgs kwargs, int use_arg_names, int call_super_init,
        int all_raise_py_exc, const char *def_error_handler,
        codeBlock *docstring)
{
    moduleDef *mod;

    /* Check the module hasn't already been defined. */
    for (mod = pt->modules; mod != NULL; mod = mod->next)
        if (mod->fullname != NULL && strcmp(mod->fullname->text, name) == 0)
            yyerror("Module is already defined");

    /*
     * If we are in a container module then create a component module and make
     * it current.
     */
    if (isContainer(module) || module->container != NULL)
    {
        mod = allocModule();

        mod->file = filename;
        mod->container = (isContainer(module) ? module : module->container);

        module = mod;
    }

    setModuleName(pt, module, name);
    module->kwargs = kwargs;
    module->virt_error_handler = def_error_handler;
    module->version = version;
    appendCodeBlock(&module->docstring, docstring);

    if (all_raise_py_exc)
        setAllRaisePyException(module);

    if (use_arg_names)
        setUseArgNames(module);

    if (call_super_init == 0)
        setCallSuperInitNo(module);
    else if (call_super_init > 0)
        setCallSuperInitYes(module);

    if (pt->genc < 0)
        pt->genc = c_module;
    else if (pt->genc != c_module)
        yyerror("Cannot mix C and C++ modules");

    return module;
}


/*
 * Add a Python naming rule to a module.
 */
static void addAutoPyName(moduleDef *mod, const char *remove_leading)
{
    autoPyNameDef *apnd, **apndp;

    for (apndp = &mod->autopyname; *apndp != NULL; apndp = &(*apndp)->next)
        ;

    apnd = sipMalloc(sizeof (autoPyNameDef));
    apnd->remove_leading = remove_leading;
    apnd->next = *apndp;

    *apndp = apnd;
}


/*
 * Check that no invalid or unknown annotations are given.
 */
static void checkAnnos(optFlags *annos, const char *valid[])
{
    if (parsingCSignature && annos->nrFlags != 0)
    {
        deprecated("Annotations should not be used in explicit C/C++ signatures");
    }
    else
    {
        int i;

        for (i = 0; i < annos->nrFlags; i++)
        {
            const char **name;

            for (name = valid; *name != NULL; ++name)
                if (strcmp(*name, annos->flags[i].fname) == 0)
                    break;

            if (*name == NULL)
                deprecated("Annotation is invalid");
        }
    }
}


/*
 * Check that no annotations were given.
 */
static void checkNoAnnos(optFlags *annos, const char *msg)
{
    if (annos->nrFlags != 0)
        deprecated(msg);
}


/*
 * Handle any /KeepReference/ annotation for a type.
 */
static void handleKeepReference(optFlags *optflgs, argDef *ad, moduleDef *mod)
{
    optFlag *of;

    if ((of = getOptFlag(optflgs, "KeepReference", opt_integer_flag)) != NULL)
    {
        setKeepReference(ad);

        if ((ad->key = of->fvalue.ival) < -1)
            yyerror("/KeepReference/ key cannot be negative");

        /* If there was no explicit key then auto-allocate one. */
        if (ad->key == -1)
            ad->key = mod->next_key--;
    }
}


/*
 * Configure the mapped type annotations that are also valid with mapped type
 * templates.
 */
static void mappedTypeAnnos(mappedTypeDef *mtd, optFlags *optflgs)
{
    if (getOptFlag(optflgs, "NoRelease", bool_flag) != NULL)
        setNoRelease(mtd);

    if (getAllowNone(optflgs))
        setHandlesNone(mtd);

    mtd->doctype = getDocType(optflgs);
}


/*
 * Initialise an argument with the derefences of another, plus a new one.
 */
static void add_new_deref(argDef *new, argDef *orig, int isconst)
{
    if ((new->nrderefs = orig->nrderefs + 1) >= MAX_NR_DEREFS)
        yyerror("Internal error - increase the value of MAX_NR_DEREFS");

    memcpy(&new->derefs[0], &orig->derefs[0], sizeof (new->derefs));
    new->derefs[orig->nrderefs] = isconst;
}


/*
 * Add the dereferences from one type to another.
 */
static void add_derefs(argDef *dst, argDef *src)
{
    int i;

    for (i = 0; i < src->nrderefs; ++i)
    {
        if (dst->nrderefs >= MAX_NR_DEREFS - 1)
            fatal("Internal error - increase the value of MAX_NR_DEREFS\n");

        dst->derefs[dst->nrderefs++] = src->derefs[i];
    }
}

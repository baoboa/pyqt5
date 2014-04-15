
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     TK_API = 258,
     TK_AUTOPYNAME = 259,
     TK_DEFDOCSTRING = 260,
     TK_DEFENCODING = 261,
     TK_PLUGIN = 262,
     TK_VIRTERRORHANDLER = 263,
     TK_DOCSTRING = 264,
     TK_DOC = 265,
     TK_EXPORTEDDOC = 266,
     TK_EXTRACT = 267,
     TK_MAKEFILE = 268,
     TK_ACCESSCODE = 269,
     TK_GETCODE = 270,
     TK_SETCODE = 271,
     TK_PREINITCODE = 272,
     TK_INITCODE = 273,
     TK_POSTINITCODE = 274,
     TK_FINALCODE = 275,
     TK_UNITCODE = 276,
     TK_UNITPOSTINCLUDECODE = 277,
     TK_MODCODE = 278,
     TK_TYPECODE = 279,
     TK_PREPYCODE = 280,
     TK_COPYING = 281,
     TK_MAPPEDTYPE = 282,
     TK_CODELINE = 283,
     TK_IF = 284,
     TK_END = 285,
     TK_NAME_VALUE = 286,
     TK_PATH_VALUE = 287,
     TK_STRING_VALUE = 288,
     TK_VIRTUALCATCHERCODE = 289,
     TK_TRAVERSECODE = 290,
     TK_CLEARCODE = 291,
     TK_GETBUFFERCODE = 292,
     TK_RELEASEBUFFERCODE = 293,
     TK_READBUFFERCODE = 294,
     TK_WRITEBUFFERCODE = 295,
     TK_SEGCOUNTCODE = 296,
     TK_CHARBUFFERCODE = 297,
     TK_PICKLECODE = 298,
     TK_METHODCODE = 299,
     TK_INSTANCECODE = 300,
     TK_FROMTYPE = 301,
     TK_TOTYPE = 302,
     TK_TOSUBCLASS = 303,
     TK_INCLUDE = 304,
     TK_OPTINCLUDE = 305,
     TK_IMPORT = 306,
     TK_EXPHEADERCODE = 307,
     TK_MODHEADERCODE = 308,
     TK_TYPEHEADERCODE = 309,
     TK_MODULE = 310,
     TK_CMODULE = 311,
     TK_CONSMODULE = 312,
     TK_COMPOMODULE = 313,
     TK_CLASS = 314,
     TK_STRUCT = 315,
     TK_PUBLIC = 316,
     TK_PROTECTED = 317,
     TK_PRIVATE = 318,
     TK_SIGNALS = 319,
     TK_SIGNAL_METHOD = 320,
     TK_SLOTS = 321,
     TK_SLOT_METHOD = 322,
     TK_BOOL = 323,
     TK_SHORT = 324,
     TK_INT = 325,
     TK_LONG = 326,
     TK_FLOAT = 327,
     TK_DOUBLE = 328,
     TK_CHAR = 329,
     TK_WCHAR_T = 330,
     TK_VOID = 331,
     TK_PYOBJECT = 332,
     TK_PYTUPLE = 333,
     TK_PYLIST = 334,
     TK_PYDICT = 335,
     TK_PYCALLABLE = 336,
     TK_PYSLICE = 337,
     TK_PYTYPE = 338,
     TK_PYBUFFER = 339,
     TK_VIRTUAL = 340,
     TK_ENUM = 341,
     TK_SIGNED = 342,
     TK_UNSIGNED = 343,
     TK_SCOPE = 344,
     TK_LOGICAL_OR = 345,
     TK_CONST = 346,
     TK_STATIC = 347,
     TK_SIPSIGNAL = 348,
     TK_SIPSLOT = 349,
     TK_SIPANYSLOT = 350,
     TK_SIPRXCON = 351,
     TK_SIPRXDIS = 352,
     TK_SIPSLOTCON = 353,
     TK_SIPSLOTDIS = 354,
     TK_SIPSSIZET = 355,
     TK_NUMBER_VALUE = 356,
     TK_REAL_VALUE = 357,
     TK_TYPEDEF = 358,
     TK_NAMESPACE = 359,
     TK_TIMELINE = 360,
     TK_PLATFORMS = 361,
     TK_FEATURE = 362,
     TK_LICENSE = 363,
     TK_QCHAR_VALUE = 364,
     TK_TRUE_VALUE = 365,
     TK_FALSE_VALUE = 366,
     TK_NULL_VALUE = 367,
     TK_OPERATOR = 368,
     TK_THROW = 369,
     TK_QOBJECT = 370,
     TK_EXCEPTION = 371,
     TK_RAISECODE = 372,
     TK_VIRTERRORCODE = 373,
     TK_EXPLICIT = 374,
     TK_TEMPLATE = 375,
     TK_ELLIPSIS = 376,
     TK_DEFMETATYPE = 377,
     TK_DEFSUPERTYPE = 378,
     TK_PROPERTY = 379,
     TK_FORMAT = 380,
     TK_GET = 381,
     TK_ID = 382,
     TK_KWARGS = 383,
     TK_LANGUAGE = 384,
     TK_LICENSEE = 385,
     TK_NAME = 386,
     TK_OPTIONAL = 387,
     TK_ORDER = 388,
     TK_REMOVELEADING = 389,
     TK_SET = 390,
     TK_SIGNATURE = 391,
     TK_TIMESTAMP = 392,
     TK_TYPE = 393,
     TK_USEARGNAMES = 394,
     TK_ALLRAISEPYEXC = 395,
     TK_CALLSUPERINIT = 396,
     TK_DEFERRORHANDLER = 397,
     TK_VERSION = 398
   };
#endif
/* Tokens.  */
#define TK_API 258
#define TK_AUTOPYNAME 259
#define TK_DEFDOCSTRING 260
#define TK_DEFENCODING 261
#define TK_PLUGIN 262
#define TK_VIRTERRORHANDLER 263
#define TK_DOCSTRING 264
#define TK_DOC 265
#define TK_EXPORTEDDOC 266
#define TK_EXTRACT 267
#define TK_MAKEFILE 268
#define TK_ACCESSCODE 269
#define TK_GETCODE 270
#define TK_SETCODE 271
#define TK_PREINITCODE 272
#define TK_INITCODE 273
#define TK_POSTINITCODE 274
#define TK_FINALCODE 275
#define TK_UNITCODE 276
#define TK_UNITPOSTINCLUDECODE 277
#define TK_MODCODE 278
#define TK_TYPECODE 279
#define TK_PREPYCODE 280
#define TK_COPYING 281
#define TK_MAPPEDTYPE 282
#define TK_CODELINE 283
#define TK_IF 284
#define TK_END 285
#define TK_NAME_VALUE 286
#define TK_PATH_VALUE 287
#define TK_STRING_VALUE 288
#define TK_VIRTUALCATCHERCODE 289
#define TK_TRAVERSECODE 290
#define TK_CLEARCODE 291
#define TK_GETBUFFERCODE 292
#define TK_RELEASEBUFFERCODE 293
#define TK_READBUFFERCODE 294
#define TK_WRITEBUFFERCODE 295
#define TK_SEGCOUNTCODE 296
#define TK_CHARBUFFERCODE 297
#define TK_PICKLECODE 298
#define TK_METHODCODE 299
#define TK_INSTANCECODE 300
#define TK_FROMTYPE 301
#define TK_TOTYPE 302
#define TK_TOSUBCLASS 303
#define TK_INCLUDE 304
#define TK_OPTINCLUDE 305
#define TK_IMPORT 306
#define TK_EXPHEADERCODE 307
#define TK_MODHEADERCODE 308
#define TK_TYPEHEADERCODE 309
#define TK_MODULE 310
#define TK_CMODULE 311
#define TK_CONSMODULE 312
#define TK_COMPOMODULE 313
#define TK_CLASS 314
#define TK_STRUCT 315
#define TK_PUBLIC 316
#define TK_PROTECTED 317
#define TK_PRIVATE 318
#define TK_SIGNALS 319
#define TK_SIGNAL_METHOD 320
#define TK_SLOTS 321
#define TK_SLOT_METHOD 322
#define TK_BOOL 323
#define TK_SHORT 324
#define TK_INT 325
#define TK_LONG 326
#define TK_FLOAT 327
#define TK_DOUBLE 328
#define TK_CHAR 329
#define TK_WCHAR_T 330
#define TK_VOID 331
#define TK_PYOBJECT 332
#define TK_PYTUPLE 333
#define TK_PYLIST 334
#define TK_PYDICT 335
#define TK_PYCALLABLE 336
#define TK_PYSLICE 337
#define TK_PYTYPE 338
#define TK_PYBUFFER 339
#define TK_VIRTUAL 340
#define TK_ENUM 341
#define TK_SIGNED 342
#define TK_UNSIGNED 343
#define TK_SCOPE 344
#define TK_LOGICAL_OR 345
#define TK_CONST 346
#define TK_STATIC 347
#define TK_SIPSIGNAL 348
#define TK_SIPSLOT 349
#define TK_SIPANYSLOT 350
#define TK_SIPRXCON 351
#define TK_SIPRXDIS 352
#define TK_SIPSLOTCON 353
#define TK_SIPSLOTDIS 354
#define TK_SIPSSIZET 355
#define TK_NUMBER_VALUE 356
#define TK_REAL_VALUE 357
#define TK_TYPEDEF 358
#define TK_NAMESPACE 359
#define TK_TIMELINE 360
#define TK_PLATFORMS 361
#define TK_FEATURE 362
#define TK_LICENSE 363
#define TK_QCHAR_VALUE 364
#define TK_TRUE_VALUE 365
#define TK_FALSE_VALUE 366
#define TK_NULL_VALUE 367
#define TK_OPERATOR 368
#define TK_THROW 369
#define TK_QOBJECT 370
#define TK_EXCEPTION 371
#define TK_RAISECODE 372
#define TK_VIRTERRORCODE 373
#define TK_EXPLICIT 374
#define TK_TEMPLATE 375
#define TK_ELLIPSIS 376
#define TK_DEFMETATYPE 377
#define TK_DEFSUPERTYPE 378
#define TK_PROPERTY 379
#define TK_FORMAT 380
#define TK_GET 381
#define TK_ID 382
#define TK_KWARGS 383
#define TK_LANGUAGE 384
#define TK_LICENSEE 385
#define TK_NAME 386
#define TK_OPTIONAL 387
#define TK_ORDER 388
#define TK_REMOVELEADING 389
#define TK_SET 390
#define TK_SIGNATURE 391
#define TK_TIMESTAMP 392
#define TK_TYPE 393
#define TK_USEARGNAMES 394
#define TK_ALLRAISEPYEXC 395
#define TK_CALLSUPERINIT 396
#define TK_DEFERRORHANDLER 397
#define TK_VERSION 398




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1676 of yacc.c  */
#line 179 "/home/bob/hg/sip/sip-4.15.3-snapshot-5dc8c370157e/sipgen/parser.y"

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



/* Line 1676 of yacc.c  */
#line 382 "/home/bob/hg/sip/sip-4.15.3-snapshot-5dc8c370157e/sipgen/parser.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;



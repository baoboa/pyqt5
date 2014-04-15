/*
 * The code generator module for SIP.
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


#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "sip.h"


/* Return the base (ie. C/C++) name of a super-type or meta-type. */
#define smtypeName(sm)          (strrchr((sm)->name->text, '.') + 1)

/* Return TRUE if a wrapped variable can be set. */
#define canSetVariable(vd)      ((vd)->type.nrderefs != 0 || !isConstArg(&(vd)->type))


/* Control what generateCalledArgs() actually generates. */
typedef enum {
    Declaration,
    Definition
} funcArgType;


/* An entry in the sorted array of methods. */
typedef struct {
    memberDef *md;                      /* The method. */
} sortedMethTab;


static int currentLineNr;               /* Current output line number. */
static const char *currentFileName;     /* Current output file name. */
static int previousLineNr;              /* Previous output line number. */
static const char *previousFileName;    /* Previous output file name. */
static int exceptions;                  /* Set if exceptions are enabled. */
static int tracing;                     /* Set if tracing is enabled. */
static int generating_c;                /* Set if generating C. */
static int release_gil;                 /* Set if always releasing the GIL. */
static const char *prcode_last = NULL;  /* The last prcode format string. */
static int prcode_xml = FALSE;          /* Set if prcode is XML aware. */
static int docstrings;                  /* Set if generating docstrings. */


static void generateDocumentation(sipSpec *pt, const char *docFile);
static void generateBuildFile(sipSpec *pt, const char *buildFile,
        const char *srcSuffix, const char *consModule);
static void generateBuildFileSources(sipSpec *pt, moduleDef *mod,
        const char *srcSuffix, FILE *fp);
static void generateInternalAPIHeader(sipSpec *pt, moduleDef *mod,
        const char *codeDir, stringList *needed_qualifiers, stringList *xsl,
        int timestamp);
static void generateCpp(sipSpec *pt, moduleDef *mod, const char *codeDir,
        const char *srcSuffix, int parts, stringList *needed_qualifiers,
        stringList *xsl, int timestamp);
static void generateCompositeCpp(sipSpec *pt, const char *codeDir,
        int timestamp);
static void generateConsolidatedCpp(sipSpec *pt, const char *codeDir,
        const char *srcSuffix, int timestamp);
static void generateComponentCpp(sipSpec *pt, const char *codeDir,
        const char *consModule, int timestamp);
static void generateSipImport(moduleDef *mod, FILE *fp);
static void generateSipImportVariables(FILE *fp);
static void generateModInitStart(moduleDef *mod, int gen_c, FILE *fp);
static void generateModDefinition(moduleDef *mod, const char *methods,
        FILE *fp);
static void generateModDocstring(moduleDef *mod, FILE *fp);
static void generateIfaceCpp(sipSpec *, ifaceFileDef *, int, const char *,
        const char *, FILE *, int);
static void generateMappedTypeCpp(mappedTypeDef *mtd, sipSpec *pt, FILE *fp);
static void generateImportedMappedTypeAPI(mappedTypeDef *mtd, sipSpec *pt,
        moduleDef *mod, FILE *fp);
static void generateMappedTypeAPI(sipSpec *pt, mappedTypeDef *mtd, FILE *fp);
static void generateClassCpp(classDef *cd, sipSpec *pt, FILE *fp);
static void generateImportedClassAPI(classDef *cd, sipSpec *pt, moduleDef *mod,
        FILE *fp);
static void generateClassAPI(classDef *cd, sipSpec *pt, FILE *fp);
static void generateClassFunctions(sipSpec *pt, moduleDef *mod, classDef *cd,
        FILE *fp);
static void generateShadowCode(sipSpec *pt, moduleDef *mod, classDef *cd,
        FILE *fp);
static void generateFunction(sipSpec *, memberDef *, overDef *, classDef *,
        classDef *, moduleDef *, FILE *);
static void generateFunctionBody(overDef *, classDef *, mappedTypeDef *,
        classDef *, int deref, moduleDef *, FILE *);
static void generateTypeDefinition(sipSpec *pt, classDef *cd, FILE *fp);
static void generateTypeInit(classDef *, moduleDef *, FILE *);
static void generateCppCodeBlock(codeBlockList *cbl, FILE *fp);
static void generateUsedIncludes(ifaceFileList *iffl, FILE *fp);
static void generateModuleAPI(sipSpec *pt, moduleDef *mod, FILE *fp);
static void generateImportedModuleAPI(sipSpec *pt, moduleDef *mod,
        moduleDef *immod, FILE *fp);
static void generateShadowClassDeclaration(sipSpec *, classDef *, FILE *);
static int hasConvertToCode(argDef *ad);
static void deleteOuts(moduleDef *mod, signatureDef *sd, FILE *fp);
static void deleteTemps(moduleDef *mod, signatureDef *sd, FILE *fp);
static void gc_ellipsis(signatureDef *sd, FILE *fp);
static void generateCallArgs(moduleDef *, signatureDef *, signatureDef *,
        FILE *);
static void generateCalledArgs(moduleDef *, ifaceFileDef *, signatureDef *,
        funcArgType, int, FILE *);
static void generateVariable(moduleDef *, ifaceFileDef *, argDef *, int,
        FILE *);
static void generateNamedValueType(ifaceFileDef *, argDef *, char *, FILE *);
static void generateBaseType(ifaceFileDef *, argDef *, int, FILE *);
static void generateNamedBaseType(ifaceFileDef *, argDef *, const char *, int,
        FILE *);
static void generateTupleBuilder(moduleDef *, signatureDef *, FILE *);
static void generateEmitters(moduleDef *mod, classDef *cd, FILE *fp);
static void generateEmitter(moduleDef *, classDef *, visibleList *, FILE *);
static void generateVirtualHandler(moduleDef *mod, virtHandlerDef *vhd,
        FILE *fp);
static void generateDefaultInstanceReturn(argDef *res, const char *indent,
        FILE *fp);
static void generateVirtualCatcher(sipSpec *pt, moduleDef *mod, classDef *cd,
        int virtNr, virtOverDef *vod, FILE *fp);
static void generateVirtHandlerCall(sipSpec *pt, moduleDef *mod, classDef *cd,
        virtOverDef *vod, argDef *res, const char *indent, FILE *fp);
static void generateProtectedEnums(sipSpec *, classDef *, FILE *);
static void generateProtectedDeclarations(classDef *, FILE *);
static void generateProtectedDefinitions(moduleDef *, classDef *, FILE *);
static void generateProtectedCallArgs(moduleDef *mod, signatureDef *sd,
        FILE *fp);
static void generateConstructorCall(classDef *, ctorDef *, int, int,
        moduleDef *, FILE *);
static void generateHandleResult(moduleDef *, overDef *, int, int, char *,
        FILE *);
static void generateOrdinaryFunction(sipSpec *pt, moduleDef *mod,
        classDef *c_scope, mappedTypeDef *mt_scope, memberDef *md, FILE *fp);
static void generateSimpleFunctionCall(fcallDef *, FILE *);
static void generateFunctionCall(classDef *c_scope, mappedTypeDef *mt_scope,
        ifaceFileDef *o_scope, overDef *od, int deref, moduleDef *mod,
        FILE *fp);
static void generateCppFunctionCall(moduleDef *mod, ifaceFileDef *scope,
        ifaceFileDef *o_scope, overDef *od, FILE *fp);
static void generateSlotArg(moduleDef *mod, signatureDef *sd, int argnr,
        FILE *fp);
static void generateComparisonSlotCall(moduleDef *mod, ifaceFileDef *scope,
        overDef *od, const char *op, const char *cop, int deref, FILE *fp);
static void generateBinarySlotCall(moduleDef *mod, ifaceFileDef *scope,
        overDef *od, const char *op, int deref, FILE *fp);
static void generateNumberSlotCall(moduleDef *mod, overDef *od, char *op,
        FILE *fp);
static void generateVariableGetter(ifaceFileDef *, varDef *, FILE *);
static void generateVariableSetter(ifaceFileDef *, varDef *, FILE *);
static int generateObjToCppConversion(argDef *, FILE *);
static void generateVarMember(varDef *vd, FILE *fp);
static int generateVoidPointers(sipSpec *pt, moduleDef *mod, classDef *cd,
        FILE *fp);
static int generateChars(sipSpec *pt, moduleDef *mod, classDef *cd, FILE *fp);
static int generateStrings(sipSpec *pt, moduleDef *mod, classDef *cd, FILE *fp);
static sortedMethTab *createFunctionTable(memberDef *, int *);
static sortedMethTab *createMethodTable(classDef *, int *);
static int generateMappedTypeMethodTable(sipSpec *pt, mappedTypeDef *mtd,
        FILE *fp);
static int generateClassMethodTable(sipSpec *pt, classDef *cd, FILE *fp);
static void prMethodTable(sipSpec *pt, sortedMethTab *mtable, int nr,
        ifaceFileDef *iff, overDef *overs, FILE *fp);
static void generateEnumMacros(sipSpec *pt, moduleDef *mod, classDef *cd,
        mappedTypeDef *mtd, FILE *fp);
static int generateEnumMemberTable(sipSpec *pt, moduleDef *mod, classDef *cd,
        mappedTypeDef *mtd, FILE *fp);
static int generateInts(sipSpec *pt, moduleDef *mod, classDef *cd, FILE *fp);
static int generateLongs(sipSpec *pt, moduleDef *mod, classDef *cd, FILE *fp);
static int generateUnsignedLongs(sipSpec *pt, moduleDef *mod, classDef *cd,
        FILE *fp);
static int generateLongLongs(sipSpec *pt, moduleDef *mod, classDef *cd,
        FILE *fp);
static int generateUnsignedLongLongs(sipSpec *pt, moduleDef *mod, classDef *cd,
        FILE *fp);
static int generateVariableType(sipSpec *pt, moduleDef *mod, classDef *cd,
        argType atype, const char *eng, const char *s1, const char *s2,
        FILE *fp);
static int generateDoubles(sipSpec *pt, moduleDef *mod, classDef *cd, FILE *fp);
static int generateClasses(sipSpec *pt, moduleDef *mod, classDef *cd, FILE *fp);
static void generateTypesInline(sipSpec *pt, moduleDef *mod, FILE *fp);
static void generateAccessFunctions(sipSpec *pt, moduleDef *mod, classDef *cd,
        FILE *fp);
static void generateConvertToDefinitions(mappedTypeDef *, classDef *, FILE *);
static void generateEncodedType(moduleDef *mod, classDef *cd, int last,
        FILE *fp);
static int generateArgParser(moduleDef *mod, signatureDef *sd,
        classDef *c_scope, mappedTypeDef *mt_scope, ctorDef *ct, overDef *od,
        int secCall, FILE *fp);
static void generateTry(throwArgs *, FILE *);
static void generateCatch(throwArgs *ta, signatureDef *sd, moduleDef *mod,
        FILE *fp, int rgil);
static void generateCatchBlock(moduleDef *mod, exceptionDef *xd,
        signatureDef *sd, FILE *fp, int rgil);
static void generateThrowSpecifier(throwArgs *, FILE *);
static void generateSlot(moduleDef *mod, classDef *cd, enumDef *ed,
        memberDef *md, FILE *fp);
static void generateCastZero(argDef *ad, FILE *fp);
static void generateCallDefaultCtor(ctorDef *ct, FILE *fp);
static void generateVoidPtrCast(argDef *ad, FILE *fp);
static int countVirtuals(classDef *);
static int skipOverload(overDef *, memberDef *, classDef *, classDef *, int);
static int compareMethTab(const void *, const void *);
static int compareEnumMembers(const void *, const void *);
static char *getSubFormatChar(char, argDef *);
static char *createIfaceFileName(const char *, ifaceFileDef *, const char *);
static FILE *createCompilationUnit(moduleDef *mod, const char *fname,
        const char *description, int timestamp);
static FILE *createFile(moduleDef *mod, const char *fname,
        const char *description, int timestamp);
static void closeFile(FILE *);
static void prScopedName(FILE *fp, scopedNameDef *snd, char *sep);
static void prTypeName(FILE *fp, argDef *ad);
static void prScopedClassName(FILE *fp, ifaceFileDef *scope, classDef *cd);
static int isMultiArgSlot(memberDef *md);
static int isIntArgSlot(memberDef *md);
static int isInplaceSequenceSlot(memberDef *md);
static int needErrorFlag(codeBlockList *cbl);
static int needOldErrorFlag(codeBlockList *cbl);
static int needNewInstance(argDef *ad);
static int needDealloc(classDef *cd);
static const char *getBuildResultFormat(argDef *ad);
static const char *getParseResultFormat(argDef *ad, int res_isref, int xfervh);
static void generateParseResultExtraArgs(moduleDef *mod, argDef *ad, int argnr,
        FILE *fp);
static char *makePartName(const char *codeDir, const char *mname, int part,
        const char *srcSuffix);
static void fakeProtectedArgs(signatureDef *sd);
static void normaliseArgs(signatureDef *);
static void restoreArgs(signatureDef *);
static const char *slotName(slotType st);
static void ints_intro(classDef *cd, FILE *fp);
static const char *argName(const char *name, codeBlockList *cbl);
static int usedInCode(codeBlockList *cbl, const char *str);
static void generateDefaultValue(moduleDef *mod, argDef *ad, int argnr,
        FILE *fp);
static void generateClassFromVoid(classDef *cd, const char *cname,
        const char *vname, FILE *fp);
static void generateMappedTypeFromVoid(mappedTypeDef *mtd, const char *cname,
        const char *vname, FILE *fp);
static int generateSubClassConvertors(sipSpec *pt, moduleDef *mod, FILE *fp);
static void generateNameCache(sipSpec *pt, FILE *fp);
static const char *resultOwner(overDef *od);
static void prCachedName(FILE *fp, nameDef *nd, const char *prefix);
static void generateSignalTableEntry(sipSpec *pt, classDef *cd, overDef *sig,
        memberDef *md, int membernr, FILE *fp);
static void generateTypesTable(sipSpec *pt, moduleDef *mod, FILE *fp);
static int py2OnlySlot(slotType st);
static int py2_5LaterSlot(slotType st);
static int keepPyReference(argDef *ad);
static int isDuplicateProtected(classDef *cd, overDef *target);
static char getEncoding(argType atype);
static void generateTypeDefName(ifaceFileDef *iff, FILE *fp);
static void generateTypeDefLink(sipSpec *pt, ifaceFileDef *iff, FILE *fp);
static int overloadHasDocstring(sipSpec *pt, overDef *od, memberDef *md);
static int hasDocstring(sipSpec *pt, overDef *od, memberDef *md,
        ifaceFileDef *scope);
static void generateDocstring(sipSpec *pt, overDef *overs, memberDef *md,
        const char *scope_name, classDef *scope_scope, FILE *fp);
static int overloadHasClassDocstring(sipSpec *pt, ctorDef *ct);
static int hasClassDocstring(sipSpec *pt, classDef *cd);
static void generateClassDocstring(sipSpec *pt, classDef *cd, FILE *fp);
static int isDefaultAPI(sipSpec *pt, apiVersionRangeDef *avd);
static void generateExplicitDocstring(codeBlockList *cbl, FILE *fp);
static int copyConstRefArg(argDef *ad);
static void generatePreprocLine(int linenr, const char *fname, FILE *fp);
static virtErrorHandler *getVirtErrorHandler(sipSpec *pt, overDef *od,
        classDef *cd, moduleDef *mod);


/*
 * Generate the code from a specification.
 */
void generateCode(sipSpec *pt, char *codeDir, char *buildfile, char *docFile,
        const char *srcSuffix, int except, int trace, int releaseGIL,
        int parts, stringList *needed_qualifiers, stringList *xsl,
        const char *consModule, int docs, int timestamp)
{
    exceptions = except;
    tracing = trace;
    release_gil = releaseGIL;
    generating_c = pt->genc;
    docstrings = docs;

    if (srcSuffix == NULL)
        srcSuffix = (generating_c ? ".c" : ".cpp");

    /* Generate the documentation. */
    if (docFile != NULL)
        generateDocumentation(pt,docFile);

    /* Generate the code. */
    if (codeDir != NULL)
    {
        if (isComposite(pt->module))
            generateCompositeCpp(pt, codeDir, timestamp);
        else if (isConsolidated(pt->module))
        {
            moduleDef *mod;

            for (mod = pt->modules; mod != NULL; mod = mod->next)
                if (mod->container == pt->module)
                    generateCpp(pt, mod, codeDir, srcSuffix, parts,
                            needed_qualifiers, xsl, timestamp);

            generateConsolidatedCpp(pt, codeDir, srcSuffix, timestamp);
        }
        else if (consModule != NULL)
            generateComponentCpp(pt, codeDir, consModule, timestamp);
        else
            generateCpp(pt, pt->module, codeDir, srcSuffix, parts,
                    needed_qualifiers, xsl, timestamp);
    }

    /* Generate the build file. */
    if (buildfile != NULL)
        generateBuildFile(pt, buildfile, srcSuffix, consModule);
}


/*
 * Generate the documentation.
 */
static void generateDocumentation(sipSpec *pt, const char *docFile)
{
    FILE *fp;
    codeBlockList *cbl;

    fp = createFile(pt->module, docFile, NULL, FALSE);

    for (cbl = pt->docs; cbl != NULL; cbl = cbl->next)
        fputs(cbl->block->frag, fp);

    closeFile(fp);
}


/*
 * Generate the build file.
 */
static void generateBuildFile(sipSpec *pt, const char *buildFile,
        const char *srcSuffix, const char *consModule)
{
    const char *mname = pt->module->name;
    FILE *fp;

    fp = createFile(pt->module, buildFile, NULL, FALSE);

    prcode(fp, "target = %s\nsources =", mname);

    if (isComposite(pt->module))
        prcode(fp, " sip%scmodule.c", mname);
    else if (isConsolidated(pt->module))
    {
        moduleDef *mod;

        for (mod = pt->modules; mod != NULL; mod = mod->next)
            if (mod->container == pt->module)
                generateBuildFileSources(pt, mod, srcSuffix, fp);

        prcode(fp, " sip%scmodule%s", mname, srcSuffix);
    }
    else if (consModule == NULL)
        generateBuildFileSources(pt, pt->module, srcSuffix, fp);
    else
        prcode(fp, " sip%scmodule.c", mname);

    if (isConsolidated(pt->module))
    {
        moduleDef *mod;

        prcode(fp, "\nheaders =");

        for (mod = pt->modules; mod != NULL; mod = mod->next)
            if (mod->container == pt->module)
                prcode(fp, " sipAPI%s.h", mod->name);
    }
    else if (!isComposite(pt->module) && consModule == NULL)
        prcode(fp, "\nheaders = sipAPI%s.h", mname);

    prcode(fp, "\n");

    closeFile(fp);
}


/*
 * Generate the list of source files for a module.
 */
static void generateBuildFileSources(sipSpec *pt, moduleDef *mod,
        const char *srcSuffix, FILE *fp)
{
    const char *mname = mod->name;

    if (mod->parts)
    {
        int p;

        for (p = 0; p < mod->parts; ++p)
            prcode(fp, " sip%spart%d%s", mname, p, srcSuffix);
    }
    else
    {
        ifaceFileDef *iff;

        prcode(fp, " sip%scmodule%s", mname, srcSuffix);

        for (iff = pt->ifacefiles; iff != NULL; iff = iff->next)
        {
            if (iff->module != mod)
                continue;

            if (iff->type == exception_iface)
                continue;

            if (iff->api_range != NULL)
                prcode(fp, " sip%s%F_%d%s", mname, iff->fqcname, iff->api_range->index, srcSuffix);
            else
                prcode(fp, " sip%s%F%s", mname, iff->fqcname, srcSuffix);
        }
    }
}


/*
 * Generate an expression in C++.
 */
void generateExpression(valueDef *vd, int in_str, FILE *fp)
{
    while (vd != NULL)
    {
        if (vd->cast != NULL)
            prcode(fp, "(%S)", vd->cast);

        if (vd->vunop != '\0')
            prcode(fp,"%c",vd->vunop);

        switch (vd->vtype)
        {
        case qchar_value:
            prcode(fp,"'%c'",vd->u.vqchar);
            break;

        case string_value:
            {
                const char *quote = (in_str ? "\\\"" : "\"");

                prcode(fp,"%s%s%s", quote, vd->u.vstr, quote);
            }

            break;

        case numeric_value:
            prcode(fp,"%l",vd->u.vnum);
            break;

        case real_value:
            prcode(fp,"%g",vd->u.vreal);
            break;

        case scoped_value:
            if (prcode_xml)
                prScopedName(fp, vd->u.vscp, ".");
            else
                prcode(fp, "%S", vd->u.vscp);

            break;

        case fcall_value:
            generateSimpleFunctionCall(vd->u.fcd,fp);
            break;
        }
 
        if (vd->vbinop != '\0')
            prcode(fp,"%c",vd->vbinop);
 
        vd = vd->next;
    }
}


/*
 * Generate the C++ internal module API header file.
 */
static void generateInternalAPIHeader(sipSpec *pt, moduleDef *mod,
        const char *codeDir, stringList *needed_qualifiers, stringList *xsl,
        int timestamp)
{
    char *hfile;
    const char *mname = mod->name;
    int noIntro;
    FILE *fp;
    nameDef *nd;
    moduleDef *imp;
    moduleListDef *mld;

    hfile = concat(codeDir, "/sipAPI", mname, ".h",NULL);
    fp = createFile(mod, hfile, "Internal module API header file.", timestamp);

    /* Include files. */

    prcode(fp,
"\n"
"#ifndef _%sAPI_H\n"
"#define _%sAPI_H\n"
"\n"
"\n"
"#include <sip.h>\n"
        , mname
        , mname);

    if (pluginPyQt4(pt) || pluginPyQt5(pt))
        prcode(fp,
"\n"
"#include <QMetaType>\n"
"#include <QThread>\n"
            );

    /* Define the qualifiers. */
    noIntro = TRUE;

    for (imp = pt->modules; imp != NULL; imp = imp->next)
    {
        qualDef *qd;

        for (qd = imp->qualifiers; qd != NULL; qd = qd->next)
        {
            const char *qtype = NULL;

            switch (qd->qtype)
            {
            case time_qualifier:
                if (selectedQualifier(needed_qualifiers, qd))
                    qtype = "TIMELINE";

                break;

            case platform_qualifier:
                if (selectedQualifier(needed_qualifiers, qd))
                    qtype = "PLATFORM";

                break;

            case feature_qualifier:
                if (!excludedFeature(xsl, qd))
                    qtype = "FEATURE";

                break;
            }

            if (qtype != NULL)
            {
                if (noIntro)
                {
                    prcode(fp,
"\n"
"/* These are the qualifiers that are enabled. */\n"
                        );

                    noIntro = FALSE;
                }

                prcode(fp,
"#define SIP_%s_%s\n"
                    , qtype, qd->name);
            }
        }
    }

    if (!noIntro)
        prcode(fp,
"\n"
            );

    generateCppCodeBlock(pt->exphdrcode, fp);
    generateCppCodeBlock(mod->hdrcode, fp);

    /*
     * Make sure any header code needed by the default exception is included.
     */
    if (mod->defexception != NULL)
        generateCppCodeBlock(mod->defexception->iff->hdrcode, fp);

    /* Shortcuts that hide the messy detail of the APIs. */
    noIntro = TRUE;

    for (nd = pt->namecache; nd != NULL; nd = nd->next)
    {
        if (!isUsedName(nd))
            continue;

        if (noIntro)
        {
            prcode(fp,
"\n"
"/*\n"
" * Convenient names to refer to various strings defined in this module.\n"
" * Only the class names are part of the public API.\n"
" */\n"
                );

            noIntro = FALSE;
        }

        prcode(fp,
"#define %n %d\n"
"#define %N &sipStrings_%s[%d]\n"
            , nd, (int)nd->offset
            , nd, pt->module->name, (int)nd->offset);
    }

    prcode(fp,
"\n"
"#define sipMalloc                   sipAPI_%s->api_malloc\n"
"#define sipFree                     sipAPI_%s->api_free\n"
"#define sipBuildResult              sipAPI_%s->api_build_result\n"
"#define sipCallMethod               sipAPI_%s->api_call_method\n"
"#define sipCallErrorHandler         sipAPI_%s->api_call_error_handler\n"
"#define sipParseResultEx            sipAPI_%s->api_parse_result_ex\n"
"#define sipParseResult              sipAPI_%s->api_parse_result\n"
"#define sipParseArgs                sipAPI_%s->api_parse_args\n"
"#define sipParseKwdArgs             sipAPI_%s->api_parse_kwd_args\n"
"#define sipParsePair                sipAPI_%s->api_parse_pair\n"
"#define sipCommonDtor               sipAPI_%s->api_common_dtor\n"
"#define sipConvertFromSequenceIndex sipAPI_%s->api_convert_from_sequence_index\n"
"#define sipConvertFromVoidPtr       sipAPI_%s->api_convert_from_void_ptr\n"
"#define sipConvertToVoidPtr         sipAPI_%s->api_convert_to_void_ptr\n"
"#define sipAddException             sipAPI_%s->api_add_exception\n"
"#define sipNoFunction               sipAPI_%s->api_no_function\n"
"#define sipNoMethod                 sipAPI_%s->api_no_method\n"
"#define sipAbstractMethod           sipAPI_%s->api_abstract_method\n"
"#define sipBadClass                 sipAPI_%s->api_bad_class\n"
"#define sipBadCatcherResult         sipAPI_%s->api_bad_catcher_result\n"
"#define sipBadCallableArg           sipAPI_%s->api_bad_callable_arg\n"
"#define sipBadOperatorArg           sipAPI_%s->api_bad_operator_arg\n"
"#define sipTrace                    sipAPI_%s->api_trace\n"
"#define sipTransferBack             sipAPI_%s->api_transfer_back\n"
"#define sipTransferTo               sipAPI_%s->api_transfer_to\n"
"#define sipTransferBreak            sipAPI_%s->api_transfer_break\n"
"#define sipSimpleWrapper_Type       sipAPI_%s->api_simplewrapper_type\n"
"#define sipWrapper_Type             sipAPI_%s->api_wrapper_type\n"
"#define sipWrapperType_Type         sipAPI_%s->api_wrappertype_type\n"
"#define sipVoidPtr_Type             sipAPI_%s->api_voidptr_type\n"
"#define sipGetPyObject              sipAPI_%s->api_get_pyobject\n"
"#define sipGetAddress               sipAPI_%s->api_get_address\n"
"#define sipGetMixinAddress          sipAPI_%s->api_get_mixin_address\n"
"#define sipGetCppPtr                sipAPI_%s->api_get_cpp_ptr\n"
"#define sipGetComplexCppPtr         sipAPI_%s->api_get_complex_cpp_ptr\n"
"#define sipIsPyMethod               sipAPI_%s->api_is_py_method\n"
"#define sipCallHook                 sipAPI_%s->api_call_hook\n"
"#define sipEndThread                sipAPI_%s->api_end_thread\n"
"#define sipConnectRx                sipAPI_%s->api_connect_rx\n"
"#define sipDisconnectRx             sipAPI_%s->api_disconnect_rx\n"
"#define sipRaiseUnknownException    sipAPI_%s->api_raise_unknown_exception\n"
"#define sipRaiseTypeException       sipAPI_%s->api_raise_type_exception\n"
"#define sipBadLengthForSlice        sipAPI_%s->api_bad_length_for_slice\n"
"#define sipAddTypeInstance          sipAPI_%s->api_add_type_instance\n"
"#define sipFreeSipslot              sipAPI_%s->api_free_sipslot\n"
"#define sipSameSlot                 sipAPI_%s->api_same_slot\n"
"#define sipPySlotExtend             sipAPI_%s->api_pyslot_extend\n"
"#define sipConvertRx                sipAPI_%s->api_convert_rx\n"
"#define sipAddDelayedDtor           sipAPI_%s->api_add_delayed_dtor\n"
"#define sipCanConvertToType         sipAPI_%s->api_can_convert_to_type\n"
"#define sipConvertToType            sipAPI_%s->api_convert_to_type\n"
"#define sipForceConvertToType       sipAPI_%s->api_force_convert_to_type\n"
"#define sipCanConvertToEnum         sipAPI_%s->api_can_convert_to_enum\n"
"#define sipReleaseType              sipAPI_%s->api_release_type\n"
"#define sipConvertFromType          sipAPI_%s->api_convert_from_type\n"
"#define sipConvertFromNewType       sipAPI_%s->api_convert_from_new_type\n"
"#define sipConvertFromNewPyType     sipAPI_%s->api_convert_from_new_pytype\n"
"#define sipConvertFromEnum          sipAPI_%s->api_convert_from_enum\n"
"#define sipGetState                 sipAPI_%s->api_get_state\n"
"#define sipLong_AsUnsignedLong      sipAPI_%s->api_long_as_unsigned_long\n"
"#define sipExportSymbol             sipAPI_%s->api_export_symbol\n"
"#define sipImportSymbol             sipAPI_%s->api_import_symbol\n"
"#define sipFindType                 sipAPI_%s->api_find_type\n"
"#define sipFindNamedEnum            sipAPI_%s->api_find_named_enum\n"
"#define sipBytes_AsChar             sipAPI_%s->api_bytes_as_char\n"
"#define sipBytes_AsString           sipAPI_%s->api_bytes_as_string\n"
"#define sipString_AsASCIIChar       sipAPI_%s->api_string_as_ascii_char\n"
"#define sipString_AsASCIIString     sipAPI_%s->api_string_as_ascii_string\n"
"#define sipString_AsLatin1Char      sipAPI_%s->api_string_as_latin1_char\n"
"#define sipString_AsLatin1String    sipAPI_%s->api_string_as_latin1_string\n"
"#define sipString_AsUTF8Char        sipAPI_%s->api_string_as_utf8_char\n"
"#define sipString_AsUTF8String      sipAPI_%s->api_string_as_utf8_string\n"
"#define sipUnicode_AsWChar          sipAPI_%s->api_unicode_as_wchar\n"
"#define sipUnicode_AsWString        sipAPI_%s->api_unicode_as_wstring\n"
"#define sipConvertFromConstVoidPtr  sipAPI_%s->api_convert_from_const_void_ptr\n"
"#define sipConvertFromVoidPtrAndSize    sipAPI_%s->api_convert_from_void_ptr_and_size\n"
"#define sipConvertFromConstVoidPtrAndSize   sipAPI_%s->api_convert_from_const_void_ptr_and_size\n"
"#define sipInvokeSlot               sipAPI_%s->api_invoke_slot\n"
"#define sipSaveSlot                 sipAPI_%s->api_save_slot\n"
"#define sipClearAnySlotReference    sipAPI_%s->api_clear_any_slot_reference\n"
"#define sipVisitSlot                sipAPI_%s->api_visit_slot\n"
"#define sipWrappedTypeName(wt)      ((wt)->type->td_cname)\n"
"#define sipDeprecated               sipAPI_%s->api_deprecated\n"
"#define sipKeepReference            sipAPI_%s->api_keep_reference\n"
"#define sipRegisterProxyResolver    sipAPI_%s->api_register_proxy_resolver\n"
"#define sipRegisterPyType           sipAPI_%s->api_register_py_type\n"
"#define sipTypeFromPyTypeObject     sipAPI_%s->api_type_from_py_type_object\n"
"#define sipTypeScope                sipAPI_%s->api_type_scope\n"
"#define sipResolveTypedef           sipAPI_%s->api_resolve_typedef\n"
"#define sipRegisterAttributeGetter  sipAPI_%s->api_register_attribute_getter\n"
"#define sipIsAPIEnabled             sipAPI_%s->api_is_api_enabled\n"
"#define sipSetDestroyOnExit         sipAPI_%s->api_set_destroy_on_exit\n"
"#define sipEnableAutoconversion     sipAPI_%s->api_enable_autoconversion\n"
"#define sipInitMixin                sipAPI_%s->api_init_mixin\n"
"#define sipExportModule             sipAPI_%s->api_export_module\n"
"#define sipInitModule               sipAPI_%s->api_init_module\n"
"\n"
"/* These are deprecated. */\n"
"#define sipMapStringToClass         sipAPI_%s->api_map_string_to_class\n"
"#define sipMapIntToClass            sipAPI_%s->api_map_int_to_class\n"
"#define sipFindClass                sipAPI_%s->api_find_class\n"
"#define sipFindMappedType           sipAPI_%s->api_find_mapped_type\n"
"#define sipConvertToArray           sipAPI_%s->api_convert_to_array\n"
"#define sipConvertToTypedArray      sipAPI_%s->api_convert_to_typed_array\n"
"#define sipWrapper_Check(w)         PyObject_TypeCheck((w), sipAPI_%s->api_wrapper_type)\n"
"#define sipGetWrapper(p, wt)        sipGetPyObject((p), (wt)->type)\n"
"#define sipReleaseInstance(p, wt, s)    sipReleaseType((p), (wt)->type, (s))\n"
"#define sipReleaseMappedType        sipReleaseType\n"
"#define sipCanConvertToInstance(o, wt, f)   sipCanConvertToType((o), (wt)->type, (f))\n"
"#define sipCanConvertToMappedType   sipCanConvertToType\n"
"#define sipConvertToInstance(o, wt, t, f, s, e)     sipConvertToType((o), (wt)->type, (t), (f), (s), (e))\n"
"#define sipConvertToMappedType      sipConvertToType\n"
"#define sipForceConvertToInstance(o, wt, t, f, s, e)    sipForceConvertToType((o), (wt)->type, (t), (f), (s), (e))\n"
"#define sipForceConvertToMappedType sipForceConvertToType\n"
"#define sipConvertFromInstance(p, wt, t)    sipConvertFromType((p), (wt)->type, (t))\n"
"#define sipConvertFromMappedType    sipConvertFromType\n"
"#define sipConvertFromNamedEnum(v, pt)  sipConvertFromEnum((v), ((sipEnumTypeObject *)(pt))->type)\n"
"#define sipConvertFromNewInstance(p, wt, t) sipConvertFromNewType((p), (wt)->type, (t))\n"
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname
        ,mname);

    /* The name strings. */
    prcode(fp,
"\n"
"/* The strings used by this module. */\n"
"extern const char sipStrings_%s[];\n"
        , pt->module->name);

    /* The unscoped enum macros. */
    generateEnumMacros(pt, mod, NULL, NULL, fp);

    generateModuleAPI(pt, mod, fp);

    prcode(fp,
"\n"
"/* The SIP API, this module's API and the APIs of any imported modules. */\n"
"extern const sipAPIDef *sipAPI_%s;\n"
"extern sipExportedModuleDef sipModuleAPI_%s;\n"
        , mname
        , mname, mname);

    for (mld = mod->allimports; mld != NULL; mld = mld->next)
    {
        generateImportedModuleAPI(pt, mod, mld->module, fp);

        prcode(fp,
"extern const sipExportedModuleDef *sipModuleAPI_%s_%s;\n"
            , mname, mld->module->name);
    }

    if (pluginPyQt4(pt) || pluginPyQt5(pt))
    {
        prcode(fp,
"\n"
"typedef const QMetaObject *(*sip_qt_metaobject_func)(sipSimpleWrapper *,sipTypeDef *);\n"
"extern sip_qt_metaobject_func sip_%s_qt_metaobject;\n"
"\n"
"typedef int (*sip_qt_metacall_func)(sipSimpleWrapper *,sipTypeDef *,QMetaObject::Call,int,void **);\n"
"extern sip_qt_metacall_func sip_%s_qt_metacall;\n"
"\n"
            , mname
            , mname);

        if (pluginPyQt5(pt))
            prcode(fp,
"typedef bool (*sip_qt_metacast_func)(sipSimpleWrapper *, const sipTypeDef *, const char *, void **);\n"
                );
        else
            prcode(fp,
"typedef int (*sip_qt_metacast_func)(sipSimpleWrapper *, sipTypeDef *, const char *);\n"
                );

        prcode(fp,
"extern sip_qt_metacast_func sip_%s_qt_metacast;\n"
            , mname);
    }

    /*
     * Note that we don't forward declare the virtual handlers.  This is
     * because we would need to #include everything needed for their argument
     * types.
     */
    prcode(fp,
"\n"
"#endif\n"
        );

    closeFile(fp);
    free(hfile);
}


/*
 * Return the filename of a source code part on the heap.
 */
static char *makePartName(const char *codeDir, const char *mname, int part,
        const char *srcSuffix)
{
    char buf[50];

    sprintf(buf, "part%d", part);

    return concat(codeDir, "/sip", mname, buf, srcSuffix, NULL);
}


/*
 * Generate the C code for a composite module.
 */
static void generateCompositeCpp(sipSpec *pt, const char *codeDir,
        int timestamp)
{
    char *cppfile;
    const char *fullname = pt->module->fullname->text;
    moduleDef *mod;
    FILE *fp;

    cppfile = concat(codeDir, "/sip", pt->module->name, "cmodule.c", NULL);
    fp = createCompilationUnit(pt->module, cppfile, "Composite module code.",
            timestamp);

    prcode(fp,
"\n"
"#include <Python.h>\n"
"\n"
"\n"
"static void sip_import_component_module(PyObject *d, const char *name)\n"
"{\n"
"#if PY_VERSION_HEX >= 0x02050000\n"
"    PyObject *mod = PyImport_ImportModule(name);\n"
"#else\n"
"    PyObject *mod = PyImport_ImportModule((char *)name);\n"
"#endif\n"
"\n"
"    /*\n"
"     * Note that we don't complain if the module can't be imported.  This\n"
"     * is a favour to Linux distro packagers who like to split PyQt into\n"
"     * different sub-packages.\n"
"     */\n"
"    if (mod)\n"
"    {\n"
"        PyDict_Merge(d, PyModule_GetDict(mod), 0);\n"
"        Py_DECREF(mod);\n"
"    }\n"
"}\n"
        );

    generateModDocstring(pt->module, fp);
    generateModInitStart(pt->module, TRUE, fp);
    generateModDefinition(pt->module, "NULL", fp);

    prcode(fp,
"\n"
"    PyObject *sipModule, *sipModuleDict;\n"
"\n"
"#if PY_MAJOR_VERSION >= 3\n"
"    sipModule = PyModule_Create(&sip_module_def);\n"
"#elif PY_VERSION_HEX >= 0x02050000\n"
            );

    if (pt->module->docstring == NULL)
        prcode(fp,
"    sipModule = Py_InitModule(\"%s\", NULL);\n"
"#else\n"
"    sipModule = Py_InitModule((char *)\"%s\", NULL);\n"
            , fullname
            , fullname);
    else
        prcode(fp,
"    sipModule = Py_InitModule3(\"%s\", NULL, doc_mod_%s);\n"
"#else\n"
"    Py_InitModule3((char *)\"%s\", NULL, doc_mod_%s);\n"
            , fullname, pt->module->name
            , fullname, pt->module->name);

    prcode(fp,
"#endif\n"
"\n"
"    if (sipModule == NULL)\n"
"        SIP_MODULE_RETURN(NULL);\n"
"\n"
"    sipModuleDict = PyModule_GetDict(sipModule);\n"
"\n"
        );

    for (mod = pt->modules; mod != NULL; mod = mod->next)
        if (mod->container == pt->module)
            prcode(fp,
"    sip_import_component_module(sipModuleDict, \"%s\");\n"
                , mod->fullname->text);

    prcode(fp,
"\n"
"    PyErr_Clear();\n"
"\n"
"    SIP_MODULE_RETURN(sipModule);\n"
"}\n"
        );

    closeFile(fp);
    free(cppfile);
}


/*
 * Generate the C/C++ code for a consolidated module.
 */
static void generateConsolidatedCpp(sipSpec *pt, const char *codeDir,
        const char *srcSuffix, int timestamp)
{
    char *cppfile;
    const char *mname = pt->module->name;
    const char *fullname = pt->module->fullname->text;
    moduleDef *mod;
    FILE *fp;

    cppfile = concat(codeDir, "/sip", mname, "cmodule", srcSuffix, NULL);
    fp = createCompilationUnit(pt->module, cppfile,
            "Consolidated module code.", timestamp);

    prcode(fp,
"\n"
"#include <Python.h>\n"
"#include <string.h>\n"
"#include <sip.h>\n"
        );

    generateNameCache(pt, fp);

    prcode(fp,
"\n"
"\n"
"/* The component module initialisers. */\n"
        );

    /* Declare the component module initialisers. */
    for (mod = pt->modules; mod != NULL; mod = mod->next)
        if (mod->container == pt->module)
            prcode(fp,
"#if PY_MAJOR_VERSION >= 3\n"
"extern PyObject *sip_init_%s(void);\n"
"#else\n"
"extern void sip_init_%s(void);\n"
"#endif\n"
                , mod->name
                , mod->name);

    /* Generate the init function. */
    prcode(fp,
"\n"
"\n"
        );

    if (!generating_c)
        prcode(fp,
"extern \"C\" {static PyObject *sip_init(PyObject *, PyObject *);}\n"
            );

    prcode(fp,
"static PyObject *sip_init(PyObject *%s, PyObject *arg)\n"
"{\n"
"    struct component {\n"
"        const char *name;\n"
"#if PY_MAJOR_VERSION >= 3\n"
"        PyObject *(*init)(void);\n"
"#else\n"
"        void (*init)(void);\n"
"#endif\n"
"    };\n"
"\n"
"    static struct component components[] = {\n"
        , (generating_c ? "self" : ""));

    for (mod = pt->modules; mod != NULL; mod = mod->next)
        if (mod->container == pt->module)
            prcode(fp,
"        {\"%s\", sip_init_%s},\n"
                , mod->fullname->text, mod->name);

    prcode(fp,
"        {NULL, NULL}\n"
"    };\n"
"\n"
"    const char *name;\n"
"    struct component *scd;\n"
"\n"
"#if PY_MAJOR_VERSION >= 3\n"
"    name = PyBytes_AsString(arg);\n"
"#else\n"
"    name = PyString_AsString(arg);\n"
"#endif\n"
"\n"
"    if (name == NULL)\n"
"        return NULL;\n"
"\n"
"    for (scd = components; scd->name != NULL; ++scd)\n"
"        if (strcmp(scd->name, name) == 0)\n"
"#if PY_MAJOR_VERSION >= 3\n"
"            return (*scd->init)();\n"
"#else\n"
"        {\n"
"            (*scd->init)();\n"
"\n"
"            Py_INCREF(Py_None);\n"
"            return Py_None;\n"
"        }\n"
"#endif\n"
"\n"
"    PyErr_Format(PyExc_ImportError, \"unknown component module %%s\", name);\n"
"\n"
"    return NULL;\n"
"}\n"
        );

    generateModDocstring(pt->module, fp);
    generateModInitStart(pt->module, generating_c, fp);

    prcode(fp,
"    static PyMethodDef sip_methods[] = {\n"
"        {SIP_MLNAME_CAST(\"init\"), sip_init, METH_O, NULL},\n"
"        {NULL, NULL, 0, NULL}\n"
"    };\n"
        );

    generateModDefinition(pt->module, "sip_methods", fp);

    prcode(fp,
"\n"
"#if PY_MAJOR_VERSION >= 3\n"
"    return PyModule_Create(&sip_module_def);\n"
"#elif PY_VERSION_HEX >= 0x02050000\n"
        );

    if (pt->module->docstring == NULL)
        prcode(fp,
"    Py_InitModule(\"%s\", sip_methods);\n"
            , fullname);
    else
        prcode(fp,
"    Py_InitModule3(\"%s\", sip_methods, doc_mod_%s);\n"
            , fullname, mname);

    prcode(fp,
"#else\n"
        );

    if (generating_c)
    {
        if (pt->module->docstring == NULL)
            prcode(fp,
"    Py_InitModule((char *)\"%s\", sip_methods);\n"
                , fullname);
        else
            prcode(fp,
"    Py_InitModule3((char *)\"%s\", sip_methods, doc_mod_%s);\n"
                , fullname, mname);
    }
    else
    {
        if (pt->module->docstring == NULL)
            prcode(fp,
"    Py_InitModule(const_cast<char *>(\"%s\"), sip_methods);\n"
                , fullname);
        else
            prcode(fp,
"    Py_InitModule3(const_cast<char *>(\"%s\"), sip_methods, doc_mod_%s);\n"
                , fullname, mname);
    }

    prcode(fp,
"#endif\n"
"}\n"
        );

    closeFile(fp);
    free(cppfile);
}


/*
 * Generate the C/C++ code for a component module.
 */
static void generateComponentCpp(sipSpec *pt, const char *codeDir,
        const char *consModule, int timestamp)
{
    char *cppfile;
    FILE *fp;

    cppfile = concat(codeDir, "/sip", pt->module->name, "cmodule.c", NULL);
    fp = createCompilationUnit(pt->module, cppfile, "Component module code.",
            timestamp);

    prcode(fp,
"\n"
"#include <Python.h>\n"
        );

    generateModInitStart(pt->module, TRUE, fp);

    prcode(fp,
"    PyObject *sip_mod, *sip_result;\n"
"\n"
"    /* Import the consolidated module. */\n"
"    if ((sip_mod = PyImport_ImportModule(\"%s\")) == NULL)\n"
"        SIP_MODULE_RETURN(NULL);\n"
"\n"
        , consModule);

    prcode(fp,
"    /* Ask the consolidated module to do the initialistion. */\n"
"#if PY_MAJOR_VERSION >= 3\n"
"    sip_result = PyObject_CallMethod(sip_mod, \"init\", \"y\", \"%s\");\n"
"#else\n"
"    sip_result = PyObject_CallMethod(sip_mod, \"init\", \"s\", \"%s\");\n"
"#endif\n"
"    Py_DECREF(sip_mod);\n"
"\n"
"#if PY_MAJOR_VERSION >= 3\n"
"    return sip_result;\n"
"#else\n"
"    Py_XDECREF(sip_result);\n"
"#endif\n"
"}\n"
        , pt->module->fullname->text
        , pt->module->fullname->text);

    closeFile(fp);
    free(cppfile);
}


/*
 * Generate the name cache definition.
 */
static void generateNameCache(sipSpec *pt, FILE *fp)
{
    nameDef *nd;

    prcode(fp,
"\n"
"/* Define the strings used by this module. */\n"
        );

    if (isConsolidated(pt->module))
        prcode(fp,
"extern const char sipStrings_%s[];\n"
            , pt->module->name);

    prcode(fp,
"const char sipStrings_%s[] = {\n"
        , pt->module->name);

    for (nd = pt->namecache; nd != NULL; nd = nd->next)
    {
        const char *cp;

        if (!isUsedName(nd) || isSubstring(nd))
            continue;

        prcode(fp, "    ");

        for (cp = nd->text; *cp != '\0'; ++cp)
            prcode(fp, "'%c', ", *cp);

        prcode(fp, "0,\n");
    }

    prcode(fp, "};\n");
}


/*
 * Generate the C/C++ code.
 */
static void generateCpp(sipSpec *pt, moduleDef *mod, const char *codeDir,
        const char *srcSuffix, int parts, stringList *needed_qualifiers,
        stringList *xsl, int timestamp)
{
    char *cppfile;
    const char *mname = mod->name;
    int nrSccs = 0, files_in_part, max_per_part, this_part, mod_nr, enum_idx;
    int is_inst_class, is_inst_voidp, is_inst_char, is_inst_string;
    int is_inst_int, is_inst_long, is_inst_ulong, is_inst_longlong;
    int is_inst_ulonglong, is_inst_double, nr_enummembers, is_api_versions;
    int is_versioned_functions;
    int hasexternal = FALSE, slot_extenders = FALSE, ctor_extenders = FALSE;
    FILE *fp;
    moduleListDef *mld;
    classDef *cd;
    memberDef *md;
    enumDef *ed;
    ifaceFileDef *iff;
    virtHandlerDef *vhd;
    virtErrorHandler *veh;
    exceptionDef *xd;

    /* Calculate the number of files in each part. */
    if (parts)
    {
        int nr_files = 1;

        for (iff = pt->ifacefiles; iff != NULL; iff = iff->next)
            if (iff->module == mod && iff->type != exception_iface)
                ++nr_files;

        max_per_part = (nr_files + parts - 1) / parts;
        files_in_part = 1;
        this_part = 0;

        cppfile = makePartName(codeDir, mname, 0, srcSuffix);
    }
    else
        cppfile = concat(codeDir, "/sip", mname, "cmodule", srcSuffix, NULL);

    fp = createCompilationUnit(mod, cppfile, "Module code.", timestamp);

    prcode(fp,
"\n"
"#include \"sipAPI%s.h\"\n"
        , mname);

    /*
     * Include the library headers for types used by virtual handlers, module
     * level functions, module level variables and Qt meta types.
     */
    generateUsedIncludes(mod->used, fp);

    generateCppCodeBlock(mod->unitpostinccode, fp);

    /*
     * If there should be a Qt support API then generate stubs values for the
     * optional parts.  These should be undefined in %ModuleCode if a C++
     * implementation is provided.
     */
    if (mod->qobjclass >= 0)
        prcode(fp,
"\n"
"#define sipQtCreateUniversalSignal          0\n"
"#define sipQtFindUniversalSignal            0\n"
"#define sipQtEmitSignal                     0\n"
"#define sipQtConnectPySignal                0\n"
"#define sipQtDisconnectPySignal             0\n"
            );

    /* Define the names. */
    if (mod->container == NULL)
        generateNameCache(pt, fp);

    /* Generate the C++ code blocks. */
    generateCppCodeBlock(mod->cppcode, fp);

    /* Generate any virtual handlers. */
    for (vhd = mod->virthandlers; vhd != NULL; vhd = vhd->next)
        if (!isDuplicateVH(vhd))
            generateVirtualHandler(mod, vhd, fp);

    /* Generate any virtual error handlers. */
    for (veh = pt->errorhandlers; veh != NULL; veh = veh->next)
        if (veh->mod == mod)
        {
            prcode(fp,
"\n"
"\n"
"void sipVEH_%s_%s(sipSimpleWrapper *%s, sip_gilstate_t%s)\n"
"{\n"
                , mname, veh->name, (usedInCode(veh->code, "sipPySelf") ? "sipPySelf" : ""), (usedInCode(veh->code, "sipGILState") ? " sipGILState" : ""));

            generateCppCodeBlock(veh->code, fp);

            prcode(fp,
"}\n"
                );
        }

    /* Generate the global functions. */
    for (md = mod->othfuncs; md != NULL; md = md->next)
        if (md->slot == no_slot)
            generateOrdinaryFunction(pt, mod, NULL, NULL, md, fp);
        else
        {
            overDef *od;

            /*
             * Make sure that there is still an overload and we haven't moved
             * them all to classes.
             */
            for (od = mod->overs; od != NULL; od = od->next)
                if (od->common == md)
                {
                    generateSlot(mod, NULL, NULL, md, fp);
                    slot_extenders = TRUE;
                    break;
                }
        }

    /* Generate any class specific ctor or slot extenders. */
    for (cd = mod->proxies; cd != NULL; cd = cd->next)
    {
        if (cd->ctors != NULL)
        {
            generateTypeInit(cd, mod, fp);
            ctor_extenders = TRUE;
        }

        for (md = cd->members; md != NULL; md = md->next)
        {
            generateSlot(mod, cd, NULL, md, fp);
            slot_extenders = TRUE;
        }
    }

    /* Generate any ctor extender table. */
    if (ctor_extenders)
    {
        prcode(fp,
"\n"
"static sipInitExtenderDef initExtenders[] = {\n"
            );

        for (cd = mod->proxies; cd != NULL; cd = cd->next)
            if (cd->ctors != NULL)
            {
                prcode(fp,
"    {%P, init_%L, ", cd->iff->api_range, cd->iff);

                generateEncodedType(mod, cd, 0, fp);

                prcode(fp, ", NULL},\n"
                    );
            }

        prcode(fp,
"    {-1, NULL, {0, 0, 0}, NULL}\n"
"};\n"
            );
    }

    /* Generate any slot extender table. */
    if (slot_extenders)
    {
        prcode(fp,
"\n"
"static sipPySlotExtenderDef slotExtenders[] = {\n"
            );

        for (md = mod->othfuncs; md != NULL; md = md->next)
        {
            overDef *od;

            if (md->slot == no_slot)
                continue;

            for (od = mod->overs; od != NULL; od = od->next)
                if (od->common == md)
                {
                    if (py2OnlySlot(md->slot))
                        prcode(fp,
"#if PY_MAJOR_VERSION < 3\n"
                            );
                    else if (py2_5LaterSlot(md->slot))
                        prcode(fp,
"#if PY_VERSION_HEX >= 0x02050000\n"
                            );

                    prcode(fp,
"    {(void *)slot_%s, %s, {0, 0, 0}},\n"
                        , md->pyname->text, slotName(md->slot));

                    if (py2OnlySlot(md->slot) || py2_5LaterSlot(md->slot))
                        prcode(fp,
"#endif\n"
                            );

                    break;
                }
        }

        for (cd = mod->proxies; cd != NULL; cd = cd->next)
            for (md = cd->members; md != NULL; md = md->next)
            {
                if (py2OnlySlot(md->slot))
                    prcode(fp,
"#if PY_MAJOR_VERSION < 3\n"
                        );
                else if (py2_5LaterSlot(md->slot))
                    prcode(fp,
"#if PY_VERSION_HEX >= 0x02050000\n"
                        );

                prcode(fp,
"    {(void *)slot_%L_%s, %s, ", cd->iff, md->pyname->text, slotName(md->slot));

                generateEncodedType(mod, cd, 0, fp);

                prcode(fp, "},\n"
                      );

                if (py2OnlySlot(md->slot) || py2_5LaterSlot(md->slot))
                    prcode(fp,
"#endif\n"
                        );
            }

        prcode(fp,
"    {NULL, (sipPySlotType)0, {0, 0, 0}}\n"
"};\n"
            );
    }

    /* Generate the global access functions. */
    generateAccessFunctions(pt, mod, NULL, fp);

    /* Generate any sub-class convertors. */
    nrSccs = generateSubClassConvertors(pt, mod, fp);

    /* Generate the external classes table if needed. */
    for (cd = pt->classes; cd != NULL; cd = cd->next)
    {
        if (!isExternal(cd))
            continue;

        if (cd->iff->module != mod)
            continue;

        if (!hasexternal)
        {
            prcode(fp,
"\n"
"\n"
"/* This defines each external type declared in this module, */\n"
"static sipExternalTypeDef externalTypesTable[] = {\n"
                );

            hasexternal = TRUE;
        }

        prcode(fp,
"    {%d, \"", cd->iff->ifacenr);
        prScopedName(fp, classFQCName(cd), ".");
        prcode(fp,"\"},\n"
            );
    }

    if (hasexternal)
        prcode(fp,
"    {-1, NULL}\n"
"};\n"
            );

    /* Generate any enum slot tables. */
    for (ed = pt->enums; ed != NULL; ed = ed->next)
    {
        memberDef *slot;

        if (ed->module != mod || ed->fqcname == NULL)
            continue;

        if (ed->slots == NULL)
            continue;

        for (slot = ed->slots; slot != NULL; slot = slot->next)
            generateSlot(mod, NULL, ed, slot, fp);

        prcode(fp,
"\n"
"static sipPySlotDef slots_%C[] = {\n"
            , ed->fqcname);

        for (slot = ed->slots; slot != NULL; slot = slot->next)
        {
            const char *stype;

            if ((stype = slotName(slot->slot)) != NULL)
            {
                if (py2OnlySlot(slot->slot))
                    prcode(fp,
"#if PY_MAJOR_VERSION < 3\n"
                        );
                else if (py2_5LaterSlot(slot->slot))
                    prcode(fp,
"#if PY_VERSION_HEX >= 0x02050000\n"
                        );

                prcode(fp,
"    {(void *)slot_%C_%s, %s},\n"
                    , ed->fqcname, slot->pyname->text, stype);

                if (py2OnlySlot(slot->slot) || py2_5LaterSlot(slot->slot))
                    prcode(fp,
"#endif\n"
                        );
            }
        }

        prcode(fp,
"    {0, (sipPySlotType)0}\n"
"};\n"
"\n"
            );
    }

    /* Generate the enum type structures. */
    enum_idx = 0;

    for (ed = pt->enums; ed != NULL; ed = ed->next)
    {
        int type_nr = -1;
        apiVersionRangeDef *avr = NULL;

        if (ed->module != mod || ed->fqcname == NULL)
            continue;

        if (ed->ecd != NULL)
        {
            if (isTemplateClass(ed->ecd))
                continue;

            type_nr = ed->ecd->iff->first_alt->ifacenr;
            avr = ed->ecd->iff->api_range;
        }
        else if (ed->emtd != NULL)
        {
            type_nr = ed->emtd->iff->first_alt->ifacenr;
            avr = ed->emtd->iff->api_range;
        }

        if (enum_idx == 0)
        {
            prcode(fp,
"static sipEnumTypeDef enumTypes[] = {\n"
                );
        }

        ed->enum_idx = enum_idx++;

        prcode(fp,
"    {{%P, ", avr);

        if (ed->next_alt != NULL)
            prcode(fp, "&enumTypes[%d].etd_base", ed->next_alt->enum_idx);
        else
            prcode(fp, "0");

        prcode(fp, ", 0, SIP_TYPE_ENUM, %n, {0}}, %n, %d, ", ed->cname, ed->pyname, type_nr);

        if (ed->slots != NULL)
            prcode(fp, "slots_%C", ed->fqcname);
        else
            prcode(fp, "NULL");

        prcode(fp, "},\n"
            );
    }

    if (enum_idx != 0)
        prcode(fp,
"};\n"
            );

    nr_enummembers = generateEnumMemberTable(pt, mod, NULL, NULL, fp);

    /* Generate the types table. */
    if (mod->nrtypes > 0)
        generateTypesTable(pt, mod, fp);

    if (mod->nrtypedefs > 0)
    {
        typedefDef *td;

        prcode(fp,
"\n"
"\n"
"/*\n"
" * These define each typedef in this module.\n"
" */\n"
"static sipTypedefDef typedefsTable[] = {\n"
            );

        for (td = pt->typedefs; td != NULL; td = td->next)
        {
            if (td->module != mod)
                continue;

            prcode(fp,
"    {\"%S\", \"", td->fqname);

            /* The default behaviour isn't right in a couple of cases. */
            if (td->type.atype == longlong_type)
                prcode(fp, "long long");
            else if (td->type.atype == ulonglong_type)
                prcode(fp, "unsigned long long");
            else
                generateBaseType(NULL, &td->type, FALSE, fp);

            prcode(fp, "\"},\n"
                );
        }

        prcode(fp,
"};\n"
            );
    }

    if (mod->nrvirthandlers > 0)
    {
        prcode(fp,
"\n"
"\n"
"/*\n"
" * This defines the virtual handlers that this module implements and can be\n"
" * used by other modules.\n"
" */\n"
"static sipVirtHandlerFunc virtHandlersTable[] = {\n"
            );

        for (vhd = mod->virthandlers; vhd != NULL; vhd = vhd->next)
            if (!isDuplicateVH(vhd))
                prcode(fp,
"    (sipVirtHandlerFunc)sipVH_%s_%d,\n"
                    , mname, vhd->virthandlernr);

        prcode(fp,
"};\n"
            );
    }

    if (mod->nrvirterrorhandlers > 0)
    {
        prcode(fp,
"\n"
"\n"
"/*\n"
" * This defines the virtual error handlers that this module implements and\n"
" * can be used by other modules.\n"
" */\n"
"static sipVirtErrorHandlerFunc virtErrorHandlersTable[] = {\n"
            );

        for (veh = pt->errorhandlers; veh != NULL; veh = veh->next)
            if (veh->mod == mod)
                prcode(fp,
"    sipVEH_%s_%s,\n"
                    , mname, veh->name);

        prcode(fp,
"};\n"
            );
    }

    if (mod->allimports != NULL)
    {
        prcode(fp,
"\n"
"\n"
"/* This defines the modules that this module needs to import. */\n"
"static sipImportedModuleDef importsTable[] = {\n"
            );

        for (mld = mod->allimports; mld != NULL; mld = mld->next)
            prcode(fp,
"    {\"%s\", %d, NULL},\n"
                , mld->module->fullname->text, mld->module->version);

        prcode(fp,
"    {NULL, -1, NULL}\n"
"};\n"
            );
    }

    if (nrSccs > 0)
    {
        prcode(fp,
"\n"
"\n"
"/* This defines the class sub-convertors that this module defines. */\n"
"static sipSubClassConvertorDef convertorsTable[] = {\n"
            );

        for (cd = pt->classes; cd != NULL; cd = cd->next)
        {
            if (cd->iff->module != mod)
                continue;

            if (cd->convtosubcode == NULL)
                continue;

            prcode(fp,
"    {sipSubClass_%C, ",classFQCName(cd));

            generateEncodedType(mod, cd->subbase, 0, fp);

            prcode(fp,", NULL},\n");
        }

        prcode(fp,
"    {NULL, {0, 0, 0}, NULL}\n"
"};\n"
            );
    }

    /* Generate any license information. */
    if (mod->license != NULL)
    {
        licenseDef *ld = mod->license;

        prcode(fp,
"\n"
"\n"
"/* Define the module's license. */\n"
"static sipLicenseDef module_license = {\n"
            );

        prcode(fp,
"    \"%s\",\n"
            , ld->type);

        if (ld->licensee != NULL)
            prcode(fp,
"    \"%s\",\n"
                , ld->licensee);
        else
            prcode(fp,
"    NULL,\n"
                );

        if (ld->timestamp != NULL)
            prcode(fp,
"    \"%s\",\n"
                , ld->timestamp);
        else
            prcode(fp,
"    NULL,\n"
                );

        if (ld->sig != NULL)
            prcode(fp,
"    \"%s\"\n"
                , ld->sig);
        else
            prcode(fp,
"    NULL\n"
                );

        prcode(fp,
"};\n"
            );
    }

    /* Generate each instance table. */
    is_inst_class = generateClasses(pt, mod, NULL, fp);
    is_inst_voidp = generateVoidPointers(pt, mod, NULL, fp);
    is_inst_char = generateChars(pt, mod, NULL, fp);
    is_inst_string = generateStrings(pt, mod, NULL, fp);
    is_inst_int = generateInts(pt, mod, NULL, fp);
    is_inst_long = generateLongs(pt, mod, NULL, fp);
    is_inst_ulong = generateUnsignedLongs(pt, mod, NULL, fp);
    is_inst_longlong = generateLongLongs(pt, mod, NULL, fp);
    is_inst_ulonglong = generateUnsignedLongLongs(pt, mod, NULL, fp);
    is_inst_double = generateDoubles(pt, mod, NULL, fp);

    /* Generate any exceptions table. */
    if (mod->nrexceptions > 0)
        prcode(fp,
"\n"
"\n"
"static PyObject *exceptionsTable[%d];\n"
            , mod->nrexceptions);

    /* Generate any API versions table. */
    if (mod->api_ranges != NULL || mod->api_versions != NULL)
    {
        apiVersionRangeDef *avr;

        is_api_versions = TRUE;

        prcode(fp,
"\n"
"\n"
"/* This defines the API versions and ranges in use. */\n"
"static int apiVersions[] = {");
        
        for (avr = mod->api_ranges; avr != NULL; avr = avr->next)
            prcode(fp, "%n, %d, %d, ", avr->api_name, avr->from, avr->to);

        for (avr = mod->api_versions; avr != NULL; avr = avr->next)
            prcode(fp, "%n, %d, -1, ", avr->api_name, avr->from);

        prcode(fp, "-1};\n"
            );
    }
    else
        is_api_versions = FALSE;

    /* Generate any versioned global functions. */
    is_versioned_functions = FALSE;

    for (md = mod->othfuncs; md != NULL; md = md->next)
        if (md->slot == no_slot)
        {
            overDef *od;
            int has_docstring;

            if (notVersioned(md))
                continue;

            if (!is_versioned_functions)
            {
                prcode(fp,
"\n"
"\n"
"/* This defines the global functions where all overloads are versioned. */\n"
"static sipVersionedFunctionDef versionedFunctions[] = {\n"
                    );

                is_versioned_functions = TRUE;
            }

            has_docstring = FALSE;

            if (md->docstring != NULL || (docstrings && hasDocstring(pt, mod->overs, md, NULL)))
                has_docstring = TRUE;

            /*
             * Every overload has an entry to capture all the version ranges.
             */
            for (od = mod->overs; od != NULL; od = od->next)
            {
                if (od->common != md)
                    continue;

                prcode(fp,
"    {%n, ", md->pyname);

                if (noArgParser(md) || useKeywordArgs(md))
                    prcode(fp, "(PyCFunction)func_%s, METH_VARARGS|METH_KEYWORDS", md->pyname->text);
                else
                    prcode(fp, "func_%s, METH_VARARGS", md->pyname->text);

                if (has_docstring)
                    prcode(fp, ", doc_%s", md->pyname->text);
                else
                    prcode(fp, ", NULL");

                prcode(fp, ", %P},\n"
                        , od->api_range);
            }
        }

    if (is_versioned_functions)
        prcode(fp,
"    {-1, 0, 0, 0, -1}\n"
"};\n"
            );

    /* Generate any Qt support API. */
    if (mod->qobjclass >= 0)
        prcode(fp,
"\n"
"\n"
"/* This defines the Qt support API. */\n"
"\n"
"static sipQtAPI qtAPI = {\n"
"    &typesTable[%d],\n"
"    sipQtCreateUniversalSignal,\n"
"    sipQtFindUniversalSignal,\n"
"    sipQtCreateUniversalSlot,\n"
"    sipQtDestroyUniversalSlot,\n"
"    sipQtFindSlot,\n"
"    sipQtConnect,\n"
"    sipQtDisconnect,\n"
"    sipQtSameSignalSlotName,\n"
"    sipQtFindSipslot,\n"
"    sipQtEmitSignal,\n"
"    sipQtConnectPySignal,\n"
"    sipQtDisconnectPySignal\n"
"};\n"
            , mod->qobjclass);

    prcode(fp,
"\n"
"\n"
"/* This defines this module. */\n"
"sipExportedModuleDef sipModuleAPI_%s = {\n"
"    0,\n"
"    SIP_API_MINOR_NR,\n"
"    %n,\n"
"    0,\n"
"    %d,\n"
"    sipStrings_%s,\n"
"    %s,\n"
"    %s,\n"
"    %d,\n"
"    %s,\n"
"    %s,\n"
"    %d,\n"
"    %s,\n"
"    %d,\n"
"    %s,\n"
"    %s,\n"
"    %s,\n"
"    %s,\n"
"    {%s, %s, %s, %s, %s, %s, %s, %s, %s, %s},\n"
"    %s,\n"
"    %s,\n"
"    %s,\n"
"    %s,\n"
"    %s,\n"
"    NULL,\n"
"    %s,\n"
"    %s\n"
"};\n"
        , mname
        , mod->fullname
        , mod->version
        , pt->module->name
        , mod->allimports != NULL ? "importsTable" : "NULL"
        , mod->qobjclass >= 0 ? "&qtAPI" : "NULL"
        , mod->nrtypes
        , mod->nrtypes > 0 ? "typesTable" : "NULL"
        , hasexternal ? "externalTypesTable" : "NULL"
        , nr_enummembers
        , nr_enummembers > 0 ? "enummembers" : "NULL"
        , mod->nrtypedefs
        , mod->nrtypedefs > 0 ? "typedefsTable" : "NULL"
        , mod->nrvirthandlers > 0 ? "virtHandlersTable" : "NULL"
        , mod->nrvirterrorhandlers > 0 ? "virtErrorHandlersTable" : "NULL"
        , nrSccs > 0 ? "convertorsTable" : "NULL"
        , is_inst_class ? "typeInstances" : "NULL"
        , is_inst_voidp ? "voidPtrInstances" : "NULL"
        , is_inst_char ? "charInstances" : "NULL"
        , is_inst_string ? "stringInstances" : "NULL"
        , is_inst_int ? "intInstances" : "NULL"
        , is_inst_long ? "longInstances" : "NULL"
        , is_inst_ulong ? "unsignedLongInstances" : "NULL"
        , is_inst_longlong ? "longLongInstances" : "NULL"
        , is_inst_ulonglong ? "unsignedLongLongInstances" : "NULL"
        , is_inst_double ? "doubleInstances" : "NULL"
        , mod->license != NULL ? "&module_license" : "NULL"
        , mod->nrexceptions > 0 ? "exceptionsTable" : "NULL"
        , slot_extenders ? "slotExtenders" : "NULL"
        , ctor_extenders ? "initExtenders" : "NULL"
        , hasDelayedDtors(mod) ? "sipDelayedDtors" : "NULL"
        , is_api_versions ? "apiVersions" : "NULL"
        , is_versioned_functions ? "versionedFunctions" : "NULL");

    generateModDocstring(mod, fp);

    /* Generate the storage for the external API pointers. */
    prcode(fp,
"\n"
"\n"
"/* The SIP API and the APIs of any imported modules. */\n"
"const sipAPIDef *sipAPI_%s;\n"
        , mname);

    for (mld = mod->allimports; mld != NULL; mld = mld->next)
        prcode(fp,
"const sipExportedModuleDef *sipModuleAPI_%s_%s;\n"
            , mname, mld->module->name);

    if (pluginPyQt4(pt) || pluginPyQt5(pt))
        prcode(fp,
"\n"
"sip_qt_metaobject_func sip_%s_qt_metaobject;\n"
"sip_qt_metacall_func sip_%s_qt_metacall;\n"
"sip_qt_metacast_func sip_%s_qt_metacast;\n"
            , mname
            , mname
            , mname);

    /* Generate the Python module initialisation function. */

    if (mod->container == pt->module)
        prcode(fp,
"\n"
"#if PY_MAJOR_VERSION >= 3\n"
"#define SIP_MODULE_DISCARD(r)   Py_DECREF(r)\n"
"#define SIP_MODULE_RETURN(r)    return (r)\n"
"PyObject *sip_init_%s()\n"
"#else\n"
"#define SIP_MODULE_DISCARD(r)\n"
"#define SIP_MODULE_RETURN(r)    return\n"
"void sip_init_%s()\n"
"#endif\n"
"{\n"
            , mname
            , mname);
    else
        generateModInitStart(pt->module, generating_c, fp);

    /* Generate the global functions. */

    prcode(fp,
"    static PyMethodDef sip_methods[] = {\n"
        );

    for (md = mod->othfuncs; md != NULL; md = md->next)
        if (md->slot == no_slot)
        {
            int has_docstring;

            if (!notVersioned(md))
                continue;

            has_docstring = FALSE;

            if (md->docstring != NULL || (docstrings && hasDocstring(pt, mod->overs, md, NULL)))
                has_docstring = TRUE;

            prcode(fp,
"        {SIP_MLNAME_CAST(%N), ", md->pyname);

            if (noArgParser(md) || useKeywordArgs(md))
                prcode(fp, "(PyCFunction)func_%s, METH_VARARGS|METH_KEYWORDS", md->pyname->text);
            else
                prcode(fp, "func_%s, METH_VARARGS", md->pyname->text);

            if (has_docstring)
                prcode(fp, ", SIP_MLDOC_CAST(doc_%s)},\n"
                    , md->pyname->text);
            else
                prcode(fp, ", NULL},\n"
                    );
        }

    prcode(fp,
"        {0, 0, 0, 0}\n"
"    };\n"
        );

    generateModDefinition(mod, "sip_methods", fp);

    prcode(fp,
"\n"
"    PyObject *sipModule, *sipModuleDict;\n"
        );

    generateSipImportVariables(fp);

    /* Generate any pre-initialisation code. */
    generateCppCodeBlock(mod->preinitcode, fp);

    prcode(fp,
"    /* Initialise the module and get it's dictionary. */\n"
"#if PY_MAJOR_VERSION >= 3\n"
"    sipModule = PyModule_Create(&sip_module_def);\n"
"#elif PY_VERSION_HEX >= 0x02050000\n"
        );

    if (mod->docstring == NULL)
        prcode(fp,
"    sipModule = Py_InitModule(%N, sip_methods);\n"
            , mod->fullname);
    else
        prcode(fp,
"    sipModule = Py_InitModule3(%N, sip_methods, doc_mod_%s);\n"
            , mod->fullname, mname);

    prcode(fp,
"#else\n"
        );

    if (generating_c)
    {
        if (mod->docstring == NULL)
            prcode(fp,
"    sipModule = Py_InitModule((char *)%N, sip_methods);\n"
                , mod->fullname);
        else
            prcode(fp,
"    sipModule = Py_InitModule3((char *)%N, sip_methods, doc_mod_%s);\n"
                , mod->fullname, mname);
    }
    else
    {
        if (mod->docstring == NULL)
            prcode(fp,
"    sipModule = Py_InitModule(const_cast<char *>(%N), sip_methods);\n"
                , mod->fullname);
        else
            prcode(fp,
"    sipModule = Py_InitModule3(const_cast<char *>(%N), sip_methods, doc_mod_%s);\n"
                , mod->fullname, mname);
    }

    prcode(fp,
"#endif\n"
"\n"
"    if (sipModule == NULL)\n"
"        SIP_MODULE_RETURN(NULL);\n"
"\n"
"    sipModuleDict = PyModule_GetDict(sipModule);\n"
"\n"
        );

    generateSipImport(mod, fp);

    /* Generate any initialisation code. */
    generateCppCodeBlock(mod->initcode, fp);

    prcode(fp,
"    /* Export the module and publish it's API. */\n"
"    if (sipExportModule(&sipModuleAPI_%s,SIP_API_MAJOR_NR,SIP_API_MINOR_NR,0) < 0)\n"
"    {\n"
"        SIP_MODULE_DISCARD(sipModule);\n"
"        SIP_MODULE_RETURN(0);\n"
"    }\n"
        , mname);

    if (pluginPyQt4(pt) || pluginPyQt5(pt))
    {
        /* Import the helpers. */
        prcode(fp,
"\n"
"    sip_%s_qt_metaobject = (sip_qt_metaobject_func)sipImportSymbol(\"qtcore_qt_metaobject\");\n"
"    sip_%s_qt_metacall = (sip_qt_metacall_func)sipImportSymbol(\"qtcore_qt_metacall\");\n"
"    sip_%s_qt_metacast = (sip_qt_metacast_func)sipImportSymbol(\"qtcore_qt_metacast\");\n"
"\n"
"    if (!sip_%s_qt_metacast)\n"
"        Py_FatalError(\"Unable to import qtcore_qt_metacast\");\n"
"\n"
            , mname
            , mname
            , mname
            , mname);
    }

    prcode(fp,
"    /* Initialise the module now all its dependencies have been set up. */\n"
"    if (sipInitModule(&sipModuleAPI_%s,sipModuleDict) < 0)\n"
"    {\n"
"        SIP_MODULE_DISCARD(sipModule);\n"
"        SIP_MODULE_RETURN(0);\n"
"    }\n"
        , mname);

    mod_nr = 0;

    for (mld = mod->allimports; mld != NULL; mld = mld->next)
    {
        if (mod_nr == 0)
            prcode(fp,
"\n"
"    /* Get the APIs of the modules that this one is dependent on. */\n"
                );

        prcode(fp,
"    sipModuleAPI_%s_%s = sipModuleAPI_%s.em_imports[%d].im_module;\n"
            , mname, mld->module->name, mname, mod_nr);

        ++mod_nr;
    }

    generateTypesInline(pt, mod, fp);

    /* Create any exceptions. */
    for (xd = pt->exceptions; xd != NULL; xd = xd->next)
    {
        if (xd->iff->module != mod)
            continue;

        if (xd->iff->type != exception_iface)
            continue;

        if (xd->exceptionnr < 0)
            continue;

        prcode(fp,
"\n"
"    if ((exceptionsTable[%d] = PyErr_NewException(\n"
"#if PY_MAJOR_VERSION >= 3\n"
"            \"%s.%s\",\n"
"#else\n"
"            const_cast<char *>(\"%s.%s\"),\n"
"#endif\n"
"            "
            , xd->exceptionnr
            , xd->iff->module->name, xd->pyname
            , xd->iff->module->name, xd->pyname);

        if (xd->bibase != NULL)
            prcode(fp, "PyExc_%s", xd->bibase);
        else if (xd->base->iff->module == mod)
            prcode(fp, "exceptionsTable[%d]", xd->base->exceptionnr);
        else
            prcode(fp, "sipException_%C", xd->base->iff->fqcname);

        prcode(fp, ",NULL)) == NULL || PyDict_SetItemString(sipModuleDict,\"%s\",exceptionsTable[%d]) < 0)\n"
"    {\n"
"        SIP_MODULE_DISCARD(sipModule);\n"
"        SIP_MODULE_RETURN(0);\n"
"    }\n"
            , xd->pyname, xd->exceptionnr);
    }

    /* Generate any post-initialisation code. */
    generateCppCodeBlock(mod->postinitcode, fp);

    prcode(fp,
"\n"
"    SIP_MODULE_RETURN(sipModule);\n"
"}\n"
        );

    /* Generate the interface source files. */
    for (iff = pt->ifacefiles; iff != NULL; iff = iff->next)
        if (iff->module == mod && iff->type != exception_iface)
        {
            int need_postinc;

            if (parts && files_in_part++ == max_per_part)
            {
                /* Close the old part. */
                closeFile(fp);
                free(cppfile);

                /* Create a new one. */
                files_in_part = 1;
                ++this_part;

                cppfile = makePartName(codeDir, mname, this_part, srcSuffix);
                fp = createCompilationUnit(mod, cppfile, "Module code.",
                        timestamp);

                prcode(fp,
"\n"
"#include \"sipAPI%s.h\"\n"
                    , mname);

                need_postinc = TRUE;
            }
            else
            {
                need_postinc = FALSE;
            }

            generateIfaceCpp(pt, iff, need_postinc, codeDir, srcSuffix,
                    (parts ? fp : NULL), timestamp);
        }

    closeFile(fp);
    free(cppfile);

    /* How many parts we actually generated. */
    if (parts)
        parts = this_part + 1;

    mod->parts = parts;

    generateInternalAPIHeader(pt, mod, codeDir, needed_qualifiers, xsl,
            timestamp);
}


/*
 * Generate the types table for a module.
 */
static void generateTypesTable(sipSpec *pt, moduleDef *mod, FILE *fp)
{
    int i;
    argDef *ad;
    const char *type_suffix;

    type_suffix = (pluginPyQt5(pt) || pluginPyQt4(pt) || pluginPyQt3(pt)) ?  ".super" : "";

    prcode(fp,
"\n"
"\n"
"/*\n"
" * This defines each type in this module.\n"
" */\n"
"static sipTypeDef *typesTable[] = {\n"
        );

    for (ad = mod->types, i = 0; i < mod->nrtypes; ++i, ++ad)
    {
        switch (ad->atype)
        {
        case class_type:
            if (isExternal(ad->u.cd))
                prcode(fp,
"    0,\n"
                    );
            else
                prcode(fp,
"    &sipTypeDef_%s_%L%s.ctd_base,\n"
                    , mod->name, ad->u.cd->iff, type_suffix);

            break;

        case mapped_type:
            prcode(fp,
"    &sipTypeDef_%s_%L.mtd_base,\n"
                , mod->name, ad->u.mtd->iff);
            break;

        case enum_type:
            prcode(fp,
"    &enumTypes[%d].etd_base,\n"
                , ad->u.ed->enum_idx);
            break;
        }
    }

    prcode(fp,
"};\n"
        );
}


/*
 * Generate the code to import the sip module and get its API.
 */
static void generateSipImport(moduleDef *mod, FILE *fp)
{
    /*
     * Note that we don't use PyCapsule_Import() because it doesn't handle
     * package.module.attribute.
     */

    prcode(fp,
"    /* Get the SIP module's API. */\n"
"#if PY_VERSION_HEX >= 0x02050000\n"
"    sip_sipmod = PyImport_ImportModule(SIP_MODULE_NAME);\n"
"#else\n"
        );

    if (generating_c)
        prcode(fp,
"    sip_sipmod = PyImport_ImportModule((char *)SIP_MODULE_NAME);\n"
            );
    else
        prcode(fp,
"    sip_sipmod = PyImport_ImportModule(const_cast<char *>(SIP_MODULE_NAME));\n"
            );

    prcode(fp,
"#endif\n"
"\n"
"    if (sip_sipmod == NULL)\n"
"    {\n"
"        SIP_MODULE_DISCARD(sipModule);\n"
"        SIP_MODULE_RETURN(NULL);\n"
"    }\n"
"\n"
"    sip_capiobj = PyDict_GetItemString(PyModule_GetDict(sip_sipmod), \"_C_API\");\n"
"    Py_DECREF(sip_sipmod);\n"
"\n"
"#if defined(SIP_USE_PYCAPSULE)\n"
"    if (sip_capiobj == NULL || !PyCapsule_CheckExact(sip_capiobj))\n"
"#else\n"
"    if (sip_capiobj == NULL || !PyCObject_Check(sip_capiobj))\n"
"#endif\n"
"    {\n"
"        SIP_MODULE_DISCARD(sipModule);\n"
"        SIP_MODULE_RETURN(NULL);\n"
"    }\n"
"\n"
        );

    if (generating_c)
        prcode(fp,
"#if defined(SIP_USE_PYCAPSULE)\n"
"    sipAPI_%s = (const sipAPIDef *)PyCapsule_GetPointer(sip_capiobj, SIP_MODULE_NAME \"._C_API\");\n"
"#else\n"
"    sipAPI_%s = (const sipAPIDef *)PyCObject_AsVoidPtr(sip_capiobj);\n"
"#endif\n"
        , mod->name
        , mod->name);
    else
        prcode(fp,
"#if defined(SIP_USE_PYCAPSULE)\n"
"    sipAPI_%s = reinterpret_cast<const sipAPIDef *>(PyCapsule_GetPointer(sip_capiobj, SIP_MODULE_NAME \"._C_API\"));\n"
"#else\n"
"    sipAPI_%s = reinterpret_cast<const sipAPIDef *>(PyCObject_AsVoidPtr(sip_capiobj));\n"
"#endif\n"
"\n"
        , mod->name
        , mod->name);

    prcode(fp,
"#if defined(SIP_USE_PYCAPSULE)\n"
"    if (sipAPI_%s == NULL)\n"
"    {\n"
"        SIP_MODULE_DISCARD(sipModule);\n"
"        SIP_MODULE_RETURN(NULL);\n"
"    }\n"
"#endif\n"
"\n"
        , mod->name);
}


/*
 * Generate the variables needed by generateSipImport().
 */
static void generateSipImportVariables(FILE *fp)
{
    prcode(fp,
"    PyObject *sip_sipmod, *sip_capiobj;\n"
"\n"
        );
}


/*
 * Generate the start of the Python module initialisation function.
 */
static void generateModInitStart(moduleDef *mod, int gen_c, FILE *fp)
{
    prcode(fp,
"\n"
"\n"
"/* The Python module initialisation function. */\n"
"#if PY_MAJOR_VERSION >= 3\n"
"#define SIP_MODULE_ENTRY        PyInit_%s\n"
"#define SIP_MODULE_TYPE         PyObject *\n"
"#define SIP_MODULE_DISCARD(r)   Py_DECREF(r)\n"
"#define SIP_MODULE_RETURN(r)    return (r)\n"
"#else\n"
"#define SIP_MODULE_ENTRY        init%s\n"
"#define SIP_MODULE_TYPE         void\n"
"#define SIP_MODULE_DISCARD(r)\n"
"#define SIP_MODULE_RETURN(r)    return\n"
"#endif\n"
"\n"
"#if defined(SIP_STATIC_MODULE)\n"
"%sSIP_MODULE_TYPE SIP_MODULE_ENTRY(%s)\n"
"#else\n"
"PyMODINIT_FUNC SIP_MODULE_ENTRY(%s)\n"
"#endif\n"
"{\n"
        , mod->name
        , mod->name
        , (gen_c ? "" : "extern \"C\" "), (gen_c ? "void" : "")
        , (gen_c ? "void" : ""));
}


/*
 * Generate the Python v3 module definition structure.
 */
static void generateModDefinition(moduleDef *mod, const char *methods,
        FILE *fp)
{
    prcode(fp,
"\n"
"#if PY_MAJOR_VERSION >= 3\n"
"    static PyModuleDef sip_module_def = {\n"
"        PyModuleDef_HEAD_INIT,\n"
"        \"%s\",\n"
        , mod->fullname->text);

    if (mod->docstring == NULL)
        prcode(fp,
"        NULL,\n"
            );
    else
        prcode(fp,
"        doc_mod_%s,\n"
            , mod->name);

    prcode(fp,
"        -1,\n"
"        %s,\n"
"        NULL,\n"
"        NULL,\n"
"        NULL,\n"
"        NULL\n"
"    };\n"
"#endif\n"
        , methods);
}


/*
 * Generate all the sub-class convertors for a module.
 */
static int generateSubClassConvertors(sipSpec *pt, moduleDef *mod, FILE *fp)
{
    int nrSccs = 0;
    classDef *cd;

    for (cd = pt->classes; cd != NULL; cd = cd->next)
    {
        int needs_sipClass;

        if (cd->iff->module != mod)
            continue;

        if (cd->convtosubcode == NULL)
            continue;

        prcode(fp,
"\n"
"\n"
"/* Convert to a sub-class if possible. */\n"
            );

        if (!generating_c)
            prcode(fp,
"extern \"C\" {static const sipTypeDef *sipSubClass_%C(void **);}\n"
                , classFQCName(cd));

        /* Allow the deprecated use of sipClass rather than sipType. */
        needs_sipClass = usedInCode(cd->convtosubcode, "sipClass");

        prcode(fp,
"static const sipTypeDef *sipSubClass_%C(void **sipCppRet)\n"
"{\n"
"    %S *sipCpp = reinterpret_cast<%S *>(*sipCppRet);\n"
            , classFQCName(cd)
            , classFQCName(cd->subbase), classFQCName(cd->subbase));

        if (needs_sipClass)
            prcode(fp,
"    sipWrapperType *sipClass;\n"
"\n"
                );
        else
            prcode(fp,
"    const sipTypeDef *sipType;\n"
"\n"
                );

        generateCppCodeBlock(cd->convtosubcode, fp);

        if (needs_sipClass)
            prcode(fp,
"\n"
"    return (sipClass ? sipClass->type : 0);\n"
"}\n"
                );
        else
            prcode(fp,
"\n"
"    return sipType;\n"
"}\n"
                );

        ++nrSccs;
    }

    return nrSccs;
}


/*
 * Generate the structure representing an encoded type.
 */
static void generateEncodedType(moduleDef *mod, classDef *cd, int last,
        FILE *fp)
{
    moduleDef *cmod = cd->iff->module;

    prcode(fp, "{%u, ", cd->iff->first_alt->ifacenr);

    if (cmod == mod)
        prcode(fp, "255");
    else
    {
        int mod_nr = 0;
        moduleListDef *mld;

        for (mld = mod->allimports; mld != NULL; mld = mld->next)
        {
            if (mld->module == cmod)
            {
                prcode(fp, "%u", mod_nr);
                break;
            }

            ++mod_nr;
        }
    }

    prcode(fp, ", %u}", last);
}


/*
 * Generate an ordinary function.
 */
static void generateOrdinaryFunction(sipSpec *pt, moduleDef *mod,
        classDef *c_scope, mappedTypeDef *mt_scope, memberDef *md, FILE *fp)
{
    overDef *od;
    int need_intro, has_auto_docstring;
    ifaceFileDef *scope;
    classDef *scope_scope;
    const char *scope_name, *kw_fw_decl, *kw_decl;

    if (mt_scope != NULL)
    {
        scope = mt_scope->iff;
        scope_name = mt_scope->pyname->text;
        scope_scope = NULL;
        od = mt_scope->overs;
    }
    else if (c_scope != NULL)
    {
        scope = c_scope->iff;
        scope_name = c_scope->pyname->text;
        scope_scope = NULL;
        od = c_scope->overs;
    }
    else
    {
        scope = NULL;
        scope_name = NULL;
        scope_scope = NULL;
        od = mod->overs;
    }

    prcode(fp,
"\n"
"\n"
        );

    /* Generate the docstrings. */
    has_auto_docstring = FALSE;

    if (md->docstring != NULL || (docstrings && hasDocstring(pt, od, md, scope)))
    {
        if (scope != NULL)
            prcode(fp,
"PyDoc_STRVAR(doc_%L_%s, ", scope, md->pyname->text);
        else
            prcode(fp,
"PyDoc_STRVAR(doc_%s, " , md->pyname->text);

        if (md->docstring != NULL)
        {
            generateExplicitDocstring(md->docstring, fp);
        }
        else
        {
            generateDocstring(pt, od, md, scope_name, scope_scope, fp);
            has_auto_docstring = TRUE;
        }

        prcode(fp, ");\n"
"\n"
            );
    }

    if (noArgParser(md) || useKeywordArgs(md))
    {
        kw_fw_decl = ", PyObject *";
        kw_decl = ", PyObject *sipKwds";
    }
    else
    {
        kw_fw_decl = "";
        kw_decl = "";
    }

    if (scope != NULL)
    {
        if (!generating_c)
            prcode(fp,
"extern \"C\" {static PyObject *meth_%L_%s(PyObject *, PyObject *%s);}\n"
                , scope, md->pyname->text, kw_fw_decl);

        prcode(fp,
"static PyObject *meth_%L_%s(PyObject *, PyObject *sipArgs%s)\n"
            , scope, md->pyname->text, kw_decl);
    }
    else
    {
        const char *self = (generating_c ? "sipSelf" : "");

        if (!generating_c)
            prcode(fp,
"extern \"C\" {static PyObject *func_%s(PyObject *,PyObject *%s);}\n"
                , md->pyname->text, kw_fw_decl);

        prcode(fp,
"static PyObject *func_%s(PyObject *%s,PyObject *sipArgs%s)\n"
            , md->pyname->text, self, kw_decl);
    }

    prcode(fp,
"{\n"
        );

    need_intro = TRUE;

    while (od != NULL)
    {
        if (od->common == md)
        {
            if (noArgParser(md))
            {
                generateCppCodeBlock(od->methodcode, fp);
                break;
            }

            if (need_intro)
            {
                prcode(fp,
"    PyObject *sipParseErr = NULL;\n"
                    );

                need_intro = FALSE;
            }

            generateFunctionBody(od, c_scope, mt_scope, c_scope, TRUE, mod, fp);
        }

        od = od->next;
    }

    if (!need_intro)
    {
        prcode(fp,
"\n"
"    /* Raise an exception if the arguments couldn't be parsed. */\n"
"    sipNoFunction(sipParseErr, %N, ", md->pyname);

        if (has_auto_docstring)
        {
            if (scope != NULL)
                prcode(fp, "doc_%L_%s", scope, md->pyname->text);
            else
                prcode(fp, "doc_%s", md->pyname->text);
        }
        else
        {
            prcode(fp, "NULL");
        }

        prcode(fp, ");\n"
"\n"
"    return NULL;\n"
            );
    }

    prcode(fp,
"}\n"
        );
}


/*
 * Generate the table of enum members for a scope.  Return the number of them.
 */
static int generateEnumMemberTable(sipSpec *pt, moduleDef *mod, classDef *cd,
        mappedTypeDef *mtd, FILE *fp)
{
    int i, nr_members;
    enumDef *ed;
    enumMemberDef **etab, **et;

    /* First we count how many. */

    nr_members = 0;

    for (ed = pt->enums; ed != NULL; ed = ed->next)
    {
        enumMemberDef *emd;

        if (ed->module != mod)
            continue;

        if (cd != NULL)
        {
            if (ed->ecd != cd || (isProtectedEnum(ed) && !hasShadow(cd)))
                continue;
        }
        else if (mtd != NULL)
        {
            if (ed->emtd != mtd)
                continue;
        }
        else if (ed->ecd != NULL || ed->emtd != NULL || ed->fqcname == NULL)
        {
            continue;
        }

        for (emd = ed->members; emd != NULL; emd = emd->next)
            ++nr_members;
    }

    if (nr_members == 0)
        return 0;

    /* Create a table so they can be sorted. */

    etab = sipCalloc(nr_members, sizeof (enumMemberDef *));

    et = etab;

    for (ed = pt->enums; ed != NULL; ed = ed->next)
    {
        enumMemberDef *emd;

        if (ed->module != mod)
            continue;

        if (cd != NULL)
        {
            if (ed->ecd != cd)
                continue;
        }
        else if (mtd != NULL)
        {
            if (ed->emtd != mtd)
                continue;
        }
        else if (ed->ecd != NULL || ed->emtd != NULL || ed->fqcname == NULL)
        {
            continue;
        }

        for (emd = ed->members; emd != NULL; emd = emd->next)
            *et++ = emd;
    }

    qsort(etab, nr_members, sizeof (enumMemberDef *), compareEnumMembers);

    /* Now generate the table. */

    if (cd == NULL && mtd == NULL)
    {
        prcode(fp,
"\n"
"/* These are the enum members of all global enums. */\n"
"static sipEnumMemberDef enummembers[] = {\n"
        );
    }
    else
    {
        ifaceFileDef *iff = (cd != NULL ? cd->iff : mtd->iff);

        prcode(fp,
"\n"
"static sipEnumMemberDef enummembers_%L[] = {\n"
            , iff);
    }

    for (i = 0; i < nr_members; ++i)
    {
        enumMemberDef *emd;

        emd = etab[i];

        prcode(fp,
"    {%N, ", emd->pyname);

        if (!isNoScope(emd->ed))
        {
            if (cd != NULL)
            {
                if (isProtectedEnum(emd->ed))
                    prcode(fp, "sip%C::", classFQCName(cd));
                else if (isProtectedClass(cd))
                    prcode(fp, "%U::", cd);
                else
                    prcode(fp, "%S::", classFQCName(cd));
            }
            else if (mtd != NULL)
            {
                prcode(fp, "%S::", mtd->iff->fqcname);
            }
        }

        prcode(fp, "%s, %d},\n", emd->cname, emd->ed->first_alt->enumnr);
    }

    prcode(fp,
"};\n"
        );

    return nr_members;
}


/*
 * The qsort helper to compare two enumMemberDef structures based on the name
 * of the enum member.
 */
static int compareEnumMembers(const void *m1,const void *m2)
{
    return strcmp((*(enumMemberDef **)m1)->pyname->text,
              (*(enumMemberDef **)m2)->pyname->text);
}


/*
 * Generate the access functions for the variables.
 */
static void generateAccessFunctions(sipSpec *pt, moduleDef *mod, classDef *cd,
        FILE *fp)
{
    varDef *vd;

    for (vd = pt->vars; vd != NULL; vd = vd->next)
    {
        if (vd->accessfunc == NULL)
            continue;

        if (vd->ecd != cd || vd->module != mod)
            continue;

        prcode(fp,
"\n"
"\n"
"/* Access function. */\n"
            );

        if (!generating_c)
            prcode(fp,
"extern \"C\" {static void *access_%C();}\n"
            , vd->fqcname);

        prcode(fp,
"static void *access_%C()\n"
"{\n"
            , vd->fqcname);

        generateCppCodeBlock(vd->accessfunc, fp);

        prcode(fp,
"}\n"
            );
    }
}


/*
 * Generate the inline code to add a set of generated type instances to a
 * dictionary.
 */
static void generateTypesInline(sipSpec *pt, moduleDef *mod, FILE *fp)
{
    int noIntro;
    varDef *vd;

    noIntro = TRUE;

    for (vd = pt->vars; vd != NULL; vd = vd->next)
    {
        if (vd->module != mod)
            continue;

        if (vd->type.atype != class_type && vd->type.atype != mapped_type && vd->type.atype != enum_type)
            continue;

        if (needsHandler(vd))
            continue;

        /* Skip classes that don't need inline code. */
        if (generating_c || vd->accessfunc != NULL || vd->type.nrderefs != 0)
            continue;

        if (noIntro)
        {
            prcode(fp,
"\n"
"    /*\n"
"     * Define the class, mapped type and enum instances that have to be\n"
"     * added inline.\n"
"     */\n"
                );

            noIntro = FALSE;
        }

        prcode(fp,
"    sipAddTypeInstance(");

        if (vd->ecd == NULL)
            prcode(fp, "sipModuleDict");
        else
            prcode(fp, "(PyObject *)sipTypeAsPyTypeObject(sipType_%C)", classFQCName(vd->ecd));

        prcode(fp, ",%N,", vd->pyname);

        if (isConstArg(&vd->type))
            prcode(fp, "const_cast<%b *>(&%S)", &vd->type, vd->fqcname);
        else
            prcode(fp, "&%S", vd->fqcname);

        if (vd->type.atype == class_type)
            prcode(fp, ",sipType_%C);\n"
                , classFQCName(vd->type.u.cd));
        else if (vd->type.atype == enum_type)
            prcode(fp, ",sipType_%C);\n"
                , vd->type.u.ed->fqcname);
        else
            prcode(fp, ",sipType_%T);\n"
                , &vd->type);
    }
}


/*
 * Generate the code to add a set of class instances to a dictionary.  Return
 * TRUE if there was at least one.
 */
static int generateClasses(sipSpec *pt, moduleDef *mod, classDef *cd, FILE *fp)
{
    int noIntro;
    varDef *vd;

    noIntro = TRUE;

    for (vd = pt->vars; vd != NULL; vd = vd->next)
    {
        if (vd->ecd != cd || vd->module != mod)
            continue;

        if (vd->type.atype != class_type && (vd->type.atype != enum_type || vd->type.u.ed->fqcname == NULL))
            continue;

        if (needsHandler(vd))
            continue;

        /*
         * Skip ordinary C++ class instances which need to be done with inline
         * code rather than through a static table.  This is because C++ does
         * not guarantee the order in which the table and the instance will be
         * created.  So far this has only been seen to be a problem when
         * statically linking SIP generated modules on Windows.
         */
        if (!generating_c && vd->accessfunc == NULL && vd->type.nrderefs == 0)
            continue;

        if (noIntro)
        {
            if (cd != NULL)
                prcode(fp,
"\n"
"\n"
"/* Define the class and enum instances to be added to this type dictionary. */\n"
"static sipTypeInstanceDef typeInstances_%C[] = {\n"
                    , classFQCName(cd));
            else
                prcode(fp,
"\n"
"\n"
"/* Define the class and enum instances to be added to this module dictionary. */\n"
"static sipTypeInstanceDef typeInstances[] = {\n"
                    );

            noIntro = FALSE;
        }

        prcode(fp,
"    {%N, ", vd->pyname);

        if (vd->type.atype == class_type)
        {
            scopedNameDef *vcname = classFQCName(vd->type.u.cd);

            if (vd->accessfunc != NULL)
            {
                prcode(fp, "(void *)access_%C, &sipType_%C, SIP_ACCFUNC|SIP_NOT_IN_MAP", vd->fqcname, vcname);
            }
            else if (vd->type.nrderefs != 0)
            {
                prcode(fp, "&%S, &sipType_%C, SIP_INDIRECT", vd->fqcname, vcname);
            }
            else if (isConstArg(&vd->type))
            {
                prcode(fp, "const_cast<%b *>(&%S), &sipType_%C, 0", &vd->type, vd->fqcname, vcname);
            }
            else
            {
                prcode(fp, "&%S, &sipType_%C, 0", vd->fqcname, vcname);
            }
        }
        else
        {
            prcode(fp, "&%S, &sipType_%C, 0", vd->fqcname, vd->type.u.ed->fqcname);
        }

        prcode(fp, "},\n"
            );
    }

    if (!noIntro)
        prcode(fp,
"    {0, 0, 0, 0}\n"
"};\n"
            );

    return !noIntro;
}


/*
 * Generate the code to add a set of void pointers to a dictionary.  Return
 * TRUE if there was at least one.
 */
static int generateVoidPointers(sipSpec *pt, moduleDef *mod, classDef *cd,
        FILE *fp)
{
    int noIntro;
    varDef *vd;

    noIntro = TRUE;

    for (vd = pt->vars; vd != NULL; vd = vd->next)
    {
        if (vd->ecd != cd || vd->module != mod)
            continue;

        if (vd->type.atype != void_type && vd->type.atype != struct_type)
            continue;

        if (needsHandler(vd))
            continue;

        if (noIntro)
        {
            if (cd != NULL)
                prcode(fp,
"\n"
"\n"
"/* Define the void pointers to be added to this type dictionary. */\n"
"static sipVoidPtrInstanceDef voidPtrInstances_%C[] = {\n"
                    , classFQCName(cd));
            else
                prcode(fp,
"\n"
"\n"
"/* Define the void pointers to be added to this module dictionary. */\n"
"static sipVoidPtrInstanceDef voidPtrInstances[] = {\n"
                    );

            noIntro = FALSE;
        }

        if (isConstArg(&vd->type))
            prcode(fp,
"    {%N, const_cast<%b *>(%S)},\n"
                , vd->pyname, &vd->type, vd->fqcname);
        else
            prcode(fp,
"    {%N, %S},\n"
                , vd->pyname, vd->fqcname);
    }

    if (!noIntro)
        prcode(fp,
"    {0, 0}\n"
"};\n"
            );

    return !noIntro;
}


/*
 * Generate the code to add a set of characters to a dictionary.  Return TRUE
 * if there was at least one.
 */
static int generateChars(sipSpec *pt, moduleDef *mod, classDef *cd, FILE *fp)
{
    int noIntro;
    varDef *vd;

    noIntro = TRUE;

    for (vd = pt->vars; vd != NULL; vd = vd->next)
    {
        argType vtype = vd->type.atype;

        if (vd->ecd != cd || vd->module != mod)
            continue;

        if (!((vtype == ascii_string_type || vtype == latin1_string_type || vtype == utf8_string_type || vtype == sstring_type || vtype == ustring_type || vtype == string_type || vtype == wstring_type) && vd->type.nrderefs == 0))
            continue;

        if (needsHandler(vd))
            continue;

        if (noIntro)
        {
            if (cd != NULL)
                prcode(fp,
"\n"
"\n"
"/* Define the chars to be added to this type dictionary. */\n"
"static sipCharInstanceDef charInstances_%C[] = {\n"
                    , classFQCName(cd));
            else
                prcode(fp,
"\n"
"\n"
"/* Define the chars to be added to this module dictionary. */\n"
"static sipCharInstanceDef charInstances[] = {\n"
                    );

            noIntro = FALSE;
        }

        prcode(fp,
"    {%N, %S, '%c'},\n"
            , vd->pyname, vd->fqcname, getEncoding(vtype));
    }

    if (!noIntro)
        prcode(fp,
"    {0, 0, 0}\n"
"};\n"
            );

    return !noIntro;
}


/*
 * Generate the code to add a set of strings to a dictionary.  Return TRUE if
 * there is at least one.
 */
static int generateStrings(sipSpec *pt, moduleDef *mod, classDef *cd, FILE *fp)
{
    int noIntro;
    varDef *vd;

    noIntro = TRUE;

    for (vd = pt->vars; vd != NULL; vd = vd->next)
    {
        argType vtype = vd->type.atype;

        if (vd->ecd != cd || vd->module != mod)
            continue;

        if (!((vtype == ascii_string_type || vtype == latin1_string_type || vtype == utf8_string_type || vtype == sstring_type || vtype == ustring_type || vtype == string_type || vtype == wstring_type) && vd->type.nrderefs != 0))
            continue;

        if (needsHandler(vd))
            continue;

        if (noIntro)
        {
            if (cd != NULL)
                prcode(fp,
"\n"
"\n"
"/* Define the strings to be added to this type dictionary. */\n"
"static sipStringInstanceDef stringInstances_%C[] = {\n"
                    , classFQCName(cd));
            else
                prcode(fp,
"\n"
"\n"
"/* Define the strings to be added to this module dictionary. */\n"
"static sipStringInstanceDef stringInstances[] = {\n"
                    );

            noIntro = FALSE;
        }

        prcode(fp,
"    {%N, %S, '%c'},\n"
            , vd->pyname, vd->fqcname, getEncoding(vtype));
    }

    if (!noIntro)
        prcode(fp,
"    {0, 0, 0}\n"
"};\n"
            );

    return !noIntro;
}


/*
 * Generate the code to add a set of ints to a dictionary.  Return TRUE if
 * there was at least one.
 */
static int generateInts(sipSpec *pt, moduleDef *mod, classDef *cd, FILE *fp)
{
    int noIntro;
    varDef *vd;
    enumDef *ed;

    noIntro = TRUE;

    for (vd = pt->vars; vd != NULL; vd = vd->next)
    {
        argType vtype = vd->type.atype;

        if (vd->ecd != cd || vd->module != mod)
            continue;

        if (!(vtype == enum_type || vtype == byte_type ||
              vtype == sbyte_type || vtype == ubyte_type ||
              vtype == ushort_type || vtype == short_type ||
              vtype == uint_type || vtype == cint_type || vtype == int_type ||
              vtype == bool_type || vtype == cbool_type))
            continue;

        if (needsHandler(vd))
            continue;

        /* Named enums are handled elsewhere. */
        if (vtype == enum_type && vd->type.u.ed->fqcname != NULL)
            continue;

        if (noIntro)
        {
            ints_intro(cd, fp);
            noIntro = FALSE;
        }

        prcode(fp,
"    {%N, %S},\n"
            , vd->pyname, vd->fqcname);
    }

    /* Now do global anonymous enums. */
    if (cd == NULL)
        for (ed = pt->enums; ed != NULL; ed = ed->next)
        {
            enumMemberDef *em;

            if (ed->ecd != cd || ed->module != mod)
                continue;

            if (ed->fqcname != NULL)
                continue;

            for (em = ed->members; em != NULL; em = em->next)
            {
                if (noIntro)
                {
                    ints_intro(cd, fp);
                    noIntro = FALSE;
                }

                prcode(fp,
"    {%N, %s},\n"
                    , em->pyname, em->cname);
            }
        }

    if (!noIntro)
        prcode(fp,
"    {0, 0}\n"
"};\n"
            );

    return !noIntro;
}


/*
 * Generate the intro for a table of int instances.
 */
static void ints_intro(classDef *cd, FILE *fp)
{
    if (cd != NULL)
        prcode(fp,
"\n"
"\n"
"/* Define the ints to be added to this type dictionary. */\n"
"static sipIntInstanceDef intInstances_%C[] = {\n"
            ,classFQCName(cd));
    else
        prcode(fp,
"\n"
"\n"
"/* Define the ints to be added to this module dictionary. */\n"
"static sipIntInstanceDef intInstances[] = {\n"
            );
}


/*
 * Generate the code to add a set of longs to a dictionary.  Return TRUE if
 * there was at least one.
 */
static int generateLongs(sipSpec *pt, moduleDef *mod, classDef *cd, FILE *fp)
{
    return generateVariableType(pt, mod, cd, long_type, "long", "Long", "long", fp);
}


/*
 * Generate the code to add a set of unsigned longs to a dictionary.  Return
 * TRUE if there was at least one.
 */
static int generateUnsignedLongs(sipSpec *pt, moduleDef *mod, classDef *cd,
        FILE *fp)
{
    return generateVariableType(pt, mod, cd, ulong_type, "unsigned long", "UnsignedLong", "unsignedLong", fp);
}


/*
 * Generate the code to add a set of long longs to a dictionary.  Return TRUE
 * if there was at least one.
 */
static int generateLongLongs(sipSpec *pt, moduleDef *mod, classDef *cd,
        FILE *fp)
{
    return generateVariableType(pt, mod, cd, longlong_type, "long long", "LongLong", "longLong", fp);
}


/*
 * Generate the code to add a set of unsigned long longs to a dictionary.
 * Return TRUE if there was at least one.
 */
static int generateUnsignedLongLongs(sipSpec *pt, moduleDef *mod, classDef *cd,
        FILE *fp)
{
    return generateVariableType(pt, mod, cd, ulonglong_type, "unsigned long long", "UnsignedLongLong", "unsignedLongLong", fp);
}


/*
 * Generate the code to add a set of a particular type to a dictionary.  Return
 * TRUE if there was at least one.
 */
static int generateVariableType(sipSpec *pt, moduleDef *mod, classDef *cd,
        argType atype, const char *eng, const char *s1, const char *s2,
        FILE *fp)
{
    int noIntro;
    varDef *vd;

    noIntro = TRUE;

    for (vd = pt->vars; vd != NULL; vd = vd->next)
    {
        argType vtype = vd->type.atype;

        if (vd->ecd != cd || vd->module != mod)
            continue;

        if (vtype != atype)
            continue;

        if (needsHandler(vd))
            continue;

        if (noIntro)
        {
            if (cd != NULL)
                prcode(fp,
"\n"
"\n"
"/* Define the %ss to be added to this type dictionary. */\n"
"static sip%sInstanceDef %sInstances_%C[] = {\n"
                    , eng
                    , s1, s2, classFQCName(cd));
            else
                prcode(fp,
"\n"
"\n"
"/* Define the %ss to be added to this module dictionary. */\n"
"static sip%sInstanceDef %sInstances[] = {\n"
                    , eng
                    , s1, s2);

            noIntro = FALSE;
        }

        prcode(fp,
"    {%N, %S},\n"
            , vd->pyname, vd->fqcname);
    }

    if (!noIntro)
        prcode(fp,
"    {0, 0}\n"
"};\n"
            );

    return !noIntro;
}


/*
 * Generate the code to add a set of doubles to a dictionary.  Return TRUE if
 * there was at least one.
 */
static int generateDoubles(sipSpec *pt, moduleDef *mod, classDef *cd, FILE *fp)
{
    int noIntro;
    varDef *vd;

    noIntro = TRUE;

    for (vd = pt->vars; vd != NULL; vd = vd->next)
    {
        argType vtype = vd->type.atype;

        if (vd->ecd != cd || vd->module != mod)
            continue;

        if (!(vtype == float_type || vtype == cfloat_type || vtype == double_type || vtype == cdouble_type))
            continue;

        if (needsHandler(vd))
            continue;

        if (noIntro)
        {
            if (cd != NULL)
                prcode(fp,
"\n"
"\n"
"/* Define the doubles to be added to this type dictionary. */\n"
"static sipDoubleInstanceDef doubleInstances_%C[] = {\n"
                    , classFQCName(cd));
            else
                prcode(fp,
"\n"
"\n"
"/* Define the doubles to be added to this module dictionary. */\n"
"static sipDoubleInstanceDef doubleInstances[] = {\n"
                    );

            noIntro = FALSE;
        }

        prcode(fp,
"    {%N, %S},\n"
            , vd->pyname, vd->fqcname);
    }

    if (!noIntro)
        prcode(fp,
"    {0, 0}\n"
"};\n"
            );

    return !noIntro;
}


/*
 * Generate the C/C++ code for an interface.
 */
static void generateIfaceCpp(sipSpec *pt, ifaceFileDef *iff, int need_postinc,
        const char *codeDir, const char *srcSuffix, FILE *master,
        int timestamp)
{
    char *cppfile;
    const char *cmname = iff->module->name;
    classDef *cd;
    mappedTypeDef *mtd;
    FILE *fp;

    if (master == NULL)
    {
        cppfile = createIfaceFileName(codeDir,iff,srcSuffix);
        fp = createCompilationUnit(iff->module, cppfile,
                "Interface wrapper code.", timestamp);

        prcode(fp,
"\n"
"#include \"sipAPI%s.h\"\n"
            , cmname);

        need_postinc = TRUE;
    }
    else
        fp = master;

    prcode(fp,
"\n"
            );

    generateCppCodeBlock(iff->hdrcode, fp);
    generateUsedIncludes(iff->used, fp);

    if (need_postinc)
        generateCppCodeBlock(iff->module->unitpostinccode, fp);

    for (cd = pt->classes; cd != NULL; cd = cd->next)
    {
        /*
         * Protected classes must be generated in the interface file of the
         * enclosing scope.
         */
        if (isProtectedClass(cd))
            continue;

        if (cd->iff == iff && !isExternal(cd))
        {
            classDef *pcd;

            generateClassCpp(cd, pt, fp);

            /* Generate any enclosed protected classes. */
            for (pcd = pt->classes; pcd != NULL; pcd = pcd->next)
                if (isProtectedClass(pcd) && pcd->ecd == cd)
                    generateClassCpp(pcd, pt, fp);
        }
    }

    for (mtd = pt->mappedtypes; mtd != NULL; mtd = mtd->next)
        if (mtd->iff == iff)
            generateMappedTypeCpp(mtd, pt, fp);

    if (master == NULL)
    {
        closeFile(fp);
        free(cppfile);
    }
}


/*
 * Return a filename for an interface C++ or header file on the heap.
 */
static char *createIfaceFileName(const char *codeDir, ifaceFileDef *iff,
        const char *suffix)
{
    char *fn;
    scopedNameDef *snd;

    fn = concat(codeDir,"/sip",iff->module->name,NULL);

    for (snd = iff->fqcname; snd != NULL; snd = snd->next)
        append(&fn,snd->name);

    if (iff->api_range != NULL)
    {
        char buf[50];

        sprintf(buf, "_%d", iff->api_range->index);
        append(&fn, buf);
    }

    append(&fn,suffix);

    return fn;
}


/*
 * Generate the C++ code for a mapped type version.
 */
static void generateMappedTypeCpp(mappedTypeDef *mtd, sipSpec *pt, FILE *fp)
{
    int need_xfer, nr_methods, nr_enums;
    memberDef *md;

    generateCppCodeBlock(mtd->typecode, fp);

    if (!noRelease(mtd))
    {
        /* Generate the assignment helper. */
        prcode(fp,
"\n"
"\n"
            );

        if (!generating_c)
            prcode(fp,
"extern \"C\" {static void assign_%L(void *, SIP_SSIZE_T, const void *);}\n"
                , mtd->iff);

        prcode(fp,
"static void assign_%L(void *sipDst, SIP_SSIZE_T sipDstIdx, const void *sipSrc)\n"
"{\n"
            , mtd->iff);

        if (generating_c)
            prcode(fp,
"    ((%b *)sipDst)[sipDstIdx] = *((const %b *)sipSrc);\n"
                , &mtd->type, &mtd->type);
        else
            prcode(fp,
"    reinterpret_cast<%b *>(sipDst)[sipDstIdx] = *reinterpret_cast<const %b *>(sipSrc);\n"
                , &mtd->type, &mtd->type);

        prcode(fp,
"}\n"
            );

        /* Generate the array allocation helper. */
        prcode(fp,
"\n"
"\n"
            );

        if (!generating_c)
            prcode(fp,
"extern \"C\" {static void *array_%L(SIP_SSIZE_T);}\n"
                , mtd->iff);

        prcode(fp,
"static void *array_%L(SIP_SSIZE_T sipNrElem)\n"
"{\n"
            , mtd->iff);

        if (generating_c)
            prcode(fp,
"    return sipMalloc(sizeof (%b) * sipNrElem);\n"
                , &mtd->type);
        else
            prcode(fp,
"    return new %b[sipNrElem];\n"
                , &mtd->type);

        prcode(fp,
"}\n"
            );

        /* Generate the copy helper. */
        prcode(fp,
"\n"
"\n"
            );

        if (!generating_c)
            prcode(fp,
"extern \"C\" {static void *copy_%L(const void *, SIP_SSIZE_T);}\n"
                , mtd->iff);

        prcode(fp,
"static void *copy_%L(const void *sipSrc, SIP_SSIZE_T sipSrcIdx)\n"
"{\n"
            , mtd->iff);

        if (generating_c)
            prcode(fp,
"    %b *sipPtr = sipMalloc(sizeof (%b));\n"
"    *sipPtr = ((const %b *)sipSrc)[sipSrcIdx];\n"
"\n"
"    return sipPtr;\n"
                , &mtd->type, &mtd->type
                , &mtd->type);
        else
            prcode(fp,
"    return new %b(reinterpret_cast<const %b *>(sipSrc)[sipSrcIdx]);\n"
                , &mtd->type, &mtd->type);

        prcode(fp,
"}\n"
            );

        prcode(fp,
"\n"
"\n"
"/* Call the mapped type's destructor. */\n"
            );

        if (!generating_c)
            prcode(fp,
"extern \"C\" {static void release_%L(void *, int);}\n"
                , mtd->iff);

        prcode(fp,
"static void release_%L(void *ptr, int%s)\n"
"{\n"
            , mtd->iff, (generating_c ? " status" : ""));

        if (release_gil)
            prcode(fp,
"    Py_BEGIN_ALLOW_THREADS\n"
                );

        if (generating_c)
            prcode(fp,
"    sipFree(ptr);\n"
                );
        else
            prcode(fp,
"    delete reinterpret_cast<%b *>(ptr);\n"
                , &mtd->type);

        if (release_gil)
            prcode(fp,
"    Py_END_ALLOW_THREADS\n"
                );

        prcode(fp,
"}\n"
"\n"
            );
    }

    generateConvertToDefinitions(mtd,NULL,fp);

    /* Generate the from type convertor. */
    need_xfer = (generating_c || usedInCode(mtd->convfromcode, "sipTransferObj"));

    prcode(fp,
"\n"
"\n"
        );

    if (!generating_c)
        prcode(fp,
"extern \"C\" {static PyObject *convertFrom_%L(void *, PyObject *);}\n"
            , mtd->iff);

    prcode(fp,
"static PyObject *convertFrom_%L(void *sipCppV, PyObject *%s)\n"
"{\n"
"   ", mtd->iff, (need_xfer ? "sipTransferObj" : ""));

    generateMappedTypeFromVoid(mtd, "sipCpp", "sipCppV", fp);

    prcode(fp, ";\n"
"\n"
        );

    generateCppCodeBlock(mtd->convfromcode,fp);

    prcode(fp,
"}\n"
        );

    /* Generate the static methods. */
    for (md = mtd->members; md != NULL; md = md->next)
        generateOrdinaryFunction(pt, mtd->iff->module, NULL, mtd, md, fp);

    nr_methods = generateMappedTypeMethodTable(pt, mtd, fp);

    nr_enums = generateEnumMemberTable(pt, mtd->iff->module, NULL, mtd, fp);

    prcode(fp,
"\n"
"\n"
"sipMappedTypeDef ");

    generateTypeDefName(mtd->iff, fp);

    prcode(fp, " = {\n"
"    {\n"
"        %P,\n"
"        "
        , mtd->iff->api_range);

    generateTypeDefLink(pt, mtd->iff, fp);

    prcode(fp, ",\n"
"        0,\n"
"        %sSIP_TYPE_MAPPED,\n"
"        %n,\n"
"        {0}\n"
"    },\n"
"    {\n"
        , (handlesNone(mtd) ? "SIP_TYPE_ALLOW_NONE|" : "")
        , mtd->cname);

    if (nr_enums == 0)
        prcode(fp,
"        -1,\n"
            );
    else
        prcode(fp,
"        %n,\n"
            , mtd->pyname);

    prcode(fp,
"        {0, 0, 1},\n"
        );

    if (nr_methods == 0)
        prcode(fp,
"        0, 0,\n"
            );
    else
        prcode(fp,
"        %d, methods_%L,\n"
            , nr_methods, mtd->iff);

    if (nr_enums == 0)
        prcode(fp,
"        0, 0,\n"
            );
    else
        prcode(fp,
"        %d, enummembers_%L,\n"
            , nr_enums, mtd->iff);

    prcode(fp,
"        0, 0,\n"
"        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}\n"
"    },\n"
        );

    if (noRelease(mtd))
        prcode(fp,
"    0,\n"
"    0,\n"
"    0,\n"
"    0,\n"
            );
    else
        prcode(fp,
"    assign_%L,\n"
"    array_%L,\n"
"    copy_%L,\n"
"    release_%L,\n"
            , mtd->iff
            , mtd->iff
            , mtd->iff
            , mtd->iff);

    prcode(fp,
"    convertTo_%L,\n"
"    convertFrom_%L\n"
        , mtd->iff
        , mtd->iff);

    prcode(fp,
"};\n"
        );
}


/*
 * Generate the name of the type structure for a class or mapped type.
 */
static void generateTypeDefName(ifaceFileDef *iff, FILE *fp)
{
    prcode(fp, "sipTypeDef_%s_%L", iff->module->name, iff);
}


/*
 * Generate the link to a type structure implementing an alternate API.
 */
static void generateTypeDefLink(sipSpec *pt, ifaceFileDef *iff, FILE *fp)
{
    if (iff->next_alt != NULL)
    {
        prcode(fp, "&");
        generateTypeDefName(iff->next_alt, fp);

        if (iff->next_alt->type == mappedtype_iface)
            prcode(fp, ".mtd_base");
        else if (pluginPyQt3(pt) || pluginPyQt4(pt) || pluginPyQt5(pt))
            prcode(fp, ".super.ctd_base");
        else
            prcode(fp, ".ctd_base");
    }
    else
        prcode(fp, "0");
}


/*
 * Generate the C++ code for a class.
 */
static void generateClassCpp(classDef *cd, sipSpec *pt, FILE *fp)
{
    moduleDef *mod = cd->iff->module;

    /* Generate any local class code. */

    generateCppCodeBlock(cd->cppcode, fp);

    generateClassFunctions(pt, mod, cd, fp);

    generateAccessFunctions(pt, mod, cd, fp);

    if (cd->iff->type != namespace_iface)
    {
        generateConvertToDefinitions(NULL,cd,fp);

        /* Generate the optional from type convertor. */
        if (cd->convfromcode != NULL)
        {
            int need_xfer;

            need_xfer = (generating_c || usedInCode(cd->convfromcode, "sipTransferObj"));

            prcode(fp,
"\n"
"\n"
                );

            if (!generating_c)
                prcode(fp,
"extern \"C\" {static PyObject *convertFrom_%L(void *, PyObject *);}\n"
                    , cd->iff);

            prcode(fp,
"static PyObject *convertFrom_%L(void *sipCppV, PyObject *%s)\n"
"{\n"
"   ", cd->iff, (need_xfer ? "sipTransferObj" : ""));

            generateClassFromVoid(cd, "sipCpp", "sipCppV", fp);

            prcode(fp, ";\n"
"\n"
                );

            generateCppCodeBlock(cd->convfromcode, fp);

            prcode(fp,
"}\n"
                );
        }
    }

    /* The type definition structure. */
    generateTypeDefinition(pt, cd, fp);
}


/*
 * Return a sorted array of relevant functions for a namespace.
 */

static sortedMethTab *createFunctionTable(memberDef *members, int *nrp)
{
    int nr;
    sortedMethTab *mtab, *mt;
    memberDef *md;

    /* First we need to count the number of applicable functions. */
    nr = 0;

    for (md = members; md != NULL; md = md->next)
        ++nr;

    if ((*nrp = nr) == 0)
        return NULL;

    /* Create the table of methods. */
    mtab = sipCalloc(nr, sizeof (sortedMethTab));

    /* Initialise the table. */
    mt = mtab;

    for (md = members; md != NULL; md = md->next)
    {
        mt->md = md;
        ++mt;
    }

    /* Finally, sort the table. */
    qsort(mtab,nr,sizeof (sortedMethTab),compareMethTab);

    return mtab;
}


/*
 * Return a sorted array of relevant methods (either lazy or non-lazy) for a
 * class.
 */
static sortedMethTab *createMethodTable(classDef *cd, int *nrp)
{
    int nr;
    visibleList *vl;
    sortedMethTab *mtab, *mt;

    /*
     * First we need to count the number of applicable methods.  Only provide
     * an entry point if there is at least one overload that is defined in this
     * class and is a non-abstract function or slot.  We allow private (even
     * though we don't actually generate code) because we need to intercept the
     * name before it reaches a more public version further up the class
     * hierarchy.  We add the ctor and any variable handlers as special
     * entries.
     */
    nr = 0;

    for (vl = cd->visible; vl != NULL; vl = vl->next)
    {
        overDef *od;

        if (vl->m->slot != no_slot)
            continue;

        for (od = vl->cd->overs; od != NULL; od = od->next)
        {
            /*
             * Skip protected methods if we don't have the means to handle
             * them.
             */
            if (isProtected(od) && !hasShadow(cd))
                continue;

            if (skipOverload(od,vl->m,cd,vl->cd,TRUE))
                continue;

            ++nr;

            break;
        }
    }

    if ((*nrp = nr) == 0)
        return NULL;

    /* Create the table of methods. */

    mtab = sipCalloc(nr, sizeof (sortedMethTab));

    /* Initialise the table. */

    mt = mtab;

    for (vl = cd->visible; vl != NULL; vl = vl->next)
    {
        int need_method;
        overDef *od;

        if (vl->m->slot != no_slot)
            continue;

        need_method = FALSE;

        for (od = vl->cd->overs; od != NULL; od = od->next)
        {
            /*
             * Skip protected methods if we don't have the means to handle
             * them.
             */
            if (isProtected(od) && !hasShadow(cd))
                continue;

            if (!skipOverload(od,vl->m,cd,vl->cd,TRUE))
                need_method = TRUE;
        }

        if (need_method)
        {
            mt->md = vl->m;
            ++mt;
        }
    }

    /* Finally sort the table. */

    qsort(mtab,nr,sizeof (sortedMethTab),compareMethTab);

    return mtab;
}


/*
 * The qsort helper to compare two sortedMethTab structures based on the Python
 * name of the method.
 */

static int compareMethTab(const void *m1,const void *m2)
{
    return strcmp(((sortedMethTab *)m1)->md->pyname->text,
              ((sortedMethTab *)m2)->md->pyname->text);
}


/*
 * Generate the sorted table of static methods for a mapped type and return
 * the number of entries.
 */
static int generateMappedTypeMethodTable(sipSpec *pt, mappedTypeDef *mtd,
        FILE *fp)
{
    int nr;
    sortedMethTab *mtab;

    mtab = createFunctionTable(mtd->members, &nr);

    if (mtab != NULL)
    {
        prMethodTable(pt, mtab, nr, mtd->iff, mtd->overs, fp);
        free(mtab);
    }

    return nr;
}


/*
 * Generate the sorted table of methods for a class and return the number of
 * entries.
 */
static int generateClassMethodTable(sipSpec *pt, classDef *cd, FILE *fp)
{
    int nr;
    sortedMethTab *mtab;

    mtab = (cd->iff->type == namespace_iface) ?
            createFunctionTable(cd->members, &nr) :
            createMethodTable(cd, &nr);

    if (mtab != NULL)
    {
        prMethodTable(pt, mtab, nr, cd->iff, cd->overs, fp);
        free(mtab);
    }

    return nr;
}


/*
 * Generate a method table for a class or mapped type.
 */
static void prMethodTable(sipSpec *pt, sortedMethTab *mtable, int nr,
        ifaceFileDef *iff, overDef *overs, FILE *fp)
{
    int i;

    prcode(fp,
"\n"
"\n"
"static PyMethodDef methods_%L[] = {\n"
        , iff);

    for (i = 0; i < nr; ++i)
    {
        memberDef *md = mtable[i].md;
        const char *cast, *flags;
        int has_docstring;

        if (noArgParser(md) || useKeywordArgs(md))
        {
            cast = "(PyCFunction)";
            flags = "|METH_KEYWORDS";
        }
        else
        {
            cast = "";
            flags = "";
        }

        /* Save the index in the table. */
        md->membernr = i;

        has_docstring = FALSE;

        if (md->docstring != NULL || (docstrings && hasDocstring(pt, overs, md, iff)))
            has_docstring = TRUE;

        prcode(fp,
"    {SIP_MLNAME_CAST(%N), %smeth_%L_%s, METH_VARARGS%s, ", md->pyname, cast, iff, md->pyname->text, flags);

        if (has_docstring)
            prcode(fp, "SIP_MLDOC_CAST(doc_%L_%s)", iff, md->pyname->text);
        else
            prcode(fp, "NULL");

        prcode(fp, "}%s\n"
            , ((i + 1) < nr) ? "," : "");
    }

    prcode(fp,
"};\n"
        );
}


/*
 * Generate the "to type" convertor definitions.
 */

static void generateConvertToDefinitions(mappedTypeDef *mtd,classDef *cd,
                     FILE *fp)
{
    codeBlockList *convtocode;
    ifaceFileDef *iff;
    argDef type;

    memset(&type, 0, sizeof (argDef));

    if (cd != NULL)
    {
        convtocode = cd->convtocode;
        iff = cd->iff;

        type.atype = class_type;
        type.u.cd = cd;
    }
    else
    {
        convtocode = mtd->convtocode;
        iff = mtd->iff;

        type.atype = mapped_type;
        type.u.mtd = mtd;
    }

    /* Generate the type convertors. */

    if (convtocode != NULL)
    {
        int need_py, need_ptr, need_iserr, need_xfer;

        /*
         * Sometimes type convertors are just stubs that set the error flag, so
         * check if we actually need everything so that we can avoid compiler
         * warnings.
         */
        need_py = (generating_c || usedInCode(convtocode, "sipPy"));
        need_ptr = (generating_c || usedInCode(convtocode, "sipCppPtr"));
        need_iserr = (generating_c || usedInCode(convtocode, "sipIsErr"));
        need_xfer = (generating_c || usedInCode(convtocode, "sipTransferObj"));

        prcode(fp,
"\n"
"\n"
            );

        if (!generating_c)
            prcode(fp,
"extern \"C\" {static int convertTo_%L(PyObject *, void **, int *, PyObject *);}\n"
                , iff);

        prcode(fp,
"static int convertTo_%L(PyObject *%s,void **%s,int *%s,PyObject *%s)\n"
"{\n"
            , iff, (need_py ? "sipPy" : ""), (need_ptr ? "sipCppPtrV" : ""), (need_iserr ? "sipIsErr" : ""), (need_xfer ? "sipTransferObj" : ""));

        if (need_ptr)
        {
            if (generating_c)
                prcode(fp,
"    %b **sipCppPtr = (%b **)sipCppPtrV;\n"
"\n"
                    , &type, &type);
            else
                prcode(fp,
"    %b **sipCppPtr = reinterpret_cast<%b **>(sipCppPtrV);\n"
"\n"
                    , &type, &type);
        }

        generateCppCodeBlock(convtocode,fp);

        prcode(fp,
"}\n"
            );
    }
}


/*
 * Generate a variable getter.
 */
static void generateVariableGetter(ifaceFileDef *scope, varDef *vd, FILE *fp)
{
    argType atype = vd->type.atype;
    const char *first_arg, *second_arg, *last_arg;
    int needsNew, keepRef;

    if (generating_c || !isStaticVar(vd))
        first_arg = "sipSelf";
    else
        first_arg = "";

    last_arg = (generating_c || usedInCode(vd->getcode, "sipPyType")) ? "sipPyType" : "";

    needsNew = ((atype == class_type || atype == mapped_type) && vd->type.nrderefs == 0 && isConstArg(&vd->type));
    keepRef = (atype == class_type && vd->type.nrderefs == 0 && !isConstArg(&vd->type));

    second_arg = (generating_c || keepRef) ? "sipPySelf" : "";

    prcode(fp,
"\n"
"\n"
        );

    if (!generating_c)
        prcode(fp,
"extern \"C\" {static PyObject *varget_%C(void *, PyObject *, PyObject *);}\n"
            , vd->fqcname);

    prcode(fp,
"static PyObject *varget_%C(void *%s, PyObject *%s, PyObject *%s)\n"
"{\n"
        , vd->fqcname, first_arg, second_arg, last_arg);

    if (vd->getcode != NULL || keepRef)
    {
        prcode(fp,
"    PyObject *sipPy;\n"
            );
    }

    if (vd->getcode == NULL)
    {
        prcode(fp,
"    ");

        generateNamedValueType(scope, &vd->type, "sipVal", fp);

        prcode(fp, ";\n"
            );
    }

    if (!isStaticVar(vd))
    {
        if (generating_c)
            prcode(fp,
"    struct %S *sipCpp = (struct %S *)sipSelf;\n"
                , classFQCName(vd->ecd), classFQCName(vd->ecd));
        else
            prcode(fp,
"    %S *sipCpp = reinterpret_cast<%S *>(sipSelf);\n"
                , classFQCName(vd->ecd), classFQCName(vd->ecd));

        prcode(fp,
"\n"
            );
    }

    /* Handle any handwritten getter. */
    if (vd->getcode != NULL)
    {
        generateCppCodeBlock(vd->getcode, fp);

        prcode(fp,
"\n"
"    return sipPy;\n"
"}\n"
            );

        return;
    }

    if (needsNew)
    {
        if (generating_c)
            prcode(fp,
"    *sipVal = ");
        else
            prcode(fp,
"    sipVal = new %b(", &vd->type);
    }
    else
    {
        prcode(fp,
"    sipVal = ");

        if ((atype == class_type || atype == mapped_type) && vd->type.nrderefs == 0)
            prcode(fp, "&");
    }

    generateVarMember(vd, fp);

    prcode(fp, "%s;\n"
"\n"
        , ((needsNew && !generating_c) ? ")" : ""));

    switch (atype)
    {
    case mapped_type:
    case class_type:
        {
            ifaceFileDef *iff;

            if (atype == mapped_type)
                iff = vd->type.u.mtd->iff;
            else
                iff = vd->type.u.cd->iff;

            prcode(fp,
"    %s sipConvertFrom%sType(", (keepRef ? "sipPy =" : "return"), (needsNew ? "New" : ""));

            if (isConstArg(&vd->type))
                prcode(fp, "const_cast<%b *>(sipVal)", &vd->type);
            else
                prcode(fp, "sipVal");

            prcode(fp, ", sipType_%C, NULL);\n"
                , iff->fqcname);

            if (keepRef)
            {
                prcode(fp,
"    sipKeepReference(sipPy, -1, sipPySelf);\n"
"\n"
"    return sipPy;\n"
                    );
            }
        }

        break;

    case bool_type:
    case cbool_type:
        prcode(fp,
"    return PyBool_FromLong(sipVal);\n"
            );

        break;

    case ascii_string_type:
        if (vd->type.nrderefs == 0)
            prcode(fp,
"    return PyUnicode_DecodeASCII(&sipVal, 1, NULL);\n"
                );
        else
            prcode(fp,
"    if (sipVal == NULL)\n"
"    {\n"
"        Py_INCREF(Py_None);\n"
"        return Py_None;\n"
"    }\n"
"\n"
"    return PyUnicode_DecodeASCII(sipVal, strlen(sipVal), NULL);\n"
                );

        break;

    case latin1_string_type:
        if (vd->type.nrderefs == 0)
            prcode(fp,
"    return PyUnicode_DecodeLatin1(&sipVal, 1, NULL);\n"
                );
        else
            prcode(fp,
"    if (sipVal == NULL)\n"
"    {\n"
"        Py_INCREF(Py_None);\n"
"        return Py_None;\n"
"    }\n"
"\n"
"    return PyUnicode_DecodeLatin1(sipVal, strlen(sipVal), NULL);\n"
                );

        break;

    case utf8_string_type:
        if (vd->type.nrderefs == 0)
            prcode(fp,
"#if PY_MAJOR_VERSION >= 3\n"
"    return PyUnicode_FromStringAndSize(&sipVal, 1);\n"
"#else\n"
"    return PyUnicode_DecodeUTF8(&sipVal, 1, NULL);\n"
"#endif\n"
                );
        else
            prcode(fp,
"    if (sipVal == NULL)\n"
"    {\n"
"        Py_INCREF(Py_None);\n"
"        return Py_None;\n"
"    }\n"
"\n"
"#if PY_MAJOR_VERSION >= 3\n"
"    return PyUnicode_FromString(sipVal);\n"
"#else\n"
"    return PyUnicode_DecodeUTF8(sipVal, strlen(sipVal), NULL);\n"
"#endif\n"
                );

        break;

    case sstring_type:
    case ustring_type:
    case string_type:
        {
            const char *cast = ((atype != string_type) ? "(char *)" : "");

            if (vd->type.nrderefs == 0)
                prcode(fp,
"    return SIPBytes_FromStringAndSize(%s&sipVal, 1);\n"
                    , cast);
            else
                prcode(fp,
"    if (sipVal == NULL)\n"
"    {\n"
"        Py_INCREF(Py_None);\n"
"        return Py_None;\n"
"    }\n"
"\n"
"    return SIPBytes_FromString(%ssipVal);\n"
                    , cast);
        }

        break;

    case wstring_type:
        if (vd->type.nrderefs == 0)
            prcode(fp,
"    return PyUnicode_FromWideChar(&sipVal, 1);\n"
                );
        else
            prcode(fp,
"    if (sipVal == NULL)\n"
"    {\n"
"        Py_INCREF(Py_None);\n"
"        return Py_None;\n"
"    }\n"
"\n"
"    return PyUnicode_FromWideChar(sipVal, (SIP_SSIZE_T)wcslen(sipVal));\n"
                );

        break;

    case float_type:
    case cfloat_type:
        prcode(fp,
"    return PyFloat_FromDouble((double)sipVal);\n"
                );
        break;

    case double_type:
    case cdouble_type:
        prcode(fp,
"    return PyFloat_FromDouble(sipVal);\n"
            );
        break;

    case enum_type:
        if (vd->type.u.ed->fqcname != NULL)
        {
            prcode(fp,
"    return sipConvertFromEnum(sipVal, sipType_%C);\n"
                , vd->type.u.ed->fqcname);

            break;
        }

        /* Drop through. */

    case byte_type:
    case sbyte_type:
    case short_type:
    case cint_type:
    case int_type:
        prcode(fp,
"    return SIPLong_FromLong(sipVal);\n"
            );
        break;

    case long_type:
        prcode(fp,
"    return PyLong_FromLong(sipVal);\n"
            );
        break;

    case ubyte_type:
    case ushort_type:
        prcode(fp,
"#if PY_MAJOR_VERSION >= 3\n"
"    return PyLong_FromUnsignedLong(sipVal);\n"
"#else\n"
"    return PyInt_FromLong(sipVal);\n"
"#endif\n"
            );
        break;

    case uint_type:
    case ulong_type:
        prcode(fp,
"    return PyLong_FromUnsignedLong(sipVal);\n"
            );
        break;

    case longlong_type:
        prcode(fp,
"    return PyLong_FromLongLong(sipVal);\n"
            );
        break;

    case ulonglong_type:
        prcode(fp,
"    return PyLong_FromUnsignedLongLong(sipVal);\n"
            );
        break;

    case struct_type:
    case void_type:
        prcode(fp,
"    return sipConvertFrom%sVoidPtr(", (isConstArg(&vd->type) ? "Const" : ""));
        generateVoidPtrCast(&vd->type, fp);
        prcode(fp, "sipVal);\n");
        break;

    case capsule_type:
        prcode(fp,
"    return SIPCapsule_FromVoidPtr(");
        generateVoidPtrCast(&vd->type, fp);
        prcode(fp, "sipVal);\n");
        break;

    case pyobject_type:
    case pytuple_type:
    case pylist_type:
    case pydict_type:
    case pycallable_type:
    case pyslice_type:
    case pytype_type:
    case pybuffer_type:
        prcode(fp,
"    Py_XINCREF(sipVal);\n"
"    return sipVal;\n"
            );
        break;
    }

    prcode(fp,
"}\n"
        );
}


/*
 * Generate a variable setter.
 */
static void generateVariableSetter(ifaceFileDef *scope, varDef *vd, FILE *fp)
{
    argType atype = vd->type.atype;
    const char *first_arg, *last_arg;
    char *deref;
    int might_be_temp, keep, need_py, need_cpp;

    keep = keepPyReference(&vd->type);

    if (generating_c || !isStaticVar(vd))
        first_arg = "sipSelf";
    else
        first_arg = "";

    if (generating_c || (!isStaticVar(vd) && keep))
        last_arg = "sipPySelf";
    else
        last_arg = "";

    need_py = (generating_c || vd->setcode == NULL || usedInCode(vd->setcode, "sipPy"));
    need_cpp = (generating_c || vd->setcode == NULL || usedInCode(vd->setcode, "sipCpp"));

    prcode(fp,
"\n"
"\n"
        );

    if (!generating_c)
        prcode(fp,
"extern \"C\" {static int varset_%C(void *, PyObject *, PyObject *);}\n"
            , vd->fqcname);

    prcode(fp,
"static int varset_%C(void *%s, PyObject *%s, PyObject *%s)\n"
"{\n"
        , vd->fqcname, (need_cpp ? first_arg : ""), (need_py ? "sipPy" : ""), last_arg);

    if (vd->setcode == NULL)
    {
        prcode(fp,
"    ");

        generateNamedValueType(scope, &vd->type, "sipVal", fp);

        prcode(fp, ";\n"
            );
    }

    if (!isStaticVar(vd) && need_cpp)
    {
        if (generating_c)
            prcode(fp,
"    struct %S *sipCpp = (struct %S *)sipSelf;\n"
                , classFQCName(vd->ecd), classFQCName(vd->ecd));
        else
            prcode(fp,
"    %S *sipCpp = reinterpret_cast<%S *>(sipSelf);\n"
                , classFQCName(vd->ecd), classFQCName(vd->ecd));

        prcode(fp,
"\n"
            );
    }

    /* Handle any handwritten setter. */
    if (vd->setcode != NULL)
    {
        prcode(fp,
"   int sipErr = 0;\n"
"\n"
            );

        generateCppCodeBlock(vd->setcode, fp);

        prcode(fp,
"\n"
"    return (sipErr ? -1 : 0);\n"
"}\n"
            );

        return;
    }

    if (vd->type.nrderefs == 0 && (atype == mapped_type || (atype == class_type && vd->type.u.cd->convtocode != NULL)))
        prcode(fp,
"    int sipValState;\n"
            );

    if (atype == class_type || atype == mapped_type)
        prcode(fp,
"    int sipIsErr = 0;\n"
"\n"
            );

    might_be_temp = generateObjToCppConversion(&vd->type, fp);

    deref = "";

    if (atype == class_type || atype == mapped_type)
    {
        if (vd->type.nrderefs == 0)
            deref = "*";

        prcode(fp,
"\n"
"    if (sipIsErr)\n"
"        return -1;\n"
"\n"
            );
    }
    else
    {
        prcode(fp,
"\n"
"    if (PyErr_Occurred() != NULL)\n"
"        return -1;\n"
"\n"
        );
    }

    if (atype == pyobject_type || atype == pytuple_type ||
        atype == pylist_type || atype == pydict_type ||
        atype == pycallable_type || atype == pyslice_type ||
        atype == pytype_type || atype == pybuffer_type)
    {
        prcode(fp,
"    Py_XDECREF(");

        generateVarMember(vd, fp);

        prcode(fp, ");\n"
"    Py_INCREF(sipVal);\n"
"\n"
            );
    }

    prcode(fp,
"    ");

    generateVarMember(vd, fp);

    prcode(fp, " = %ssipVal;\n"
        , deref);

    /* Note that wchar_t * leaks here. */

    if (might_be_temp)
        prcode(fp,
"\n"
"    sipReleaseType(sipVal, sipType_%C, sipValState);\n"
            , classFQCName(vd->type.u.cd));
    else if (vd->type.atype == mapped_type && vd->type.nrderefs == 0 && !noRelease(vd->type.u.mtd))
        prcode(fp,
"\n"
"    sipReleaseType(sipVal, sipType_%T, sipValState);\n"
            , &vd->type);

    /* Generate the code to keep the object alive while we use its data. */
    if (keep)
    {
        if (isStaticVar(vd))
        {
            prcode(fp,
"\n"
"    static PyObject *sipKeep = 0;\n"
"\n"
"    Py_XDECREF(sipKeep);\n"
"    sipKeep = sipPy;\n"
"    Py_INCREF(sipKeep);\n"
                );
        }
        else
        {
            vd->type.key = scope->module->next_key--;

            prcode(fp,
"\n"
"    sipKeepReference(sipPySelf, %d, sipPy);\n"
                , vd->type.key);
        }
    }

    prcode(fp,
"\n"
"    return 0;\n"
"}\n"
        );
}


/*
 * Generate the member variable of a class.
 */
static void generateVarMember(varDef *vd, FILE *fp)
{
    if (isStaticVar(vd))
        prcode(fp, "%S::", classFQCName(vd->ecd));
    else
        prcode(fp, "sipCpp->");

    prcode(fp, "%s", scopedNameTail(vd->fqcname));
}


/*
 * Generate the declaration of a variable that is initialised from a Python
 * object.  Return TRUE if the value might be a temporary on the heap.
 */
static int generateObjToCppConversion(argDef *ad,FILE *fp)
{
    int might_be_temp = FALSE;
    char *rhs = NULL;

    prcode(fp,
"    sipVal = ");

    switch (ad->atype)
    {
    case mapped_type:
        {
            const char *tail;

            if (generating_c)
            {
                prcode(fp, "(%b *)", ad);
                tail = "";
            }
            else
            {
                prcode(fp, "reinterpret_cast<%b *>(", ad);
                tail = ")";
            }

            /* Note that we don't support /Transfer/ but could do. */

            prcode(fp, "sipForceConvertToType(sipPy,sipType_%T,NULL,%s,%s,&sipIsErr)", ad, (ad->nrderefs ? "0" : "SIP_NOT_NONE"), (ad->nrderefs ? "NULL" : "&sipValState"));

            prcode(fp, "%s;\n"
                , tail);
        }
        break;

    case class_type:
        {
            const char *tail;

            if (ad->nrderefs == 0 && ad->u.cd->convtocode != NULL)
                might_be_temp = TRUE;

            if (generating_c)
            {
                prcode(fp, "(%b *)", ad);
                tail = "";
            }
            else
            {
                prcode(fp, "reinterpret_cast<%b *>(", ad);
                tail = ")";
            }

            /*
             * Note that we don't support /Transfer/ but could do.  We could
             * also support /Constrained/ (so long as we also supported it for
             * all types).
             */

            prcode(fp, "sipForceConvertToType(sipPy,sipType_%C,NULL,%s,%s,&sipIsErr)", classFQCName(ad->u.cd), (ad->nrderefs ? "0" : "SIP_NOT_NONE"), (might_be_temp ? "&sipValState" : "NULL"));

            prcode(fp, "%s;\n"
                , tail);
        }
        break;

    case enum_type:
        prcode(fp, "(%E)SIPLong_AsLong(sipPy);\n"
            , ad->u.ed);
        break;

    case sstring_type:
        if (ad->nrderefs == 0)
            rhs = "(signed char)sipBytes_AsChar(sipPy)";
        else if (isConstArg(ad))
            rhs = "(const signed char *)sipBytes_AsString(sipPy)";
        else
            rhs = "(signed char *)sipBytes_AsString(sipPy)";
        break;

    case ustring_type:
        if (ad->nrderefs == 0)
            rhs = "(unsigned char)sipBytes_AsChar(sipPy)";
        else if (isConstArg(ad))
            rhs = "(const unsigned char *)sipBytes_AsString(sipPy)";
        else
            rhs = "(unsigned char *)sipBytes_AsString(sipPy)";
        break;

    case ascii_string_type:
        if (ad->nrderefs == 0)
            rhs = "sipString_AsASCIIChar(sipPy)";
        else if (isConstArg(ad))
            rhs = "sipString_AsASCIIString(&sipPy)";
        else
            rhs = "(char *)sipString_AsASCIIString(&sipPy)";
        break;

    case latin1_string_type:
        if (ad->nrderefs == 0)
            rhs = "sipString_AsLatin1Char(sipPy)";
        else if (isConstArg(ad))
            rhs = "sipString_AsLatin1String(&sipPy)";
        else
            rhs = "(char *)sipString_AsLatin1String(&sipPy)";
        break;

    case utf8_string_type:
        if (ad->nrderefs == 0)
            rhs = "sipString_AsUTF8Char(sipPy)";
        else if (isConstArg(ad))
            rhs = "sipString_AsUTF8String(&sipPy)";
        else
            rhs = "(char *)sipString_AsUTF8String(&sipPy)";
        break;

    case string_type:
        if (ad->nrderefs == 0)
            rhs = "sipBytes_AsChar(sipPy)";
        else if (isConstArg(ad))
            rhs = "sipBytes_AsString(sipPy)";
        else
            rhs = "(const *)sipBytes_AsString(sipPy)";
        break;

    case wstring_type:
        if (ad->nrderefs == 0)
            rhs = "sipUnicode_AsWChar(sipPy)";
        else
            rhs = "sipUnicode_AsWString(sipPy)";
        break;

    case float_type:
    case cfloat_type:
        rhs = "(float)PyFloat_AsDouble(sipPy)";
        break;

    case double_type:
    case cdouble_type:
        rhs = "PyFloat_AsDouble(sipPy)";
        break;

    case bool_type:
    case cbool_type:
        rhs = "(bool)SIPLong_AsLong(sipPy)";
        break;

    case byte_type:
        rhs = "(char)SIPLong_AsLong(sipPy)";
        break;

    case sbyte_type:
        rhs = "(signed char)SIPLong_AsLong(sipPy)";
        break;

    case ubyte_type:
        rhs = "(unsigned char)sipLong_AsUnsignedLong(sipPy)";
        break;

    case ushort_type:
        rhs = "(unsigned short)sipLong_AsUnsignedLong(sipPy)";
        break;

    case short_type:
        rhs = "(short)SIPLong_AsLong(sipPy)";
        break;

    case uint_type:
        rhs = "(uint)sipLong_AsUnsignedLong(sipPy)";
        break;

    case int_type:
    case cint_type:
        rhs = "(int)SIPLong_AsLong(sipPy)";
        break;

    case ulong_type:
        rhs = "sipLong_AsUnsignedLong(sipPy)";
        break;

    case long_type:
        rhs = "PyLong_AsLong(sipPy)";
        break;

    case ulonglong_type:
        rhs = "PyLong_AsUnsignedLongLongMask(sipPy)";
        break;

    case longlong_type:
        rhs = "PyLong_AsLongLong(sipPy)";
        break;

    case struct_type:
        prcode(fp, "(struct %S *)sipConvertToVoidPtr(sipPy);\n"
            , ad->u.sname);
        break;

    case void_type:
        rhs = "sipConvertToVoidPtr(sipPy)";
        break;

    case capsule_type:
        prcode(fp, "SIPCapsule_AsVoidPtr(sipPy, \"%S\");\n"
            , ad->u.cap);
        break;

    case pyobject_type:
    case pytuple_type:
    case pylist_type:
    case pydict_type:
    case pycallable_type:
    case pyslice_type:
    case pytype_type:
    case pybuffer_type:
        rhs = "sipPy";
        break;
    }

    if (rhs != NULL)
        prcode(fp, "%s;\n"
            , rhs);

    return might_be_temp;
}


/*
 * Returns TRUE if the given method is a slot that takes zero arguments.
 */
int isZeroArgSlot(memberDef *md)
{
    slotType st = md->slot;

    return (st == str_slot || st == int_slot || st == long_slot ||
        st == float_slot || st == invert_slot || st == neg_slot ||
        st == len_slot || st == bool_slot || st == pos_slot ||
        st == abs_slot || st == repr_slot || st == hash_slot ||
        st == index_slot || st == iter_slot || st == next_slot);
}


/*
 * Returns TRUE if the given method is a slot that takes more than one
 * argument.
 */
static int isMultiArgSlot(memberDef *md)
{
    slotType st = md->slot;

    return (st == setitem_slot || st == call_slot);
}


/*
 * Returns TRUE if the given method is a slot that returns void (ie. nothing
 * other than an error indicator).
 */
int isVoidReturnSlot(memberDef *md)
{
    slotType st = md->slot;

    return (st == setitem_slot || st == delitem_slot || st == setattr_slot);
}


/*
 * Returns TRUE if the given method is a slot that returns int.
 */
int isIntReturnSlot(memberDef *md)
{
    slotType st = md->slot;

    return (st == bool_slot || st == contains_slot || st == cmp_slot);
}


/*
 * Returns TRUE if the given method is a slot that returns SIP_SSIZE_T.
 */
int isSSizeReturnSlot(memberDef *md)
{
    slotType st = md->slot;

    return (st == len_slot);
}


/*
 * Returns TRUE if the given method is a slot that returns long.
 */
int isLongReturnSlot(memberDef *md)
{
    slotType st = md->slot;

    return (st == hash_slot);
}


/*
 * Returns TRUE if the given method is a slot that takes an int argument.
 */
static int isIntArgSlot(memberDef *md)
{
    slotType st = md->slot;

    return (st == repeat_slot || st == irepeat_slot);
}


/*
 * Returns TRUE if the given method is an inplace number slot.
 */
int isInplaceNumberSlot(memberDef *md)
{
    slotType st = md->slot;

    return (st == iadd_slot || st == isub_slot || st == imul_slot ||
        st == idiv_slot || st == imod_slot || st == ifloordiv_slot ||
        st == itruediv_slot || st == ior_slot || st == ixor_slot ||
        st == iand_slot || st == ilshift_slot || st == irshift_slot);
}


/*
 * Returns TRUE if the given method is an inplace sequence slot.
 */
static int isInplaceSequenceSlot(memberDef *md)
{
    slotType st = md->slot;

    return (st == iconcat_slot || st == irepeat_slot);
}


/*
 * Returns TRUE if the given method is a number slot.
 */
int isNumberSlot(memberDef *md)
{
    slotType st = md->slot;

    return (st == add_slot || st == sub_slot || st == mul_slot ||
        st == div_slot || st == mod_slot || st == floordiv_slot ||
        st == truediv_slot || st == and_slot || st == or_slot ||
        st == xor_slot || st == lshift_slot || st == rshift_slot);
}


/*
 * Returns TRUE if the given method is a rich compare slot.
 */
int isRichCompareSlot(memberDef *md)
{
    slotType st = md->slot;

    return (st == lt_slot || st == le_slot || st == eq_slot ||
        st == ne_slot || st == gt_slot || st == ge_slot);
}


/*
 * Generate a Python slot handler for either a class, an enum or an extender.
 */
static void generateSlot(moduleDef *mod, classDef *cd, enumDef *ed,
        memberDef *md, FILE *fp)
{
    char *arg_str, *decl_arg_str, *prefix, *ret_type;
    int ret_int, has_args;
    overDef *od, *overs;
    scopedNameDef *fqcname;
    nameDef *pyname;

    if (ed != NULL)
    {
        prefix = "Type";
        pyname = ed->pyname;
        fqcname = ed->fqcname;
        overs = ed->overs;
    }
    else if (cd != NULL)
    {
        prefix = "Type";
        pyname = cd->pyname;
        fqcname = classFQCName(cd);
        overs = cd->overs;
    }
    else
    {
        prefix = NULL;
        pyname = NULL;
        fqcname = NULL;
        overs = mod->overs;
    }

    if (isVoidReturnSlot(md) || isIntReturnSlot(md))
    {
        ret_int = TRUE;
        ret_type = "int ";
    }
    else
    {
        ret_int = FALSE;

        if (isSSizeReturnSlot(md))
            ret_type = "SIP_SSIZE_T ";
        else if (isLongReturnSlot(md))
            ret_type = "long ";
        else
            ret_type = "PyObject *";
    }

    has_args = TRUE;

    if (isIntArgSlot(md))
    {
        has_args = FALSE;
        arg_str = "PyObject *sipSelf,int a0";
        decl_arg_str = "PyObject *,int";
    }
    else if (md->slot == call_slot)
    {
        if (generating_c || useKeywordArgs(md) || noArgParser(md))
            arg_str = "PyObject *sipSelf,PyObject *sipArgs,PyObject *sipKwds";
        else
            arg_str = "PyObject *sipSelf,PyObject *sipArgs,PyObject *";

        decl_arg_str = "PyObject *,PyObject *,PyObject *";
    }
    else if (isMultiArgSlot(md))
    {
        arg_str = "PyObject *sipSelf,PyObject *sipArgs";
        decl_arg_str = "PyObject *,PyObject *";
    }
    else if (isZeroArgSlot(md))
    {
        has_args = FALSE;
        arg_str = "PyObject *sipSelf";
        decl_arg_str = "PyObject *";
    }
    else if (isNumberSlot(md))
    {
        arg_str = "PyObject *sipArg0,PyObject *sipArg1";
        decl_arg_str = "PyObject *,PyObject *";
    }
    else if (md->slot == setattr_slot)
    {
        arg_str = "PyObject *sipSelf,PyObject *sipName,PyObject *sipValue";
        decl_arg_str = "PyObject *,PyObject *,PyObject *";
    }
    else
    {
        arg_str = "PyObject *sipSelf,PyObject *sipArg";
        decl_arg_str = "PyObject *,PyObject *";
    }

    prcode(fp,
"\n"
"\n"
        );

    if (py2OnlySlot(md->slot))
        prcode(fp,
"#if PY_MAJOR_VERSION < 3\n"
            );
    else if (py2_5LaterSlot(md->slot))
        prcode(fp,
"#if PY_VERSION_HEX >= 0x02050000\n"
            );

    if (!generating_c)
    {
        prcode(fp,
"extern \"C\" {static %sslot_", ret_type);

        if (cd != NULL)
            prcode(fp, "%L_", cd->iff);
        else if (fqcname != NULL)
            prcode(fp, "%C_", fqcname);

        prcode(fp, "%s(%s);}\n"
            , md->pyname->text, decl_arg_str);
    }

    prcode(fp,
"static %sslot_", ret_type);

    if (cd != NULL)
        prcode(fp, "%L_", cd->iff);
    else if (fqcname != NULL)
        prcode(fp, "%C_", fqcname);

    prcode(fp, "%s(%s)\n"
"{\n"
        , md->pyname->text, arg_str);

    if (md->slot == call_slot && noArgParser(md))
    {
        for (od = overs; od != NULL; od = od->next)
            if (od->common == md)
                generateCppCodeBlock(od->methodcode, fp);
    }
    else
    {
        if (isInplaceNumberSlot(md))
            prcode(fp,
"    if (!PyObject_TypeCheck(sipSelf, sipTypeAsPyTypeObject(sip%s_%C)))\n"
"    {\n"
"        Py_INCREF(Py_NotImplemented);\n"
"        return Py_NotImplemented;\n"
"    }\n"
"\n"
                , prefix, fqcname);

        if (!isNumberSlot(md))
        {
            if (cd != NULL)
                prcode(fp,
"    %S *sipCpp = reinterpret_cast<%S *>(sipGetCppPtr((sipSimpleWrapper *)sipSelf,sipType_%C));\n"
"\n"
"    if (!sipCpp)\n"
"        return %s;\n"
"\n"
                    , fqcname, fqcname, fqcname
                    , (md->slot == cmp_slot ? "-2" : (ret_int ? "-1" : "0")));
            else
                prcode(fp,
"    %S sipCpp = static_cast<%S>(SIPLong_AsLong(sipSelf));\n"
"\n"
                    , fqcname, fqcname);
        }

        if (has_args)
            prcode(fp,
"    PyObject *sipParseErr = NULL;\n"
                );

        for (od = overs; od != NULL; od = od->next)
            if (od->common == md && isAbstract(od))
            {
                prcode(fp,
"    PyObject *sipOrigSelf = sipSelf;\n"
                    );

                break;
            }

        for (od = overs; od != NULL; od = od->next)
            if (od->common == md)
                generateFunctionBody(od, cd, NULL, cd, (ed == NULL && !dontDerefSelf(od)), mod, fp);

        if (has_args)
        {
            switch (md->slot)
            {
            case cmp_slot:
                prcode(fp,
"\n"
"    return 2;\n"
                    );
                break;

            case concat_slot:
            case iconcat_slot:
            case repeat_slot:
            case irepeat_slot:
                prcode(fp,
"\n"
"    /* Raise an exception if the argument couldn't be parsed. */\n"
"    sipBadOperatorArg(sipSelf,sipArg,%s);\n"
"\n"
"    return NULL;\n"
                    ,slotName(md->slot));
                break;

            default:
                if (isNumberSlot(md) || isRichCompareSlot(md) || isInplaceNumberSlot(md))
                {
                    prcode(fp,
"\n"
"    Py_XDECREF(sipParseErr);\n"
"\n"
"    if (sipParseErr == Py_None)\n"
"        return NULL;\n"
                        );
                }

                if (isNumberSlot(md) || isRichCompareSlot(md))
                {
                    /* We can't extend enum slots. */
                    if (cd == NULL)
                        prcode(fp,
"\n"
"    Py_INCREF(Py_NotImplemented);\n"
"    return Py_NotImplemented;\n"
                            );
                    else if (isNumberSlot(md))
                        prcode(fp,
"\n"
"    return sipPySlotExtend(&sipModuleAPI_%s,%s,NULL,sipArg0,sipArg1);\n"
                            , mod->name, slotName(md->slot));
                    else
                        prcode(fp,
"\n"
"    return sipPySlotExtend(&sipModuleAPI_%s,%s,sipType_%C,sipSelf,sipArg);\n"
                            , mod->name, slotName(md->slot), fqcname);
                }
                else if (isInplaceNumberSlot(md))
                {
                    prcode(fp,
"\n"
"    PyErr_Clear();\n"
"\n"
"    Py_INCREF(Py_NotImplemented);\n"
"    return Py_NotImplemented;\n"
                        );
                }
                else
                {
                    prcode(fp,
"\n"
"    /* Raise an exception if the arguments couldn't be parsed. */\n"
"    sipNoMethod(sipParseErr, %N, ", pyname);

                    if (md->slot == setattr_slot)
                        prcode(fp, "(sipValue != NULL ? sipName___setattr__ : sipName___delattr__)");
                    else
                        prcode(fp, "%N", md->pyname);

                    prcode(fp, ", NULL);\n"
"\n"
"    return %s;\n"
                        ,ret_int ? "-1" : "0");
                }
            }
        }
        else
        {
            prcode(fp,
"\n"
"    return 0;\n"
                );
        }
    }

    prcode(fp,
"}\n"
        );

    if (py2OnlySlot(md->slot) || py2_5LaterSlot(md->slot))
        prcode(fp,
"#endif\n"
            );
}


/*
 * Generate the member functions for a class.
 */
static void generateClassFunctions(sipSpec *pt, moduleDef *mod, classDef *cd,
        FILE *fp)
{
    visibleList *vl;
    memberDef *md;

    /* Any shadow code. */
    if (hasShadow(cd))
    {
        if (!isExportDerived(cd))
            generateShadowClassDeclaration(pt, cd, fp);

        generateShadowCode(pt, mod, cd, fp);
    }

    /* The member functions. */
    for (vl = cd->visible; vl != NULL; vl = vl->next)
        if (vl->m->slot == no_slot)
            generateFunction(pt, vl->m, vl->cd->overs, cd, vl->cd, mod, fp);

    /* The slot functions. */
    for (md = cd->members; md != NULL; md = md->next)
        if (cd->iff->type == namespace_iface)
            generateOrdinaryFunction(pt, mod, cd, NULL, md, fp);
        else if (md->slot != no_slot && md->slot != unicode_slot)
            generateSlot(mod, cd, NULL, md, fp);

    if (cd->iff->type != namespace_iface && !generating_c)
    {
        classList *cl;
        int need_ptr, need_cast_ptr, need_state;

        /* The cast function. */
        prcode(fp,
"\n"
"\n"
"/* Cast a pointer to a type somewhere in its superclass hierarchy. */\n"
"extern \"C\" {static void *cast_%L(void *, const sipTypeDef *);}\n"
"static void *cast_%L(void *ptr, const sipTypeDef *targetType)\n"
"{\n"
            , cd->iff
            , cd->iff);

        if (cd->supers != NULL)
            prcode(fp,
"    void *res;\n"
"\n"
                );

        prcode(fp,
"    if (targetType == sipType_%C)\n"
"        return ptr;\n"
            ,classFQCName(cd));

        for (cl = cd->supers; cl != NULL; cl = cl->next)
        {
            scopedNameDef *sname = cl->cd->iff->fqcname;

            prcode(fp,
"\n"
"    if ((res = ((const sipClassTypeDef *)sipType_%C)->ctd_cast((%S *)(%S *)ptr,targetType)) != NULL)\n"
"        return res;\n"
                ,sname,sname,classFQCName(cd));
        }

        prcode(fp,
"\n"
"    return NULL;\n"
"}\n"
            );

        /* Generate the release function without compiler warnings. */
        need_ptr = need_cast_ptr = need_state = FALSE;

        if (cd->dealloccode != NULL)
            need_ptr = need_cast_ptr = usedInCode(cd->dealloccode, "sipCpp");

        if (canCreate(cd) || isPublicDtor(cd))
        {
            if ((pluginPyQt4(pt) || pluginPyQt5(pt)) && isQObjectSubClass(cd) && isPublicDtor(cd))
                need_ptr = need_cast_ptr = TRUE;
            else if (hasShadow(cd))
                need_ptr = need_state = TRUE;
            else if (isPublicDtor(cd))
                need_ptr = TRUE;
        }

        prcode(fp,
"\n"
"\n"
"/* Call the instance's destructor. */\n"
            );

        if (!generating_c)
            prcode(fp,
"extern \"C\" {static void release_%L(void *, int);}\n"
                , cd->iff);

        prcode(fp,
"static void release_%L(void *%s,int%s)\n"
"{\n"
            , cd->iff, (need_ptr ? "sipCppV" : ""), (need_state ? " sipState" : ""));

        if (need_cast_ptr)
        {
            prcode(fp,
"    ");

            generateClassFromVoid(cd, "sipCpp", "sipCppV", fp);

            prcode(fp, ";\n"
"\n"
                );
        }

        if (cd->dealloccode != NULL)
        {
            generateCppCodeBlock(cd->dealloccode, fp);

            prcode(fp,
"\n"
                );
        }

        if (canCreate(cd) || isPublicDtor(cd))
        {
            int rgil = ((release_gil || isReleaseGILDtor(cd)) && !isHoldGILDtor(cd));

            /*
             * If there is an explicit public dtor then assume there is some
             * way to call it which we haven't worked out (because we don't
             * fully understand C++).
             */

            if (rgil)
                prcode(fp,
"    Py_BEGIN_ALLOW_THREADS\n"
"\n"
                    );

            if ((pluginPyQt4(pt) || pluginPyQt5(pt)) && isQObjectSubClass(cd) && isPublicDtor(cd))
            {
                /*
                 * QObjects should only be deleted in the threads that they
                 * belong to.
                 */
                prcode(fp,
"    if (QThread::currentThread() == sipCpp->thread())\n"
"        delete sipCpp;\n"
"    else\n"
"        sipCpp->deleteLater();\n"
                        );
            }
            else if (hasShadow(cd))
            {
                prcode(fp,
"    if (sipState & SIP_DERIVED_CLASS)\n"
"        delete reinterpret_cast<sip%C *>(sipCppV);\n"
                    , classFQCName(cd));

                if (isPublicDtor(cd))
                    prcode(fp,
"    else\n"
"        delete reinterpret_cast<%U *>(sipCppV);\n"
                        , cd);
            }
            else if (isPublicDtor(cd))
                prcode(fp,
"    delete reinterpret_cast<%U *>(sipCppV);\n"
                    , cd);

            if (rgil)
                prcode(fp,
"\n"
"    Py_END_ALLOW_THREADS\n"
                    );
        }

        prcode(fp,
"}\n"
            );
    }

    /* The traverse function. */
    if (cd->travcode != NULL)
    {
        prcode(fp,
"\n"
"\n"
            );

        if (!generating_c)
            prcode(fp,
"extern \"C\" {static int traverse_%C(void *, visitproc, void *);}\n"
                , classFQCName(cd));

        prcode(fp,
"static int traverse_%C(void *sipCppV,visitproc sipVisit,void *sipArg)\n"
"{\n"
"    ", classFQCName(cd));

        generateClassFromVoid(cd, "sipCpp", "sipCppV", fp);

        prcode(fp, ";\n"
"    int sipRes;\n"
"\n"
            );

        generateCppCodeBlock(cd->travcode, fp);

        prcode(fp,
"\n"
"    return sipRes;\n"
"}\n"
            );
    }

    /* The clear function. */
    if (cd->clearcode != NULL)
    {
        prcode(fp,
"\n"
"\n"
            );

        if (!generating_c)
            prcode(fp,
"extern \"C\" {static int clear_%C(void *);}\n"
                , classFQCName(cd));

        prcode(fp,
"static int clear_%C(void *sipCppV)\n"
"{\n"
"    ", classFQCName(cd));

        generateClassFromVoid(cd, "sipCpp", "sipCppV", fp);

        prcode(fp, ";\n"
"    int sipRes;\n"
"\n"
            );

        generateCppCodeBlock(cd->clearcode, fp);

        prcode(fp,
"\n"
"    return sipRes;\n"
"}\n"
            );
    }

    /* The buffer interface functions. */
    if (cd->getbufcode != NULL)
    {
        int need_cpp = usedInCode(cd->getbufcode, "sipCpp");

        prcode(fp,
"\n"
"\n"
"#if PY_MAJOR_VERSION >= 3\n"
            );

        if (!generating_c)
            prcode(fp,
"extern \"C\" {static int getbuffer_%C(PyObject *, void *, Py_buffer *, int);}\n"
                , classFQCName(cd));

        prcode(fp,
"static int getbuffer_%C(PyObject *%s, void *%s, Py_buffer *sipBuffer, int %s)\n"
"{\n"
            , classFQCName(cd), argName("sipSelf", cd->getbufcode), (generating_c || need_cpp ? "sipCppV" : ""), argName("sipFlags", cd->getbufcode));

        if (need_cpp)
        {
            prcode(fp, "    ");
            generateClassFromVoid(cd, "sipCpp", "sipCppV", fp);
            prcode(fp, ";\n"
                );
        }

        prcode(fp,
"    int sipRes;\n"
"\n"
            );

        generateCppCodeBlock(cd->getbufcode, fp);

        prcode(fp,
"\n"
"    return sipRes;\n"
"}\n"
"#endif\n"
            );
    }

    if (cd->releasebufcode != NULL)
    {
        prcode(fp,
"\n"
"\n"
"#if PY_MAJOR_VERSION >= 3\n"
            );

        if (!generating_c)
            prcode(fp,
"extern \"C\" {static void releasebuffer_%C(PyObject *, void *, Py_buffer *);}\n"
                , classFQCName(cd));

        prcode(fp,
"static void releasebuffer_%C(PyObject *%s, void *sipCppV, Py_buffer *)\n"
"{\n"
"    ", classFQCName(cd)
     , argName("sipSelf", cd->releasebufcode));

        generateClassFromVoid(cd, "sipCpp", "sipCppV", fp);

        prcode(fp, ";\n"
"\n"
            );

        generateCppCodeBlock(cd->releasebufcode, fp);

        prcode(fp,
"}\n"
"#endif\n"
            );
    }

    if (cd->readbufcode != NULL)
    {
        prcode(fp,
"\n"
"\n"
"#if PY_MAJOR_VERSION < 3\n"
            );

        if (!generating_c)
            prcode(fp,
"extern \"C\" {static SIP_SSIZE_T getreadbuffer_%C(PyObject *, void *, SIP_SSIZE_T, void **);}\n"
                , classFQCName(cd));

        prcode(fp,
"static SIP_SSIZE_T getreadbuffer_%C(PyObject *%s, void *sipCppV, SIP_SSIZE_T %s, void **%s)\n"
"{\n"
"    ", classFQCName(cd)
     , argName("sipSelf", cd->readbufcode)
     , argName("sipSegment", cd->readbufcode)
     , argName("sipPtrPtr", cd->readbufcode));

        generateClassFromVoid(cd, "sipCpp", "sipCppV", fp);

        prcode(fp, ";\n"
"    SIP_SSIZE_T sipRes;\n"
"\n"
            );

        generateCppCodeBlock(cd->readbufcode, fp);

        prcode(fp,
"\n"
"    return sipRes;\n"
"}\n"
"#endif\n"
            );
    }

    if (cd->writebufcode != NULL)
    {
        prcode(fp,
"\n"
"\n"
"#if PY_MAJOR_VERSION < 3\n"
            );

        if (!generating_c)
            prcode(fp,
"extern \"C\" {static SIP_SSIZE_T getwritebuffer_%C(PyObject *, void *, SIP_SSIZE_T, void **);}\n"
                , classFQCName(cd));

        prcode(fp,
"static SIP_SSIZE_T getwritebuffer_%C(PyObject *%s, void *sipCppV, SIP_SSIZE_T %s, void **%s)\n"
"{\n"
"    ", classFQCName(cd)
     , argName("sipSelf", cd->writebufcode)
     , argName("sipSegment", cd->writebufcode)
     , argName("sipPtrPtr", cd->writebufcode));

        generateClassFromVoid(cd, "sipCpp", "sipCppV", fp);

        prcode(fp, ";\n"
"    SIP_SSIZE_T sipRes;\n"
"\n"
            );

        generateCppCodeBlock(cd->writebufcode, fp);

        prcode(fp,
"\n"
"    return sipRes;\n"
"}\n"
"#endif\n"
            );
    }

    if (cd->segcountcode != NULL)
    {
        prcode(fp,
"\n"
"\n"
"#if PY_MAJOR_VERSION < 3\n"
            );

        if (!generating_c)
            prcode(fp,
"extern \"C\" {static SIP_SSIZE_T getsegcount_%C(PyObject *, void *, SIP_SSIZE_T *);}\n"
                , classFQCName(cd));

        prcode(fp,
"static SIP_SSIZE_T getsegcount_%C(PyObject *%s, void *sipCppV, SIP_SSIZE_T *%s)\n"
"{\n"
"    ", classFQCName(cd)
     , argName("sipSelf", cd->segcountcode)
     , argName("sipLenPtr", cd->segcountcode));

        generateClassFromVoid(cd, "sipCpp", "sipCppV", fp);

        prcode(fp, ";\n"
"    SIP_SSIZE_T sipRes;\n"
"\n"
            );

        generateCppCodeBlock(cd->segcountcode, fp);

        prcode(fp,
"\n"
"    return sipRes;\n"
"}\n"
"#endif\n"
            );
    }

    if (cd->charbufcode != NULL)
    {
        prcode(fp,
"\n"
"\n"
"#if PY_MAJOR_VERSION < 3\n"
            );

        if (!generating_c)
            prcode(fp,
"extern \"C\" {static SIP_SSIZE_T getcharbuffer_%C(PyObject *, void *, SIP_SSIZE_T, void **);}\n"
                , classFQCName(cd));

        prcode(fp,
"static SIP_SSIZE_T getcharbuffer_%C(PyObject *%s, void *sipCppV, SIP_SSIZE_T %s, void **%s)\n"
"{\n"
"    ", classFQCName(cd)
     , argName("sipSelf", cd->charbufcode)
     , argName("sipSegment", cd->charbufcode)
     , argName("sipPtrPtr", cd->charbufcode));

        generateClassFromVoid(cd, "sipCpp", "sipCppV", fp);

        prcode(fp, ";\n"
"    SIP_SSIZE_T sipRes;\n"
"\n"
            );

        generateCppCodeBlock(cd->charbufcode, fp);

        prcode(fp,
"\n"
"    return sipRes;\n"
"}\n"
"#endif\n"
            );
    }

    /* The pickle function. */
    if (cd->picklecode != NULL)
    {
        prcode(fp,
"\n"
"\n"
            );

        if (!generating_c)
            prcode(fp,
"extern \"C\" {static PyObject *pickle_%C(void *);}\n"
                , classFQCName(cd));

        prcode(fp,
"static PyObject *pickle_%C(void *sipCppV)\n"
"{\n"
"    ", classFQCName(cd));

        generateClassFromVoid(cd, "sipCpp", "sipCppV", fp);

        prcode(fp, ";\n"
"    PyObject *sipRes;\n"
"\n"
            );

        generateCppCodeBlock(cd->picklecode, fp);

        prcode(fp,
"\n"
"    return sipRes;\n"
"}\n"
            );
    }

    /* The finalisation function. */
    if (cd->finalcode != NULL)
    {
        int need_cpp = usedInCode(cd->finalcode, "sipCpp");

        prcode(fp,
"\n"
"\n"
            );

        if (!generating_c)
            prcode(fp,
"extern \"C\" {static int final_%C(PyObject *, void *, PyObject *, PyObject **);}\n"
                , classFQCName(cd));

        prcode(fp,
"static int final_%C(PyObject *%s, void *%s, PyObject *%s, PyObject **%s)\n"
"{\n"
            , classFQCName(cd), (usedInCode(cd->finalcode, "sipSelf") ? "sipSelf" : ""), (need_cpp ? "sipCppV" : ""), (usedInCode(cd->finalcode, "sipKwds") ? "sipKwds" : ""), (usedInCode(cd->finalcode, "sipUnused") ? "sipUnused" : ""));

        if (need_cpp)
        {
            prcode(fp,
"    ");
            generateClassFromVoid(cd, "sipCpp", "sipCppV", fp);
            prcode(fp, ";\n"
"\n"
                );
        }

        generateCppCodeBlock(cd->finalcode, fp);

        prcode(fp,
"}\n"
            );
    }

    /* The mixin initialisation function. */
    if (isMixin(cd))
    {
        prcode(fp,
"\n"
"\n"
            );

        if (!generating_c)
            prcode(fp,
"extern \"C\" {static int mixin_%C(PyObject *, PyObject *, PyObject *);}\n"
                , classFQCName(cd));

        prcode(fp,
"static int mixin_%C(PyObject *sipSelf, PyObject *sipArgs, PyObject *sipKwds)\n"
"{\n"
"    return sipInitMixin(sipSelf, sipArgs, sipKwds, (sipClassTypeDef *)&"
            , classFQCName(cd));

        generateTypeDefName(cd->iff, fp);

        prcode(fp, ");\n"
"}\n"
            );
    }

    if (generating_c || assignmentHelper(cd))
    {
        /* The assignment helper. */
        prcode(fp,
"\n"
"\n"
            );

        if (!generating_c)
            prcode(fp,
"extern \"C\" {static void assign_%L(void *, SIP_SSIZE_T, const void *);}\n"
                , cd->iff);

        prcode(fp,
"static void assign_%L(void *sipDst, SIP_SSIZE_T sipDstIdx, const void *sipSrc)\n"
"{\n"
            , cd->iff);

        if (generating_c)
            prcode(fp,
"    ((struct %S *)sipDst)[sipDstIdx] = *((const struct %S *)sipSrc);\n"
                , classFQCName(cd), classFQCName(cd));
        else
            prcode(fp,
"    reinterpret_cast<%S *>(sipDst)[sipDstIdx] = *reinterpret_cast<const %S *>(sipSrc);\n"
                , classFQCName(cd), classFQCName(cd));

        prcode(fp,
"}\n"
            );

        /* The array allocation helper. */
        prcode(fp,
"\n"
"\n"
            );

        if (!generating_c)
            prcode(fp,
"extern \"C\" {static void *array_%L(SIP_SSIZE_T);}\n"
                , cd->iff);

        prcode(fp,
"static void *array_%L(SIP_SSIZE_T sipNrElem)\n"
"{\n"
            , cd->iff);

        if (generating_c)
            prcode(fp,
"    return sipMalloc(sizeof (struct %S) * sipNrElem);\n"
                , classFQCName(cd));
        else
            prcode(fp,
"    return new %S[sipNrElem];\n"
                , classFQCName(cd));

        prcode(fp,
"}\n"
            );

        /* The copy helper. */
        prcode(fp,
"\n"
"\n"
            );

        if (!generating_c)
            prcode(fp,
"extern \"C\" {static void *copy_%L(const void *, SIP_SSIZE_T);}\n"
                , cd->iff);

        prcode(fp,
"static void *copy_%L(const void *sipSrc, SIP_SSIZE_T sipSrcIdx)\n"
"{\n"
            , cd->iff);

        if (generating_c)
            prcode(fp,
"    struct %S *sipPtr = sipMalloc(sizeof (struct %S));\n"
"    *sipPtr = ((const struct %S *)sipSrc)[sipSrcIdx];\n"
"\n"
"    return sipPtr;\n"
                , classFQCName(cd), classFQCName(cd)
                , classFQCName(cd));
        else
            prcode(fp,
"    return new %S(reinterpret_cast<const %S *>(sipSrc)[sipSrcIdx]);\n"
                , classFQCName(cd), classFQCName(cd));

        prcode(fp,
"}\n"
            );
    }

    /* The dealloc function. */
    if (needDealloc(cd))
    {
        prcode(fp,
"\n"
"\n"
            );

        if (!generating_c)
            prcode(fp,
"extern \"C\" {static void dealloc_%L(sipSimpleWrapper *);}\n"
                , cd->iff);

        prcode(fp,
"static void dealloc_%L(sipSimpleWrapper *sipSelf)\n"
"{\n"
            , cd->iff);

        if (tracing)
            prcode(fp,
"    sipTrace(SIP_TRACE_DEALLOCS,\"dealloc_%L()\\n\");\n"
"\n"
                , cd->iff);

        /* Disable the virtual handlers. */
        if (hasShadow(cd))
            prcode(fp,
"    if (sipIsDerived(sipSelf))\n"
"        reinterpret_cast<sip%C *>(sipGetAddress(sipSelf))->sipPySelf = NULL;\n"
"\n"
                ,classFQCName(cd));

        if (generating_c || isPublicDtor(cd) || (hasShadow(cd) && isProtectedDtor(cd)))
        {
            prcode(fp,
"    if (sipIsPyOwned(sipSelf))\n"
"    {\n"
                );

            if (isDelayedDtor(cd))
            {
                prcode(fp,
"        sipAddDelayedDtor(sipSelf);\n"
                    );
            }
            else if (generating_c)
            {
                if (cd->dealloccode != NULL)
                    generateCppCodeBlock(cd->dealloccode, fp);

                prcode(fp,
"        sipFree(sipGetAddress(sipSelf));\n"
                    );
            }
            else
            {
                prcode(fp,
"        release_%L(sipGetAddress(sipSelf),%s);\n"
                    , cd->iff, (hasShadow(cd) ? "sipSelf->flags" : "0"));
            }

            prcode(fp,
"    }\n"
                );
        }

        prcode(fp,
"}\n"
            );
    }

    /* The type initialisation function. */
    if (canCreate(cd))
        generateTypeInit(cd, mod, fp);
}


/*
 * Generate the shadow (derived) class code.
 */
static void generateShadowCode(sipSpec *pt, moduleDef *mod, classDef *cd,
        FILE *fp)
{
    int nrVirts, virtNr;
    virtOverDef *vod;
    ctorDef *ct;

    nrVirts = countVirtuals(cd);

    /* Generate the wrapper class constructors. */

    for (ct = cd->ctors; ct != NULL; ct = ct->next)
    {
        ctorDef *dct;

        if (isPrivateCtor(ct))
            continue;

        if (ct->cppsig == NULL)
            continue;

        /* Check we haven't already handled this C++ signature. */
        for (dct = cd->ctors; dct != ct; dct = dct->next)
            if (dct->cppsig != NULL && sameSignature(dct->cppsig, ct->cppsig, TRUE))
                break;

        if (dct != ct)
            continue;

        prcode(fp,
"\n"
"sip%C::sip%C(",classFQCName(cd),classFQCName(cd));

        generateCalledArgs(mod, cd->iff, ct->cppsig, Definition, TRUE, fp);

        prcode(fp,")%X: %S(",ct->exceptions,classFQCName(cd));

        generateProtectedCallArgs(mod, ct->cppsig, fp);

        prcode(fp,"), sipPySelf(0)\n"
"{\n"
            );

        if (tracing)
        {
            prcode(fp,
"    sipTrace(SIP_TRACE_CTORS,\"sip%C::sip%C(",classFQCName(cd),classFQCName(cd));
            generateCalledArgs(NULL, cd->iff, ct->cppsig, Declaration, TRUE, fp);
            prcode(fp,")%X (this=0x%%08x)\\n\",this);\n"
"\n"
                ,ct->exceptions);
        }

        if (nrVirts > 0)
            prcode(fp,
"    memset(sipPyMethods, 0, sizeof (sipPyMethods));\n"
                );

        prcode(fp,
"}\n"
            );
    }

    /* The destructor. */

    if (!isPrivateDtor(cd))
    {
        prcode(fp,
"\n"
"sip%C::~sip%C()%X\n"
"{\n"
            ,classFQCName(cd),classFQCName(cd),cd->dtorexceptions);

        if (tracing)
            prcode(fp,
"    sipTrace(SIP_TRACE_DTORS,\"sip%C::~sip%C()%X (this=0x%%08x)\\n\",this);\n"
"\n"
                ,classFQCName(cd),classFQCName(cd),cd->dtorexceptions);

        if (cd->dtorcode != NULL)
            generateCppCodeBlock(cd->dtorcode,fp);

        prcode(fp,
"    sipCommonDtor(sipPySelf);\n"
"}\n"
            );
    }

    /* The meta methods if required. */
    if ((pluginPyQt4(pt) || pluginPyQt5(pt)) && isQObjectSubClass(cd))
    {
        if (!noPyQt4QMetaObject(cd))
        {
            prcode(fp,
"\n"
"const QMetaObject *sip%C::metaObject() const\n"
"{\n"
                , classFQCName(cd));

            if (pluginPyQt5(pt))
                prcode(fp,
"    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : sip_%s_qt_metaobject(sipPySelf,sipType_%C);\n"
                    , mod->name, classFQCName(cd));
            else
                prcode(fp,
"    return sip_%s_qt_metaobject(sipPySelf,sipType_%C);\n"
                    , mod->name, classFQCName(cd));

            prcode(fp,
"}\n"
                );
        }

        prcode(fp,
"\n"
"int sip%C::qt_metacall(QMetaObject::Call _c,int _id,void **_a)\n"
"{\n"
"    _id = %S::qt_metacall(_c,_id,_a);\n"
"\n"
"    if (_id >= 0)\n"
"        _id = sip_%s_qt_metacall(sipPySelf,sipType_%C,_c,_id,_a);\n"
"\n"
"    return _id;\n"
"}\n"
"\n"
"void *sip%C::qt_metacast(const char *_clname)\n"
"{\n"
            , classFQCName(cd)
            , classFQCName(cd)
            , mod->name, classFQCName(cd)
            , classFQCName(cd));

        if (pluginPyQt5(pt))
            prcode(fp,
"    void *sipCpp;\n"
"\n"
"    return (sip_%s_qt_metacast(sipPySelf, sipType_%C, _clname, &sipCpp) ? sipCpp : %S::qt_metacast(_clname));\n"
                , mod->name, classFQCName(cd), classFQCName(cd));
        else
            prcode(fp,
"    return (sip_%s_qt_metacast(sipPySelf, sipType_%C, _clname)) ? this : %S::qt_metacast(_clname);\n"
                , mod->name, classFQCName(cd), classFQCName(cd));

        prcode(fp,
"}\n"
                );
    }

    /* Generate the virtual catchers. */
 
    virtNr = 0;
 
    for (vod = cd->vmembers; vod != NULL; vod = vod->next)
    {
        overDef *od = &vod->o;
        virtOverDef *dvod;

        if (isPrivate(od))
            continue;

        /*
         * Check we haven't already handled this C++ signature.  The same C++
         * signature should only appear more than once for overloads that are
         * enabled for different APIs and that differ in their /In/ and/or
         * /Out/ annotations.
         */
        for (dvod = cd->vmembers; dvod != vod; dvod = dvod->next)
            if (strcmp(dvod->o.cppname, od->cppname) == 0 && sameSignature(dvod->o.cppsig, od->cppsig, TRUE))
                break;

        if (dvod == vod)
            generateVirtualCatcher(pt, mod, cd, virtNr++, vod, fp);
    }

    /* Generate the wrapper around each protected member function. */
    generateProtectedDefinitions(mod, cd, fp);

    /* Generate the emitters if needed. */
    if (pluginPyQt3(pt))
        generateEmitters(mod, cd, fp);
}


/*
 * Generate the emitter functions.
 */
static void generateEmitters(moduleDef *mod, classDef *cd, FILE *fp)
{
    int noIntro;
    visibleList *vl;

    for (vl = cd->visible; vl != NULL; vl = vl->next)
    {
        overDef *od;

        for (od = vl->cd->overs; od != NULL; od = od->next)
            if (od->common == vl->m && isSignal(od))
            {
                generateEmitter(mod, cd, vl, fp);
                break;
            }
    }

    /* Generate the table of signals to support fan-outs. */

    noIntro = TRUE;

    for (vl = cd->visible; vl != NULL; vl = vl->next)
    {
        overDef *od;

        for (od = vl->cd->overs; od != NULL; od = od->next)
            if (od->common == vl->m && isSignal(od))
            {
                if (noIntro)
                {
                    setHasSigSlots(cd);

                    prcode(fp,
"\n"
"static pyqt3QtSignal signals_%C[] = {\n"
                        ,classFQCName(cd));

                    noIntro = FALSE;
                }

                prcode(fp,
"    {%N, %C_emit_%s},\n"
                    ,vl->m->pyname,classFQCName(cd),vl->m->pyname->text);

                break;
            }
    }

    if (!noIntro)
        prcode(fp,
"    {NULL, NULL}\n"
"};\n"
            );
}


/*
 * Generate the protected enums for a class.
 */
static void generateProtectedEnums(sipSpec *pt,classDef *cd,FILE *fp)
{
    enumDef *ed;

    for (ed = pt->enums; ed != NULL; ed = ed->next)
    {
        char *eol;
        mroDef *mro;
        enumMemberDef *emd;

        if (!isProtectedEnum(ed))
            continue;

        /* See if the class defining the enum is in our class hierachy. */
        for (mro = cd->mro; mro != NULL; mro = mro->next)
            if (mro->cd == ed->ecd)
                break;

        if (mro == NULL)
            continue;

        prcode(fp,
"\n"
"    /* Expose this protected enum. */\n"
"    enum");

        if (ed->fqcname != NULL)
            prcode(fp," sip%s",scopedNameTail(ed->fqcname));

        prcode(fp," {");

        eol = "\n";

        for (emd = ed->members; emd != NULL; emd = emd->next)
        {
            prcode(fp,"%s"
"        %s = %S::%s",eol,emd->cname,classFQCName(ed->ecd),emd->cname);

            eol = ",\n";
        }

        prcode(fp,"\n"
"    };\n"
            );
    }
}


/*
 * Generate the catcher for a virtual function.
 */
static void generateVirtualCatcher(sipSpec *pt, moduleDef *mod, classDef *cd,
        int virtNr, virtOverDef *vod, FILE *fp)
{
    overDef *od = &vod->o;
    argDef *res;
    apiVersionRangeDef *avr;

    normaliseArgs(od->cppsig);

    res = &od->cppsig->result;

    if (res->atype == void_type && res->nrderefs == 0)
        res = NULL;

    prcode(fp,
"\n");

    generateBaseType(cd->iff, &od->cppsig->result, TRUE, fp);

    prcode(fp," sip%C::%O(",classFQCName(cd),od);
    generateCalledArgs(mod, cd->iff, od->cppsig, Definition, TRUE, fp);
    prcode(fp,")%s%X\n"
"{\n"
        ,(isConst(od) ? " const" : ""),od->exceptions);

    if (tracing)
    {
        prcode(fp,
"    sipTrace(SIP_TRACE_CATCHERS,\"");

        generateBaseType(cd->iff, &od->cppsig->result, TRUE, fp);
        prcode(fp," sip%C::%O(",classFQCName(cd),od);
        generateCalledArgs(NULL, cd->iff, od->cppsig, Declaration, TRUE, fp);
        prcode(fp,")%s%X (this=0x%%08x)\\n\",this);\n"
"\n"
            ,(isConst(od) ? " const" : ""),od->exceptions);
    }

    restoreArgs(od->cppsig);

    prcode(fp,
"    sip_gilstate_t sipGILState;\n"
"    PyObject *sipMeth;\n"
"\n"
"    sipMeth = sipIsPyMethod(&sipGILState,");

    if (isConst(od))
        prcode(fp, "const_cast<char *>(");

    prcode(fp,"&sipPyMethods[%d]",virtNr);

    if (isConst(od))
        prcode(fp,")");

    prcode(fp,",sipPySelf,");

    if (isAbstract(od))
        prcode(fp, "%N", cd->pyname);
    else
        prcode(fp,"NULL");

    prcode(fp,",%N);\n"
"\n"
        ,od->common->pyname);

    prcode(fp,
"    if (!sipMeth)\n"
        );

    if (isAbstract(od))
        generateDefaultInstanceReturn(res, "    ", fp);
    else
    {
        int a;

        if (res == NULL)
            prcode(fp,
"    {\n"
"        ");
        else
            prcode(fp,
"        return ");

        prcode(fp, "%S::%O(", classFQCName(cd), od);
 
        for (a = 0; a < od->cppsig->nrArgs; ++a)
        {
            argDef *ad = &od->cppsig->args[a];

            prcode(fp, "%s%a", (a == 0 ? "" : ","), mod, ad, a);
        }
 
        prcode(fp,");\n"
            );

        if (res == NULL)
        {
            /*
             * Note that we should also generate this if the function returns a
             * value, but we are lazy and this is all that is needed by PyQt.
             */
            if (isNewThread(od))
                prcode(fp,
"        sipEndThread();\n"
                    );

            prcode(fp,
"        return;\n"
"    }\n"
                );
        }
    }

    /*
     * If this overload doesn't have an API version assume that there are none
     * that do.
     */
    avr = od->api_range;

    if (avr == NULL)
    {
        prcode(fp,
"\n"
            );

        generateVirtHandlerCall(pt, mod, cd, vod, res, "    ", fp);
    }
    else
    {
        virtOverDef *versioned_vod = vod;

        do
        {
            prcode(fp,
"\n"
"    if (sipIsAPIEnabled(%N, %d, %d))\n"
"    {\n"
                , avr->api_name, avr->from, avr->to);

            generateVirtHandlerCall(pt, mod, cd, versioned_vod, res, "        ", fp);

            if (res == NULL)
                prcode(fp,
"        return;\n"
                    );

            prcode(fp,
"    }\n"
                );

            /* Find the next overload. */
            while ((versioned_vod = versioned_vod->next) != NULL)
            {
                if (strcmp(versioned_vod->o.cppname, od->cppname) == 0 && sameSignature(versioned_vod->o.cppsig, od->cppsig, TRUE))
                {
                    avr = versioned_vod->o.api_range;

                    /* Check that it has an API specified. */
                    if (avr == NULL)
                    {
                        fatalScopedName(classFQCName(cd));
                        fatal("::");
                        prOverloadName(stderr, od);
                        fatal(" has versioned and unversioned overloads\n");
                    }

                    break;
                }
            }
        }
        while (versioned_vod != NULL);

        prcode(fp,
"\n"
            );

        /* Generate a default result in case no API is enabled. */
        if (isAbstract(od))
            generateDefaultInstanceReturn(res, "", fp);
        else
        {
            int a;

            prcode(fp, "    %s%S::%O(", (res != NULL ? "return " : ""), classFQCName(cd), od);
 
            for (a = 0; a < od->cppsig->nrArgs; ++a)
            {
                argDef *ad = &od->cppsig->args[a];

                prcode(fp, "%s%a", (a == 0 ? "" : ","), mod, ad, a);
            }
 
            prcode(fp,");\n"
                );
        }
    }

    prcode(fp,
"}\n"
        );
}


/*
 * Generate a call to a single virtual handler.
 */
static void generateVirtHandlerCall(sipSpec *pt, moduleDef *mod, classDef *cd,
        virtOverDef *vod, argDef *res, const char *indent, FILE *fp)
{
    overDef *od = &vod->o;
    virtHandlerDef *vhd = od->virthandler;
    virtErrorHandler *veh;
    signatureDef saved;
    argDef *ad;
    int a, args_keep = FALSE, result_keep = FALSE;

    veh = getVirtErrorHandler(pt, od, cd, mod);

    saved = *vhd->cppsig;
    fakeProtectedArgs(vhd->cppsig);

    if (vhd->module == mod)
    {
        prcode(fp,
"%sextern ", indent);

        generateBaseType(cd->iff, &od->cppsig->result, FALSE, fp);

        prcode(fp, " sipVH_%s_%d(sip_gilstate_t, sipVirtErrorHandlerFunc, sipSimpleWrapper *, PyObject *", vhd->module->name, vhd->virthandlernr);
    }
    else
    {
        prcode(fp,
"%stypedef ", indent);

        generateBaseType(cd->iff, &od->cppsig->result, FALSE, fp);

        prcode(fp, " (*sipVH_%s_%d)(sip_gilstate_t, sipVirtErrorHandlerFunc, sipSimpleWrapper *, PyObject *", vhd->module->name, vhd->virthandlernr);
    }

    if (vhd->cppsig->nrArgs > 0)
    {
        prcode(fp, ", ");
        generateCalledArgs(NULL, cd->iff, vhd->cppsig, Declaration, FALSE, fp);
    }

    *vhd->cppsig = saved;

    /* Add extra arguments for all the references we need to keep. */
    if (res != NULL && keepPyReference(res))
    {
        result_keep = TRUE;
        res->key = mod->next_key--;
        prcode(fp, ", int");
    }

    for (ad = od->cppsig->args, a = 0; a < od->cppsig->nrArgs; ++a, ++ad)
        if (isOutArg(ad) && keepPyReference(ad))
        {
            args_keep = TRUE;
            ad->key = mod->next_key--;
            prcode(fp, ", int");
        }

    prcode(fp,");\n"
        );

    if (veh != NULL && veh->mod == mod)
        prcode(fp,
"%sextern void sipVEH_%s_%s(sipSimpleWrapper *, sip_gilstate_t);\n"
            , indent, mod->name, veh->name);

    prcode(fp,
"\n"
"%s", indent);

    if (!isNewThread(od) && res != NULL)
        prcode(fp, "return ");

    if (vhd->module == mod)
        prcode(fp, "sipVH_%s_%d", vhd->module->name,vhd->virthandlernr);
    else
        prcode(fp, "((sipVH_%s_%d)(sipModuleAPI_%s_%s->em_virthandlers[%d]))", vhd->module->name, vhd->virthandlernr, mod->name, vhd->module->name, vhd->virthandlernr);

    prcode(fp, "(sipGILState, ");

    if (veh == NULL)
        prcode(fp, "0");
    else if (veh->mod == mod)
        prcode(fp, "sipVEH_%s_%s" , mod->name, veh->name);
    else
        prcode(fp, "sipModuleAPI_%s_%s->em_virterrorhandlers[%d]", mod->name, veh->mod->name, veh->index);

    prcode(fp, ", sipPySelf, sipMeth");

    for (ad = od->cppsig->args, a = 0; a < od->cppsig->nrArgs; ++a, ++ad)
    {
        if (ad->atype == class_type && isProtectedClass(ad->u.cd))
            prcode(fp, ", %s%a", ((isReference(ad) || ad->nrderefs == 0) ? "&" : ""), mod, ad, a);
        else if (ad->atype == enum_type && isProtectedEnum(ad->u.ed))
            prcode(fp, ", (%E)%a", ad->u.ed, mod, ad, a);
        else
            prcode(fp, ", %a", mod, ad, a);
    }

    /* Pass the keys to maintain the kept references. */
    if (result_keep)
        prcode(fp, ", %d", res->key);

    if (args_keep)
        for (ad = od->cppsig->args, a = 0; a < od->cppsig->nrArgs; ++a, ++ad)
            if (isOutArg(ad) && keepPyReference(ad))
                prcode(fp, ", %d", ad->key);

    prcode(fp,");\n"
        );

    if (isNewThread(od))
        prcode(fp,
"\n"
"%ssipEndThread();\n"
            , indent);
}


/*
 * Get the virtual error handler for a function.
 */
static virtErrorHandler *getVirtErrorHandler(sipSpec *pt, overDef *od,
        classDef *cd, moduleDef *mod)
{
    const char *name;
    virtErrorHandler *veh;
    
    /* Handle the trivial case. */
    if (noErrorHandler(od))
        return NULL;

    /* Check the function itself. */
    if ((name = od->virt_error_handler) == NULL)
    {
        mroDef *mro;

        /* Check the class hierarchy. */
        for (mro = cd->mro; mro != NULL; mro = mro->next)
            if ((name = mro->cd->virt_error_handler) != NULL)
                break;

        if (name == NULL)
        {
            /* Check the module. */
            if ((name = mod->virt_error_handler) == NULL)
            {
                moduleListDef *mld;

                /* Check the module hierarchy. */
                for (mld = mod->allimports; mld != NULL; mld = mld->next)
                    if ((name = mld->module->virt_error_handler) != NULL)
                        break;
            }
        }
    }

    if (name == NULL)
        return NULL;

    /* Find the handler with the name. */
    for (veh = pt->errorhandlers; veh != NULL; veh = veh->next)
        if (strcmp(veh->name, name) == 0)
            break;

    if (veh == NULL)
        fatal("Unknown virtual error handler \"%s\"\n", name);

    return veh;
}


/*
 * Generate a cast to zero.
 */
static void generateCastZero(argDef *ad,FILE *fp)
{
    if (ad->atype == enum_type)
        prcode(fp,"(%E)",ad->u.ed);

    prcode(fp,"0");
}


/*
 * Generate a statement to return the default instance of a type typically on
 * error (ie. when there is nothing sensible to return).
 */
static void generateDefaultInstanceReturn(argDef *res, const char *indent,
        FILE *fp)
{
    codeBlockList *instance_code;

    /* Handle the trivial case. */
    if (res == NULL)
    {
        prcode(fp,
"%s    return;\n"
            , indent);

        return;
    }

    /* Handle any %InstanceCode. */
    instance_code = NULL;

    if (res->nrderefs == 0)
    {
        if (res->atype == mapped_type)
            instance_code = res->u.mtd->instancecode;
        else if (res->atype == class_type)
            instance_code = res->u.cd->instancecode;
    }

    if (instance_code != NULL)
    {
        argDef res_noconstref;

        res_noconstref = *res;
        resetIsConstArg(&res_noconstref);
        resetIsReference(&res_noconstref);

        prcode(fp,
"%s{\n"
"%s    static %B *sipCpp = 0;\n"
"\n"
"%s    if (!sipCpp)\n"
"%s    {\n"
            , indent
            , indent, &res_noconstref
            , indent
            , indent);

        generateCppCodeBlock(instance_code, fp);

        prcode(fp,
"%s    }\n"
"\n"
"%s    return *sipCpp;\n"
"%s}\n"
            , indent
            , indent
            , indent);

        return;
    }

    prcode(fp,
"%s    return ", indent);

    if (res->atype == mapped_type && res->nrderefs == 0)
    {
        argDef res_noconstref;

        /*
         * We don't know anything about the mapped type so we just hope is has
         * a default ctor.
         */

        if (isReference(res))
            prcode(fp,"*new ");

        res_noconstref = *res;
        resetIsConstArg(&res_noconstref);
        resetIsReference(&res_noconstref);
        prcode(fp,"%B()",&res_noconstref);
    }
    else if (res->atype == class_type && res->nrderefs == 0)
    {
        ctorDef *ct = res->u.cd->defctor;

        /*
         * If we don't have a suitable ctor then the generated code will issue
         * an error message.
         */
        if (ct != NULL && isPublicCtor(ct) && ct->cppsig != NULL)
        {
            argDef res_noconstref;

            /*
             * If this is a badly designed class.  We can only generate correct
             * code by leaking memory.
             */
            if (isReference(res))
                prcode(fp,"*new ");

            res_noconstref = *res;
            resetIsConstArg(&res_noconstref);
            resetIsReference(&res_noconstref);
            prcode(fp,"%B",&res_noconstref);

            generateCallDefaultCtor(ct,fp);
        }
        else
        {
            fatalScopedName(classFQCName(res->u.cd));
            fatal(" must have a default constructor\n");
        }
    }
    else
        generateCastZero(res,fp);

    prcode(fp,";\n"
        );
}


/*
 * Generate the call to a default ctor.
 */
static void generateCallDefaultCtor(ctorDef *ct, FILE *fp)
{
    int a;

    prcode(fp, "(");

    for (a = 0; a < ct->cppsig->nrArgs; ++a)
    {
        argDef *ad = &ct->cppsig->args[a];
        argType atype = ad->atype;

        if (ad->defval != NULL)
            break;

        if (a > 0)
            prcode(fp, ",");

        /* Do what we can to provide type information to the compiler. */
        if (atype == class_type && ad->nrderefs > 0 && !isReference(ad))
            prcode(fp, "static_cast<%B>(0)", ad);
        else if (atype == enum_type)
            prcode(fp, "static_cast<%E>(0)", ad->u.ed);
        else if (atype == float_type || atype == cfloat_type)
            prcode(fp, "0.0F");
        else if (atype == double_type || atype == cdouble_type)
            prcode(fp, "0.0");
        else if (atype == uint_type)
            prcode(fp, "0U");
        else if (atype == long_type || atype == longlong_type)
            prcode(fp, "0L");
        else if (atype == ulong_type || atype == ulonglong_type)
            prcode(fp, "0UL");
        else if ((atype == ascii_string_type || atype == latin1_string_type || atype == utf8_string_type || atype == ustring_type || atype == sstring_type || atype == string_type) && ad->nrderefs == 0)
            prcode(fp, "'\\0'");
        else if (atype == wstring_type && ad->nrderefs == 0)
            prcode(fp, "L'\\0'");
        else
            prcode(fp, "0");
    }

    prcode(fp, ")");
}


/*
 * Generate the emitter function for a signal.
 */
static void generateEmitter(moduleDef *mod, classDef *cd, visibleList *vl,
        FILE *fp)
{
    const char *pname = vl->m->pyname->text;
    overDef *od;

    prcode(fp,
"\n"
"int sip%C::sipEmit_%s(PyObject *sipArgs)\n"
"{\n"
"    PyObject *sipParseErr = NULL;\n"
        ,classFQCName(cd),pname);

    for (od = vl->cd->overs; od != NULL; od = od->next)
    {
        int rgil = ((release_gil || isReleaseGIL(od)) && !isHoldGIL(od));

        if (od->common != vl->m || !isSignal(od))
            continue;

        /*
         * Generate the code that parses the args and emits the appropriate
         * overloaded signal.
         */
        prcode(fp,
"\n"
"    {\n"
            );

        generateArgParser(mod, &od->pysig, cd, NULL, NULL, NULL, FALSE, fp);

        prcode(fp,
"        {\n"
            );

        if (rgil)
            prcode(fp,
"            Py_BEGIN_ALLOW_THREADS\n"
                );

        prcode(fp,
"            emit %s("
            ,od->cppname);

        generateCallArgs(mod, od->cppsig, &od->pysig, fp);

        prcode(fp,");\n"
            );

        if (rgil)
            prcode(fp,
"            Py_END_ALLOW_THREADS\n"
                );

        deleteTemps(mod, &od->pysig, fp);

        prcode(fp,
"\n"
"            return 0;\n"
"        }\n"
"    }\n"
            );
    }

    prcode(fp,
"\n"
"    sipNoMethod(sipParseErr, %N, %N, NULL);\n"
"\n"
"    return -1;\n"
"}\n"
"\n"
        , cd->pyname, vl->m->pyname);

    if (!generating_c)
        prcode(fp,
"extern \"C\" {static int %C_emit_%s(sipSimpleWrapper *, PyObject *);}\n"
            , classFQCName(cd), pname);

    prcode(fp,
"static int %C_emit_%s(sipSimpleWrapper *sw,PyObject *sipArgs)\n"
"{\n"
"    sip%C *ptr = reinterpret_cast<sip%C *>(sipGetComplexCppPtr(sw));\n"
"\n"
"    return (ptr ? ptr->sipEmit_%s(sipArgs) : -1);\n"
"}\n"
        ,classFQCName(cd),pname
        ,classFQCName(cd),classFQCName(cd)
        ,pname);
}


/*
 * Generate the declarations of the protected wrapper functions for a class.
 */
static void generateProtectedDeclarations(classDef *cd,FILE *fp)
{
    int noIntro;
    visibleList *vl;

    noIntro = TRUE;

    for (vl = cd->visible; vl != NULL; vl = vl->next)
    {
        overDef *od;

        if (vl->m->slot != no_slot)
            continue;

        for (od = vl->cd->overs; od != NULL; od = od->next)
        {
            if (od->common != vl->m || !isProtected(od))
                continue;

            /*
             * Check we haven't already handled this signature (eg. if we have
             * specified the same method with different Python names.
             */
            if (isDuplicateProtected(cd, od))
                continue;

            if (noIntro)
            {
                prcode(fp,
"\n"
"    /*\n"
"     * There is a public method for every protected method visible from\n"
"     * this class.\n"
"     */\n"
                    );

                noIntro = FALSE;
            }

            prcode(fp,
"    ");

            if (isStatic(od))
                prcode(fp,"static ");

            generateBaseType(cd->iff, &od->cppsig->result, TRUE, fp);

            if (!isStatic(od) && !isAbstract(od) && (isVirtual(od) || isVirtualReimp(od)))
            {
                prcode(fp, " sipProtectVirt_%s(bool", od->cppname);

                if (od->cppsig->nrArgs > 0)
                    prcode(fp, ",");
            }
            else
                prcode(fp, " sipProtect_%s(", od->cppname);

            generateCalledArgs(NULL, cd->iff, od->cppsig, Declaration, TRUE, fp);
            prcode(fp,")%s;\n"
                ,(isConst(od) ? " const" : ""));
        }
    }
}


/*
 * Generate the definitions of the protected wrapper functions for a class.
 */
static void generateProtectedDefinitions(moduleDef *mod, classDef *cd, FILE *fp)
{
    visibleList *vl;

    for (vl = cd->visible; vl != NULL; vl = vl->next)
    {
        overDef *od;

        if (vl->m->slot != no_slot)
            continue;

        for (od = vl->cd->overs; od != NULL; od = od->next)
        {
            char *mname = od->cppname;
            int parens;
            argDef *res;

            if (od->common != vl->m || !isProtected(od))
                continue;

            /*
             * Check we haven't already handled this signature (eg. if we have
             * specified the same method with different Python names.
             */
            if (isDuplicateProtected(cd, od))
                continue;

            prcode(fp,
"\n"
                );

            generateBaseType(cd->iff, &od->cppsig->result, TRUE, fp);

            if (!isStatic(od) && !isAbstract(od) && (isVirtual(od) || isVirtualReimp(od)))
            {
                prcode(fp, " sip%C::sipProtectVirt_%s(bool sipSelfWasArg", classFQCName(cd), mname);

                if (od->cppsig->nrArgs > 0)
                    prcode(fp, ",");
            }
            else
                prcode(fp, " sip%C::sipProtect_%s(", classFQCName(cd), mname);

            generateCalledArgs(mod, cd->iff, od->cppsig, Definition, TRUE, fp);
            prcode(fp,")%s\n"
"{\n"
                ,(isConst(od) ? " const" : ""));

            parens = 1;

            res = &od->cppsig->result;

            if (res->atype == void_type && res->nrderefs == 0)
                prcode(fp,
"    ");
            else
            {
                prcode(fp,
"    return ");

                if (res->atype == class_type && isProtectedClass(res->u.cd))
                {
                    prcode(fp,"static_cast<%U *>(",res->u.cd);
                    ++parens;
                }
                else if (res->atype == enum_type && isProtectedEnum(res->u.ed))
                    /*
                     * One or two older compilers can't handle a static_cast
                     * here so we revert to a C-style cast.
                     */
                    prcode(fp,"(%E)",res->u.ed);
            }

            if (!isAbstract(od))
            {
                if (isVirtual(od) || isVirtualReimp(od))
                {
                    prcode(fp, "(sipSelfWasArg ? %S::%s(", classFQCName(vl->cd), mname);

                    generateProtectedCallArgs(mod, od->cppsig, fp);

                    prcode(fp, ") : ");
                    ++parens;
                }
                else
                    prcode(fp, "%S::", classFQCName(vl->cd));
            }

            prcode(fp,"%s(",mname);

            generateProtectedCallArgs(mod, od->cppsig, fp);

            while (parens--)
                prcode(fp,")");

            prcode(fp,";\n"
"}\n"
                );
        }
    }
}


/*
 * Return TRUE if a protected method is a duplicate.
 */
static int isDuplicateProtected(classDef *cd, overDef *target)
{
    visibleList *vl;

    for (vl = cd->visible; vl != NULL; vl = vl->next)
    {
        overDef *od;

        if (vl->m->slot != no_slot)
            continue;

        for (od = vl->cd->overs; od != NULL; od = od->next)
        {
            if (od->common != vl->m || !isProtected(od))
                continue;

            if (od == target)
                return FALSE;

            if (strcmp(od->cppname, target->cppname) == 0 && sameSignature(od->cppsig, target->cppsig, TRUE))
                return TRUE;
        }
    }

    /* We should never actually get here. */
    return FALSE;
}


/*
 * Generate the arguments for a call to a protected method.
 */
static void generateProtectedCallArgs(moduleDef *mod, signatureDef *sd,
        FILE *fp)
{
    int a;

    for (a = 0; a < sd->nrArgs; ++a)
    {
        argDef *ad = &sd->args[a];

        if (a > 0)
            prcode(fp, ",");

        if (ad->atype == enum_type && isProtectedEnum(ad->u.ed))
            prcode(fp, "(%S)", ad->u.ed->fqcname);

        prcode(fp, "%a", mod, ad, a);
    }
}


/*
 * Generate the function that does most of the work to handle a particular
 * virtual function.
 */
static void generateVirtualHandler(moduleDef *mod, virtHandlerDef *vhd,
        FILE *fp)
{
    int a, nrvals, res_isref;
    argDef *res, res_noconstref, *ad;
    signatureDef saved;

    res = &vhd->cppsig->result;

    res_isref = FALSE;

    if (res->atype == void_type && res->nrderefs == 0)
        res = NULL;
    else
    {
        /*
         * If we are returning a reference to an instance then we take care to
         * handle Python errors but still return a valid C++ instance.
         */
        if ((res->atype == class_type || res->atype == mapped_type) && res->nrderefs == 0)
        {
            if (isReference(res))
                res_isref = TRUE;
        }

        res_noconstref = *res;
        resetIsConstArg(&res_noconstref);
        resetIsReference(&res_noconstref);
    }

    prcode(fp,
"\n"
        );

    saved = *vhd->cppsig;
    fakeProtectedArgs(vhd->cppsig);

    generateBaseType(NULL, &vhd->cppsig->result, FALSE, fp);

    prcode(fp," sipVH_%s_%d(sip_gilstate_t sipGILState, sipVirtErrorHandlerFunc sipErrorHandler, sipSimpleWrapper *sipPySelf, PyObject *sipMethod"
        , vhd->module->name, vhd->virthandlernr);

    if (vhd->cppsig->nrArgs > 0)
    {
        prcode(fp,", ");
        generateCalledArgs(mod, NULL, vhd->cppsig, Definition, FALSE, fp);
    }

    *vhd->cppsig = saved;

    /* Declare the extra arguments for kept references. */
    if (res != NULL && keepPyReference(res))
    {
        prcode(fp, ", int");

        if (vhd->virtcode == NULL || usedInCode(vhd->virtcode, "sipResKey"))
            prcode(fp, " sipResKey");
    }

    for (ad = vhd->cppsig->args, a = 0; a < vhd->cppsig->nrArgs; ++a, ++ad)
        if (isOutArg(ad) && keepPyReference(ad))
            prcode(fp, ", int %aKey", mod, ad, a);

    prcode(fp,")\n"
"{\n"
        );

    if (res != NULL)
    {
        prcode(fp, "    ");

        /*
         * wchar_t * return values are always on the heap.  To reduce memory
         * leaks we keep the last result around until we have a new one.  This
         * means that ownership of the return value stays with the function
         * returning it - which is consistent with how other types work, even
         * thought it may not be what's required in all cases.  Note that we
         * should do this in the code that calls the handler instead of here
         * (as we do with strings) so that it doesn't get shared between all
         * callers.
         */
        if (res->atype == wstring_type && res->nrderefs == 1)
            prcode(fp, "static ");

        generateBaseType(NULL, &res_noconstref, FALSE, fp);

        prcode(fp," %ssipRes",(res_isref ? "*" : ""));

        if ((res->atype == class_type || res->atype == mapped_type) && res->nrderefs == 0)
        {
            if (res->atype == class_type)
            {
                ctorDef *ct = res->u.cd->defctor;

                if (ct != NULL && isPublicCtor(ct) && ct->cppsig != NULL && ct->cppsig->nrArgs > 0 && ct->cppsig->args[0].defval == NULL)
                    generateCallDefaultCtor(ct,fp);
            }
        }
        else
        {
            /*
             * We initialise the result to try and suppress a compiler warning.
             */
            prcode(fp," = ");
            generateCastZero(res,fp);
        }

        prcode(fp,";\n"
            );

        if (res->atype == wstring_type && res->nrderefs == 1)
            prcode(fp,
"\n"
"    if (sipRes)\n"
"    {\n"
"        // Return any previous result to the heap.\n"
"        sipFree(%s);\n"
"        sipRes = 0;\n"
"    }\n"
"\n"
                , (isConstArg(res) ? "const_cast<wchar_t *>(sipRes)" : "sipRes"));
    }

    if (vhd->virtcode != NULL)
    {
        int error_flag = needErrorFlag(vhd->virtcode);
        int old_error_flag = needOldErrorFlag(vhd->virtcode);

        if (error_flag)
            prcode(fp,
"    sipErrorState sipError = sipErrorNone;\n"
                );
        else if (old_error_flag)
            prcode(fp,
"    int sipIsErr = 0;\n"
                );

        prcode(fp,
"\n"
            );

        generateCppCodeBlock(vhd->virtcode,fp);

        prcode(fp,
"\n"
"    Py_DECREF(sipMethod);\n"
            );

        if (error_flag || old_error_flag)
            prcode(fp,
"\n"
"    if (%s)\n"
"        sipCallErrorHandler(sipErrorHandler, sipPySelf, sipGILState);\n"
                , (error_flag ? "sipError != sipErrorNone" : "sipIsErr"));

        prcode(fp,
"\n"
"    SIP_RELEASE_GIL(sipGILState)\n"
            );

        if (res != NULL)
            prcode(fp,
"\n"
"    return sipRes;\n"
                );

        prcode(fp,
"}\n"
            );

        return;
    }

    /* See how many values we expect. */
    nrvals = (res != NULL ? 1 : 0);

    for (a = 0; a < vhd->pysig->nrArgs; ++a)
        if (isOutArg(&vhd->pysig->args[a]))
            ++nrvals;

    /* Call the method. */
    prcode(fp,
"    PyObject *sipResObj = sipCallMethod(0, sipMethod, ");

    saved = *vhd->pysig;
    fakeProtectedArgs(vhd->pysig);
    generateTupleBuilder(mod, vhd->pysig, fp);
    *vhd->pysig = saved;

    prcode(fp, ");\n"
"\n"
"    %ssipParseResultEx(sipGILState, sipErrorHandler, sipPySelf, sipMethod, sipResObj, \"", (res_isref ? "int sipRc = " : ""));

    /* Build the format string. */
    if (nrvals == 0)
        prcode(fp,"Z");
    else
    {
        if (nrvals > 1)
            prcode(fp,"(");

        if (res != NULL)
            prcode(fp, "%s", getParseResultFormat(res, res_isref, isTransferVH(vhd)));

        for (a = 0; a < vhd->pysig->nrArgs; ++a)
        {
            argDef *ad = &vhd->pysig->args[a];

            if (isOutArg(ad))
                prcode(fp, "%s", getParseResultFormat(ad, FALSE, FALSE));
        }

        if (nrvals > 1)
            prcode(fp,")");
    }

    prcode(fp,"\"");

    /* Pass the destination pointers. */
    if (res != NULL)
    {
        generateParseResultExtraArgs(NULL, res, -1, fp);
        prcode(fp, ", &sipRes");
    }

    for (a = 0; a < vhd->pysig->nrArgs; ++a)
    {
        argDef *ad = &vhd->pysig->args[a];

        if (isOutArg(ad))
        {
            generateParseResultExtraArgs(mod, ad, a, fp);
            prcode(fp, ", %s%a", (isReference(ad) ? "&" : ""), mod, ad, a);
        }
    }

    prcode(fp, ");\n"
        );

    if (res != NULL)
    {
        if (res_isref)
        {
            prcode(fp,
"\n"
"    if (sipRc < 0)\n"
                );

            generateDefaultInstanceReturn(res, "    ", fp);
        }

        prcode(fp,
"\n"
"    return %ssipRes;\n"
            , (res_isref ? "*" : ""));
    }

    prcode(fp,
"}\n"
        );
}


/*
 * Generate the extra arguments needed by sipParseResultEx() for a particular
 * type.
 */
static void generateParseResultExtraArgs(moduleDef *mod, argDef *ad, int argnr,
        FILE *fp)
{
    switch (ad->atype)
    {
    case mapped_type:
        prcode(fp, ", sipType_%T", ad);
        break;

    case class_type:
        prcode(fp, ", sipType_%C", classFQCName(ad->u.cd));
        break;

    case pytuple_type:
        prcode(fp,", &PyTuple_Type");
        break;

    case pylist_type:
        prcode(fp,", &PyList_Type");
        break;

    case pydict_type:
        prcode(fp,", &PyDict_Type");
        break;

    case pyslice_type:
        prcode(fp,", &PySlice_Type");
        break;

    case pytype_type:
        prcode(fp,", &PyType_Type");
        break;

    case enum_type:
        if (ad->u.ed->fqcname != NULL)
            prcode(fp, ", sipType_%C", ad->u.ed->fqcname);
        break;

    case capsule_type:
        prcode(fp,", \"%S\"", ad->u.cap);
        break;

    default:
        if (keepPyReference(ad))
        {
            if (argnr < 0)
                prcode(fp, ", sipResKey");
            else
                prcode(fp, ", %aKey", mod, ad, argnr);
        }
    }
}


/*
 * Return the format characters used by sipParseResultEx() for a particular
 * type.
 */
static const char *getParseResultFormat(argDef *ad, int res_isref, int xfervh)
{
    switch (ad->atype)
    {
    case mapped_type:
    case fake_void_type:
    case class_type:
        {
            static const char *type_formats[] = {
                "H0", "H1", "H2", "H3", "H4", "H5", "H6", "H7"
            };

            int f = 0x00;

            if (ad->nrderefs == 0)
            {
                f |= 0x01;

                if (!res_isref)
                    f |= 0x04;
            }

            if (xfervh)
                f |= 0x02;

            return type_formats[f];
        }

    case bool_type:
    case cbool_type:
        return "b";

    case ascii_string_type:
        return ((ad->nrderefs == 0) ? "aA" : "AA");

    case latin1_string_type:
        return ((ad->nrderefs == 0) ? "aL" : "AL");

    case utf8_string_type:
        return ((ad->nrderefs == 0) ? "a8" : "A8");

    case sstring_type:
    case ustring_type:
    case string_type:
        return ((ad->nrderefs == 0) ? "c" : "B");

    case wstring_type:
        return ((ad->nrderefs == 0) ? "w" : "x");

    case enum_type:
        return ((ad->u.ed->fqcname != NULL) ? "F" : "e");

    case byte_type:
    case sbyte_type:
        return "L";

    case ubyte_type:
        return "M";

    case ushort_type:
        return "t";

    case short_type:
        return "h";

    case int_type:
    case cint_type:
        return "i";

    case uint_type:
        return "u";

    case long_type:
        return "l";

    case ulong_type:
        return "m";

    case longlong_type:
        return "n";

    case ulonglong_type:
        return "o";

    case void_type:
    case struct_type:
        return "V";

    case capsule_type:
        return "z";

    case float_type:
    case cfloat_type:
        return "f";

    case double_type:
    case cdouble_type:
        return "d";

    case pyobject_type:
        return "O";

    case pytuple_type:
    case pylist_type:
    case pydict_type:
    case pyslice_type:
    case pytype_type:
        return (isAllowNone(ad) ? "N" : "T");

    case pybuffer_type:
        return (isAllowNone(ad) ? "$" : "!");
    }

    /* We should never get here. */
    return " ";
}


/*
 * Generate the code to build a tuple of Python arguments.
 */
static void generateTupleBuilder(moduleDef *mod, signatureDef *sd,FILE *fp)
{
    int a, arraylenarg;

    prcode(fp,"\"");

    for (a = 0; a < sd->nrArgs; ++a)
    {
        char *fmt = "";
        argDef *ad = &sd->args[a];

        if (!isInArg(ad))
            continue;

        switch (ad->atype)
        {
        case ascii_string_type:
            if (ad->nrderefs == 0 || (ad->nrderefs == 1 && isOutArg(ad)))
                fmt = "aA";
            else
                fmt = "AA";

            break;

        case latin1_string_type:
            if (ad->nrderefs == 0 || (ad->nrderefs == 1 && isOutArg(ad)))
                fmt = "aL";
            else
                fmt = "AL";

            break;

        case utf8_string_type:
            if (ad->nrderefs == 0 || (ad->nrderefs == 1 && isOutArg(ad)))
                fmt = "a8";
            else
                fmt = "A8";

            break;

        case sstring_type:
        case ustring_type:
        case string_type:
            if (ad->nrderefs == 0 || (ad->nrderefs == 1 && isOutArg(ad)))
                fmt = "c";
            else if (isArray(ad))
                fmt = "g";
            else
                fmt = "s";

            break;

        case wstring_type:
            if (ad->nrderefs == 0 || (ad->nrderefs == 1 && isOutArg(ad)))
                fmt = "w";
            else if (isArray(ad))
                fmt = "G";
            else
                fmt = "x";

            break;

        case bool_type:
        case cbool_type:
            fmt = "b";
            break;

        case enum_type:
            fmt = (ad->u.ed->fqcname != NULL) ? "F" : "e";
            break;

        case cint_type:
            fmt = "i";
            break;

        case uint_type:
            if (isArraySize(ad))
                arraylenarg = a;
            else
                fmt = "u";

            break;

        case int_type:
            if (isArraySize(ad))
                arraylenarg = a;
            else
                fmt = "i";

            break;

        case byte_type:
        case sbyte_type:
            if (isArraySize(ad))
                arraylenarg = a;
            else
                fmt = "L";

            break;

        case ubyte_type:
            if (isArraySize(ad))
                arraylenarg = a;
            else
                fmt = "M";

            break;

        case ushort_type:
            if (isArraySize(ad))
                arraylenarg = a;
            else
                fmt = "t";

            break;

        case short_type:
            if (isArraySize(ad))
                arraylenarg = a;
            else
                fmt = "h";

            break;

        case long_type:
            if (isArraySize(ad))
                arraylenarg = a;
            else
                fmt = "l";

            break;

        case ulong_type:
            if (isArraySize(ad))
                arraylenarg = a;
            else
                fmt = "m";

            break;

        case longlong_type:
            if (isArraySize(ad))
                arraylenarg = a;
            else
                fmt = "n";

            break;

        case ulonglong_type:
            if (isArraySize(ad))
                arraylenarg = a;
            else
                fmt = "o";

            break;

        case struct_type:
        case void_type:
            fmt = "V";
            break;

        case capsule_type:
            fmt = "z";
            break;

        case float_type:
        case cfloat_type:
            fmt = "f";
            break;

        case double_type:
        case cdouble_type:
            fmt = "d";
            break;

        case signal_type:
        case slot_type:
        case slotcon_type:
        case slotdis_type:
            fmt = "s";
            break;

        case mapped_type:
        case class_type:
            if (isArray(ad))
            {
                fmt = "r";
                break;
            }

            if (copyConstRefArg(ad))
            {
                fmt = "N";
                break;
            }

            /* Drop through. */

        case fake_void_type:
        case rxcon_type:
        case rxdis_type:
        case qobject_type:
            fmt = "D";
            break;

        case pyobject_type:
        case pytuple_type:
        case pylist_type:
        case pydict_type:
        case pycallable_type:
        case pyslice_type:
        case pytype_type:
        case pybuffer_type:
            fmt = "S";
            break;
        }

        prcode(fp,fmt);
    }

    prcode(fp,"\"");

    for (a = 0; a < sd->nrArgs; ++a)
    {
        int derefs;
        argDef *ad = &sd->args[a];

        if (!isInArg(ad))
            continue;

        derefs = ad->nrderefs;

        switch (ad->atype)
        {
        case ascii_string_type:
        case latin1_string_type:
        case utf8_string_type:
        case sstring_type:
        case ustring_type:
        case string_type:
        case wstring_type:
            if (!(ad->nrderefs == 0 || (ad->nrderefs == 1 && isOutArg(ad))))
                --derefs;

            break;

        case mapped_type:
        case fake_void_type:
        case class_type:
            if (ad->nrderefs > 0)
                --derefs;

            break;

        case struct_type:
        case void_type:
            --derefs;
            break;
        }

        if (ad->atype == mapped_type || ad->atype == class_type ||
            ad->atype == rxcon_type || ad->atype == rxdis_type ||
            ad->atype == qobject_type || ad->atype == fake_void_type)
        {
            int copy = copyConstRefArg(ad);

            prcode(fp,",");

            if (copy)
            {
                prcode(fp,"new %b(",ad);
            }
            else
            {
                if (isConstArg(ad))
                    prcode(fp,"const_cast<%b *>(",ad);

                if (ad->nrderefs == 0)
                    prcode(fp,"&");
                else
                    while (derefs-- != 0)
                        prcode(fp,"*");
            }

            prcode(fp, "%a", mod, ad, a);

            if (copy || isConstArg(ad))
                prcode(fp,")");

            if (isArray(ad))
                prcode(fp, ",(SIP_SSIZE_T)%a", mod, &sd->args[arraylenarg], arraylenarg);

            if (ad->atype == mapped_type)
                prcode(fp, ",sipType_%T", ad);
            else if (ad->atype == fake_void_type || ad->atype == class_type)
                prcode(fp, ",sipType_%C", classFQCName(ad->u.cd));
            else
                prcode(fp,",sipType_QObject");

            if (!isArray(ad))
                prcode(fp, ",NULL");
        }
        else if (ad->atype == capsule_type)
        {
            prcode(fp, ", \"%S\"", ad->u.cap);
        }
        else
        {
            if (!isArraySize(ad))
            {
                prcode(fp, ",");

                while (derefs-- != 0)
                    prcode(fp, "*");

                prcode(fp, "%a", mod, ad, a);
            }

            if (isArray(ad))
                prcode(fp, ",(SIP_SSIZE_T)%a", mod, &sd->args[arraylenarg], arraylenarg);
            else if (ad->atype == enum_type && ad->u.ed->fqcname != NULL)
                prcode(fp, ",sipType_%C", ad->u.ed->fqcname);
        }
    }
}


/*
 * Generate the library header #include directives required by either a class
 * or a module.
 */
static void generateUsedIncludes(ifaceFileList *iffl, FILE *fp)
{
    prcode(fp,
"\n"
        );

    while (iffl != NULL)
    {
        generateCppCodeBlock(iffl->iff->hdrcode, fp);
        iffl = iffl->next;
    }
}


/*
 * Generate the API details for a module.
 */
static void generateModuleAPI(sipSpec *pt, moduleDef *mod, FILE *fp)
{
    classDef *cd;
    mappedTypeDef *mtd;
    exceptionDef *xd;

    for (cd = pt->classes; cd != NULL; cd = cd->next)
        if (cd->iff->module == mod)
            generateClassAPI(cd, pt, fp);

    for (mtd = pt->mappedtypes; mtd != NULL; mtd = mtd->next)
        if (mtd->iff->module == mod)
            generateMappedTypeAPI(pt, mtd, fp);

    for (xd = pt->exceptions; xd != NULL; xd = xd->next)
        if (xd->iff->module == mod && xd->exceptionnr >= 0)
            prcode(fp,
"\n"
"#define sipException_%C sipModuleAPI_%s.em_exceptions[%d]\n"
                , xd->iff->fqcname, mod->name, xd->exceptionnr);
}


/*
 * Generate the API details for an imported module.
 */
static void generateImportedModuleAPI(sipSpec *pt, moduleDef *mod,
        moduleDef *immod, FILE *fp)
{
    classDef *cd;
    mappedTypeDef *mtd;
    exceptionDef *xd;

    for (cd = pt->classes; cd != NULL; cd = cd->next)
        if (cd->iff->module == immod && !isExternal(cd))
            generateImportedClassAPI(cd, pt, mod, fp);

    for (mtd = pt->mappedtypes; mtd != NULL; mtd = mtd->next)
        if (mtd->iff->module == immod)
            generateImportedMappedTypeAPI(mtd, pt, mod, fp);

    for (xd = pt->exceptions; xd != NULL; xd = xd->next)
        if (xd->iff->module == immod && xd->exceptionnr >= 0)
                prcode(fp,
"\n"
"#define sipException_%C sipModuleAPI_%s_%s->em_exceptions[%d]\n"
                    , xd->iff->fqcname, mod->name, xd->iff->module->name, xd->exceptionnr);
}


/*
 * Generate the API details for an imported mapped type.
 */
static void generateImportedMappedTypeAPI(mappedTypeDef *mtd, sipSpec *pt,
        moduleDef *mod, FILE *fp)
{
    /* Ignore alternate API implementations. */
    if (mtd->iff->first_alt == mtd->iff)
    {
        const char *mname = mod->name;
        const char *imname = mtd->iff->module->name;
        argDef type;

        memset(&type, 0, sizeof (argDef));

        type.atype = mapped_type;
        type.u.mtd = mtd;

        prcode(fp,
"\n"
"#define sipType_%T      sipModuleAPI_%s_%s->em_types[%d]\n"
            , &type, mname, imname, mtd->iff->ifacenr);
    }

    generateEnumMacros(pt, mod, NULL, mtd, fp);
}


/*
 * Generate the API details for a mapped type.
 */
static void generateMappedTypeAPI(sipSpec *pt, mappedTypeDef *mtd, FILE *fp)
{
    argDef type;

    memset(&type, 0, sizeof (argDef));

    type.atype = mapped_type;
    type.u.mtd = mtd;

    if (mtd->iff->first_alt == mtd->iff)
        prcode(fp,
"\n"
"#define sipType_%T      sipModuleAPI_%s.em_types[%d]\n"
            , &type, mtd->iff->module->name, mtd->iff->ifacenr);

    prcode(fp,
"\n"
"extern sipMappedTypeDef sipTypeDef_%s_%L;\n"
        , mtd->iff->module->name, mtd->iff);

    generateEnumMacros(pt, mtd->iff->module, NULL, mtd, fp);
}


/*
 * Generate the API details for an imported class.
 */
static void generateImportedClassAPI(classDef *cd, sipSpec *pt, moduleDef *mod,
        FILE *fp)
{
    prcode(fp,
"\n"
        );

    /* Ignore alternate API implementations. */
    if (cd->iff->first_alt == cd->iff)
    {
        const char *mname = mod->name;
        const char *imname = cd->iff->module->name;

        if (cd->iff->type == namespace_iface)
            prcode(fp,
"#if !defined(sipType_%L)\n"
                , cd->iff);

        prcode(fp,
"#define sipType_%C              sipModuleAPI_%s_%s->em_types[%d]\n"
"#define sipClass_%C             sipModuleAPI_%s_%s->em_types[%d]->u.td_wrapper_type\n"
            , classFQCName(cd), mname, imname, cd->iff->ifacenr
            , classFQCName(cd), mname, imname, cd->iff->ifacenr);

        if (cd->iff->type == namespace_iface)
            prcode(fp,
"#endif\n"
                );
    }

    generateEnumMacros(pt, mod, cd, NULL, fp);
}


/*
 * Generate the C++ API for a class.
 */
static void generateClassAPI(classDef *cd, sipSpec *pt, FILE *fp)
{
    const char *mname = cd->iff->module->name;

    prcode(fp,
"\n"
            );

    if (cd->real == NULL && cd->iff->first_alt == cd->iff)
        prcode(fp,
"#define sipType_%C              sipModuleAPI_%s.em_types[%d]\n"
"#define sipClass_%C             sipModuleAPI_%s.em_types[%d]->u.td_wrapper_type\n"
            , classFQCName(cd), mname, cd->iff->ifacenr
            , classFQCName(cd), mname, cd->iff->ifacenr);

    generateEnumMacros(pt, cd->iff->module, cd, NULL, fp);

    if (!isExternal(cd))
    {
        const char *type_prefix;

        if (pluginPyQt4(pt) || pluginPyQt5(pt))
            type_prefix = "pyqt4";
        else if (pluginPyQt3(pt))
            type_prefix = "pyqt3";
        else
            type_prefix = "sip";

        prcode(fp,
"\n"
"extern %sClassTypeDef sipTypeDef_%s_%L;\n"
            , type_prefix, mname, cd->iff);

        if (isExportDerived(cd))
        {
            generateCppCodeBlock(cd->iff->hdrcode, fp);
            generateShadowClassDeclaration(pt, cd, fp);
        }
    }
}


/*
 * Generate the sipEnum_* macros.
 */
static void generateEnumMacros(sipSpec *pt, moduleDef *mod, classDef *cd,
        mappedTypeDef *mtd, FILE *fp)
{
    enumDef *ed;
    int noIntro = TRUE;

    for (ed = pt->enums; ed != NULL; ed = ed->next)
    {
        if (ed->fqcname == NULL)
            continue;

        if (ed->first_alt != ed)
            continue;

        if (cd != NULL)
        {
            if (ed->ecd != cd)
                continue;
        }
        else if (mtd != NULL)
        {
            if (ed->emtd != mtd)
                continue;
        }
        else if (ed->ecd != NULL || ed->emtd != NULL)
        {
            continue;
        }

        if (noIntro)
        {
            prcode(fp,
"\n"
                );

            noIntro = FALSE;
        }

        if (mod == ed->module)
            prcode(fp,
"#define sipType_%C              sipModuleAPI_%s.em_types[%d]\n"
"#define sipEnum_%C              sipModuleAPI_%s.em_types[%d]->u.td_py_type\n"
                , ed->fqcname, mod->name, ed->enumnr
                , ed->fqcname, mod->name, ed->enumnr);
        else
            prcode(fp,
"#define sipType_%C              sipModuleAPI_%s_%s->em_types[%d]\n"
"#define sipEnum_%C              sipModuleAPI_%s_%s->em_types[%d]->u.td_py_type\n"
                , ed->fqcname, mod->name, ed->module->name, ed->enumnr
                , ed->fqcname, mod->name, ed->module->name, ed->enumnr);
    }
}


/*
 * Generate the shadow class declaration.
 */
static void generateShadowClassDeclaration(sipSpec *pt,classDef *cd,FILE *fp)
{
    int noIntro, nrVirts;
    ctorDef *ct;
    virtOverDef *vod;
    classDef *pcd;

    prcode(fp,
"\n"
"\n"
"class sip%C : public %S\n"
"{\n"
"public:\n"
        ,classFQCName(cd),classFQCName(cd));

    /* Define a shadow class for any protected classes we have. */

    for (pcd = pt->classes; pcd != NULL; pcd = pcd->next)
    {
        mroDef *mro;

        if (!isProtectedClass(pcd))
            continue;

        /* See if the class defining the class is in our class hierachy. */
        for (mro = cd->mro; mro != NULL; mro = mro->next)
            if (mro->cd == pcd->ecd)
                break;

        if (mro == NULL)
            continue;

        prcode(fp,
"    class sip%s : public %s {\n"
"    public:\n"
                , classBaseName(pcd), classBaseName(pcd));

        generateProtectedEnums(pt, pcd, fp);

        prcode(fp,
"    };\n"
"\n"
                );
    }

    /* The constructor declarations. */

    for (ct = cd->ctors; ct != NULL; ct = ct->next)
    {
        ctorDef *dct;

        if (isPrivateCtor(ct))
            continue;

        if (ct->cppsig == NULL)
            continue;

        /* Check we haven't already handled this C++ signature. */
        for (dct = cd->ctors; dct != ct; dct = dct->next)
            if (dct->cppsig != NULL && sameSignature(dct->cppsig, ct->cppsig, TRUE))
                break;

        if (dct != ct)
            continue;

        prcode(fp,
"    sip%C(",classFQCName(cd));

        generateCalledArgs(NULL, cd->iff, ct->cppsig, Declaration, TRUE, fp);

        prcode(fp,")%X;\n"
            ,ct->exceptions);
    }

    /* The destructor. */

    if (!isPrivateDtor(cd))
        prcode(fp,
"    %s~sip%C()%X;\n"
            ,(cd->vmembers != NULL ? "virtual " : ""),classFQCName(cd),cd->dtorexceptions);

    /* The metacall methods if required. */
    if ((pluginPyQt4(pt) || pluginPyQt5(pt)) && isQObjectSubClass(cd))
    {
        prcode(fp,
"\n"
"    int qt_metacall(QMetaObject::Call,int,void **);\n"
"    void *qt_metacast(const char *);\n"
            );

        if (!noPyQt4QMetaObject(cd))
            prcode(fp,
"    const QMetaObject *metaObject() const;\n"
                );
    }

    /* The exposure of protected enums. */

    generateProtectedEnums(pt,cd,fp);

    /* The wrapper around each protected member function. */

    generateProtectedDeclarations(cd,fp);

    /* The public wrapper around each signal emitter. */
    if (pluginPyQt3(pt))
    {
        visibleList *vl;

        noIntro = TRUE;

        for (vl = cd->visible; vl != NULL; vl = vl->next)
        {
            overDef *od;

            if (vl->m->slot != no_slot)
                continue;

            for (od = vl->cd->overs; od != NULL; od = od->next)
            {
                if (od->common != vl->m || !isSignal(od))
                    continue;

                if (noIntro)
                {
                    prcode(fp,
"\n"
"    /*\n"
"     * There is a public method for every Qt signal that can be emitted\n"
"     * by this object.  This function is called by Python to emit the\n"
"     * signal.\n"
"     */\n"
                        );

                    noIntro = FALSE;
                }

                prcode(fp,
"    int sipEmit_%s(PyObject *);\n"
                    ,vl->m->pyname->text);

                break;
            }
        }
    }

    /* The catcher around each virtual function in the hierarchy. */
    noIntro = TRUE;

    for (vod = cd->vmembers; vod != NULL; vod = vod->next)
    {
        overDef *od = &vod->o;
        virtOverDef *dvod;

        if (isPrivate(od))
            continue;

        /* Check we haven't already handled this C++ signature. */
        for (dvod = cd->vmembers; dvod != vod; dvod = dvod->next)
            if (strcmp(dvod->o.cppname,od->cppname) == 0 && sameSignature(dvod->o.cppsig,od->cppsig,TRUE))
                break;

        if (dvod != vod)
            continue;

        if (noIntro)
        {
            prcode(fp,
"\n"
"    /*\n"
"     * There is a protected method for every virtual method visible from\n"
"     * this class.\n"
"     */\n"
"protected:\n"
                );

            noIntro = FALSE;
        }

        prcode(fp,
"    ");
 
        prOverloadDecl(fp, cd->iff, od, FALSE);
        prcode(fp, ";\n");
    }

    prcode(fp,
"\n"
"public:\n"
"    sipSimpleWrapper *sipPySelf;\n"
        );

    /* The private declarations. */

    prcode(fp,
"\n"
"private:\n"
"    sip%C(const sip%C &);\n"
"    sip%C &operator = (const sip%C &);\n"
        ,classFQCName(cd),classFQCName(cd)
        ,classFQCName(cd),classFQCName(cd));

    if ((nrVirts = countVirtuals(cd)) > 0)
        prcode(fp,
"\n"
"    char sipPyMethods[%d];\n"
            ,nrVirts);

    prcode(fp,
"};\n"
        );
}


/*
 * Generate the C++ declaration for an overload.
 */
void prOverloadDecl(FILE *fp, ifaceFileDef *scope, overDef *od, int defval)
{
    int a;

    normaliseArgs(od->cppsig);

    generateBaseType(scope, &od->cppsig->result, TRUE, fp);
 
    prcode(fp, " %O(", od);

    for (a = 0; a < od->cppsig->nrArgs; ++a)
    {
        argDef *ad = &od->cppsig->args[a];

        if (a > 0)
            prcode(fp, ",");

        generateBaseType(scope, ad, TRUE, fp);

        if (defval && ad->defval != NULL)
        {
            prcode(fp, " = ");
            generateExpression(ad->defval, FALSE, fp);
        }
    }
 
    prcode(fp, ")%s%X", (isConst(od) ? " const" : ""), od->exceptions);

    restoreArgs(od->cppsig);
}


/*
 * Generate typed arguments for a declaration or a definition.
 */
static void generateCalledArgs(moduleDef *mod, ifaceFileDef *scope,
        signatureDef *sd, funcArgType ftype, int use_typename, FILE *fp)
{
    const char *name;
    char buf[50];
    int a;

    for (a = 0; a < sd->nrArgs; ++a)
    {
        argDef *ad = &sd->args[a];

        if (a > 0)
            prcode(fp,",");

        name = buf;

        if (ftype == Definition)
        {
            if (mod != NULL && useArgNames(mod) && ad->name != NULL)
                name = ad->name->text;
            else
                sprintf(buf, "a%d", a);
        }
        else
        {
            buf[0] = '\0';
        }

        generateNamedBaseType(scope, ad, name, use_typename, fp);
    }
}


/*
 * Generate typed arguments for a call.
 */
static void generateCallArgs(moduleDef *mod, signatureDef *sd,
        signatureDef *py_sd, FILE *fp)
{
    int a;

    for (a = 0; a < sd->nrArgs; ++a)
    {
        char *ind = NULL;
        argDef *ad, *py_ad;

        if (a > 0)
            prcode(fp,",");

        ad = &sd->args[a];

        /* See if the argument needs dereferencing or it's address taking. */
        switch (ad->atype)
        {
        case ascii_string_type:
        case latin1_string_type:
        case utf8_string_type:
        case sstring_type:
        case ustring_type:
        case string_type:
        case wstring_type:
            if (ad->nrderefs > (isOutArg(ad) ? 0 : 1))
                ind = "&";

            break;

        case mapped_type:
        case class_type:
            if (ad->nrderefs == 2)
                ind = "&";
            else if (ad->nrderefs == 0)
                ind = "*";

            break;

        case struct_type:
        case void_type:
            if (ad->nrderefs == 2)
                ind = "&";

            break;

        default:
            if (ad->nrderefs == 1)
                ind = "&";
        }

        if (ind != NULL)
            prcode(fp, ind);

        /*
         * See if we need to cast a Python void * to the correct C/C++ pointer
         * type.
         */
        if (py_sd != sd)
        {
            py_ad = &py_sd->args[a];

            if ((py_ad->atype != void_type && py_ad->atype != capsule_type) || ad->atype == void_type || ad->atype == capsule_type || py_ad->nrderefs != ad->nrderefs)
                py_ad = NULL;
        }
        else
            py_ad = NULL;

        if (py_ad == NULL)
        {
            if (isArraySize(ad))
                prcode(fp, "(%b)", ad);

            prcode(fp, "%a", mod, ad, a);
        }
        else if (generating_c)
            prcode(fp, "(%b *)%a", ad, mod, ad, a);
        else
            prcode(fp, "reinterpret_cast<%b *>(%a)", ad, mod, ad, a);
    }
}


/*
 * Generate the declaration of a named variable to hold a result from a C++
 * function call.
 */
static void generateNamedValueType(ifaceFileDef *scope, argDef *ad,
        char *name, FILE *fp)
{
    argDef mod = *ad;

    if (ad->nrderefs == 0)
    {
        if (ad->atype == class_type || ad->atype == mapped_type)
            mod.nrderefs = 1;
        else
            resetIsConstArg(&mod);
    }

    resetIsReference(&mod);
    generateNamedBaseType(scope, &mod, name, TRUE, fp);
}


/*
 * Generate a C++ type.
 */
static void generateBaseType(ifaceFileDef *scope, argDef *ad,
        int use_typename, FILE *fp)
{
    generateNamedBaseType(scope, ad, "", use_typename, fp);
}


/*
 * Generate a C++ type and name.
 */
static void generateNamedBaseType(ifaceFileDef *scope, argDef *ad,
        const char *name, int use_typename, FILE *fp)
{
    typedefDef *td = ad->original_type;
    int nr_derefs = ad->nrderefs;
    int is_reference = isReference(ad);

    if (use_typename && td != NULL && !noTypeName(td) && !isArraySize(ad))
    {
        if (isConstArg(ad) && !isConstArg(&td->type))
            prcode(fp, "const ");

        nr_derefs -= td->type.nrderefs;

        if (isReference(&td->type))
            is_reference = FALSE;

        prcode(fp, "%S", td->fqname);
    }
    else
    {
        /*
         * A function type is handled differently because of the position of
         * the name.
         */
        if (ad->atype == function_type)
        {
            int i;
            signatureDef *sig = ad->u.sa;

            generateBaseType(scope, &sig->result, TRUE, fp);

            prcode(fp," (");

            for (i = 0; i < nr_derefs; ++i)
                prcode(fp, "*");

            prcode(fp, "%s)(",name);
            generateCalledArgs(NULL, scope, sig, Declaration, use_typename, fp);
            prcode(fp, ")");

            return;
        }

        if (isConstArg(ad))
            prcode(fp, "const ");

        switch (ad->atype)
        {
        case sbyte_type:
        case sstring_type:
            prcode(fp, "signed char");
            break;

        case ubyte_type:
        case ustring_type:
            prcode(fp, "unsigned char");
            break;

        case wstring_type:
            prcode(fp, "wchar_t");
            break;

        case signal_type:
        case slot_type:
        case anyslot_type:
        case slotcon_type:
        case slotdis_type:
            nr_derefs = 1;

            /* Drop through. */

        case byte_type:
        case ascii_string_type:
        case latin1_string_type:
        case utf8_string_type:
        case string_type:
            prcode(fp, "char");
            break;

        case ushort_type:
            prcode(fp, "unsigned short");
            break;

        case short_type:
            prcode(fp, "short");
            break;

        case uint_type:
            prcode(fp, "uint");
            break;

        case int_type:
        case cint_type:
            prcode(fp, "int");
            break;

        case ssize_type:
            prcode(fp, "SIP_SSIZE_T");
            break;

        case ulong_type:
            prcode(fp, "unsigned long");
            break;

        case long_type:
            prcode(fp, "long");
            break;

        case ulonglong_type:
            prcode(fp, "unsigned PY_LONG_LONG");
            break;

        case longlong_type:
            prcode(fp, "PY_LONG_LONG");
            break;

        case struct_type:
            prcode(fp, "struct %S", ad->u.sname);
            break;

        case capsule_type:
            nr_derefs = 1;

            /* Drop through. */

        case fake_void_type:
        case void_type:
            prcode(fp, "void");
            break;

        case bool_type:
        case cbool_type:
            prcode(fp, "bool");
            break;

        case float_type:
        case cfloat_type:
            prcode(fp, "float");
            break;

        case double_type:
        case cdouble_type:
            prcode(fp, "double");
            break;

        case defined_type:
            /*
             * The only defined types still remaining are arguments to
             * templates and default values.
             */
            if (prcode_xml)
            {
                prScopedName(fp, ad->u.snd, ".");
            }
            else
            {
                if (generating_c)
                    fprintf(fp, "struct ");

                prScopedName(fp, ad->u.snd, "::");
            }

            break;

        case rxcon_type:
        case rxdis_type:
            nr_derefs = 1;
            prcode(fp, "QObject");
            break;

        case mapped_type:
            generateBaseType(scope, &ad->u.mtd->type, TRUE, fp);
            break;

        case class_type:
            prcode(fp, "%V", scope, ad->u.cd);
            break;

        case template_type:
            {
                static const char tail[] = ">";
                int a;
                templateDef *td = ad->u.td;

                prcode(fp, "%S%s", td->fqname, (prcode_xml ? "&lt;" : "<"));

                for (a = 0; a < td->types.nrArgs; ++a)
                {
                    if (a > 0)
                        prcode(fp, ",");

                    generateBaseType(scope, &td->types.args[a], TRUE, fp);
                }

                if (prcode_last == tail)
                    prcode(fp, " ");

                prcode(fp, (prcode_xml ? "&gt;" : tail));
                break;
            }

        case enum_type:
            prcode(fp, "%E", ad->u.ed);
            break;

        case pyobject_type:
        case pytuple_type:
        case pylist_type:
        case pydict_type:
        case pycallable_type:
        case pyslice_type:
        case pytype_type:
        case pybuffer_type:
        case qobject_type:
        case ellipsis_type:
            prcode(fp, "PyObject *");
            break;
        }
    }

    if (nr_derefs > 0)
    {
        int i;

        prcode(fp, " *");

        for (i = 1; i < nr_derefs; ++i)
        {
            if (ad->derefs[i])
                prcode(fp, " const ");

            prcode(fp, "*");
        }
    }

    if (is_reference)
        prcode(fp, (prcode_xml ? "&amp;" : "&"));

    if (*name != '\0')
    {
        if (nr_derefs == 0)
            prcode(fp, " ");

        prcode(fp, name);
    }
}


/*
 * Generate the definition of an argument variable and any supporting
 * variables.
 */
static void generateVariable(moduleDef *mod, ifaceFileDef *scope, argDef *ad,
        int argnr, FILE *fp)
{
    argType atype = ad->atype;
    argDef orig;

    if (isInArg(ad) && ad->defval != NULL &&
        (atype == class_type || atype == mapped_type) &&
        (ad->nrderefs == 0 || isReference(ad)))
    {
        /*
         * Generate something to hold the default value as it cannot be
         * assigned straight away.
         */
        prcode(fp,
"        %A %adef = ", scope, ad, mod, ad, argnr);

        generateExpression(ad->defval, FALSE, fp);

        prcode(fp,";\n"
            );
    }

    /* Adjust the type so we have the type that will really handle it. */

    orig = *ad;

    switch (atype)
    {
    case ascii_string_type:
    case latin1_string_type:
    case utf8_string_type:
    case sstring_type:
    case ustring_type:
    case string_type:
    case wstring_type:
        if (!isReference(ad))
        {
            if (ad->nrderefs == 2)
                ad->nrderefs = 1;
            else if (ad->nrderefs == 1 && isOutArg(ad))
                ad->nrderefs = 0;
        }

        break;

    case mapped_type:
    case class_type:
    case void_type:
    case struct_type:
        ad->nrderefs = 1;
        break;

    default:
        ad->nrderefs = 0;
    }

    /* Array sizes are always SIP_SSIZE_T. */
    if (isArraySize(ad))
        ad->atype = ssize_type;

    resetIsReference(ad);

    if (ad->nrderefs == 0)
        resetIsConstArg(ad);

    prcode(fp,
"        %A %a", scope, ad, mod, ad, argnr);

    if (atype == anyslot_type)
        prcode(fp, "Name");

    *ad = orig;

    generateDefaultValue(mod, ad, argnr, fp);

    prcode(fp,";\n"
        );

    /* Some types have supporting variables. */
    if (isInArg(ad))
    {
        if (isGetWrapper(ad))
            prcode(fp,
"        PyObject *%aWrapper%s;\n"
                , mod, ad, argnr, (ad->defval != NULL ? " = 0" : ""));
        else if (keepReference(ad))
            prcode(fp,
"        PyObject *%aKeep%s;\n"
                , mod, ad, argnr, (ad->defval != NULL ? " = 0" : ""));

        switch (atype)
        {
        case class_type:
            if (!isArray(ad) && ad->u.cd->convtocode != NULL && !isConstrained(ad))
                prcode(fp,
"        int %aState = 0;\n"
                    , mod, ad, argnr);

            break;

        case mapped_type:
            if (!noRelease(ad->u.mtd) && !isConstrained(ad))
                prcode(fp,
"        int %aState = 0;\n"
                    , mod, ad, argnr);

            break;

        case ascii_string_type:
        case latin1_string_type:
        case utf8_string_type:
            if (!keepReference(ad) && ad->nrderefs == 1)
                prcode(fp,
"        PyObject *%aKeep%s;\n"
                    , mod, ad, argnr, (ad->defval != NULL ? " = 0" : ""));

            break;

        case anyslot_type:
            prcode(fp,
"        PyObject *%aCallable", mod, ad, argnr);
            generateDefaultValue(mod, ad, argnr, fp);
            prcode(fp, ";\n"
                );
            break;
        }
    }
}


/*
 * Generate a default value.
 */
static void generateDefaultValue(moduleDef *mod, argDef *ad, int argnr,
        FILE *fp)
{
    if (isInArg(ad) && ad->defval != NULL)
    {
        prcode(fp," = ");

        if ((ad->atype == class_type || ad->atype == mapped_type) &&
                (ad->nrderefs == 0 || isReference(ad)))
            prcode(fp, "&%adef", mod, ad, argnr);
        else
            generateExpression(ad->defval, FALSE, fp);
    }
}


/*
 * Generate a simple function call.
 */
static void generateSimpleFunctionCall(fcallDef *fcd,FILE *fp)
{
    int i;

    prcode(fp, "%B(", &fcd->type);

    for (i = 0; i < fcd->nrArgs; ++i)
    {
        if (i > 0)
            prcode(fp,",");

        generateExpression(fcd->args[i], FALSE, fp);
    }

    prcode(fp,")");
}


/*
 * Generate the type structure that contains all the information needed by the
 * meta-type.  A sub-set of this is used to extend namespaces.
 */
static void generateTypeDefinition(sipSpec *pt, classDef *cd, FILE *fp)
{
    const char *mname, *sep, *type_prefix;
    int is_slots, is_signals, nr_methods, nr_enums, nr_vars, embedded;
    int is_inst_class, is_inst_voidp, is_inst_char, is_inst_string;
    int is_inst_int, is_inst_long, is_inst_ulong, is_inst_longlong;
    int is_inst_ulonglong, is_inst_double, has_docstring;
    memberDef *md;
    moduleDef *mod;
    propertyDef *pd;

    mod = cd->iff->module;
    mname = mod->name;

    if (cd->supers != NULL)
    {
        classList *cl;

        prcode(fp,
"\n"
"\n"
"/* Define this type's super-types. */\n"
"static sipEncodedTypeDef supers_%C[] = {", classFQCName(cd));

        for (cl = cd->supers; cl != NULL; cl = cl->next)
        {
            if (cl != cd->supers)
                prcode(fp, ", ");

            generateEncodedType(mod, cl->cd, (cl->next == NULL), fp);
        }

        prcode(fp,"};\n"
            );
    }

    /* Generate the slots table. */
    is_slots = FALSE;

    for (md = cd->members; md != NULL; md = md->next)
    {
        const char *stype;

        if (md->slot == no_slot)
            continue;

        if (!is_slots)
        {
            prcode(fp,
"\n"
"\n"
"/* Define this type's Python slots. */\n"
"static sipPySlotDef slots_%L[] = {\n"
                , cd->iff);

            is_slots = TRUE;
        }

        if ((stype = slotName(md->slot)) != NULL)
        {
            if (py2OnlySlot(md->slot))
                prcode(fp,
"#if PY_MAJOR_VERSION < 3\n"
                    );
            else if (py2_5LaterSlot(md->slot))
                prcode(fp,
"#if PY_VERSION_HEX >= 0x02050000\n"
                    );

            prcode(fp,
"    {(void *)slot_%L_%s, %s},\n"
                , cd->iff, md->pyname->text, stype);

            if (py2OnlySlot(md->slot) || py2_5LaterSlot(md->slot))
                prcode(fp,
"#endif\n"
                    );
        }
    }

    if (is_slots)
        prcode(fp,
"    {0, (sipPySlotType)0}\n"
"};\n"
            );

    /* Generate the attributes tables. */
    nr_methods = generateClassMethodTable(pt, cd, fp);
    nr_enums = generateEnumMemberTable(pt, mod, cd, NULL, fp);

    /* Generate the PyQt4 signals table. */
    is_signals = FALSE;

    if ((pluginPyQt4(pt) || pluginPyQt5(pt)) && isQObjectSubClass(cd))
    {
        /* The signals must be grouped by name. */
        for (md = cd->members; md != NULL; md = md->next)
        {
            overDef *od;
            int membernr = md->membernr;

            for (od = cd->overs; od != NULL; od = od->next)
            {
                int a, nr_args;
                signatureDef *cppsig;

                if (od->common != md || !isSignal(od))
                    continue;

                if (membernr >= 0)
                {
                    /* See if there is a non-signal overload. */

                    overDef *nsig;

                    for (nsig = cd->overs; nsig != NULL; nsig = nsig->next)
                        if (nsig != od && nsig->common == md && !isSignal(nsig))
                            break;

                    if (nsig == NULL)
                        membernr = -1;
                }

                if (!is_signals)
                {
                    is_signals = TRUE;

                    prcode(fp,
"\n"
"\n"
"/* Define this type's PyQt4 signals. */\n"
"static const pyqt4QtSignal pyqt4_signals_%C[] = {\n"
                        , classFQCName(cd));
                }

                /*
                 * Default arguments are handled as multiple signals.  We make
                 * sure the largest is first and the smallest last which is
                 * what Qt does.
                 */
                cppsig = od->cppsig;
                nr_args = cppsig->nrArgs;

                generateSignalTableEntry(pt, cd, od, md, membernr, fp);
                membernr = -1;

                for (a = nr_args - 1; a >= 0; --a)
                {
                    if (cppsig->args[a].defval == NULL)
                        break;

                    cppsig->nrArgs = a;
                    generateSignalTableEntry(pt, cd, od, md, -1, fp);
                }

                cppsig->nrArgs = nr_args;
            }
        }

        if (is_signals)
            prcode(fp,
"    {0, 0, 0}\n"
"};\n"
                );
    }

    /* Generate the property and variable handlers. */
    nr_vars = 0;

    if (hasVarHandlers(cd))
    {
        varDef *vd;

        for (vd = pt->vars; vd != NULL; vd = vd->next)
            if (vd->ecd == cd && needsHandler(vd))
            {
                ++nr_vars;

                generateVariableGetter(cd->iff, vd, fp);

                if (canSetVariable(vd))
                    generateVariableSetter(cd->iff, vd, fp);
            }
    }

    /* Generate any docstrings. */
    for (pd = cd->properties; pd != NULL; pd = pd->next)
    {
        ++nr_vars;

        if (pd->docstring != NULL)
        {
            prcode(fp,
"\n"
"PyDoc_STRVAR(doc_%L_%s, " , cd->iff, pd->name->text);

            generateExplicitDocstring(pd->docstring, fp);

            prcode(fp, ");\n"
                );
        }
    }

    /* Generate the variables table. */
    if (nr_vars > 0)
        prcode(fp,
"\n"
"sipVariableDef variables_%L[] = {\n"
            , cd->iff);

    for (pd = cd->properties; pd != NULL; pd = pd->next)
    {
        prcode(fp,
"    {PropertyVariable, %N, &methods_%L[%d], ", pd->name, cd->iff, findMethod(cd, pd->get)->membernr);

        if (pd->set != NULL)
            prcode(fp, "&methods_%L[%d], ", cd->iff, findMethod(cd, pd->set)->membernr);
        else
            prcode(fp, "NULL, ");

        /* We don't support a deleter yet. */
        prcode(fp, "NULL, ");

        if (pd->docstring != NULL)
            prcode(fp, "doc_%L_%s", cd->iff, pd->name->text);
        else
            prcode(fp, "NULL");

        prcode(fp, "},\n"
            );
    }

    if (hasVarHandlers(cd))
    {
        varDef *vd;

        for (vd = pt->vars; vd != NULL; vd = vd->next)
            if (vd->ecd == cd && needsHandler(vd))
            {
                prcode(fp,
"    {%s, %N, (PyMethodDef *)varget_%C, ", (isStaticVar(vd) ? "ClassVariable" : "InstanceVariable"), vd->pyname, vd->fqcname);

                if (canSetVariable(vd))
                    prcode(fp, "(PyMethodDef *)varset_%C", vd->fqcname);
                else
                    prcode(fp, "NULL");

                prcode(fp, ", NULL, NULL},\n"
                    );
            }
    }

    if (nr_vars > 0)
        prcode(fp,
"};\n"
            );

    /* Generate each instance table. */
    is_inst_class = generateClasses(pt, mod, cd, fp);
    is_inst_voidp = generateVoidPointers(pt, mod, cd, fp);
    is_inst_char = generateChars(pt, mod, cd, fp);
    is_inst_string = generateStrings(pt, mod, cd, fp);
    is_inst_int = generateInts(pt, mod, cd, fp);
    is_inst_long = generateLongs(pt, mod, cd, fp);
    is_inst_ulong = generateUnsignedLongs(pt, mod, cd, fp);
    is_inst_longlong = generateLongLongs(pt, mod, cd, fp);
    is_inst_ulonglong = generateUnsignedLongLongs(pt, mod, cd, fp);
    is_inst_double = generateDoubles(pt, mod, cd, fp);

    /* Generate the docstrings. */
    has_docstring = FALSE;

    if (cd->docstring != NULL || (docstrings && hasClassDocstring(pt, cd)))
    {
        prcode(fp,
"\n"
"PyDoc_STRVAR(doc_%L, ", cd->iff);

        if (cd->docstring != NULL)
            generateExplicitDocstring(cd->docstring, fp);
        else
            generateClassDocstring(pt, cd, fp);

        prcode(fp, ");\n"
            );

        has_docstring = TRUE;
    }

    if (pluginPyQt4(pt) || pluginPyQt5(pt))
    {
        type_prefix = "pyqt4";
        embedded = TRUE;
    }
    else if (pluginPyQt3(pt))
    {
        type_prefix = "pyqt3";
        embedded = TRUE;
    }
    else
    {
        type_prefix = "sip";
        embedded = FALSE;
    }

    prcode(fp,
"\n"
"\n"
"%sClassTypeDef ", type_prefix);

    generateTypeDefName(cd->iff, fp);

    prcode(fp, " = {\n"
"%s"
"    {\n"
"        %P,\n"
"        "
        , (embedded ? "{\n" : "")
        , cd->iff->api_range);

    generateTypeDefLink(pt, cd->iff, fp);

    prcode(fp, ",\n"
"        0,\n"
"        ");

    sep = "";

    if (isAbstractClass(cd))
    {
        prcode(fp, "%sSIP_TYPE_ABSTRACT", sep);
        sep = "|";
    }

    if (cd->subbase != NULL)
    {
        prcode(fp, "%sSIP_TYPE_SCC", sep);
        sep = "|";
    }

    if (classHandlesNone(cd))
    {
        prcode(fp, "%sSIP_TYPE_ALLOW_NONE", sep);
        sep = "|";
    }

    if (hasNonlazyMethod(cd))
    {
        prcode(fp, "%sSIP_TYPE_NONLAZY", sep);
        sep = "|";
    }

    if (isCallSuperInitYes(mod))
    {
        prcode(fp, "%sSIP_TYPE_SUPER_INIT", sep);
        sep = "|";
    }

    if (cd->iff->type == namespace_iface)
    {
        prcode(fp, "%sSIP_TYPE_NAMESPACE", sep);
        sep = "|";
    }
    else
    {
        prcode(fp, "%sSIP_TYPE_CLASS", sep);
        sep = "|";
    }

    if (*sep == '\0')
        prcode(fp, "0");

    prcode(fp, ",\n");

    prcode(fp,
"        %n,\n"
"        {0}\n"
"    },\n"
"    {\n"
        , cd->iff->name);

    if (cd->real == NULL)
        prcode(fp,
"        %n,\n"
            , cd->pyname);
    else
        prcode(fp,
"        -1,\n"
            );

    prcode(fp, "        ");

    if (cd->real != NULL)
        generateEncodedType(mod, cd->real, 0, fp);
    else if (cd->ecd != NULL)
        generateEncodedType(mod, cd->ecd, 0, fp);
    else
        prcode(fp, "{0, 0, 1}");

    prcode(fp, ",\n"
        );

    if (nr_methods == 0)
        prcode(fp,
"        0, 0,\n"
            );
    else
        prcode(fp,
"        %d, methods_%L,\n"
            , nr_methods, cd->iff);

    if (nr_enums == 0)
        prcode(fp,
"        0, 0,\n"
            );
    else
        prcode(fp,
"        %d, enummembers_%L,\n"
            , nr_enums, cd->iff);

    if (nr_vars == 0)
        prcode(fp,
"        0, 0,\n"
            );
    else
        prcode(fp,
"        %d, variables_%L,\n"
            , nr_vars, cd->iff);

    prcode(fp,
"        {");

    if (is_inst_class)
        prcode(fp, "typeInstances_%C, ", classFQCName(cd));
    else
        prcode(fp, "0, ");

    if (is_inst_voidp)
        prcode(fp, "voidPtrInstances_%C, ", classFQCName(cd));
    else
        prcode(fp, "0, ");

    if (is_inst_char)
        prcode(fp, "charInstances_%C, ", classFQCName(cd));
    else
        prcode(fp, "0, ");

    if (is_inst_string)
        prcode(fp, "stringInstances_%C, ", classFQCName(cd));
    else
        prcode(fp, "0, ");

    if (is_inst_int)
        prcode(fp, "intInstances_%C, ", classFQCName(cd));
    else
        prcode(fp, "0, ");

    if (is_inst_long)
        prcode(fp, "longInstances_%C, ", classFQCName(cd));
    else
        prcode(fp, "0, ");

    if (is_inst_ulong)
        prcode(fp, "unsignedLongInstances_%C, ", classFQCName(cd));
    else
        prcode(fp, "0, ");

    if (is_inst_longlong)
        prcode(fp, "longLongInstances_%C, ", classFQCName(cd));
    else
        prcode(fp,"0, ");

    if (is_inst_ulonglong)
        prcode(fp, "unsignedLongLongInstances_%C, ", classFQCName(cd));
    else
        prcode(fp, "0, ");

    if (is_inst_double)
        prcode(fp, "doubleInstances_%C", classFQCName(cd));
    else
        prcode(fp, "0");

    prcode(fp,"},\n"
"    },\n"
        );

    if (has_docstring)
        prcode(fp,
"    doc_%L,\n"
            , cd->iff);
    else
        prcode(fp,
"    0,\n"
            );

    if (cd->metatype != NULL)
        prcode(fp,
"    %n,\n"
            , cd->metatype);
    else
        prcode(fp,
"    -1,\n"
            );

    if (cd->supertype != NULL)
        prcode(fp,
"    %n,\n"
            , cd->supertype);
    else
        prcode(fp,
"    -1,\n"
            );

    if (cd->supers != NULL)
        prcode(fp,
"    supers_%C,\n"
            , classFQCName(cd));
    else
        prcode(fp,
"    0,\n"
            );

    if (is_slots)
        prcode(fp,
"    slots_%L,\n"
            , cd->iff);
    else
        prcode(fp,
"    0,\n"
            );

    if (canCreate(cd))
        prcode(fp,
"    init_%L,\n"
            , cd->iff);
    else
        prcode(fp,
"    0,\n"
            );

    if (cd->travcode != NULL)
        prcode(fp,
"    traverse_%C,\n"
            , classFQCName(cd));
    else
        prcode(fp,
"    0,\n"
            );

    if (cd->clearcode != NULL)
        prcode(fp,
"    clear_%C,\n"
            , classFQCName(cd));
    else
        prcode(fp,
"    0,\n"
            );

    prcode(fp,
"#if PY_MAJOR_VERSION >= 3\n"
        );

    if (cd->getbufcode != NULL)
        prcode(fp,
"    getbuffer_%C,\n"
            , classFQCName(cd));
    else
        prcode(fp,
"    0,\n"
            );

    if (cd->releasebufcode != NULL)
        prcode(fp,
"    releasebuffer_%C,\n"
            , classFQCName(cd));
    else
        prcode(fp,
"    0,\n"
            );

    prcode(fp,
"#else\n"
        );

    if (cd->readbufcode != NULL)
        prcode(fp,
"    getreadbuffer_%C,\n"
            , classFQCName(cd));
    else
        prcode(fp,
"    0,\n"
            );

    if (cd->writebufcode != NULL)
        prcode(fp,
"    getwritebuffer_%C,\n"
            , classFQCName(cd));
    else
        prcode(fp,
"    0,\n"
            );

    if (cd->segcountcode != NULL)
        prcode(fp,
"    getsegcount_%C,\n"
            , classFQCName(cd));
    else
        prcode(fp,
"    0,\n"
            );

    if (cd->charbufcode != NULL)
        prcode(fp,
"    getcharbuffer_%C,\n"
            , classFQCName(cd));
    else
        prcode(fp,
"    0,\n"
            );

    prcode(fp,
"#endif\n"
        );

    if (needDealloc(cd))
        prcode(fp,
"    dealloc_%L,\n"
            , cd->iff);
    else
        prcode(fp,
"    0,\n"
            );

    if (generating_c || assignmentHelper(cd))
        prcode(fp,
"    assign_%L,\n"
"    array_%L,\n"
"    copy_%L,\n"
            , cd->iff
            , cd->iff
            , cd->iff);
    else
        prcode(fp,
"    0,\n"
"    0,\n"
"    0,\n"
            );

    if (cd->iff->type == namespace_iface || generating_c)
        prcode(fp,
"    0,\n"
"    0,\n"
            );
    else
        prcode(fp,
"    release_%L,\n"
"    cast_%L,\n"
            , cd->iff
            , cd->iff);

    if (cd->iff->type == namespace_iface)
    {
        prcode(fp,
"    0,\n"
            );
    }
    else
    {
        if (cd->convtocode != NULL)
            prcode(fp,
"    convertTo_%L,\n"
                , cd->iff);
        else
            prcode(fp,
"    0,\n"
                );
    }

    if (cd->iff->type == namespace_iface)
    {
        prcode(fp,
"    0,\n"
            );
    }
    else
    {
        if (cd->convfromcode != NULL)
            prcode(fp,
"    convertFrom_%L,\n"
                , cd->iff);
        else
            prcode(fp,
"    0,\n"
                );
    }

    prcode(fp,
"    0,\n"
        );

    if (cd->picklecode != NULL)
        prcode(fp,
"    pickle_%C,\n"
            , classFQCName(cd));
    else
        prcode(fp,
"    0,\n"
            );

    if (cd->finalcode != NULL)
        prcode(fp,
"    final_%C,\n"
            , classFQCName(cd));
    else
        prcode(fp,
"    0,\n"
            );

    if (isMixin(cd))
        prcode(fp,
"    mixin_%C\n"
            , classFQCName(cd));
    else
        prcode(fp,
"    0\n"
            );

    if (embedded)
        prcode(fp,
"},\n"
            );

    if (pluginPyQt3(pt))
    {
        if (hasSigSlots(cd))
            prcode(fp,
"    signals_%C\n"
                , classFQCName(cd));
        else
            prcode(fp,
"    0\n"
                );
    }

    if (pluginPyQt4(pt) || pluginPyQt5(pt))
    {
        if (isQObjectSubClass(cd) && !noPyQt4QMetaObject(cd))
            prcode(fp,
"    &%U::staticMetaObject,\n"
                , cd);
        else
            prcode(fp,
"    0,\n"
                );

        prcode(fp,
"    %u,\n"
            , cd->pyqt4_flags);

        if (is_signals)
            prcode(fp,
"    pyqt4_signals_%C,\n"
                , classFQCName(cd));
        else
            prcode(fp,
"    0,\n"
                );

        if (cd->pyqt_interface != NULL)
            prcode(fp,
"    \"%s\"\n"
                , cd->pyqt_interface);
        else
            prcode(fp,
"    0\n"
                );
    }

    prcode(fp,
"};\n"
        );
}


/*
 * Generate an entry in the PyQt4 signal table.
 */
static void generateSignalTableEntry(sipSpec *pt, classDef *cd, overDef *sig,
        memberDef *md, int membernr, FILE *fp)
{
    int a;

    prcode(fp,
"    {\"%s(", sig->cppname);

    for (a = 0; a < sig->cppsig->nrArgs; ++a)
    {
        argDef arg = sig->cppsig->args[a];

        if (a > 0)
            prcode(fp,",");

        /* Do some normalisation so that Qt doesn't have to. */
        if (isConstArg(&arg) && isReference(&arg))
        {
            resetIsConstArg(&arg);
            resetIsReference(&arg);
        }

        generateNamedBaseType(cd->iff, &arg, "", TRUE, fp);
    }

    prcode(fp,")\", ");

    if (docstrings)
    {
        if (md->docstring != NULL)
        {
            generateExplicitDocstring(md->docstring, fp);
        }
        else
        {
            fprintf(fp, "\"\\1");
            prScopedPythonName(fp, cd->ecd, cd->pyname->text);
            fprintf(fp, ".%s", md->pyname->text);
            prPythonSignature(pt, fp, &sig->pysig, FALSE, FALSE, FALSE, FALSE,
                    TRUE);
            fprintf(fp, "\"");
        }

        fprintf(fp, ", ");
    }
    else
    {
        prcode(fp, "0, ");
    }

    if (membernr >= 0)
        prcode(fp, "&methods_%L[%d]", cd->iff, membernr);
    else
        prcode(fp, "0");

    prcode(fp,"},\n"
        );
}


/*
 * Return TRUE if the slot is specific to Python v2.
 */
static int py2OnlySlot(slotType st)
{
    /*
     * Note that we place interpretations on div_slot and idiv_slot for Python
     * v3 so they are not included.
     */
    return (st == long_slot || st == cmp_slot);
}


/*
 * Return TRUE if the slot is specific to Python v2.5 and later.
 */
static int py2_5LaterSlot(slotType st)
{
    return (st == index_slot);
}


/*
 * Return the sip module's string equivalent of a slot.
 */
static const char *slotName(slotType st)
{
    const char *sn;

    switch (st)
    {
    case str_slot:
        sn = "str_slot";
        break;

    case int_slot:
        sn = "int_slot";
        break;

    case long_slot:
        sn = "long_slot";
        break;

    case float_slot:
        sn = "float_slot";
        break;

    case len_slot:
        sn = "len_slot";
        break;

    case contains_slot:
        sn = "contains_slot";
        break;

    case add_slot:
        sn = "add_slot";
        break;

    case concat_slot:
        sn = "concat_slot";
        break;

    case sub_slot:
        sn = "sub_slot";
        break;

    case mul_slot:
        sn = "mul_slot";
        break;

    case repeat_slot:
        sn = "repeat_slot";
        break;

    case div_slot:
        sn = "div_slot";
        break;

    case mod_slot:
        sn = "mod_slot";
        break;

    case floordiv_slot:
        sn = "floordiv_slot";
        break;

    case truediv_slot:
        sn = "truediv_slot";
        break;

    case and_slot:
        sn = "and_slot";
        break;

    case or_slot:
        sn = "or_slot";
        break;

    case xor_slot:
        sn = "xor_slot";
        break;

    case lshift_slot:
        sn = "lshift_slot";
        break;

    case rshift_slot:
        sn = "rshift_slot";
        break;

    case iadd_slot:
        sn = "iadd_slot";
        break;

    case iconcat_slot:
        sn = "iconcat_slot";
        break;

    case isub_slot:
        sn = "isub_slot";
        break;

    case imul_slot:
        sn = "imul_slot";
        break;

    case irepeat_slot:
        sn = "irepeat_slot";
        break;

    case idiv_slot:
        sn = "idiv_slot";
        break;

    case imod_slot:
        sn = "imod_slot";
        break;

    case ifloordiv_slot:
        sn = "ifloordiv_slot";
        break;

    case itruediv_slot:
        sn = "itruediv_slot";
        break;

    case iand_slot:
        sn = "iand_slot";
        break;

    case ior_slot:
        sn = "ior_slot";
        break;

    case ixor_slot:
        sn = "ixor_slot";
        break;

    case ilshift_slot:
        sn = "ilshift_slot";
        break;

    case irshift_slot:
        sn = "irshift_slot";
        break;

    case invert_slot:
        sn = "invert_slot";
        break;

    case call_slot:
        sn = "call_slot";
        break;

    case getitem_slot:
        sn = "getitem_slot";
        break;

    case setitem_slot:
        sn = "setitem_slot";
        break;

    case delitem_slot:
        sn = "delitem_slot";
        break;

    case lt_slot:
        sn = "lt_slot";
        break;

    case le_slot:
        sn = "le_slot";
        break;

    case eq_slot:
        sn = "eq_slot";
        break;

    case ne_slot:
        sn = "ne_slot";
        break;

    case gt_slot:
        sn = "gt_slot";
        break;

    case ge_slot:
        sn = "ge_slot";
        break;

    case cmp_slot:
        sn = "cmp_slot";
        break;

    case bool_slot:
        sn = "bool_slot";
        break;

    case neg_slot:
        sn = "neg_slot";
        break;

    case pos_slot:
        sn = "pos_slot";
        break;

    case abs_slot:
        sn = "abs_slot";
        break;

    case repr_slot:
        sn = "repr_slot";
        break;

    case hash_slot:
        sn = "hash_slot";
        break;

    case index_slot:
        sn = "index_slot";
        break;

    case iter_slot:
        sn = "iter_slot";
        break;

    case next_slot:
        sn = "next_slot";
        break;

    case setattr_slot:
        sn = "setattr_slot";
        break;

    default:
        sn = NULL;
    }

    return sn;
}


/*
 * Generate the initialisation function or cast operators for the type.
 */
static void generateTypeInit(classDef *cd, moduleDef *mod, FILE *fp)
{
    ctorDef *ct;
    int need_self, need_owner;

    /*
     * See if we need to name the self and owner arguments so that we can avoid
     * a compiler warning about an unused argument.
     */
    need_self = (generating_c || hasShadow(cd));
    need_owner = generating_c;

    for (ct = cd->ctors; ct != NULL; ct = ct->next)
    {
        if (usedInCode(ct->methodcode, "sipSelf"))
            need_self = TRUE;

        if (isResultTransferredCtor(ct))
            need_owner = TRUE;
        else
        {
            int a;

            for (a = 0; a < ct->pysig.nrArgs; ++a)
            {
                argDef *ad = &ct->pysig.args[a];

                if (!isInArg(ad))
                    continue;

                if (keepReference(ad))
                    need_self = TRUE;

                if (isThisTransferred(ad))
                    need_owner = TRUE;
            }
        }
    }

    prcode(fp,
"\n"
"\n"
        );

    if (!generating_c)
        prcode(fp,
"extern \"C\" {static void *init_%L(sipSimpleWrapper *, PyObject *, PyObject *, PyObject **, PyObject **, PyObject **);}\n"
            , cd->iff);

    prcode(fp,
"static void *init_%L(sipSimpleWrapper *%s, PyObject *sipArgs, PyObject *sipKwds, PyObject **sipUnused, PyObject **%s, PyObject **sipParseErr)\n"
"{\n"
        , cd->iff, (need_self ? "sipSelf" : ""), (need_owner ? "sipOwner" : ""));

    if (hasShadow(cd))
        prcode(fp,
"    sip%C *sipCpp = 0;\n"
            ,classFQCName(cd));
    else
        prcode(fp,
"    %U *sipCpp = 0;\n"
            ,cd);

    if (tracing)
        prcode(fp,
"\n"
"    sipTrace(SIP_TRACE_INITS,\"init_%L()\\n\");\n"
            , cd->iff);

    /*
     * Generate the code that parses the Python arguments and calls the
     * correct constructor.
     */
    for (ct = cd->ctors; ct != NULL; ct = ct->next)
    {
        int needSecCall, error_flag, old_error_flag;
        apiVersionRangeDef *avr;

        if (isPrivateCtor(ct))
            continue;

        avr = ct->api_range;

        prcode(fp,
"\n"
            );

        if (avr != NULL)
            prcode(fp,
"    if (sipIsAPIEnabled(%N, %d, %d))\n"
                , avr->api_name, avr->from, avr->to);

        prcode(fp,
"    {\n"
            );

        if (ct->methodcode != NULL)
        {
            error_flag = needErrorFlag(ct->methodcode);
            old_error_flag = needOldErrorFlag(ct->methodcode);
        }
        else
        {
            error_flag = old_error_flag = FALSE;
        }

        needSecCall = generateArgParser(mod, &ct->pysig, cd, NULL, ct, NULL,
                FALSE, fp);
        generateConstructorCall(cd, ct, error_flag, old_error_flag, mod, fp);

        if (needSecCall)
        {
            prcode(fp,
"    }\n"
"\n"
                );

            if (avr != NULL)
                prcode(fp,
"    if (sipIsAPIEnabled(%N, %d, %d))\n"
                    , avr->api_name, avr->from, avr->to);

            prcode(fp,
"    {\n"
                );

            generateArgParser(mod, &ct->pysig, cd, NULL, ct, NULL, TRUE, fp);
            generateConstructorCall(cd, ct, error_flag, old_error_flag, mod,
                    fp);
        }

        prcode(fp,
"    }\n"
            );
    }

    prcode(fp,
"\n"
"    return NULL;\n"
"}\n"
        );
}


/*
 * Count the number of virtual members in a class.
 */
static int countVirtuals(classDef *cd)
{
    int nrvirts;
    virtOverDef *vod;
 
    nrvirts = 0;
 
    for (vod = cd->vmembers; vod != NULL; vod = vod->next)
        if (!isPrivate(&vod->o))
            ++nrvirts;
 
    return nrvirts;
}

 
/*
 * Generate the try block for a call.
 */
static void generateTry(throwArgs *ta,FILE *fp)
{
    /*
     * Generate the block if there was no throw specifier, or a non-empty
     * throw specifier.
     */
    if (exceptions && (ta == NULL || ta->nrArgs > 0))
        prcode(fp,
"            try\n"
"            {\n"
            );
}


/*
 * Generate the catch blocks for a call.
 */
static void generateCatch(throwArgs *ta, signatureDef *sd, moduleDef *mod,
        FILE *fp, int rgil)
{
    /*
     * Generate the block if there was no throw specifier, or a non-empty
     * throw specifier.
     */
    if (exceptions && (ta == NULL || ta->nrArgs > 0))
    {
        prcode(fp,
"            }\n"
            );

        if (ta != NULL)
        {
            int a;

            for (a = 0; a < ta->nrArgs; ++a)
                generateCatchBlock(mod, ta->args[a], sd, fp, rgil);
        }
        else if (mod->defexception != NULL)
        {
            generateCatchBlock(mod, mod->defexception, sd, fp, rgil);
        }

        prcode(fp,
"            catch (...)\n"
"            {\n"
            );

        if (rgil)
            prcode(fp,
"                Py_BLOCK_THREADS\n"
"\n"
                );

        deleteOuts(mod, sd, fp);
        deleteTemps(mod, sd, fp);

        prcode(fp,
"                sipRaiseUnknownException();\n"
"                return NULL;\n"
"            }\n"
            );
    }
}


/*
 * Generate a single catch block.
 */
static void generateCatchBlock(moduleDef *mod, exceptionDef *xd,
        signatureDef *sd, FILE *fp, int rgil)
{
    scopedNameDef *ename = xd->iff->fqcname;

    prcode(fp,
"            catch (%S &%s)\n"
"            {\n"
        ,ename,(xd->cd != NULL || usedInCode(xd->raisecode, "sipExceptionRef")) ? "sipExceptionRef" : "");

    if (rgil)
        prcode(fp,
"\n"
"                Py_BLOCK_THREADS\n"
            );

    deleteOuts(mod, sd, fp);
    deleteTemps(mod, sd, fp);

    /* See if the exception is a wrapped class. */
    if (xd->cd != NULL)
        prcode(fp,
"                /* Hope that there is a valid copy ctor. */\n"
"                %S *sipExceptionCopy = new %S(sipExceptionRef);\n"
"\n"
"                sipRaiseTypeException(sipType_%C,sipExceptionCopy);\n"
            , ename, ename
            , ename);
    else
        generateCppCodeBlock(xd->raisecode, fp);

    prcode(fp,
"\n"
"                return NULL;\n"
"            }\n"
        );
}


/*
 * Generate a throw specifier.
 */
static void generateThrowSpecifier(throwArgs *ta,FILE *fp)
{
    if (exceptions && ta != NULL)
    {
        int a;

        prcode(fp," throw(");

        for (a = 0; a < ta->nrArgs; ++a)
        {
            if (a > 0)
                prcode(fp,",");

            prcode(fp,"%S",ta->args[a]->iff->fqcname);
        }

        prcode(fp,")");
    }
}


/*
 * Generate a single constructor call.
 */
static void generateConstructorCall(classDef *cd, ctorDef *ct, int error_flag,
        int old_error_flag, moduleDef *mod, FILE *fp)
{
    prcode(fp,
"        {\n"
        );

    if (error_flag)
        prcode(fp,
"            sipErrorState sipError = sipErrorNone;\n"
"\n"
            );
    else if (old_error_flag)
        prcode(fp,
"            int sipIsErr = 0;\n"
"\n"
            );

    if (isDeprecatedCtor(ct))
        /* Note that any temporaries will leak if an exception is raised. */
        prcode(fp,
"            if (sipDeprecated(%N,NULL) < 0)\n"
"                return NULL;\n"
"\n"
            , cd->pyname);

    /* Call any pre-hook. */
    if (ct->prehook != NULL)
        prcode(fp,
"            sipCallHook(\"%s\");\n"
"\n"
            ,ct->prehook);

    if (ct->methodcode != NULL)
        generateCppCodeBlock(ct->methodcode,fp);
    else if (generating_c)
        prcode(fp,
"            sipCpp = sipMalloc(sizeof (struct %S));\n"
            ,classFQCName(cd));
    else
    {
        int a;
        int rgil = ((release_gil || isReleaseGILCtor(ct)) && !isHoldGILCtor(ct));

        if (raisesPyExceptionCtor(ct))
            prcode(fp,
"            PyErr_Clear();\n"
"\n"
                );

        if (rgil)
            prcode(fp,
"            Py_BEGIN_ALLOW_THREADS\n"
                );

        generateTry(ct->exceptions,fp);

        if (hasShadow(cd))
            prcode(fp,
"            sipCpp = new sip%C(",classFQCName(cd));
        else
            prcode(fp,
"            sipCpp = new %U(",cd);

        if (isCastCtor(ct))
        {
            classDef *ocd;

            /* We have to fiddle the type to generate the correct code. */
            ocd = ct->pysig.args[0].u.cd;
            ct->pysig.args[0].u.cd = cd;
            prcode(fp, "a0->operator %B()", &ct->pysig.args[0]);
            ct->pysig.args[0].u.cd = ocd;
        }
        else
            generateCallArgs(mod, ct->cppsig, &ct->pysig, fp);

        prcode(fp,");\n"
            );

        generateCatch(ct->exceptions, &ct->pysig, mod, fp, rgil);

        if (rgil)
            prcode(fp,
"            Py_END_ALLOW_THREADS\n"
                );

        /* Handle any /KeepReference/ arguments. */
        for (a = 0; a < ct->pysig.nrArgs; ++a)
        {
            argDef *ad = &ct->pysig.args[a];

            if (!isInArg(ad))
                continue;

            if (keepReference(ad))
            {
                prcode(fp,
"\n"
"            sipKeepReference((PyObject *)sipSelf, %d, %a%s);\n"
                    , ad->key, mod, ad, a, (((ad->atype == ascii_string_type || ad->atype == latin1_string_type || ad->atype == utf8_string_type) && ad->nrderefs == 1) || !isGetWrapper(ad) ? "Keep" : "Wrapper"));
            }
        }

        /*
         * This is a bit of a hack to say we want the result transferred.  We
         * don't simply call sipTransferTo() because the wrapper object hasn't
         * been fully initialised yet.
         */
        if (isResultTransferredCtor(ct))
            prcode(fp,
"\n"
"            *sipOwner = Py_None;\n"
                );
    }

    gc_ellipsis(&ct->pysig, fp);

    deleteTemps(mod, &ct->pysig, fp);

    prcode(fp,
"\n"
        );

    if (raisesPyExceptionCtor(ct))
    {
        prcode(fp,
"            if (PyErr_Occurred())\n"
"            {\n"
"                delete sipCpp;\n"
"                return NULL;\n"
"            }\n"
"\n"
                );
    }

    if (error_flag)
    {
        prcode(fp,
"            if (sipError == sipErrorNone)\n"
            );

        if (hasShadow(cd) || ct->posthook != NULL)
            prcode(fp,
"            {\n"
                );

        if (hasShadow(cd))
            prcode(fp,
"                sipCpp->sipPySelf = sipSelf;\n"
"\n"
                );

        /* Call any post-hook. */
        if (ct->posthook != NULL)
            prcode(fp,
"            sipCallHook(\"%s\");\n"
"\n"
                , ct->posthook);

        prcode(fp,
"                return sipCpp;\n"
            );

        if (hasShadow(cd) || ct->posthook != NULL)
            prcode(fp,
"            }\n"
                );

        prcode(fp,
"\n"
"            if (sipUnused)\n"
"            {\n"
"                Py_XDECREF(*sipUnused);\n"
"            }\n"
"\n"
"            sipAddException(sipError, sipParseErr);\n"
"\n"
"            if (sipError == sipErrorFail)\n"
"                return NULL;\n"
            );
    }
    else
    {
        if (old_error_flag)
        {
            prcode(fp,
"            if (sipIsErr)\n"
"            {\n"
"                if (sipUnused)\n"
"                {\n"
"                    Py_XDECREF(*sipUnused);\n"
"                }\n"
"\n"
"                sipAddException(sipErrorFail, sipParseErr);\n"
"                return NULL;\n"
"            }\n"
"\n"
                );
        }

        if (hasShadow(cd))
            prcode(fp,
"            sipCpp->sipPySelf = sipSelf;\n"
"\n"
                );

        /* Call any post-hook. */
        if (ct->posthook != NULL)
            prcode(fp,
"            sipCallHook(\"%s\");\n"
"\n"
                , ct->posthook);

        prcode(fp,
"            return sipCpp;\n"
            );
    }

    prcode(fp,
"        }\n"
        );
}


/*
 * See if a member overload should be skipped.
 */
static int skipOverload(overDef *od,memberDef *md,classDef *cd,classDef *ccd,
            int want_local)
{
    /* Skip if it's not the right name. */
    if (od->common != md)
        return TRUE;

    /* Skip if it's a signal. */
    if (isSignal(od))
        return TRUE;

    /* Skip if it's a private abstract. */
    if (isAbstract(od) && isPrivate(od))
        return TRUE;

    /*
     * If we are disallowing them, skip if it's not in the current class unless
     * it is protected.
     */
    if (want_local && !isProtected(od) && ccd != cd)
        return TRUE;

    return FALSE;
}


/*
 * Generate a class member function.
 */
static void generateFunction(sipSpec *pt, memberDef *md, overDef *overs,
        classDef *cd, classDef *ocd, moduleDef *mod, FILE *fp)
{
    overDef *od;
    int need_method, need_self, need_args, need_selfarg, need_orig_self;

    /*
     * Check that there is at least one overload that needs to be handled.  See
     * if we can avoid naming the "self" argument (and suppress a compiler
     * warning).  See if we need to remember if "self" was explicitly passed as
     * an argument.  See if we need to handle keyword arguments.
     */
    need_method = need_self = need_args = need_selfarg = need_orig_self = FALSE;

    for (od = overs; od != NULL; od = od->next)
    {
        /* Skip protected methods if we don't have the means to handle them. */
        if (isProtected(od) && !hasShadow(cd))
            continue;

        if (!skipOverload(od,md,cd,ocd,TRUE))
        {
            need_method = TRUE;

            if (!isPrivate(od))
            {
                need_args = TRUE;

                if (!isStatic(od))
                {
                    need_self = TRUE;

                    if (isAbstract(od))
                        need_orig_self = TRUE;
                    else if (isVirtual(od) || isVirtualReimp(od) || usedInCode(od->methodcode, "sipSelfWasArg"))
                        need_selfarg = TRUE;
                }
            }
        }
    }

    if (need_method)
    {
        const char *pname = md->pyname->text;
        int has_auto_docstring;

        prcode(fp,
"\n"
"\n"
            );

        /* Generate the docstrings. */
        has_auto_docstring = FALSE;

        if (md->docstring != NULL || (docstrings && hasDocstring(pt, overs, md, cd->iff)))
        {
            prcode(fp,
"PyDoc_STRVAR(doc_%L_%s, " , cd->iff, pname);

            if (md->docstring != NULL)
            {
                generateExplicitDocstring(md->docstring, fp);
            }
            else
            {
                generateDocstring(pt, overs, md, cd->pyname->text, cd->ecd, fp);
                has_auto_docstring = TRUE;
            }

            prcode(fp, ");\n"
"\n"
                );
        }

        if (!generating_c)
            prcode(fp,
"extern \"C\" {static PyObject *meth_%L_%s(PyObject *, PyObject *%s);}\n"
            , cd->iff, pname, (noArgParser(md) || useKeywordArgs(md) ? ", PyObject *" : ""));

        prcode(fp,
"static PyObject *meth_%L_%s(PyObject *%s, PyObject *%s%s)\n"
"{\n"
            , cd->iff, pname, (need_self ? "sipSelf" : ""), (need_args ? "sipArgs" : ""), (noArgParser(md) || useKeywordArgs(md) ? ", PyObject *sipKwds" : ""));

        if (tracing)
            prcode(fp,
"    sipTrace(SIP_TRACE_METHODS,\"meth_%L_%s()\\n\");\n"
"\n"
                , cd->iff, pname);

        if (!noArgParser(md))
        {
            if (need_args)
                prcode(fp,
"    PyObject *sipParseErr = NULL;\n"
                    );

            if (need_selfarg)
            {
                /*
                 * This determines if we call the explicitly scoped version or
                 * the unscoped version (which will then go via the vtable).
                 *
                 * - If the call was unbound and self was passed as the first
                 *   argument (ie. Foo.meth(self)) then we always want to call
                 *   the explicitly scoped version.
                 *
                 * - If the call was bound then we only call the unscoped
                 *   version in case there is a C++ sub-class reimplementation
                 *   that Python knows nothing about.  Otherwise, if the call
                 *   was invoked by super() within a Python reimplementation
                 *   then the Python reimplementation would be called
                 *   recursively.
                 *
                 * Note that we would like to rename 'sipSelfWasArg' to
                 * 'sipExplicitScope' but it is part of the public API.
                 */
                prcode(fp,
"    bool sipSelfWasArg = (!sipSelf || sipIsDerived((sipSimpleWrapper *)sipSelf));\n"
                    );
            }

            if (need_orig_self)
            {
                /*
                 * This is similar to the above but for abstract methods.  We
                 * allow the (potential) recursion because it means that the
                 * concrete implementation can be put in a mixin and it will
                 * all work.
                 */
                prcode(fp,
"    PyObject *sipOrigSelf = sipSelf;\n"
                    );
            }
        }

        for (od = overs; od != NULL; od = od->next)
        {
            /* If we are handling one variant then we must handle them all. */
            if (skipOverload(od, md, cd, ocd, FALSE))
                continue;

            if (isPrivate(od))
                continue;

            if (noArgParser(md))
            {
                generateCppCodeBlock(od->methodcode, fp);
                break;
            }

            generateFunctionBody(od, cd, NULL, ocd, TRUE, mod, fp);
        }

        if (!noArgParser(md))
        {
            prcode(fp,
"\n"
"    /* Raise an exception if the arguments couldn't be parsed. */\n"
"    sipNoMethod(%s, %N, %N, ", (need_args ? "sipParseErr" : "NULL"), cd->pyname, md->pyname);

            if (has_auto_docstring)
                prcode(fp, "doc_%L_%s", cd->iff, pname);
            else
                prcode(fp, "NULL");

            prcode(fp, ");\n"
"\n"
"    return NULL;\n"
                );
        }

        prcode(fp,
"}\n"
            );
    }
}


/*
 * Generate the function calls for a particular overload.
 */
static void generateFunctionBody(overDef *od, classDef *c_scope,
        mappedTypeDef *mt_scope, classDef *ocd, int deref, moduleDef *mod,
        FILE *fp)
{
    int needSecCall;
    signatureDef saved;
    ifaceFileDef *o_scope;
    apiVersionRangeDef *avr;

    if (mt_scope != NULL)
        o_scope = mt_scope->iff;
    else if (ocd != NULL)
        o_scope = ocd->iff;
    else
        o_scope = NULL;

    if (o_scope != NULL)
        avr = od->api_range;
    else
        avr = NULL;

    if (avr != NULL)
        prcode(fp,
"\n"
"    if (sipIsAPIEnabled(%N, %d, %d))\n"
"    {\n"
            , avr->api_name, avr->from, avr->to);
    else
        prcode(fp,
"\n"
"    {\n"
            );

    /* In case we have to fiddle with it. */
    saved = od->pysig;

    if (isNumberSlot(od->common))
    {
        /*
         * Number slots must have two arguments because we parse them slightly
         * differently.
         */
        if (od->pysig.nrArgs == 1)
        {
            od->pysig.nrArgs = 2;
            od->pysig.args[1] = od->pysig.args[0];

            /* Insert self in the right place. */
            od->pysig.args[0].atype = class_type;
            od->pysig.args[0].name = NULL;
            od->pysig.args[0].argflags = ARG_IS_REF|ARG_IN;
            od->pysig.args[0].nrderefs = 0;
            od->pysig.args[0].defval = NULL;
            od->pysig.args[0].original_type = NULL;
            od->pysig.args[0].u.cd = ocd;
        }

        generateArgParser(mod, &od->pysig, c_scope, mt_scope, NULL, od, FALSE,
                fp);
        needSecCall = FALSE;
    }
    else if (isIntArgSlot(od->common) || isZeroArgSlot(od->common))
        needSecCall = FALSE;
    else
        needSecCall = generateArgParser(mod, &od->pysig, c_scope, mt_scope,
                NULL, od, FALSE, fp);

    generateFunctionCall(c_scope, mt_scope, o_scope, od, deref, mod, fp);

    if (needSecCall)
    {
        prcode(fp,
"    }\n"
"\n"
"    {\n"
            );

        generateArgParser(mod, &od->pysig, c_scope, mt_scope, NULL, od, TRUE,
                fp);
        generateFunctionCall(c_scope, mt_scope, o_scope, od, deref, mod, fp);
    }

    prcode(fp,
"    }\n"
        );

    od->pysig = saved;
}


/*
 * Generate the code to handle the result of a call to a member function.
 */
static void generateHandleResult(moduleDef *mod, overDef *od, int isNew,
        int result_size, char *prefix, FILE *fp)
{
    const char *vname;
    char vnamebuf[50];
    int a, nrvals, only, has_owner;
    argDef *res, *ad;

    res = &od->pysig.result;

    if (res->atype == void_type && res->nrderefs == 0)
        res = NULL;

    /* See if we are returning 0, 1 or more values. */
    nrvals = 0;

    if (res != NULL)
    {
        only = -1;
        ++nrvals;
    }

    has_owner = FALSE;

    for (a = 0; a < od->pysig.nrArgs; ++a)
    {
        if (isOutArg(&od->pysig.args[a]))
        {
            only = a;
            ++nrvals;
        }

        if (isThisTransferred(&od->pysig.args[a]))
            has_owner = TRUE;
    }

    /* Handle the trivial case. */
    if (nrvals == 0)
    {
        prcode(fp,
"            Py_INCREF(Py_None);\n"
"            %s Py_None;\n"
            ,prefix);

        return;
    }

    /* Handle results that are classes or mapped types separately. */
    if (res != NULL)
    {
        ifaceFileDef *iff;

        if (res->atype == mapped_type)
            iff = res->u.mtd->iff;
        else if (res->atype == class_type)
            iff = res->u.cd->iff;
        else
            iff = NULL;

        if (iff != NULL)
        {
            if (isNew || isFactory(od))
            {
                prcode(fp,
"            %s sipConvertFromNewType(",(nrvals == 1 ? prefix : "PyObject *sipResObj ="));

                if (isConstArg(res))
                    prcode(fp,"const_cast<%b *>(sipRes)",res);
                else
                    prcode(fp,"sipRes");

                prcode(fp,",sipType_%C,%s);\n"
                    , iff->fqcname, ((has_owner && isFactory(od)) ? "(PyObject *)sipOwner" : resultOwner(od)));

                /*
                 * Shortcut if this is the only value returned.
                 */
                if (nrvals == 1)
                    return;
            }
            else
            {
                prcode(fp,
"            %s sipConvertFromType(",(nrvals == 1 ? prefix : "PyObject *sipResObj ="));

                if (isConstArg(res))
                    prcode(fp,"const_cast<%b *>(sipRes)",res);
                else
                    prcode(fp,"sipRes");

                prcode(fp, ",sipType_%C,%s);\n"
                    , iff->fqcname, resultOwner(od));

                /*
                 * Shortcut if this is the only value returned.
                 */
                if (nrvals == 1)
                    return;
            }
        }
    }

    /* If there are multiple values then build a tuple. */
    if (nrvals > 1)
    {
        prcode(fp,
"            %s sipBuildResult(0,\"(",prefix);

        /* Build the format string. */
        if (res != NULL)
            prcode(fp, "%s", ((res->atype == mapped_type || res->atype == class_type) ? "R" : getBuildResultFormat(res)));

        for (a = 0; a < od->pysig.nrArgs; ++a)
        {
            argDef *ad = &od->pysig.args[a];

            if (isOutArg(ad))
                prcode(fp, "%s", getBuildResultFormat(ad));
        }

        prcode(fp,")\"");

        /* Pass the values for conversion. */
        if (res != NULL)
        {
            prcode(fp, ",sipRes");

            if (res->atype == mapped_type || res->atype == class_type)
                prcode(fp, "Obj");
            else if (res->atype == enum_type && res->u.ed->fqcname != NULL)
                prcode(fp, ",sipType_%C", res->u.ed->fqcname);
        }

        for (a = 0; a < od->pysig.nrArgs; ++a)
        {
            argDef *ad = &od->pysig.args[a];

            if (isOutArg(ad))
            {
                prcode(fp, ",%a", mod, ad, a);

                if (ad->atype == mapped_type)
                    prcode(fp, ",sipType_%T,%s", ad, (isTransferredBack(ad) ? "Py_None" : "NULL"));
                else if (ad->atype == class_type)
                    prcode(fp, ",sipType_%C,%s", classFQCName(ad->u.cd), (isTransferredBack(ad) ? "Py_None" : "NULL"));
                else if (ad->atype == enum_type && ad->u.ed->fqcname != NULL)
                    prcode(fp,",sipType_%C", ad->u.ed->fqcname);
            }
        }

        prcode(fp,");\n"
            );

        /* All done for multiple values. */
        return;
    }

    /* Deal with the only returned value. */
    if (only < 0)
    {
        ad = res;
        vname = "sipRes";
    }
    else
    {
        ad = &od->pysig.args[only];

        if (useArgNames(mod) && ad->name != NULL)
        {
            vname = ad->name->text;
        }
        else
        {
            sprintf(vnamebuf, "a%d", only);
            vname = vnamebuf;
        }
    }

    switch (ad->atype)
    {
    case mapped_type:
    case class_type:
        {
            int needNew = needNewInstance(ad);
            ifaceFileDef *iff;

            if (ad->atype == mapped_type)
                iff = ad->u.mtd->iff;
            else
                iff = ad->u.cd->iff;

            prcode(fp,
"            %s sipConvertFrom%sType(", prefix, (needNew ? "New" : ""));

            if (isConstArg(ad))
                prcode(fp,"const_cast<%b *>(%s)",ad,vname);
            else
                prcode(fp,"%s",vname);

            prcode(fp, ",sipType_%C,", iff->fqcname);

            if (needNew || !isTransferredBack(ad))
                prcode(fp, "NULL);\n");
            else
                prcode(fp, "Py_None);\n");
        }

        break;

    case bool_type:
    case cbool_type:
        prcode(fp,
"            %s PyBool_FromLong(%s);\n"
            ,prefix,vname);

        break;

    case ascii_string_type:
        if (ad->nrderefs == 0)
            prcode(fp,
"            %s PyUnicode_DecodeASCII(&%s, 1, NULL);\n"
                , prefix, vname);
        else
            prcode(fp,
"            if (%s == NULL)\n"
"            {\n"
"                Py_INCREF(Py_None);\n"
"                return Py_None;\n"
"            }\n"
"\n"
"            %s PyUnicode_DecodeASCII(%s, strlen(%s), NULL);\n"
            , vname
            , prefix, vname, vname);

        break;

    case latin1_string_type:
        if (ad->nrderefs == 0)
            prcode(fp,
"            %s PyUnicode_DecodeLatin1(&%s, 1, NULL);\n"
                , prefix, vname);
        else
            prcode(fp,
"            if (%s == NULL)\n"
"            {\n"
"                Py_INCREF(Py_None);\n"
"                return Py_None;\n"
"            }\n"
"\n"
"            %s PyUnicode_DecodeLatin1(%s, strlen(%s), NULL);\n"
            , vname
            , prefix, vname, vname);

        break;

    case utf8_string_type:
        if (ad->nrderefs == 0)
            prcode(fp,
"#if PY_MAJOR_VERSION >= 3\n"
"            %s PyUnicode_FromStringAndSize(&%s, 1);\n"
"#else\n"
"            %s PyUnicode_DecodeUTF8(&%s, 1, NULL);\n"
"#endif\n"
                , prefix, vname
                , prefix, vname);
        else
            prcode(fp,
"            if (%s == NULL)\n"
"            {\n"
"                Py_INCREF(Py_None);\n"
"                return Py_None;\n"
"            }\n"
"\n"
"#if PY_MAJOR_VERSION >= 3\n"
"            %s PyUnicode_FromString(%s);\n"
"#else\n"
"            %s PyUnicode_DecodeUTF8(%s, strlen(%s), NULL);\n"
"#endif\n"
            , vname
            , prefix, vname
            , prefix, vname, vname);

        break;

    case sstring_type:
    case ustring_type:
    case string_type:
        if (ad->nrderefs == 0)
            prcode(fp,
"            %s SIPBytes_FromStringAndSize(%s&%s,1);\n"
                ,prefix,(ad->atype != string_type) ? "(char *)" : "",vname);
        else
            prcode(fp,
"            if (%s == NULL)\n"
"            {\n"
"                Py_INCREF(Py_None);\n"
"                return Py_None;\n"
"            }\n"
"\n"
"            %s SIPBytes_FromString(%s%s);\n"
            ,vname
            ,prefix,(ad->atype != string_type) ? "(char *)" : "",vname);

        break;

    case wstring_type:
        if (ad->nrderefs == 0)
            prcode(fp,
"            %s PyUnicode_FromWideChar(&%s,1);\n"
                , prefix, vname);
        else
            prcode(fp,
"            if (%s == NULL)\n"
"            {\n"
"                Py_INCREF(Py_None);\n"
"                return Py_None;\n"
"            }\n"
"\n"
"            %s PyUnicode_FromWideChar(%s,(SIP_SSIZE_T)wcslen(%s));\n"
            , vname
            , prefix, vname, vname);

        break;

    case enum_type:
        if (ad->u.ed->fqcname != NULL)
        {
            prcode(fp,
"            %s sipConvertFromEnum(%s,sipType_%C);\n"
                , prefix, vname, ad->u.ed->fqcname);

            break;
        }

        /* Drop through. */

    case byte_type:
    case sbyte_type:
    case short_type:
    case int_type:
    case cint_type:
        prcode(fp,
"            %s SIPLong_FromLong(%s);\n"
            ,prefix,vname);

        break;

    case long_type:
        prcode(fp,
"            %s PyLong_FromLong(%s);\n"
            ,prefix,vname);

        break;

    case ubyte_type:
    case ushort_type:
        prcode(fp,
"#if PY_MAJOR_VERSION >= 3\n"
"            %s PyLong_FromUnsignedLong(%s);\n"
"#else\n"
"            %s PyInt_FromLong(%s);\n"
"#endif\n"
            , prefix, vname
            , prefix, vname);

        break;

    case uint_type:
    case ulong_type:
        prcode(fp,
"            %s PyLong_FromUnsignedLong(%s);\n"
            , prefix, vname);

        break;

    case longlong_type:
        prcode(fp,
"            %s PyLong_FromLongLong(%s);\n"
            ,prefix,vname);

        break;

    case ulonglong_type:
        prcode(fp,
"            %s PyLong_FromUnsignedLongLong(%s);\n"
            ,prefix,vname);

        break;

    case void_type:
        {
            prcode(fp,
"            %s sipConvertFrom%sVoidPtr", prefix, (isConstArg(ad) ? "Const" : ""));

            if (result_size < 0)
            {
                prcode(fp, "(");
                generateVoidPtrCast(ad, fp);
                prcode(fp, "%s", vname);
            }
            else
            {
                prcode(fp, "AndSize(");
                generateVoidPtrCast(ad, fp);
                prcode(fp, "%s,%a", vname, mod, &od->pysig.args[result_size], result_size);
            }

            prcode(fp, ");\n"
                    );
        }

        break;

    case capsule_type:
        prcode(fp,
"            %s SIPCapsule_FromVoidPtr(%s, \"%S\");\n"
            , prefix, vname, ad->u.cap);
        break;

    case struct_type:
        prcode(fp,
"            %s sipConvertFrom%sVoidPtr(%s);\n"
            , prefix, (isConstArg(ad) ? "Const" : ""), vname);
        break;

    case float_type:
    case cfloat_type:
        prcode(fp,
"            %s PyFloat_FromDouble((double)%s);\n"
            ,prefix,vname);

        break;

    case double_type:
    case cdouble_type:
        prcode(fp,
"            %s PyFloat_FromDouble(%s);\n"
            ,prefix,vname);

        break;

    case pyobject_type:
    case pytuple_type:
    case pylist_type:
    case pydict_type:
    case pycallable_type:
    case pyslice_type:
    case pytype_type:
    case pybuffer_type:
        prcode(fp,
"            %s %s;\n"
            ,prefix,vname);

        break;
    }
}


/*
 * Return the owner of a method result.
 */
static const char *resultOwner(overDef *od)
{
    if (isResultTransferredBack(od))
        return "Py_None";

    if (isResultTransferred(od))
        return "sipSelf";

    return "NULL";
}


/*
 * Return the format string used by sipBuildResult() for a particular type.
 */
static const char *getBuildResultFormat(argDef *ad)
{
    switch (ad->atype)
    {
    case fake_void_type:
    case mapped_type:
    case class_type:
        if (needNewInstance(ad))
            return "N";

        return "D";

    case bool_type:
    case cbool_type:
        return "b";

    case ascii_string_type:
        return (ad->nrderefs > (isOutArg(ad) ? 1 : 0)) ? "AA" : "aA";

    case latin1_string_type:
        return (ad->nrderefs > (isOutArg(ad) ? 1 : 0)) ? "AL" : "aL";

    case utf8_string_type:
        return (ad->nrderefs > (isOutArg(ad) ? 1 : 0)) ? "A8" : "a8";

    case sstring_type:
    case ustring_type:
    case string_type:
        return (ad->nrderefs > (isOutArg(ad) ? 1 : 0)) ? "s" : "c";

    case wstring_type:
        return (ad->nrderefs > (isOutArg(ad) ? 1 : 0)) ? "x" : "w";

    case enum_type:
        return (ad->u.ed->fqcname != NULL) ? "F" : "e";

    case byte_type:
    case sbyte_type:
        return "L";

    case ubyte_type:
        return "M";

    case short_type:
        return "h";

    case ushort_type:
        return "t";

    case int_type:
    case cint_type:
        return "i";

    case uint_type:
        return "u";

    case long_type:
        return "l";

    case ulong_type:
        return "m";

    case longlong_type:
        return "n";

    case ulonglong_type:
        return "o";

    case void_type:
    case struct_type:
        return "V";

    case capsule_type:
        return "z";

    case float_type:
    case cfloat_type:
        return "f";

    case double_type:
    case cdouble_type:
        return "d";

    case pyobject_type:
    case pytuple_type:
    case pylist_type:
    case pydict_type:
    case pycallable_type:
    case pyslice_type:
    case pytype_type:
    case pybuffer_type:
        return "R";
    }

    /* We should never get here. */
    return "";
}


/*
 * Return TRUE if an argument (or result) should be copied because it is a
 * const reference to a type.
 */
static int copyConstRefArg(argDef *ad)
{
    if (!noCopy(ad) && (ad->atype == class_type || ad->atype == mapped_type) && ad->nrderefs == 0)
    {
        /* Make a copy if it is not a reference or it is a const reference. */
        if (!isReference(ad) || isConstArg(ad))
        {
            /* If it is a class then we must be able to copy it. */
            if (ad->atype != class_type || !(cannotCopy(ad->u.cd) || isAbstractClass(ad->u.cd)))
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}


/*
 * Generate a function call.
 */
static void generateFunctionCall(classDef *c_scope, mappedTypeDef *mt_scope,
        ifaceFileDef *o_scope, overDef *od, int deref, moduleDef *mod,
        FILE *fp)
{
    int needsNew, error_flag, old_error_flag, newline, is_result, result_size,
            a, deltemps, post_process, static_factory;
    const char *error_value;
    argDef *res = &od->pysig.result, orig_res;
    ifaceFileDef *scope;
    nameDef *pyname;

    if (mt_scope != NULL)
    {
        scope = mt_scope->iff;
        pyname = mt_scope->pyname;
    }
    else if (c_scope != NULL)
    {
        scope = c_scope->iff;
        pyname = c_scope->pyname;
    }
    else
    {
        scope = NULL;
        pyname = NULL;
    }

    static_factory = ((scope == NULL || isStatic(od)) && isFactory(od));

    prcode(fp,
"        {\n"
        );

    /*
     * If there is no shadow class then protected methods can never be called.
     */
    if (isProtected(od) && !hasShadow(c_scope))
    {
        prcode(fp,
"            /* Never reached. */\n"
"        }\n"
            );

        return;
    }

    /* Save the full result type as we may want to fiddle with it. */
    orig_res = *res;

    /* See if we need to make a copy of the result on the heap. */
    needsNew = copyConstRefArg(res);

    if (needsNew)
        resetIsConstArg(res);

    /* See if sipRes is needed. */
    is_result = (!isInplaceNumberSlot(od->common) &&
             !isInplaceSequenceSlot(od->common) &&
             (res->atype != void_type || res->nrderefs != 0));

    newline = FALSE;

    if (is_result)
    {
        prcode(fp,
"            ");

        generateNamedValueType(scope, res, "sipRes", fp);

        /*
         * The typical %MethodCode usually causes a compiler warning, so we
         * initialise the result in that case to try and suppress it.
         */
        if (od->methodcode != NULL)
        {
            prcode(fp," = ");

            generateCastZero(res,fp);
        }

        prcode(fp,";\n"
            );

        newline = TRUE;
    }

    result_size = -1;
    deltemps = TRUE;
    post_process = FALSE;

    /* See if we want to keep a reference to the result. */
    if (keepReference(res))
        post_process = TRUE;

    for (a = 0; a < od->pysig.nrArgs; ++a)
    {
        argDef *ad = &od->pysig.args[a];

        if (isResultSize(ad))
            result_size = a;

        if (static_factory && keepReference(ad))
            post_process = TRUE;

        /*
         * If we have an In,Out argument that has conversion code then we delay
         * the destruction of any temporary variables until after we have
         * converted the outputs.
         */
        if (isInArg(ad) && isOutArg(ad) && hasConvertToCode(ad))
        {
            deltemps = FALSE;
            post_process = TRUE;
        }

        /*
         * If we are returning a class via an output only reference or pointer
         * then we need an instance on the heap.
         */
        if (needNewInstance(ad))
        {
            prcode(fp,
"            %a = new %b();\n"
                , mod, ad, a, ad);

            newline = TRUE;
        }
    }

    if (post_process)
    {
        prcode(fp,
"            PyObject *sipResObj;\n"
                );

        newline = TRUE;
    }

    error_flag = old_error_flag = FALSE;

    if (od->methodcode != NULL)
    {
        /* See if the handwritten code seems to be using the error flag. */
        if (needErrorFlag(od->methodcode))
        {
            prcode(fp,
"            sipErrorState sipError = sipErrorNone;\n"
                );

            newline = TRUE;
            error_flag = TRUE;
        }
        else if (needOldErrorFlag(od->methodcode))
        {
            prcode(fp,
"            int sipIsErr = 0;\n"
                );

            newline = TRUE;
            old_error_flag = TRUE;
        }
    }

    if (newline)
        prcode(fp,
"\n"
            );

    /* If it is abstract make sure that self was bound. */
    if (isAbstract(od))
        prcode(fp,
"            if (!sipOrigSelf)\n"
"            {\n"
"                sipAbstractMethod(%N, %N);\n"
"                return NULL;\n"
"            }\n"
"\n"
            , c_scope->pyname, od->common->pyname);

    if (isDeprecated(od))
    {
        /* Note that any temporaries will leak if an exception is raised. */
        if (pyname != NULL)
            prcode(fp,
"            if (sipDeprecated(%N,%N) < 0)\n"
                , pyname, od->common->pyname);
        else
            prcode(fp,
"            if (sipDeprecated(NULL,%N) < 0)\n"
                , od->common->pyname);

        prcode(fp,
"                return %s;\n"
"\n"
            , ((isVoidReturnSlot(od->common) || isIntReturnSlot(od->common) || isSSizeReturnSlot(od->common) || isLongReturnSlot(od->common)) ? "-1" : "NULL"));
    }

    /* Call any pre-hook. */
    if (od->prehook != NULL)
        prcode(fp,
"            sipCallHook(\"%s\");\n"
"\n"
            ,od->prehook);

    if (od->methodcode != NULL)
        generateCppCodeBlock(od->methodcode,fp);
    else
    {
        int rgil = ((release_gil || isReleaseGIL(od)) && !isHoldGIL(od));
        int closing_paren = FALSE;

        if (needsNew && generating_c)
        {
            prcode(fp,
"            if ((sipRes = (%b *)sipMalloc(sizeof (%b))) == NULL)\n"
"        {\n"
                ,res,res);

            gc_ellipsis(&od->pysig, fp);

            prcode(fp,
"                return NULL;\n"
"            }\n"
"\n"
                );
        }

        if (raisesPyException(od))
            prcode(fp,
"            PyErr_Clear();\n"
"\n"
                );

        if (rgil)
            prcode(fp,
"            Py_BEGIN_ALLOW_THREADS\n"
                );

        generateTry(od->exceptions,fp);

        prcode(fp,
"            ");

        if (od->common->slot != cmp_slot && is_result)
        {
            /* Construct a copy on the heap if needed. */
            if (needsNew)
            {
                if (generating_c)
                {
                    prcode(fp,"*sipRes = ");
                }
                else
                {
                    prcode(fp,"sipRes = new %b(",res);
                    closing_paren = TRUE;
                }
            }
            else
            {
                prcode(fp,"sipRes = ");

                /* See if we need the address of the result. */
                if ((res->atype == class_type || res->atype == mapped_type) && (res->nrderefs == 0 || isReference(res)))
                    prcode(fp,"&");
            }
        }

        switch (od->common->slot)
        {
        case no_slot:
            generateCppFunctionCall(mod, scope, o_scope, od, fp);
            break;

        case getitem_slot:
            prcode(fp, "(*sipCpp)[");
            generateSlotArg(mod, &od->pysig, 0, fp);
            prcode(fp,"]");
            break;

        case call_slot:
            prcode(fp, "(*sipCpp)(");
            generateCallArgs(mod, od->cppsig, &od->pysig, fp);
            prcode(fp,")");
            break;

        case int_slot:
        case long_slot:
        case float_slot:
            prcode(fp, "*sipCpp");
            break;

        case add_slot:
            generateNumberSlotCall(mod, od, "+", fp);
            break;

        case concat_slot:
            generateBinarySlotCall(mod, scope, od, "+", deref, fp);
            break;

        case sub_slot:
            generateNumberSlotCall(mod, od, "-", fp);
            break;

        case mul_slot:
            generateNumberSlotCall(mod, od, "*", fp);
            break;

        case repeat_slot:
            generateBinarySlotCall(mod, scope, od, "*", deref, fp);
            break;

        case div_slot:
        case truediv_slot:
            generateNumberSlotCall(mod, od, "/", fp);
            break;

        case mod_slot:
            generateNumberSlotCall(mod, od, "%", fp);
            break;

        case and_slot:
            generateNumberSlotCall(mod, od, "&", fp);
            break;

        case or_slot:
            generateNumberSlotCall(mod, od, "|", fp);
            break;

        case xor_slot:
            generateNumberSlotCall(mod, od, "^", fp);
            break;

        case lshift_slot:
            generateNumberSlotCall(mod, od, "<<", fp);
            break;

        case rshift_slot:
            generateNumberSlotCall(mod, od, ">>", fp);
            break;

        case iadd_slot:
        case iconcat_slot:
            generateBinarySlotCall(mod, scope, od, "+=", deref, fp);
            break;

        case isub_slot:
            generateBinarySlotCall(mod, scope, od, "-=", deref, fp);
            break;

        case imul_slot:
        case irepeat_slot:
            generateBinarySlotCall(mod, scope, od, "*=", deref, fp);
            break;

        case idiv_slot:
        case itruediv_slot:
            generateBinarySlotCall(mod, scope, od, "/=", deref, fp);
            break;

        case imod_slot:
            generateBinarySlotCall(mod, scope, od, "%=", deref, fp);
            break;

        case iand_slot:
            generateBinarySlotCall(mod, scope, od, "&=", deref, fp);
            break;

        case ior_slot:
            generateBinarySlotCall(mod, scope, od, "|=", deref, fp);
            break;

        case ixor_slot:
            generateBinarySlotCall(mod, scope, od, "^=", deref, fp);
            break;

        case ilshift_slot:
            generateBinarySlotCall(mod, scope, od, "<<=", deref, fp);
            break;

        case irshift_slot:
            generateBinarySlotCall(mod, scope, od, ">>=", deref, fp);
            break;

        case invert_slot:
            prcode(fp, "~(*sipCpp)");
            break;

        case lt_slot:
            generateComparisonSlotCall(mod, scope, od, "<", ">=", deref, fp);
            break;

        case le_slot:
            generateComparisonSlotCall(mod, scope, od, "<=", ">", deref, fp);
            break;

        case eq_slot:
            generateComparisonSlotCall(mod, scope, od, "==", "!=", deref, fp);
            break;

        case ne_slot:
            generateComparisonSlotCall(mod, scope, od, "!=", "==", deref, fp);
            break;

        case gt_slot:
            generateComparisonSlotCall(mod, scope, od, ">", "<=", deref, fp);
            break;

        case ge_slot:
            generateComparisonSlotCall(mod, scope, od, ">=", "<", deref, fp);
            break;

        case neg_slot:
            prcode(fp, "-(*sipCpp)");
            break;

        case pos_slot:
            prcode(fp, "+(*sipCpp)");
            break;

        case cmp_slot:
            prcode(fp,"if ");
            generateBinarySlotCall(mod, scope, od, "<", deref, fp);
            prcode(fp,"\n"
"                sipRes = -1;\n"
"            else if ");
            generateBinarySlotCall(mod, scope, od, ">", deref, fp);
            prcode(fp,"\n"
"                sipRes = 1;\n"
"            else\n"
"                sipRes = 0");

            break;
        }

        if (closing_paren)
            prcode(fp,")");

        prcode(fp,";\n"
            );

        generateCatch(od->exceptions, &od->pysig, mod, fp, rgil);

        if (rgil)
            prcode(fp,
"            Py_END_ALLOW_THREADS\n"
                );
    }

    for (a = 0; a < od->pysig.nrArgs; ++a)
    {
        argDef *ad = &od->pysig.args[a];

        if (!isInArg(ad))
            continue;

        /* Handle any /KeepReference/ arguments except for static factories. */
        if (!static_factory && keepReference(ad))
        {
            prcode(fp,
"\n"
"            sipKeepReference(%s, %d, %a%s);\n"
                , (scope == NULL || isStatic(od) ? "NULL" : "sipSelf"), ad->key, mod, ad, a, (((ad->atype == ascii_string_type || ad->atype == latin1_string_type || ad->atype == utf8_string_type) && ad->nrderefs == 1) || !isGetWrapper(ad) ? "Keep" : "Wrapper"));
        }

        /* Handle /TransferThis/ for non-factory methods. */
        if (!isFactory(od) && isThisTransferred(ad))
        {
            prcode(fp,
"\n"
"            if (sipOwner)\n"
"                sipTransferTo(sipSelf, (PyObject *)sipOwner);\n"
"            else\n"
"                sipTransferBack(sipSelf);\n"
                    );
        }
    }

    if (isThisTransferredMeth(od))
        prcode(fp,
"\n"
"            sipTransferTo(sipSelf, NULL);\n"
                );

    gc_ellipsis(&od->pysig, fp);

    if (deltemps && !isZeroArgSlot(od->common))
        deleteTemps(mod, &od->pysig, fp);

    prcode(fp,
"\n"
        );

    /* Handle the error flag if it was used. */
    error_value = ((isVoidReturnSlot(od->common) || isIntReturnSlot(od->common) || isSSizeReturnSlot(od->common) || isLongReturnSlot(od->common)) ? "-1" : "0");

    if (raisesPyException(od))
    {
        prcode(fp,
"            if (PyErr_Occurred())\n"
"                return %s;\n"
"\n"
                , error_value);
    }
    else if (error_flag)
    {
        if (!isZeroArgSlot(od->common))
            prcode(fp,
"            if (sipError == sipErrorFail)\n"
"                return %s;\n"
"\n"
                , error_value);

        prcode(fp,
"            if (sipError == sipErrorNone)\n"
"            {\n"
            );
    }
    else if (old_error_flag)
    {
        prcode(fp,
"            if (sipIsErr)\n"
"                return %s;\n"
"\n"
            , error_value);
    }

    /* Call any post-hook. */
    if (od->posthook != NULL)
        prcode(fp,
"\n"
"            sipCallHook(\"%s\");\n"
            ,od->posthook);

    if (isVoidReturnSlot(od->common))
        prcode(fp,
"            return 0;\n"
            );
    else if (isInplaceNumberSlot(od->common) || isInplaceSequenceSlot(od->common))
        prcode(fp,
"            Py_INCREF(sipSelf);\n"
"            return sipSelf;\n"
            );
    else if (isIntReturnSlot(od->common) || isSSizeReturnSlot(od->common) || isLongReturnSlot(od->common))
        prcode(fp,
"            return sipRes;\n"
            );
    else
    {
        generateHandleResult(mod, od, needsNew, result_size,
                (post_process ? "sipResObj =" : "return"), fp);

        /* Delete the temporaries now if we haven't already done so. */
        if (!deltemps)
            deleteTemps(mod, &od->pysig, fp);

        /*
         * Keep a reference to a pointer to a class if it isn't owned by
         * Python.
         */
        if (keepReference(res))
            prcode(fp,
"\n"
"            sipKeepReference(sipSelf, %d, sipResObj);\n"
                , res->key);

        /*
         * Keep a reference to any argument with the result if the function is
         * a static factory.
         */
        if (static_factory)
        {
            for (a = 0; a < od->pysig.nrArgs; ++a)
            {
                argDef *ad = &od->pysig.args[a];

                if (!isInArg(ad))
                    continue;

                if (keepReference(ad))
                {
                    prcode(fp,
"\n"
"            sipKeepReference(sipResObj, %d, %a%s);\n"
                        , ad->key, mod, ad, a, (((ad->atype == ascii_string_type || ad->atype == latin1_string_type || ad->atype == utf8_string_type) && ad->nrderefs == 1) || !isGetWrapper(ad) ? "Keep" : "Wrapper"));
                }
            }
        }

        if (post_process)
            prcode(fp,
"\n"
"            return sipResObj;\n"
                );
    }

    if (error_flag)
    {
        prcode(fp,
"            }\n"
            );

        if (!isZeroArgSlot(od->common))
            prcode(fp,
"\n"
"            sipAddException(sipError, &sipParseErr);\n"
                );
    }

    prcode(fp,
"        }\n"
        );

    /* Restore the full type of the result. */
    *res = orig_res;
}


/*
 * Generate a call to a C++ function.
 */
static void generateCppFunctionCall(moduleDef *mod, ifaceFileDef *scope,
        ifaceFileDef *o_scope, overDef *od, FILE *fp)
{
    char *mname = od->cppname;
    int parens = 1;

    /*
     * If the function is protected then call the public wrapper.  If it is
     * virtual then call the explicit scoped function if "self" was passed as
     * the first argument.
     */

    if (scope == NULL)
        prcode(fp, "%s(", mname);
    else if (scope->type == namespace_iface)
        prcode(fp, "%S::%s(", scope->fqcname, mname);
    else if (isStatic(od))
    {
        if (isProtected(od))
            prcode(fp, "sip%C::sipProtect_%s(", scope->fqcname, mname);
        else
            prcode(fp, "%S::%s(", o_scope->fqcname, mname);
    }
    else if (isProtected(od))
    {
        if (!isAbstract(od) && (isVirtual(od) || isVirtualReimp(od)))
        {
            prcode(fp, "sipCpp->sipProtectVirt_%s(sipSelfWasArg", mname);

            if (od->cppsig->nrArgs > 0)
                prcode(fp, ",");
        }
        else
            prcode(fp, "sipCpp->sipProtect_%s(", mname);
    }
    else if (!isAbstract(od) && (isVirtual(od) || isVirtualReimp(od)))
    {
        prcode(fp, "(sipSelfWasArg ? sipCpp->%S::%s(", o_scope->fqcname, mname);
        generateCallArgs(mod, od->cppsig, &od->pysig, fp);
        prcode(fp, ") : sipCpp->%s(", mname);
        ++parens;
    }
    else
        prcode(fp, "sipCpp->%s(", mname);

    generateCallArgs(mod, od->cppsig, &od->pysig, fp);

    while (parens--)
        prcode(fp, ")");
}


/*
 * Generate argument to a slot.
 */
static void generateSlotArg(moduleDef *mod, signatureDef *sd, int argnr,
        FILE *fp)
{
    argDef *ad;
    int deref;

    ad = &sd->args[argnr];
    deref = ((ad->atype == class_type || ad->atype == mapped_type) && ad->nrderefs == 0);

    prcode(fp, "%s%a", (deref ? "*" : ""), mod, ad, argnr);
}


/*
 * Generate the call to a comparison slot method.
 */
static void generateComparisonSlotCall(moduleDef *mod, ifaceFileDef *scope,
        overDef *od, const char *op, const char *cop, int deref, FILE *fp)
{
    if (isComplementary(od))
    {
        op = cop;
        prcode(fp, "!");
    }

    if (!isGlobal(od))
    {
        const char *deref_s = (deref ? "->" : ".");

        if (isAbstract(od))
            prcode(fp, "sipCpp%soperator%s(", deref_s, op);
        else
            prcode(fp, "sipCpp%s%S::operator%s(", deref_s, scope->fqcname, op);
    }
    else if (deref)
        prcode(fp, "operator%s((*sipCpp), ", op);
    else
        prcode(fp, "operator%s(sipCpp, ", op);

    generateSlotArg(mod, &od->pysig, 0, fp);
    prcode(fp, ")");
}


/*
 * Generate the call to a binary (non-number) slot method.
 */
static void generateBinarySlotCall(moduleDef *mod, ifaceFileDef *scope,
        overDef *od, const char *op, int deref, FILE *fp)
{
    generateComparisonSlotCall(mod, scope, od, op, "", deref, fp);
}


/*
 * Generate the call to a binary number slot method.
 */
static void generateNumberSlotCall(moduleDef *mod, overDef *od, char *op,
        FILE *fp)
{
    prcode(fp, "(");
    generateSlotArg(mod, &od->pysig, 0, fp);
    prcode(fp, " %s ", op);
    generateSlotArg(mod, &od->pysig, 1, fp);
    prcode(fp, ")");
}


/*
 * Generate the argument variables for a member function/constructor/operator.
 */
static int generateArgParser(moduleDef *mod, signatureDef *sd,
        classDef *c_scope, mappedTypeDef *mt_scope, ctorDef *ct, overDef *od,
        int secCall, FILE *fp)
{
    int a, isQtSlot, optargs, arraylenarg, sigarg, handle_self, single_arg;
    int slotconarg, slotdisarg, need_owner;
    ifaceFileDef *scope;
    argDef *arraylenarg_ad, *sigarg_ad, *slotconarg_ad, *slotdisarg_ad;

    if (mt_scope != NULL)
        scope = mt_scope->iff;
    else if (c_scope != NULL)
    {
        /* If the class is just a namespace, then ignore it. */
        if (c_scope->iff->type == namespace_iface)
        {
            c_scope = NULL;
            scope = NULL;
        }
        else
            scope = c_scope->iff;
    }
    else
        scope = NULL;

    handle_self = (od != NULL && od->common->slot == no_slot && !isStatic(od) && c_scope != NULL);

    /* Assume there isn't a Qt slot. */
    isQtSlot = FALSE;

    /*
     * Generate the local variables that will hold the parsed arguments and
     * values returned via arguments.
     */
    sigarg = -1;
    need_owner = FALSE;

    for (a = 0; a < sd->nrArgs; ++a)
    {
        argDef *ad = &sd->args[a];

        switch (ad->atype)
        {
        case signal_type:
            sigarg_ad = ad;
            sigarg = a;
            break;

        case rxcon_type:
        case rxdis_type:
            isQtSlot = TRUE;
            break;

        case slotcon_type:
            slotconarg_ad = ad;
            slotconarg = a;
            break;

        case slotdis_type:
            slotdisarg_ad = ad;
            slotdisarg = a;
            break;
        }

        if (isArraySize(ad))
        {
            arraylenarg_ad = ad;
            arraylenarg = a;
        }

        generateVariable(mod, scope, ad, a, fp);

        if (isThisTransferred(ad))
            need_owner = TRUE;
    }

    if (od != NULL && need_owner)
        prcode(fp,
"        sipWrapper *sipOwner = 0;\n"
            );

    if (handle_self)
    {
        const char *const_str = (isConst(od) ? "const " : "");

        if (isProtected(od) && hasShadow(c_scope))
            prcode(fp,
"        %ssip%C *sipCpp;\n"
                , const_str, classFQCName(c_scope));
        else
            prcode(fp,
"        %s%U *sipCpp;\n"
                , const_str, c_scope);

        prcode(fp,
"\n"
            );
    }
    else if (sd->nrArgs != 0)
        prcode(fp,
"\n"
            );

    /* Generate the call to the parser function. */
    single_arg = FALSE;

    if (od != NULL && isNumberSlot(od->common))
    {
        prcode(fp,
"        if (sipParsePair(&sipParseErr, sipArg0, sipArg1, \"");
    }
    else if (od != NULL && od->common->slot == setattr_slot)
    {
        /*
         * We don't even try to invoke the parser if there is a value and there
         * shouldn't be (or vice versa) so that the list of errors doesn't get
         * poluted with signatures that can never apply.
         */
        prcode(fp,
"        if (sipValue %s NULL && sipParsePair(&sipParseErr, sipName, %s, \"", (isDelattr(od) ? "==" : "!="), (isDelattr(od) ? "NULL" : "sipValue"));
    }
    else if ((od != NULL && useKeywordArgs(od->common)) || ct != NULL)
    {
        KwArgs kwargs;
        int is_ka_list;

        /*
         * We handle keywords if we might have been passed some (because one of
         * the overloads uses them or we are a ctor).  However this particular
         * overload might not have any.
         */
        if (od != NULL)
            kwargs = od->kwargs;
        else if (ct != NULL)
            kwargs = ct->kwargs;
        else
            kwargs = NoKwArgs;

        /*
         * The above test isn't good enough because when the flags were set in
         * the parser we couldn't know for sure if an argument was an output
         * pointer.  Therefore we check here.  The drawback is that we may
         * generate the name string for the argument but never use it, or we
         * might have an empty keyword name array or one that contains only
         * NULLs.
         */
        is_ka_list = FALSE;

        if (kwargs != NoKwArgs)
        {
            int a;

            for (a = 0; a < sd->nrArgs; ++a)
            {
                argDef *ad = &sd->args[a];

                if (isInArg(ad))
                {
                    if (!is_ka_list)
                    {
                        prcode(fp,
"        static const char *sipKwdList[] = {\n"
                            );

                        is_ka_list = TRUE;
                    }

                    if (ad->name != NULL && (kwargs == AllKwArgs || ad->defval != NULL))
                        prcode(fp,
"            %N,\n"
                            , ad->name);
                    else
                        prcode(fp,
"            NULL,\n"
                            );
                }
            }

            if (is_ka_list)
                prcode(fp,
    "        };\n"
    "\n"
                    );
        }

        prcode(fp,
"        if (sipParseKwdArgs(%ssipParseErr, sipArgs, sipKwds, %s, %s, \"", (ct != NULL ? "" : "&"), (is_ka_list ? "sipKwdList" : "NULL"), (ct != NULL ? "sipUnused" : "NULL"));
    }
    else
    {
        single_arg = (od != NULL && od->common->slot != no_slot && !isMultiArgSlot(od->common));

        prcode(fp,
"        if (sipParseArgs(%ssipParseErr, sipArg%s, \"", (ct != NULL ? "" : "&"), (single_arg ? "" : "s"));
    }

    /* Generate the format string. */
    optargs = FALSE;

    if (single_arg)
        prcode(fp, "1");

    if (handle_self)
        prcode(fp,"%c",(isReallyProtected(od) ? 'p' : 'B'));
    else if (isQtSlot && od == NULL)
        prcode(fp,"C");

    for (a = 0; a < sd->nrArgs; ++a)
    {
        char *fmt = "";
        argDef *ad = &sd->args[a];

        if (!isInArg(ad))
            continue;

        if (ad->defval != NULL && !optargs)
        {
            prcode(fp,"|");
            optargs = TRUE;
        }

        switch (ad->atype)
        {
        case ascii_string_type:
            if (ad->nrderefs == 0 || (isOutArg(ad) && ad->nrderefs == 1))
                fmt = "aA";
            else
                fmt = "AA";

            break;

        case latin1_string_type:
            if (ad->nrderefs == 0 || (isOutArg(ad) && ad->nrderefs == 1))
                fmt = "aL";
            else
                fmt = "AL";

            break;

        case utf8_string_type:
            if (ad->nrderefs == 0 || (isOutArg(ad) && ad->nrderefs == 1))
                fmt = "a8";
            else
                fmt = "A8";

            break;

        case sstring_type:
        case ustring_type:
        case string_type:
            if (ad->nrderefs == 0 || (isOutArg(ad) && ad->nrderefs == 1))
                fmt = "c";
            else if (isArray(ad))
                fmt = "k";
            else
                fmt = "s";

            break;

        case wstring_type:
            if (ad->nrderefs == 0 || (isOutArg(ad) && ad->nrderefs == 1))
                fmt = "w";
            else if (isArray(ad))
                fmt = "K";
            else
                fmt = "x";

            break;

        case enum_type:
            if (ad->u.ed->fqcname == NULL)
                fmt = "e";
            else if (isConstrained(ad))
                fmt = "XE";
            else
                fmt = "E";
            break;

        case bool_type:
            fmt = "b";
            break;

        case cbool_type:
            fmt = "Xb";
            break;

        case int_type:
            if (!isArraySize(ad))
                fmt = "i";

            break;

        case uint_type:
            if (!isArraySize(ad))
                fmt = "u";

            break;

        case cint_type:
            fmt = "Xi";
            break;

        case byte_type:
        case sbyte_type:
            if (!isArraySize(ad))
                fmt = "L";

            break;

        case ubyte_type:
            if (!isArraySize(ad))
                fmt = "M";

            break;

        case short_type:
            if (!isArraySize(ad))
                fmt = "h";

            break;

        case ushort_type:
            if (!isArraySize(ad))
                fmt = "t";

            break;

        case long_type:
            if (!isArraySize(ad))
                fmt = "l";

            break;

        case ulong_type:
            if (!isArraySize(ad))
                fmt = "m";

            break;

        case longlong_type:
            if (!isArraySize(ad))
                fmt = "n";

            break;

        case ulonglong_type:
            if (!isArraySize(ad))
                fmt = "o";

            break;

        case struct_type:
        case void_type:
            fmt = "v";
            break;

        case capsule_type:
            fmt = "z";
            break;

        case float_type:
            fmt = "f";
            break;

        case cfloat_type:
            fmt = "Xf";
            break;

        case double_type:
            fmt = "d";
            break;

        case cdouble_type:
            fmt = "Xd";
            break;

        case signal_type:
            fmt = "G";
            break;

        case slot_type:
            fmt = "S";
            break;

        case anyslot_type:
            fmt = "U";
            break;

        case slotcon_type:
        case slotdis_type:
            fmt = (secCall ? "" : "S");
            break;

        case rxcon_type:
            fmt = (secCall ? (isSingleShot(ad) ? "g" : "y") : "q");
            break;

        case rxdis_type:
            fmt = (secCall ? "Y" : "Q");
            break;

        case mapped_type:
        case class_type:
            if (isArray(ad))
            {
                if (ad->nrderefs != 1 || !isInArg(ad) || isReference(ad))
                    fatal("Mapped type or class with /Array/ is not a pointer\n");

                if (ad->atype == mapped_type && noRelease(ad->u.mtd))
                    fatal("Mapped type does not support /Array/\n");

                if (ad->atype == class_type && !(generating_c || assignmentHelper(ad->u.cd)))
                {
                    fatalScopedName(classFQCName(ad->u.cd));
                    fatal(" does not support /Array/\n");
                }

                fmt = "r";
            }
            else
            {
                fmt = getSubFormatChar('J', ad);
            }

            break;

        case pyobject_type:
            fmt = getSubFormatChar('P',ad);
            break;

        case pytuple_type:
        case pylist_type:
        case pydict_type:
        case pyslice_type:
        case pytype_type:
            fmt = (isAllowNone(ad) ? "N" : "T");
            break;

        case pycallable_type:
            fmt = (isAllowNone(ad) ? "H" : "F");
            break;

        case pybuffer_type:
            fmt = (isAllowNone(ad) ? "$" : "!");
            break;

        case qobject_type:
            fmt = "R";
            break;

        case ellipsis_type:
            fmt = "W";
            break;
        }

        /*
         * Get the wrapper if explicitly asked for or we are going to keep a
         * reference to.  However if it is an encoded string then we will get
         * the actual wrapper from the format character.
         */
        if (isGetWrapper(ad) || (keepReference(ad) && ad->atype != ascii_string_type && ad->atype != latin1_string_type && ad->atype != utf8_string_type) || (keepReference(ad) && ad->nrderefs != 1))
            prcode(fp, "@");

        prcode(fp,fmt);
    }

    prcode(fp,"\"");

    /* Generate the parameters corresponding to the format string. */

    if (handle_self)
        prcode(fp,", &sipSelf, sipType_%C, &sipCpp",classFQCName(c_scope));
    else if (isQtSlot && od == NULL)
        prcode(fp,", sipSelf");

    for (a = 0; a < sd->nrArgs; ++a)
    {
        argDef *ad = &sd->args[a];

        if (!isInArg(ad))
            continue;

        /* Use the wrapper name if it was explicitly asked for. */
        if (isGetWrapper(ad))
            prcode(fp, ", &%aWrapper", mod, ad, a);
        else if (keepReference(ad))
            prcode(fp, ", &%aKeep", mod, ad, a);

        switch (ad->atype)
        {
        case mapped_type:
            prcode(fp, ", sipType_%T,&%a", ad, mod, ad, a);

            if (isArray(ad))
            {
                prcode(fp, ", &%a", mod, arraylenarg_ad, arraylenarg);
            }
            else if (!isConstrained(ad))
            {
                if (noRelease(ad->u.mtd))
                    prcode(fp, ",NULL");
                else
                    prcode(fp, ", &%aState", mod, ad, a);
            }

            break;

        case class_type:
            prcode(fp, ", sipType_%T, &%a", ad, mod, ad, a);

            if (isArray(ad))
            {
                prcode(fp, ", &%a", mod, arraylenarg_ad, arraylenarg);
            }
            else
            {
                if (isThisTransferred(ad))
                    prcode(fp, ", %ssipOwner", (ct != NULL ? "" : "&"));

                if (ad->u.cd->convtocode != NULL && !isConstrained(ad))
                    prcode(fp, ", &%aState", mod, ad, a);
            }

            break;

        case ascii_string_type:
            if (!keepReference(ad) && ad->nrderefs == 1)
                prcode(fp, ", &%aKeep", mod, ad, a);

            prcode(fp, ", &%a", mod, ad, a);
            break;

        case latin1_string_type:
            if (!keepReference(ad) && ad->nrderefs == 1)
                prcode(fp, ", &%aKeep", mod, ad, a);

            prcode(fp, ", &%a", mod, ad, a);
            break;

        case utf8_string_type:
            if (!keepReference(ad) && ad->nrderefs == 1)
                prcode(fp, ", &%aKeep", mod, ad, a);

            prcode(fp, ", &%a", mod, ad, a);
            break;

        case rxcon_type:
            {
                if (sigarg > 0)
                    prcode(fp, ", %a", mod, sigarg_ad, sigarg);
                else
                {
                    prcode(fp,", \"(");

                    generateCalledArgs(NULL, scope, slotconarg_ad->u.sa, Declaration, TRUE, fp);

                    prcode(fp,")\"");
                }

                prcode(fp, ", &%a, &%a", mod, ad, a, mod, slotconarg_ad, slotconarg);

                break;
            }

        case rxdis_type:
            {
                prcode(fp,", \"(");

                generateCalledArgs(NULL, scope, slotdisarg_ad->u.sa, Declaration, TRUE, fp);

                prcode(fp, ")\", &%a, &%a", mod, ad, a, mod, slotdisarg_ad, slotdisarg);

                break;
            }

        case slotcon_type:
        case slotdis_type:
            if (!secCall)
                prcode(fp, ", &%a", mod, ad, a);

            break;

        case anyslot_type:
            prcode(fp, ", &%aName, &%aCallable", mod, ad, a, mod, ad, a);
            break;

        case pytuple_type:
            prcode(fp, ", &PyTuple_Type, &%a", mod, ad, a);
            break;

        case pylist_type:
            prcode(fp, ", &PyList_Type, &%a", mod, ad, a);
            break;

        case pydict_type:
            prcode(fp, ", &PyDict_Type, &%a", mod, ad, a);
            break;

        case pyslice_type:
            prcode(fp, ", &PySlice_Type, &%a", mod, ad, a);
            break;

        case pytype_type:
            prcode(fp, ", &PyType_Type, &%a", mod, ad, a);
            break;

        case enum_type:
            if (ad->u.ed->fqcname != NULL)
                prcode(fp, ", sipType_%C", ad->u.ed->fqcname);

            prcode(fp, ", &%a", mod, ad, a);
            break;

        case capsule_type:
            prcode(fp, ", \"%S\", &%a", ad->u.cap, mod, ad, a);
            break;

        default:
            if (!isArraySize(ad))
                prcode(fp, ", &%a", mod, ad, a);

            if (isArray(ad))
                prcode(fp, ", &%a", mod, arraylenarg_ad, arraylenarg);
        }
    }

    prcode(fp,"))\n");

    return isQtSlot;
}


/*
 * Get the format character string for something that has sub-formats.
 */

static char *getSubFormatChar(char fc, argDef *ad)
{
    static char fmt[3];
    char flags;

    flags = 0;

    if (isTransferred(ad))
        flags |= 0x02;

    if (isTransferredBack(ad))
        flags |= 0x04;

    if (ad->atype == class_type || ad->atype == mapped_type)
    {
        if (ad->nrderefs == 0)
            flags |= 0x01;

        if (isThisTransferred(ad))
            flags |= 0x10;

        if (isConstrained(ad) || (ad->atype == class_type && ad->u.cd->convtocode == NULL))
            flags |= 0x08;
    }

    fmt[0] = fc;
    fmt[1] = '0' + flags;
    fmt[2] = '\0';

    return fmt;
}


/*
 * Return TRUE if a type has %ConvertToTypeCode.
 */
static int hasConvertToCode(argDef *ad)
{
    codeBlockList *convtocode;

    if (ad->atype == class_type && !isConstrained(ad))
        convtocode = ad->u.cd->convtocode;
    else if (ad->atype == mapped_type && !isConstrained(ad))
        convtocode = ad->u.mtd->convtocode;
    else
        convtocode = NULL;

    return (convtocode != NULL);
}


/*
 * Garbage collect any ellipsis argument.
 */
static void gc_ellipsis(signatureDef *sd, FILE *fp)
{
    if (sd->nrArgs > 0 && sd->args[sd->nrArgs - 1].atype == ellipsis_type)
        prcode(fp,
"\n"
"            Py_DECREF(a%d);\n"
            , sd->nrArgs - 1);
}


/*
 * Delete any instances created to hold /Out/ arguments.
 */
static void deleteOuts(moduleDef *mod, signatureDef *sd, FILE *fp)
{
    int a;

    for (a = 0; a < sd->nrArgs; ++a)
    {
        argDef *ad = &sd->args[a];

        if (needNewInstance(ad))
            prcode(fp,
"                delete %a;\n"
                , mod, ad, a);
    }
}



/*
 * Delete any temporary variables on the heap created by type convertors.
 */
static void deleteTemps(moduleDef *mod, signatureDef *sd, FILE *fp)
{
    int a;

    for (a = 0; a < sd->nrArgs; ++a)
    {
        argDef *ad = &sd->args[a];

        if (isArray(ad) && (ad->atype == mapped_type || ad->atype == class_type))
        {
            if (!isTransferred(ad))
            {
                if (generating_c)
                    prcode(fp,
"            sipFree(%a);\n"
                        , mod, ad, a);
                else
                    prcode(fp,
"            delete[] %a;\n"
                        , mod, ad, a);
            }

            continue;
        }

        if (!isInArg(ad))
            continue;

        if ((ad->atype == ascii_string_type || ad->atype == latin1_string_type || ad->atype == utf8_string_type) && ad->nrderefs == 1)
        {
            prcode(fp,
"            Py_%sDECREF(%aKeep);\n"
                , (ad->defval != NULL ? "X" : ""), mod, ad, a);
        }
        else if (ad->atype == wstring_type && ad->nrderefs == 1)
        {
            if (generating_c || !isConstArg(ad))
                prcode(fp,
"            sipFree(%a);\n"
                    , mod, ad, a);
            else
                prcode(fp,
"            sipFree(const_cast<wchar_t *>(%a));\n"
                    , mod, ad, a);
        }
        else if (hasConvertToCode(ad))
        {
            if (ad->atype == mapped_type && noRelease(ad->u.mtd))
                continue;

            if (generating_c || !isConstArg(ad))
                prcode(fp,
"            sipReleaseType(%a,sipType_%T,%aState);\n"
                    , mod, ad, a, ad, mod, ad, a);
            else
                prcode(fp,
"            sipReleaseType(const_cast<%b *>(%a),sipType_%T,%aState);\n"
                    , ad, mod, ad, a, ad, mod, ad, a);
        }
    }
}


/*
 * Generate a list of C++ code blocks.
 */
static void generateCppCodeBlock(codeBlockList *cbl, FILE *fp)
{
    int reset_line = FALSE;

    while (cbl != NULL)
    {
        codeBlock *cb = cbl->block;

        /*
         * Fragmented fragments (possibly created when applying template types)
         * don't have a filename.
         */
        if (cb->filename != NULL)
        {
            generatePreprocLine(cb->linenr, cb->filename, fp);
            reset_line = TRUE;
        }

        prcode(fp, "%s", cb->frag);

        cbl = cbl->next;
    }

    if (reset_line)
        generatePreprocLine(currentLineNr + 1, currentFileName, fp);
}


/*
 * Generate a #line preprocessor directive.
 */
static void generatePreprocLine(int linenr, const char *fname, FILE *fp)
{
    prcode(fp,
"#line %d \"", linenr);

    while (*fname != '\0')
    {
        prcode(fp, "%c", *fname);

        if (*fname == '\\')
            prcode(fp, "\\");

        ++fname;
    }

    prcode(fp, "\"\n"
        );
}


/*
 * Create a source file.
 */
static FILE *createCompilationUnit(moduleDef *mod, const char *fname,
        const char *description, int timestamp)
{
    FILE *fp = createFile(mod, fname, description, timestamp);

    if (fp != NULL)
        generateCppCodeBlock(mod->unitcode, fp);

    return fp;
}


/*
 * Create a file with an optional standard header.
 */
static FILE *createFile(moduleDef *mod, const char *fname,
        const char *description, int timestamp)
{
    FILE *fp;

    /* Create the file. */
    if ((fp = fopen(fname, "w")) == NULL)
        fatal("Unable to create file \"%s\"\n",fname);

    /* The "stack" doesn't have to be very deep. */
    previousLineNr = currentLineNr;
    currentLineNr = 1;
    previousFileName = currentFileName;
    currentFileName = fname;

    if (description != NULL)
    {
        int needComment;
        codeBlockList *cbl;

        /* Write the header. */
        prcode(fp,
"/*\n"
" * %s\n"
" *\n"
" * Generated by SIP %s"
            , description
            , sipVersion);

        if (timestamp)
        {
            time_t now = time(NULL);

            prcode(fp, " on %s", ctime(&now));
        }
        else
        {
            prcode(fp, "\n"
                );
        }

        if (mod->copying != NULL)
            prcode(fp,
" *\n"
                );

        needComment = TRUE;

        for (cbl = mod->copying; cbl != NULL; cbl = cbl->next)
        {
            const char *cp;

            for (cp = cbl->block->frag; *cp != '\0'; ++cp)
            {
                if (needComment)
                {
                    needComment = FALSE;
                    prcode(fp," * ");
                }

                prcode(fp,"%c",*cp);

                if (*cp == '\n')
                    needComment = TRUE;
            }
        }

        prcode(fp,
" */\n"
            );
    }

    return fp;
}


/*
 * Close a file and report any errors.
 */
static void closeFile(FILE *fp)
{
    if (ferror(fp))
        fatal("Error writing to \"%s\"\n",currentFileName);

    if (fclose(fp))
        fatal("Error closing \"%s\"\n",currentFileName);

    currentLineNr = previousLineNr;
    currentFileName = previousFileName;
}


/*
 * Print formatted code.
 */
void prcode(FILE *fp, const char *fmt, ...)
{
    char ch;
    va_list ap;

    prcode_last = fmt;

    va_start(ap,fmt);

    while ((ch = *fmt++) != '\0')
        if (ch == '%')
        {
            ch = *fmt++;

            switch (ch)
            {
            case 'c':
                {
                    char c = (char)va_arg(ap,int);

                    if (c == '\n')
                        ++currentLineNr;

                    fputc(c,fp);
                    break;
                }

            case 's':
                {
                    const char *cp = va_arg(ap,const char *);

                    while (*cp != '\0')
                    {
                        if (*cp == '\n')
                            ++currentLineNr;

                        fputc(*cp,fp);
                        ++cp;
                    }

                    break;
                }

            case 'l':
                fprintf(fp,"%ld",va_arg(ap,long));
                break;

            case 'u':
                fprintf(fp,"%u",va_arg(ap,unsigned));
                break;

            case 'd':
                fprintf(fp,"%d",va_arg(ap,int));
                break;

            case 'g':
                fprintf(fp,"%g",va_arg(ap,double));
                break;

            case 'x':
                fprintf(fp,"0x%08x",va_arg(ap,unsigned));
                break;

            case '\0':
                fputc('%',fp);
                --fmt;
                break;

            case '\n':
                fputc('\n',fp);
                ++currentLineNr;
                break;

            case 'b':
                {
                    argDef *ad, orig;

                    ad = va_arg(ap,argDef *);
                    orig = *ad;

                    resetIsConstArg(ad);
                    resetIsReference(ad);
                    ad->nrderefs = 0;

                    generateBaseType(NULL, ad, TRUE, fp);

                    *ad = orig;

                    break;
                }

            case 'M':
                prcode_xml = !prcode_xml;
                break;

            case 'A':
                {
                    ifaceFileDef *scope = va_arg(ap, ifaceFileDef *);
                    argDef *ad = va_arg(ap, argDef *);

                    generateBaseType(scope, ad, TRUE, fp);
                    break;
                }

            case 'a':
                {
                    moduleDef *mod = va_arg(ap, moduleDef *);
                    argDef *ad = va_arg(ap, argDef *);
                    int argnr = va_arg(ap, int);

                    if (useArgNames(mod) && ad->name != NULL)
                        fprintf(fp, "%s", ad->name->text);
                    else
                        fprintf(fp, "a%d", argnr);

                    break;
                }

            case 'B':
                generateBaseType(NULL, va_arg(ap,argDef *),TRUE, fp);
                break;

            case 'T':
                prTypeName(fp, va_arg(ap,argDef *));
                break;

            case 'I':
                {
                    int indent = va_arg(ap,int);

                    while (indent-- > 0)
                        fputc('\t',fp);

                    break;
                }

            case 'N':
                {
                    nameDef *nd = va_arg(ap,nameDef *);

                    prCachedName(fp, nd, "sipName_");
                    break;
                }

            case 'n':
                {
                    nameDef *nd = va_arg(ap,nameDef *);

                    prCachedName(fp, nd, "sipNameNr_");
                    break;
                }

            case 'E':
                {
                    enumDef *ed = va_arg(ap,enumDef *);

                    if (ed->fqcname == NULL || isProtectedEnum(ed))
                        fprintf(fp,"int");
                    else
                        prScopedName(fp,ed->fqcname,"::");

                    break;
                }

            case 'F':
                prScopedName(fp,va_arg(ap,scopedNameDef *),"");
                break;

            case 'C':
                prScopedName(fp,va_arg(ap,scopedNameDef *),"_");
                break;

            case 'L':
                {
                    ifaceFileDef *iff = va_arg(ap, ifaceFileDef *);

                    prScopedName(fp, iff->fqcname, "_");

                    if (iff->api_range != NULL)
                        fprintf(fp, "_%d", iff->api_range->index);

                    break;
                }

            case 'P':
                {
                    apiVersionRangeDef *avr = va_arg(ap, apiVersionRangeDef *);

                    fprintf(fp, "%d", (avr != NULL ? avr->index : -1));

                    break;
                }

            case 'S':
                prScopedName(fp, va_arg(ap, scopedNameDef *), "::");
                break;

            case 'U':
                {
                    classDef *cd = va_arg(ap, classDef *);

                    if (generating_c)
                        fprintf(fp,"struct ");

                    prScopedClassName(fp, cd->iff, cd);
                    break;
                }

            case 'V':
                {
                    ifaceFileDef *scope = va_arg(ap, ifaceFileDef *);
                    classDef *cd = va_arg(ap, classDef *);

                    if (generating_c)
                        fprintf(fp,"struct ");

                    prScopedClassName(fp, scope, cd);
                    break;
                }

            case 'O':
                prOverloadName(fp, va_arg(ap, overDef *));
                break;

            case 'X':
                generateThrowSpecifier(va_arg(ap,throwArgs *),fp);
                break;

            default:
                fputc(ch,fp);
            }
        }
        else if (ch == '\n')
        {
            fputc('\n',fp);
            ++currentLineNr;
        }
        else
            fputc(ch,fp);

    va_end(ap);
}


/*
 * Generate the symbolic name of a cached name.
 */
static void prCachedName(FILE *fp, nameDef *nd, const char *prefix)
{
    prcode(fp, "%s", prefix);

    /*
     * If the name seems to be a template then just use the offset to ensure
     * that it is unique.
     */
    if (strchr(nd->text, '<') != NULL)
        prcode(fp, "%d", nd->offset);
    else
    {
        const char *cp;

        /* Handle C++ and Python scopes. */
        for (cp = nd->text; *cp != '\0'; ++cp)
        {
            char ch = *cp;

            if (ch == ':' || ch == '.')
                ch = '_';

            prcode(fp, "%c", ch);
        }
    }
}


/*
 * Generate the C++ name of an overloaded function.
 */
void prOverloadName(FILE *fp, overDef *od)
{
    char *pt1, *pt2;

    pt1 = "operator";

    switch (od->common->slot)
    {
    case add_slot:
        pt2 = "+";
        break;

    case sub_slot:
        pt2 = "-";
        break;

    case mul_slot:
        pt2 = "*";
        break;

    case div_slot:
    case truediv_slot:
        pt2 = "/";
        break;

    case mod_slot:
        pt2 = "%";
        break;

    case and_slot:
        pt2 = "&";
        break;

    case or_slot:
        pt2 = "|";
        break;

    case xor_slot:
        pt2 = "^";
        break;

    case lshift_slot:
        pt2 = "<<";
        break;

    case rshift_slot:
        pt2 = ">>";
        break;

    case iadd_slot:
        pt2 = "+=";
        break;

    case isub_slot:
        pt2 = "-=";
        break;

    case imul_slot:
        pt2 = "*=";
        break;

    case idiv_slot:
    case itruediv_slot:
        pt2 = "/=";
        break;

    case imod_slot:
        pt2 = "%=";
        break;

    case iand_slot:
        pt2 = "&=";
        break;

    case ior_slot:
        pt2 = "|=";
        break;

    case ixor_slot:
        pt2 = "^=";
        break;

    case ilshift_slot:
        pt2 = "<<=";
        break;

    case irshift_slot:
        pt2 = ">>=";
        break;

    case invert_slot:
        pt2 = "~";
        break;

    case call_slot:
        pt2 = "()";
        break;

    case getitem_slot:
        pt2 = "[]";
        break;

    case lt_slot:
        pt2 = "<";
        break;

    case le_slot:
        pt2 = "<=";
        break;

    case eq_slot:
        pt2 = "==";
        break;

    case ne_slot:
        pt2 = "!=";
        break;

    case gt_slot:
        pt2 = ">";
        break;

    case ge_slot:
        pt2 = ">=";
        break;

    default:
        pt1 = "";
        pt2 = od->cppname;
    }

    fprintf(fp, "%s%s", pt1, pt2);
}


/*
 * Generate a scoped name with the given separator string.
 */
static void prScopedName(FILE *fp,scopedNameDef *snd,char *sep)
{
    while (snd != NULL)
    {
        fprintf(fp,"%s",snd->name);

        if ((snd = snd->next) != NULL)
            fprintf(fp,"%s",sep);
    }
}


/*
 * Generate a scoped class name.
 */
static void prScopedClassName(FILE *fp, ifaceFileDef *scope, classDef *cd)
{
    /* Protected classes have to be explicitly scoped. */
    if (isProtectedClass(cd))
    {
        /* This should never happen. */
        if (scope == NULL)
            scope = cd->iff;

        prcode(fp, "sip%C::sip%s", scope->fqcname, classBaseName(cd));
    }
    else
    {
        scopedNameDef *snd = classFQCName(cd);

        while (snd != NULL)
        {
            fprintf(fp,"%s",snd->name);

            if ((snd = snd->next) != NULL)
                fprintf(fp, "::");
        }
    }
}


/*
 * Generate a type name to be used as part of an identifier name.
 */
static void prTypeName(FILE *fp, argDef *ad)
{
    scopedNameDef *snd;

    switch (ad->atype)
    {
    case struct_type:
        snd = ad->u.sname;
        break;

    case defined_type:
        snd = ad->u.snd;
        break;

    case enum_type:
        snd = ad->u.ed->fqcname;
        break;

    case mapped_type:
        snd = ad->u.mtd->iff->fqcname;
        break;

    case class_type:
        snd = classFQCName(ad->u.cd);
        break;

    default:
        /* This should never happen. */
        snd = NULL;
    }

    if (snd != NULL)
        prcode(fp, "%C", snd);
}


/*
 * Return TRUE if handwritten code uses the error flag.
 */
static int needErrorFlag(codeBlockList *cbl)
{
    return usedInCode(cbl, "sipError");
}


/*
 * Return TRUE if handwritten code uses the deprecated error flag.
 */
static int needOldErrorFlag(codeBlockList *cbl)
{
    return usedInCode(cbl, "sipIsErr");
}


/*
 * Return TRUE if the argument type means an instance needs to be created on
 * the heap to pass back to Python.
 */
static int needNewInstance(argDef *ad)
{
    return ((ad->atype == mapped_type || ad->atype == class_type) &&
        ((isReference(ad) && ad->nrderefs == 0) || (!isReference(ad) && ad->nrderefs == 1)) &&
        !isInArg(ad) && isOutArg(ad));
}


/*
 * Convert any protected arguments (ie. those whose type is unavailable outside
 * of a shadow class) to a fundamental type to be used instead (with suitable
 * casts).
 */
static void fakeProtectedArgs(signatureDef *sd)
{
    int a;
    argDef *ad = sd->args;

    for (a = 0; a < sd->nrArgs; ++a)
    {
        if (ad->atype == class_type && isProtectedClass(ad->u.cd))
        {
            ad->atype = fake_void_type;
            ad->nrderefs = 1;
            resetIsReference(ad);
        }
        else if (ad->atype == enum_type && isProtectedEnum(ad->u.ed))
            ad->atype = int_type;

        ++ad;
    }
}


/*
 * Reset and save any argument flags so that the signature will be rendered
 * exactly as defined in C++.
 */
static void normaliseArgs(signatureDef *sd)
{
    int a;
    argDef *ad = sd->args;

    for (a = 0; a < sd->nrArgs; ++a)
    {
        if (ad->atype == class_type && isProtectedClass(ad->u.cd))
        {
            resetIsProtectedClass(ad->u.cd);
            setWasProtectedClass(ad->u.cd);
        }
        else if (ad->atype == enum_type && isProtectedEnum(ad->u.ed))
        {
            resetIsProtectedEnum(ad->u.ed);
            setWasProtectedEnum(ad->u.ed);
        }

        ++ad;
    }
}


/*
 * Restore any argument flags modified by normaliseArgs().
 */
static void restoreArgs(signatureDef *sd)
{
    int a;
    argDef *ad = sd->args;

    for (a = 0; a < sd->nrArgs; ++a)
    {
        if (ad->atype == class_type && wasProtectedClass(ad->u.cd))
        {
            resetWasProtectedClass(ad->u.cd);
            setIsProtectedClass(ad->u.cd);
        }
        else if (ad->atype == enum_type && wasProtectedEnum(ad->u.ed))
        {
            resetWasProtectedEnum(ad->u.ed);
            setIsProtectedEnum(ad->u.ed);
        }

        ++ad;
    }
}


/*
 * Return TRUE if a dealloc function is needed for a class.
 */
static int needDealloc(classDef *cd)
{
    if (cd->iff->type == namespace_iface)
        return FALSE;

    /* All of these conditions cause some code to be generated. */

    if (tracing)
        return TRUE;

    if (generating_c)
        return TRUE;

    if (cd->dealloccode != NULL)
        return TRUE;

    if (isPublicDtor(cd))
        return TRUE;

    if (hasShadow(cd))
        return TRUE;

    return FALSE;
}


/*
 * Return the argument name to use in a function definition for handwritten
 * code.
 */
static const char *argName(const char *name, codeBlockList *cbl)
{
    static const char noname[] = "";

    /* Always use the name in C code. */
    if (generating_c)
        return name;

    /* Use the name if it is used in the handwritten code. */
    if (usedInCode(cbl, name))
        return name;

    /* Don't use the name and avoid a compiler warning. */
    return noname;
}


/*
 * Returns TRUE if a string is used in code.
 */
static int usedInCode(codeBlockList *cbl, const char *str)
{
    while (cbl != NULL)
    {
        if (strstr(cbl->block->frag, str) != NULL)
            return TRUE;

        cbl = cbl->next;
    }

    return FALSE;
}


/*
 * Generate an assignment statement from a void * variable to a class instance
 * variable.
 */
static void generateClassFromVoid(classDef *cd, const char *cname,
        const char *vname, FILE *fp)
{
    if (generating_c)
        prcode(fp, "struct %S *%s = (struct %S *)%s", classFQCName(cd), cname, classFQCName(cd), vname);
    else
        prcode(fp, "%S *%s = reinterpret_cast<%S *>(%s)", classFQCName(cd), cname, classFQCName(cd), vname);
}


/*
 * Generate an assignment statement from a void * variable to a mapped type
 * variable.
 */
static void generateMappedTypeFromVoid(mappedTypeDef *mtd, const char *cname,
        const char *vname, FILE *fp)
{
    if (generating_c)
        prcode(fp, "%b *%s = (%b *)%s", &mtd->type, cname, &mtd->type, vname);
    else
        prcode(fp, "%b *%s = reinterpret_cast<%b *>(%s)", &mtd->type, cname, &mtd->type, vname);
}


/*
 * Returns TRUE if the argument has a type that requires an extra reference to
 * the originating object to be kept.
 */
static int keepPyReference(argDef *ad)
{
    if (ad->atype == ascii_string_type || ad->atype == latin1_string_type ||
            ad->atype == utf8_string_type || ad->atype == ustring_type ||
            ad->atype == sstring_type || ad->atype == string_type)
    {
        if (!isReference(ad) && ad->nrderefs > 0)
            return TRUE;
    }

    return FALSE;
}


/*
 * Return the encoding character for the given type.
 */
static char getEncoding(argType atype)
{
    char encoding;

    switch (atype)
    {
    case ascii_string_type:
        encoding = 'A';
        break;

    case latin1_string_type:
        encoding = 'L';
        break;

    case utf8_string_type:
        encoding = '8';
        break;

    default:
        encoding = 'N';
    }

    return encoding;
}


/*
 * Return TRUE if a docstring can be automatically generated for a function
 * overload.
 */
static int overloadHasDocstring(sipSpec *pt, overDef *od, memberDef *md)
{
    if (isPrivate(od) || isSignal(od))
        return FALSE;

    if (od->common != md)
        return FALSE;

    /* If it is versioned then make sure it is the default API. */
    return isDefaultAPI(pt, od->api_range);
}


/*
 * Return TRUE if a docstring can be automatically generated for a function.
 */
static int hasDocstring(sipSpec *pt, overDef *overs, memberDef *md,
        ifaceFileDef *scope)
{
    overDef *od;

    if (noArgParser(md))
        return FALSE;

    if (scope != NULL && !isDefaultAPI(pt, scope->api_range))
        return FALSE;

    for (od = overs; od != NULL; od = od->next)
        if (overloadHasDocstring(pt, od, md))
            return TRUE;

    return FALSE;
}


/*
 * Generate the docstring for a function or method.
 */
static void generateDocstring(sipSpec *pt, overDef *overs, memberDef *md,
        const char *scope_name, classDef *scope_scope, FILE *fp)
{
    const char *sep = NULL;
    overDef *od;

    for (od = overs; od != NULL; od = od->next)
    {
        int need_sec;

        if (!overloadHasDocstring(pt, od, md))
            continue;

        if (sep == NULL)
        {
            prcode(fp, "\"");
            sep = "\\n\"\n    \"";
        }
        else
        {
            prcode(fp, "%s", sep);
        }

        prScopedPythonName(fp, scope_scope, scope_name);

        if (scope_name != NULL)
            prcode(fp, ".");

        prcode(fp, "%s", md->pyname->text);
        need_sec = prPythonSignature(pt, fp, &od->pysig, FALSE, TRUE, TRUE,
                TRUE, FALSE);

        if (need_sec)
        {
            prcode(fp, "%s", sep);

            prScopedPythonName(fp, scope_scope, scope_name);

            if (scope_name != NULL)
                prcode(fp, ".");

            prcode(fp, "%s", md->pyname->text);
            prPythonSignature(pt, fp, &od->pysig, TRUE, TRUE, TRUE, TRUE,
                    FALSE);
        }
    }

    if (sep != NULL)
        prcode(fp, "\"");
}


/*
 * Return TRUE if a docstring can be automatically generated for a class
 * overload.
 */
static int overloadHasClassDocstring(sipSpec *pt, ctorDef *ct)
{
    if (isPrivateCtor(ct))
        return FALSE;

    /* If it is versioned then make sure it is the default API. */
    return isDefaultAPI(pt, ct->api_range);
}


/*
 * Return TRUE if a docstring can be automatically generated for a class.
 */
static int hasClassDocstring(sipSpec *pt, classDef *cd)
{
    ctorDef *ct;

    if (!canCreate(cd))
        return FALSE;

    if (!isDefaultAPI(pt, cd->iff->api_range))
        return FALSE;

    for (ct = cd->ctors; ct != NULL; ct = ct->next)
        if (overloadHasClassDocstring(pt, ct))
            return TRUE;

    return FALSE;
}


/*
 * Generate the docstring for a class.
 */
static void generateClassDocstring(sipSpec *pt, classDef *cd, FILE *fp)
{
    const char *sep = NULL;
    ctorDef *ct;

    for (ct = cd->ctors; ct != NULL; ct = ct->next)
    {
        int need_sec;

        if (!overloadHasClassDocstring(pt, ct))
            continue;

        if (sep == NULL)
        {
            fprintf(fp, "\"\\1");
            sep = "\\n\"\n    \"";
        }
        else
        {
            fprintf(fp, "%s", sep);
        }

        prScopedPythonName(fp, cd->ecd, cd->pyname->text);
        need_sec = prPythonSignature(pt, fp, &ct->pysig, FALSE, TRUE, TRUE,
                TRUE, FALSE);
        ++currentLineNr;

        if (need_sec)
        {
            fprintf(fp, "%s", sep);

            prScopedPythonName(fp, cd->ecd, cd->pyname->text);
            prPythonSignature(pt, fp, &ct->pysig, TRUE, TRUE, TRUE, TRUE,
                    FALSE);
            ++currentLineNr;
        }
    }

    if (sep != NULL)
        fprintf(fp, "\"");
}


/*
 * Returns TRUE if the given API version corresponds to the default.
 */
static int isDefaultAPI(sipSpec *pt, apiVersionRangeDef *avd)
{
    int def_api;

    /* Handle the trivial case. */
    if (avd == NULL)
        return TRUE;

    def_api = findAPI(pt, avd->api_name->text)->from;

    if (avd->from > 0 && avd->from > def_api)
        return FALSE;

    if (avd->to > 0 && avd->to <= def_api)
        return FALSE;

    return TRUE;
}


/*
 * Generate an explicit docstring.
 */
static void generateExplicitDocstring(codeBlockList *cbl, FILE *fp)
{
    const char *sep = NULL;

    while (cbl != NULL)
    {
        const char *cp;

        if (sep == NULL)
        {
            prcode(fp, "\"");
            sep = "\\n\"\n    \"";
        }
        else
        {
            prcode(fp, "%s", sep);
        }

        for (cp = cbl->block->frag; *cp != '\0'; ++cp)
        {
            if (*cp == '\n')
            {
                /* Ignore if this is the last character of the fragment. */
                if (cp[1] != '\0')
                    prcode(fp, "%s", sep);
            }
            else
            {
                if (*cp == '\\' || *cp == '\"')
                    prcode(fp, "\\");

                prcode(fp, "%c", *cp);
            }
        }

        cbl = cbl->next;
    }

    if (sep != NULL)
        prcode(fp, "\"");
}


/*
 * Generate the definition of a module's optional docstring.
 */
static void generateModDocstring(moduleDef *mod, FILE *fp)
{
    if (mod->docstring != NULL)
    {
        prcode(fp,
"\n"
"PyDoc_STRVAR(doc_mod_%s, ", mod->name);

        generateExplicitDocstring(mod->docstring, fp);

        prcode(fp, ");\n"
            );
    }
}


/*
 * Generate a void* cast for an argument if needed.
 */
static void generateVoidPtrCast(argDef *ad, FILE *fp)
{
    /*
     * Generate a cast if the argument's type was a typedef.  This allows us to
     * use typedef's to void* to hide something more complex that we don't
     * handle.
     */
    if (ad->original_type != NULL)
        prcode(fp, "(%svoid *)", (isConstArg(ad) ? "const " : ""));
}
